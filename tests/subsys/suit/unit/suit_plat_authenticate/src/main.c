/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <suit_platform.h>
#include <mocks.h>


static suit_manifest_class_id_t sample_class_id = {{0xca, 0xd8, 0x52, 0x3a, 0xf8, 0x29, 0x5a, 0x9a,
						    0xba, 0x85, 0x2e, 0xa0, 0xb2, 0xf5, 0x77,
						    0xc9}};
static uint32_t sample_integer_key_id = 0x7fffffe0;
static enum suit_cose_alg valid_cose_alg = suit_cose_es256;
static psa_algorithm_t valid_psa_alg = PSA_ALG_ECDSA(PSA_ALG_SHA_256);

static struct zcbor_string valid_manifest_component_id = {
	.value = (const uint8_t *)0x1234,
	.len = 123,
};

struct zcbor_string valid_key_id = {.value = (const uint8_t *)0x3456, .len = 123};
struct zcbor_string valid_signature = {.value = (const uint8_t *)0x5678, .len = 123};
struct zcbor_string valid_data = {
	.value = (const uint8_t *)0x7890,
	.len = 8,
};

static void test_before(void *data)
{
	/* Reset mocks */
	mocks_reset();

	/* Reset common FFF internal structures */
	FFF_RESET_HISTORY();
}

ZTEST_SUITE(suit_platform_crypto_psa_authenticate_tests, NULL, NULL, test_before, NULL, NULL);

