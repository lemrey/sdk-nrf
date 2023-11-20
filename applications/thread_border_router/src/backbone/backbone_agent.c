/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief nRF Thread Border Router's Backbone Agent
 */

#include "mcast_routing.h"
#include "nd_proxy.h"
#include "tbr.h"

#include <openthread/backbone_router_ftd.h>

#include <zephyr/net/openthread.h>

LOG_MODULE_DECLARE(nrf_tbr, CONFIG_NRF_TBR_LOG_LEVEL);

static otBackboneRouterState last_bbr_state;

static void on_thread_state_changed(otChangedFlags flags, struct openthread_context *ot_context,
				    void *user_data);

static struct openthread_state_changed_cb ot_state_chaged_cb = { .state_changed_cb =
									 on_thread_state_changed };

static void become_primary()
{
	LOG_DBG("Backbone Agent: becoming Primary BBR");

	nd_proxy_enable();
	mcast_routing_set_enabled(true);
}

static void become_secondary_or_disabled()
{
	LOG_DBG("Backbone Agent: becoming Secondary BBR or disabled");

	mcast_routing_set_enabled(false);
}

static void handle_backbone_router_state() {
	struct tbr_context *ctx = tbr_get_context();
	otBackboneRouterState new_state = otBackboneRouterGetState(ctx->ot->instance);

	if (new_state != last_bbr_state) {
		if (new_state == OT_BACKBONE_ROUTER_STATE_PRIMARY) {
			become_primary();
		} else {
			become_secondary_or_disabled();
		}
		last_bbr_state = new_state;
	}
}

static void on_thread_state_changed(otChangedFlags flags, struct openthread_context *ot_context,
				    void *user_data)
{
	ARG_UNUSED(user_data);

	if (flags & OT_CHANGED_THREAD_BACKBONE_ROUTER_STATE) {
		handle_backbone_router_state();
	}
}

void backbone_agent_init(void)
{
	struct tbr_context *ctx = tbr_get_context();
	last_bbr_state = OT_BACKBONE_ROUTER_STATE_DISABLED;

	handle_backbone_router_state();
	openthread_state_changed_cb_register(ctx->ot, &ot_state_chaged_cb);
	mcast_routing_init();
}
