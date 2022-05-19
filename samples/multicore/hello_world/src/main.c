/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <sys/printk.h>

#ifdef CONFIG_BOARD_NRF54FPGA_NRF5420_CPUAPP
#define PPR_START_ADDR 0x1E0A0000
#include <hal/nrf_vpr.h>
#endif

int main(void)
{
#ifdef CONFIG_BOARD_NRF54FPGA_NRF5420_CPUAPP
	/* Enable PPr core */
	nrf_vpr_initpc_set(NRF_VPR130, PPR_START_ADDR);
	nrf_vpr_cpurun_set(NRF_VPR130, true);
#endif

	printk("Hello world from %s\n", CONFIG_BOARD);

	return 0;
}
