/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DFU_CACHE_RW_H__
#define DFU_CACHE_RW_H__

#include <stddef.h>
#include <stdint.h>
#include <zcbor_decode.h>
#include <zephyr/storage/flash_map.h>
#include <dfu_cache.h>

#ifdef __cplusplus
extern "C" {
#endif

/* This header is meant to be used only from APP core context.
 * It extends dfu_cache by adding to cache write and erase capabilities.
 */

struct suit_cache_slot {
	size_t slot_offset;
	size_t size;
	size_t size_offset;
	size_t data_offset;
	size_t eb_size;
};

/**
 * @brief Initialize cache in R/W mode for APP context
 *
 * @note dfu_cache structure is initialized based on dfu_partitions_ext which in turn is
 *       initialized using SUIT cache partitions information from Device Tree.
 *
 * @param addr  DFU partition address
 * @param size  DFU partition size
 *
 * @return SUIT_PLAT_SUCCESS in case of success, otherwise error code
 */
suit_plat_err_t suit_dfu_cache_initialize_rw(void *addr, size_t size);

/**
 * @brief Deinitialize SUIT cache
 */
void suit_dfu_cache_deinitialize_rw(void);

/**
 * @brief Function tries to allocate slot in cache pointed by ID
 *
 * @param cache_partition_id ID of the cache partition in which slot should be allocated
 * @param slot Pointer to structure that will be filled with allocated slot info
 * @param uri URI that will be used as a key in cache cbor map
 * @param uri_size URI size
 *
 * @return SUIT_PLAT_SUCCESS in case of success, otherwise error code
 */
suit_plat_err_t dfu_create_cache_slot(uint8_t cache_partition_id, struct suit_cache_slot *slot,
				      const uint8_t *uri, size_t uri_size);

/**
 * @brief Closes slot by updating bstring header with size of data that was written
 *        and moves CBOB indefinite map end marker to new end of cache partition.
 *
 * @param slot Structure containing info about opened slot
 * @param data_end_offset Offset pointing to the end of data written in slot
 *
 * @return SUIT_PLAT_SUCCESS in case of success, otherwise error code
 */
suit_plat_err_t dfu_commit_cache_slot(struct suit_cache_slot *slot, size_t data_end_offset);

/**
 * @brief Commits changes written to slot by updating cbor header for the cache slot
 *
 * @param slot Pointer to opened cache slot
 * @param data_end_offset Offset of where written data ends
 *
 * @return SUIT_PLAT_SUCCESS in case of success, otherwise error code
 */
suit_plat_err_t dfu_close_cache_slot(struct suit_cache_slot *slot, size_t data_end_offset);

/**
 * @brief Drop data written to slot and revert slot allocation
 *
 * @param slot Pointer to slot that should be dropped
 *
 * @return SUIT_PLAT_SUCCESS in case of success, otherwise error code
 */
suit_plat_err_t dfu_drop_cache_slot(struct suit_cache_slot *slot);

#ifdef __cplusplus
}
#endif

#endif /* DFU_CACHE_RW_H__ */
