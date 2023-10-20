/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Unit Tests for nRF TBR utils functions.
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest_assert.h>
#include <zephyr/types.h>
#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net/udp.h>
#include "ipv4.h"
#include "udp_internal.h"

#include "utils.h"

struct net_test_context {
	uint8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_linkaddr ll_addr;
};

static uint8_t *net_test_get_mac(const struct device *dev)
{
	struct net_test_context *context = dev->data;

	memset(context->mac_addr, 0x00, sizeof(struct net_eth_addr));
	context->mac_addr[5] = 0x53;

	return context->mac_addr;
}

static void net_test_iface_init(struct net_if *iface)
{
	uint8_t *mac = net_test_get_mac(net_if_get_device(iface));

	net_if_set_link_addr(iface, mac, 6, NET_LINK_ETHERNET);
}

static int test_send(const struct device *dev, struct net_pkt *pkt)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pkt);

	return 0;
}

int net_test_dev_init(const struct device *dev)
{
	struct net_test_context *net_test_context = dev->data;

	net_test_context = net_test_context;

	return 0;
}

struct net_test_context net_test_context_data;

static struct dummy_api net_test_if_api = {
	.iface_api.init = net_test_iface_init,
	.send = test_send,
};

#define _ETH_L2_LAYER DUMMY_L2
#define _ETH_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(DUMMY_L2)

NET_DEVICE_INIT(net_test, "net_test",
		net_test_dev_init, NULL,
		&net_test_context_data, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&net_test_if_api, _ETH_L2_LAYER, _ETH_L2_CTX_TYPE, NET_ETH_MTU);

static struct net_pkt *test_udp_pkt_build(struct net_if *iface,
					  struct in_addr *src,
					  struct in_addr *dst,
					  uint16_t src_port,
					  uint16_t dst_port)
{
	struct net_pkt *pkt;

	pkt = net_pkt_alloc_with_buffer(iface, 0, AF_INET,
					IPPROTO_UDP, K_FOREVER);
	zassert_not_null(pkt, "Out of mem");

	if (net_ipv4_create(pkt, src, dst) ||
	    net_udp_create(pkt, htons(src_port), htons(dst_port))) {
		printk("Cannot create IPv4 UDP pkt %p", pkt);
		zassert_true(0, "exiting");
	}

	net_pkt_cursor_init(pkt);
	net_ipv4_finalize(pkt, IPPROTO_UDP);

	return pkt;
}

ZTEST(test_utils_fn, test_pkt_checksum_updt)
{
	struct net_if *iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));
	struct in_addr orig_src_addr = { { { 192, 0, 2, 1 } } };
	struct in_addr new_src_addr = { { { 192, 168, 255, 1 } } };
	struct in_addr orig_dst_addr = { { { 192, 0, 2, 9 } } };
	uint16_t src_port = 1212;
	uint16_t dst_port = 8080;
	uint16_t chksum_orig;
	uint16_t chksum_exp;
	uint16_t chksum_got;
	struct net_pkt *pkt_orig;
	struct net_pkt *pkt_new;
	struct net_ipv4_hdr *ipv4_hdr;

	pkt_orig = test_udp_pkt_build(iface, &orig_src_addr, &orig_dst_addr, src_port, dst_port);
	ipv4_hdr = NET_IPV4_HDR(pkt_orig);

	if (!ipv4_hdr) {
		zassert_true(0, "exiting");
	}
	zassert_not_equal(ipv4_hdr->chksum, 0, "IPv4 checksum is not set");
	/* Store checksum from original packet */
	chksum_orig = ipv4_hdr->chksum;

	/* Use new IP address */
	pkt_new = test_udp_pkt_build(iface, &new_src_addr, &orig_dst_addr, src_port, dst_port);

	ipv4_hdr = NET_IPV4_HDR(pkt_new);
	if (!ipv4_hdr) {
		zassert_true(0, "exiting");
	}
	zassert_not_equal(ipv4_hdr->chksum, 0, "IPv4 checksum is not set");
	/* Save expected checksum */
	chksum_exp = ipv4_hdr->chksum;

	/* Set new IP address in header */
	net_ipv4_addr_copy_raw(NET_IPV4_HDR(pkt_orig)->src, new_src_addr.s4_addr);

	/* Recompute checksum for IP header. */
	chksum_got = update_chksum(chksum_orig, orig_src_addr.s4_addr,
					   new_src_addr.s4_addr, sizeof(struct in_addr));

	zassert_equal(chksum_got, chksum_exp,
		      "Mismatch between reference and calculated checksum\n");

	net_pkt_unref(pkt_orig);
	net_pkt_unref(pkt_new);
}

ZTEST_SUITE(test_utils_fn, NULL, NULL, NULL, NULL, NULL);
