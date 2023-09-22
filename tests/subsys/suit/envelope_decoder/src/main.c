/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <stdint.h>
#include <psa/crypto.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <platform_mem_util.h>
#ifdef CONFIG_FLASH_SIMULATOR
#include <zephyr/drivers/flash/flash_simulator.h>
#endif /* CONFIG_FLASH_SIMULATOR */

#include <suit.h>
#include <suit_platform.h>

#ifdef CONFIG_FLASH_SIMULATOR
static uint8_t *f_base_address = NULL;
#endif /* CONFIG_FLASH_SIMULATOR */

#define DFU_PARTITION_ADDRESS ((uint8_t *)FLASH_ADDRESS(DFU_PARTITION_OFFSET))
#define DFU_PARTITION_OFFSET  ((size_t)FIXED_PARTITION_OFFSET(dfu_partition))
#define DFU_PARTITION_SIZE    ((size_t)FIXED_PARTITION_SIZE(dfu_partition))

static const uint8_t public_key[] = {
	0x04, /* POINT_CONVERSION_UNCOMPRESSED */
	0xed, 0xd0, 0x9e, 0xa5, 0xec, 0xe4, 0xed, 0xbe, 0x6c, 0x08, 0xe7, 0x47, 0x09,
	0x55, 0x9a, 0x38, 0x29, 0xc5, 0x31, 0x33, 0x22, 0x7b, 0xf4, 0xf0, 0x11, 0x6e,
	0x8c, 0x05, 0x2d, 0x02, 0x0e, 0x0e, 0xc3, 0xe0, 0xd8, 0x37, 0xf4, 0xc2, 0x6f,
	0xc1, 0x28, 0x80, 0x2f, 0x45, 0x38, 0x1a, 0x23, 0x2b, 0x6d, 0xd5, 0xda, 0x28,
	0x60, 0x00, 0x5d, 0xab, 0xe2, 0xa0, 0x83, 0xdb, 0xef, 0x38, 0x55, 0x13};
static psa_key_id_t public_key_id;

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
	zassert_equal(err, 0, "Unable to initialize crypto backend\n");

#ifdef CONFIG_FLASH_SIMULATOR
	const struct device *fdev = DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));
	zassert_true(device_is_ready(fdev), "Unable to initialize flash simulator device.");

	size_t f_size = 0;
	f_base_address = flash_simulator_get_memory(fdev, &f_size);
#endif /* CONFIG_FLASH_SIMULATOR */

	err = suit_processor_init();
	zassert_equal(err, 0, "Unable to initialise SUIT processor (err: %d)", err);

	return NULL;
}

static void test_suit_teardown(void *f)
{
	int err = destroy_keys();
	zassert_equal(err, 0, "Unable to uninitialize crypto backend\n");
}

ZTEST_SUITE(envelope_decoder, NULL, test_suit_setup, NULL, NULL, test_suit_teardown);

ZTEST(envelope_decoder, test_get_manifest_authenticated_metadata)
{
	int err = suit_processor_get_manifest_metadata(DFU_PARTITION_ADDRESS, DFU_PARTITION_SIZE,
						       false, NULL, NULL, NULL);
	zassert_equal(err, 0, "Reading authenticated envelope failed (err: %d)", err);
}

ZTEST(envelope_decoder, test_get_manifest_unauthenticated_metadata)
{
	int err = suit_processor_get_manifest_metadata(DFU_PARTITION_ADDRESS, DFU_PARTITION_SIZE,
						       true, NULL, NULL, NULL);
	zassert_equal(err, 0, "Reading unauthenticated envelope failed (err: %d)", err);
}

ZTEST(envelope_decoder, test_process_sequence_parse)
{
	int err = suit_process_sequence(DFU_PARTITION_ADDRESS, DFU_PARTITION_SIZE, SUIT_SEQ_PARSE);
	zassert_equal(err, 0, "Parsing SUIT envelope failed (err: %d)", err);
}
