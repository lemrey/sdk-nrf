/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUIT_STORAGE_H__
#define SUIT_STORAGE_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <suit_mci.h>
#include <suit_mreg.h>
#include <suit_plat_err.h>

/**
 * @brief Initialize the SUIT storage driver.
 *
 * @param[in]  supported_class_id  List of references to the supported manifest class IDs.
 * @param[in]  len                 Length of the list.
 *
 * @return SUIT_PLAT_SUCCESS in case of success, otherwise error code
 */
suit_plat_err_t suit_storage_init(const suit_manifest_class_id_t **supported_class_id, size_t len);

/**
 * @brief Get the memory regions, containing update candidate.
 *
 * @param[out]  regions  List of update candidate memory regions (envelope, caches).
 *                       By convention, the first region holds the SUIT envelope.
 * @param[out]  len      Length of the memory regions list.
 *
 * @return SUIT_PLAT_SUCCESS in case of success, otherwise error code
 */
suit_plat_err_t suit_storage_update_cand_get(const suit_plat_mreg_t **regions, size_t *len);

/**
 * @brief Save the information about update candidate.
 *
 * @param[in]  regions  List of update candidate memory regions (envelope, caches).
 *                      By convention, the first region holds the SUIT envelope.
 * @param[in]  len      Length of the memory regions list.
 *
 * @return SUIT_PLAT_SUCCESS in case of success, otherwise error code
 */
suit_plat_err_t suit_storage_update_cand_set(suit_plat_mreg_t *regions, size_t len);

/**
 * @brief Get the address and size of the envelope, stored inside the SUIT partition.
 *
 * @param[in]   id     Class ID of the manifest inside the envelope.
 * @param[out]  addr   SUIT envelope address.
 * @param[out]  size   SUIT envelope size.
 *
 * @return SUIT_PLAT_SUCCESS in case of success, otherwise error code
 */
suit_plat_err_t suit_storage_installed_envelope_get(const suit_manifest_class_id_t *id,
												    uint8_t **addr, size_t *size);

/**
 * @brief Install the authentication block and manifest of the envelope inside the SUIT storage.
 *
 * @note This API removes all severable elements of the SUIT envelope, such as integrated
 *       payloads, text fields, etc.
 *
 * @param[in]  id     Class ID of the manifest inside the envelope.
 * @param[in]  addr   SUIT envelope address.
 * @param[in]  size   SUIT envelope size.
 *
 * @return SUIT_PLAT_SUCCESS in case of success, otherwise error code
 */
suit_plat_err_t suit_storage_install_envelope(const suit_manifest_class_id_t *id, uint8_t *addr,
											  size_t size);

#endif /* SUIT_STORAGE_H__ */
