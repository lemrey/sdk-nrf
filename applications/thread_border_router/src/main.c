/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief nRF Thread Border Router's main function
 */

#include "border_agent.h"

#include <ipv6.h>

#include <openthread/border_routing.h>
#include <openthread/thread.h>
#include <openthread/platform/infra_if.h>

#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/net/net_config.h>
#include <zephyr/net/openthread.h>

LOG_MODULE_REGISTER(nrf_tbr, CONFIG_NRF_TBR_LOG_LEVEL);

#if !defined(CONFIG_NET_IPV6_DAD)
#error "Duplicate address detection (DAD) must be enabled!"
#endif

K_SEM_DEFINE(ll_addr_wait, 0, 1);
K_SEM_DEFINE(run_app, 0, 1);

static struct net_mgmt_event_callback net_event_cb;

static struct {
	struct net_if *backbone_iface;
	struct in6_addr *ll_addr;
	struct openthread_context *ot;
} context;

static void net_ev_cb_handler(struct net_mgmt_event_callback *cb,
			      uint32_t mgmt_event, struct net_if *iface)
{
	if (context.backbone_iface == iface) {
		struct in6_addr *addr = net_if_ipv6_get_ll(iface, NET_ADDR_PREFERRED);

		if (addr) {
			context.ll_addr = addr;
			k_sem_give(&ll_addr_wait);
		}
	}
}

static bool join_all_routers_group(struct net_if *iface)
{
	struct in6_addr all_routers_addr;
	int ret;

	k_sem_take(&ll_addr_wait, K_FOREVER);
	net_mgmt_del_event_callback(&net_event_cb);

	net_ipv6_addr_create_ll_allrouters_mcast(&all_routers_addr);

	ret = net_ipv6_mld_join(iface, &all_routers_addr);
	if (ret < 0 && ret != -EALREADY) {
		LOG_ERR("Cannot join all-routers group (%d)", ret);
		return false;
	}

	return true;
}

static int init_backbone_iface(void)
{
	const struct device *backbone_device;

	backbone_device = DEVICE_DT_GET(DT_NODELABEL(enc424j600_link_board_eth));
	k_sem_give(&run_app);

	if (!device_is_ready(backbone_device)) {
		LOG_ERR("Backbone device not ready.");
		return EXIT_FAILURE;
	}

	LOG_INF("Backbone link device: %s", backbone_device->name);
	context.backbone_iface = net_if_lookup_by_dev(backbone_device);

	if (!context.backbone_iface) {
		LOG_ERR("No backbone interface");
		return EXIT_FAILURE;
	}

	net_config_init_app(net_if_get_device(context.backbone_iface),
			    "Initializing backbone interface");

	return EXIT_SUCCESS;
}

static bool configure_backbone_link(struct net_if *iface)
{
	if (!join_all_routers_group(iface)) {
		return false;
	}

	/* Router solicitation can be sent before connection is alive, send it
	 * explicitly after link is set up.
	 */
	net_ipv6_send_rs(iface);

	return true;
}

static void on_thread_state_changed(otChangedFlags flags, struct openthread_context *ot_context,
				    void *user_data)
{
	ARG_UNUSED(user_data);

	if (flags & OT_CHANGED_THREAD_ROLE) {
		switch (otThreadGetDeviceRole(ot_context->instance)) {
		case OT_DEVICE_ROLE_CHILD:
		case OT_DEVICE_ROLE_ROUTER:
		case OT_DEVICE_ROLE_LEADER:
			if (otBorderRoutingGetState(ot_context->instance) ==
			    OT_BORDER_ROUTING_STATE_UNINITIALIZED) {
				NET_DBG("Enabling border routing");
				otBorderRoutingInit(context.ot->instance,
						    net_if_get_by_iface(context.backbone_iface),
						    context.ll_addr != NULL);
				otBorderRoutingSetEnabled(ot_context->instance, true);
			}
			break;

		case OT_DEVICE_ROLE_DISABLED:
		case OT_DEVICE_ROLE_DETACHED:
		default:
			NET_DBG("Disabling border routing");
			otBorderRoutingSetEnabled(ot_context->instance, false);
			break;
		}
	}
}

static struct openthread_state_changed_cb ot_state_chaged_cb = { .state_changed_cb =
									 on_thread_state_changed };

int main(void)
{
	context.ll_addr = NULL;
	context.ot = openthread_get_default_context();

	net_mgmt_init_event_callback(&net_event_cb,
				     net_ev_cb_handler,
				     NET_EVENT_IPV6_DAD_SUCCEED);
	net_mgmt_add_event_callback(&net_event_cb);

	openthread_state_changed_cb_register(context.ot, &ot_state_chaged_cb);

	k_sem_take(&run_app, K_FOREVER);

	LOG_DBG("IPv6 Link-Local address ready");

	if (!configure_backbone_link(context.backbone_iface)) {
		LOG_WRN("Backbone link configuration fail");
		return EXIT_FAILURE;
	}

	openthread_set_backbone_iface(openthread_get_default_context(),
				      context.backbone_iface);

	otPlatInfraIfStateChanged(context.ot->instance, net_if_get_by_iface(context.backbone_iface),
				  true);

	border_agent_init();

	return EXIT_SUCCESS;
}

/* As we cannot use NET_CONFIG_AUTO_INIT option (it would pick OpenThread as the
 * main interface. We need to call net_config_init_app() before mdns_responder
 * modules gets initialized.
 */
SYS_INIT(init_backbone_iface, APPLICATION, 1);
