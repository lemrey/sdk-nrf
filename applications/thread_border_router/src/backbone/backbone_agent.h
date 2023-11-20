/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief nRF Thread Border Router's Backbone Agent
 */

#ifndef TBR_INCLUDE_BACKBONE_AGENT_H_
#define TBR_INCLUDE_BACKBONE_AGENT_H_

#define TBR_BACKBONE_AGENT_BBR_PORT 61631

/**
 * @brief Initialize Backbone Agent
 */
void backbone_agent_init(void);

#endif /* TBR_INCLUDE_BACKBONE_AGENT_H_ */
