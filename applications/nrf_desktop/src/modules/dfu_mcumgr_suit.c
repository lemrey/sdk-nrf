/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/mgmt/mcumgr/mgmt/callbacks.h>
#include <zephyr/sys/atomic.h>

#include <app_event_manager.h>

#define MODULE dfu_mcumgr_suit
#include <caf/events/module_state_event.h>
#include <caf/events/ble_smp_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_DFU_MCUMGR_SUIT_LOG_LEVEL);

static atomic_t mcumgr_event_active = ATOMIC_INIT(false);

/* nRF Desktop MCUmgr SUIT DFU module cannot be enabled together with
 * CAF BLE SMP, Config Channel DFU or the nRF Desktop MCUmgr DFU (MCUboot) module. */
BUILD_ASSERT(!IS_ENABLED(CONFIG_CAF_BLE_SMP));
BUILD_ASSERT(!IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_DFU_ENABLE));
BUILD_ASSERT(!IS_ENABLED(CONFIG_DESKTOP_DFU_MCUMGR_ENABLE));

static int32_t smp_cmd_recv(uint32_t event,
			    int32_t rc,
			    bool *abort_more,
			    void *data,
			    size_t data_size)
{
	LOG_DBG("MCUmgr SMP Command Recv Event");

	if (IS_ENABLED(CONFIG_MCUMGR_TRANSPORT_BT) &&
	    atomic_cas(&mcumgr_event_active, false, true)) {
		APP_EVENT_SUBMIT(new_ble_smp_transfer_event());
	}

	return MGMT_ERR_EOK;
}

static struct mgmt_callback cmd_recv_cb = {
	.callback = smp_cmd_recv,
	.event_id = MGMT_EVT_OP_CMD_RECV,
};

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (IS_ENABLED(CONFIG_MCUMGR_TRANSPORT_BT) &&
	    is_ble_smp_transfer_event(aeh)) {
		bool res = atomic_cas(&mcumgr_event_active, true, false);

		__ASSERT_NO_MSG(res);
		ARG_UNUSED(res);

		return false;
	}

	if (is_module_state_event(aeh)) {
		const struct module_state_event *event =
			cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			LOG_INF("SUIT image version: %d", CONFIG_SUIT_ENVELOPE_SEQUENCE_NUM);

			mgmt_callback_register(&cmd_recv_cb);
		}
		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
#if CONFIG_MCUMGR_TRANSPORT_BT
APP_EVENT_SUBSCRIBE_FINAL(MODULE, ble_smp_transfer_event);
#endif
