/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <suit_cache.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>
#include "zcbor_encode.h"
#include <zephyr/sys/byteorder.h>
#include <zephyr/devicetree.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/util_macro.h>

LOG_MODULE_REGISTER(suit_cache, CONFIG_SUIT_LOG_LEVEL);

static bool init_done = false;
struct suit_cache suit_cache;

#define SUCCESS 0

static bool uricmp(struct zcbor_string current_key, const struct zcbor_string *uri);
static bool search_partition(struct suit_cache_partition *cache_partition,
			     const struct zcbor_string *uri, struct zcbor_string *payload);

static bool uricmp(struct zcbor_string current_key, const struct zcbor_string *uri)
{
	/* Check what type of string is uri */
	if (uri->value[uri->len - 1] == '\0') {
		/* uri is null terminated string */
		if ((uri->len - 1) != current_key.len) {
			return false;
		}
	} else {
		/* uri is zcbor tstr */
		if (uri->len != current_key.len) {
			return false;
		}
	}

	return !strncmp(current_key.value, uri->value, current_key.len);
}

static bool search_partition(struct suit_cache_partition *cache_partition,
			     const struct zcbor_string *uri, struct zcbor_string *payload)
{
	bool ret = true;
	zcbor_state_t states[3];
	struct zcbor_string current_key;
	struct zcbor_string current_data;

	if ((cache_partition != NULL) && (uri != NULL) && (payload != NULL)) {
		zcbor_new_state(states, sizeof(states) / sizeof(zcbor_state_t),
				cache_partition->address, cache_partition->size, 1);
		ret = zcbor_map_start_decode(states);

		do {
			ret = ret && (((zcbor_tstr_decode(states, &current_key))) &&
				      (zcbor_bstr_decode(states, &current_data)));

			if ((ret) && uricmp(current_key, uri)) {
				/* Matching uri was found */
				payload->value = (uint8_t *)current_data.value;
				payload->len = current_data.len;
				break;
			}
		} while (ret);

		zcbor_list_map_end_force_decode(states);
		zcbor_map_end_decode(states);

		return ret;
	}

	LOG_ERR("Invalid argument.");
	return false;
}

suit_plat_err_t suit_cache_search(const struct zcbor_string *uri, struct zcbor_string *payload)
{
	LOG_INF("suit_cache.partitions_count: %u", suit_cache.partitions_count);
	if ((uri != NULL) && (payload != NULL)) {
		for (size_t i = 0; i < suit_cache.partitions_count; i++) {
			if (search_partition(&suit_cache.partitions[i], uri, payload)) {
				return SUIT_PLAT_SUCCESS;
			}
		}

		return SUIT_PLAT_ERR_NOT_FOUND;
	}

	return SUIT_PLAT_ERR_INVAL;
}

void suit_cache_clear(struct suit_cache *cache)
{
	if (cache == NULL) {
		return;
	}

	for (size_t i = 0; i < cache->partitions_count; i++) {
		cache->partitions[i].address = NULL;
		cache->partitions[i].size = 0;
	}
}

suit_plat_err_t suit_cache_copy(struct suit_cache *dst_cache, const struct suit_cache *src_cache)
{
	if (src_cache->partitions_count > CONFIG_SUIT_CACHE_MAX_CACHES) {
		return SUIT_PLAT_ERR_NOMEM;
	}

	dst_cache->partitions_count = src_cache->partitions_count;
	memcpy(&(dst_cache->partitions), src_cache->partitions,
	       src_cache->partitions_count * sizeof(struct suit_cache_partition));

	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_cache_initialize(struct suit_cache *cache)
{
	suit_plat_err_t ret = suit_cache_copy(&suit_cache, cache);

	if (ret != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Copying cache from storage, failed: %i", ret);
		return ret;
	}

	init_done = true;

	return SUIT_PLAT_SUCCESS;
}

void suit_cache_deinitialize(void)
{
	suit_cache_clear(&suit_cache);
	init_done = false;
}
