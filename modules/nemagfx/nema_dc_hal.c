/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <hal/nrf_dispc.h>
#include <hal/nrf_auxpll.h>

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>

#include <nema_dc.h>
#include <nema_dc_hal.h>
#include <nema_hal_ext.h>
#include <nema_dc_shim.h>

LOG_MODULE_REGISTER(nema_dc_hal, CONFIG_NEMA_LOG_LEVEL);

#define DISPC_IRQn   112

#define NEMADC_REGS_BASE      (uintptr_t)(NRF_DISPC_NS_BASE + 0x1000)
#define NEMADC_BASE_FREQUENCY 50000000

static K_SEM_DEFINE(nemadc_irq_flag, 0, 1);

static dispc_event_cb_t dispc_cb = NULL;
static void *dispc_pdata = NULL;

void nemadc_register_callback(dispc_event_cb_t cb, void *pdata)
{
	dispc_cb = cb;
	dispc_pdata = pdata;
}

static uint16_t auxpll_freq_calc(uint32_t freq, uint32_t outsel_div)
{
	/* Function calculates the value, that must be written to
	 * AUXPLL_FREQ[15:0] to output requested output frequency (freq) with
	 * predefined output divider (outsel_div)
	 *
	 * Dependency is following:
	 *    freq * outsel_div = (R + (freq_reg / 2^16)) * 32 MHz
	 *
	 *    Where R is AUXPLL_RANGE[1:0] value, in our case equals to 4.
	 */

	uint32_t freq_reg = (freq * outsel_div / 32) - 4000000UL;
	freq_reg = (freq_reg * 256) / 1000000UL;

	return freq_reg * 256;
}

static void auxpll_init(void)
{
	uint16_t freq_found = auxpll_freq_calc(NEMADC_BASE_FREQUENCY, 3);

	nrf_auxpll_ctrl_frequency_set(NRF_AUXPLL, freq_found);
	nrf_auxpll_ctrl_outsel_set(NRF_AUXPLL, NRF_AUXPLL_CTRL_OUTSEL_DIV_3);

	nrf_auxpll_task_trigger(NRF_AUXPLL, NRF_AUXPLL_TASK_FREQ_NEW_BASE);
	nrf_auxpll_event_clear(NRF_AUXPLL, NRF_AUXPLL_TASK_FREQ_NEW_BASE);

	nrf_auxpll_task_trigger(NRF_AUXPLL, NRF_AUXPLL_TASK_START);

	while (!nrf_auxpll_event_check(NRF_AUXPLL, NRF_AUXPLL_EVENT_STARTED));

	uint16_t actual_freq = (nrf_auxpll_status_get(NRF_AUXPLL) &
				MRF_AUXPLL_STATUS_FREQUENCY_ACTUAL_MASK) >> 16;
	uint16_t expected_freq = nrf_auxpll_ctrl_frequency_get(NRF_AUXPLL);

	LOG_DBG("AUXPLL expected frequency: %d, AUXPLL actual frequency: %d",
		expected_freq, actual_freq);

	if (actual_freq != expected_freq) {
		LOG_WRN("AUXPLL frequency set up with deviation.");
	}

	nrf_auxpll_event_clear(NRF_AUXPLL, NRF_AUXPLL_EVENT_STARTED);

	LOG_DBG("AUXPLL enabled");
}

void DISPC_IRQHandler(void)
{
	if (nrf_dispc_event_check(NRF_DISPC, NRF_DISPC_EVENT_CORE)) {
		if (dispc_cb != NULL) {
			dispc_cb(NRF_DISPC_EVENT_CORE, dispc_pdata);
		}
		nemadc_irq_reset();
		nrf_dispc_event_clear(NRF_DISPC, NRF_DISPC_EVENT_CORE);
		k_sem_give(&nemadc_irq_flag);
	}
}

int32_t nemadc_sys_init(void)
{
	nrf_dispc_enable(NRF_DISPC);
	nemadc_irq_reset();
	nrf_dispc_event_clear(NRF_DISPC, NRF_DISPC_EVENT_CORE);
	nrf_dispc_int_enable(NRF_DISPC, NRF_DISPC_INT_CORE_MASK);
	irq_enable(DISPC_IRQn);
	auxpll_init();

	k_sem_take(&nemadc_irq_flag, K_NO_WAIT);

	return 0;
}

void nemadc_wait_vsync(void)
{
	nemadc_irq_set();
	k_sem_take(&nemadc_irq_flag, K_FOREVER);
}

uint32_t  nemadc_reg_read(uint32_t reg)
{
	return sys_read32(NEMADC_REGS_BASE + reg);;
}

void nemadc_reg_write(uint32_t reg, uint32_t value)
{
	sys_write32(value, NEMADC_REGS_BASE + reg);
}

static int nemagfx_dc_sys_init(void)
{
	LOG_DBG("NemaDC Init");

	if (nemadc_init() != 0) {
		LOG_ERR("NemaDC initialization failed!");
		return -ENODEV;
	}

	return 0;
}

SYS_INIT(nemagfx_dc_sys_init, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);
