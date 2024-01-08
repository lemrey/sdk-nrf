/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/ztest.h>
#include <suit_mci.h>

static void test_mci_nordic_vendor_id_value(void)
{

	suit_uuid_t expected_vid = {{0x76, 0x17, 0xda, 0xa5, 0x71, 0xfd, 0x5a, 0x85, 0x8f, 0x94,
				     0xe2, 0x8d, 0x73, 0x5c, 0xe9, 0xf4}};

	const suit_uuid_t *obtained_vid = NULL;
	mci_err_t rc = SUIT_PLAT_SUCCESS;

	rc = suit_mci_nordic_vendor_id_get(&obtained_vid);
	zassert_equal(rc, SUIT_PLAT_SUCCESS, "suit_mci_nordic_vendor_id_get returned (%d)", rc);
	zassert_not_null(obtained_vid, "obtained_vid points to NULL");
	zassert_mem_equal(obtained_vid->raw, expected_vid.raw, sizeof(((suit_uuid_t *)0)->raw),
			  "unexpected vendor_id");
}

static void test_mci_nordic_unspecified_class_id_value(void)
{

	suit_uuid_t expected_cid = {{0xca, 0xd8, 0x52, 0x3a, 0xf8, 0x29, 0x5a, 0x9a, 0xba, 0x85,
				     0x2e, 0xa0, 0xb2, 0xf5, 0x77, 0xc9}};

	const suit_uuid_t *obtained_cid = NULL;
	mci_err_t rc = SUIT_PLAT_SUCCESS;

	rc = suit_mci_nordic_unspecified_class_id_get(&obtained_cid);
	zassert_equal(rc, SUIT_PLAT_SUCCESS,
		      "suit_mci_nordic_unspecified_class_id_get returned (%d)", rc);
	zassert_not_null(obtained_cid, "obtained_cid points to NULL");
	zassert_mem_equal(obtained_cid->raw, expected_cid.raw, sizeof(((suit_uuid_t *)0)->raw), "");
}

void test_generic_ids(void)
{
	ztest_test_suite(test_suit_mci_generic_ids,
			 ztest_unit_test(test_mci_nordic_vendor_id_value),
			 ztest_unit_test(test_mci_nordic_unspecified_class_id_value));

	ztest_run_test_suite(test_suit_mci_generic_ids);
}
