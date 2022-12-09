/*
 * Copyright (c) 2022 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-4-Clause
 */

#ifndef SUIT_STORAGE_H__
#define SUIT_STORAGE_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>


/**
 * @brief Initialize the SUIT storage driver.
 *
 * @return 0 on success, negative value otherwise.
 */
int suit_storage_init(void);

/**
 * @brief Check if there is a new update available.
 *
 * @return True if the new update is available, false otherwise.
 */
bool suit_storage_update_available(void);

/**
 * @brief Get the address and size of the DFU partition, containing update candidate.
 *
 * @param[out]  addr  DFU partition address
 * @param[out]  size  DFU partition size
 *
 * @return 0 on success, negative value otherwise.
 */
int suit_storage_update_get(void **addr, size_t *size);

/**
 * @brief Save the information about update candidate.
 *
 * @param[in]  addr  DFU partition address.
 * @param[in]  size  DFU partition size.
 *
 * @return 0 on success, negative value otherwise.
 */
int suit_storage_update_set(void *addr, size_t size);

/**
 * @brief Clear the flag, indicating availability of an update.
 *
 * @return 0 on success, negative value otherwise.
 */
int suit_storage_update_clear(void);

/**
 * @brief Get the address and size of the envelope, stored inside the SUIT partition.
 *
 * @param[in]   index  Index of the stored envelope to get.
 * @param[out]  addr   SUIT envelope address.
 * @param[out]  size   SUIT envelope size.
 *
 * @return 0 on success, negative value otherwise.
 */
int suit_storage_installed_envelope_get(size_t index, void **addr, size_t *size);

/**
 * @brief Install the authentication block and manifest of the envelope inside the SUIT storage.
 *
 * @note This API removes all severable elements of the SUIT enevelope, such as integrated
 *       payloads, text fields, etc.
 *
 * @param[in]  index  Index of the envelope list inside the storage to use.
 * @param[in]  addr   SUIT envelope address.
 * @param[in]  size   SUIT envelope size.
 *
 * @return 0 on success, negative value otherwise.
 */
int suit_storage_install_envelope(size_t index, void *addr, size_t size);

#endif /* SUIT_STORAGE_H__ */
