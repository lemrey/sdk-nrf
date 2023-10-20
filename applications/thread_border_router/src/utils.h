/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Utility functions for nRF TBR.
 */

#ifndef NRF_TBR_UTILS_H_
#define NRF_TBR_UTILS_H_

#include <zephyr/types.h>

/**
 * @brief Update checksum for changed data.
 *
 * @param sum_in	Old checksum to recompute.
 * @param data_old	Old value.
 * @param data_new	New value.
 * @param len		Number of bytes which have been replaced.
 *
 * @return New checksum.
 */
uint16_t update_chksum(uint16_t sum_in, const uint8_t *data_old,
		       const uint8_t *data_new, size_t len);

#endif /* NRF_TBR_UTILS_H_ */
