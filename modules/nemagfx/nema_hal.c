/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <hal/nrf_gpu.h>

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>

#include <nema_hal.h>
#include <nema_hal_ext.h>
#include <nema_shim.h>
#include <nema_core.h>
#include <nema_error.h>

LOG_MODULE_REGISTER(nema_hal, CONFIG_NEMA_LOG_LEVEL);

#define GPU_IRQn   115

#define NEMA_REGS_BASE         (uintptr_t)(NRF_GPU_NS_BASE + 0x1000)
#define NEMA_HEAP_START_ADDR   DT_REG_ADDR(DT_NODELABEL(graphic_memory))
#define NEMA_HEAP_SIZE         DT_REG_SIZE(DT_NODELABEL(graphic_memory))
#define NEMA_RING_SIZE         1024

static struct k_mutex nema_mutex_list[MUTEX_MAX];
static struct k_heap nema_heap;

static int last_cl_id = -1;

static nema_ringbuffer_t ring_buffer_str = {{0}};

static K_SEM_DEFINE(gpu_irq_flag, 0, 1);

static gpu_event_cb_t gpu_cb = NULL;
static void *gpu_pdata = NULL;

void nema_register_callback(gpu_event_cb_t cb, void *pdata)
{
	gpu_cb = cb;
	gpu_pdata = pdata;
}

uint32_t nema_reg_read(uint32_t reg)
{
	return sys_read32(NEMA_REGS_BASE + reg);
}

void nema_reg_write(uint32_t reg, uint32_t value)
{
	sys_write32(value, NEMA_REGS_BASE + reg);
}

void GPU_IRQHandler(void)
{
	if (nrf_gpu_event_check(NRF_GPU, NRF_GPU_EVENT_CORE)) {
		if (gpu_cb != NULL) {
			gpu_cb(NRF_GPU_EVENT_CORE, gpu_pdata);
		}
		nema_irq_reset();
		nrf_gpu_event_clear(NRF_GPU, NRF_GPU_EVENT_CORE);

		k_sem_give(&gpu_irq_flag);
	}

	if (nrf_gpu_event_check(NRF_GPU, NRF_GPU_EVENT_SYS_ERROR)) {
		if (gpu_cb != NULL) {
			gpu_cb(NRF_GPU_EVENT_SYS_ERROR, gpu_pdata);
		}
		nema_sysirq_reset();
		nrf_gpu_event_clear(NRF_GPU, NRF_GPU_EVENT_SYS_ERROR);
	}
}

int32_t nema_sys_init(void)
{
	int ret = 0;

	nrf_gpu_enable(NRF_GPU);

	nema_irq_reset();
	nema_sysirq_reset();

	nrf_gpu_event_clear(NRF_GPU, NRF_GPU_EVENT_CORE);
	nrf_gpu_event_clear(NRF_GPU, NRF_GPU_EVENT_SYS_ERROR);

	nrf_gpu_int_enable(NRF_GPU, NRF_GPU_INT_CORE_MASK | NRF_GPU_INT_SYS_ERROR_MASK);

	irq_enable(GPU_IRQn);

	k_sem_take(&gpu_irq_flag, K_NO_WAIT);

	/* Initialize Graphics Memory */
	k_heap_init(&nema_heap, (void*)NEMA_HEAP_START_ADDR, NEMA_HEAP_SIZE);

	for (size_t i = 0; i < MUTEX_MAX; i++) {
		k_mutex_init(&nema_mutex_list[i]);
	}

	/* Allocate ring_buffer memory */
	ring_buffer_str.bo = nema_buffer_create(NEMA_RING_SIZE);
	(void)nema_buffer_map(&ring_buffer_str.bo);

	/* Initialize Ring Buffer */
	ret = nema_rb_init(&ring_buffer_str, 1);
	if (ret) {
		LOG_ERR("NemaGFX Ring buffer initialization failed!");
		return ret;
	}

	last_cl_id = -1;

	return 0;
}

