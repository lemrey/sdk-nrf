/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <modem/at_cmd_hook.h>
#include <modem/lte_lc.h>
#include <modem/nrf_modem_lib.h>
#include <nrf_modem_at.h>

#if defined(CONFIG_AT_CMD_HOOK)
static void pdn_cfun_evt(const char *cmd, int err)
{
	int mode;
	char *mode_str;

	if (err) {
		return;
	}

	mode_str = (char *)cmd + strlen("AT+CFUN=");

	if (mode_str[0] < '0' || mode_str[0] > '9') {
		return;
	}

	mode = atoi(mode_str);

	printk("LTE mode changed to %d\n", mode);
}

AT_CMD_HOOK(pdn_athook_cfun, "AT+CFUN=", NULL, pdn_cfun_evt);
#endif /* AT_CMD_HOOK */

int main(void)
{
	int err;

	printk("Modem trace backend sample started\n");

	printk("Initializing modem library\n");
	err = nrf_modem_lib_init();
	if (err) {
		printk("Modem library initialization failed, error: %d\n", err);
		return 0;
	}

	printk("Connecting to network\n");
	err = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_NORMAL);
	if (err) {
		printk("Failed to change LTE mode, err %d\n", err);
		return 0;
	}

	k_sleep(K_SECONDS(5));

	err = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_POWER_OFF);
	if (err) {
		printk("Failed to change LTE mode, err %d\n", err);
		return 0;
	}

	nrf_modem_lib_shutdown();

	printk("Bye\n");

	return 0;
}
