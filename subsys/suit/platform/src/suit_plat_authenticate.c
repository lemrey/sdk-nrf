/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <psa/crypto.h>
#include <suit_platform.h>
#include <suit_mci.h>
#include <suit_plat_decode_util.h>
#include <suit_plat_component_compatibility.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(suit_plat_authenticate, CONFIG_SUIT_LOG_LEVEL);

int suit_plat_authenticate_manifest(struct zcbor_string *manifest_component_id,
				    enum suit_cose_alg alg_id, struct zcbor_string *key_id,
				    struct zcbor_string *signature, struct zcbor_string *data)
{
	psa_algorithm_t psa_alg;
	psa_key_id_t public_key_id = 0;
	suit_manifest_class_id_t *class_id;

	switch (alg_id) {
	case suit_cose_es256:
		psa_alg = PSA_ALG_ECDSA(PSA_ALG_SHA_256);
		break;
	case suit_cose_EdDSA:
		psa_alg = PSA_ALG_ED25519PH; /* ed25519/curve25519 with internal SHA-512 */
		break;
	default:
		return SUIT_ERR_DECODING;
	}

	if ((manifest_component_id == NULL) || (key_id == NULL) || (signature == NULL) ||
	    (data == NULL) || (key_id->value == NULL) || (key_id->len == 0) ||
	    (signature->value == NULL) || (signature->len == 0) || (data->value == NULL) ||
	    (data->len == 0)) {
		return SUIT_ERR_DECODING;
	}

	/* Check if component ID is a manifest class */
	if (suit_plat_decode_manifest_class_id(manifest_component_id, &class_id)
	    != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Component ID is not a manifest class");
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	/* Validate manifest class ID against supported manifests */
	mci_err_t ret = suit_mci_manifest_class_id_validate(class_id);
	if (ret != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Manifest class ID validation failed: MCI err %i", ret);
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	/* Try to get uint32_t key_id from zcbor_string */
	if (suit_plat_decode_key_id(key_id, &public_key_id) != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Decoding key ID failed");
		return SUIT_ERR_UNSUPPORTED_PARAMETER;
	}

	/* Validate KEY ID */
	ret = suit_mci_signing_key_id_validate(class_id, public_key_id);
	if (ret != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Signing key validation failed: MCI err %i", ret);
		return SUIT_ERR_AUTHENTICATION;
	}

	/* Verify data */
	if (psa_verify_message(public_key_id, psa_alg, data->value, data->len, signature->value,
			       signature->len) == PSA_SUCCESS) {
		return SUIT_SUCCESS;
	}

	return SUIT_ERR_AUTHENTICATION;
}

int suit_plat_authorize_unsigned_manifest(struct zcbor_string *manifest_component_id)
{
	suit_manifest_class_id_t *class_id;

	if ((manifest_component_id == NULL) ||
		(manifest_component_id->value == NULL) ||
		(manifest_component_id->len == 0)) {
		return SUIT_ERR_DECODING;
	}

	/* Check if component ID is a manifest class */
	if (suit_plat_decode_manifest_class_id(manifest_component_id, &class_id)
	    != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Component ID is not a manifest class");
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	/* Validate manifest class ID against supported manifests */
	mci_err_t ret = suit_mci_manifest_class_id_validate(class_id);
	if (ret != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Manifest class ID validation failed: MCI err %i", ret);
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	/* Check if unsigned manifest is allowed - pass key_id == 0*/
	ret = suit_mci_signing_key_id_validate(class_id, 0);

	if (ret == SUIT_PLAT_SUCCESS) {
		return SUIT_SUCCESS;
	}

	return SUIT_ERR_AUTHENTICATION;
}

int suit_plat_authorize_component_id(struct zcbor_string *manifest_component_id,
				     struct zcbor_string *component_id)
{
	suit_manifest_class_id_t *class_id;

	if ((manifest_component_id == NULL) || (component_id == NULL) ||
	    (manifest_component_id->value == NULL) || (manifest_component_id->len == 0) ||
	    (component_id->value == NULL) || (component_id->len == 0)) {
		return SUIT_ERR_DECODING;
	}

	/* Check if component ID is a manifest class */
	if (suit_plat_decode_manifest_class_id(manifest_component_id, &class_id)
	    != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Component ID is not a manifest class");
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	return suit_plat_component_compatibility_check(class_id, component_id);
}
