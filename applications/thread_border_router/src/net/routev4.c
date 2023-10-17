/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief IPv4 routing rules
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(routev4, CONFIG_NRF_TBR_ROUTEV4_LOG_LEVEL);

#include <zephyr/net/net_pkt_filter.h>
#if defined(CONFIG_OPENTHREAD_NAT64_TRANSLATOR)
#include <zephyr/net/openthread.h>
#include <openthread/nat64.h>
#endif  /* CONFIG_OPENTHREAD_NAT64_TRANSLATOR */

struct routing_test {
	struct npf_test test;
};

static bool prerouting_cb(struct npf_test *test, struct net_pkt *pkt);
static bool postrouting_cb(struct npf_test *test, struct net_pkt *pkt);

struct routing_test prerouting_rule = {
	.test.fn = prerouting_cb,
};

struct routing_test postrouting_rule = {
	.test.fn = postrouting_cb,
};

static NPF_RULE(iface_prerouting, NET_DROP, prerouting_rule);
static NPF_RULE(iface_postrouting, NET_OK, postrouting_rule);

static const char *iface2str(struct net_if *iface)
{
#if CONFIG_NRF_TBR_ROUTEV4_LOG_LEVEL >= LOG_LEVEL_DBG

#ifdef CONFIG_NET_L2_IEEE802154
	if (net_if_l2(iface) == &NET_L2_GET_NAME(IEEE802154)) {
		return "IEEE 802.15.4";
	}
#endif

#ifdef CONFIG_NET_L2_ETHERNET
	if (net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET)) {
		return "Ethernet";
	}
#endif

#ifdef CONFIG_NET_L2_VIRTUAL
	if (net_if_l2(iface) == &NET_L2_GET_NAME(VIRTUAL)) {
		return "Virtual";
	}
#endif

#ifdef CONFIG_NET_L2_PPP
	if (net_if_l2(iface) == &NET_L2_GET_NAME(PPP)) {
		return "PPP";
	}
#endif

#ifdef CONFIG_NET_L2_DUMMY
	if (net_if_l2(iface) == &NET_L2_GET_NAME(DUMMY)) {
		return "Dummy";
	}
#endif

#ifdef CONFIG_NET_L2_OPENTHREAD
	if (net_if_l2(iface) == &NET_L2_GET_NAME(OPENTHREAD)) {
		return "OpenThread";
	}
#endif

#ifdef CONFIG_NET_L2_BT
	if (net_if_l2(iface) == &NET_L2_GET_NAME(BLUETOOTH)) {
		return "Bluetooth";
	}
#endif

#ifdef CONFIG_NET_OFFLOAD
	if (net_if_is_ip_offloaded(iface)) {
		return "IP Offload";
	}
#endif

#ifdef CONFIG_NET_L2_CANBUS_RAW
	if (net_if_l2(iface) == &NET_L2_GET_NAME(CANBUS_RAW)) {
		return "CANBUS_RAW";
	}
#endif

#else
	ARG_UNUSED(iface);
#endif

	return "<unknown type>";
}

#if defined(CONFIG_OPENTHREAD_BORDER_ROUTING) && defined(CONFIG_OPENTHREAD_NAT64_TRANSLATOR)

static bool is_ot_nat64_addr_mask(const struct in_addr *addr)
{
	otIp4Cidr cidr;

	if (otNat64GetCidr(openthread_get_default_instance(), &cidr) == OT_ERROR_NONE) {
		struct in_addr cidr_netmask = {
			.s_addr = ~htonl((uint32_t)(0xffffffffLL >> cidr.mLength))
		};
		char netmask_buf[INET_ADDRSTRLEN];

		LOG_DBG("nat64 cidr netmask %s",
			net_addr_ntop(AF_INET, &cidr_netmask, netmask_buf, sizeof(netmask_buf)));

		if ((addr->s_addr & cidr_netmask.s_addr) ==
		    (cidr.mAddress.mFields.m32 & cidr_netmask.s_addr)) {
			return true;
		}
	}
	return false;
}

