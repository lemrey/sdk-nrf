/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>

#include <suit_orchestrator.h>

#include <suit_storage.h>
#include <suit_mci.h>
#include <suit_types.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <suit_plat_mem_util.h>
#include <update_magic_values.h>
#include <dfu_cache.h>

#define SUIT_STORAGE_ADDRESS suit_plat_mem_nvm_ptr_get(SUIT_STORAGE_OFFSET)
#define SUIT_STORAGE_OFFSET  FIXED_PARTITION_OFFSET(suit_storage)
#define SUIT_STORAGE_SIZE    FIXED_PARTITION_SIZE(suit_storage)

const suit_manifest_class_id_t supported_class_id = {{0x5b, 0x46, 0x9f, 0xd1, 0x90, 0xee, 0x53,
						      0x9c, 0xa3, 0x18, 0x68, 0x1b, 0x03, 0x69,
						      0x5e, 0x36}};

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
	zassert_not_null(buf, "NULL buf");

	suit_plat_mreg_t update_candidate[1] = {{
		.mem = buf,
		.size = len,
	}};

	setup_erased_flash();

	int err = suit_mci_init();
	zassert_equal(SUIT_PLAT_SUCCESS, err, "Failed to initialize MCI (%d)", err);

	err = suit_storage_init();
	zassert_equal(SUIT_PLAT_SUCCESS, err, "Failed to init suit storage (%d)", err);

	err = suit_storage_update_cand_set(update_candidate, ARRAY_SIZE(update_candidate));
	zassert_equal(SUIT_PLAT_SUCCESS, err,
		      "Unable to set correct DFU before test execution (0x%x, %d)", buf, len);
}

static void setup_store_update_candidate_data(void *data, size_t len)
{
	setup_erased_flash();
	const struct device *fdev = DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));
	zassert_not_null(fdev, "Unable to find a flash driver");
	zassert_equal(0, flash_write(fdev, (off_t)SUIT_STORAGE_OFFSET, data, len),
		      "Unable to write data to flash");
}

ZTEST_SUITE(orchestrator_tests, NULL, NULL, NULL, NULL, NULL);

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
	zassert_equal(SUIT_PLAT_SUCCESS, err,
		      "The envelope was not installed after successful update");
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
	/* ... and the bootable envelope is installed */
	int err = suit_storage_installed_envelope_get(&supported_class_id, &addr_boot, &size_boot);
	zassert_equal(SUIT_PLAT_SUCCESS, err,
		      "The envelope was not installed after successful update");

	/* WHEN when orchestrator is initialized again (reboot simulation)... */
	zassert_equal(0, suit_orchestrator_init(), "Orchestrator not initialized second time");
	/* ... and orchestrator is launched again */
	err = suit_orchestrator_entry();

	/* THEN installed SW is invoked successfully (orchestrator returns no error) */
	zassert_equal(0, err, "Unexpected error code");
	/* ... and the envelope remains installed */
	err = suit_storage_installed_envelope_get(&supported_class_id, &addr_update, &size_update);
	zassert_equal(SUIT_PLAT_SUCCESS, err,
		      "The envelope was not installed after successful update");
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

	/* THEN candidate availability flag is cleared... */
	uint32_t *storage = (uint32_t *)SUIT_STORAGE_ADDRESS;
	zassert_equal(storage[0], (uint32_t)UPDATE_MAGIC_VALUE_AVAILABLE,
		      "Update candidate flag changed");
	zassert_equal(storage[1], 0, "Update candidate presence not cleared");
	/* ... and orchestrator returns success */
	zassert_equal(0, err, "Unexpected error code");
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
