/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_platform.h>
#include <suit_platform_internal.h>
#include <suit_plat_decode_util.h>
#include <suit_memptr_storage.h>

#include <psa/crypto.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(suit_plat_check_image_match, CONFIG_SUIT_LOG_LEVEL);

static int suit_plat_check_image_match_mem(struct zcbor_string *component_id,
					   enum suit_cose_alg alg_id, struct zcbor_string *digest,
					   size_t image_size)
{
	intptr_t address = (intptr_t)NULL;
	size_t size = 0;

	if (!suit_plat_decode_address_size(component_id, &address, &size)) {
		LOG_ERR("Failed to decode address and size");
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	LOG_DBG("Address: 0x%08x, size: 0x%08x", (unsigned int)address, image_size);

	if (image_size > size) {
		LOG_ERR("Size mismatch: image: %d, component: %d", image_size, size);
		return SUIT_FAIL_CONDITION;
	}

	struct zcbor_string payload = {
		.value = (const uint8_t *)address,
		.len = image_size,
	};

	int err = suit_plat_check_digest(alg_id, digest, &payload);
	if (err) {
		LOG_ERR("Failed to check digest: %d", err);
		/* Translate error code to allow entering another branches in try-each sequence */
		err = SUIT_FAIL_CONDITION;
	}

	return err;
}

static int suit_plat_check_image_match_cand_img(suit_component_t component,
						enum suit_cose_alg alg_id,
						struct zcbor_string *digest, size_t image_size)
{
	void *impl_data = NULL;
	int err = suit_plat_component_impl_data_get(component, &impl_data);
	if (err) {
		LOG_ERR("Failed to get implementation data: %d", err);
		return err;
	}

	uint8_t *data = NULL;
	size_t size = 0;
	err = get_memptr_ptr((memptr_storage_handle)impl_data, &data, &size);
	if (err) {
		LOG_ERR("Failed to get memptr ptr: %d", err);
		return err;
	}

	if (image_size != size) {
		LOG_ERR("Size mismatch: image: %d, component: %d", image_size, size);
		return SUIT_FAIL_CONDITION;
	}

	struct zcbor_string payload = {
		.value = (const uint8_t *)data,
		.len = size,
	};

	err = suit_plat_check_digest(alg_id, digest, &payload);
	if (err) {
		LOG_ERR("Failed to check digest: %d", err);
		/* Translate error code to allow entering another branches in try-each sequence */
		err = SUIT_FAIL_CONDITION;
	}

	return err;
}

static int suit_plat_check_image_match_soc_spec_sdfw(struct zcbor_string *component_id,
						     enum suit_cose_alg alg_id,
						     struct zcbor_string *digest)
{
#ifdef CONFIG_SOC_NRF54H20
	if (suit_cose_sha512 != alg_id) {
		LOG_ERR("Unsupported digest algorithm: %d", alg_id);
		return SUIT_ERR_UNSUPPORTED_PARAMETER;
	}

	uint8_t *current_sdfw_digest = (uint8_t *)(NRF_SICR->UROT.SM.TBS.FW.DIGEST);

	if (PSA_HASH_LENGTH(PSA_ALG_SHA_512) != digest->len) {
		LOG_ERR("Digest length mismatch: %d instead of %d", digest->len,
			PSA_HASH_LENGTH(PSA_ALG_SHA_512));
		return SUIT_FAIL_CONDITION;
	}

	if (memcmp((void *)current_sdfw_digest, (void *)digest->value,
		   PSA_HASH_LENGTH(PSA_ALG_SHA_512))) {
		LOG_INF("Digest mismatch");
		return SUIT_FAIL_CONDITION;
	}

	return SUIT_SUCCESS;
#else  /* CONFIG_SOC_NRF54H20 */
	return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
#endif /* CONFIG_SOC_NRF54H20 */
}

static int suit_plat_check_image_match_soc_spec(struct zcbor_string *component_id,
						enum suit_cose_alg alg_id,
						struct zcbor_string *digest, size_t image_size)
{
	uint32_t number = 0;

	if (!suit_plat_decode_component_number(component_id, &number)) {
		LOG_ERR("Missing component id number");
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	LOG_DBG("Component id number: %d", number);

	int err = SUIT_ERR_UNSUPPORTED_COMPONENT_ID;

	if (1 == number) {
		/* SDFW */
		err = suit_plat_check_image_match_soc_spec_sdfw(component_id, alg_id, digest);
	} else if (2 == number) {
		/* SDFW recovery */
		err = SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	} else {
		/* Unsupported */
		err = SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	return err;
}

int suit_plat_check_image_match(suit_component_t component, enum suit_cose_alg alg_id,
				struct zcbor_string *digest, size_t image_size)
{
	struct zcbor_string *component_id = NULL;
	suit_component_type_t component_type = SUIT_COMPONENT_TYPE_UNSUPPORTED;

	int err = suit_plat_component_id_get(component, &component_id);
	if (err) {
		LOG_ERR("Failed to get component id: %d", err);
		return err;
	}

	if (!suit_plat_decode_component_type(component_id, &component_type)) {
		LOG_ERR("Failed to decode component type");
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	LOG_DBG("Component type: %d", component_type);

	switch (component_type) {
	case SUIT_COMPONENT_TYPE_UNSUPPORTED: {
		LOG_ERR("Unsupported component type");
		err = SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
		break;
	}
	case SUIT_COMPONENT_TYPE_MEM: {
		err = suit_plat_check_image_match_mem(component_id, alg_id, digest, image_size);
		break;
	}
	case SUIT_COMPONENT_TYPE_CAND_IMG: {
		err = suit_plat_check_image_match_cand_img(component, alg_id, digest, image_size);
		break;
	}
	case SUIT_COMPONENT_TYPE_SOC_SPEC: {
		err = suit_plat_check_image_match_soc_spec(component_id, alg_id, digest,
							   image_size);
		break;
	}
	case SUIT_COMPONENT_TYPE_CAND_MFST:
	case SUIT_COMPONENT_TYPE_INSTLD_MFST:
	case SUIT_COMPONENT_TYPE_CACHE_POOL:
	default: {
		LOG_ERR("Unhandled component type: %d", component_type);
		err = SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
		break;
	}
	}

	return err;
}
