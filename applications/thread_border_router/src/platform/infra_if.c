/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "tbr.h"

#include <ipv6.h>

#include <openthread/error.h>
#include <openthread/ip6.h>
#include <openthread/border_routing.h>
#include <openthread/platform/infra_if.h>

#include <zephyr/net/ethernet.h>
#include <zephyr/net/openthread.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>

#define ICMPV6_OPTION_SRC_LL_ADDR_TYPE   1
#define ICMPV6_OPTION_SRC_LL_ADDR_LENGTH 1 /* Per RFC - in units of 8 octets */
#define PREFIX_INFINITE_LIFETIME 0xFFFFFFFF

struct icmpv6_option_header {
	uint8_t type;
	uint8_t length;
} __packed;

struct icmpv6_option_src_ll_addr {
	struct icmpv6_option_header hdr;
	struct net_eth_addr eth_addr;
} __packed;

#if defined(CONFIG_OPENTHREAD_BORDER_ROUTING)

LOG_MODULE_REGISTER(net_otPlat_infra, CONFIG_OPENTHREAD_L2_LOG_LEVEL);

static enum net_verdict handle_icmpv6_nd(struct net_pkt *pkt, struct net_ipv6_hdr *ip_hdr,
					 struct net_icmp_hdr *icmp_hdr);

static struct net_icmpv6_handler icpmv6_ra_handler = {
	.type = NET_ICMPV6_RA,
	.code = 0,
	.handler = handle_icmpv6_nd,
};

static struct net_icmpv6_handler icpmv6_na_handler = {
	.type = NET_ICMPV6_NA,
	.code = 0,
	.handler = handle_icmpv6_nd,
};

static struct net_icmpv6_handler icpmv6_rs_handler = {
	.type = NET_ICMPV6_RS,
	.code = 0,
	.handler = handle_icmpv6_nd,
};

static struct otIp6Prefix prev_onlink_prefix;

static enum net_verdict handle_icmpv6_nd(struct net_pkt *pkt, struct net_ipv6_hdr *ip_hdr,
					 struct net_icmp_hdr *icmp_hdr)
{
	struct net_pkt_cursor prev_cursor;
	struct tbr_context *tbr_ctx;
	uint16_t data_length;
	uint16_t icmp_hdr_offset;
	uint8_t *icmp_start = NULL;
	bool is_contiguous;

	tbr_ctx = tbr_get_context();

	if (!tbr_ctx->backbone_iface) {
		/* This handler must always return NET_CONTINUE to let other ICMPv6
		 * handlers process the packet.
		 */
		return NET_CONTINUE;
	}

	/* Packet's data can be splitted into multiple buffers. When this function
	 * is called the cursor is set right after the ICMPv6 header. However, OpenThread
	 * needs a contignous buffer including the header, we need to take care of that.
	 */
	net_pkt_cursor_backup(pkt, &prev_cursor);

	icmp_hdr_offset = net_pkt_get_current_offset(pkt) - sizeof(struct net_icmp_hdr);

	net_pkt_cursor_init(pkt);
	net_pkt_skip(pkt, icmp_hdr_offset);

	data_length = net_pkt_remaining_data(pkt);
	is_contiguous = net_pkt_is_contiguous(pkt, data_length);

	if (!is_contiguous) {
		struct net_pkt_data_access access = { .data = NULL,
						      .size = data_length };

		/* Allocate memory based on required space to avoid pushing IPv6 MTU
		 * to the stack.
		 */
		icmp_start = k_malloc(data_length);

		if (!icmp_start) {
			NET_WARN("Failed to allocate %u bytes for ICMPv6 message", data_length);
			goto exit;
		}

		access.data = icmp_start;

		if (!net_pkt_get_data(pkt, &access)) {
			NET_DBG("Failed to read data from the ND packet");
			goto exit;
		}
	} else {
		icmp_start = net_pkt_cursor_get_pos(pkt);
	}

	otPlatInfraIfRecvIcmp6Nd(tbr_ctx->ot->instance, net_if_get_by_iface(tbr_ctx->backbone_iface),
				 (const otIp6Address *)ip_hdr->src, icmp_start, data_length);

exit:
	net_pkt_cursor_restore(pkt, &prev_cursor);

	if (!is_contiguous && icmp_start) {
		k_free(icmp_start);
	}
	return NET_CONTINUE;
}

