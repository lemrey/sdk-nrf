/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_storage_internal.h>
#include <suit_plat_mem_util.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/logging/log.h>
#include <suit_plat_decode_util.h>

LOG_MODULE_REGISTER(suit_storage, CONFIG_SUIT_LOG_LEVEL);

static const struct device *fdev;
static const suit_manifest_class_id_t *manifest_class_ids[CONFIG_SUIT_STORAGE_N_ENVELOPES];
static size_t manifest_class_ids_len;

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
 * @param[in]  reset  Erase envelope storage before writing.
 * @param[in]  flush  Write all remaining, unaligned data into the envelope storage.
 *
 * @return SUIT_PLAT_SUCCESS in case of success, otherwise error code
 */
static suit_plat_err_t save_envelope_partial(size_t index, const void *addr, size_t size,
					     bool reset, bool flush)
{
	int err = 0;
	static uint8_t write_buf[SUIT_STORAGE_WRITE_SIZE];
	static uint8_t buf_fill_level = 0;
	static size_t offset = 0;
	static int current_index = -1;
	size_t envelope_offset = SUIT_STORAGE_OFFSET +
				 offsetof(struct suit_storage, envelopes[index].envelope_encoded);

	if (fdev == NULL) {
		return SUIT_PLAT_ERR_HW_NOT_READY;
	}

	if ((index >= CONFIG_SUIT_STORAGE_N_ENVELOPES) ||
	    (size >= (CONFIG_SUIT_STORAGE_ENVELOPE_SIZE - offset))) {
		return SUIT_PLAT_ERR_INVAL;
	}

	if (reset) {
		buf_fill_level = 0;
		offset = 0;
		current_index = index;
		err = flash_erase(
			fdev, SUIT_STORAGE_OFFSET + offsetof(struct suit_storage, envelopes[index]),
			sizeof(((struct suit_storage *)0)->envelopes[0].erase_block));
	}

	/* Require to set reset flag before changing envelope index. */
	if ((current_index == -1) || (index != current_index)) {
		return SUIT_PLAT_ERR_INVAL;
	}

	LOG_DBG("Writing %d bytes (cache fill: %d)", size, buf_fill_level);

	/* Fill the write buffer to flush non-aligned bytes from the previous call. */
	if ((err == 0) && buf_fill_level) {
		size_t len = sizeof(write_buf) - buf_fill_level;
		len = MIN(len, size);
		memcpy(&write_buf[buf_fill_level], addr, len);

		buf_fill_level += len;
		addr = ((uint8_t *)addr) + len;
		size -= len;

		/* If write buffer is full - write it into the memory. */
		if (buf_fill_level == sizeof(write_buf)) {
			LOG_DBG("Write continuous %d cache bytes (address: %p)", sizeof(write_buf),
				(void *)(envelope_offset + offset));
			err = flash_write(fdev, envelope_offset + offset, write_buf,
					  sizeof(write_buf));

			buf_fill_level = 0;
			offset += sizeof(write_buf);
		}
	}

	/* Write aligned data directly from input buffer. */
	if ((err == 0) && (size >= sizeof(write_buf))) {
		size_t write_size = ((size / sizeof(write_buf)) * sizeof(write_buf));

		LOG_DBG("Write continuous %d image bytes (address: %p)", write_size,
			(void *)(envelope_offset + offset));
		err = flash_write(fdev, envelope_offset + offset, addr, write_size);

		size -= write_size;
		offset += write_size;
		addr = ((uint8_t *)addr + write_size);
	}

	/* Store remaining bytes into the write buffer. */
	if ((err == 0) && (size > 0)) {
		LOG_DBG("Cache %d bytes (address: %p)", size, (void *)(envelope_offset + offset));

		memcpy(&write_buf[buf_fill_level], addr, size);
		buf_fill_level += size;
	}

	/* Flush buffer if requested. */
	if ((err == 0) && (flush) && (buf_fill_level > 0) && (sizeof(write_buf) > 1)) {
		/* Do not leak information about the previous requests. */
		memset(&write_buf[buf_fill_level], 0xFF, sizeof(write_buf) - buf_fill_level);

		LOG_DBG("Flush %d bytes (address: %p)", sizeof(write_buf),
			(void *)(envelope_offset + offset));
		err = flash_write(fdev, envelope_offset + offset, write_buf, sizeof(write_buf));

		buf_fill_level = 0;
		offset += sizeof(write_buf);
	}

	if (err != 0) {
		return SUIT_PLAT_ERR_IO;
	}
	return SUIT_PLAT_SUCCESS;
}

/** @brief Append the next CBOR map element with payload encoded as byte string.
 *
 * @param[in]  index  Index in the envelope list to be used.
 * @param[in]  key    The CBOR map key value.
 * @param[in]  bstr   The byte string to append.
 * @param[in]  flush  Write all remaining, unaligned data into the envelope storage.
 *
 * @return SUIT_PLAT_SUCCESS in case of success, otherwise error code
 */
