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
#include "tcp_private.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nat_proto, CONFIG_NRF_TBR_NAT_LOG_LEVEL);

/* Net Unreachable */
#define NET_ICMPV4_DST_UNREACH_NO_HOST   0
/* Communication Administratively Prohibited */
#define NET_ICMPV4_DST_UNREACH_COMM_PROHIBITED  13

#define ICMPV4_HDR_UNUSED_SIZE (4 * sizeof(uint8_t))

#if CONFIG_NRF_TBR_NAT_LOG_LEVEL >= LOG_LEVEL_DBG
#define PRINT_TUPLE(_str, _tuple)       print_tuple(__FUNCTION__, _str, _tuple)
#else
#define PRINT_TUPLE(_str, _tuple)
#endif

enum nat_proto {
	ICMP = 0,
#if defined(CONFIG_NET_UDP)
	UDP,
#endif /* CONFIG_NET_UDP */
#if defined(CONFIG_NET_TCP)
	TCP,
#endif /* CONFIG_NET_TCP */
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
		/** all */
		uint16_t all;
	};

	enum nat_proto proto;
};

struct nat_node {
	/** Entry list node. */
	sys_snode_t node;

	struct nat_tuple tuple;
	struct k_work_delayable timeout;
};

static K_MUTEX_DEFINE(lock_mtx);

K_MEM_SLAB_DEFINE_STATIC(records_pool, sizeof(struct nat_node),
			 CONFIG_NRF_TBR_NAT_RECORDS_COUNT, 4);
static sys_slist_t nat_records[PROTO_NUM_MAX];

extern uint16_t net_calc_chksum(struct net_pkt *pkt, uint8_t proto);

#if CONFIG_NRF_TBR_NAT_LOG_LEVEL >= LOG_LEVEL_DBG

static void print_tuple(const char *func, const char *str, const struct nat_tuple *tuple)
{
	char src[INET_ADDRSTRLEN];
	char dst[INET_ADDRSTRLEN];

	__ASSERT(tuple != NULL, "Null pointer");

	if (tuple->proto == ICMP) {
		LOG_DBG("%s: %s tuple: [%s %u] [%s %u]", !func ? "" : func, !str ? "" : str,
			net_addr_ntop(AF_INET, &tuple->src.addr, src, sizeof(src)),
			ntohs(tuple->src.query_id),
			net_addr_ntop(AF_INET, &tuple->dst.addr, dst, sizeof(dst)),
			ntohs(tuple->dst.query_id));
	} else {
		LOG_DBG("%s: %s tuple: [%s %u] [%s %u] [%u]", !func ? "" : "", !str ? "" : str,
			net_addr_ntop(AF_INET, &tuple->src.addr, src, sizeof(src)),
			ntohs(tuple->src.port),
			net_addr_ntop(AF_INET, &tuple->dst.addr, dst, sizeof(dst)),
			ntohs(tuple->dst.port),
			ntohs(tuple->port));
	}
}

#endif

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

static enum nat_proto nat_proto_get(const struct net_ipv4_hdr *hdr)
{
	switch (hdr->proto) {
	case IPPROTO_ICMP: return ICMP;
#if defined(CONFIG_NET_UDP)
	case IPPROTO_UDP: return UDP;
#endif /* CONFIG_NET_UDP */
#if defined(CONFIG_NET_TCP)
	case IPPROTO_TCP: return TCP;
#endif /* CONFIG_NET_TCP */
	}
	return PROTO_NUM_MAX;
}

static void tuple_invert(struct nat_tuple *tuple)
{
	struct nataddr_in tmp;

	__ASSERT(tuple != NULL, "Null pointer");

	memcpy(&tmp, &tuple->src, sizeof(struct nataddr_in));
	memcpy(&tuple->src, &tuple->dst, sizeof(struct nataddr_in));
	memcpy(&tuple->dst, &tmp, sizeof(struct nataddr_in));
}

