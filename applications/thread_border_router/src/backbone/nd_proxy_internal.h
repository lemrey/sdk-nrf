/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief nRF Thread Border Router's Neighbor Discovery Proxy internals
 */

#ifndef TBR_INCLUDE_ND_PROXY_INTERNAL_H_
#define TBR_INCLUDE_ND_PROXY_INTERNAL_H_

#include <zephyr/net/net_ip.h>
#include <zephyr/sys/slist.h>

struct nd_proxy_entry {
	struct in6_addr dua;
	struct net_if_mcast_addr *maddr;
	sys_snode_t node;
};

#define ND_PROXY_SLAB_BLOCK_SIZE (sizeof(struct nd_proxy_entry))
#define ND_PROXY_SLAB_BLOCK_CNT (CONFIG_NRF_TBR_ND_PROXY_ENTRY_COUNT)
#define ND_PROXY_SLAB_BLOCK_ALIGN (4)

#endif /* TBR_INCLUDE_ND_PROXY_INTERNAL_H_ */
