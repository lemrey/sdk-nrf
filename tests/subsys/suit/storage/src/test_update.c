/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <suit_storage.h>
#include <suit_plat_mem_util.h>
#include <update_magic_values.h>

#define SUIT_STORAGE_ADDRESS suit_plat_mem_nvm_ptr_get(SUIT_STORAGE_OFFSET)
#define SUIT_STORAGE_OFFSET  FIXED_PARTITION_OFFSET(suit_storage)
#define SUIT_STORAGE_SIZE    FIXED_PARTITION_SIZE(suit_storage)

#define zassert_area_empty()                                                                       \
	zassert_equal(*((uint32_t *)SUIT_STORAGE_ADDRESS), UPDATE_MAGIC_VALUE_EMPTY,               \
		      "Test precondition not met: area is not empty (0x%x).",                      \
		      *((uint32_t *)SUIT_STORAGE_ADDRESS));

#define zassert_area_update_available()                                                            \
	zassert_equal(*((uint32_t *)SUIT_STORAGE_ADDRESS), UPDATE_MAGIC_VALUE_AVAILABLE,           \
		      "Test precondition not met: update is not available (0x%x).",                \
		      *((uint32_t *)SUIT_STORAGE_ADDRESS));

static void test_suite_before(void *f)
{
	uint32_t *storage = (uint32_t *)SUIT_STORAGE_ADDRESS;

	if (*storage == UPDATE_MAGIC_VALUE_EMPTY) {
		return;
	}

	/* Erase the area, to met the preconditions in the next test. */
	const struct device *fdev = DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));
	zassert_not_null(fdev, "Unable to find a driver to erase area");

	int rc = flash_erase(fdev, SUIT_STORAGE_OFFSET, SUIT_STORAGE_SIZE);
	zassert_equal(rc, 0, "Unable to erase memory before test execution");

	rc = suit_storage_init();
	zassert_equal(rc, SUIT_PLAT_SUCCESS, "Failed to initialize SUIT storage module (%d).", rc);
}

ZTEST_SUITE(suit_storage_update_tests, NULL, NULL, test_suite_before, NULL, NULL);

ZTEST(suit_storage_update_tests, test_empty_update_available)
{
	const suit_plat_mreg_t *update_regions = NULL;
	size_t update_regions_len = 0;

	int rc = suit_storage_update_cand_get(&update_regions, &update_regions_len);

	zassert_area_empty();

	zassert_not_equal(rc, SUIT_PLAT_SUCCESS,
			  "Storage is empty, but update availability is reported (0x%x, %d).",
			  update_regions, update_regions_len);
}

ZTEST(suit_storage_update_tests, test_empty_update_get)
{
	const suit_plat_mreg_t *update_regions = NULL;
	size_t update_regions_len = 0;
	int rc = SUIT_PLAT_SUCCESS;

	zassert_area_empty();

	rc = suit_storage_update_cand_get(NULL, NULL);
	zassert_not_equal(rc, SUIT_PLAT_SUCCESS,
			  "Storage is empty, but update candidate was received");

	rc = suit_storage_update_cand_get(NULL, &update_regions_len);
	zassert_not_equal(rc, SUIT_PLAT_SUCCESS,
			  "Storage is empty, but update candidate was received (%d)",
			  update_regions_len);

	rc = suit_storage_update_cand_get(&update_regions, NULL);
	zassert_not_equal(rc, SUIT_PLAT_SUCCESS,
			  "Storage is empty, but update candidate was received (0x%x)",
			  update_regions);

	rc = suit_storage_update_cand_get(&update_regions, &update_regions_len);
	zassert_not_equal(rc, SUIT_PLAT_SUCCESS,
			  "Storage is empty, but update candidate was received (0x%x, %d)",
			  update_regions, update_regions_len);
}

ZTEST(suit_storage_update_tests, test_empty_update_clear)
{
	zassert_area_empty();

	int rc = suit_storage_update_cand_set(NULL, 0);
	zassert_equal(rc, SUIT_PLAT_SUCCESS, "Unable to clear empty partition (%d)", rc);
}

void verify_storage_updates(void)
{
	void *addresses[] = {NULL, (void *)0xCAFEFECA};
	size_t sizes[] = {0, SUIT_STORAGE_SIZE + 1, SUIT_STORAGE_SIZE, 0x2048, 2044};
	int rc = SUIT_PLAT_SUCCESS;
	suit_plat_mreg_t update_candidate[1];

	for (int addr_i = 0; addr_i < ARRAY_SIZE(addresses); addr_i++) {
		for (int size_i = 0; size_i < ARRAY_SIZE(sizes); size_i++) {
			update_candidate[0].mem = addresses[addr_i];
			update_candidate[0].size = sizes[size_i];

			if ((update_candidate[0].mem != NULL) && (update_candidate[0].size > 0)) {
				rc = suit_storage_update_cand_set(update_candidate,
								  ARRAY_SIZE(update_candidate));
				zassert_equal(rc, SUIT_PLAT_SUCCESS,
					      "Setting correct DFU partition failed (0x%x, %d).",
					      update_candidate[0].mem, update_candidate[0].size);

				/* Verify set values */
				const suit_plat_mreg_t *update_regions = NULL;
				size_t update_regions_len = 0;

				rc = suit_storage_update_cand_get(&update_regions,
								  &update_regions_len);
				zassert_equal(
					rc, SUIT_PLAT_SUCCESS,
					"Set succeeded, but update availability is not reported.");
				zassert_equal(update_regions_len, 1,
					      "Set succeeded, but length of update candidate "
					      "regions is invalid (%d).",
					      update_regions_len);
				zassert_equal(update_regions[0].mem, addresses[addr_i],
					      "Update set with wrong address (%p != %p)",
					      update_regions[0].mem, addresses[addr_i]);
				zassert_equal(update_regions[0].size, sizes[size_i],
					      "Update set with wrong size (0x%x != 0x%x)",
					      update_regions[0].size, sizes[size_i]);
			} else {
				rc = suit_storage_update_cand_set(update_candidate,
								  ARRAY_SIZE(update_candidate));
				zassert_not_equal(
					rc, SUIT_PLAT_SUCCESS,
					"Setting incorrect DFU partition should fail (0x%x, %d).",
					addresses[addr_i], sizes[size_i]);

				const suit_plat_mreg_t *update_regions = NULL;
				size_t update_regions_len = 0;

				rc = suit_storage_update_cand_get(&update_regions,
								  &update_regions_len);
				zassert_not_equal(rc, SUIT_PLAT_SUCCESS,
						  "Set not succeeded, but update availability is "
						  "reported (0x%x, %d).",
						  update_regions, update_regions_len);
			}
		}
	}
}

