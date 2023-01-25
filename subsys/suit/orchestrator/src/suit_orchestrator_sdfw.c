/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-4-Clause
 */

#include <zephyr/logging/log.h>
#include <suit_orchestrator.h>

#include <psa/crypto.h>
#include <stdbool.h>
#include <suit.h>
#include <suit_platform.h>
#include <suit_storage.h>

LOG_MODULE_REGISTER(suit, CONFIG_SUIT_LOG_LEVEL);

#define SUIT_ORCHESTRATOR_MANIFEST_INDEX_ROOT 0

#ifdef CONFIG_SDFW_MRAM_ERASE_VALUE
#define EMPTY_STORAGE_VALUE ((((uint32_t)(CONFIG_SDFW_MRAM_ERASE_VALUE)) << 24) |\
			     (((uint32_t)(CONFIG_SDFW_MRAM_ERASE_VALUE)) << 16) |\
			     (((uint32_t)(CONFIG_SDFW_MRAM_ERASE_VALUE)) << 8)  |\
			     (((uint32_t)(CONFIG_SDFW_MRAM_ERASE_VALUE))))
#else
#define EMPTY_STORAGE_VALUE 0xFFFFFFFF
#endif

static struct suit_processor_state state;

static int load_keys(void)
{
	const uint8_t public_key[] = { 0x04, /* POINT_CONVERSION_UNCOMPRESSED */
				       0xed, 0xd0, 0x9e, 0xa5, 0xec, 0xe4, 0xed, 0xbe, 0x6c, 0x08,
				       0xe7, 0x47, 0x09, 0x55, 0x9a, 0x38, 0x29, 0xc5, 0x31, 0x33,
				       0x22, 0x7b, 0xf4, 0xf0, 0x11, 0x6e, 0x8c, 0x05, 0x2d, 0x02,
				       0x0e, 0x0e, 0xc3, 0xe0, 0xd8, 0x37, 0xf4, 0xc2, 0x6f, 0xc1,
				       0x28, 0x80, 0x2f, 0x45, 0x38, 0x1a, 0x23, 0x2b, 0x6d, 0xd5,
				       0xda, 0x28, 0x60, 0x00, 0x5d, 0xab, 0xe2, 0xa0, 0x83, 0xdb,
				       0xef, 0x38, 0x55, 0x13 };
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;
	psa_key_id_t public_key_id;

	/* Initialize PSA Crypto */
	psa_status = psa_crypto_init();
	if (psa_status != PSA_SUCCESS) {
		LOG_ERR("Failed to initialize PSA Crypto: %d", psa_status);
		return psa_status;
	}

	/* Add keys */
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_VERIFY_HASH);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1));

	psa_status = psa_import_key(&key_attributes, public_key, sizeof(public_key), &public_key_id);
	if (psa_status != PSA_SUCCESS) {
		LOG_ERR("Failed to add public key: %d", psa_status);
		return psa_status;
	}
	LOG_DBG("Loaded key ID: 0x%x", public_key_id);

	return 0;
}

static int enter_emergency_recovery(void)
{
	LOG_WRN("TODO: Implement entering emergency recovery");
	return 0;
}

static int validate_update_candidate_address_and_size(void *addr, size_t size)
{
	if (addr == NULL || addr == (void*)EMPTY_STORAGE_VALUE) {
		LOG_DBG("Invalid update candidate address: %p", addr);
		return EFAULT;
	}

	if (size == 0 || size == EMPTY_STORAGE_VALUE) {
		LOG_DBG("Invalid update candidate size: %d", size);
		return EFAULT;
	}

	return 0;
}

/* Common for installed manifest and update candidate */
static int validate_manifest(void *addr, size_t size)
{
	int err = suit_decode_envelope(addr, size, &state);

	if (err) {
		LOG_ERR("Failed to decode envelope: %d", err);
		return err;
	}
	LOG_DBG("Envelope decoded");

	err = suit_validate_envelope(&state);
	if (err) {
		LOG_ERR("Failed to validate envelope: %d", err);
		return err;
	}
	LOG_DBG("Envelope validated");

	err = suit_decode_manifest(&state);
	if (err) {
		LOG_ERR("Failed to decode manifest: %d", err);
		return err;
	}
	LOG_DBG("Manifest decoded");

	err = suit_validate_manifest(&state);
	if (err) {
		LOG_ERR("Failed to validate manifest: %d", err);
		return err;
	}

	LOG_DBG("Manifest validated");

	return 0;
}

static bool update_candidate_applicable(void)
{
	LOG_WRN("TODO: Implement update candidate applicability check");
	return true;
}

static int validate_update_candidate_manifest(void *manifest_address, size_t manifest_size)
{
	int err = validate_manifest(manifest_address, manifest_size);

	if (err) {
		LOG_ERR("Failed to validate update candidate manifest: %d", err);
		return err;
	}

	if (update_candidate_applicable()) {
		LOG_INF("Update candidate applicable");
	} else {
		LOG_INF("Update candidate not applicable");
		return ENOTSUP;
	}

	return 0;
}

