/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief nRF Thread Border Router's Neighbor Discovery Proxy
 */

#include "tbr.h"
#include "nd_proxy_internal.h"

#include <assert.h>
#include <ipv6.h>

#include <openthread/backbone_router_ftd.h>

#include <zephyr/kernel.h>
#include <zephyr/net/net_pkt.h>

LOG_MODULE_DECLARE(nrf_tbr, CONFIG_NRF_TBR_LOG_LEVEL);

static struct in6_addr all_nodes_mcast_addr = { { { 0xff, 0x02, 0, 0, 0, 0, 0, 0,
						    0, 0, 0, 0, 0, 0, 0, 0x1 } } };
static sys_slist_t entries;

K_MEM_SLAB_DEFINE_STATIC(entries_slab, ND_PROXY_SLAB_BLOCK_SIZE, ND_PROXY_SLAB_BLOCK_CNT,
			 ND_PROXY_SLAB_BLOCK_ALIGN);

static enum net_verdict handle_icmpv6_ns(struct net_pkt *pkt, struct net_ipv6_hdr *ip_hdr,
					    struct net_icmp_hdr *icmp_hdr);

static struct nd_proxy_entry *entry_find(const struct in6_addr *addr);

static struct net_icmpv6_handler icpmv6_ns_handler = {
	.type = NET_ICMPV6_NS,
	.code = 0,
	.handler = handle_icmpv6_ns,
};

static void send_na(const struct in6_addr *dst, const struct in6_addr *target)
{
	uint32_t flags;
	struct tbr_context *ctx = tbr_get_context();
	bool is_mcast = net_ipv6_is_addr_mcast(dst);

	flags = NET_ICMPV6_NA_FLAG_ROUTER;
	flags |= is_mcast ? 0 : NET_ICMPV6_NA_FLAG_SOLICITED;
	flags |= is_mcast ? NET_ICMPV6_NA_FLAG_SOLICITED : 0;

	if (net_ipv6_send_na(ctx->backbone_iface, ctx->ll_addr, dst, target, flags) < 0) {
		LOG_ERR("ND Proxy: failed to send a Neighbor Advertisement");
	}
}

static enum net_verdict handle_icmpv6_ns(struct net_pkt *pkt, struct net_ipv6_hdr *ip_hdr,
					    struct net_icmp_hdr *icmp_hdr)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(ns_access, struct net_icmpv6_ns_hdr);
	struct net_icmpv6_ns_hdr *ns_hdr;
	struct nd_proxy_entry *entry;
	char addr_str[INET6_ADDRSTRLEN];

	if (!pkt || !ip_hdr || !icmp_hdr) {
		return NET_DROP;
	}

	ns_hdr = (struct net_icmpv6_ns_hdr *)net_pkt_get_data(pkt, &ns_access);

	if (!ns_hdr) {
		LOG_ERR("ND proxy: Failed to read NS header");
		return NET_DROP;
	}

	net_addr_ntop(AF_INET6, &ns_hdr->tgt, addr_str, sizeof(addr_str));
	LOG_DBG("ND proxy: looking for entry address [%s]", addr_str);

	entry = entry_find((const struct in6_addr *)&ns_hdr->tgt);

	if (!entry) {
		LOG_DBG("ND proxy: no entry matched");

		/* Pass Neighbor Solicitation message to the Zephyr stack */
		return NET_CONTINUE;
	}

	send_na((const struct in6_addr *)ip_hdr->src, &entry->dua);

	/** TODO: HANDLE RECEIVED NS STATS **/

	return NET_OK;
}

static void leave_mcast_group(struct net_if_mcast_addr *maddr)
{
	struct tbr_context *ctx = tbr_get_context();
	struct nd_proxy_entry *current;
	struct nd_proxy_entry *next;
	size_t count = 0;

	if (!maddr) {
		return;
	}

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&entries, current, next, node) {
		/* if multiple entries may point to the same group pointers are the same */
		if (current->maddr == maddr) {
			count++;
		}
	}

	if (count <= 1) {
		net_if_ipv6_maddr_leave(maddr);
		net_if_ipv6_maddr_rm(ctx->backbone_iface, &maddr->address.in6_addr);
	}
}

static struct nd_proxy_entry *entry_add(const struct in6_addr *addr)
{
	struct nd_proxy_entry *entry;
	struct in6_addr solicited_addr;
	struct tbr_context *ctx = tbr_get_context();

