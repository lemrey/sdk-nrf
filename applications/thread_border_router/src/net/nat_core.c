/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief NA(P)T core functionality for IPv4
 */

#include <zephyr/net/net_pkt_filter.h>
#include <zephyr/net/net_ip.h>
#include "nat_proto.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nat_core, CONFIG_NRF_TBR_NAT_LOG_LEVEL);

#define VALID_NAT_PROTO(_pkt) (NET_IPV4_HDR(_pkt)->proto != IPPROTO_ICMP   \
			       && NET_IPV4_HDR(_pkt)->proto != IPPROTO_TCP \
			       && NET_IPV4_HDR(_pkt)->proto != IPPROTO_UDP)

struct nat_test {
	struct npf_test test;
};

static struct net_mgmt_event_callback nat_conn_mgmt_cb;

static bool nat_rx_cb(struct npf_test *test, struct net_pkt *pkt);
static bool nat_tx_cb(struct npf_test *test, struct net_pkt *pkt);

static struct nat_test nat_rule_rx = {
	.test.fn = nat_rx_cb
};

static struct nat_test nat_rule_tx = {
	.test.fn = nat_tx_cb
};

static NPF_RULE(nat_rx, NET_DROP, nat_rule_rx);
static NPF_RULE(nat_tx, NET_OK, nat_rule_tx);

static bool nat_rx_cb(struct npf_test *test, struct net_pkt *pkt)
{
	ARG_UNUSED(test);

	bool drop_pkt = false;

	__ASSERT_NO_MSG(pkt != NULL);

	if (net_pkt_forwarding(pkt) || VALID_NAT_PROTO(pkt)) {
		return drop_pkt;
	}

	if (!net_ipv4_is_addr_bcast(net_pkt_iface(pkt),
				    (struct in_addr *)NET_IPV4_HDR(pkt)->dst) &&
	    net_ipv4_is_my_addr((struct in_addr *)NET_IPV4_HDR(pkt)->dst)) {
		switch (NET_IPV4_HDR(pkt)->proto) {
		case IPPROTO_ICMP:
			LOG_DBG("ICMP <");
			if (nat_icmp(pkt, false) == NET_DROP) {
				drop_pkt = true;
			}
			break;
		case IPPROTO_UDP:
			LOG_DBG("UDP <");
			if (nat_udp(pkt, false) == NET_DROP) {
				drop_pkt = true;
			}
			break;
		case IPPROTO_TCP:
			LOG_DBG("TCP <");
			if (nat_tcp(pkt, false) == NET_DROP) {
				drop_pkt = true;
			}
			break;
		}

		net_pkt_set_forwarding(pkt, true);
	}

	/* Decision will be made in post-routing hook. */
	return drop_pkt;
}

static bool nat_tx_cb(struct npf_test *test, struct net_pkt *pkt)
{
	ARG_UNUSED(test);

	bool is_pkt_ok = true;

	__ASSERT_NO_MSG(pkt != NULL);

	if (net_pkt_family(pkt) != PF_INET || net_pkt_forwarding(pkt) || VALID_NAT_PROTO(pkt) ||
	    net_ipv4_is_addr_unspecified((struct in_addr *)NET_IPV4_HDR(pkt)->src)) {
		return is_pkt_ok;
	}

	switch (NET_IPV4_HDR(pkt)->proto) {
	case IPPROTO_ICMP:
		LOG_DBG("ICMP >");
		if (nat_icmp(pkt, true) == NET_DROP) {
			is_pkt_ok = false;
		}
		break;
	case IPPROTO_UDP:
		LOG_DBG("UDP >");
		if (nat_udp(pkt, true) == NET_DROP) {
			is_pkt_ok = false;
		}
		break;
	case IPPROTO_TCP:
		LOG_DBG("TCP >");
		if (nat_tcp(pkt, true) == NET_DROP) {
			is_pkt_ok = false;
		}
		break;
	}

	net_pkt_set_forwarding(pkt, true);

	/* Decision will be made in post-routing hook. */
	return is_pkt_ok;
}

static void eth_iface_down(struct net_mgmt_event_callback *cb,
			   uint32_t mgmt_event, struct net_if *iface)
{
	ARG_UNUSED(cb);
	ARG_UNUSED(iface);

	/* Remove all NAT records when connection is down. */
	nat_records_remove_all();
}

static int nat_init(void)
{
	npf_append_ipv4_recv_rule(&nat_rx);
	npf_insert_send_rule(&nat_tx);
	net_mgmt_init_event_callback(&nat_conn_mgmt_cb, eth_iface_down, NET_EVENT_IF_DOWN);
	net_mgmt_add_event_callback(&nat_conn_mgmt_cb);
	return 0;
}

SYS_INIT(nat_init, POST_KERNEL, CONFIG_NRF_TBR_NAT_INIT_PRIORITY);
