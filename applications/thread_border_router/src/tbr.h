/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Miscellaneous nRF Thread Border Router related functions
 */

#ifndef NRF_TBR_INCLUDE_TBR_H_
#define NRF_TBR_INCLUDE_TBR_H_

#include <zephyr/net/openthread.h>

/**
 * @brief nRF TBR's context
 *
 * This structure stores miscellaneous data that is related to TBR - * pointers
 * to interfaces, states, addresses etc. The is a single instance of this structure
 * and its pointer can be retreived by other modules using @ref tbr_get_context()
 */
struct tbr_context {
	struct net_if *backbone_iface;
	struct in6_addr *ll_addr;
	struct openthread_context *ot;
};

/**
 * @brief Get pointer to the singleton instance of @ref tbr_context
 */
struct tbr_context *tbr_get_context(void);

#endif /* NRF_TBR_INCLUDE_TBR_H_ */
