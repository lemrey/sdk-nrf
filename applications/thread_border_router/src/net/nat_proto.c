/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Protocol handlers for NAT.
 */

#include <zephyr/net/net_ip.h>
#include <zephyr/random/rand32.h>
#include <utils.h>
#include "nat_proto.h"
#include "icmpv4.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nat_proto, CONFIG_NRF_TBR_NAT_LOG_LEVEL);

#define GET_NAT_PROTO(_hdr) ((_hdr->proto == IPPROTO_ICMP) ? ICMP : PROTO_NUM_MAX)

enum nat_proto {
	ICMP = 0,
	PROTO_NUM_MAX
};

struct nataddr_in {
	struct in_addr addr;
	union {
		/** icmp */
		uint16_t query_id;
		/** tcp/udp */
		uint16_t port;
	};
};

struct nat_tuple {
	union {
		struct nataddr_in src;
		struct nataddr_in orig;
	};
	union {
		struct nataddr_in dst;
		struct nataddr_in new;
	};

	enum nat_proto proto;
};

struct nat_node {
	/** Entry list node. */
	sys_snode_t node;

	struct nat_tuple tuple;
};

K_MEM_SLAB_DEFINE_STATIC(records_pool, sizeof(struct nat_node),
			 CONFIG_NRF_TBR_NAT_RECORDS_COUNT, 4);
static sys_slist_t nat_records[PROTO_NUM_MAX];

static void modify_ipv4_hdr(struct net_ipv4_hdr *hdr, const struct in_addr *new_addr, bool src)
{
	char addr_buff[INET_ADDRSTRLEN];
	uint8_t *addr;

	__ASSERT(hdr != NULL, "Null pointer");
	__ASSERT(new_addr != NULL, "Null pointer");

	addr = src ? (uint8_t *)hdr->src : (uint8_t *)hdr->dst;

	LOG_DBG("Address translation from %s",
		net_addr_ntop(AF_INET, addr, addr_buff, sizeof(addr_buff)));

	hdr->chksum = update_chksum(hdr->chksum, addr,
				    (uint8_t *)new_addr, sizeof(struct in_addr));

	net_ipv4_addr_copy_raw(addr, (uint8_t *)new_addr);

	LOG_DBG("to %s",
		net_addr_ntop(AF_INET, new_addr, addr_buff, sizeof(addr_buff)));
}

static bool record_add(const struct nat_tuple *tuple)
{
	struct nat_node *new_node = NULL;
	char src[INET_ADDRSTRLEN];
	char dst[INET_ADDRSTRLEN];

	__ASSERT(tuple != NULL, "Null pointer");

	if (tuple->proto >= PROTO_NUM_MAX) {
		LOG_ERR("Wrong protocol");
		return false;
	}

	if (k_mem_slab_alloc(&records_pool, (void **)&new_node, K_NO_WAIT) != 0) {
		LOG_ERR("No memory!");
		return false;
	}

	memcpy(&new_node->tuple, tuple, sizeof(struct nat_tuple));

	sys_slist_append(&nat_records[tuple->proto], (sys_snode_t *)&new_node->node);

	LOG_DBG("Added tuple: [%s %u] [%s %u]",
		net_addr_ntop(AF_INET, &tuple->src.addr, src, sizeof(src)), ntohs(tuple->src.query_id),
		net_addr_ntop(AF_INET, &tuple->dst.addr, dst, sizeof(dst)), ntohs(tuple->dst.query_id));
	return true;
}

static void tuple_cpy_from_hdr(struct nat_tuple *tuple, const struct net_ipv4_hdr *hdr)
{
	__ASSERT(tuple != NULL, "Null pointer");
	__ASSERT(hdr != NULL, "Null pointer");

	tuple->proto = GET_NAT_PROTO(hdr);
	net_ipv4_addr_copy_raw((uint8_t *)&tuple->src.addr, hdr->src);
	net_ipv4_addr_copy_raw((uint8_t *)&tuple->dst.addr, hdr->dst);
}

static bool tp_hairpinning(const struct nat_tuple *tuple)
{
	__ASSERT(tuple != NULL, "Null pointer");

	if (!record_add(tuple)) {
		LOG_ERR("Cannot add record");
		return false;
	}
	return true;
}

static bool icmp_echo_resp_handle(struct nat_tuple *tuple)
{
	struct nat_node *_node, *tmp_node;
	struct nat_node *prev = NULL;
	char addr[INET_ADDRSTRLEN];

	__ASSERT(tuple != NULL, "Null pointer");

	LOG_DBG("Search record: %s:%u",
		net_addr_ntop(AF_INET, &tuple->src.addr, addr, sizeof(addr)),
		ntohs(tuple->src.query_id));

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&nat_records[ICMP], _node, tmp_node, node){
		LOG_DBG("Record: %s:%u",
			net_addr_ntop(AF_INET, &_node->tuple.dst.addr, addr, sizeof(addr)),
			ntohs(_node->tuple.new.query_id));

		if (net_ipv4_addr_cmp(&tuple->src.addr, &_node->tuple.dst.addr) &&
		    (tuple->src.query_id == _node->tuple.new.query_id)) {
			net_ipv4_addr_copy_raw((uint8_t *)&tuple->new.addr,
					       (uint8_t *)&_node->tuple.orig.addr);
			tuple->new.query_id = _node->tuple.orig.query_id;
			sys_slist_remove(&nat_records[ICMP], prev ? &prev->node : NULL,
					 &_node->node);
			k_mem_slab_free(&records_pool, (void **)&_node);

			LOG_DBG("Found, restored sender original addr %s and query_id %u",
				net_addr_ntop(AF_INET, &tuple->new.addr, addr, sizeof(addr)),
				ntohs(tuple->new.query_id));

			return true;
		}
		prev = _node;
	}

	return false;
}

