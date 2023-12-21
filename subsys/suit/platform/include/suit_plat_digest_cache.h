/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Module that contains a cache holding pointers to digests.
 *
 * In some cases the firmware needs to quickly access the digests for images
 * which have already been verified. To avoid having to go through the execution
 * of the whole manifest in order to obtain these digests caching was added.
 *
 * Instead of copying whole digests into the cache pointers to the digests present in the
 * envelope kept in the non volatile memory are kept in the cache.
 * This can be done under one assumption - the memory holding the given digests
 * MUST NOT be changed as long as the pointer to this digest is kept in the cache.
 * This assumption is currently met, as the memory which holds the envelope
 * during normal operation is only changed after the firmware update starts
 * - at the start of these procedure the cache should be cleared and during this
 * procedure caching of digests should be disabled.
 */


#ifndef SUIT_PLAT_DIGEST_CACHE_H__
#define SUIT_PLAT_DIGEST_CACHE_H__

#include <suit_types.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enable modifications of the the cache.
 *
 * @note By default the cache is in "locked" state, which means no modifications are possible.
 */
void suit_plat_digest_cache_unlock(void);

/**
 * @brief Disable modifications of the the cache.
 *
 * @note By default the cache is in "locked" state, which means no modifications are possible.
 */
void suit_plat_digest_cache_lock(void);

/**
 * @brief Check if the cache is unlocked and its contents can be modified.
 *
 * @retval true  The contents of the cache can be modified.
 * @retval false The contents of the cache cannot be modified.
 */
bool suit_plat_digest_cache_is_unlocked(void);

/**
 * @brief Add a (component_id, digest) pair to the cache.
 *        If the given component_id is already present in the cache, update the cached digest.
 *
 * @note Neither the component_id nor the digest bytes will be copied.
 *       Instead links to the memory area pointed by the .value fields of the zbor_string
 *       structures will be added to the cache.
 *       The user of this API must make sure that the contents of the memory to which these
 *       parameters point are not changed as long as the entry is present in the cache.
 *
 * @param[in] component_id CBOR string containing the Component ID data.
 * @param[in] digest       CBOR string containing the digest for the given Component ID.
 *
 * @retval SUIT_SUCCESS                   Entry added or updated successfully
 * @retval SUIT_ERR_UNSUPPORTED_COMMAND   The cache is locked
 * @retval SUIT_ERR_UNSUPPORTED_PARAMETER One of the parameters is not a valid pointer
 * @retval SUIT_ERR_OVERFLOW              The cache is full
 */
int suit_plat_digest_cache_add(struct zcbor_string *component_id, struct zcbor_string *digest);

/**
 * @brief Invalidate element in the cache
 *
 * @param[in] component_id CBOR string containing the Component ID data.
 *
 * @retval SUIT_SUCCESS	                Invalidating the element was successful
 * @retval SUIT_ERR_UNSUPPORTED_COMMAND The cache is locked
 */
int suit_plat_digest_cache_remove(struct zcbor_string *component_id);

/**
 * @brief Check if the digest stored in the cache for a given component is matching
 *        the provided digest.
 *
 * @param[in]  component_id The Component ID.
 * @param[out] digest       The CBOR string containing the digest to compare against.
 *
 * @retval SUIT_SUCCESS                   The provided digest and the digest stored for the
 *                                        given Component ID are matching
 * @retval SUIT_FAIL_CONDITION            The provided digest does not match the digest stored
 *                                        for the given Component ID
 * @retval SUIT_ERR_MISSING_COMPONENT     The given Component ID was not found in the cache
 */
int suit_plat_digest_cache_compare(const struct zcbor_string *component_id,
				   const struct zcbor_string * digest);

/**
 * @brief Invalidate all elements in the cache,
 *
 * @retval SUIT_SUCCESS	                Invalidating the cache was successful
 * @retval SUIT_ERR_UNSUPPORTED_COMMAND The cache is locked
 */
int suit_plat_digest_cache_remove_all(void);

/**
 * @brief This is an additional utility function which takes a component handle
 *        and removes the entry corresponding to a given component.
 *
 * @param[in] handle The component handle
 *
 * @retval SUIT_SUCCESS	                     Invalidating the element was successful
 * @retval SUIT_ERR_UNSUPPORTED_COMPONENT_ID The handle does not correspond to a valid
 *                                           Component ID.
 * @retval SUIT_ERR_UNSUPPORTED_COMMAND      The cache is locked
 */
int suit_plat_digest_cache_remove_by_handle(suit_component_t handle);

#ifdef __cplusplus
}
#endif

#endif /* SUIT_PLAT_DIGEST_CACHE_H__ */