static k_timeout_t proto_timeout(struct nat_tuple *tuple)
{
	switch (tuple->proto) {
	/* RFC5508: REQ-2: An ICMP Query session timer MUST NOT expire in less
	 * than 60 seconds.
	 */
	case ICMP: return K_SECONDS(CONFIG_NAT_ICMP_QUERY_SESSION_TIMEOUT_SEC);
#if defined(CONFIG_NET_UDP)
	/* RFC4787:  REQ-5:  A NAT UDP mapping timer MUST NOT expire in less than two
	 * minutes, unless REQ-5a applies.
	 * a) For specific destination ports in the well-known port range
	 *    (ports 0-1023), a NAT MAY have shorter UDP mapping timers that
	 *    are specific to the IANA-registered application running over
	 *    that specific destination port.
	 * b) The value of the NAT UDP mapping timer MAY be configurable.
	 * c) A default value of five minutes or more for the NAT UDP mapping
	 *    timer is RECOMMENDED.
	 */
	case UDP: return K_SECONDS(CONFIG_NAT_UDP_SESSION_TIMEOUT_SEC);
#endif /* CONFIG_NET_UDP */
#if defined(CONFIG_NET_TCP)
	/* RFC5382:  REQ-5: f a NAT cannot determine whether the endpoints of a TCP
	 * connection are active, it MAY abandon the session if it has been
	 * idle for some time.  In such cases, the value of the "established
	 * connection idle-timeout" MUST NOT be less than 2 hours 4 minutes.
	 */
	case TCP: return K_SECONDS(CONFIG_NAT_TCP_SESSION_TIMEOUT_SEC);
#endif /* CONFIG_NET_TCP */
	case PROTO_NUM_MAX:
	default:
	}
	return K_FOREVER;
}

static void timeout_cb(struct k_work *work)
{
	struct nat_node *_node = CONTAINER_OF(work, struct nat_node, timeout);

	PRINT_TUPLE("Remove", &_node->tuple);

	k_mutex_lock(&lock_mtx, K_FOREVER);
	sys_slist_find_and_remove(&nat_records[_node->tuple.proto], &_node->node);
	k_mem_slab_free(&records_pool, (void **)&_node);

	k_mutex_unlock(&lock_mtx);
}

static void timeout_start(struct nat_node *record)
{
	k_work_init_delayable(&record->timeout, timeout_cb);
	k_work_schedule(&record->timeout, proto_timeout(&record->tuple));
}

static void timeout_restart(struct nat_node *record)
{
	k_work_reschedule(&record->timeout, proto_timeout(&record->tuple));
}

static void timeout_cancel(struct nat_node *record)
{
	k_work_cancel_delayable(&record->timeout);
}

static bool record_add(const struct nat_tuple *tuple)
{
	struct nat_node *new_node = NULL;

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
	timeout_start(new_node);

	k_mutex_unlock(&lock_mtx);

	PRINT_TUPLE("Add", &new_node->tuple);

	return true;
}

