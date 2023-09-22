/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>

#include <suit_orchestrator.h>

#include <suit_storage.h>
#include <suit_types.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <platform_mem_util.h>
#include <update_magic_values.h>
#include <suit_cache.h>
#ifdef CONFIG_FLASH_SIMULATOR
#include <zephyr/drivers/flash/flash_simulator.h>
#endif /* CONFIG_FLASH_SIMULATOR */

#include <psa/crypto.h>

#ifdef CONFIG_FLASH_SIMULATOR
static uint8_t *f_base_address = NULL;
#endif /* CONFIG_FLASH_SIMULATOR */

#define SUIT_STORAGE_ADDRESS FLASH_ADDRESS(SUIT_STORAGE_OFFSET)
#define SUIT_STORAGE_OFFSET  FIXED_PARTITION_OFFSET(suit_storage)
#define SUIT_STORAGE_SIZE    FIXED_PARTITION_SIZE(suit_storage)

#define MAX_PSA_KEY_COUNT  128
#define PSA_DEFAULT_KEY_ID 0x7fffffe0

#ifdef CONFIG_SOC_NRF54H20
/** The value of UUID5("nRF54H20_sample_app"). */
const suit_manifest_class_id_t supported_class_id = {{0x08, 0xc1, 0xb5, 0x99, 0x55, 0xe8, 0x5f,
						      0xbc, 0x9e, 0x76, 0x7b, 0xc2, 0x9c, 0xe1,
						      0xb0, 0x4d}};
#else  /* CONFIG_SOC_NRF54H20 */
/* The value of UUID5("posix_sample_app"). */
const suit_manifest_class_id_t supported_class_id = {{0x56, 0xdc, 0x9a, 0x14, 0x28, 0xd8, 0x52,
						      0xd3, 0xbd, 0x62, 0xe7, 0x7a, 0x08, 0xbc,
						      0x8b, 0x91}};
#endif /* CONFIG_SOC_NRF54H20 */

/* Valid envelope */
extern const uint8_t manifest_valid_buf[];
extern const size_t manifest_valid_len;

/* Manifest generated using component id with size field set to zero */
extern const uint8_t manifest_zero_size_buf[];
extern const size_t manifest_zero_size_len;

/* Originally valid envelope with manipulated single byte */
extern const uint8_t manifest_manipulated_buf[];
extern const size_t manifest_manipulated_len;

/* Envelope generated with "UNSUPPORTED!" as a component type */
extern const uint8_t manifest_unsupported_component_buf[];
extern const size_t manifest_unsupported_component_len;

/* Envelope with manifest version set to 2 */
extern const uint8_t manifest_wrong_version_buf[];
extern const size_t manifest_wrong_version_len;

/* Envelope signed with a different private key */
extern const uint8_t manifest_different_key_buf[];
extern const size_t manifest_different_key_len;

static void *orchestrator_suite_setup(void)
{
#ifdef CONFIG_FLASH_SIMULATOR
	const struct device *fdev = DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));
	size_t f_size = 0;
	f_base_address = flash_simulator_get_memory(fdev, &f_size);
#endif /* CONFIG_FLASH_SIMULATOR */

	return NULL;
}

static void setup_erased_flash(void)
{
	uint32_t *storage = (uint32_t *)SUIT_STORAGE_ADDRESS;

	if (*storage == UPDATE_MAGIC_VALUE_EMPTY) {
		return;
	}

	const struct device *fdev = DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));
	zassert_not_null(fdev, "Unable to find a driver to erase area");

	int err = flash_erase(fdev, SUIT_STORAGE_OFFSET, SUIT_STORAGE_SIZE);
	zassert_equal(0, err, "Unable to erase memory before test execution");
}

static void setup_update_candidate(const uint8_t *buf, size_t len)
{
	const suit_manifest_class_id_t *supported_class_ids[CONFIG_SUIT_STORAGE_N_ENVELOPES];
	size_t supported_class_ids_len = ARRAY_SIZE(supported_class_ids);

	zassert_not_null(buf, "NULL buf");

	suit_plat_mreg_t update_candidate[1] = {{
		.mem = buf,
		.size = len,
	}};

	setup_erased_flash();

	int err = mci_get_supported_manifest_class_ids(
		(const suit_manifest_class_id_t **)&supported_class_ids, &supported_class_ids_len);
	zassert_equal(0, err, "Failed to get list of supported manifest class IDs (%d)", err);

	err = suit_storage_init(supported_class_ids, supported_class_ids_len);
	zassert_equal(0, err, "Failed to init suit storage (%d)", err);

	err = suit_storage_update_cand_set(update_candidate, ARRAY_SIZE(update_candidate));
	zassert_equal(0, err, "Unable to set correct DFU before test execution (0x%x, %d)", buf,
		      len);
}

