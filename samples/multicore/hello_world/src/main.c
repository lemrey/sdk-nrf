/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <sys/printk.h>

#if defined(CONFIG_BOARD_NRF54FPGA_NRF5420_CPUAPP) || \
    defined(CONFIG_BOARD_NRF54FPGA_NRF5420_SOC1_CPUAPP)
#define PPR_START_ADDR 0x1E09A000
#include <hal/nrf_vpr.h>
#endif

static void timer_cb(struct k_timer *timer)
{
	printk("In GRTC irq\n");
}

#ifdef NRF_PPR
#define COOP_THREAD_STACK_SIZE 256
#define COOP_THREAD_PRIO (-1)
static struct k_thread coop_thread_data;
static K_THREAD_STACK_DEFINE(coop_thread_stack, COOP_THREAD_STACK_SIZE);

static void coop_thread_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		k_yield();
	}
}
#endif

int main(void)
{
#if defined(CONFIG_BOARD_NRF54FPGA_NRF5420_CPUAPP) || \
    defined(CONFIG_BOARD_NRF54FPGA_NRF5420_SOC1_CPUAPP)
	/* Enable PPr core */
	nrf_vpr_initpc_set(NRF_VPR130, PPR_START_ADDR);
	nrf_vpr_cpurun_set(NRF_VPR130, true);
#endif
	static struct k_timer timer;

	k_timer_init(&timer, timer_cb, NULL);
	k_timer_start(&timer, K_MSEC(10), K_MSEC(1000));

#ifdef NRF_PPR
	k_thread_create(&coop_thread_data, coop_thread_stack,
			K_THREAD_STACK_SIZEOF(coop_thread_stack),
			coop_thread_fn, NULL, NULL, NULL,
			COOP_THREAD_PRIO,
			0, K_NO_WAIT);
#endif

	while (1) {
		printk("Hello world from %s\n", CONFIG_BOARD);
		k_msleep(1000);
	}

	return 0;
}
