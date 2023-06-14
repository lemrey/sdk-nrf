/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/ztest.h>

#include <app_event_manager.h>
#include <event_manager_proxy.h>

#include "test_config.h"
#include "common_utils.h"
#include "test_events.h"

#define MODULE test_init

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE);

#if DT_NODE_EXISTS(DT_NODELABEL(ppr_code))
#define PPR_START_ADDR DT_REG_ADDR(DT_NODELABEL(ppr_code))
#include <hal/nrf_vpr.h>

static int vpr_initialization(void)
{
	/* Enable PPR core */
	nrf_vpr_initpc_set(NRF_VPR130, PPR_START_ADDR);
	nrf_vpr_cpurun_set(NRF_VPR130, true);

	return 0;
}

SYS_INIT(vpr_initialization, APPLICATION, CONFIG_APP_SYSTEM_INIT_PRIO);
#endif

static int test_event_manager_proxy_init(void)
{
	int ret;
	const struct device *ipc_instance  = REMOTE_IPC_DEV;

	ret = app_event_manager_init();
	__ASSERT(ret >= 0, "Event manager init failed (%d)", ret);

	ret = event_manager_proxy_add_remote(ipc_instance);
	__ASSERT(ret >= 0, "Cannot add remote (%d)", ret);

	REMOTE_EVENT_SUBSCRIBE(ipc_instance, test_end_remote_event);
	REMOTE_EVENT_SUBSCRIBE(ipc_instance, test_start_ack_event);

	return 0;
}

SYS_INIT(test_event_manager_proxy_init, APPLICATION, CONFIG_APP_PROXY_INIT_PRIO);

static int test_event_manager_proxy_start(void)
{
	int ret;

	ret = event_manager_proxy_start();
	__ASSERT(ret >= 0, "Cannot start event manager proxy (%d)", ret);

	ret = event_manager_proxy_wait_for_remotes(K_SECONDS(REMOTE_START_TIMEOUT_S));
	__ASSERT(ret >= 0, "Error when waiting for remote (%d)", ret);

	return 0;
}

SYS_INIT(test_event_manager_proxy_start, APPLICATION, CONFIG_APP_PROXY_START_PRIO);
