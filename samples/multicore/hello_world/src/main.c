/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <sys/printk.h>

#if defined(CONFIG_BOARD_NRF54FPGA_NRF5420_CPUAPP) || \
    defined(CONFIG_BOARD_NRF54FPGA_NRF5420_SOC1_CPUAPP)
#define PPR_START_ADDR DT_REG_ADDR(DT_NODELABEL(ppr_code))
#include <hal/nrf_vpr.h>
#endif

int main(void)
{
#if defined(CONFIG_BOARD_NRF54FPGA_NRF5420_CPUAPP) || \
    defined(CONFIG_BOARD_NRF54FPGA_NRF5420_SOC1_CPUAPP)
	/* Enable PPr core */
	nrf_vpr_initpc_set(NRF_VPR130, PPR_START_ADDR);
	nrf_vpr_cpurun_set(NRF_VPR130, true);
#endif

	while (1) {
		printk("Hello world from %s\n", CONFIG_BOARD);
		k_msleep(1000);
	}

	return 0;
}
