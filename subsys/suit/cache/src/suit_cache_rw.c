/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <suit_cache_rw.h>
#include <zephyr/drivers/flash.h>
#include <suit_plat_mem_util.h>
#include <zephyr/logging/log.h>
#include "zcbor_encode.h"
#include <zephyr/sys/byteorder.h>
#include <zephyr/devicetree.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/util_macro.h>

LOG_MODULE_REGISTER(suit_cache_rw, CONFIG_SUIT_LOG_LEVEL);

#define SUCCESS 0

extern struct suit_cache suit_cache;

struct suit_cache_partition_ext { /* Extended structure describing single cache partition */
	const struct device *fdev;
	size_t size;
	size_t offset;
	size_t eb_size;
	uint8_t id;
	const size_t erase_block_size;
};

#define PARTITION_INIT(index, label)                                                               \
	{                                                                                          \
		.fdev = FIXED_PARTITION_DEVICE(label),                                             \
		.offset = FIXED_PARTITION_OFFSET(label),                                           \
		.size = FIXED_PARTITION_SIZE(label),                                               \
		.eb_size = FLASH_AREA_ERASE_BLOCK_SIZE(label),                                     \
		.id = index,                                                                       \
	},

#define INDEX_IN_RAGE(index) IN_RANGE(index, 1, (CONFIG_SUIT_CACHE_MAX_CACHES - 1))
#define PARTITION_IS_USABLE(label)                                                                 \
	UTIL_AND(FIXED_PARTITION_EXISTS(label),                                                    \
		 DT_NODE_HAS_STATUS(DT_MTD_FROM_FIXED_PARTITION(DT_NODELABEL(label)), okay))

#define PARTITION_INIT_IF_INDEX_OK(label, index)                                                   \
	IF_ENABLED(UTIL_BOOL(INDEX_IN_RANGE(index)), (PARTITION_INIT(index, label)))

#define PARTITION_DEFINE_(index, label)                                                            \
	IF_ENABLED(PARTITION_IS_USABLE(label), (PARTITION_INIT_IF_INDEX_OK(label, index)))

#define PARTITION_DEFINE(index, prefix) PARTITION_DEFINE_(index, prefix##index)

static struct suit_cache_partition_ext dfu_partitions_ext[] = {
	{
		.fdev = FIXED_PARTITION_DEVICE(dfu_partition),
		.offset = FIXED_PARTITION_OFFSET(dfu_partition),
		.size = FIXED_PARTITION_SIZE(dfu_partition),
		.eb_size = FLASH_AREA_ERASE_BLOCK_SIZE(dfu_partition),
		.id = 0,
	},
	LISTIFY(CONFIG_SUIT_CACHE_MAX_CACHES, PARTITION_DEFINE, (), suit_cache_)};

static uint8_t erase_cache[FLASH_AREA_ERASE_BLOCK_SIZE(dfu_partition)];

#define ENCODING_OUTPUT_BUFFER_LENGTH                                                              \
	(CONFIG_SUIT_MAX_URI_LENGTH + 5) /* Adding 5 bytes for bstring header */

static suit_plat_err_t initialize_partition(struct suit_cache_partition_ext *part);
static suit_plat_err_t update_cache_0(void *address, size_t size);
static struct suit_cache_partition_ext *get_cache_partition(uint8_t partition_id);
static suit_plat_err_t allocate_slot_in_cache_partition(const struct zcbor_string *uri,
					 		struct suit_cache_slot *slot,
					 		struct suit_cache_partition_ext *part);
static suit_plat_err_t cache_check_free_space(struct suit_cache_partition_ext *part,
					      struct suit_cache_slot *slot);

suit_plat_err_t suit_cache_initialize_rw(void *addr, size_t size)
{
	suit_plat_err_t ret = update_cache_0(addr, size);

	if (ret != SUIT_PLAT_SUCCESS) {
		return ret;
	}

	suit_cache.partitions_count = ARRAY_SIZE(dfu_partitions_ext);

	for (size_t i = 1; i < suit_cache.partitions_count; i++) {
		ret = initialize_partition(&dfu_partitions_ext[i]);

		if (ret != SUIT_PLAT_SUCCESS) {
			LOG_WRN("suit_cache_%u initialization failed: %i", dfu_partitions_ext[i].id,
				ret);
		}
	}

	for (size_t i = 0; i < suit_cache.partitions_count; i++) {
		suit_cache.partitions[i].size = dfu_partitions_ext[i].size;
		suit_cache.partitions[i].address =
			suit_plat_mem_nvm_ptr_get(dfu_partitions_ext[i].offset);
		LOG_DBG("Found partition %d: (addr: %p, size: 0x%x)", i,
			(void *)suit_cache.partitions[i].address, suit_cache.partitions[i].size);
	}

	return SUIT_PLAT_SUCCESS;
}

