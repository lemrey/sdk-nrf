/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief nRF Thread Border Router's main function
 */

#include "border_agent.h"
#include "tbr.h"
#include "backbone/backbone_agent.h"
#include "platform/infra_if.h"
#include "net/mdns_server.h"

#include <ipv6.h>

#include <openthread/border_routing.h>
#include <openthread/backbone_router_ftd.h>
#include <openthread/thread.h>
#include <openthread/platform/infra_if.h>

#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/net/net_config.h>
#include <zephyr/net/net_pkt_filter.h>
#include <zephyr/net/openthread.h>

#if defined(CONFIG_WIFI_NRF700X)
#include <zephyr/net/wifi_mgmt.h>
#include <net/wifi_mgmt_ext.h>
#endif /* CONFIG_WIFI_NRF700X */

LOG_MODULE_REGISTER(nrf_tbr, CONFIG_NRF_TBR_LOG_LEVEL);

#define WIFI_MGMT_EVENTS (NET_EVENT_WIFI_CONNECT_RESULT)

#define MAC_IPV6_MCAST_BYTE0 0x33
#define MAC_IPV6_MCAST_BYTE1 0x33

K_SEM_DEFINE(ll_addr_wait, 0, 1);
K_SEM_DEFINE(run_app, 0, 1);

static struct net_mgmt_event_callback net_event_cb;
static struct tbr_context *context;

#if defined(CONFIG_WIFI_NRF700X)
static struct net_mgmt_event_callback wifi_mgmt_cb;

struct ipv6_input_test {
	struct npf_test test;
};

static bool filter_mcast_loopback(struct npf_test *test, struct net_pkt *pkt);

struct ipv6_input_test mcast_loopback_test = {
	.test.fn = filter_mcast_loopback,
};

static NPF_RULE(filter_mcast_loopback_rule, NET_OK, mcast_loopback_test);

static inline bool is_mac_addr_ipv6_mcast(struct net_eth_addr *address)
{
	return address->addr[0] == MAC_IPV6_MCAST_BYTE0 && address->addr[1] == MAC_IPV6_MCAST_BYTE1;
}

static void handle_wifi_connect_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status =
		(const struct wifi_status *) cb->info;

	if (status->status) {
		LOG_ERR("Backbone link connection request failed (%d)", status->status);
	} else {
		LOG_INF("Backbone link connected");
		k_sem_give(&run_app);
	}
}

static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb,
				    uint32_t mgmt_event, struct net_if *iface)
{
	ARG_UNUSED(iface);

	switch (mgmt_event) {
	case NET_EVENT_WIFI_CONNECT_RESULT:
		handle_wifi_connect_result(cb);
		break;
	default:
		break;
	}
}

static bool filter_mcast_loopback(struct npf_test *test, struct net_pkt *pkt)
{
	ARG_UNUSED(test);

	struct net_eth_hdr *hdr;
	struct net_linkaddr *backbone_mac_addr;

	if (!context->backbone_iface || pkt->iface != context->backbone_iface) {
		return true;
	}

	hdr = NET_ETH_HDR(pkt);
	if (!hdr) {
		LOG_WRN("Multicast loopback filter - failed to read the header");
		return false;
	}

	backbone_mac_addr = net_if_get_link_addr(context->backbone_iface);

	/* Eliminate a multicast loop by filtering out frames with a source MAC addresses
	 * within IPv6 multicast range sent back by the backbone interface (with its
	 * source address).
	 */
	return !is_mac_addr_ipv6_mcast(&hdr->dst) || memcmp(backbone_mac_addr->addr, &hdr->src,
							    sizeof(hdr->src));
}

#endif /* defined(CONFIG_WIFI_NRF700X) */