static void tuple_cpy_from_hdr(struct nat_tuple *tuple, const struct net_ipv4_hdr *hdr)
{
	__ASSERT(tuple != NULL, "Null pointer");
	__ASSERT(hdr != NULL, "Null pointer");

	tuple->proto = nat_proto_get(hdr);
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

static bool mapping_find(struct nat_tuple *tuple)
{
	struct nat_node *_node, *tmp_node;
	struct nat_node *prev = NULL;
	char addr[INET_ADDRSTRLEN];

	__ASSERT(tuple != NULL, "Null pointer");

	PRINT_TUPLE("Search", tuple);

	k_mutex_lock(&lock_mtx, K_FOREVER);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&nat_records[tuple->proto], _node, tmp_node, node){
		PRINT_TUPLE("Record", &_node->tuple);

		if (addr_cmp(&tuple->dst, &_node->tuple.dst)) {
			switch (tuple->proto) {
			case ICMP:
				/* Restore original addr & query ID. */
				memcpy(&tuple->orig, &_node->tuple.orig, sizeof(struct nataddr_in));

				sys_slist_remove(&nat_records[tuple->proto],
						 prev ? &prev->node : NULL,
						 &_node->node);
				timeout_cancel(_node);
				k_mem_slab_free(&records_pool, (void **)&_node);
				goto found;
				break;

#if defined(CONFIG_NET_UDP)
			case UDP:
				if (tuple->port == _node->tuple.port) {
					/* Restore original addr & port number. */
					memcpy(&tuple->orig, &_node->tuple.orig,
					       sizeof(struct nataddr_in));
					timeout_restart(_node);
					goto found;
				}
				break;
#endif /* CONFIG_NET_UDP */

#if defined(CONFIG_NET_TCP)
			case TCP:
				if (tuple->port == _node->tuple.port) {
					/* Restore original addr & port number. */
					memcpy(&tuple->orig, &_node->tuple.orig,
					       sizeof(struct nataddr_in));
					timeout_restart(_node);
					goto found;
				}
				break;
#endif /* CONFIG_NET_TCP */

			default:
				goto exit;
			}
		}
		prev = _node;
	}

exit:
	k_mutex_unlock(&lock_mtx);
	return false;

found:
	k_mutex_unlock(&lock_mtx);

	LOG_DBG("Found, restored sender original addr %s and %s %u",
		net_addr_ntop(AF_INET, &tuple->orig.addr, addr, sizeof(addr)),
		tuple->proto == ICMP ? "query_id" : "port",
		ntohs(tuple->orig.all));

	return true;
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

static void send_icmp_error(struct net_pkt *pkt)
{
	__ASSERT(pkt != NULL, "Null pointer");

	/* rfc5508: REQ-8: When a NAT device is unable to establish a NAT Session for a
	 * new transport-layer (TCP, UDP, ICMP, etc.) flow due to
	 * resource constraints or administrative restrictions, the NAT
	 * device SHOULD send an ICMP destination unreachable message,
	 * with a code of 13 (Communication administratively prohibited)
	 * to the sender, and drop the original packet.
	 */
	net_icmpv4_send_error(pkt, NET_ICMPV4_DST_UNREACH,
			      NET_ICMPV4_DST_UNREACH_COMM_PROHIBITED);
}

static enum net_verdict icmp_error_handle(struct net_pkt *pkt, struct nat_tuple *tuple,
					  struct net_icmp_hdr *main_icmp_hdr)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(icmp_hdr_access, struct net_icmp_hdr);
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(icmp_echo_req_access,
					      struct net_icmpv4_echo_req);
	struct net_pkt_cursor icmp_hdr_pos;
	struct net_icmpv4_echo_req *echo_req;
	struct net_icmp_hdr *icmp_hdr = NULL;

	/* The ICMP error message body contains fragment of failed IP packet in original form thus
	 * it is a ICMP message in this case.
	 */
	icmp_hdr = (struct net_icmp_hdr *)net_pkt_get_data(pkt, &icmp_hdr_access);
	if (!icmp_hdr) {
		LOG_ERR("Failed to read ICMP header");
		return NET_DROP;
	}

	net_pkt_cursor_backup(pkt, &icmp_hdr_pos);
	if (net_pkt_skip(pkt, sizeof(struct net_icmp_hdr))) {
		return NET_DROP;
	}

	if (icmp_hdr->type == NET_ICMPV4_ECHO_REQUEST) {
		echo_req = (struct net_icmpv4_echo_req *)net_pkt_get_data(pkt,
									  &icmp_echo_req_access);
		tuple->dst.query_id = echo_req->identifier;

		if (mapping_find(tuple)) {
			echo_req->identifier = tuple->orig.query_id;
		} else {
			/* rfc5508: REQ-4 & REQ-5: If a NAT device receives an ICMP Error packet
			 * from an external (the private) realm, and the NAT device does not have
			 * an active mapping for the embedded payload, the NAT SHOULD silently
			 * drop the ICMP Error packet.
			 */
			return NET_DROP;
		}
	} else {
		return NET_DROP;
	}

	net_pkt_cursor_restore(pkt, &icmp_hdr_pos);

	icmp_hdr->chksum = update_chksum(icmp_hdr->chksum,
					 (uint8_t *)&tuple->new.query_id,
					 (uint8_t *)&tuple->orig.query_id,
					 sizeof(uint16_t));

	/* Update checksum in main ICMP header as well. */
	main_icmp_hdr->chksum = update_chksum(main_icmp_hdr->chksum,
					      (uint8_t *)&tuple->new.query_id,
					      (uint8_t *)&tuple->orig.query_id,
					      sizeof(uint16_t));

	return NET_OK;
}