void suit_cache_deinitialize_rw(void)
{
	suit_cache_clear(&suit_cache);
}

/**
 * @brief Creates indefinite map at given offset by adding appropriate header and end marker
 *
 * @param part Pointer to cache partition structure
 * @return SUIT_PLAT_SUCCESS on success, otherwise error code
 */
static suit_plat_err_t initialize_partition(struct suit_cache_partition_ext *part)
{
	const uint8_t header[] = {0xBF,	 /* Indefinite length map */
				  0xFF}; /* End marker */

	if (part != NULL) {
		if (part->size > sizeof(header)) {
			if (!device_is_ready(part->fdev)) {
				LOG_ERR("Flash device not ready.");
				return SUIT_PLAT_ERR_IO;
			}

			int ret = flash_write(part->fdev, part->offset, header, sizeof(header));

			if (ret != 0) {
				LOG_ERR("Writing header and end marker failed. %i", ret);
				return SUIT_PLAT_ERR_IO;
			}

			return SUIT_PLAT_SUCCESS;
		}

		LOG_ERR("Partition too small to fit CBOR headers (%d)", part->size);
		return SUIT_PLAT_ERR_NOMEM;
	}

	LOG_ERR("Invalid argument.");
	return SUIT_PLAT_ERR_INVAL;
}

/**
 * @brief Get cache partition of specified id
 *
 * @param partition_id Integer from partition label from dts.
 *		For example if partition label is suit_cache_3 than 3 is partition id.
 * @return struct suit_cache_partition_ext* In case of success pointer to partition or
 *		NULL if requested partition was not found
 */
static struct suit_cache_partition_ext *get_cache_partition(uint8_t partition_id)
{
	for (size_t i = 0; i < ARRAY_SIZE(dfu_partitions_ext); i++) {
		if (dfu_partitions_ext[i].id == partition_id) {
			return &dfu_partitions_ext[i];
		}
	}

	return NULL;
}

/**
 * @brief Check size of available free space in given cache and get allocable slot info
 *
 * @param cache Pointer to structure with information about single cache
 * @param slot Pointer to structure that will contain allocable slot info
 * @return SUIT_PLAT_SUCCESS on success, otherwise error code
 */
static suit_plat_err_t cache_check_free_space(struct suit_cache_partition_ext *part,
					      struct suit_cache_slot *slot)
{
	bool ret = true;
	zcbor_state_t states[3];
	struct zcbor_string current_key;
	struct zcbor_string current_data;
	size_t part_tmp_offset;

	if ((part != NULL) && (slot != NULL)) {
		part_tmp_offset = part->offset;

		zcbor_new_state(states, sizeof(states) / sizeof(zcbor_state_t),
				suit_plat_mem_nvm_ptr_get(part->offset), part->size, 1);
		ret = zcbor_map_start_decode(states);

		do {
			ret = ret && (((zcbor_tstr_decode(states, &current_key))) &&
				      (zcbor_bstr_decode(states, &current_data)));

			if (ret) {
				part_tmp_offset = suit_plat_mem_nvm_offset_get(
					(uint8_t *)(current_data.value + current_data.len));
				break;
			}
		} while (ret);

		zcbor_list_map_end_force_decode(states);
		zcbor_map_end_decode(states);

		for (; part_tmp_offset < (part->offset + part->size); part_tmp_offset++) {
			/* cache is indefinite-length map, so to find end we look for 0xFF marker */
			if ((*suit_plat_mem_nvm_ptr_get(part_tmp_offset)) == 0xFF) {
				slot->size = part->offset + part->size - part_tmp_offset;
				slot->slot_offset = part_tmp_offset;

				return SUIT_PLAT_SUCCESS;
			}
		}

		LOG_ERR("Indefinite map marker not found in searched cache.");
		return SUIT_PLAT_ERR_NOT_FOUND;
	}

	LOG_ERR("Invalid argument. NULL pointer.");
	return SUIT_PLAT_ERR_INVAL;
}

/**
 * @brief If possible allocate new slot in given cache partition
 *
 * @param uri URI that will be used as a key in cache map
 * @param slot Pointer to structure containing allocated slot inforamation
 * @param cache_index Index of cache in which slot shall be allocated
 * @return SUIT_PLAT_SUCCESS on success, otherwise error code
 */
static suit_plat_err_t allocate_slot_in_cache_partition(const struct zcbor_string *uri,
							struct suit_cache_slot *slot,
							struct suit_cache_partition_ext *part)
{
	size_t encoded_size = 0;
	uint8_t output[ENCODING_OUTPUT_BUFFER_LENGTH];
	zcbor_state_t states[3];

