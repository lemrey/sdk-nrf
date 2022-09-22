/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <app_event_manager.h>

#define MODULE main

#include <caf/events/module_state_event.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(MODULE);

#include "event_proxy_init.h"

#if defined(CONFIG_BOARD_NRF54H20DK_NRF54H20_CPUAPP) && \
	defined(CONFIG_APP_INCLUDE_REMOTE_IMAGE)
#define PPR_START_ADDR DT_REG_ADDR(DT_NODELABEL(ppr_code))
#include <hal/nrf_vpr.h>
#endif

void main(void)
{
#if defined(CONFIG_BOARD_NRF54H20DK_NRF54H20_CPUAPP) && \
	defined(CONFIG_APP_INCLUDE_REMOTE_IMAGE)
	/* Enable PPr core */
	nrf_vpr_initpc_set(NRF_VPR130, PPR_START_ADDR);
	nrf_vpr_cpurun_set(NRF_VPR130, true);
#endif
	int ret;

	ret = app_event_manager_init();
	if (ret) {
		LOG_ERR("Event Manager not initialized, err: %d", ret);
		__ASSERT_NO_MSG(false);
		return;
	}
	LOG_INF("Event manager initialized");

	ret = init_event_proxy("sensor_data_aggregator_event");
	if (ret) {
		LOG_ERR("Event Manager Proxy not initialized, err: %d", ret);
		__ASSERT_NO_MSG(false);
		return;
	}

	module_set_state(MODULE_STATE_READY);
}