/* For storing and counting psa keys that were imported */
static psa_key_id_t psa_key_ids[MAX_PSA_KEY_COUNT];
static uint8_t psa_keys_count = 0;

static void setup_use_all_available_psa_key_slots(void)
{
	zassert_equal(PSA_SUCCESS, psa_crypto_init(), "Cannot initialize psa crypto");

	/* Public key to be imported */
	const uint8_t public_key[] = {
		0x04, /* POINT_CONVERSION_UNCOMPRESSED */
		0xed, 0xd0, 0x9e, 0xa5, 0xec, 0xe4, 0xed, 0xbe, 0x6c, 0x08, 0xe7, 0x47, 0x09,
		0x55, 0x9a, 0x38, 0x29, 0xc5, 0x31, 0x33, 0x22, 0x7b, 0xf4, 0xf0, 0x11, 0x6e,
		0x8c, 0x05, 0x2d, 0x02, 0x0e, 0x0e, 0xc3, 0xe0, 0xd8, 0x37, 0xf4, 0xc2, 0x6f,
		0xc1, 0x28, 0x80, 0x2f, 0x45, 0x38, 0x1a, 0x23, 0x2b, 0x6d, 0xd5, 0xda, 0x28,
		0x60, 0x00, 0x5d, 0xab, 0xe2, 0xa0, 0x83, 0xdb, 0xef, 0x38, 0x55, 0x13};
	/* Key attributes to be set */
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_VERIFY_HASH);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1));

	/* Clear key ids */
	memset(psa_key_ids, 0, sizeof(psa_key_ids));

	for (psa_keys_count = 0; psa_keys_count < MAX_PSA_KEY_COUNT; psa_keys_count++) {
		/* Keep importing keys until all available slots are used */
		if (PSA_SUCCESS != psa_import_key(&key_attributes, public_key, sizeof(public_key),
						  &psa_key_ids[psa_keys_count])) {
			break;
		}
	}

	printk("Imported %d keys\n", psa_keys_count);
}

static void teardown_release_all_used_psa_key_slots(void)
{
	printk("Keys to destroy: %d\n", psa_keys_count);

	for (; psa_keys_count > 0; psa_keys_count--) {
		/* Skip the key ID that will be destroyed by the *after* function. */
		if (psa_key_ids[psa_keys_count - 1] == PSA_DEFAULT_KEY_ID) {
			continue;
		}
		int err = psa_destroy_key((mbedtls_svc_key_id_t)psa_key_ids[psa_keys_count - 1]);
		zassert_equal(PSA_SUCCESS, err, "Failed to destroy allocated key with index %d: %d",
			      psa_keys_count - 1, err);
	}

	memset(psa_key_ids, 0, sizeof(psa_key_ids));
}

static void setup_store_update_candidate_data(void *data, size_t len)
{
	setup_erased_flash();
	const struct device *fdev = DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));
	zassert_not_null(fdev, "Unable to find a flash driver");
	zassert_equal(0, flash_write(fdev, (off_t)SUIT_STORAGE_OFFSET, data, len),
		      "Unable to write data to flash");
}

static void destroy_default_key(void *fixture)
{
	int err = psa_destroy_key((mbedtls_svc_key_id_t)PSA_DEFAULT_KEY_ID);
	zassert_equal(PSA_SUCCESS, err, "Failed to destroy the default key %d: %d",
		      PSA_DEFAULT_KEY_ID, err);
}

ZTEST_SUITE(orchestrator_tests, NULL, orchestrator_suite_setup, NULL, destroy_default_key, NULL);

ZTEST(orchestrator_tests, test_boot_path_empty_storage)
{
	/* GIVEN empty flash (suit storage is erased)... */
	setup_erased_flash();
	/* ... and suit orchestrator is initialized */
	zassert_equal(0, suit_orchestrator_init(), "Orchestrator not initialized");

	/* WHEN orchestrator is launched */
	int err = suit_orchestrator_entry();

	/* THEN emergency recovery is triggered... */
	/* ... and orchestrator exits without an error */
	zassert_equal(0, err, "Unexpected error code");
}

ZTEST(orchestrator_tests, test_update_path_successful_update)
{
	uint8_t *addr;
	size_t size;

	/* GIVEN valid update candidate in suit storage... */
	setup_update_candidate(manifest_valid_buf, manifest_valid_len);
	/* ... and suit orchestrator is initialized */
	zassert_equal(0, suit_orchestrator_init(), "Orchestrator not initialized");

	/* WHEN orchestrator is launched */
	int err = suit_orchestrator_entry();

	/* THEN installation is performed successfully... */
	/* ... and orchestrator exits without an error */
	zassert_equal(0, err, "Unexpected error code");
	/* ... and the envelope get installed */
	err = suit_storage_installed_envelope_get(&supported_class_id, &addr, &size);
	zassert_equal(0, err, "The envelope was not installed after successful update");
}