	if ((uri != NULL) && (slot != NULL) && (part != NULL)) {
		/* Check if uri is not a duplicate */
		struct zcbor_string payload;

		if (suit_cache_search(uri, &payload) == SUIT_PLAT_SUCCESS) {
			/* Key URI is a duplicate */
			LOG_ERR("Key URI already exists.");
			return SUIT_PLAT_ERR_EXISTS;
		}

		/* Check how much free space is in given cache partition*/
		suit_plat_err_t ret = cache_check_free_space(part, slot);

		if (ret != SUIT_PLAT_SUCCESS) {
			return ret;
		}

		zcbor_new_state(states, sizeof(states) / sizeof(zcbor_state_t), output,
				ENCODING_OUTPUT_BUFFER_LENGTH, 1);

		if (!zcbor_tstr_encode(states, uri)) {
			return SUIT_PLAT_ERR_CRASH;
		}

		encoded_size = MIN(ENCODING_OUTPUT_BUFFER_LENGTH,
				   (size_t)states[0].payload - (size_t)output);

		/* 0x5A - first byte that indicates byte string type of length written on following
		 *4 bytes 0x5A = 0x40(type 2) + 0x1A(26)
		 */
		output[encoded_size++] = 0x5A;
		slot->size_offset = encoded_size;

		/* Fill 4 size bytes to 0xFF so that they can be written latter during slot closing
		 */
		for (size_t i = 0; i < 4; i++, encoded_size++) {
			output[encoded_size] = 0xFF;
		}

		if (!device_is_ready(slot->fdev)) {
			LOG_ERR("Flash device not ready.");
			return SUIT_PLAT_ERR_IO;
		}

		if (slot->size < encoded_size) {
			LOG_ERR("Not enough free space in slot to write header.");
			return SUIT_PLAT_ERR_NOMEM;
		}

		int flash_ret = flash_write(slot->fdev, slot->slot_offset, output, encoded_size);

		if (ret != 0) {
			LOG_ERR("Writing cache slot header failed: %i", flash_ret);
			return SUIT_PLAT_ERR_IO;
		}

		slot->data_offset = encoded_size;

		return SUIT_PLAT_SUCCESS;
	}

	LOG_ERR("Invalid argument.");
	return SUIT_PLAT_ERR_INVAL;
}

/**
 * @brief Update size and offset of suit_cache_0 partition based of envelope location
 *
 * @param address  DFU partition address
 * @param size     DFU partition size
 *
 * @return SUIT_PLAT_SUCCESS on success, otherwise error code
 */
static suit_plat_err_t update_cache_0(void *address, size_t size)