int nema_wait_irq(void)
{
	k_sem_take(&gpu_irq_flag, K_FOREVER);

	/* While running nemap_integration test switching order in which
	 * interrupts were cleared lead to errors. Track this issue in future.
	 */
	nema_irq_reset();
	nema_sysirq_reset();

	nrf_gpu_event_clear(NRF_GPU, NRF_GPU_EVENT_CORE);
	nrf_gpu_event_clear(NRF_GPU, NRF_GPU_EVENT_SYS_ERROR);

	return 0;
}

int nema_wait_irq_cl(int cl_id)
{
	while (last_cl_id < cl_id) {
		nema_wait_irq();
		last_cl_id = nema_get_clid();
	}

	return 0;
}

nema_buffer_t nema_buffer_create(int size)
{
	nema_buffer_t bo;

	nema_mutex_lock(MUTEX_MALLOC);

	bo.base_virt = k_heap_alloc(&nema_heap, size, K_NO_WAIT);
	bo.base_phys = (uint32_t) bo.base_virt;

	if (bo.base_virt == NULL) {
		LOG_ERR("Memory allocation for graphics item failed!");
	}

	bo.size = size;
	bo.fd = 0;

	nema_mutex_unlock(MUTEX_MALLOC);

	return bo;
}

nema_buffer_t nema_buffer_create_pool(int pool, int size)
{
	ARG_UNUSED(pool);
	return nema_buffer_create(size);
}

void *nema_buffer_map(nema_buffer_t *bo)
{
	if (bo == NULL) {
		nema_error_set(NEMA_ERR_INVALID_BO);
		LOG_ERR("NemaGFX Invalid buffer pointer!");
		return NULL;
	}

	return bo->base_virt;
}

void nema_buffer_unmap(nema_buffer_t *bo)
{
	/* Usually left blank */
	ARG_UNUSED(bo);
}

void nema_buffer_destroy(nema_buffer_t *bo)
{
	if (bo == NULL) {
		nema_error_set(NEMA_ERR_INVALID_BO);
		LOG_ERR("NemaGFX Invalid buffer pointer!");
		return;
	}

	nema_mutex_lock(MUTEX_MALLOC);

	k_free(bo->base_virt);

	bo->base_virt = NULL;
	bo->base_phys = 0;
	bo->size = 0;
	bo->fd = -1;

	nema_mutex_unlock(MUTEX_MALLOC);
}

uintptr_t nema_buffer_phys(nema_buffer_t *bo)
{
	if (bo == NULL) {
		nema_error_set(NEMA_ERR_INVALID_BO);
		LOG_ERR("NemaGFX Invalid buffer pointer!");
		return -1;
	}

	return bo->base_phys;
}

void nema_buffer_flush(nema_buffer_t *bo)
{
	/* Usually left blank */
	ARG_UNUSED(bo);
}

void *nema_host_malloc(size_t size)
{
	nema_mutex_lock(MUTEX_MALLOC);
	void *ptr = k_malloc(size);
	nema_mutex_unlock(MUTEX_MALLOC);

	return ptr;
}

void nema_host_free(void *ptr)
{
	nema_mutex_lock(MUTEX_MALLOC);
	k_free(ptr);
	nema_mutex_unlock(MUTEX_MALLOC);
}

int nema_mutex_lock(int mutex_id)
{
	k_mutex_lock(&nema_mutex_list[mutex_id], K_FOREVER);

	return 0;
}

int nema_mutex_unlock(int mutex_id)
{
	k_mutex_unlock(&nema_mutex_list[mutex_id]);

	return 0;
}

static int nemagfx_sys_init(void)
{
	LOG_DBG("NemaGFX Init");

	if (nema_init() != 0) {
		LOG_ERR("NemaGFX initialization failed!");
		return -ENOBUFS;
	}

	return 0;
}

SYS_INIT(nemagfx_sys_init, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);