ZTEST(orchestrator_tests, test_boot_path_successful_boot)
{
	uint8_t *addr_boot;
	size_t size_boot;
	uint8_t *addr_update;
	size_t size_update;

	/* GIVEN valid update candidate in suit storage... */
	setup_update_candidate(manifest_valid_buf, manifest_valid_len);
	/* ... and suit orchestrator is initialized... */
	zassert_equal(0, suit_orchestrator_init(), "Orchestrator not initialized first time");
	/* ... and orchestrator is launched first time to perform successful update */
	zassert_equal(0, suit_orchestrator_entry(), "Unsuccessful first orchestrator launch");
	/* ... and the key installed during the first initialization is destroyed */
	destroy_default_key(NULL);
	/* ... and the bootable envelope is installed */
	int err = suit_storage_installed_envelope_get(&supported_class_id, &addr_boot, &size_boot);
	zassert_equal(0, err, "The envelope was not installed after successful update");

	/* WHEN when orchestrator is initialized again (reboot simulation)... */
	zassert_equal(0, suit_orchestrator_init(), "Orchestrator not initialized second time");
	/* ... and orchestrator is launched again */
	err = suit_orchestrator_entry();

	/* THEN installed SW is invoked successfully (orchestrator returns no error) */
	zassert_equal(0, err, "Unexpected error code");
	/* ... and the envelope remains installed */
	err = suit_storage_installed_envelope_get(&supported_class_id, &addr_update, &size_update);
	zassert_equal(0, err, "The envelope was not installed after successful update");
	/* .. and the same index in SUIT storage is used */
	zassert_equal(addr_boot, addr_update,
		      "The envelope was not installed in the same SUIT storage slot");
}

ZTEST(orchestrator_tests, test_update_path_size_zero)
{
	/* GIVEN suit storage contains envelope with component id's size field set to zero... */
	setup_update_candidate(manifest_zero_size_buf, manifest_zero_size_len);
	/* ... and orchestrator is initialized */
	zassert_equal(0, suit_orchestrator_init(), "Failed to setup test");

	/* WHEN orchestrator is launched */
	int err = suit_orchestrator_entry();

	/* THEN it exits with error code */
	zassert_equal(SUIT_ERR_CRASH, err, "Unexpected error code");
}

ZTEST(orchestrator_tests, test_init_cannot_import_key)
{
	/* GIVEN all available psa key slots are occupied */
	setup_use_all_available_psa_key_slots();

	/* Try to initialize orchestrator - failure on key import should happen */

	/* WHEN orchestrator is initialized */
	int err = suit_orchestrator_init();

	/* THEN it should fail on importing psa key */
	zassert_equal(PSA_ERROR_INSUFFICIENT_MEMORY, err, "Unexpected error code");

	/* Release all used psa keys as a cleanup before other tests */
	teardown_release_all_used_psa_key_slots();
}

ZTEST(orchestrator_tests, test_update_path_manipulated_envelope)
{
	/* GIVEN suit storage contains envelope with one of its byte changed to different value...
	 */
	setup_update_candidate(manifest_manipulated_buf, manifest_manipulated_len);
	/* ... and suit orchestrator is initialized */
	zassert_equal(0, suit_orchestrator_init(), "Orchestrator not initialized");

	/* WHEN orchestrator is launched */
	int err = suit_orchestrator_entry();

	/* THEN candidate availability flag is cleared... */
	uint32_t *storage = (uint32_t *)SUIT_STORAGE_ADDRESS;
	zassert_equal(storage[0], (uint32_t)UPDATE_MAGIC_VALUE_AVAILABLE,
		      "Update candidate flag changed");
	zassert_equal(storage[1], 0, "Update candidate presence not cleared");
	/* ... and orchestrator returns success */
	zassert_equal(0, err, "Unexpected error code");
}

