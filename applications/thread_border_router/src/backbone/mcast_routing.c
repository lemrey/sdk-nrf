/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief nRF Thread Border Router's multicast routing
 */

#include "tbr.h"
#include "mcast_routing.h"

#include <route.h>

#include <openthread/icmp6.h>
#include <openthread/backbone_router_ftd.h>

LOG_MODULE_DECLARE(nrf_tbr, CONFIG_NRF_TBR_LOG_LEVEL);

#define MCAST_ROUTE_PREFIX_LEN 128

static void add_listener(const struct otIp6Address *address)
{
	struct in6_addr addr;
	struct net_route_entry_mcast *ot_mroute;
	struct tbr_context *ctx = tbr_get_context();
	char addr_str[INET6_ADDRSTRLEN];

	/* Avoid unaligned pointer access */
	net_ipv6_addr_copy_raw(addr.s6_addr, (uint8_t *)address);
	net_addr_ntop(AF_INET6, &addr.s6_addr, addr_str, sizeof(addr_str));

	ot_mroute = net_route_mcast_add(ctx->ot->iface, &addr, MCAST_ROUTE_PREFIX_LEN);
	if (!ot_mroute) {
		LOG_ERR("Failed to add multicast route to OT iface: %s", addr_str);
		return;
	}

	if (!net_route_mcast_add(ctx->backbone_iface, &addr, MCAST_ROUTE_PREFIX_LEN)) {
		LOG_ERR("Failed to add multicast route to backbone iface: %s", addr_str);
		net_route_mcast_del(ot_mroute);
	}
}

static void remove_listener(const struct otIp6Address *address)
{
	struct in6_addr addr;
	struct net_route_entry_mcast *mcast_entry;

	/* Avoid unaligned pointer access */
	net_ipv6_addr_copy_raw(addr.s6_addr, (uint8_t *)address);

	/* Remove all multicast routes for the given address */
	mcast_entry = net_route_mcast_lookup(&addr);
	while (mcast_entry) {
		net_route_mcast_del(mcast_entry);
		mcast_entry = net_route_mcast_lookup(&addr);
	}
}

static void handle_event(void *context,
			 otBackboneRouterMulticastListenerEvent event,
			 const struct otIp6Address *address)
{
	ARG_UNUSED(context);

	switch (event)
	{
	case OT_BACKBONE_ROUTER_MULTICAST_LISTENER_ADDED:
		add_listener(address);
		break;
	case OT_BACKBONE_ROUTER_MULTICAST_LISTENER_REMOVED:
		remove_listener(address);
		break;
	}
}

void mcast_routing_init(void)
{
	struct tbr_context *ctx = tbr_get_context();

	otIcmp6SetEchoMode(ctx->ot->instance, OT_ICMP6_ECHO_HANDLER_DISABLED);
	otBackboneRouterSetMulticastListenerCallback(ctx->ot->instance, handle_event, NULL);
}

void mcast_routing_set_enabled(bool enabled)
{
	struct tbr_context *ctx = tbr_get_context();

	if (IS_ENABLED(CONFIG_NET_ROUTE_MCAST)) {
		LOG_INF("Enabling multicast routing");

		if (enabled) {
			net_if_flag_set(ctx->backbone_iface, NET_IF_FORWARD_MULTICASTS);
			net_if_flag_set(ctx->ot->iface, NET_IF_FORWARD_MULTICASTS);
		} else {
			net_if_flag_clear(ctx->backbone_iface, NET_IF_FORWARD_MULTICASTS);
			net_if_flag_clear(ctx->ot->iface, NET_IF_FORWARD_MULTICASTS);
		}
	}
}
