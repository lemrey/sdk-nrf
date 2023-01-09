/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <stdint.h>
#include <psa/crypto.h>

#include <suit.h>
#include <suit_platform.h>

/** @brief 	SUIT envelope generated using the manifest/sample.json input file.
 *
 * @details This envelope contains 256-byte random numbers attached as an integrated
 *          payload and fetched directly into the memory-mapped region during install step.
 *          The envelope has been signed using the manifest/key_private.pem key.
 *
 * @note 	Please use the manifest/reenerate.sh script for regenerating payloads.
 */
extern uint8_t manifest_buf[];
extern const size_t manifest_len;

static const uint8_t public_key[] = { 0x04, /* POINT_CONVERSION_UNCOMPRESSED */
				      0xed, 0xd0, 0x9e, 0xa5, 0xec, 0xe4, 0xed, 0xbe, 0x6c, 0x08,
				      0xe7, 0x47, 0x09, 0x55, 0x9a, 0x38, 0x29, 0xc5, 0x31, 0x33,
				      0x22, 0x7b, 0xf4, 0xf0, 0x11, 0x6e, 0x8c, 0x05, 0x2d, 0x02,
				      0x0e, 0x0e, 0xc3, 0xe0, 0xd8, 0x37, 0xf4, 0xc2, 0x6f, 0xc1,
				      0x28, 0x80, 0x2f, 0x45, 0x38, 0x1a, 0x23, 0x2b, 0x6d, 0xd5,
				      0xda, 0x28, 0x60, 0x00, 0x5d, 0xab, 0xe2, 0xa0, 0x83, 0xdb,
				      0xef, 0x38, 0x55, 0x13 };
psa_key_id_t public_key_id;

static int load_keys(void)
{
	psa_status_t status = PSA_ERROR_GENERIC_ERROR;

	/* Initialize PSA Crypto */
	status = psa_crypto_init();
	if (status != PSA_SUCCESS) {
		printk("Failed to initialize PSA Crypto\n");
		return SUIT_ERR_CRASH;
	}

	/* Add keys */
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_VERIFY_HASH);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1));

	status = psa_import_key(&key_attributes, public_key, sizeof(public_key), &public_key_id);
	if (status != PSA_SUCCESS) {
		printk("Failed to add public key\n");
		return SUIT_ERR_CRASH;
	}
	printk("Loaded key ID: 0x%x\n", public_key_id);

	return SUIT_SUCCESS;
}

static int destroy_keys(void)
{
	/* Remove public key from the driver */
	int ret = psa_destroy_key(public_key_id);

	return ret;
}

static void test_suit_setup(void)
{
	int err = load_keys();
	zassert_equal(err, 0, "Unable to initialize crypto backend\n");
}

static void test_suit_teardown(void)
{
	int err = destroy_keys();
	zassert_equal(err, 0, "Unable to uninitialize crypto backend\n");
}

/** @brief Authenticate, validate and execute all command sequences from the sample SUIT envelope.
 *
 * @details The main purpose of this test is to give an entry point for SUIT processor platform implementation.
 *          If the platform design requires it, please update the envelope payload, defined at the beginning of this file.
 */
static void test_suit_process(void)
{
	static struct suit_processor_state state;
	int err = SUIT_SUCCESS;

	err = suit_decode_envelope(manifest_buf, manifest_len, &state);
	zassert_equal(err, 0, "suit_decode_envelope failed\n");

	err = suit_validate_envelope(&state);
	zassert_equal(err, 0, "suit_validate_envelope failed\n");

	err = suit_decode_manifest(&state);
	zassert_equal(err, 0, "suit_decode_manifest failed\n");

	err = suit_validate_manifest(&state);
	zassert_equal(err, 0, "suit_validate_manifest failed\n");

	for (enum suit_manifest_step i = SUIT_NO_STEP + 1; (i < SUIT_LAST_STEP) && (err == 0);
	     i++) {
		err = suit_process_manifest(&state, i);
		if (err == SUIT_ERR_UNAVAILABLE_COMMAND_SEQ) {
			err = 0;
		}
		zassert_equal(err, 0, "suit_process_manifest (step: %d) failed\n", i);
	}
}

void test_main(void)
{
	ztest_test_suite(test_suit,
			 ztest_unit_test_setup_teardown(test_suit_process, test_suit_setup,
							test_suit_teardown));

	ztest_run_test_suite(test_suit);
}
