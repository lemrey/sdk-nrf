/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUIT_STORAGE_INTERNAL_H__
#define SUIT_STORAGE_INTERNAL_H__

#include <suit_storage.h>
#include <zcbor_common.h>
#include <platform_mem_util.h>

#define SUIT_STORAGE_ADDRESS	FLASH_ADDRESS(SUIT_STORAGE_OFFSET)
#define SUIT_STORAGE_OFFSET	FIXED_PARTITION_OFFSET(suit_storage)
#define SUIT_STORAGE_SIZE	FIXED_PARTITION_SIZE(suit_storage)
#define SUIT_STORAGE_WRITE_SIZE FLASH_AREA_WRITE_BLOCK_SIZE(suit_storage)
#define SUIT_STORAGE_EB_SIZE	FLASH_AREA_ERASE_BLOCK_SIZE(suit_storage)

#define CEIL_DIV(a, b) ((((a)-1) / (b)) + 1)
#define EB_SIZE(type)  (CEIL_DIV(sizeof(type), SUIT_STORAGE_EB_SIZE) * SUIT_STORAGE_EB_SIZE)

/* Values defined in the SUIT manifest specification. */
#define SUIT_AUTHENTICATION_WRAPPER_TAG 2
#define SUIT_MANIFEST_TAG		3
#define SUIT_MANIFEST_COMPONENT_ID_TAG	5

/* The maximum length of the encoded CBOR key-value pair header with integer key and bstr value. */
#define SUIT_STORAGE_ENCODED_BSTR_HDR_LEN_MAX 6

/* The maximum length of the encoded SUIT storage header with envelope header. */
#define SUIT_STORAGE_ENCODED_ENVELOPE_HDR_LEN_MAX                                                  \
	(1 /* CBOR MAP */ + 4 /* CBOR keys (3 bytes) and version (1 byte) */ +                     \
	 3 /* class ID offset */ + 3 /* SUIT envelope length */ + 3 /* SUIT envelope tag */)

/* The length of CBOR tag and SUIT envelope header. */
#define SUIT_STORAGE_ENCODED_ENVELOPE_TAG_LEN 3

/* Decoded SUIT storage envelope slot. */
typedef struct {
	suit_plat_mreg_t envelope;
	size_t class_id_offset;
} suit_envelope_hdr_t;

/* Decoded severed SUIT envelope with the manifest component ID field.
 *
 * This representation is used to construct the severed SUIT envelope as well as
 * additional fields, stored inside SUIT storage envelope slot map.
 */
struct SUIT_Envelope_severed {
	struct zcbor_string suit_authentication_wrapper;
	struct zcbor_string suit_manifest;
	struct zcbor_string suit_manifest_component_id;
};

/* Update candidate metadata. */
struct update_candidate_info {
	uint32_t update_magic_value;
	size_t update_regions_len;
	suit_plat_mreg_t update_regions[CONFIG_SUIT_STORAGE_N_UPDATE_REGIONS];
};

/* Update candidate metadata, aligned to the erase block size. */
union suit_update_candidate_entry {
	struct update_candidate_info update;
	uint8_t erase_block[EB_SIZE(struct update_candidate_info)];
};

/* SUIT envelope slot.
 *
 * Each slot contains a CBOR-encoded map with severed SUIT envelope.
 */
typedef uint8_t suit_envelope_encoded_t[CONFIG_SUIT_STORAGE_ENVELOPE_SIZE];

/* SUIT envelope slot, aligned to the erase block size. */
union suit_envelope_entry {
	suit_envelope_encoded_t envelope_encoded;
	uint8_t erase_block[EB_SIZE(suit_envelope_encoded_t)];
};

/* SUIT configuration erea, set by application (i.e. thorough IPC service) and
 * checked using suit-condition-check-content.
 *
 * Currently both the service and the directive is not supported.
 * This definition allows to implement up to three 32-bit configuration values.
 * It is defined here, so the system will not be repartitioned once this feature
 * will be introduced in the future.
 */
typedef uint8_t suit_config_t[CONFIG_SUIT_STORAGE_CONFIG_SIZE];

/* SUIT configuration area, aligned to the erase block size. */
union suit_config_entry {
	suit_config_t config;
	uint8_t erase_block[EB_SIZE(suit_config_t)];
};

/* SUIT storage structure, that can be used to parse the NVM contents. */
struct suit_storage {
	/** Update candidate information. */
	union suit_update_candidate_entry update;
	/** SUIT configuration area */
	union suit_config_entry config;
	/** A copy of the configuration area to protect against random resets. */
	union suit_config_entry config_backup;
	/** The main storage for the SUIT envelopes. */
	union suit_envelope_entry envelopes[CONFIG_SUIT_STORAGE_N_ENVELOPES];
};

/**
 * @brief Initialize the SUIT storage submodule, responsible for storing/returning
 *        update candidate info.
 *
 * @return 0 on success, negative value otherwise.
 */
int suit_storage_update_init(void);

/** @brief Calculate the value of encoded key-value CBOR map header.
 *
 * @details All values (key, length) are limited to 0xFFFF (16-bit).
 *
 * @param[in]     key       CBOR map key value.
 * @param[in]     bstr_len  CBOR bstr length to be encoded inside the header.
 *
 * @return Expected header length on success, negative value otherwise.
 */
int suit_storage_bstr_kv_header_len(uint32_t key, size_t bstr_len);

/** @brief Encode key-value CBOR map header.
 *
 * @details All values (key, length) are limited to 0xFFFF (16-bit).
 *
 * @param[in]     key       CBOR map key value.
 * @param[in]     bstr_len  CBOR bstr length to be encoded inside the header.
 * @param[out]    buf       Buffer to fill.
 * @param[inout]  len       As input - length of the buffer,
 *                          as output - number of bytes consumed.
 *
 * @return 0 on success, negative value otherwise.
 */
int suit_storage_encode_bstr_kv_header(uint32_t key, size_t bstr_len, uint8_t *buf, size_t *len);

/** @brief Decode SUIT storage entry.
 *
 * @param[in]   buf       Buffer to prase.
 * @param[in]   len       Length of the buffer.
 * @param[out]  envelope  Decoded entry.
 *
 * @return 0 on success, negative value otherwise.
 */
int suit_storage_decode_envelope_header(const uint8_t *buf, size_t len,
					suit_envelope_hdr_t *envelope);

/** @brief Encode SUIT storage entry with SUIT envelope header.
 *
 * @param[in]     envelope  Envelope header to encode.
 * @param[out]    buf       Buffer to fill.
 * @param[inout]  len       As input - length of the buffer,
 *                          as output - number of bytes consumed.
 *
 * @return 0 on success, negative value otherwise.
 */
int suit_storage_encode_envelope_header(suit_envelope_hdr_t *envelope, uint8_t *buf, size_t *len);

/** @brief Decode SUIT envelope.
 *
 * @param[in]   buf           Buffer to prase.
 * @param[in]   len           Length of the buffer.
 * @param[out]  envelope      Decoded envelope.
 * @param[out]  envelope_len  Decoded envelope length.
 *
 * @return 0 on success, negative value otherwise.
 */
int suit_storage_decode_suit_envelope_severed(const uint8_t *buf, size_t len,
					      struct SUIT_Envelope_severed *envelope,
					      size_t *envelope_len);

#endif /* SUIT_STORAGE_INTERNAL_H__ */