static int update_path(void)
{
	void *update_candidate_address = NULL;
	size_t update_candidate_size = 0;

	int err = suit_storage_update_get(&update_candidate_address, &update_candidate_size);
	if (err) {
		LOG_ERR("Failed to get update candidate data: %d", err);
		return err;
	}

	LOG_DBG("Update candidate address: %p", update_candidate_address);
	LOG_DBG("Update candidate size: %d", update_candidate_size);

	err = validate_update_candidate_address_and_size(update_candidate_address, update_candidate_size);
	if (err) {
		LOG_INF("Invalid update candidate: %d", err);

		err = suit_storage_update_clear();
		if (err) {
			LOG_ERR("Failed to clear update candidate");
			return err;
		}

		LOG_DBG("Update candidate cleared");

		/* Do not return error if candidate is invalid - this can happen */
		return 0;
	}

	err = validate_update_candidate_manifest(update_candidate_address, update_candidate_size);
	if (err) {
		LOG_ERR("Failed to validate update candidate manifest: %d", err);
		err = suit_storage_update_clear();
		if (err) {
			LOG_ERR("Failed to clear update candidate");
			return err;
		}

		LOG_DBG("Update candidate cleared");
		/* Do not return error if candidate is invalid - this can happen */
		return 0;
	}
	LOG_DBG("Manifest validated");

	err = suit_process_manifest(&state, SUIT_INSTALL);
	if (err) {
		LOG_ERR("Failed to execute suit-install: %d", err);
		return err;
	}

	LOG_DBG("suit-install successful");

	err = suit_storage_install_envelope(SUIT_ORCHESTRATOR_MANIFEST_INDEX_ROOT, update_candidate_address, update_candidate_size);
	if (err) {
		LOG_ERR("Failed to install envelope: %d", err);
	}

	LOG_DBG("Envelope installed");

	err = suit_storage_update_clear();
	if (err) {
		LOG_ERR("Failed to clear update candidate");
		return err;
	}

	LOG_DBG("Update candidate cleared");

	return 0;
}

static int boot_path(void)
{
	void *installed_envelope_address = NULL;
	size_t installed_envelope_size = 0;

	int err = suit_storage_installed_envelope_get(SUIT_ORCHESTRATOR_MANIFEST_INDEX_ROOT, &installed_envelope_address, &installed_envelope_size);

	if (err) {
		LOG_ERR("Failed to get installed envelope data: %d", err);
		return enter_emergency_recovery();
	}
	if (installed_envelope_address == NULL){
		LOG_ERR("Invalid envelope address");
		return enter_emergency_recovery();
	}
	if (installed_envelope_size == 0){
		LOG_ERR("Invalid envelope size");
		return enter_emergency_recovery();
	}
	LOG_DBG("Found installed envelope");

	err = validate_manifest(installed_envelope_address, installed_envelope_size);
	if (err) {
		LOG_ERR("Failed to validate installed root manifest: %d", err);
		return enter_emergency_recovery();
	}
	LOG_DBG("Validated installed root manifest");

	err = suit_process_manifest(&state, SUIT_VALIDATE);
	if (err) {
		LOG_ERR("Failed to execute suit-validate: %d", err);
		return enter_emergency_recovery();
	}
	LOG_DBG("Processed suit-validate");

	err = suit_process_manifest(&state, SUIT_LOAD);
	if (err) {
		if (err == SUIT_ERR_UNAVAILABLE_COMMAND_SEQ) {
			LOG_DBG("Command sequence suit-load not available - skip it");
			err =  0;
		}
		else {
			LOG_ERR("Failed to execute suit-load: %d", err);
			return enter_emergency_recovery();
		}
	}
	LOG_DBG("Processed suit-load");

	err = suit_process_manifest(&state, SUIT_INVOKE);
	if (err) {
		LOG_ERR("Failed to execute suit-invoke: %d", err);
		return enter_emergency_recovery();
	}
	LOG_DBG("Processed suit-invoke");

	return 0;
}

int suit_orchestrator_init(void)
{
	suit_reset_state(&state);

	int err = load_keys();
	if (err) {
		LOG_ERR("Failed to load keys: %d", err);
		return err;
	}

	err = suit_storage_init();
	if (err) {
		LOG_ERR("Failed to init suit storage: %d", err);
		return err;
	}

	LOG_DBG("SUIT orchestrator init ok");
	return 0;
}

int suit_orchestrator_entry(void)
{
	if (suit_storage_update_available()) {
		LOG_INF("Update path");
		return update_path();
	}
	else {
		LOG_INF("Boot path");
		return boot_path();
	}
}
