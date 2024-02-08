/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <modem/nrf_modem_lib.h>

int main(void)
{
	int err;
#if 0
	static volatile int stop = 1;
	while (stop) {;}
#endif

	printk("The AT client sample started\n");
	k_sleep(K_SECONDS(10));
	printk("CAN IT WORK\n");

	err = nrf_modem_lib_init();
	if (err) {
		printk("Modem library initialization failed, error: %d\n", err);
		return 0;
	}
	printk("Ready\n");

	return 0;
}
