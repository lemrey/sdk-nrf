/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>
#include <stdio.h>
#include <string.h>
#include <nrf_modem.h>
#include <modem/nrf_modem_lib.h>
#include <net/fota_download.h>
#include <nrf_modem_at.h>
#include "update.h"

#if defined(CONFIG_LWM2M_CARRIER)
#include <lwm2m_carrier.h>
#endif

#define FOTA_TEST "FOTA-TEST"

static K_SEM_DEFINE(started_application, 0, 1);

NRF_MODEM_LIB_ON_INIT(modem_delta_update_init_hook,
		      on_modem_lib_init, NULL);

/* Initialized to value different than success (0) */
static int modem_lib_init_result = -1;

static void on_modem_lib_init(int ret, void *ctx)
{
	modem_lib_init_result = ret;
}

static char version[256];

static bool is_test_firmware(void)
{
	static bool version_read;
	int err;

	if (!version_read) {
		err = nrf_modem_at_cmd(version, sizeof(version), "AT+CGMR");
		if (err != 0) {
			printk("Unable to read modem version: %d\n", err);
			return false;
		}

		if (strstr(version, CONFIG_SUPPORTED_BASE_VERSION) == NULL) {
			printk("Unsupported base modem version: %s\n", version);
			printk("Supported base version (set in prj.conf): %s\n",
			       CONFIG_SUPPORTED_BASE_VERSION);
			return false;
		}

		version_read = true;
	}

	return strstr(version, FOTA_TEST) != NULL;
}

static const char *get_file(void)
{
	if (is_test_firmware()) {
		return CONFIG_DOWNLOAD_FILE_FOTA_TEST_TO_BASE;
	} else {
		return CONFIG_DOWNLOAD_FILE_BASE_TO_FOTA_TEST;
	}
}

void fota_dl_handler(const struct fota_download_evt *evt)
{
	switch (evt->id) {
	case FOTA_DOWNLOAD_EVT_ERROR:
		printk("Received error from fota_download\n");
		update_sample_stop();
		break;

	case FOTA_DOWNLOAD_EVT_FINISHED:
		update_sample_done();
		printk("Press 'Reset' button or enter 'reset' to apply new firmware\n");
		break;

	default:
		break;
	}
}

static int num_leds(void)
{
	if (is_test_firmware()) {
		return 2;
	} else {
		return 1;
	}
}

#if defined(CONFIG_LWM2M_CARRIER)
void nrf_modem_recoverable_error_handler(uint32_t err)
{
	printk("Modem library recoverable error: %u\n", (unsigned int)err);
}

static void lte_event_handler(const struct lte_lc_evt *const evt)
{
	/* This event handler is not in use here. */
	ARG_UNUSED(evt);
}

void print_err(const lwm2m_carrier_event_t *evt)
{
	const lwm2m_carrier_event_error_t *err = evt->data.error;

	static const char *strerr[] = {
		[LWM2M_CARRIER_ERROR_NO_ERROR] =
			"No error",
		[LWM2M_CARRIER_ERROR_BOOTSTRAP] =
			"Bootstrap error",
		[LWM2M_CARRIER_ERROR_LTE_LINK_UP_FAIL] =
			"Failed to connect to the LTE network",
		[LWM2M_CARRIER_ERROR_LTE_LINK_DOWN_FAIL] =
			"Failed to disconnect from the LTE network",
		[LWM2M_CARRIER_ERROR_FOTA_PKG] =
			"Package refused from modem",
		[LWM2M_CARRIER_ERROR_FOTA_PROTO] =
			"Protocol error",
		[LWM2M_CARRIER_ERROR_FOTA_CONN] =
			"Connection to remote server failed",
		[LWM2M_CARRIER_ERROR_FOTA_CONN_LOST] =
			"Connection to remote server lost",
		[LWM2M_CARRIER_ERROR_FOTA_FAIL] =
			"Modem firmware update failed",
		[LWM2M_CARRIER_ERROR_CONFIGURATION] =
			"Illegal object configuration detected",
		[LWM2M_CARRIER_ERROR_INIT] =
			"Initialization failure",
		[LWM2M_CARRIER_ERROR_INTERNAL] =
			"Internal failure",
	};

	printk("%s, reason %d\n", strerr[err->type], err->value);
}

void print_deferred(const lwm2m_carrier_event_t *evt)
{
	const lwm2m_carrier_event_deferred_t *def = evt->data.deferred;

	static const char *strdef[] = {
		[LWM2M_CARRIER_DEFERRED_NO_REASON] =
			"No reason given",
		[LWM2M_CARRIER_DEFERRED_PDN_ACTIVATE] =
			"Failed to activate PDN",
		[LWM2M_CARRIER_DEFERRED_BOOTSTRAP_NO_ROUTE] =
			"No route to bootstrap server",
		[LWM2M_CARRIER_DEFERRED_BOOTSTRAP_CONNECT] =
			"Failed to connect to bootstrap server",
		[LWM2M_CARRIER_DEFERRED_BOOTSTRAP_SEQUENCE] =
			"Bootstrap sequence not completed",
		[LWM2M_CARRIER_DEFERRED_SERVER_NO_ROUTE] =
			"No route to server",
		[LWM2M_CARRIER_DEFERRED_SERVER_CONNECT] =
			"Failed to connect to server",
		[LWM2M_CARRIER_DEFERRED_SERVER_REGISTRATION] =
			"Server registration sequence not completed",
		[LWM2M_CARRIER_DEFERRED_SERVICE_UNAVAILABLE] =
			"Server in maintenance mode",
		[LWM2M_CARRIER_DEFERRED_SIM_MSISDN] =
			"Waiting for SIM MSISDN",
	};

	printk("Reason: %s, timeout: %d seconds\n", strdef[def->reason],
		def->timeout);
}