	entry = entry_find(addr);
	if (entry) {
		return entry;
	}

	if (k_mem_slab_alloc(&entries_slab, (void **)&entry, K_NO_WAIT) < 0) {
		return NULL;
	}

	net_ipv6_addr_copy_raw((uint8_t *)entry->dua.s6_addr, (uint8_t *)addr->s6_addr);
	sys_slist_append(&entries, &entry->node);

	/* DO NOT use MLD join as per Thread Specification (9.4.8.2.2.2) */
	net_ipv6_addr_create_solicited_node(addr, &solicited_addr);
	entry->maddr = net_if_ipv6_maddr_lookup(&solicited_addr, &ctx->backbone_iface);

	if (!entry->maddr) {
		entry->maddr = net_if_ipv6_maddr_add(ctx->backbone_iface, &solicited_addr);
		net_if_ipv6_maddr_join(entry->maddr);
	}

	return entry;
}

static struct nd_proxy_entry *entry_find(const struct in6_addr *addr)
{
	struct nd_proxy_entry *current;
	struct nd_proxy_entry *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&entries, current, next, node) {
		if (net_ipv6_addr_cmp(&current->dua, addr)) {
			return current;
		}
	}

	return NULL;
}

static void entry_remove(struct nd_proxy_entry *entry)
{
	if (entry) {
		leave_mcast_group(entry->maddr);
		sys_slist_find_and_remove(&entries, &entry->node);
		k_mem_slab_free(&entries_slab, (void **)&entry);
	}
}

static void entry_clear_all(void)
{
	struct nd_proxy_entry *current = NULL;
	struct nd_proxy_entry *next = NULL;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&entries, current, next, node) {
		leave_mcast_group(current->maddr);
		sys_slist_remove(&entries, NULL, &current->node);
		k_mem_slab_free(&entries_slab, (void **)&current);
	};
}

static void handle_nd_proxy_event(void *cb_ctx, otBackboneRouterNdProxyEvent event,
				  const struct otIp6Address *dua)
{
	ARG_UNUSED(cb_ctx);

	struct in6_addr addr;
	struct nd_proxy_entry *entry;
	struct tbr_context *ctx = tbr_get_context();
	char addr_str[INET6_ADDRSTRLEN];

	if (!ctx->backbone_iface) {
		LOG_DBG("ND Proxy: backbone interface not initialized");
		return;
	}

	if (dua) {
		/* Avoid unaligned access to a packed member */
		net_ipv6_addr_copy_raw((uint8_t *)addr.s6_addr, (uint8_t *)dua->mFields.m8);
		net_addr_ntop(AF_INET6, &addr.s6_addr, addr_str, sizeof(addr_str));
	}

	switch (event)
	{
	case OT_BACKBONE_ROUTER_NDPROXY_ADDED:
	case OT_BACKBONE_ROUTER_NDPROXY_RENEWED:
		__ASSERT(dua, "ND Proxy: addition or renewal without the address");

		entry = entry_add(&addr);

		if (!entry) {
			LOG_ERR("ND Proxy: failed to handle an event");
			return;
		}

		LOG_DBG("ND Proxy: entry [%s] added", addr_str);

		/* Send unsolicited Neighbor Advertisement */
		send_na(&all_nodes_mcast_addr, &addr);
		break;
	case OT_BACKBONE_ROUTER_NDPROXY_REMOVED:
		__ASSERT(dua, "ND Proxy: removal without the address");

		entry = entry_find(&addr);

		if (!entry) {
			LOG_DBG("ND Proxy: entry [%s] not found", addr_str);
			return;
		}

		entry_remove(entry);
		LOG_DBG("ND Proxy: entry [%s] removed", addr_str);

		break;
	case OT_BACKBONE_ROUTER_NDPROXY_CLEARED:
		LOG_DBG("ND Proxy: removing all entires");

		entry_clear_all();
		break;
	}
}

void nd_proxy_enable(void)
{
	struct tbr_context *ctx = tbr_get_context();

	sys_slist_init(&entries);

	/** TODO: Start after Domain Prefix is set **/
	net_icmpv6_register_handler(&icpmv6_ns_handler);
	otBackboneRouterSetNdProxyCallback(ctx->ot->instance, handle_nd_proxy_event, NULL);
}
