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

#define WELL_KNOWN_PORTS_MAX 1023U
#define ICMPV4_HDR_UNUSED_SIZE (4 * sizeof(uint8_t))

#if defined(CONFIG_NET_UDP)
#define UDP_PROTO(_hdr) ((_hdr->proto == IPPROTO_UDP) ? UDP : PROTO_NUM_MAX)
#else
#define UDP_PROTO(_hdr) (PROTO_NUM_MAX)
#endif

#define GET_NAT_PROTO(_hdr) ((_hdr->proto == IPPROTO_ICMP) ? ICMP : \
			     UDP_PROTO(_hdr))

enum nat_proto {
	ICMP = 0,
#if defined(CONFIG_NET_UDP)
	UDP,
#endif /* CONFIG_NET_UDP */
	PROTO_NUM_MAX
};

struct nataddr_in {
	struct in_addr addr;
	union {
		/** icmp */
		uint16_t query_id;
		/** tcp/udp */
		uint16_t port;
		/** all protocols */
		uint16_t all;
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

	union {
		/** Port assigned to external connection */
		uint16_t port;
		/** ICMPv4 */
		uint16_t unused;
	};

	enum nat_proto proto;
};

struct nat_node {
	/** Entry list node. */
	sys_snode_t node;

	struct nat_tuple tuple;
};

static K_MUTEX_DEFINE(lock_mtx);

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

static uint16_t port_no_generate()
{
	uint16_t port;

	do {
		sys_rand_get(&port, sizeof(port));
	} while (port == 0U);

	if (port > WELL_KNOWN_PORTS_MAX) {
		return port;
	}

	port += ((port << 10) & 0xFFFF);
	return port;
}

static void tuple_invert(struct nat_tuple *tuple)
{
	struct nataddr_in tmp;

	__ASSERT(tuple != NULL, "Null pointer");

	memcpy(&tmp, &tuple->src, sizeof(struct nataddr_in));
	memcpy(&tuple->src, &tuple->dst, sizeof(struct nataddr_in));
	memcpy(&tuple->dst, &tmp, sizeof(struct nataddr_in));
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

	k_mutex_lock(&lock_mtx, K_FOREVER);

	sys_slist_append(&nat_records[tuple->proto], (sys_snode_t *)&new_node->node);

	k_mutex_unlock(&lock_mtx);

	if (tuple->proto == ICMP) {
		LOG_DBG("Added tuple: [%s %u] [%s %u]",
			net_addr_ntop(AF_INET, &tuple->src.addr, src, sizeof(src)),
			ntohs(tuple->src.query_id),
			net_addr_ntop(AF_INET, &tuple->dst.addr, dst, sizeof(dst)),
			ntohs(tuple->dst.query_id));
	} else {
		LOG_DBG("Added tuple: [%s %u] [%s %u] [%u]",
			net_addr_ntop(AF_INET, &tuple->src.addr, src, sizeof(src)),
			ntohs(tuple->src.port),
			net_addr_ntop(AF_INET, &tuple->dst.addr, dst, sizeof(dst)),
			ntohs(tuple->dst.port),
			ntohs(tuple->port));
	}

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

static bool addr_cmp(const struct nataddr_in *addr1, const struct nataddr_in *addr2)
{
	if (net_ipv4_addr_cmp(&addr1->addr, &addr2->addr) && (addr1->all == addr2->all)) {
		return true;
	}
	return false;
}

static bool tp_rx_conn_handle(struct nat_tuple *tuple)
{
	struct nat_node *_node, *tmp_node;
	struct nat_node *prev = NULL;
	char addr[INET_ADDRSTRLEN];

	__ASSERT(tuple != NULL, "Null pointer");

	LOG_DBG("Search record: %s:%u",
		net_addr_ntop(AF_INET, &tuple->src.addr, addr, sizeof(addr)),
		ntohs(tuple->src.all));

	k_mutex_lock(&lock_mtx, K_FOREVER);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&nat_records[tuple->proto], _node, tmp_node, node){
		LOG_DBG("Record: %s:%u",
			net_addr_ntop(AF_INET, &_node->tuple.dst.addr, addr, sizeof(addr)),
			ntohs(_node->tuple.new.query_id));

		if (addr_cmp(&tuple->src, &_node->tuple.dst)) {
			if ((tuple->proto == ICMP) &&
			    (tuple->src.query_id == _node->tuple.new.query_id)) {
				/* Restore original addr & query ID. */
				net_ipaddr_copy(&tuple->new.addr, &_node->tuple.orig.addr);
				tuple->new.query_id = _node->tuple.orig.query_id;

				sys_slist_remove(&nat_records[tuple->proto],
						 prev ? &prev->node : NULL,
						 &_node->node);
				k_mem_slab_free(&records_pool, (void **)&_node);
			} else if (tuple->dst.port == _node->tuple.port) {
				/* Restore original addr & port number. */
				memcpy(&tuple->dst, &_node->tuple.orig, sizeof(struct nataddr_in));
			}

			k_mutex_unlock(&lock_mtx);

			LOG_DBG("Found, restored sender original addr %s and %s %u",
				net_addr_ntop(AF_INET, &tuple->new.addr, addr, sizeof(addr)),
				tuple->proto == ICMP ? "query_id" : "port",
				ntohs(tuple->new.all));

			return true;
		}
		prev = _node;
	}

	k_mutex_unlock(&lock_mtx);
	return false;
}

static bool tp_tx_conn_handle(struct nat_tuple *tuple)
{
	struct nat_node *_node;

	__ASSERT(tuple != NULL, "Null pointer");

	k_mutex_lock(&lock_mtx, K_FOREVER);
	SYS_SLIST_FOR_EACH_CONTAINER(&nat_records[tuple->proto], _node, node){
		if (addr_cmp(&tuple->dst, &_node->tuple.dst) &&
		    addr_cmp(&tuple->orig, &_node->tuple.orig)) {
			/* Set proper port number. */
			tuple->port = _node->tuple.port;
			k_mutex_unlock(&lock_mtx);
			return true;
		}
	}
	k_mutex_unlock(&lock_mtx);

	return false;
}

static bool is_src_port_used(const struct nat_tuple *tuple)
{
	struct nat_node *_node;

	__ASSERT(tuple != NULL, "Null pointer");

	k_mutex_lock(&lock_mtx, K_FOREVER);
	SYS_SLIST_FOR_EACH_CONTAINER(&nat_records[tuple->proto], _node, node){
		if (_node->tuple.src.port == tuple->src.port) {
			LOG_DBG("Port %d found", _node->tuple.src.port);
			k_mutex_unlock(&lock_mtx);
			return true;
		}
	}
	k_mutex_unlock(&lock_mtx);

	return false;
}

static bool is_ext_port_used(const struct nat_tuple *tuple)
{
	struct nat_node *_node;

	__ASSERT(tuple != NULL, "Null pointer");

	k_mutex_lock(&lock_mtx, K_FOREVER);
	SYS_SLIST_FOR_EACH_CONTAINER(&nat_records[tuple->proto], _node, node){
		if (_node->tuple.port == tuple->port) {
			LOG_DBG("Port %d found", _node->tuple.port);
			k_mutex_unlock(&lock_mtx);
			return true;
		}
	}
	k_mutex_unlock(&lock_mtx);

	return false;
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

static void icmp_dst_port_unreach(struct net_pkt *pkt, bool tx)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(ipv4_access, struct net_ipv4_hdr);
	struct net_ipv4_hdr *ip_hdr;

	__ASSERT(pkt != NULL, "Null pointer");

	LOG_DBG("Destination port unreachable");

	/* Skip unused bytes. */
	if (net_pkt_skip(pkt, ICMPV4_HDR_UNUSED_SIZE)) {
		return;
	}

	ip_hdr = (struct net_ipv4_hdr *)net_pkt_get_data(pkt, &ipv4_access);

	if (!ip_hdr) {
		LOG_WRN("Failed to read IPv4 header");
		return;
	}

	if ((IS_ENABLED(CONFIG_NET_UDP) && ip_hdr->proto == IPPROTO_UDP) ||
	    (IS_ENABLED(CONFIG_NET_TCP) && ip_hdr->proto == IPPROTO_TCP)) {
		struct nat_tuple tuple;
		struct net_pkt_cursor pos;

		/* Copy to tuple before changing cursor position. */
		tuple_cpy_from_hdr(&tuple, ip_hdr);

		net_pkt_cursor_backup(pkt, &pos);
		if (net_pkt_skip(pkt, net_pkt_ip_hdr_len(pkt) +
				 net_pkt_ip_opts_len(pkt))) {
			return;
		}

		if (IS_ENABLED(CONFIG_NET_UDP) && ip_hdr->proto == IPPROTO_UDP) {
			NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(udp_hdr_access,
							      struct net_udp_hdr);
			struct net_udp_hdr *udp_hdr =
				(struct net_udp_hdr *)net_pkt_get_data(pkt, &udp_hdr_access);
			if (!udp_hdr) {
				LOG_WRN("Failed to read UDP header");
				goto exit;
			}

			LOG_DBG("Ports: src %u dst %u", ntohs(udp_hdr->src_port),
				ntohs(udp_hdr->dst_port));
			tuple.dst.port = udp_hdr->dst_port;
			tuple.src.port = udp_hdr->src_port;
		} else if (IS_ENABLED(CONFIG_NET_TCP) && ip_hdr->proto == IPPROTO_TCP) {
		}

		if (tx) {
			tuple_invert(&tuple);
		}

		tuple.port = tuple.src.port;
		/* TODO:
		 * RFC5508 REQ-5:
		 * If a NAT device receives an ICMP Error packet from the private
		 * realm, and the NAT does not have an active mapping for the
		 * embedded payload, the NAT SHOULD silently drop the ICMP Error
		 * packet. If the NAT has active mapping for the embedded
		 * payload, then the NAT MUST do the following prior to
		 * forwarding the packet, unless explicitly overridden by local
		 * policy:
		 * a) Revert the IP and transport headers of the embedded IP
		 *    packet to their original form, using the matching mapping;
		 *    and
		 * b) Leave the ICMP Error type and code unchanged; and
		 * c) If the NAT enforces Basic NAT function ([NAT-TRAD]), and
		 *    the NAT has active mapping for the IP address that sent the
		 *    ICMP Error, translate the source IP address of the ICMP
		 *    Error packet with the public IP address in the mapping.  In
		 *    all other cases, translate the source IP address of the
		 *    ICMP Error packet with its own public IP address.
		 */

exit:
		net_pkt_cursor_restore(pkt, &pos);
	}
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
			new_addr = net_if_ipv4_select_src_addr(net_pkt_iface(
								       pkt),
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
			if (tp_rx_conn_handle(&tuple)) {
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
		if (icmp_hdr->code == NET_ICMPV4_DST_UNREACH_NO_PORT) {
			icmp_dst_port_unreach(pkt, tx);
		}
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

#if defined(CONFIG_NET_UDP)

enum net_verdict nat_udp(struct net_pkt *pkt, bool tx)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(udp_hdr_access, struct net_udp_hdr);
	struct net_pkt_cursor ipv4_hdr_pos;
	struct net_udp_hdr *udp_hdr;
	struct nat_tuple tuple;
	const struct in_addr *new_addr;
	struct in_addr prev_addr;
	bool overwrite;

	if (!pkt) {
		return NET_DROP;
	}

	new_addr = net_if_ipv4_select_src_addr(net_pkt_iface(pkt),
					       (struct in_addr *)NET_IPV4_HDR(pkt)->dst);

	tuple_cpy_from_hdr(&tuple, NET_IPV4_HDR(pkt));

	overwrite = net_pkt_is_being_overwritten(pkt);
	net_pkt_set_overwrite(pkt, true);
	net_pkt_cursor_backup(pkt, &ipv4_hdr_pos);
	if (net_pkt_skip(pkt, net_pkt_ip_hdr_len(pkt) + net_pkt_ip_opts_len(pkt))) {
		goto drop;
	}

	udp_hdr = (struct net_udp_hdr *)net_pkt_get_data(pkt, &udp_hdr_access);
	if (!udp_hdr) {
		LOG_WRN("Failed to read UDP header");
		goto drop;
	}

	tuple.dst.port = udp_hdr->dst_port;
	tuple.src.port = udp_hdr->src_port;

	if (tx) {
		/* Store previous IP address. */
		net_ipaddr_copy(&prev_addr, &tuple.src.addr);

		if (!tp_tx_conn_handle(&tuple)) {
			LOG_DBG("New mapping");

			if (is_src_port_used(&tuple)) {
				/* If port number is already reserved, generate a new port
				 * number.
				 */
				do {
					tuple.port = udp_hdr->src_port = htons(port_no_generate());
				} while (is_ext_port_used(&tuple));

				LOG_DBG("New port number: %d", ntohs(udp_hdr->src_port));
				udp_hdr->chksum = update_chksum(udp_hdr->chksum,
								(uint8_t *)&tuple.orig.port,
								(uint8_t *)&udp_hdr->src_port,
								sizeof(uint16_t));
			} else {
				/* Otherwise left the same port number. */
				tuple.port = udp_hdr->src_port;
			}

			if (!tp_hairpinning(&tuple)) {
				goto drop;
			}

			/* TODO 4. set timeout. */
		} else {
			LOG_DBG("Mapping exists");

			/* Recompute checksum only when ports are different. */
			if (tuple.orig.port != tuple.port) {
				udp_hdr->chksum = update_chksum(udp_hdr->chksum,
								(uint8_t *)&tuple.orig.port,
								(uint8_t *)&tuple.port,
								sizeof(uint16_t));
			}

			/* TODO 2a. Update timeout. */
		}
	} else {
		/* Store previous IP address. */
		net_ipaddr_copy(&prev_addr, &tuple.dst.addr);

		if (tp_rx_conn_handle(&tuple)) {
			LOG_DBG("Mapping exists");
			/* Checksum must be recomputed before data will be overwritten. */
			udp_hdr->chksum = update_chksum(udp_hdr->chksum,
							(uint8_t *)&udp_hdr->dst_port,
							(uint8_t *)&tuple.orig.port,
							sizeof(uint16_t));

			new_addr = &tuple.dst.addr;
			udp_hdr->dst_port = tuple.dst.port;
		} else {
			LOG_DBG("New mapping");
			/* Left the port number, because it may already has
			 * been binded.
			 */
			tuple.port = udp_hdr->dst_port;

			/* In order to make this code more generic, invert tuple and
			 * add it to NAT records.
			 */
			tuple_invert(&tuple);
			if (!tp_hairpinning(&tuple)) {
				goto drop;
			}

			/* And now throw in packet to the net stack
			 * without any additional modification.
			 */
			new_addr = net_ipv4_unspecified_address();
		}
	}

	if (!net_ipv4_is_addr_unspecified(new_addr)) {
		/* rfc768: "Checksum is the 16-bit one's complement of the one's complement
		 * sum of a pseudo header of information from the IP header, the UDP header..."
		 * The UDP checksum must include IP address as well.
		 */
		udp_hdr->chksum = update_chksum(udp_hdr->chksum,
						(uint8_t *)&prev_addr,
						(uint8_t *)new_addr,
						sizeof(struct in_addr));

		net_pkt_cursor_restore(pkt, &ipv4_hdr_pos);
		modify_ipv4_hdr(NET_IPV4_HDR(pkt), new_addr, tx);
	} else {
		net_pkt_cursor_restore(pkt, &ipv4_hdr_pos);
	}

	net_pkt_set_overwrite(pkt, overwrite);

	return NET_OK;

drop:
	net_pkt_cursor_restore(pkt, &ipv4_hdr_pos);
	net_pkt_set_overwrite(pkt, overwrite);
	return NET_DROP;
}

#endif /* CONFIG_NET_UDP */