static void handle_local_prefix_advertisement(void)
{
	struct otIp6Prefix curr_prefix;
	struct tbr_context *tbr_ctx;
	struct in6_addr addr;

	tbr_ctx = tbr_get_context();

	otBorderRoutingGetOnLinkPrefix(tbr_ctx->ot->instance, &curr_prefix);

	if (otIp6ArePrefixesEqual(&prev_onlink_prefix, &curr_prefix)) {
		return;
	}

	/* Copy packed structure member to avoid unaligned access */
	memcpy(&addr, &prev_onlink_prefix.mPrefix, sizeof(addr));

	if (prev_onlink_prefix.mLength > 0) {
		/* On-Link prefix has changed, remove old one from backbone link and add the new one */
		net_if_ipv6_prefix_rm(tbr_ctx->backbone_iface, &addr, curr_prefix.mLength);
	}

	/* Copy packed structure member to avoid unaligned access */
	memcpy(&addr, &curr_prefix.mPrefix, sizeof(addr));

	net_if_ipv6_prefix_add(tbr_ctx->backbone_iface, &addr, curr_prefix.mLength,
			       PREFIX_INFINITE_LIFETIME);

	memcpy(&prev_onlink_prefix, &curr_prefix, sizeof(curr_prefix));
}

otError otPlatInfraIfSendIcmp6Nd(uint32_t aInfraIfIndex, const otIp6Address *aDestAddress,
				 const uint8_t *aBuffer, uint16_t aBufferLength)
{
	struct net_pkt *pkt;
	struct net_if *iface;
	struct in6_addr dst;
	const struct in6_addr *src;
	size_t pkt_len;
	struct net_linkaddr *ll_addr;
	struct icmpv6_option_src_ll_addr ll_addr_opt;

	iface = net_if_get_by_index(aInfraIfIndex);
	ll_addr = net_if_get_link_addr(iface);

	ll_addr_opt.hdr.type = ICMPV6_OPTION_SRC_LL_ADDR_TYPE;
	ll_addr_opt.hdr.length = ICMPV6_OPTION_SRC_LL_ADDR_LENGTH;

	if (aBuffer[0] == NET_ICMPV6_RA) {
		/* OpenThread sends Router Advertisement, it means Local On-Link Prefix
		 * could change.
		 */
		handle_local_prefix_advertisement();
	}

	memcpy(&ll_addr_opt.eth_addr.addr, ll_addr->addr, sizeof(struct net_eth_addr));

	pkt_len = aBufferLength + sizeof(struct icmpv6_option_src_ll_addr);
	pkt = net_pkt_alloc_with_buffer(iface, pkt_len, AF_INET6, IPPROTO_ICMPV6,
					K_FOREVER);

	if (!pkt) {
		NET_WARN("Failed to allocate packet");
		return OT_ERROR_NO_BUFS;
	}

	/* copy the address to avoid accessing address of the packed member */
	memcpy(dst.s6_addr, aDestAddress->mFields.m8, OT_IP6_ADDRESS_SIZE);

	src = net_if_ipv6_select_src_addr(iface, &dst);

	if (!src) {
		goto fail;
	}

	/* Per RFC 4861 */
	net_pkt_set_ipv6_hop_limit(pkt, NET_IPV6_ND_HOP_LIMIT);

	if (net_ipv6_create(pkt, src, &dst)) {
		goto fail;
	}

	if (net_pkt_write(pkt, aBuffer, aBufferLength) || net_pkt_write(pkt, &ll_addr_opt,
							  sizeof(ll_addr_opt))) {
		goto fail;
	}

	net_pkt_cursor_init(pkt);

	if (net_ipv6_finalize(pkt, IPPROTO_ICMPV6) || net_send_data(pkt) != NET_OK) {
		goto fail;
	}

	return OT_ERROR_NONE;

fail:
	net_pkt_unref(pkt);
	NET_ERR("Failed to send ICMPv6 ND message");
	return OT_ERROR_FAILED;
}

bool otPlatInfraIfHasAddress(uint32_t aInfraIfIndex, const otIp6Address *aAddress)
{
	struct net_if *iface;
	struct net_if_addr *if_addr;
	struct in6_addr address;

	/* copy the address to avoid accessing address of the packed member */
	memcpy(address.s6_addr, aAddress->mFields.m8, OT_IP6_ADDRESS_SIZE);

	iface = net_if_get_by_index(aInfraIfIndex);
	if_addr = net_if_ipv6_addr_lookup_by_iface(iface, &address);

	return if_addr != NULL;
}

void infra_if_init(void)
{
	memset(&prev_onlink_prefix, 0, sizeof(prev_onlink_prefix));
	net_icmpv6_register_handler(&icpmv6_ra_handler);
	net_icmpv6_register_handler(&icpmv6_na_handler);
	net_icmpv6_register_handler(&icpmv6_rs_handler);
}

#endif /* defined(CONFIG_OPENTHREAD_BORDER_ROUTING) */