static suit_plat_err_t save_envelope_partial_bstr(size_t index, uint16_t key,
						  struct zcbor_string *bstr, bool flush)
{
	uint8_t kv_hdr_buf[SUIT_STORAGE_ENCODED_BSTR_HDR_LEN_MAX];
	size_t hdr_len = sizeof(kv_hdr_buf);

	suit_plat_err_t err =
		suit_storage_encode_bstr_kv_header(key, bstr->len, kv_hdr_buf, &hdr_len);
	if (err != SUIT_PLAT_SUCCESS) {
		return err;
	}

	err = save_envelope_partial(index, kv_hdr_buf, hdr_len, false, false);
	if (err != SUIT_PLAT_SUCCESS) {
		return err;
	}

	return save_envelope_partial(index, bstr->value, bstr->len, false, flush);
}

/** @brief Check if the given manifest class ID is supported.
 *
 * @param[in]  id  Manifest class ID to check.
 *
 * @return true if manifest class ID is supported, false otherwise.
 */
static bool is_supported_class_id(const suit_manifest_class_id_t *id)
{
	if (id == NULL) {
		return false;
	}

	for (size_t i = 0; i < manifest_class_ids_len; i++) {
		if (suit_metadata_uuid_compare(id, manifest_class_ids[i]) == SUIT_PLAT_SUCCESS) {
			return true;
		}
	}

	return false;
}

/** @brief Find the current slot of the manifest with the given class and the next available index.
 *
 * @note The value of CONFIG_SUIT_STORAGE_N_ENVELOPES is used to notify that the manifest was not
 * found or there is no free slot available.
 *
 * @param[in]   id       Manifest class ID to look for.
 * @param[out]  current  Current envelope index that holds a manifest with a given class.
 * @param[out]  free     The next available slot.
 *
 * @return SUIT_PLAT_SUCCESS in case of success, otherwise error code
 */
static suit_plat_err_t find_manifest_index(const suit_manifest_class_id_t *id, size_t *current,
					   size_t *free)
{
	struct suit_storage *storage = (struct suit_storage *)SUIT_STORAGE_ADDRESS;
	size_t index;

	if ((current == NULL) || (free == NULL)) {
		return SUIT_PLAT_ERR_INVAL;
	}

	*current = CONFIG_SUIT_STORAGE_N_ENVELOPES;
	*free = CONFIG_SUIT_STORAGE_N_ENVELOPES;

	if (!is_supported_class_id(id)) {
		LOG_ERR("Manifest class ID not supported.");
		return SUIT_PLAT_ERR_INVAL;
	}

	for (index = 0; index < CONFIG_SUIT_STORAGE_N_ENVELOPES; index++) {
		suit_envelope_hdr_t envelope;

		suit_plat_err_t ret = suit_storage_decode_envelope_header(
			(const uint8_t *)&storage->envelopes[index].envelope_encoded,
			CONFIG_SUIT_STORAGE_ENVELOPE_SIZE, &envelope);
		if (ret != SUIT_PLAT_SUCCESS) {
			*free = index;
			continue;
		}

		suit_manifest_class_id_t *class_id = (suit_manifest_class_id_t *)&envelope.envelope
							     .mem[envelope.class_id_offset];

		if (!is_supported_class_id(class_id)) {
			*free = index;
			continue;
		}

		if (suit_metadata_uuid_compare(id, class_id) == SUIT_PLAT_SUCCESS) {
			LOG_DBG("Envelope with given class ID found at index %d", index);
			break;
		}
	}

	if (index != CONFIG_SUIT_STORAGE_N_ENVELOPES) {
		*current = index;
	}

	return SUIT_PLAT_SUCCESS;
}

/** @brief Erase unsupported envelopes.
 *
 * @details This API uses the internal list of supported manifest class IDs,
 *          set through the initialization function.
 *
 * @return SUIT_PLAT_SUCCESS in case of success, otherwise error code
 */