enum net_verdict nat_icmp(struct net_pkt *pkt, bool tx)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(icmp_hdr_access, struct net_icmp_hdr);
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(icmp_echo_req_access,
					      struct net_icmpv4_echo_req);
	struct net_pkt_cursor ipv4_hdr_pos;
	struct net_pkt_cursor icmp_hdr_pos;
	struct net_icmp_hdr *icmp_hdr = NULL;
	struct net_icmpv4_echo_req *echo_req;
	struct nat_tuple tuple;
	const struct in_addr *new_addr = net_ipv4_unspecified_address();
	bool overwrite;

	if (!pkt) {
		return NET_DROP;
	}

	tuple_cpy_from_hdr(&tuple, NET_IPV4_HDR(pkt));

	overwrite = net_pkt_is_being_overwritten(pkt);
	net_pkt_set_overwrite(pkt, true);
	net_pkt_cursor_backup(pkt, &ipv4_hdr_pos);
	if (net_pkt_skip(pkt, net_pkt_ip_hdr_len(pkt) + net_pkt_ip_opts_len(pkt))) {
		goto drop;
	}

	icmp_hdr = (struct net_icmp_hdr *)net_pkt_get_data(pkt, &icmp_hdr_access);
	if (!icmp_hdr) {
		LOG_ERR("Failed to read ICMP header");
		goto drop;
	}

	LOG_DBG("Packet received type %d code %d", icmp_hdr->type, icmp_hdr->code);

	net_pkt_cursor_backup(pkt, &icmp_hdr_pos);
	if (net_pkt_skip(pkt, sizeof(struct net_icmp_hdr))) {
		goto drop;
	}

	switch (icmp_hdr->type) {
	case NET_ICMPV4_ECHO_REQUEST:
	{
		if (tx) {
			echo_req = (struct net_icmpv4_echo_req *)net_pkt_get_data(pkt,
										  &icmp_echo_req_access);
			tuple.orig.query_id = echo_req->identifier;
			tuple.new.query_id = echo_req->identifier = htons(sys_rand32_get());
			if (!tp_hairpinning(&tuple)) {
				goto drop;
			}
			new_addr = net_if_ipv4_select_src_addr(net_pkt_iface(pkt),
							       (struct in_addr *)NET_IPV4_HDR(
								       pkt)->dst);
		} else {
			/* Incoming echo request is routed to net stack. */
			goto exit;
		}
	}
	break;
	case NET_ICMPV4_ECHO_REPLY:
	{
		if (!tx) {
			echo_req = (struct net_icmpv4_echo_req *)net_pkt_get_data(pkt,
										  &icmp_echo_req_access);
			tuple.src.query_id = echo_req->identifier;
			if (icmp_echo_resp_handle(&tuple)) {
				echo_req->identifier = tuple.new.query_id;
				new_addr = &tuple.new.addr;
			}
		} else {
			/* Echo response for outgoing request don't need to be handled. */
			goto exit;
		}
	}
	break;
	case NET_ICMPV4_DST_UNREACH:
		LOG_DBG("ICMP ID NET_ICMPV4_DST_UNREACH");
		goto exit;
		break;
	case NET_ICMPV4_TIME_EXCEEDED:
		LOG_DBG("ICMP NET_ICMPV4_TIME_EXCEEDED");
		goto exit;
		break;
	case NET_ICMPV4_BAD_IP_HEADER:
		LOG_DBG("ICMP NET_ICMPV4_BAD_IP_HEADER");
		goto exit;
		break;
	default:
		goto drop;
	}

	net_pkt_cursor_restore(pkt, &icmp_hdr_pos);
	icmp_hdr->chksum = update_chksum(icmp_hdr->chksum,
					 (uint8_t *)&tuple.orig.query_id,
					 (uint8_t *)&tuple.new.query_id,
					 sizeof(uint16_t));

exit:
	net_pkt_cursor_restore(pkt, &ipv4_hdr_pos);

	if (!net_ipv4_is_addr_unspecified(new_addr)) {
		modify_ipv4_hdr(NET_IPV4_HDR(pkt), new_addr, tx);
	}

	net_pkt_set_overwrite(pkt, overwrite);

	return NET_OK;

drop:
	net_pkt_cursor_restore(pkt, &ipv4_hdr_pos);
	net_pkt_set_overwrite(pkt, overwrite);
	return NET_DROP;
}