{
	if ((address == NULL) || (size == 0)) {
		LOG_WRN("Initialized with empty DFU partition");
		dfu_partitions_ext[0].size = 0;
		return SUIT_PLAT_SUCCESS;
	}

	/* Check if update address is in dfu_partition range */
	if ((suit_plat_mem_nvm_offset_get(address) < dfu_partitions_ext[0].offset) ||
	    (suit_plat_mem_nvm_offset_get(address) >=
	     (dfu_partitions_ext[0].offset + dfu_partitions_ext[0].size))) {
		LOG_ERR("Envelope address doesn't match dfu_partition");
		return SUIT_PLAT_ERR_INVAL;
	}

	size_t tmp_offset = suit_plat_mem_nvm_offset_get(address) + size;

	if (tmp_offset >= (dfu_partitions_ext[0].offset + dfu_partitions_ext[0].size)) {
		LOG_WRN("No free space for cache");
		dfu_partitions_ext[0].size = 0;
	} else {
		/* Set offset to the end of the envelope */
		dfu_partitions_ext[0].offset = tmp_offset;

		/* Calculate remaining free space in dfu_partition */
		dfu_partitions_ext[0].size -= (suit_plat_mem_nvm_offset_get(address) + size);
	}

	if (dfu_partitions_ext[0].size > 0) {
		return initialize_partition(&dfu_partitions_ext[0]);
	}

	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t dfu_create_cache_slot(uint8_t cache_partition_id, struct suit_cache_slot *slot,
				      const uint8_t *uri, size_t uri_size)
{
	if ((slot != NULL) && (uri != NULL) && (uri_size > 0)) {
		struct zcbor_string tmp_uri = {.value = uri, .len = uri_size};

		if (uri[uri_size - 1] == '\0') {
			tmp_uri.len--;
		}

		struct suit_cache_partition_ext *part = get_cache_partition(cache_partition_id);

		if (part == NULL) {
			LOG_ERR("Partition not found");
			return SUIT_PLAT_ERR_NOT_FOUND;
		}

		slot->fdev = part->fdev;
		slot->eb_size = part->eb_size;
		suit_plat_err_t ret = allocate_slot_in_cache_partition(&tmp_uri, slot, part);

		if (ret != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Allocating slot in cache failed.");
			return ret;
		}

		return SUIT_PLAT_SUCCESS;
	}

	LOG_ERR("Invalid argument. NULL pointer.");
	return SUIT_PLAT_ERR_INVAL;
}

suit_plat_err_t dfu_close_cache_slot(struct suit_cache_slot *slot, size_t data_end_offset)
{
	if (slot != NULL) {
		if (slot->fdev == NULL) {
			LOG_ERR("Partition fdev is NULL");
			return SUIT_PLAT_ERR_INVAL;
		}

		if (!device_is_ready(slot->fdev)) {
			LOG_ERR("Flash device not ready.");
			return SUIT_PLAT_ERR_IO;
		}

		uint32_t tmp = __bswap_32(data_end_offset);

		/* Update byte string size */
		if (flash_write(slot->fdev, slot->slot_offset + slot->size_offset, &tmp,
				sizeof(uint32_t)) != 0) {
			LOG_ERR("Updating cache slot size in header failed.");
			return SUIT_PLAT_ERR_IO;
		}

		/* To be used as end marker */
		tmp = 0xFFFFFFFF;

		/* Add indefinite map, end marker 0xFF */
		if (flash_write(slot->fdev, slot->slot_offset + slot->data_offset + data_end_offset,
				&tmp, 1) != 0) {
			LOG_ERR("Writing CBOR map end marker to cache partition failed.");
			return SUIT_PLAT_ERR_IO;
		}

		return SUIT_PLAT_SUCCESS;
	}

	LOG_ERR("Invalid argument. NULL pointer.");
	return SUIT_PLAT_ERR_INVAL;
}

suit_plat_err_t dfu_drop_cache_slot(struct suit_cache_slot *slot, size_t data_end_offset)
{
	if ((slot != NULL) && (slot->fdev != NULL)) {
		if (!device_is_ready(slot->fdev)) {
			LOG_ERR("Flash device not ready.");
			return SUIT_PLAT_ERR_IO;
		}
		if (slot->eb_size > sizeof(erase_cache)) {
			LOG_ERR("Unable to drop slot: erase cache too small (%d < %d)",
				sizeof(erase_cache), slot->eb_size);
			return SUIT_PLAT_ERR_IO;
		}

		size_t erase_offset = (slot->slot_offset / slot->eb_size) * slot->eb_size;
		size_t erase_max_offset = ((slot->slot_offset + slot->data_offset +
					    data_end_offset + slot->eb_size - 1) /
					   slot->eb_size) *
					  slot->eb_size;

		if (erase_max_offset > (slot->slot_offset + slot->size)) {
			erase_max_offset -= slot->eb_size;
		}

		if (erase_max_offset <= erase_offset) {
			LOG_ERR("Unable to erase area: (addr: 0x%x, size: 0x%x)", slot->slot_offset,
				slot->data_offset + data_end_offset);
			return SUIT_PLAT_ERR_IO;
		}

		if (erase_offset < slot->slot_offset) {
			LOG_DBG("Cache area (0x%x - 0x%x)", erase_offset, slot->slot_offset);
			memset(erase_cache, 0xFF, slot->eb_size);

			int ret = flash_read(slot->fdev, erase_offset, erase_cache,
					     slot->slot_offset - erase_offset);
			if (ret != 0) {
				LOG_ERR("Unable to cache slot before erase: %i", ret);
				return SUIT_PLAT_ERR_IO;
			}
		}

		for (size_t offset = erase_offset; offset < erase_max_offset;
		     offset += slot->eb_size) {
			int ret = flash_erase(slot->fdev, erase_offset, slot->eb_size);
			if (ret != 0) {
				LOG_ERR("Erasing cache failed: %i", ret);
				return SUIT_PLAT_ERR_IO;
			}
		}

		if (erase_offset < slot->slot_offset) {
			LOG_DBG("Restore area (0x%x - 0x%x)", erase_offset, slot->slot_offset);
			int ret = flash_write(slot->fdev, erase_offset, erase_cache,
					      slot->slot_offset - erase_offset);
			if (ret != 0) {
				LOG_ERR("Unable to restore slot after erase: %i", ret);
				return SUIT_PLAT_ERR_IO;
			}
		}

		/* To be used as end marker */
		uint8_t tmp = 0xFF;

		/* Add indefinite map, end marker 0xFF */
		if (flash_write(slot->fdev, slot->slot_offset, &tmp, 1) != 0) {
			LOG_ERR("Writing CBOR map end marker to cache partition failed.");
			return SUIT_PLAT_ERR_IO;
		}
	}

	return SUIT_PLAT_SUCCESS;
}
