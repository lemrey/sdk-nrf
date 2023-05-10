/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <modem/nrf_modem_lib.h>
#include <modem/at_cmd_hook.h>
#include <modem/lte_lc.h>

NRF_MODEM_LIB_ON_INIT(init_hook, on_modem_init, NULL);
NRF_MODEM_LIB_ON_SHUTDOWN(shutdown_hook, on_modem_shutdown, NULL);

static void on_modem_init(int ret, void *ctx)
{
	printk("> Initialized with value %d\n", ret);
}

static void on_modem_shutdown(void *ctx)
{
	printk("> Shutting down\n");
}

static void cfun_before_cb(const char *cmd)
{
	printk("> Received callback before %s is sent to the modem\n", cmd);
}

static void cfun_after_cb(const char *cmd, int err)
{
	int mode;
	char *mode_str;

	printk("> Received callback after %s was sent %s to the modem\n",
	       cmd, err ? "unsuccessfully" : "successfully");

	if (err) {
		return;
	}

	mode_str = (char *)cmd + strlen("AT+CFUN=");

	if (mode_str[0] < '0' || mode_str[0] > '9') {
		return;
	}

	mode = atoi(mode_str);

	printk("> Functional mode has changed to %d\n", mode);
}

AT_CMD_HOOK(cfun_hook, "AT+CFUN", cfun_before_cb, cfun_after_cb);

int main(void)
{
	int err;

	printk("Modem callbacks sample started\n");

	printk("Initializing modem library\n");
	err = nrf_modem_lib_init();
	if (err) {
		printk("Modem initialization failed, err %d\n", err);
		return 0;
	}

	printk("Changing functional mode\n");
	err = lte_lc_init_and_connect();
	if (err) {
		printk("lte_lc_init_and_connect() failed, err %d\n", err);
	}

	printk("Shutting down modem library\n");
	err = nrf_modem_lib_shutdown();
	if (err) {
		printk("Shutting down modem failed, err %d\n", err);
		return 0;
	}

	printk("Bye\n");

	return 0;
}