static suit_plat_err_t erase_unsupported_envelopes(void)
{
	const size_t eb_size = sizeof(((struct suit_storage *)0)->envelopes[0].erase_block);
	struct suit_storage *storage = (struct suit_storage *)SUIT_STORAGE_ADDRESS;
	size_t index;

	for (index = 0; index < CONFIG_SUIT_STORAGE_N_ENVELOPES; index++) {
		suit_envelope_hdr_t envelope;

		suit_plat_err_t ret = suit_storage_decode_envelope_header(
			(const uint8_t *)&storage->envelopes[index].envelope_encoded,
			CONFIG_SUIT_STORAGE_ENVELOPE_SIZE, &envelope);
		if (ret != SUIT_PLAT_SUCCESS) {
			continue;
		}

		suit_manifest_class_id_t *class_id = (suit_manifest_class_id_t *)&envelope.envelope
							     .mem[envelope.class_id_offset];

		if (!is_supported_class_id(class_id)) {
			int flash_ret =
				flash_erase(fdev,
					    SUIT_STORAGE_OFFSET +
						    offsetof(struct suit_storage, envelopes[index]),
					    eb_size);
			if (flash_ret != 0) {
				LOG_ERR("Unable to erase unsupported envelope at index %d (er: %d)",
					index, ret);
				return SUIT_PLAT_ERR_IO;
			}
		}
	}

	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_storage_init(const suit_manifest_class_id_t **supported_class_id, size_t len)
{
	fdev = DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));

	if ((len == 0) || (supported_class_id == NULL) ||
	    (len > ZCBOR_ARRAY_SIZE(manifest_class_ids))) {
		return SUIT_PLAT_ERR_INVAL;
	}

	if (!device_is_ready(fdev)) {
		fdev = NULL;
		return SUIT_PLAT_ERR_HW_NOT_READY;
	}

	if (sizeof(struct suit_storage) > SUIT_STORAGE_SIZE) {
		return SUIT_PLAT_ERR_NOMEM;
	}

	if (CEIL_DIV(SUIT_STORAGE_SIZE, SUIT_STORAGE_EB_SIZE) * SUIT_STORAGE_EB_SIZE !=
	    SUIT_STORAGE_SIZE) {
		return SUIT_PLAT_ERR_CRASH;
	}

	LOG_DBG("Supporting %d manifest class IDs:", len);
	for (size_t i = 0; i < len; i++) {
		manifest_class_ids[i] = supported_class_id[i];
		LOG_DBG("\t%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
			supported_class_id[i]->raw[0], supported_class_id[i]->raw[1],
			supported_class_id[i]->raw[2], supported_class_id[i]->raw[3],
			supported_class_id[i]->raw[4], supported_class_id[i]->raw[5],
			supported_class_id[i]->raw[6], supported_class_id[i]->raw[7],
			supported_class_id[i]->raw[8], supported_class_id[i]->raw[9],
			supported_class_id[i]->raw[10], supported_class_id[i]->raw[11],
			supported_class_id[i]->raw[12], supported_class_id[i]->raw[13],
			supported_class_id[i]->raw[14], supported_class_id[i]->raw[15]);
	}
	manifest_class_ids_len = len;

	suit_plat_err_t err = erase_unsupported_envelopes();
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Unable to erase unsupported envelopes (err: %d)", err);
		return err;
	}

	err = suit_storage_update_init();
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Unable to initialize update candidate staging area (err: %d)", err);
	}

	return err;
}

suit_plat_err_t suit_storage_installed_envelope_get(const suit_manifest_class_id_t *id,
						    uint8_t **addr, size_t *size)
{
	struct SUIT_Envelope_severed envelope;
	struct suit_storage *storage = (struct suit_storage *)SUIT_STORAGE_ADDRESS;
	suit_plat_err_t err;
	size_t index = 0;
	size_t free_index;

	if (fdev == NULL) {
		return SUIT_PLAT_ERR_HW_NOT_READY;
	}

	if ((id == NULL) || (addr == NULL) || (size == NULL)) {
		return SUIT_PLAT_ERR_INVAL;
	}

	err = find_manifest_index(id, &index, &free_index);
	if ((err != SUIT_PLAT_SUCCESS) || (index >= CONFIG_SUIT_STORAGE_N_ENVELOPES)) {
		return SUIT_PLAT_ERR_NOT_FOUND;
	}

	suit_envelope_hdr_t envelope_hdr;

	LOG_DBG("Decode envelope at index: %d address: 0x%lx", index,
		(intptr_t)(uint8_t *)(&storage->envelopes[index].envelope_encoded));

	err = suit_storage_decode_envelope_header(
		(uint8_t *)(&storage->envelopes[index].envelope_encoded),
		CONFIG_SUIT_STORAGE_ENVELOPE_SIZE, &envelope_hdr);
	if (err != SUIT_PLAT_SUCCESS) {
		return err;
	}

	*addr = (void *)(envelope_hdr.envelope.mem);

	/* Validate stored data by parsing buffer as SUIT envelope.
	 * Set the size variable value to the decoded envelope size,
	 * that does not contain severable elements.
	 */
	err = suit_storage_decode_suit_envelope_severed(
		envelope_hdr.envelope.mem, envelope_hdr.envelope.size, &envelope, size);
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Unable to parse envelope installed at index %d", index);
		return err;
	}

	LOG_INF("Valid envelope with given class ID found at index %d", index);

	return err;
}

