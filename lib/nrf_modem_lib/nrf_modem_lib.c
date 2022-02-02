/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <init.h>
#include <device.h>
#include <zephyr.h>
#include <zephyr/types.h>
#include <nrfx_ipc.h>
#include <nrf_modem.h>
#include <nrf_modem_platform.h>
#include <pm_config.h>
#include <nrf_modem_at.h>
#include <logging/log.h>

LOG_MODULE_DECLARE(nrf_modem_lib, CONFIG_NRF_MODEM_LIB_LOG_LEVEL);

#ifdef CONFIG_LTE_LINK_CONTROL
#include <modem/lte_lc.h>
#endif

#ifndef CONFIG_TRUSTED_EXECUTION_NONSECURE
#error  nrf_modem_lib must be run as non-secure firmware.\
	Are you building for the correct board ?
#endif

struct shutdown_thread {
	sys_snode_t node;
	struct k_sem sem;
};

static sys_slist_t shutdown_threads;
static bool first_time_init;
static struct k_mutex slist_mutex;

static int init_ret;

static const nrf_modem_init_params_t init_params = {
	.ipc_irq_prio = NRF_MODEM_NETWORK_IRQ_PRIORITY,
	.shmem.ctrl = {
		.base = PM_NRF_MODEM_LIB_CTRL_ADDRESS,
		.size = CONFIG_NRF_MODEM_LIB_SHMEM_CTRL_SIZE,
	},
	.shmem.tx = {
		.base = PM_NRF_MODEM_LIB_TX_ADDRESS,
		.size = CONFIG_NRF_MODEM_LIB_SHMEM_TX_SIZE,
	},
	.shmem.rx = {
		.base = PM_NRF_MODEM_LIB_RX_ADDRESS,
		.size = CONFIG_NRF_MODEM_LIB_SHMEM_RX_SIZE,
	},
#if CONFIG_NRF_MODEM_LIB_TRACE_ENABLED
	.shmem.trace = {
		.base = PM_NRF_MODEM_LIB_TRACE_ADDRESS,
		.size = CONFIG_NRF_MODEM_LIB_SHMEM_TRACE_SIZE,
	},
#endif
};

static void log_fw_version_uuid(void)
{
	int err;
	char fw_version_buf[32];
	char *fw_version;
	char fw_uuid_buf[64];
	char *fw_uuid;

	err = nrf_modem_at_cmd(fw_version_buf, sizeof(fw_version_buf), "AT+CGMR");
	if (err == 0) {
		/* Get first token before '\r\n' which
		 * corresponds to FW version string.
		 */
		fw_version = strtok(fw_version_buf, "\r\n");
		LOG_INF("Modem FW version: %s", log_strdup(fw_version));
	} else {
		LOG_ERR("Unable to obtain modem FW version (ERR: %d, ERR TYPE: %d)",
			nrf_modem_at_err(err), nrf_modem_at_err_type(err));
	}

	err = nrf_modem_at_cmd(fw_uuid_buf, sizeof(fw_uuid_buf), "AT%%XMODEMUUID");
	if (err == 0) {
		/* Get first token before ' ' which
		 * corresponds to "%XMODEMUUID:", then
		 * move to the next token before '\r\n'
		 * which corresponds to the FW UUID string.
		 */
		fw_uuid = strtok(fw_uuid_buf, " ");
		fw_uuid = strtok(NULL, "\r\n");
		LOG_INF("Modem FW UUID: %s\n", log_strdup(fw_uuid));
	} else {
		LOG_ERR("Unable to obtain modem FW UUID (ERR: %d, ERR TYPE: %d)\n",
			nrf_modem_at_err(err), nrf_modem_at_err_type(err));
	}
}

static int _nrf_modem_lib_init(const struct device *unused)
{
	if (!first_time_init) {
		sys_slist_init(&shutdown_threads);
		k_mutex_init(&slist_mutex);
		first_time_init = true;
	}

	/* Setup the network IRQ used by the Modem library.
	 * Note: No call to irq_enable() here, that is done through nrf_modem_init().
	 */
	IRQ_CONNECT(NRF_MODEM_NETWORK_IRQ, NRF_MODEM_NETWORK_IRQ_PRIORITY,
		    nrfx_isr, nrfx_ipc_irq_handler, 0);

	init_ret = nrf_modem_init(&init_params, NORMAL_MODE);

#if CONFIG_NRF_MODEM_LIB_LOG_FW_VERSION_UUID
	log_fw_version_uuid();
#endif

	k_mutex_lock(&slist_mutex, K_FOREVER);
	if (sys_slist_peek_head(&shutdown_threads) != NULL) {
		struct shutdown_thread *thread, *next_thread;

		/* Wake up all sleeping threads. */
		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&shutdown_threads, thread,
					     next_thread, node) {
			k_sem_give(&thread->sem);
		}
	}
	k_mutex_unlock(&slist_mutex);

	if (IS_ENABLED(CONFIG_NRF_MODEM_LIB_SYS_INIT)) {
		/* nrf_modem_init() returns values from a different namespace
		 * than Zephyr's. Make sure to return something in Zephyr's
		 * namespace, in this case 0, when called during SYS_INIT.
		 * Non-zero values in SYS_INIT are currently ignored.
		 */
		return 0;
	}

	return init_ret;
}

void nrf_modem_lib_shutdown_wait(void)
{
	struct shutdown_thread thread;

	k_sem_init(&thread.sem, 0, 1);

	k_mutex_lock(&slist_mutex, K_FOREVER);
	sys_slist_append(&shutdown_threads, &thread.node);
	k_mutex_unlock(&slist_mutex);

	(void)k_sem_take(&thread.sem, K_FOREVER);

	k_mutex_lock(&slist_mutex, K_FOREVER);
	sys_slist_find_and_remove(&shutdown_threads, &thread.node);
	k_mutex_unlock(&slist_mutex);
}

int nrf_modem_lib_init(enum nrf_modem_mode_t mode)
{
	if (mode == NORMAL_MODE) {
		return _nrf_modem_lib_init(NULL);
	} else {
		return nrf_modem_init(&init_params, FULL_DFU_MODE);
	}
}

int nrf_modem_lib_get_init_ret(void)
{
	return init_ret;
}

int nrf_modem_lib_shutdown(void)
{
#ifdef CONFIG_LTE_LINK_CONTROL
	lte_lc_deinit();
#endif
	nrf_modem_shutdown();

	return 0;
}

#if defined(CONFIG_NRF_MODEM_LIB_SYS_INIT)
/* Initialize during SYS_INIT */
SYS_INIT(_nrf_modem_lib_init, POST_KERNEL, 0);
#endif
