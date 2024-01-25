/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/sys/util.h>
#include <string.h>
#include <stdio.h>

#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>
#include <mgmt/mcumgr/util/zcbor_bulk.h>

#ifdef CONFIG_SSF_SUIT_SERVICE_ENABLED
#include <sdfw_services/suit_service.h>
#endif /* CONFIG_SSF_SUIT_SERVICE_ENABLED */

#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/mgmt/handlers.h>
#include "suitfu_mgmt_priv.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(suitfu_mgmt, CONFIG_MGMT_SUITFU_LOG_LEVEL);

int suitfu_mgmt_suit_manifests_list(struct smp_streamer *ctx)
{
	zcbor_state_t *zse = ctx->writer->zs;
	bool ok;
	struct zcbor_string zcs;

	suit_plat_err_t rc;
	suit_ssf_manifest_class_info_t
		class_info[CONFIG_MGMT_SUITFU_GRP_SUIT_MFSTS_STATE_MFSTS_COUNT] = {0};
	size_t class_info_count = ARRAY_SIZE(class_info);

	rc = suit_get_supported_manifest_class_info(class_info, &class_info_count);
	if (rc != SUIT_PLAT_SUCCESS) {
		return MGMT_ERR_EBADSTATE;
	}

	ok = zcbor_tstr_put_term(zse, "manifests") &&
	     zcbor_list_start_encode(zse, class_info_count);
	if (!ok) {
		return MGMT_ERR_EMSGSIZE;
	}

	for (int mfst_idx = 0; mfst_idx < class_info_count; mfst_idx++) {
		suit_ssf_manifest_class_info_t *ci = &class_info[mfst_idx];

		ok = zcbor_map_start_encode(zse, 2);
		if (!ok) {
			return MGMT_ERR_EMSGSIZE;
		}

		zcs.value = ci->class_id.raw;
		zcs.len = sizeof(ci->class_id.raw);

		ok = zcbor_tstr_put_term(zse, "class_id") && zcbor_bstr_encode(zse, &zcs);
		if (!ok) {
			return MGMT_ERR_EMSGSIZE;
		}

		ok = zcbor_tstr_put_term(zse, "role") && zcbor_uint32_put(zse, ci->role);
		if (!ok) {
			return MGMT_ERR_EMSGSIZE;
		}

		ok = zcbor_map_end_encode(zse, 2);
		if (!ok) {
			return MGMT_ERR_EMSGSIZE;
		}
	}

	ok = zcbor_list_end_encode(zse, class_info_count);
	if (!ok) {
		return MGMT_ERR_EMSGSIZE;
	}

	return MGMT_ERR_EOK;
}

int suitfu_mgmt_suit_manifest_state_read(struct smp_streamer *ctx)
{
	zcbor_state_t *zsd = ctx->reader->zs;
	zcbor_state_t *zse = ctx->writer->zs;
	bool ok;
	size_t decoded = 0;
	struct zcbor_string zcs = {0};
	int rc = 0;

	suit_manifest_class_id_t class_id = {{0}};
	suit_uuid_t vendor_id = {{0}};
	unsigned int seq_num = 0;
	suit_semver_raw_t semver_raw = {0};
	suit_digest_status_t digest_status = SUIT_DIGEST_UNKNOWN;
	int digest_alg_id = 0;
	uint8_t digest_buf[IMG_MGMT_HASH_LEN] = {0};
	suit_plat_mreg_t digest;

	struct zcbor_map_decode_key_val manifest_state_read_decode[] = {
		ZCBOR_MAP_DECODE_KEY_VAL(class_id, zcbor_bstr_decode, &zcs)};

	if (zcbor_map_decode_bulk(zsd, manifest_state_read_decode,
				  ARRAY_SIZE(manifest_state_read_decode), &decoded) != 0) {
		LOG_ERR("Decoding manifest state read request failed");
		return MGMT_ERR_EINVAL;
	}

	if (zcs.len == sizeof(class_id.raw)) {
		memcpy(&class_id.raw, zcs.value, zcs.len);
	}

	ok = zcbor_tstr_put_term(zse, "class_id") && zcbor_bstr_encode(zse, &zcs);
	if (!ok) {
		return MGMT_ERR_EMSGSIZE;
	}

	rc = suit_get_vendor_id_for_manifest_class_id(&class_id, &vendor_id);
	if (rc != SUIT_PLAT_SUCCESS) {
		return MGMT_ERR_EBADSTATE;
	}

	zcs.value = vendor_id.raw;
	zcs.len = sizeof(vendor_id.raw);

	ok = zcbor_tstr_put_term(zse, "vendor_id") && zcbor_bstr_encode(zse, &zcs);
	if (!ok) {
		return MGMT_ERR_EMSGSIZE;
	}

	digest.mem = digest_buf;
	digest.size = sizeof(digest_buf);

	rc = suit_get_installed_manifest_info(&class_id, &seq_num, &semver_raw, &digest_status,
					      &digest_alg_id, &digest);

	if (rc == SUIT_PLAT_SUCCESS) {
		zcs.value = digest.mem;
		zcs.len = digest.size;

		ok = zcbor_tstr_put_term(zse, "digest") && zcbor_bstr_encode(zse, &zcs);
		if (!ok) {
			return MGMT_ERR_EMSGSIZE;
		}

		ok = zcbor_tstr_put_term(zse, "digest_algorithm") &&
		     zcbor_int32_put(zse, digest_alg_id);
		if (!ok) {
			return MGMT_ERR_EMSGSIZE;
		}

		ok = zcbor_tstr_put_term(zse, "signature_check") &&
		     zcbor_uint32_put(zse, digest_status);
		if (!ok) {
			return MGMT_ERR_EMSGSIZE;
		}

		ok = zcbor_tstr_put_term(zse, "sequence_number") && zcbor_uint32_put(zse, seq_num);
		if (!ok) {
			return MGMT_ERR_EMSGSIZE;
		}

		if (semver_raw.len > 0 && semver_raw.len <= 5) {

			ok = zcbor_tstr_put_term(zse, "semantic_version") &&
			     zcbor_list_start_encode(zse, semver_raw.len);
			if (!ok) {
				return MGMT_ERR_EMSGSIZE;
			}

			for (size_t semver_idx = 0; semver_idx < semver_raw.len; semver_idx++) {

				ok = zcbor_int32_put(zse, semver_raw.raw[semver_idx]);
				if (!ok) {
					return MGMT_ERR_EMSGSIZE;
				}
			}

			ok = zcbor_list_end_encode(zse, semver_raw.len);
			if (!ok) {
				return MGMT_ERR_EMSGSIZE;
			}
		}
	}

	return MGMT_ERR_EOK;
}
