/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_platform.h>
#include <digest_sink.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(suit_plat_digest, CONFIG_SUIT_LOG_LEVEL);

int suit_plat_check_digest(enum suit_cose_alg alg_id, struct zcbor_string *digest,
			   struct zcbor_string *payload)
{
	psa_algorithm_t psa_alg;

	/* Find the PSA hash ID */
	switch (alg_id) {
	case suit_cose_sha256:
		psa_alg = PSA_ALG_SHA_256;
		break;
	default:
		LOG_ERR("Unsupported digest algorithm: %d", alg_id);
		return SUIT_ERR_DECODING;
	}

	if ((digest == NULL) || (payload == NULL)) {
		LOG_ERR("Invalid argument");
		return SUIT_FAIL_CONDITION;
	}

	if ((payload->value == NULL) || (payload->len == 0)) {
		LOG_ERR("Ivalid payload");
		return SUIT_ERR_DECODING;
	}

	size_t expected_hash_len = PSA_HASH_LENGTH(psa_alg);
	if (digest->len != expected_hash_len) {
		LOG_ERR("Unexpected hash length: %d instead of %d", digest->len, expected_hash_len);
		return SUIT_ERR_DECODING;
	}

	struct stream_sink digest_sink;

	int err = digest_sink_get(&digest_sink, psa_alg, digest->value);
	if (err) {
		LOG_ERR("Failed to get digest sink: %d", err);
		return err;
	}

	err = digest_sink.write(digest_sink.ctx, (uint8_t *)payload->value, &payload->len);
	if (err) {
		LOG_ERR("Failed to write to stream: %d", err);
		return err;
	}

	int ret = digest_sink_digest_match(digest_sink.ctx);

	err = digest_sink.release(digest_sink.ctx);
	if (err) {
		LOG_ERR("Failed to release stream: %d", err);
		if (SUIT_SUCCESS == ret) {
			return err;
		}
	}

	return ret;
}