#endif

static struct net_if *select_iface(const struct in_addr *addr)
{
	struct net_if *iface = NULL;

#if defined(CONFIG_OPENTHREAD_BORDER_ROUTING) && defined(CONFIG_OPENTHREAD_NAT64_TRANSLATOR)

	if (is_ot_nat64_addr_mask(addr)) {
		iface = net_if_get_first_by_type(&NET_L2_GET_NAME(OPENTHREAD));
		LOG_DBG("Address matched to Thread NAT64 CIDR");
		return iface;
	}

#endif

	iface = net_if_ipv4_select_src_iface(addr);

	return iface;
}

static void pkt_clone_and_send(struct net_pkt *pkt, struct net_if *iface)
{
	struct net_pkt *pkt_c = net_pkt_clone(pkt, K_NO_WAIT);

	if (!pkt_c) {
		LOG_ERR("pkt cloning failed");
		return;
	}

	net_pkt_set_orig_iface(pkt_c, net_pkt_iface(pkt));
	net_pkt_set_iface(pkt_c, iface);

	if (CONFIG_NRF_TBR_ROUTEV4_LOG_LEVEL >= LOG_LEVEL_DBG) {
		LOG_DBG("pkt %p has been cloned %p and routed from %s to %s",
			pkt, pkt_c,
			iface2str(net_pkt_iface(pkt)),
			iface2str(net_pkt_iface(pkt_c)));
	}

	if (net_send_data(pkt_c) < 0) {
		LOG_ERR("net_send_data failed");
		net_pkt_unref(pkt_c);
	}
}

static bool prerouting_cb(struct npf_test *test, struct net_pkt *pkt)
{
	ARG_UNUSED(test);

	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(ipv4_access, struct net_ipv4_hdr);
	struct net_ipv4_hdr *hdr = (struct net_ipv4_hdr *)net_pkt_get_data(pkt, &ipv4_access);
	struct net_if *iface = select_iface((struct in_addr *)hdr->dst);

	/* Drop packet when interface is not ready. */
	if (!net_if_is_up(iface)) {
		return true;
	}

	/* An interface assigned to a packet is different than the selected one according
	 * to a destination address, hence the packet must be rerouted.
	 */
	if (iface != net_pkt_iface(pkt)) {
		if (net_ipv4_is_my_addr((struct in_addr *)hdr->dst)) {
			return false;
		}

		pkt_clone_and_send(pkt, iface);
		return true;
	}
	return false;
}

static bool postrouting_cb(struct npf_test *test, struct net_pkt *pkt)
{
	ARG_UNUSED(test);

	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(ipv4_access, struct net_ipv4_hdr);
	struct net_ipv4_hdr *hdr = (struct net_ipv4_hdr *)net_pkt_get_data(pkt, &ipv4_access);
	struct net_if *iface = select_iface((struct in_addr *)hdr->dst);

	/* Drop packet when interface is not ready. */
	if (!net_if_is_up(iface)) {
		return false;
	}

	/* An interface assigned to a packet is different than the selected one according
	 * to a destination address, hence the packet must be rerouted.
	 */
	if (iface != net_pkt_iface(pkt)) {
		if (net_ipv4_is_my_addr((struct in_addr *)hdr->dst)) {
			return true;
		}

		pkt_clone_and_send(pkt, iface);
		/* Drop packet */
		return false;
	}
	return true;
}

static int routing_init(void)
{
	LOG_DBG("Pre and post-routing initialization");
	npf_insert_ipv4_recv_rule(&iface_prerouting);
	npf_append_ipv4_recv_rule(&iface_postrouting);

	return 0;
}

SYS_INIT(routing_init, POST_KERNEL, CONFIG_NRF_TBR_ROUTE_IPV4_INIT_PRIORITY);
