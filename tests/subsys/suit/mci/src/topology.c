/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/ztest.h>
#include <suit_mci.h>

#define OUTPUT_MAX_SIZE 32
static const suit_uuid_t *result_uuid[OUTPUT_MAX_SIZE];

static int compare_manifest_class_id(const suit_manifest_class_id_t *manifest_class_id1,
				     const suit_manifest_class_id_t *manifest_class_id2)
{
	return memcmp(manifest_class_id1->raw, manifest_class_id2->raw,
		      sizeof(((suit_manifest_class_id_t *)0)->raw));
}

static void test_duplicate_ids_in_supported_manifest(void)
{
	int rc = 0;
	size_t output_size = OUTPUT_MAX_SIZE;

	rc = mci_get_supported_manifest_class_ids(result_uuid, &output_size);
	zassert_equal(rc, 0, "mci_get_supported_manifest_class_ids returned (%d)", rc);

	for (int i = 0; i < output_size; ++i) {
		for (int j = i + 1; j < output_size; ++j) {
			rc = compare_manifest_class_id(result_uuid[i], result_uuid[j]);
			zassert_true((rc != 0), "the same uuid used for two manifests");
		}
	}
}

static void test_duplicate_ids_in_invoke_order(void)
{
	int rc = 0;
	size_t output_size = OUTPUT_MAX_SIZE;

	rc = mci_get_invoke_order(result_uuid, &output_size);
	zassert_equal(rc, 0, "mci_get_invoke_order returned (%d)", rc);

	for (int i = 0; i < output_size; ++i) {
		for (int j = i + 1; j < output_size; ++j) {
			rc = compare_manifest_class_id(result_uuid[i], result_uuid[j]);
			zassert_true((rc != 0), "the same uuid used for two manifests");
		}
	}
}

void test_topology(void)
{
	ztest_test_suite(test_suit_mci_topology,
			 ztest_unit_test(test_duplicate_ids_in_supported_manifest),
			 ztest_unit_test(test_duplicate_ids_in_invoke_order));

	ztest_run_test_suite(test_suit_mci_topology);
}