enum net_verdict nat_icmp(struct net_pkt *pkt, bool tx)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(icmp_hdr_access, struct net_icmp_hdr);
	struct net_pkt_cursor ipv4_hdr_pos;
	struct net_pkt_cursor icmp_hdr_pos;
	struct net_icmp_hdr *icmp_hdr = NULL;
	struct nat_tuple tuple;
	struct in_addr new_addr;
	bool overwrite;

	if (!pkt) {
		return NET_DROP;
	}

	net_ipaddr_copy(&new_addr, net_ipv4_unspecified_address());
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
	case NET_ICMPV4_ECHO_REPLY:
	{
		NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(icmp_echo_req_access,
						      struct net_icmpv4_echo_req);
		struct net_icmpv4_echo_req *echo_req;
		uint16_t old_q_id;
		uint16_t new_q_id;

		if (icmp_hdr->type == NET_ICMPV4_ECHO_REQUEST) {
			if (tx) {
				echo_req = (struct net_icmpv4_echo_req *)net_pkt_get_data(
					pkt,
					&icmp_echo_req_access);
				old_q_id = tuple.orig.query_id = echo_req->identifier;
				new_q_id = tuple.new.query_id = echo_req->identifier = htons(
					sys_rand32_get());
				if (!tp_hairpinning(&tuple)) {
					send_icmp_error(pkt);
					goto drop;
				}
				net_ipaddr_copy(&new_addr, net_if_ipv4_select_src_addr(
							net_pkt_iface(pkt),
							(struct in_addr *)NET_IPV4_HDR(pkt)->dst));
			} else {
				/* Incoming echo request is routed to net stack. */
				goto exit;
			}
		} else if (icmp_hdr->type == NET_ICMPV4_ECHO_REPLY) {
			if (!tx) {
				echo_req = (struct net_icmpv4_echo_req *)net_pkt_get_data(
					pkt,
					&icmp_echo_req_access);
				old_q_id = tuple.src.query_id = echo_req->identifier;
				tuple_invert(&tuple);

				if (mapping_find(&tuple)) {
					new_q_id = echo_req->identifier = tuple.orig.query_id;
					net_ipaddr_copy(&new_addr, &tuple.orig.addr);
				}
			} else {
				/* Echo response for outgoing request don't need to be handled. */
				goto exit;
			}
		}

		net_pkt_cursor_restore(pkt, &icmp_hdr_pos);
		icmp_hdr->chksum = update_chksum(icmp_hdr->chksum,
						 (uint8_t *)&old_q_id,
						 (uint8_t *)&new_q_id,
						 sizeof(uint16_t));
	}
	break;
	case NET_ICMPV4_DST_UNREACH:
	case NET_ICMPV4_TIME_EXCEEDED:
	case NET_ICMPV4_BAD_IP_HEADER:
		/* rfc5508: REQ-3: When an ICMP Error packet is received, if the ICMP checksum
		 * fails to validate, the NAT SHOULD silently drop the ICMP Error packet.
		 * So, we can do it here for for each supported ICMP error packet.
		 */
		if (net_calc_chksum(pkt, IPPROTO_ICMP) != 0U) {
			LOG_DBG("Invalid checksum");
			goto drop;
		}

		if (icmp_hdr->type == NET_ICMPV4_DST_UNREACH) {
			if (icmp_hdr->code == NET_ICMPV4_DST_UNREACH_NO_PORT ||
			    icmp_hdr->code == NET_ICMPV4_DST_UNREACH_NO_HOST) {
				NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(ipv4_access,
								      struct net_ipv4_hdr);
				struct net_ipv4_hdr *ip_hdr;
				struct net_pkt_cursor msg_body;
				struct net_pkt_cursor ipv4_hdr_pos;
				struct nat_tuple icmp_tuple;

				LOG_DBG("Destination unreachable");

				net_pkt_cursor_backup(pkt, &msg_body);
				/* Skip unused bytes. */
				if (net_pkt_skip(pkt, ICMPV4_HDR_UNUSED_SIZE)) {
					goto drop;
				}

				ip_hdr = (struct net_ipv4_hdr *)net_pkt_get_data(pkt, &ipv4_access);
				if (!ip_hdr) {
					LOG_WRN("Failed to read IPv4 header");
					goto drop;
				}

				if (ip_hdr->proto != IPPROTO_ICMP &&
				    (IS_ENABLED(CONFIG_NET_UDP) && ip_hdr->proto != IPPROTO_UDP) &&
				    (IS_ENABLED(CONFIG_NET_TCP) && ip_hdr->proto != IPPROTO_TCP)) {
					LOG_WRN("Wrong protocol");
					goto drop;
				}

				/* Copy to tuple before changing cursor position. */
				tuple_cpy_from_hdr(&icmp_tuple, ip_hdr);

				net_pkt_cursor_backup(pkt, &ipv4_hdr_pos);
				if (net_pkt_skip(pkt, net_pkt_ip_hdr_len(pkt) +
						 net_pkt_ip_opts_len(pkt))) {
					goto drop;
				}

				if (ip_hdr->proto == IPPROTO_ICMP) {
					icmp_error_handle(pkt, &icmp_tuple, icmp_hdr);
				}

				/* It is still ICMP error message body. Restore old IP address
				 * in the failed IP packet.
				 * But first update checksum for main ICMP header.
				 */
				icmp_hdr->chksum = update_chksum(icmp_hdr->chksum,
								 (uint8_t *)&ip_hdr->src,
								 (uint8_t *)&icmp_tuple.orig.addr,
								 sizeof(uint16_t));
				net_pkt_cursor_restore(pkt, &ipv4_hdr_pos);
				modify_ipv4_hdr(ip_hdr, &icmp_tuple.orig.addr, true);

				/* And copy original address. */
				net_ipaddr_copy(&new_addr, &icmp_tuple.orig.addr);
				net_pkt_cursor_restore(pkt, &msg_body);

				goto exit;
			}

			goto drop;
		} else if (icmp_hdr->type == NET_ICMPV4_TIME_EXCEEDED) {
			LOG_DBG("ICMP NET_ICMPV4_TIME_EXCEEDED");
		} else if (icmp_hdr->type == NET_ICMPV4_TIME_EXCEEDED) {
			LOG_DBG("ICMP NET_ICMPV4_BAD_IP_HEADER");
		} else {
			goto drop;
		}
		break;
	default:
		goto drop;
	}

