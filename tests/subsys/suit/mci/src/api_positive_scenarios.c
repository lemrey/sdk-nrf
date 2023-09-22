/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/ztest.h>
#include <suit_mci.h>

#define OUTPUT_MAX_SIZE 32
static const suit_uuid_t *result_uuid[OUTPUT_MAX_SIZE];

static void test_mci_get_supported_manifest_class_ids(void)
{
	int rc = 0;
	size_t output_size = OUTPUT_MAX_SIZE;

	rc = mci_get_supported_manifest_class_ids(result_uuid, &output_size);
	zassert_equal(rc, 0, "mci_get_supported_manifest_class_ids returned (%d)", rc);
}

static void test_mci_get_invoke_order(void)
{
	int rc = 0;
	size_t output_size = OUTPUT_MAX_SIZE;

	rc = mci_get_invoke_order(result_uuid, &output_size);
	zassert_equal(rc, 0, "mci_get_invoke_order returned (%d)", rc);
}

static void test_mci_get_downgrade_prevention_policy(void)
{
	int rc = 0;
	size_t output_size = OUTPUT_MAX_SIZE;

	rc = mci_get_supported_manifest_class_ids(result_uuid, &output_size);
	zassert_equal(rc, 0, "mci_get_supported_manifest_class_ids returned (%d)", rc);

	for (int i = 0; i < output_size; ++i) {
		downgrade_prevention_policy_t policy;

		rc = mci_get_downgrade_prevention_policy(result_uuid[i], &policy);
		zassert_equal(rc, 0, "mci_get_downgrade_prevention_policy returned (%d)", rc);
	}
}

static void test_mci_validate_manifest_class_id(void)
{
	int rc = 0;
	size_t output_size = OUTPUT_MAX_SIZE;

	rc = mci_get_supported_manifest_class_ids(result_uuid, &output_size);
	zassert_equal(rc, 0, "mci_get_supported_manifest_class_ids returned (%d)", rc);

	for (int i = 0; i < output_size; ++i) {
		rc = mci_validate_manifest_class_id(result_uuid[i]);
		zassert_equal(rc, 0, "mci_validate_manifest_class_id returned (%d)", rc);
	}
}

static void test_mci_validate_signing_key_id(void)
{
	int rc = 0;
	size_t output_size = OUTPUT_MAX_SIZE;

	rc = mci_get_supported_manifest_class_ids(result_uuid, &output_size);
	zassert_equal(rc, 0, "mci_get_supported_manifest_class_ids returned (%d)", rc);

	for (int i = 0; i < output_size; ++i) {
		uint32_t key_id = 0;

		rc = mci_validate_signing_key_id(result_uuid[i], key_id);
		zassert_true((-MCI_ENOACCESS == rc || 0 == rc),
			     "mci_validate_signing_key_id returned (%d)", rc);
	}
}

static void test_mci_validate_processor_start_rights(void)
{
	int rc = 0;
	size_t output_size = OUTPUT_MAX_SIZE;

	rc = mci_get_supported_manifest_class_ids(result_uuid, &output_size);
	zassert_equal(rc, 0, "mci_get_supported_manifest_class_ids returned (%d)", rc);

	for (int i = 0; i < output_size; ++i) {
		int processor_id = 0;

		rc = mci_validate_processor_start_rights(result_uuid[i], processor_id);
		zassert_true((-MCI_ENOACCESS == rc || 0 == rc),
			     "mci_validate_processor_start_rights returned (%d)", rc);
	}
}

static void test_mci_validate_memory_access_rights(void)
{
	int rc = 0;
	size_t output_size = OUTPUT_MAX_SIZE;

	rc = mci_get_supported_manifest_class_ids(result_uuid, &output_size);
	zassert_equal(rc, 0, "mci_get_supported_manifest_class_ids returned (%d)", rc);

	for (int i = 0; i < output_size; ++i) {
		void *address = &address;
		size_t mem_size = sizeof(void *);

		rc = mci_validate_memory_access_rights(result_uuid[i], address, mem_size);
		zassert_true((-MCI_ENOACCESS == rc || 0 == rc),
			     "mci_validate_memory_access_rights returned (%d)", rc);
	}
}

static void test_mci_validate_platform_specific_component_rights(void)
{
	int rc = 0;
	size_t output_size = OUTPUT_MAX_SIZE;

	rc = mci_get_supported_manifest_class_ids(result_uuid, &output_size);
	zassert_equal(rc, 0, "mci_get_supported_manifest_class_ids returned (%d)", rc);

	for (int i = 0; i < output_size; ++i) {
		int platform_specific_component_number = 0;

		rc = mci_validate_platform_specific_component_rights(
			result_uuid[i], platform_specific_component_number);
		zassert_true((-MCI_ENOACCESS == rc || 0 == rc),
			     "mci_validate_platform_specific_component_rights returned (%d)", rc);
	}
}

static void test_mci_validate_manifest_parent_child(void)
{
	int rc = 0;
	size_t output_size = OUTPUT_MAX_SIZE;

	rc = mci_get_supported_manifest_class_ids(result_uuid, &output_size);
	zassert_equal(rc, 0, "mci_get_supported_manifest_class_ids returned (%d)", rc);

	for (int i = 0; i < output_size; ++i) {

		rc = mci_validate_manifest_parent_child(result_uuid[0], result_uuid[i]);
		zassert_true((-MCI_ENOACCESS == rc || 0 == rc),
			     "mci_validate_manifest_parent_child returned (%d)", rc);
	}
}

static void test_mci_get_vendor_id_for_manifest_class_id(void)
{
	int rc = 0;
	size_t output_size = OUTPUT_MAX_SIZE;

	rc = mci_get_supported_manifest_class_ids(result_uuid, &output_size);
	zassert_equal(rc, 0, "mci_get_supported_manifest_class_ids returned (%d)", rc);

	for (int i = 0; i < output_size; ++i) {
		const suit_uuid_t *vendor_id = NULL;

		rc = mci_get_vendor_id_for_manifest_class_id(result_uuid[i], &vendor_id);
		zassert_equal(rc, 0, "mci_get_vendor_id_for_manifest_class_id returned (%d)", rc);
	}
}

void test_api_positive_scenarios(void)
{
	ztest_test_suite(test_suit_mci_api,
			 ztest_unit_test(test_mci_get_supported_manifest_class_ids),
			 ztest_unit_test(test_mci_get_invoke_order),
			 ztest_unit_test(test_mci_get_downgrade_prevention_policy),
			 ztest_unit_test(test_mci_validate_signing_key_id),
			 ztest_unit_test(test_mci_validate_manifest_class_id),
			 ztest_unit_test(test_mci_validate_processor_start_rights),
			 ztest_unit_test(test_mci_validate_memory_access_rights),
			 ztest_unit_test(test_mci_validate_platform_specific_component_rights),
			 ztest_unit_test(test_mci_validate_manifest_parent_child),
			 ztest_unit_test(test_mci_get_vendor_id_for_manifest_class_id));

	ztest_run_test_suite(test_suit_mci_api);
}
