/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief nRF Thread Border Router's multicast routing
 */

#ifndef TBR_INCLUDE_MCAST_ROUTING_H_
#define TBR_INCLUDE_MCAST_ROUTING_H_

#include <openthread/instance.h>

#include <zephyr/net/net_if.h>

/**
 * @brief Initialize multicast routing module
 */
void mcast_routing_init(void);

/**
 * @brief Handle changes in OpenThread's state
 *
 * @param instance  OT instance
 * @param flags     Flags that have changed
 */
void mcast_routing_handle_ot_state(struct otInstance *instance, otChangedFlags flags);

#endif /* TBR_INCLUDE_MCAST_ROUTING_H_ */
