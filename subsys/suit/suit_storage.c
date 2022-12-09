/*
 * Copyright (c) 2022 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-4-Clause
 */

#include <drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zcbor_decode.h>
#include "suit_storage.h"


#define FLASH_AREA_ERASE_BLOCK_SIZE(a) \
	DT_PROP(DT_GPARENT(DT_NODE_BY_FIXED_PARTITION_LABEL(suit_storage)), erase_block_size)
#define FLASH_AREA_WRITE_BLOCK_SIZE(a) \
	DT_PROP(DT_GPARENT(DT_NODE_BY_FIXED_PARTITION_LABEL(suit_storage)), write_block_size)
#define CEIL_DIV(a, b)  ((((a) - 1) / (b)) + 1)
#define EB_SIZE(type)  (CEIL_DIV(sizeof(type), SUIT_STORAGE_EB_SIZE) * SUIT_STORAGE_EB_SIZE)

#define SUIT_STORAGE_ADDRESS         FLASH_AREA_OFFSET(suit_storage)
#ifdef CONFIG_SOC_NRF54H20
#define SUIT_STORAGE_OFFSET          (SUIT_STORAGE_ADDRESS - DT_REG_ADDR(DT_NODELABEL(mram10)))
#else /* CONFIG_SOC_NRF54H20 */
#define SUIT_STORAGE_OFFSET          SUIT_STORAGE_ADDRESS
#endif /* CONFIG_SOC_NRF54H20 */
#define SUIT_STORAGE_SIZE            FLASH_AREA_SIZE(suit_storage)
#define SUIT_STORAGE_EB_SIZE         FLASH_AREA_ERASE_BLOCK_SIZE(suit_storage)
#define SUIT_STORAGE_WRITE_SIZE      FLASH_AREA_WRITE_BLOCK_SIZE(suit_storage)

#define UPDATE_MAGIC_VALUE_AVAILABLE 0x5555AAAA
#define UPDATE_MAGIC_VALUE_CLEARED   0xAAAA5555
#define UPDATE_MAGIC_VALUE_EMPTY     0xFFFFFFFF



struct update_candidate_info {
	int update_magic_value;
	void* dfu_partition_addr;
	size_t dfu_partition_size;
};

typedef uint8_t suit_envelope_t[CONFIG_SUIT_PARTITION_ENVELOPE_SIZE];

union suit_envelope_entry {
	suit_envelope_t envelope;
	uint8_t envelope_eb[EB_SIZE(suit_envelope_t)];
};

struct suit_storage {
	union {
		struct update_candidate_info update;
		uint8_t update_eb[EB_SIZE(struct update_candidate_info)];
	};
	union suit_envelope_entry envelopes[CONFIG_SUIT_PARTITION_N_ENVELOPES];
};

struct SUIT_Envelope {
	struct zcbor_string suit_authentication_wrapper;
	struct zcbor_string suit_manifest;
};

static const struct device *fdev;

static int decode_SUIT_Envelope_Tagged(
		const uint8_t *payload, size_t payload_len,
		struct SUIT_Envelope *result,
		size_t *payload_len_out)
{
	bool ret = false;
	zcbor_state_t states[3];

	zcbor_new_state(states, sizeof(states) / sizeof(zcbor_state_t), payload, payload_len, 1);

	if (zcbor_tag_expect(states, 107) && zcbor_map_start_decode(states) == true) {
		/* Parse authentication wrapper and manifest. */
		ret = (zcbor_uint32_expect(states, 2)
			&& zcbor_bstr_decode(states, &(result->suit_authentication_wrapper))
			&& zcbor_uint32_expect(states, 3)
			&& zcbor_bstr_decode(states, &(result->suit_manifest)));

		/* Ignore remaining items, consume backups and finish decoding.*/
		if (zcbor_list_map_end_force_decode(states) == false) {
			ret = false;
		};
	}

	if (ret && (payload_len_out != NULL)) {
		*payload_len_out = MIN(payload_len,
				(size_t)states[0].payload - (size_t)payload);
	}

	if (!ret) {
		int err = zcbor_pop_error(states);

		return (err == ZCBOR_SUCCESS) ? ZCBOR_ERR_UNKNOWN : err;
	}
	return ZCBOR_SUCCESS;
}

/** Calculate the length of the CBOR byte string header
 */
static size_t header_len(size_t num_elems)
{
	if (num_elems <= 23) {
		return 1;
	} else if (num_elems <= 255) {
		return 2;
	} else if (num_elems <= 0xFFFF) {
		return 3;
	} else if (num_elems <= 0xFFFFFFFF) {
		return 5;
	} else {
		return 9;
	}
}

