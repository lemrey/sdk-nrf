/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <stdint.h>
#include <psa/crypto.h>

#include <suit.h>
#include <suit_platform.h>
#include <suit_storage.h>

#ifdef CONFIG_ARCH_POSIX
#include <zephyr/drivers/flash.h>
#define SOC_NV_FLASH_NODE	    DT_CHILD(DT_INST(0, zephyr_sim_flash), flash_0)
#define FLASH_SIMULATOR_BASE_OFFSET DT_REG_ADDR(SOC_NV_FLASH_NODE)
#define FLASH_SIMULATOR_FLASH_SIZE  DT_REG_SIZE(SOC_NV_FLASH_NODE)
#endif /* CONFIG_ARCH_POSIX */

/* The SUIT envelope is defined inside the board-specific manfest_*.c file. */
extern uint8_t manifest_buf[];
extern const size_t manifest_len;

static const uint8_t public_key[] = {
	0x04, /* POINT_CONVERSION_UNCOMPRESSED */
	0xed, 0xd0, 0x9e, 0xa5, 0xec, 0xe4, 0xed, 0xbe, 0x6c, 0x08, 0xe7, 0x47, 0x09,
	0x55, 0x9a, 0x38, 0x29, 0xc5, 0x31, 0x33, 0x22, 0x7b, 0xf4, 0xf0, 0x11, 0x6e,
	0x8c, 0x05, 0x2d, 0x02, 0x0e, 0x0e, 0xc3, 0xe0, 0xd8, 0x37, 0xf4, 0xc2, 0x6f,
	0xc1, 0x28, 0x80, 0x2f, 0x45, 0x38, 0x1a, 0x23, 0x2b, 0x6d, 0xd5, 0xda, 0x28,
	0x60, 0x00, 0x5d, 0xab, 0xe2, 0xa0, 0x83, 0xdb, 0xef, 0x38, 0x55, 0x13};
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

static void *test_suit_setup(void)
{
	int err = load_keys();
	zassert_equal(err, 0, "Unable to initialize crypto backend");

#ifdef CONFIG_ARCH_POSIX
	static const struct device *const flash_dev =
		DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));

	err = flash_erase(flash_dev, FLASH_SIMULATOR_BASE_OFFSET, FLASH_SIMULATOR_FLASH_SIZE);
	zassert_equal(0, err, "Unable to erase flash area before test execution");
#endif /* CONFIG_ARCH_POSIX */

	return NULL;
}

static void test_suit_teardown(void *f)
{
	int err = destroy_keys();
	zassert_equal(err, 0, "Unable to uninitialize crypto backend");
}

ZTEST_SUITE(fetch_integrated_payoad_flash_tests, NULL, test_suit_setup, NULL, NULL,
	    test_suit_teardown);

/** @brief Authenticate, validate and execute all command sequences from the sample SUIT envelope.
 *
 * @details The main purpose of this test is to give an entry point for SUIT processor platform
 * implementation. If the platform design requires it, please update the envelope payload, defined
 * at the beginning of this file.
 */
ZTEST(fetch_integrated_payoad_flash_tests, test_suit_process)
{
	int err = SUIT_SUCCESS;

	err = suit_processor_init();
	zassert_equal(err, 0, "Unable to initialise SUIT processor (err: %d)", err);

	const suit_manifest_class_id_t *supported_class_ids[CONFIG_SUIT_STORAGE_N_ENVELOPES];
	size_t supported_class_ids_len = ARRAY_SIZE(supported_class_ids);

	err = mci_get_supported_manifest_class_ids(
		(const suit_manifest_class_id_t **)&supported_class_ids, &supported_class_ids_len);
	zassert_equal(0, err, "Failed to get list of supported manifest class IDs (%d)", err);

	err = suit_storage_init(supported_class_ids, supported_class_ids_len);
	zassert_equal(0, err, "Failed to init suit storage (%d)", err);

	err = suit_process_sequence(manifest_buf, manifest_len, SUIT_SEQ_PARSE);
	zassert_equal(err, 0, "Parsing SUIT envelope failed (err: %d)", err);

	for (enum suit_command_sequence i = SUIT_SEQ_PARSE + 1; (i < SUIT_SEQ_MAX) && (err == 0);
	     i++) {
		err = suit_process_sequence(manifest_buf, manifest_len, i);
		if (err == SUIT_ERR_UNAVAILABLE_COMMAND_SEQ) {
			err = 0;
		}
		zassert_equal(err, 0, "Processing SUIT sequence %d failed with error %i", i, err);
	}
}
