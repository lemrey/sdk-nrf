/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef IUICC_H
#define IUICC_H

#include <stdint.h>
#include <stddef.h>

/* Maximum allowed Le */
#define SIM_HAL_MAX_LE 512

/**
 * @brief Function for card activation.
 * @param[in] data		buffer of maximum size SIM_HAL_MAX_LE to be used for copying response
 * 				ATR data into.
 * @param[in] p_data_len	pointer to the current size of the @p data buffer.
 *				Needs to be updated based on the updated content
 *				of the @p data buffer.
 *
 * @retval			0 if successful, -1 if unsuccessful.
 */
int iuicc_card_activate(uint8_t *data, size_t *p_data_len);

/**
 * @brief Function for card transport APDU.
 * @param[in] data		buffer of maximum size SIM_HAL_MAX_LE containing incoming APDU data,
 * 				and to be used for outgoing APDU responses.
 * @param[in] p_data_len	pointer to the current size of the @p data buffer.
 *				Needs to be updated based on the updated content
 *				of the @p data buffer.
 *
 * @retval			0 if successful, -1 if unsuccessful.
 */
int iuicc_transport_apdu(uint8_t *data, size_t *p_data_len);

/**
 * @brief Function for card deactivation.
 *
 * @retval			0 if successful, -1 if unsuccessful.
 */
int iuicc_card_deactivate(void);

#endif //IUICC_H