static suit_plat_err_t
suit_plat_decode_manifest_class_id_correct_fake_func(struct zcbor_string *component_id,
						     suit_manifest_class_id_t **class_id)
{
	zassert_equal(&valid_manifest_component_id, component_id,
		      "Invalid manifest component ID value");
	zassert_not_equal(class_id, NULL,
			  "The API must provide a valid pointer, to decode manifest class ID");
	*class_id = &sample_class_id;

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t
suit_plat_decode_manifest_class_id_invalid_fake_func(struct zcbor_string *component_id,
						     suit_manifest_class_id_t **class_id)
{
	zassert_equal(&valid_manifest_component_id, component_id,
		      "Invalid manifest component ID value");
	zassert_not_equal(class_id, NULL,
			  "The API must provide a valid pointer, to decode manifest class ID");
	*class_id = NULL;

	return SUIT_PLAT_ERR_CRASH;
}

static int
suit_mci_manifest_class_id_validate_correct_fake_func(const suit_manifest_class_id_t *class_id)
{
	zassert_equal(class_id, &sample_class_id, "Invalid manifest class ID value");

	return SUIT_PLAT_SUCCESS;
}

static int
suit_mci_manifest_class_id_validate_invalid_fake_func(const suit_manifest_class_id_t *class_id)
{
	zassert_equal(class_id, &sample_class_id, "Invalid manifest class ID value");

	return SUIT_FAIL_CONDITION;
}

static suit_plat_err_t suit_plat_decode_key_id_correct_fake_func(struct zcbor_string *key_id,
						      uint32_t *integer_key_id)
{
	zassert_equal(&valid_key_id, key_id, "Invalid key ID value");
	zassert_not_equal(integer_key_id, NULL,
			  "The API must provide a valid pointer, to decode key ID");
	*integer_key_id = sample_integer_key_id;

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t suit_plat_decode_key_id_invalid_fake_func(struct zcbor_string *key_id,
						      uint32_t *integer_key_id)
{
	zassert_equal(&valid_key_id, key_id, "Invalid key ID value");
	zassert_not_equal(integer_key_id, NULL,
			  "The API must provide a valid pointer, to decode key ID");
	*integer_key_id = 0;

	return SUIT_PLAT_ERR_CRASH;
}

static int suit_mci_signing_key_id_validate_correct_fake_func(
							 const suit_manifest_class_id_t *class_id,
							 uint32_t key_id)
{
	zassert_equal(class_id, &sample_class_id, "Invalid manifest class ID value");
	zassert_equal(key_id, sample_integer_key_id, "Invalid key ID value");

	return SUIT_PLAT_SUCCESS;
}

static int suit_mci_signing_key_id_validate_invalid_fake_func(
							 const suit_manifest_class_id_t *class_id,
							 uint32_t key_id)
{
	zassert_equal(class_id, &sample_class_id, "Invalid manifest class ID value");
	zassert_equal(key_id, sample_integer_key_id, "Invalid key ID value");

	return MCI_ERR_MANIFESTCLASSID;
}

static int psa_verify_message_correct_fake_func(mbedtls_svc_key_id_t key, psa_algorithm_t alg,
						const uint8_t *input, size_t input_length,
						const uint8_t *signature, size_t signature_length)
{
	zassert_equal(key, sample_integer_key_id, "Invalid key ID value");
	zassert_equal(alg, valid_psa_alg, "Invalid alg value");
	zassert_equal(input, valid_data.value, "Invalid input value");
	zassert_equal(input_length, valid_data.len, "Invalid input length");
	zassert_equal(signature, valid_signature.value, "Invalid signature value");
	zassert_equal(signature_length, valid_signature.len, "Invalid signature length");

	return PSA_SUCCESS;
}

static int psa_verify_message_correct_eddsa_fake_func(mbedtls_svc_key_id_t key, psa_algorithm_t alg,
						      const uint8_t *input, size_t input_length,
						      const uint8_t *signature,
						      size_t signature_length)
{
	zassert_equal(key, sample_integer_key_id, "Invalid key ID value");
	zassert_equal(alg, PSA_ALG_ED25519PH, "Invalid alg value");
	zassert_equal(input, valid_data.value, "Invalid input value");
	zassert_equal(input_length, valid_data.len, "Invalid input length");
	zassert_equal(signature, valid_signature.value, "Invalid signature value");
	zassert_equal(signature_length, valid_signature.len, "Invalid signature length");

	return PSA_SUCCESS;
}

static int psa_verify_message_invalid_fake_func(mbedtls_svc_key_id_t key, psa_algorithm_t alg,
						const uint8_t *input, size_t input_length,
						const uint8_t *signature, size_t signature_length)
{
	zassert_equal(key, sample_integer_key_id, "Invalid key ID value");
	zassert_equal(alg, valid_psa_alg, "Invalid alg value");
	zassert_equal(input, valid_data.value, "Invalid input value");
	zassert_equal(input_length, valid_data.len, "Invalid input length");
	zassert_equal(signature, valid_signature.value, "Invalid signature value");
	zassert_equal(signature_length, valid_signature.len, "Invalid signature length");

	return PSA_ERROR_CORRUPTION_DETECTED;
}

/****** suit_plat_authenticate_manifest ******/
ZTEST(suit_platform_crypto_psa_authenticate_tests, test_null_args)
{
	int ret =
		suit_plat_authenticate_manifest(NULL, // struct zcbor_string *manifest_component_id
						valid_cose_alg, // enum suit_cose_alg alg_id
						NULL,		// struct zcbor_string *key_id
						NULL,		// struct zcbor_string *signature
						NULL);		// struct zcbor_string *data

	/* Manifest authentication fails */
	zassert_equal(SUIT_ERR_DECODING, ret, "Failed to catch null argument");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_plat_decode_key_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_key_id_fake() calls");
	zassert_equal(suit_mci_signing_key_id_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_signing_key_id_validate() calls");
	zassert_equal(psa_verify_message_fake.call_count, 0,
		      "Incorrect number of psa_verify_message() calls");
}

ZTEST(suit_platform_crypto_psa_authenticate_tests, test_null_manifest_component_id)
{
	struct zcbor_string invalid_manifest_component_id = {
		.value = NULL,
		.len = 8,
	};

	int ret = suit_plat_authenticate_manifest(
		&invalid_manifest_component_id, // struct zcbor_string *manifest_component_id
		valid_cose_alg,			// enum suit_cose_alg alg_id
		NULL,				// struct zcbor_string *key_id
		NULL,				// struct zcbor_string *signature
		NULL);				// struct zcbor_string *data

	/* Manifest authentication fails */
	zassert_equal(SUIT_ERR_DECODING, ret, "Failed to catch null argument");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_plat_decode_key_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_key_id_fake() calls");
	zassert_equal(suit_mci_signing_key_id_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_signing_key_id_validate() calls");
	zassert_equal(psa_verify_message_fake.call_count, 0,
		      "Incorrect number of psa_verify_message() calls");
}

ZTEST(suit_platform_crypto_psa_authenticate_tests, test_null_key_id)
{
	struct zcbor_string invalid_key_id = {
		.value = NULL,
		.len = 8,
	};

	int ret = suit_plat_authenticate_manifest(
		&valid_manifest_component_id, // struct zcbor_string *manifest_component_id
		valid_cose_alg,		      // enum suit_cose_alg alg_id
		&invalid_key_id,	      // struct zcbor_string *key_id
		NULL,			      // struct zcbor_string *signature
		NULL);			      // struct zcbor_string *data

	/* Manifest authentication fails */
	zassert_equal(SUIT_ERR_DECODING, ret, "Failed to catch null argument");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_plat_decode_key_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_key_id_fake() calls");
	zassert_equal(suit_mci_signing_key_id_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_signing_key_id_validate() calls");
	zassert_equal(psa_verify_message_fake.call_count, 0,
		      "Incorrect number of psa_verify_message() calls");
}

ZTEST(suit_platform_crypto_psa_authenticate_tests, test_null_signature)
{
	struct zcbor_string invalid_signature = {
		.value = NULL,
		.len = 8,
	};

	int ret = suit_plat_authenticate_manifest(
		&valid_manifest_component_id, // struct zcbor_string *manifest_component_id
		valid_cose_alg,		      // enum suit_cose_alg alg_id
		&valid_key_id,		      // struct zcbor_string *key_id
		&invalid_signature,	      // struct zcbor_string *signature
		NULL);			      // struct zcbor_string *data

	/* Manifest authentication fails */
	zassert_equal(SUIT_ERR_DECODING, ret, "Failed to catch null argument");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_plat_decode_key_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_key_id_fake() calls");
	zassert_equal(suit_mci_signing_key_id_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_signing_key_id_validate() calls");
	zassert_equal(psa_verify_message_fake.call_count, 0,
		      "Incorrect number of psa_verify_message() calls");
}

ZTEST(suit_platform_crypto_psa_authenticate_tests, test_null_data)
{
	struct zcbor_string invalid_data = {
		.value = NULL,
		.len = 8,
	};

	int ret = suit_plat_authenticate_manifest(
		&valid_manifest_component_id, // struct zcbor_string *manifest_component_id
		valid_cose_alg,		      // enum suit_cose_alg alg_id
		&valid_key_id,		      // struct zcbor_string *key_id
		&valid_signature,	      // struct zcbor_string *signature
		&invalid_data);		      // struct zcbor_string *data

	/* Manifest authentication fails */
	zassert_equal(SUIT_ERR_DECODING, ret, "Failed to catch null argument");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_plat_decode_key_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_key_id_fake() calls");
	zassert_equal(suit_mci_signing_key_id_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_signing_key_id_validate() calls");
	zassert_equal(psa_verify_message_fake.call_count, 0,
		      "Incorrect number of psa_verify_message() calls");
}

ZTEST(suit_platform_crypto_psa_authenticate_tests, test_invalid_decode_manifest_class_id)
{
	suit_plat_decode_manifest_class_id_fake.custom_fake =
		suit_plat_decode_manifest_class_id_invalid_fake_func;

	int ret = suit_plat_authenticate_manifest(
		&valid_manifest_component_id, // struct zcbor_string *manifest_component_id
		valid_cose_alg,		      // enum suit_cose_alg alg_id
		&valid_key_id,		      // struct zcbor_string *key_id
		&valid_signature,	      // struct zcbor_string *signature
		&valid_data);		      // struct zcbor_string *data

	/* Manifest authentication fails */
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret,
		      "Failed to catch invalid manifest_component_id");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_plat_decode_key_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_key_id_fake() calls");
	zassert_equal(suit_mci_signing_key_id_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_signing_key_id_validate() calls");
	zassert_equal(psa_verify_message_fake.call_count, 0,
		      "Incorrect number of psa_verify_message() calls");
}

ZTEST(suit_platform_crypto_psa_authenticate_tests, test_invalid_validate_manifest_class_id)
{
	suit_plat_decode_manifest_class_id_fake.custom_fake =
		suit_plat_decode_manifest_class_id_correct_fake_func;
	suit_mci_manifest_class_id_validate_fake.custom_fake =
		suit_mci_manifest_class_id_validate_invalid_fake_func;

	int ret = suit_plat_authenticate_manifest(
		&valid_manifest_component_id, // struct zcbor_string *manifest_component_id
		valid_cose_alg,		      // enum suit_cose_alg alg_id
		&valid_key_id,		      // struct zcbor_string *key_id
		&valid_signature,	      // struct zcbor_string *signature
		&valid_data);		      // struct zcbor_string *data

	/* Manifest authentication fails */
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret,
		      "Failed to catch invalid manifest class ID");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 1,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_plat_decode_key_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_key_id_fake() calls");
	zassert_equal(suit_mci_signing_key_id_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_signing_key_id_validate() calls");
	zassert_equal(psa_verify_message_fake.call_count, 0,
		      "Incorrect number of psa_verify_message() calls");
}

ZTEST(suit_platform_crypto_psa_authenticate_tests, test_invalid_decode_key_id)
{
	suit_plat_decode_manifest_class_id_fake.custom_fake =
		suit_plat_decode_manifest_class_id_correct_fake_func;
	suit_mci_manifest_class_id_validate_fake.custom_fake =
		suit_mci_manifest_class_id_validate_correct_fake_func;
	suit_plat_decode_key_id_fake.custom_fake = suit_plat_decode_key_id_invalid_fake_func;

	int ret = suit_plat_authenticate_manifest(
		&valid_manifest_component_id, // struct zcbor_string *manifest_component_id
		valid_cose_alg,		      // enum suit_cose_alg alg_id
		&valid_key_id,		      // struct zcbor_string *key_id
		&valid_signature,	      // struct zcbor_string *signature
		&valid_data);		      // struct zcbor_string *data

	/* Manifest authentication fails */
	zassert_equal(SUIT_ERR_UNSUPPORTED_PARAMETER, ret, "Failed to catch invalid key ID");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 1,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_plat_decode_key_id_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_key_id_fake() calls");
	zassert_equal(suit_mci_signing_key_id_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_signing_key_id_validate() calls");
	zassert_equal(psa_verify_message_fake.call_count, 0,
		      "Incorrect number of psa_verify_message() calls");
}

ZTEST(suit_platform_crypto_psa_authenticate_tests, test_invalid_key_id_validation)
{
	suit_plat_decode_manifest_class_id_fake.custom_fake =
		suit_plat_decode_manifest_class_id_correct_fake_func;
	suit_mci_manifest_class_id_validate_fake.custom_fake =
		suit_mci_manifest_class_id_validate_correct_fake_func;
	suit_plat_decode_key_id_fake.custom_fake = suit_plat_decode_key_id_correct_fake_func;
	suit_mci_signing_key_id_validate_fake.custom_fake =
		suit_mci_signing_key_id_validate_invalid_fake_func;

	int ret = suit_plat_authenticate_manifest(
		&valid_manifest_component_id, // struct zcbor_string *manifest_component_id
		valid_cose_alg,		      // enum suit_cose_alg alg_id
		&valid_key_id,		      // struct zcbor_string *key_id
		&valid_signature,	      // struct zcbor_string *signature
		&valid_data);		      // struct zcbor_string *data

	/* Manifest authentication fails */
	zassert_equal(SUIT_ERR_AUTHENTICATION, ret, "Failed to catch unsupported key ID");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 1,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_plat_decode_key_id_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_key_id_fake() calls");
	zassert_equal(suit_mci_signing_key_id_validate_fake.call_count, 1,
		      "Incorrect number of suit_mci_signing_key_id_validate() calls");
	zassert_equal(psa_verify_message_fake.call_count, 0,
		      "Incorrect number of psa_verify_message() calls");
}

ZTEST(suit_platform_crypto_psa_authenticate_tests, test_invalid_psa_verify_message)
{
	suit_plat_decode_manifest_class_id_fake.custom_fake =
		suit_plat_decode_manifest_class_id_correct_fake_func;
	suit_mci_manifest_class_id_validate_fake.custom_fake =
		suit_mci_manifest_class_id_validate_correct_fake_func;
	suit_plat_decode_key_id_fake.custom_fake = suit_plat_decode_key_id_correct_fake_func;
	suit_mci_signing_key_id_validate_fake.custom_fake =
		suit_mci_signing_key_id_validate_correct_fake_func;
	psa_verify_message_fake.custom_fake = psa_verify_message_invalid_fake_func;

	int ret = suit_plat_authenticate_manifest(
		&valid_manifest_component_id, // struct zcbor_string *manifest_component_id
		valid_cose_alg,		      // enum suit_cose_alg alg_id
		&valid_key_id,		      // struct zcbor_string *key_id
		&valid_signature,	      // struct zcbor_string *signature
		&valid_data);		      // struct zcbor_string *data

	/* Manifest authentication fails */
	zassert_not_equal(SUIT_SUCCESS, ret, "Verification should have failed");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 1,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_plat_decode_key_id_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_key_id_fake() calls");
	zassert_equal(suit_mci_signing_key_id_validate_fake.call_count, 1,
		      "Incorrect number of suit_mci_signing_key_id_validate() calls");
	zassert_equal(psa_verify_message_fake.call_count, 1,
		      "Incorrect number of psa_verify_message() calls");
}

ZTEST(suit_platform_crypto_psa_authenticate_tests, test_OK)
{
	suit_plat_decode_manifest_class_id_fake.custom_fake =
		suit_plat_decode_manifest_class_id_correct_fake_func;
	suit_mci_manifest_class_id_validate_fake.custom_fake =
		suit_mci_manifest_class_id_validate_correct_fake_func;
	suit_plat_decode_key_id_fake.custom_fake = suit_plat_decode_key_id_correct_fake_func;
	suit_mci_signing_key_id_validate_fake.custom_fake =
		suit_mci_signing_key_id_validate_correct_fake_func;
	psa_verify_message_fake.custom_fake = psa_verify_message_correct_fake_func;

	int ret = suit_plat_authenticate_manifest(
		&valid_manifest_component_id, // struct zcbor_string *manifest_component_id
		valid_cose_alg,		      // enum suit_cose_alg alg_id
		&valid_key_id,		      // struct zcbor_string *key_id
		&valid_signature,	      // struct zcbor_string *signature
		&valid_data);		      // struct zcbor_string *data

	/* Manifest authentication succeeds */
	zassert_equal(SUIT_SUCCESS, ret, "Authentication should have succeeded");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 1,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_plat_decode_key_id_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_key_id_fake() calls");
	zassert_equal(suit_mci_signing_key_id_validate_fake.call_count, 1,
		      "Incorrect number of suit_mci_signing_key_id_validate() calls");
	zassert_equal(psa_verify_message_fake.call_count, 1,
		      "Incorrect number of psa_verify_message() calls");
}

ZTEST(suit_platform_crypto_psa_authenticate_tests, test_OK_EdDSA)
{
	suit_plat_decode_manifest_class_id_fake.custom_fake =
		suit_plat_decode_manifest_class_id_correct_fake_func;
	suit_mci_manifest_class_id_validate_fake.custom_fake =
		suit_mci_manifest_class_id_validate_correct_fake_func;
	suit_plat_decode_key_id_fake.custom_fake = suit_plat_decode_key_id_correct_fake_func;
	suit_mci_signing_key_id_validate_fake.custom_fake =
		suit_mci_signing_key_id_validate_correct_fake_func;
	psa_verify_message_fake.custom_fake = psa_verify_message_correct_eddsa_fake_func;

	int ret = suit_plat_authenticate_manifest(
		&valid_manifest_component_id, // struct zcbor_string *manifest_component_id
		suit_cose_EdDSA,		      // enum suit_cose_alg alg_id
		&valid_key_id,		      // struct zcbor_string *key_id
		&valid_signature,	      // struct zcbor_string *signature
		&valid_data);		      // struct zcbor_string *data

	/* Manifest authentication succeeds */
	zassert_equal(SUIT_SUCCESS, ret, "Authentication should have succeeded");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 1,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_plat_decode_key_id_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_key_id_fake() calls");
	zassert_equal(suit_mci_signing_key_id_validate_fake.call_count, 1,
		      "Incorrect number of suit_mci_signing_key_id_validate() calls");
	zassert_equal(psa_verify_message_fake.call_count, 1,
		      "Incorrect number of psa_verify_message() calls");
}

ZTEST(suit_platform_crypto_psa_authenticate_tests, test_unsupported_alg)
{
	int ret = suit_plat_authenticate_manifest(
		&valid_manifest_component_id, // struct zcbor_string *manifest_component_id
		0xAA,		              // enum suit_cose_alg alg_id
		&valid_key_id,		      // struct zcbor_string *key_id
		&valid_signature,	      // struct zcbor_string *signature
		&valid_data);		      // struct zcbor_string *data

	/* Manifest authentication succeeds */
	zassert_equal(SUIT_ERR_DECODING, ret, "Authentication should have failed");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_plat_decode_key_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_key_id_fake() calls");
	zassert_equal(suit_mci_signing_key_id_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_signing_key_id_validate() calls");
	zassert_equal(psa_verify_message_fake.call_count, 0,
		      "Incorrect number of psa_verify_message() calls");
}

/****** suit_plat_authorize_unsigned_manifest ******/
ZTEST_SUITE(suit_platform_authenticate_unsigned_tests, NULL, NULL, test_before, NULL, NULL);

static int suit_mci_signing_key_id_validate_0_correct_fake_func(
							 const suit_manifest_class_id_t *class_id,
							 uint32_t key_id)
{
	zassert_equal(class_id, &sample_class_id, "Invalid manifest class ID value");
	zassert_equal(key_id, 0, "Invalid key ID value");

	return SUIT_PLAT_SUCCESS;
}

static int suit_mci_signing_key_id_validate_0_invalid_fake_func(
							 const suit_manifest_class_id_t *class_id,
							 uint32_t key_id)
{
	zassert_equal(class_id, &sample_class_id, "Invalid manifest class ID value");
	zassert_equal(key_id, 0, "Invalid key ID value");

	return MCI_ERR_MANIFESTCLASSID;
}

ZTEST(suit_platform_authenticate_unsigned_tests, test_null_manifest_component_id)
{
	int ret = suit_plat_authorize_unsigned_manifest(NULL);

	/* Manifest authentication fails */
	zassert_equal(SUIT_ERR_DECODING, ret, "Failed to catch null argument");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_plat_decode_key_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_key_id_fake() calls");
	zassert_equal(suit_mci_signing_key_id_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_signing_key_id_validate() calls");
	zassert_equal(psa_verify_message_fake.call_count, 0,
		      "Incorrect number of psa_verify_message() calls");
}

ZTEST(suit_platform_authenticate_unsigned_tests, test_invalid_manifest_component_id_null_value)
{
	struct zcbor_string invalid_manifest_component_id = {
		.value = NULL,
		.len = 8,
	};

	int ret = suit_plat_authorize_unsigned_manifest(&invalid_manifest_component_id);

	/* Manifest authentication fails */
	zassert_equal(SUIT_ERR_DECODING, ret, "Failed to catch null argument");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_decode_key_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_key_id_fake() calls");
	zassert_equal(suit_mci_signing_key_id_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_signing_key_id_validate() calls");
	zassert_equal(psa_verify_message_fake.call_count, 0,
		      "Incorrect number of psa_verify_message() calls");
}

ZTEST(suit_platform_authenticate_unsigned_tests, test_invalid_manifest_component_id_0_len)
{
	struct zcbor_string invalid_manifest_component_id = {
		.value = (const uint8_t *)0x1234,
		.len = 0,
	};

	int ret = suit_plat_authorize_unsigned_manifest(&invalid_manifest_component_id);

	/* Manifest authentication fails */
	zassert_equal(SUIT_ERR_DECODING, ret, "Failed to catch null argument");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_decode_key_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_key_id_fake() calls");
	zassert_equal(suit_mci_signing_key_id_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_signing_key_id_validate() calls");
	zassert_equal(psa_verify_message_fake.call_count, 0,
		      "Incorrect number of psa_verify_message() calls");
}

ZTEST(suit_platform_authenticate_unsigned_tests, test_unsigned_manifest_invalid_decode_class_id)
{
	suit_plat_decode_manifest_class_id_fake.custom_fake
		= suit_plat_decode_manifest_class_id_invalid_fake_func;
	suit_mci_manifest_class_id_validate_fake.custom_fake
		= suit_mci_manifest_class_id_validate_invalid_fake_func;
	suit_mci_signing_key_id_validate_fake.custom_fake = suit_mci_signing_key_id_validate_0_invalid_fake_func;

	int ret = suit_plat_authorize_unsigned_manifest(&valid_manifest_component_id);

	/* Unsigned manifest authentication fails */
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Verification should have failed");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_mci_signing_key_id_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_signing_key_id_validate() calls");
}

ZTEST(suit_platform_authenticate_unsigned_tests, test_unsigned_manifest_invalid_validate_class_id)
{
	suit_plat_decode_manifest_class_id_fake.custom_fake = suit_plat_decode_manifest_class_id_correct_fake_func;
	suit_mci_manifest_class_id_validate_fake.custom_fake = suit_mci_manifest_class_id_validate_invalid_fake_func;
	suit_mci_signing_key_id_validate_fake.custom_fake = suit_mci_signing_key_id_validate_0_invalid_fake_func;

	int ret = suit_plat_authorize_unsigned_manifest(&valid_manifest_component_id);

	/* Unsigned manifest authentication fails */
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Verification should have failed");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 1,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_mci_signing_key_id_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_signing_key_id_validate() calls");
}

ZTEST(suit_platform_authenticate_unsigned_tests, test_unsigned_manifest_invalid_validate_key_id)
{
	suit_plat_decode_manifest_class_id_fake.custom_fake = suit_plat_decode_manifest_class_id_correct_fake_func;
	suit_mci_manifest_class_id_validate_fake.custom_fake = suit_mci_manifest_class_id_validate_correct_fake_func;
	suit_mci_signing_key_id_validate_fake.custom_fake = suit_mci_signing_key_id_validate_0_invalid_fake_func;

	int ret = suit_plat_authorize_unsigned_manifest(&valid_manifest_component_id);

	/* Unsigned manifest authentication fails */
	zassert_equal(SUIT_ERR_AUTHENTICATION, ret, "Verification should have failed");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 1,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_mci_signing_key_id_validate_fake.call_count, 1,
		      "Incorrect number of suit_mci_signing_key_id_validate() calls");
}

ZTEST(suit_platform_authenticate_unsigned_tests, test_unsigned_manifest_OK)
{
	suit_plat_decode_manifest_class_id_fake.custom_fake = suit_plat_decode_manifest_class_id_correct_fake_func;
	suit_mci_manifest_class_id_validate_fake.custom_fake = suit_mci_manifest_class_id_validate_correct_fake_func;
	suit_mci_signing_key_id_validate_fake.custom_fake = suit_mci_signing_key_id_validate_0_correct_fake_func;

	int ret = suit_plat_authorize_unsigned_manifest(&valid_manifest_component_id);

	/* Unsigned manifest authentication succeeds */
	zassert_equal(SUIT_SUCCESS, ret, "Verification should have succeeded");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 1,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_mci_signing_key_id_validate_fake.call_count, 1,
		      "Incorrect number of suit_mci_signing_key_id_validate() calls");
}
