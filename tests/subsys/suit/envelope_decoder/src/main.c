/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <stdint.h>
#include <psa/crypto.h>
#include <drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#ifdef CONFIG_FLASH_SIMULATOR
#include <zephyr/drivers/flash/flash_simulator.h>
#endif /* CONFIG_FLASH_SIMULATOR */

#include <suit.h>
#include <suit_platform.h>


/* Calculate absolute address from flash offset. */
#ifdef CONFIG_FLASH_SIMULATOR
static uint8_t *f_base_address = NULL;
#define FLASH_ADDRESS(address)  (f_base_address + address)
#else /* CONFIG_FLASH_SIMULATOR */
#define FLASH_ADDRESS(address)  (COND_CODE_1(DT_NODE_EXISTS(DT_NODELABEL(mram10)), \
	((address) + (DT_REG_ADDR(DT_NODELABEL(mram10)) & 0xEFFFFFFFUL)), \
	(address)))
#endif /* CONFIG_FLASH_SIMULATOR */

#define DFU_PARTITION_ADDRESS   ((uint8_t *)FLASH_ADDRESS(DFU_PARTITION_OFFSET))
#define DFU_PARTITION_OFFSET    ((size_t)FLASH_AREA_OFFSET(dfu_partition))
#define DFU_PARTITION_SIZE      ((size_t)FLASH_AREA_SIZE(dfu_partition))


static const uint8_t public_key[] = { 0x04, /* POINT_CONVERSION_UNCOMPRESSED */
				      0xed, 0xd0, 0x9e, 0xa5, 0xec, 0xe4, 0xed, 0xbe, 0x6c, 0x08,
				      0xe7, 0x47, 0x09, 0x55, 0x9a, 0x38, 0x29, 0xc5, 0x31, 0x33,
				      0x22, 0x7b, 0xf4, 0xf0, 0x11, 0x6e, 0x8c, 0x05, 0x2d, 0x02,
				      0x0e, 0x0e, 0xc3, 0xe0, 0xd8, 0x37, 0xf4, 0xc2, 0x6f, 0xc1,
				      0x28, 0x80, 0x2f, 0x45, 0x38, 0x1a, 0x23, 0x2b, 0x6d, 0xd5,
				      0xda, 0x28, 0x60, 0x00, 0x5d, 0xab, 0xe2, 0xa0, 0x83, 0xdb,
				      0xef, 0x38, 0x55, 0x13 };
static psa_key_id_t public_key_id;
static struct suit_processor_state state;


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


static void test_suit_load_keys(void)
{
	int err = load_keys();
	zassert_equal(err, 0, "Unable to initialize crypto backend (%d)", err);
}

static void test_suit_reset_state(void)
{
#ifdef CONFIG_FLASH_SIMULATOR
	const struct device *fdev = DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));
	zassert_true(device_is_ready(fdev), "Unable to initialize flash simulator device.");

	size_t f_size = 0;
	f_base_address = flash_simulator_get_memory(fdev, &f_size);
#endif /* CONFIG_FLASH_SIMULATOR */

	suit_reset_state(&state);
}

static void test_suit_decode_envelope(void)
{
	int err = suit_decode_envelope(DFU_PARTITION_ADDRESS, DFU_PARTITION_SIZE, &state);
	zassert_equal(err, 0, "suit_decode_envelope failed (%d)", err);
}

static void test_suit_validate_envelope(void)
{
	int err = suit_validate_envelope(&state);
	zassert_equal(err, 0, "suit_validate_envelope failed (%d)", err);
}

static void test_suit_decode_manifest(void)
{
	int err = suit_decode_manifest(&state);
	zassert_equal(err, 0, "suit_decode_manifest failed (%d)", err);
}

static void test_suit_validate_manifest(void)
{
	int err = suit_validate_manifest(&state);
	zassert_equal(err, 0, "suit_validate_manifest failed (%d)", err);
}

static void test_suit_destroy_keys(void)
{
	int err = destroy_keys();
	zassert_equal(err, 0, "Unable to uninitialize crypto backend (%d)", err);
}


void test_main(void)
{
	ztest_test_suite(test_suit_envelope_decoder,
			 ztest_unit_test(test_suit_load_keys),
			 ztest_unit_test(test_suit_reset_state),
			 ztest_unit_test(test_suit_decode_envelope),
			 ztest_unit_test(test_suit_validate_envelope),
			 ztest_unit_test(test_suit_decode_manifest),
			 ztest_unit_test(test_suit_validate_manifest),
			 ztest_unit_test(test_suit_destroy_keys));

	ztest_run_test_suite(test_suit_envelope_decoder);
}