exit:
	net_pkt_cursor_restore(pkt, &ipv4_hdr_pos);

	if (!net_ipv4_is_addr_unspecified(&new_addr)) {
		modify_ipv4_hdr(NET_IPV4_HDR(pkt), &new_addr, tx);
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
		tuple.port = udp_hdr->src_port;
	} else {
		/* Store previous IP address. */
		net_ipaddr_copy(&prev_addr, &tuple.dst.addr);
		tuple.port = udp_hdr->dst_port;

		tuple_invert(&tuple);
	}

	if (mapping_find(&tuple)) {
		LOG_DBG("Mapping exists");

		if (tx) {
			if (!net_ipv4_addr_cmp(&tuple.src.addr, &prev_addr)) {
				LOG_DBG("Src address does not match");
				/* Port conflict, drop packet for current implementation. */
				goto drop;
			}
		} else {
			new_addr = &tuple.orig.addr;
		}
	} else {
		LOG_DBG("New mapping");
		if (!tp_hairpinning(&tuple)) {
			send_icmp_error(pkt);
			goto drop;
		}
	}

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

	net_pkt_set_overwrite(pkt, overwrite);

	return NET_OK;

drop:
	net_pkt_cursor_restore(pkt, &ipv4_hdr_pos);
	net_pkt_set_overwrite(pkt, overwrite);
	return NET_DROP;
}