/** @brief Save the next part of the envelope.
 *
 * @details Since the SUIT envelope must be modified, to remove parts like integrated payloads,
 *          the save functionality cannot be done in a single transaction.
 *          This function allows to pass unaligned buffers and trigger only aligned transactions
 *          on the flash driver.
 *
 * @param[in]  index  Index in the envelope list to be used.
 * @param[in]  addr   Address of a buffer (part of SUIT envelope) to append.
 * @param[in]  size   Size of a buffer (part of SUIT envelope) to append.
 * @param[in]  reset  Erase enevlope storage before writing.
 * @param[in]  flush  Write all remaining, unaligned data into the envelope storage.
 *
 * @return 0 on success, negative value otherwise.
 */
static int save_envelope_partial(size_t index, const void *addr, size_t size, bool reset, bool flush)
{
	int err = 0;
	static uint8_t write_buf[SUIT_STORAGE_WRITE_SIZE];
	static uint8_t buf_fill_level = 0;
	static size_t offset = 0;
	static int current_index = -1;

	if (fdev == NULL) {
		return -ENXIO;
	}

	if ((index >= CONFIG_SUIT_PARTITION_N_ENVELOPES) ||
	    (size >= (CONFIG_SUIT_PARTITION_ENVELOPE_SIZE - offset))) {
		return -EINVAL;
	}

	if (reset) {
		buf_fill_level = 0;
		offset = 0;
		current_index = index;
		err = flash_erase(fdev,
			SUIT_STORAGE_OFFSET + offsetof(struct suit_storage, envelopes[index]),
			sizeof(((struct suit_storage *)0)->envelopes[0].envelope_eb));
	}

	/* Require to set reset flag before changing envelope index. */
	if ((current_index == -1) || (index != current_index)) {
		return -EINVAL;
	}

	/* Fill the write buffer to flush non-aligned bytes from the previous call. */
	while ((err == 0) && (size > 0) && (buf_fill_level % sizeof(write_buf) != 0)) {
		write_buf[buf_fill_level] = *((uint8_t *)addr);
		buf_fill_level++;
		addr = ((uint8_t *)addr + 1);
		size--;
	}

	/* If write buffer is full - write it into the memory. */
	if ((err == 0) && (buf_fill_level == sizeof(write_buf))) {
		err = flash_write(fdev,
			SUIT_STORAGE_OFFSET
			+ offsetof(struct suit_storage, envelopes[index].envelope)
			+ offset,
			write_buf,
			sizeof(write_buf));

		buf_fill_level = 0;
		offset += sizeof(write_buf);
	}

	/* Write aligned data directly from input buffer. */
	if ((err == 0) && (size >= sizeof(write_buf))) {
		size_t write_size = ((size / sizeof(write_buf)) * sizeof(write_buf));

		err = flash_write(fdev,
			SUIT_STORAGE_OFFSET
			+ offsetof(struct suit_storage, envelopes[index].envelope)
			+ offset,
			addr,
			write_size);

		size -= write_size;
		offset += write_size;
		addr = ((uint8_t *)addr + write_size);
	}

	/* Store remaining byte into the write buffer. */
	while ((err == 0) && (size > 0)) {
		write_buf[buf_fill_level] = *((uint8_t *)addr);
		buf_fill_level++;
		addr = ((uint8_t *)addr + 1);
		size--;
	}

	/* Flush buffer if requested. */
	if ((err == 0) && (flush) && (buf_fill_level > 0)) {
		/* Do not loeak information about the previous requests. */
		while (buf_fill_level < sizeof(write_buf)) {
			write_buf[buf_fill_level] = 0xFF;
			buf_fill_level++;
		}

		err = flash_write(fdev,
			SUIT_STORAGE_OFFSET
			+ offsetof(struct suit_storage, envelopes[index].envelope)
			+ offset,
			write_buf,
			sizeof(write_buf));

		buf_fill_level = 0;
		offset = 0;
	}

	return err;
}

static int save_update_info(struct update_candidate_info *info)
{
	if (fdev == NULL) {
		return -ENXIO;
	}

	int err = flash_erase(fdev,
		SUIT_STORAGE_OFFSET + offsetof(struct suit_storage, update_eb),
		sizeof(((struct suit_storage *)0)->update_eb));
	if (err != 0) {
		return err;
	}

	return flash_write(fdev,
		SUIT_STORAGE_OFFSET + offsetof(struct suit_storage, update),
		info, sizeof(*info));
}


int suit_storage_init(void)
{
	fdev = DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));

	if (!device_is_ready(fdev)) {
		fdev = NULL;
		return -ENXIO;
	}

	if (sizeof(struct suit_storage) > SUIT_STORAGE_SIZE) {
		return -ENOSR;
	}

	if (CEIL_DIV(SUIT_STORAGE_SIZE,
		SUIT_STORAGE_EB_SIZE) * SUIT_STORAGE_EB_SIZE != SUIT_STORAGE_SIZE) {
		return -EBADF;
	}

	return 0;
}

