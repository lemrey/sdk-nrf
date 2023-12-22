/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_platform.h>
#include <suit_platform_internal.h>
#include <suit_plat_decode_util.h>
#include <suit_plat_digest_cache.h>
#include <suit_plat_error_convert.h>
#include <suit_memptr_storage.h>
#include <memptr_streamer.h>
#include <digest_sink.h>

#include <psa/crypto.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(suit_plat_check_image_match, CONFIG_SUIT_LOG_LEVEL);

static int suit_plat_check_image_match_mem_mapped(suit_component_t component,
						  enum suit_cose_alg alg_id,
						  struct zcbor_string *digest)
{
	void *impl_data = NULL;
	int err = suit_plat_component_impl_data_get(component, &impl_data);
	if (err != SUIT_SUCCESS) {
		LOG_ERR("Failed to get implementation data: %d", err);
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	uint8_t *data = NULL;
	size_t size = 0;
	err = suit_memptr_storage_ptr_get((memptr_storage_handle_t)impl_data, &data, &size);
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Failed to get memptr ptr: %d", err);
		return SUIT_ERR_CRASH;
	}

	psa_algorithm_t psa_alg;

	if (suit_cose_sha512 == alg_id) {
		psa_alg = PSA_ALG_SHA_512;
	} else if (suit_cose_sha256 == alg_id) {
		psa_alg = PSA_ALG_SHA_256;
	} else {
		LOG_ERR("Unsupported hash algorithm: %d", alg_id);
		return SUIT_ERR_UNSUPPORTED_PARAMETER;
	}

	struct stream_sink digest_sink;

	err = suit_digest_sink_get(&digest_sink, psa_alg, digest->value);
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Failed to get digest sink: %d", err);
		return suit_plat_err_to_processor_err_convert(err);
	}

	err = suit_memptr_streamer_stream(data, size, &digest_sink);
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Failed to stream to digest sink: %d", err);
		err = suit_plat_err_to_processor_err_convert(err);
	} else {
		err = suit_digest_sink_digest_match(digest_sink.ctx);
		if (err != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Failed to check digest: %d", err);
			/* Translate error code to allow entering another branches in try-each
			 * sequence */
			err = SUIT_FAIL_CONDITION;
		}
		else
		{
			err = SUIT_SUCCESS;
		}
	}

	suit_plat_err_t release_err = digest_sink.release(digest_sink.ctx);
	if (release_err != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Failed to release digest sink: %d", release_err);
		if (err != SUIT_SUCCESS) {
			err = suit_plat_err_to_processor_err_convert(release_err);
		}
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
						struct zcbor_string *digest)
{
	uint32_t number = 0;

	if (suit_plat_decode_component_number(component_id, &number) != SUIT_PLAT_SUCCESS) {
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
				struct zcbor_string *digest)
{
	struct zcbor_string *component_id = NULL;
	suit_component_type_t component_type = SUIT_COMPONENT_TYPE_UNSUPPORTED;

	int err = suit_plat_component_id_get(component, &component_id);
	if (err != SUIT_SUCCESS) {
		LOG_ERR("Failed to get component id: %d", err);
		return err;
	}

	if (suit_plat_decode_component_type(component_id, &component_type) != SUIT_PLAT_SUCCESS) {
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
		err = suit_plat_check_image_match_mem_mapped(component, alg_id, digest);
		break;
	}
	case SUIT_COMPONENT_TYPE_CAND_IMG: {
		err = suit_plat_check_image_match_mem_mapped(component, alg_id, digest);
		break;
	}
	case SUIT_COMPONENT_TYPE_SOC_SPEC: {
		err = suit_plat_check_image_match_soc_spec(component_id, alg_id, digest);
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

#if CONFIG_SUIT_DIGEST_CACHE
	if (err == SUIT_SUCCESS)
	{
		int ret;

		switch(component_type)
		{
		case SUIT_COMPONENT_TYPE_MEM:
		case SUIT_COMPONENT_TYPE_SOC_SPEC: {
			ret = suit_plat_digest_cache_add(component_id, alg_id, digest);

			if (ret != SUIT_SUCCESS)
			{
				LOG_WRN("Failed to cache digest for component type %d, err %d", component_type, ret);
			}
		}
		default: {
			break;
		}
		}
	}
#endif /* CONFIG_SUIT_DIGEST_CACHE */

	return err;
}
