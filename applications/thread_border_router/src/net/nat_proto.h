/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Protocol handlers for NAT.
 */

#ifndef NAT_PROTO_H_
#define NAT_PROTO_H_

#include <zephyr/types.h>
#include <zephyr/net/net_pkt.h>

/**
 * @brief ICMP packets handler for NAT.
 *
 * @param pkt   Network packet.
 * @param tx    Transmission direction flag.
 *
 * @return Return verdict about the packet.
 */
enum net_verdict nat_icmp(struct net_pkt *pkt, bool tx);

/**
 * @brief UDP packets handler for NAT.
 *
 * @param pkt   Network packet.
 * @param tx    Transmission direction flag.
 *
 * @return Return verdict about the packet.
 */
#if defined(CONFIG_NET_UDP)

enum net_verdict nat_udp(struct net_pkt *pkt, bool tx);

#else /* CONFIG_NET_UDP */

static inline enum net_verdict nat_udp(struct net_pkt *pkt, bool tx)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(tx);

	return NET_DROP;
}

#endif  /* CONFIG_NET_UDP */

#endif  /* NAT_PROTO_H_ */
