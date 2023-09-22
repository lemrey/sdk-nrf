/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <psa/crypto.h>
#include <suit_platform.h>
#include <suit_platform_internal.h>

#if defined(CONFIG_SOC_NRF52840)
#define DEFAULT_PSA_KEY_ID 0x7fffffe0
#elif defined(CONFIG_SOC_NRF54H20)
#define DEFAULT_PSA_KEY_ID 0x7fffffe0
#else
#define DEFAULT_PSA_KEY_ID 0x7fffffe0
#endif

int suit_plat_check_digest(enum suit_cose_alg alg_id, struct zcbor_string *digest,
			   struct zcbor_string *payload)
{
	size_t expected_hash_len = 0;
	psa_hash_operation_t operation = PSA_HASH_OPERATION_INIT;
	psa_algorithm_t psa_alg;

	/* Find the PSA hash ID */
	switch (alg_id) {
	case suit_cose_sha256:
		psa_alg = PSA_ALG_SHA_256;
		break;
	default:
		return SUIT_ERR_DECODING;
	}

	if ((payload->value == NULL) || (payload->len == 0)) {
		return SUIT_ERR_DECODING;
	}

	/* Verify digest length */
	expected_hash_len = PSA_HASH_LENGTH(psa_alg);
	if (digest->len != expected_hash_len) {
		return SUIT_ERR_DECODING;
	}

	/* Calculate digest over input */
	if (psa_hash_setup(&operation, psa_alg) != PSA_SUCCESS) {
		return SUIT_ERR_DECODING;
	}

	if (psa_hash_update(&operation, payload->value, payload->len) != PSA_SUCCESS) {
		/* Clean up hash operation context */
		psa_hash_abort(&operation);
		return SUIT_ERR_DECODING;
	}

	/* Verify digest */
	if (psa_hash_verify(&operation, digest->value, digest->len) == PSA_SUCCESS) {
		return SUIT_SUCCESS;
	}

	/* Clean up hash operation context */
	psa_hash_abort(&operation);

	return SUIT_ERR_AUTHENTICATION;
}

int suit_plat_authenticate_manifest(struct zcbor_string *manifest_component_id,
				    enum suit_cose_alg alg_id, struct zcbor_string *key_id,
				    struct zcbor_string *signature, struct zcbor_string *data)
{
	psa_algorithm_t psa_alg;
	psa_key_id_t public_key_id = DEFAULT_PSA_KEY_ID;

	switch (alg_id) {
	case suit_cose_es256:
		psa_alg = PSA_ALG_ECDSA(PSA_ALG_SHA_256);
		break;
	default:
		return SUIT_ERR_DECODING;
	}

	if ((data->value == NULL) || (data->len == 0) || (signature->value == NULL) ||
	    (signature->len == 0)) {
		return SUIT_ERR_DECODING;
	}

	if (psa_verify_message(public_key_id, psa_alg, data->value, data->len, signature->value,
			       signature->len) == PSA_SUCCESS) {
		return SUIT_SUCCESS;
	}

	return SUIT_ERR_AUTHENTICATION;
}