bool suit_storage_update_available(void)
{
	struct suit_storage *storage = (struct suit_storage *)SUIT_STORAGE_ADDRESS;

	if (fdev == NULL) {
		return false;
	}

	if ((storage->update.update_magic_value != UPDATE_MAGIC_VALUE_AVAILABLE) ||
	    (storage->update.dfu_partition_addr == NULL) ||
	    (storage->update.dfu_partition_size == 0)){
		return false;
	}

	return true;
}

int suit_storage_update_get(void **addr, size_t *size)
{
	struct suit_storage *storage = (struct suit_storage *)SUIT_STORAGE_ADDRESS;

	if (fdev == NULL) {
		return -ENXIO;
	}

	if ((addr == NULL) || (size == NULL)) {
		return -EINVAL;
	}

	if (!suit_storage_update_available()) {
		return -ENOENT;
	}

	*addr = storage->update.dfu_partition_addr;
	*size = storage->update.dfu_partition_size;

	return 0;
}

int suit_storage_update_clear(void)
{
	struct update_candidate_info info = {
		.update_magic_value = UPDATE_MAGIC_VALUE_CLEARED,
		.dfu_partition_addr = NULL,
		.dfu_partition_size = 0,
	};

	return save_update_info(&info);
}

int suit_storage_installed_envelope_get(size_t index, void **addr, size_t *size)
{
	struct SUIT_Envelope envelope;
	struct suit_storage *storage = (struct suit_storage *)SUIT_STORAGE_ADDRESS;

	if (fdev == NULL) {
		return -ENXIO;
	}

	if ((addr == NULL) || (size == NULL) || (index >= CONFIG_SUIT_PARTITION_N_ENVELOPES)) {
		return -EINVAL;
	}

	*addr = (void *)(&storage->envelopes[index].envelope);

	/* Validate stored data by parsing buffer as SUIT envelope.
	 * Set the size variable value to the decoded envelope size,
	 * that does not contain severable elements.
	 */
	int err = decode_SUIT_Envelope_Tagged(
		(const uint8_t *)(*addr), CONFIG_SUIT_PARTITION_ENVELOPE_SIZE,
		&envelope, size);
	if (err != 0) {
		return err;
	}

	return 0;
}


int suit_storage_update_set(void *addr, size_t size)
{
	static struct update_candidate_info info;

	if ((addr == NULL) || (size == 0)) {
		return -EINVAL;
	}

	info.update_magic_value = UPDATE_MAGIC_VALUE_AVAILABLE;
	info.dfu_partition_addr = addr;
	info.dfu_partition_size = size;

	return save_update_info(&info);
}

int suit_storage_install_envelope(size_t index, void *addr, size_t size)
{
	struct SUIT_Envelope envelope;
	const uint8_t envelope_hdr[] = {
		0xD8, 0x6B, // SUIT envelope tag (107)
		0xA2, // map with 2 elements
		0x02, // suit-authentication wrapper (key)
	};
	const uint8_t manifest_key = 0x03; // manifest (key)

	if (fdev == NULL) {
		return -ENXIO;
	}

	if ((addr == NULL) || (size == 0) || (index >= CONFIG_SUIT_PARTITION_N_ENVELOPES)) {
	    return -EINVAL;
	}

	/* Validate input data by parsing buffer as SUIT envelope.
	 * Additionally change the size variable value from envelope size to
	 * the decoded envelope size, that does not contain severable elements.
	 */
	int err = decode_SUIT_Envelope_Tagged((const uint8_t *)addr, size, &envelope, &size);
	if (err != 0) {
		return err;
	}

	if (size > CONFIG_SUIT_PARTITION_ENVELOPE_SIZE) {
		return -EINVAL;
	}

	err = save_envelope_partial(index, envelope_hdr, sizeof(envelope_hdr), true, false);
	if (err != 0) {
		return err;
	}

	size_t offset = header_len(envelope.suit_authentication_wrapper.len);
	err = save_envelope_partial(index,
		envelope.suit_authentication_wrapper.value - offset,
		envelope.suit_authentication_wrapper.len + offset,
		false, false);
	if (err != 0) {
		return err;
	}

	err = save_envelope_partial(index, &manifest_key, sizeof(manifest_key), false, false);
	if (err != 0) {
		return err;
	}

	offset = header_len(envelope.suit_manifest.len);
	err = save_envelope_partial(index,
		envelope.suit_manifest.value - offset,
		envelope.suit_manifest.len + offset,
		false, true);
	if (err != 0) {
		return err;
	}

	return err;
}
