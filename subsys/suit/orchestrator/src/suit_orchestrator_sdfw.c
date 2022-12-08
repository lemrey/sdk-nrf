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
#include <suit_storage.h>

LOG_MODULE_REGISTER(suit, CONFIG_SUIT_LOG_LEVEL);

#define SUIT_ORCHESTRATOR_MANIFEST_INDEX_ROOT 0

static uint8_t *installed_envelope_address = NULL;
static size_t installed_envelope_size = 0;
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

static int update_path(void)
{
	LOG_WRN("TODO: Implement update path");
	return 0;
}

static int validate_installed_root_manifest(void)
{
	int err = suit_decode_envelope(installed_envelope_address, installed_envelope_size, &state);

	if (err) {
		LOG_ERR("Failed to decode SUIT envelope: %d", err);
		return err;
	}
	LOG_DBG("SUIT envelope decoded");

	err = suit_validate_envelope(&state);
	if (err) {
		LOG_ERR("Failed to validate SUIT envelope: %d", err);
		return err;
	}
	LOG_DBG("SUIT envelope validated");

	err = suit_decode_manifest(&state);
	if (err) {
		LOG_ERR("Failed to decode SUIT manifest: %d", err);
		return err;
	}
	LOG_DBG("SUIT manifest decoded");

	err = suit_validate_manifest(&state);
	if (err) {
		LOG_ERR("Failed to validate SUIT manifest: %d", err);
		return err;
	}

	LOG_DBG("SUIT manifest validated");
	return 0;
}

static int boot_path(void)
{
	int err = suit_storage_installed_envelope_get(SUIT_ORCHESTRATOR_MANIFEST_INDEX_ROOT, (void **)(&installed_envelope_address), &installed_envelope_size);

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

	err = validate_installed_root_manifest();
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
	installed_envelope_address = NULL;
	installed_envelope_size = 0;

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
		/* TODO: Remove when update path is implemented */
		LOG_WRN("Simulating installation of envelope as current one");
		suit_storage_install_envelope(SUIT_ORCHESTRATOR_MANIFEST_INDEX_ROOT, (void *)0x1e100000, 2048);

		LOG_INF("Boot path");
		return boot_path();
	}
}