ZTEST(suit_storage_update_tests, test_empty_update_set)
{
	zassert_area_empty();

	verify_storage_updates();
}

ZTEST(suit_storage_update_tests, test_cleared_update_set)
{
	zassert_area_empty();

	int rc = suit_storage_update_cand_set(NULL, 0);
	zassert_equal(rc, SUIT_PLAT_SUCCESS, "Unable to clear empty partition (%d)", rc);

	verify_storage_updates();
}

ZTEST(suit_storage_update_tests, test_cleared_update_clear)
{
	zassert_area_empty();

	int rc = suit_storage_update_cand_set(NULL, 0);
	zassert_equal(rc, SUIT_PLAT_SUCCESS, "Unable to clear empty partition (%d)", rc);

	rc = suit_storage_update_cand_set(NULL, 0);
	zassert_equal(rc, SUIT_PLAT_SUCCESS, "Unable to clear cleared partition (%d)", rc);
}

ZTEST(suit_storage_update_tests, test_valid_update_clear)
{
	zassert_area_empty();

	suit_plat_mreg_t update_candidate[1] = {{
		.mem = (uint8_t *)0xCAFEFECA,
		.size = 2044,
	}};
	int rc = suit_storage_update_cand_set(update_candidate, ARRAY_SIZE(update_candidate));
	zassert_equal(rc, SUIT_PLAT_SUCCESS,
		      "Unable to set correct DFU before test execution (0x%x, %d).", 0xCAFEFECA,
		      2044);

	zassert_area_update_available();

	rc = suit_storage_update_cand_set(NULL, 0);
	zassert_equal(rc, SUIT_PLAT_SUCCESS, "Unable to clear empty partition (%d)", rc);
}

ZTEST(suit_storage_update_tests, test_update_with_caches)
{
	zassert_area_empty();

	suit_plat_mreg_t update_candidate[3] = {
		{
			.mem = (uint8_t *)0xCAFEFECA,
			.size = 2044,
		},
		{
			.mem = (uint8_t *)0x000CA52E,
			.size = 123,
		},
		{
			.mem = (uint8_t *)0xB16CA52E,
			.size = 65543,
		},
	};
	int rc = suit_storage_update_cand_set(update_candidate, ARRAY_SIZE(update_candidate));
	zassert_equal(rc, SUIT_PLAT_SUCCESS, "Unable to set complex update candidate info");

	const suit_plat_mreg_t *update_regions = NULL;
	size_t update_regions_len = 0;
	rc = suit_storage_update_cand_get(&update_regions, &update_regions_len);
	zassert_equal(rc, SUIT_PLAT_SUCCESS,
		      "Set succeeded, but update availability is not reported.");
	zassert_equal(update_regions_len, 3,
		      "Set succeeded, but length of update candidate regions is invalid (%d).",
		      update_regions_len);
	for (size_t i = 0; i < ARRAY_SIZE(update_candidate); i++) {
		zassert_equal(update_regions[i].mem, update_candidate[i].mem,
			      "Update region %d set with wrong address (%p != %p)", i,
			      update_regions[i].mem, update_candidate[i].mem);
		zassert_equal(update_regions[i].size, update_candidate[i].size,
			      "Update region %d set with wrong size (0x%x != 0x%x)", i,
			      update_regions[i].size, update_candidate[i].size);
	}
}

ZTEST(suit_storage_update_tests, test_update_with_too_many_caches)
{
	zassert_area_empty();

	suit_plat_mreg_t update_candidate[CONFIG_SUIT_STORAGE_N_UPDATE_REGIONS + 1];
	for (size_t i = 0; i < ARRAY_SIZE(update_candidate); i++) {
		update_candidate[i].mem = (uint8_t *)0xCAFEFECA;
		update_candidate[i].size = 2044;
	}
	int rc = suit_storage_update_cand_set(update_candidate, ARRAY_SIZE(update_candidate));
	zassert_not_equal(rc, SUIT_PLAT_SUCCESS,
			  "Too long complex update candidate info set shall fail");

	const suit_plat_mreg_t *update_regions = NULL;
	size_t update_regions_len = 0;
	rc = suit_storage_update_cand_get(&update_regions, &update_regions_len);
	zassert_not_equal(rc, SUIT_PLAT_SUCCESS,
			  "Set not succeeded, but update availability is reported (0x%x, %d).",
			  update_regions, update_regions_len);
}