#endif /* CONFIG_NET_UDP */

#if defined(CONFIG_NET_TCP)

enum net_verdict nat_tcp(struct net_pkt *pkt, bool tx)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(tcp_hdr_access, struct tcphdr);
	struct net_pkt_cursor ipv4_hdr_pos;
	struct tcphdr *tcp_hdr;
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

	tcp_hdr = (struct tcphdr *)net_pkt_get_data(pkt, &tcp_hdr_access);
	if (!tcp_hdr) {
		LOG_WRN("Failed to read TCP header");
		goto drop;
	}

	tuple.dst.port = tcp_hdr->th_dport;
	tuple.src.port = tcp_hdr->th_sport;

	if (tx) {
		/* Store previous IP address. */
		net_ipaddr_copy(&prev_addr, &tuple.src.addr);
		tuple.port = tcp_hdr->th_sport;
	} else {
		/* Store previous IP address. */
		net_ipaddr_copy(&prev_addr, &tuple.dst.addr);
		tuple.port = tcp_hdr->th_dport;

		tuple_invert(&tuple);
	}

	if (mapping_find(&tuple)) {
		LOG_DBG("Mapping exists");

		if (tx) {
			if (!net_ipv4_addr_cmp(&tuple.src.addr, &prev_addr)) {
				LOG_DBG("Src address does not match");
				/* Port conflict, drop packet for current implementation. */
				goto drop;
			}
		} else {
			new_addr = &tuple.orig.addr;
		}
	} else {
		LOG_DBG("New mapping");
		if (!tp_hairpinning(&tuple)) {
			send_icmp_error(pkt);
			goto drop;
		}
	}

	/* rfc793: "The checksum also covers a 96 bit pseudo header conceptually
	 * prefixed to the TCP header.  This pseudo header contains the Source Address,
	 * the Destination Address, the Protocol, and TCP length."
	 * The TCP checksum must include IP address as well.
	 */
	tcp_hdr->th_sum = update_chksum(tcp_hdr->th_sum,
					(uint8_t *)&prev_addr,
					(uint8_t *)new_addr,
					sizeof(struct in_addr));

	net_pkt_cursor_restore(pkt, &ipv4_hdr_pos);
	modify_ipv4_hdr(NET_IPV4_HDR(pkt), new_addr, tx);

	net_pkt_set_overwrite(pkt, overwrite);

	return NET_OK;

drop:
	net_pkt_cursor_restore(pkt, &ipv4_hdr_pos);
	net_pkt_set_overwrite(pkt, overwrite);
	return NET_DROP;
}

#endif /* CONFIG_NET_TCP */

void nat_records_remove_all()
{
	struct nat_node *_node, *tmp_node;

	k_mutex_lock(&lock_mtx, K_FOREVER);
	for (enum nat_proto p_it = ICMP; p_it < PROTO_NUM_MAX; p_it++) {
		sys_slist_t *rec = &nat_records[p_it];

		if (sys_slist_is_empty(rec)) {
			continue;
		}

		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(rec, _node, tmp_node, node){
			PRINT_TUPLE("Remove", &_node->tuple);

			timeout_cancel(_node);
			sys_slist_remove(rec, NULL, &_node->node);
			k_mem_slab_free(&records_pool, (void **)&_node);
		}
	}
	k_mutex_unlock(&lock_mtx);
}