static void net_ev_cb_handler(struct net_mgmt_event_callback *cb,
			      uint32_t mgmt_event, struct net_if *iface)
{
	if (context->backbone_iface == iface) {
		struct in6_addr *addr = net_if_ipv6_get_ll(iface, NET_ADDR_PREFERRED);

		if (addr) {
			LOG_DBG("IPv6 Link-Local address ready");
			context->ll_addr = addr;
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

#if defined(CONFIG_WIFI_NRF700X)

	/* Wait for WiFi connection */
	net_mgmt_init_event_callback(&wifi_mgmt_cb,
				     wifi_mgmt_event_handler,
				     WIFI_MGMT_EVENTS);
	net_mgmt_add_event_callback(&wifi_mgmt_cb);

	npf_insert_recv_rule(&filter_mcast_loopback_rule);

	backbone_device = device_get_binding("wlan0");

#else
	backbone_device = DEVICE_DT_GET(DT_NODELABEL(enc424j600_link_board_eth));
	k_sem_give(&run_app);
#endif

	if (!device_is_ready(backbone_device)) {
		LOG_ERR("Backbone device not ready.");
		return EXIT_FAILURE;
	}

	LOG_INF("Backbone link device: %s", backbone_device->name);
	context->backbone_iface = net_if_lookup_by_dev(backbone_device);

	if (!context->backbone_iface) {
		LOG_ERR("No backbone interface");
		return EXIT_FAILURE;
	}

	net_config_init_app(net_if_get_device(context->backbone_iface),
			    "Initializing backbone interface");

	return EXIT_SUCCESS;
}

static int init_application(void)
{
#ifdef CLOCK_FEATURE_HFCLK_DIVIDE_PRESENT
	/* For now hardcode to 128MHz */
	nrfx_clock_divider_set(NRF_CLOCK_DOMAIN_HFCLK,
			       NRF_CLOCK_HFCLK_DIV_1);
#endif

	context = tbr_get_context();
	context->ll_addr = NULL;
	context->ot = openthread_get_default_context();

	infra_if_init();
	mdns_server_init();
	backbone_agent_init();

	net_mgmt_init_event_callback(&net_event_cb,
				     net_ev_cb_handler,
				     NET_EVENT_IPV6_DAD_SUCCEED);
	net_mgmt_add_event_callback(&net_event_cb);

	return init_backbone_iface();
}

static void routing_set_enabled(struct otInstance *instance, bool enabled)
{
	NET_DBG("%s border routing", enabled ? "Enabling" : "Disabling");
	otBorderRoutingSetEnabled(instance, enabled);

	NET_DBG("%sbackbone router", enabled ? "Enabling" : "Disabling");
	otBackboneRouterSetEnabled(instance, enabled);
}

static void handle_role_change(struct otInstance *instance, otChangedFlags flags)
{
	switch (otThreadGetDeviceRole(instance)) {
	case OT_DEVICE_ROLE_CHILD:
	case OT_DEVICE_ROLE_ROUTER:
	case OT_DEVICE_ROLE_LEADER:
		if (otBorderRoutingGetState(instance) ==
		    OT_BORDER_ROUTING_STATE_UNINITIALIZED) {
			otBorderRoutingInit(instance,
					    net_if_get_by_iface(context->backbone_iface),
					    context->ll_addr != NULL);
		}
		routing_set_enabled(instance, true);
		break;

	case OT_DEVICE_ROLE_DISABLED:
	case OT_DEVICE_ROLE_DETACHED:
	default:
		routing_set_enabled(instance, false);
		break;
	}
}

static void on_thread_state_changed(otChangedFlags flags, struct openthread_context *ot_context,
				    void *user_data)
{
	ARG_UNUSED(user_data);

	if (flags & OT_CHANGED_THREAD_ROLE) {
		handle_role_change(ot_context->instance, flags);
	}
}

static struct openthread_state_changed_cb ot_state_chaged_cb = { .state_changed_cb =
									 on_thread_state_changed };

int main(void)
{
	int err;
	struct mdns_server_listener_config mdns_config = { 0 };

#if defined(CONFIG_WIFI_NRF700X)
	/* Add temporary fix to prevent using Wi-Fi before WPA supplicant is ready. */
	k_sleep(K_SECONDS(1));

	err = net_mgmt(NET_REQUEST_WIFI_CONNECT_STORED, net_if_get_default(), NULL, 0);

	if (err) {
		LOG_ERR("Connecting to Wi-Fi failed. error: %d", err);
	}

#endif /* CONFIG_WIFI_NRF700X */

	k_sem_take(&run_app, K_FOREVER);

	openthread_state_changed_cb_register(context->ot, &ot_state_chaged_cb);

	if (!context->backbone_iface) {
		LOG_ERR("Backbone link interface uninitialized");
		return EXIT_FAILURE;
	}

	if (!join_all_routers_group(context->backbone_iface)) {
		return false;
	}

	/* Router solicitation can be sent before connection is alive, send it
	 * explicitly after link is set up.
	 */
	net_ipv6_send_rs(context->backbone_iface);

	otPlatInfraIfStateChanged(context->ot->instance,
				  net_if_get_by_iface(context->backbone_iface),
				  true);

	mdns_config.iface = context->backbone_iface;
	mdns_config.setup_ipv6 = true;
	mdns_config.setup_ipv4 = true;

	err = mdns_server_listener_start(&mdns_config);

	if (err) {
		LOG_ERR("Failed to start mDNS sever: %d", err);
	}

	border_agent_init();

	return EXIT_SUCCESS;
}

/* As we cannot use NET_CONFIG_AUTO_INIT option (it would pick OpenThread as the
 * main interface. We need to call net_config_init_app() before mdns_responder
 * modules gets initialized.
 */
SYS_INIT(init_application, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
