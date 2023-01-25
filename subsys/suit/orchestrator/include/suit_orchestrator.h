/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-4-Clause
 */

/**
 * @brief Initialize the SUIT orchestrator before it can be used.
 *
 * @return 0 on success, non-zero value otherwise.
 */
int suit_orchestrator_init(void);

/**
 * @brief Main function of the orchestrator, starting boot or update.
 *
 * @return 0 on success, negative value otherwise.
 */
int suit_orchestrator_entry(void);
