/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUIT_CACHE_H__
#define SUIT_CACHE_H__

#include <stddef.h>
#include <stdint.h>
#include <zcbor_decode.h>
#include <zephyr/storage/flash_map.h>
#include <suit_plat_err.h>

/* This header has no restrictions on the context in which it can be used.
 * Contains functions allowing searching and reading from cache.
 *
 *			┌────────────────────────────────────────────────────────────────────┐
 *			│                             SUIT CACHE                             │
 *			│                                                                    │
 *			│ ┌──────────────────────────────┐  ┌──────────────────────────────┐ │
 *			│ │  DFU PARTITION: suit_cache_1 │  │  DFU PARTITION: suit_cache_3 │ │
 *			│ │     (Indefinite CBOR map)    │  │    (Indefinite CBOR map)     │ │
 *			│ │                              │  │                              │ │
 *			│ │┌────────────────────────────┐│  │┌────────────────────────────┐│ │
 *			│ ││       SUIT CACHE SLOT 1    ││  ││       SUIT CACHE SLOT 1    ││ │
 *			│ ││ (single entry in CBOR map) ││  ││ (single entry in CBOR map) ││ │
 *			│ ││┌──────────────────────────┐││  ││┌──────────────────────────┐││ │
 *			│ │││       UNIQUE URI_1       │││  │││       UNIQUE URI_3       │││ │
 *			│ ││└──────────────────────────┘││  ││└──────────────────────────┘││ │
 *			│ ││┌──────────────────────────┐││  ││┌──────────────────────────┐││ │
 *			│ │││           DATA           │││  │││           DATA           │││ │
 *			│ ││└──────────────────────────┘││  ││└──────────────────────────┘││ │
 *			│ │└────────────────────────────┘│  │└────────────────────────────┘│ │
 *			│ │┌────────────────────────────┐│  │┌────────────────────────────┐│ │
 *			│ ││      SUIT CACHE SLOT 2     ││  ││      SUIT CACHE SLOT 2     ││ │
 *			│ ││ (single entry in CBOR map) ││  ││ (single entry in CBOR map) ││ │
 *			│ ││┌──────────────────────────┐││  ││┌──────────────────────────┐││ │
 *			│ │││       UNIQUE URI_2       │││  │││       UNIQUE URI_4       │││ │
 *			│ ││└──────────────────────────┘││  ││└──────────────────────────┘││ │
 *			│ ││┌──────────────────────────┐││  ││┌──────────────────────────┐││ │
 *			│ │││           DATA           │││  │││           DATA           │││ │
 *			│ ││└──────────────────────────┘││  ││└──────────────────────────┘││ │
 *			│ │└────────────────────────────┘│  │└────────────────────────────┘│ │
 *			│ └──────────────────────────────┘  └──────────────────────────────┘ │
 *			└────────────────────────────────────────────────────────────────────┘
 */

/* SUIT cache is meant as a storage used during update process. It can be used to fetch
 * payloads from external sources. SUIT cache consists of cache partitions that can be defined
 * across all available nonvolatile memories. Each cache partition can hold multiple images. SUIT
 * cache slot represents area occupied by single image with its identifier (URI stored as CBOR
 * tstring). In short, SUIT cache consists of partitions, partition contains slots and one slot
 *holds one "image". As a rule, suit_cache_0 is predefined and occupies free space on the DFU
 *partition, right after the envelope. If there's no envelope in DFU partition, suit_cache_0
 *initialization will fail and it will be unavailable . suit_cache_0 size is variable. Other caches
 *can be defined by the user via device tree and must follow the naming convention: suit_cache_N
 *where N is a ordinal number starting at 1.
 *
 * Slot allocation creates initial entry in cache map with tstr uri as key, followed by header
 *		byte 0x5A and 4 bytes of size. Size is left unwritten.
 * Slot closing takes size of data that was written and updates byte string size with that value.
 *		The indefinite end tag (0xFF), which determines the end of the partition, is written
 *		at the end.
 *
 * suit_cache_slot: Describes new allocated/open slot in cache
 *		- slot_offset: Absolute offset of the slot in the memory. Where the slot header
 *			       starts.
 *		- size: It follows how much data is written and that value is used to update size in
 *			cache
 *		- slot header when the slot is being closed
 *		- size_offset: offset from slot_offset, points to where size value in header should
 *			       be written
 *		- data_offset: offset from slot_offset, points to where data begins in slot
 *		- fdev: pointer to driver associated with cache partition on which slot lays
 *
 * suit_cache_partition: Describes single cache partition
 *		- size: Maximum size of cache, SUIT cache partition size
 *		- offset: offset were cache begins
 *
 * suit_cache: Contains list of all registered cache partitions - cache pool
 *		- partitions_count: Number of registered cache partitions
 *		- partitions: List containing registered partitions
 */

/* IPC contract */
struct suit_cache_partition { /* Structure describing single cache partition */
	size_t size;
	uint8_t *address;
};

struct suit_cache {
	uint8_t partitions_count;
	struct suit_cache_partition partitions[CONFIG_SUIT_CACHE_MAX_CACHES];
};

/**
 * @brief Initializes SUIT cache in readonly mode for use in contexts other than APP
 *
 * @param cache  Pointer to the SUIT cache structure.
 *
 * @return SUIT_PLAT_SUCCESS in case of success, otherwise error code
 */
suit_plat_err_t suit_cache_initialize(struct suit_cache *cache);

/**
 * @brief Deinitialize SUIT cache
 */
void suit_cache_deinitialize(void);

/**
 * @brief Clear SUIT cache.
 *
 * @param cache  Pointer to the SUIT cache structure.
 */
void suit_cache_clear(struct suit_cache *cache);

/**
 * @brief Copy the pointers between SUIT cache structures.
 *
 * @note This operation does not make a copy of the data, pointed by the caches.
 *       It copies the pointers to the data, so the contents may be referred from both of the
 *       structures.
 *
 * @param dst_cache  Destination cache to be aligned.
 * @param src_cache  Source cache to be copied.
 *
 * @return SUIT_PLAT_SUCCESS in case of success, otherwise error code
 */
suit_plat_err_t suit_cache_copy(struct suit_cache *dst_cache, const struct suit_cache *src_cache);

/**
 * @brief Search registered caches for data with equal uri key.
 *
 * @param uri      Uri key value to look for.
 * @param payload  Return pointer to data object if found.
 *
 * @return SUIT_PLAT_SUCCESS in case of success, otherwise error code
 */
suit_plat_err_t suit_cache_search(const struct zcbor_string *uri, struct zcbor_string *payload);

#endif /* SUIT_CACHE_H__ */