int lwm2m_carrier_event_handler(const lwm2m_carrier_event_t *event)
{
	int err = 0;

	switch (event->type) {
	case LWM2M_CARRIER_EVENT_INIT:
		printk("LWM2M_CARRIER_EVENT_INIT\n");
		err = lte_lc_init();
		lte_lc_register_handler(lte_event_handler);
		break;
	case LWM2M_CARRIER_EVENT_LTE_LINK_UP:
		printk("LWM2M_CARRIER_EVENT_LTE_LINK_UP\n");
		err = lte_lc_connect_async(NULL);
		break;
	case LWM2M_CARRIER_EVENT_LTE_LINK_DOWN:
		printk("LWM2M_CARRIER_EVENT_LTE_LINK_DOWN\n");
		err = lte_lc_offline();
		break;
	case LWM2M_CARRIER_EVENT_LTE_POWER_OFF:
		printk("LWM2M_CARRIER_EVENT_LTE_POWER_OFF\n");
		err = lte_lc_power_off();
		break;
	case LWM2M_CARRIER_EVENT_BOOTSTRAPPED:
		printk("LWM2M_CARRIER_EVENT_BOOTSTRAPPED\n");
		break;
	case LWM2M_CARRIER_EVENT_REGISTERED:
		printk("LWM2M_CARRIER_EVENT_REGISTERED\n");
		break;
	case LWM2M_CARRIER_EVENT_DEFERRED:
		printk("LWM2M_CARRIER_EVENT_DEFERRED\n");
		print_deferred(event);
		break;
	case LWM2M_CARRIER_EVENT_FOTA_START:
		printk("LWM2M_CARRIER_EVENT_FOTA_START\n");
		break;
	case LWM2M_CARRIER_EVENT_REBOOT:
		printk("LWM2M_CARRIER_EVENT_REBOOT\n");
		break;
	case LWM2M_CARRIER_EVENT_ERROR:
		printk("LWM2M_CARRIER_EVENT_ERROR\n");
		print_err(event);
		break;
	}

	return err;
}
#endif /* CONFIG_LWM2M_CARRIER */

void main(void)
{
	int err;

	printk("HTTP delta modem update sample started\n");

#if !defined(CONFIG_LWM2M_CARRIER)
	printk("Initializing modem library\n");
#if !defined(CONFIG_NRF_MODEM_LIB_SYS_INIT)
	err = nrf_modem_lib_init(NORMAL_MODE);
#else
	/* If nrf_modem_lib is initialized on post-kernel we should
	 * fetch the returned error code instead of nrf_modem_lib_init
	 */
	err = modem_lib_init_result;
#endif /* CONFIG_NRF_MODEM_LIB_SYS_INIT */
	switch (err) {
	case MODEM_DFU_RESULT_OK:
		printk("Modem firmware update successful!\n");
		printk("Modem will run the new firmware after reboot\n");
		printk("Press 'Reset' button or enter 'reset' to apply new firmware\n");
		k_thread_suspend(k_current_get());
		break;
	case MODEM_DFU_RESULT_UUID_ERROR:
	case MODEM_DFU_RESULT_AUTH_ERROR:
		printk("Modem firmware update failed\n");
		printk("Modem will run non-updated firmware on reboot.\n");
		printk("Press 'Reset' button or enter 'reset'\n");
		break;
	case MODEM_DFU_RESULT_HARDWARE_ERROR:
	case MODEM_DFU_RESULT_INTERNAL_ERROR:
		printk("Modem firmware update failed\n");
		printk("Fatal error.\n");
		break;
	case -1:
		printk("Could not initialize momdem library.\n");
		printk("Fatal error.\n");
		return;
	default:
		break;
	}
	printk("Initialized modem library\n");
#endif /* CONFIG_LWM2M_CARRIER */

	err = fota_download_init(fota_dl_handler);
	if (err != 0) {
		printk("fota_download_init() failed, err %d\n", err);
		return;
	}

	k_sem_take(&started_application, K_SECONDS(1));

	err = update_sample_init(&(struct update_sample_init_params){
					.update_start = update_sample_start,
					.num_leds = num_leds(),
					.filename = get_file()
				});
	if (err != 0) {
		printk("update_sample_init() failed, err %d\n", err);
		return;
	}

	printk("Current modem firmware version: %s\n", version);

	printk("Press Button 1 for enter 'download' to download modem delta update\n");
}