suit_plat_err_t suit_storage_install_envelope(const suit_manifest_class_id_t *id, uint8_t *addr,
					      size_t size)
{
	uint8_t envelope_hdr_buf[SUIT_STORAGE_ENCODED_ENVELOPE_HDR_LEN_MAX];
	size_t envelope_hdr_buf_len = sizeof(envelope_hdr_buf);
	suit_envelope_hdr_t envelope_hdr;
	suit_manifest_class_id_t *class_id;
	struct SUIT_Envelope_severed envelope;
	suit_plat_err_t err;
	size_t index = 0;
	size_t free_index;

	if (fdev == NULL) {
		return SUIT_PLAT_ERR_HW_NOT_READY;
	}

	if ((addr == NULL) || (size == 0)) {
		return SUIT_PLAT_ERR_INVAL;
	}

	err = find_manifest_index(id, &index, &free_index);
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Unable to find indexes for the manifest (%d).", err);
		return err;
	}

	if (index >= CONFIG_SUIT_STORAGE_N_ENVELOPES) {
		LOG_DBG("Current index not found, trying to allocate a new entry.");
		index = free_index;
	}

	if (index >= CONFIG_SUIT_STORAGE_N_ENVELOPES) {
		LOG_ERR("Unable to find a free slot");
		return SUIT_PLAT_ERR_NOT_FOUND;
	}

	/* Validate input data by parsing buffer as SUIT envelope.
	 * Additionally change the size variable value from envelope size to
	 * the decoded envelope size, that does not contain severable elements.
	 */
	err = suit_storage_decode_suit_envelope_severed((const uint8_t *)addr, size, &envelope,
							&size);
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Unable to install envelope: decode failed (%d)", err);
		return err;
	}

	if (suit_plat_decode_manifest_class_id(&envelope.suit_manifest_component_id, &class_id) !=
	    SUIT_PLAT_SUCCESS) {
		LOG_ERR("Unable to install envelope: class ID not decoded");
		return SUIT_PLAT_ERR_INVAL;
	}

	if (SUIT_PLAT_SUCCESS != suit_metadata_uuid_compare(id, class_id)) {
		LOG_ERR("Unable to install envelope: class ID does not match");
		return SUIT_PLAT_ERR_INVAL;
	}

	size_t class_id_offset = (size_t)class_id - (size_t)envelope.suit_manifest.value +
				 envelope.suit_authentication_wrapper.len;
	/* Move offset and size by:
	 * - SUIT envelope tag
	 * - Authentication wrapper bstr header
	 * - Manifest bstr header
	 */
	size_t encoding_overhead = SUIT_STORAGE_ENCODED_ENVELOPE_TAG_LEN;
	size_t header_len = 0;

	err = suit_storage_bstr_kv_header_len(SUIT_AUTHENTICATION_WRAPPER_TAG,
					      envelope.suit_authentication_wrapper.len,
					      &header_len);
	if (err != SUIT_PLAT_SUCCESS) {
		return err;
	}
	encoding_overhead += header_len;

	err = suit_storage_bstr_kv_header_len(SUIT_MANIFEST_TAG, envelope.suit_manifest.len,
					      &header_len);

	if (err != SUIT_PLAT_SUCCESS) {
		return err;
	}
	encoding_overhead += header_len;

	envelope_hdr.class_id_offset = encoding_overhead + class_id_offset;
	envelope_hdr.envelope.mem = addr;
	envelope_hdr.envelope.size = encoding_overhead + envelope.suit_manifest.len +
				     envelope.suit_authentication_wrapper.len;

	if (envelope_hdr_buf_len + envelope_hdr.envelope.size > CONFIG_SUIT_STORAGE_ENVELOPE_SIZE) {
		LOG_ERR("Unable to install envelope: envelope too big (%d)",
			envelope_hdr.envelope.size);
		return SUIT_PLAT_ERR_INVAL;
	}

	err = suit_storage_encode_envelope_header(&envelope_hdr, envelope_hdr_buf,
						  &envelope_hdr_buf_len);
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Unable to encode envelope header (%d)", err);
		return err;
	}

	err = save_envelope_partial(index, envelope_hdr_buf, envelope_hdr_buf_len, true, false);
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Unable to save envelope header (%d)", err);
		return err;
	}

	err = save_envelope_partial_bstr(index, SUIT_AUTHENTICATION_WRAPPER_TAG,
					 &envelope.suit_authentication_wrapper, false);
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Unable to save euthentication wrapper (%d)", err);
		return err;
	}

	err = save_envelope_partial_bstr(index, SUIT_MANIFEST_TAG, &envelope.suit_manifest, true);
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Unable to save manifest (%d)", err);
		return err;
	}

	LOG_INF("Envelope saved at index: %d.", index);

	return err;
}