ZTEST(orchestrator_tests, test_update_path_candidate_invalid_address_and_size)
{
	/* GIVEN suit storage indicates presence of update candidate... */
	uint32_t data[3];
	data[0] = (uint32_t)UPDATE_MAGIC_VALUE_AVAILABLE;
	/* ... but has empty address field...*/
	data[1] = (uint32_t)UPDATE_MAGIC_VALUE_EMPTY;
	/* ... and has empty size field... */
	data[2] = (uint32_t)UPDATE_MAGIC_VALUE_EMPTY;
	setup_store_update_candidate_data((void *)data, sizeof(data));

	/* ... and orchestrator is initialized */
	zassert_equal(0, suit_orchestrator_init(), "Orchestrator not initialized");

	/* WHEN orchestrator is launched */
	int err = suit_orchestrator_entry();

	/* THEN candidate availability flag is cleared... */
	uint32_t *storage = (uint32_t *)SUIT_STORAGE_ADDRESS;
	zassert_equal(storage[0], (uint32_t)UPDATE_MAGIC_VALUE_AVAILABLE,
		      "Update candidate flag changed");
	zassert_equal(storage[1], 0, "Update candidate presence not cleared");
	/* ... and orchestrator returns success */
	zassert_equal(0, err, "Unexpected error code");
}

ZTEST(orchestrator_tests, test_update_path_candidate_empty_size)
{
	/* GIVEN suit storage has update candidate flag set... */
	uint32_t data[3];
	data[0] = (uint32_t)UPDATE_MAGIC_VALUE_AVAILABLE;
	/* ...and some non-empty and non-zero address field...*/
	data[1] = (uint32_t)0xDEADBEEF;
	/* ... but empty size field... */
	data[2] = (uint32_t)UPDATE_MAGIC_VALUE_EMPTY;
	setup_store_update_candidate_data((void *)data, sizeof(data));

	/* ... and orchestrator is initialized */
	zassert_equal(0, suit_orchestrator_init(), "Orchestrator not initialized");

	/* WHEN orchestrator is launched */
	int err = suit_orchestrator_entry();

	/* THEN candidate availability flag is cleared... */
	uint32_t *storage = (uint32_t *)SUIT_STORAGE_ADDRESS;
	zassert_equal(storage[0], (uint32_t)UPDATE_MAGIC_VALUE_AVAILABLE,
		      "Update candidate flag changed");
	zassert_equal(storage[1], 0, "Update candidate presence not cleared");
	/* ... and orchestrator returns success */
	zassert_equal(0, err, "Unexpected error code");
}

ZTEST(orchestrator_tests, test_update_path_unsupported_component)
{
	/* GIVEN suit storage contains update candidate with unsupported component... */
	setup_update_candidate(manifest_unsupported_component_buf,
			       manifest_unsupported_component_len);
	/* ... and orchestrator is initialized */
	zassert_equal(0, suit_orchestrator_init(), "Orchestrator not initialized");

	/* WHEN orchestrator is launched */
	int err = suit_orchestrator_entry();

	/* THEN candidate availability flag is cleared... */
	uint32_t *storage = (uint32_t *)SUIT_STORAGE_ADDRESS;
	zassert_equal(storage[0], (uint32_t)UPDATE_MAGIC_VALUE_AVAILABLE,
		      "Update candidate flag changed");
	zassert_equal(storage[1], 0, "Update candidate presence not cleared");
	/* ... and orchestrator returns success */
	zassert_equal(0, err, "Unexpected error code");
}

ZTEST(orchestrator_tests, test_update_path_wrong_manifest_version)
{
	/* GIVEN suit storage contains update candidate with invalid manifest version... */
	setup_update_candidate(manifest_wrong_version_buf, manifest_wrong_version_len);

	/* ... and orchestrator is initialized */
	zassert_equal(0, suit_orchestrator_init(), "Orchestrator not initialized");

	/* WHEN orchestrator is launched */
	int err = suit_orchestrator_entry();

	/* THEN candidate availability flag is cleared... */
	uint32_t *storage = (uint32_t *)SUIT_STORAGE_ADDRESS;
	zassert_equal(storage[0], (uint32_t)UPDATE_MAGIC_VALUE_AVAILABLE,
		      "Update candidate flag changed");
	zassert_equal(storage[1], 0, "Update candidate presence not cleared");
	/* ... and orchestrator returns success */
	zassert_equal(0, err, "Unexpected error code");
}

ZTEST(orchestrator_tests, test_update_path_signed_with_different_key)
{
	/* GIVEN suit storage contains update candidate with invalid manifest version... */
	setup_update_candidate(manifest_different_key_buf, manifest_different_key_len);

	/* ... and orchestrator is initialized */
	zassert_equal(0, suit_orchestrator_init(), "Orchestrator not initialized");

	/* WHEN orchestrator is launched */
	int err = suit_orchestrator_entry();

	/* THEN candidate availability flag is cleared... */
	uint32_t *storage = (uint32_t *)SUIT_STORAGE_ADDRESS;
	zassert_equal(storage[0], (uint32_t)UPDATE_MAGIC_VALUE_AVAILABLE,
		      "Update candidate flag changed");
	zassert_equal(storage[1], 0, "Update candidate presence not cleared");
	/* ... and orchestrator returns success */
	zassert_equal(0, err, "Unexpected error code");
}
