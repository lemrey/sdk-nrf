/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/ztest.h>
#include <suit_mci.h>

#define OUTPUT_MAX_SIZE 32
static const suit_uuid_t *result_uuid[OUTPUT_MAX_SIZE];

static void test_null_pointers(void)
{
	const suit_manifest_class_id_t unsupported_manifest_class_id = {
		{'u', 'n', 's', 'u', 'p', 'p', 'o', 'r', 't', 'e', 'd', '!', '!', '!', ' ', ' '}};
	size_t output_size = OUTPUT_MAX_SIZE;
	uint32_t key_id = 0;
	int processor_id = 0;
	void *mem_address = &mem_address;
	size_t mem_size = sizeof(mem_address);
	int platform_specific_component_number = 0;
	downgrade_prevention_policy_t policy;
	int rc = 0;

	rc = mci_get_nordic_vendor_id(NULL);
	zassert_equal(rc, -MCI_EINVAL, "mci_get_nordic_vendor_id returned (%d)", rc);

	rc = mci_get_nordic_unspecified_class_id(NULL);
	zassert_equal(rc, -MCI_EINVAL, "mci_get_nordic_unspecified_class_id returned (%d)", rc);

	output_size = OUTPUT_MAX_SIZE;
	rc = mci_get_supported_manifest_class_ids(NULL, &output_size);
	zassert_equal(rc, -MCI_EINVAL, "mci_get_supported_manifest_class_ids returned (%d)", rc);

	rc = mci_get_supported_manifest_class_ids(result_uuid, NULL);
	zassert_equal(rc, -MCI_EINVAL, "mci_get_supported_manifest_class_ids returned (%d)", rc);

	output_size = OUTPUT_MAX_SIZE;
	rc = mci_get_invoke_order(NULL, &output_size);
	zassert_equal(rc, -MCI_EINVAL, "mci_get_invoke_order returned (%d)", rc);

	rc = mci_get_invoke_order(result_uuid, NULL);
	zassert_equal(rc, -MCI_EINVAL, "mci_get_invoke_order returned (%d)", rc);

	rc = mci_get_downgrade_prevention_policy(NULL, &policy);
	zassert_equal(rc, -MCI_EINVAL, "mci_get_downgrade_prevention_policy returned (%d)", rc);

	rc = mci_get_downgrade_prevention_policy(&unsupported_manifest_class_id, NULL);
	zassert_equal(rc, -MCI_EINVAL, "mci_get_downgrade_prevention_policy returned (%d)", rc);

	rc = mci_validate_signing_key_id(NULL, key_id);
	zassert_equal(rc, -MCI_EINVAL, "mci_validate_signing_key_id returned (%d)", rc);

	rc = mci_validate_manifest_class_id(NULL);
	zassert_equal(rc, -MCI_EINVAL, "mci_validate_manifest_class_id returned (%d)", rc);

	rc = mci_validate_processor_start_rights(NULL, processor_id);
	zassert_equal(rc, -MCI_EINVAL, "mci_validate_processor_start_rights returned (%d)", rc);

	rc = mci_validate_memory_access_rights(NULL, mem_address, mem_size);
	zassert_equal(rc, -MCI_EINVAL, "mci_validate_memory_access_rights returned (%d)", rc);

	rc = mci_validate_memory_access_rights(&unsupported_manifest_class_id, NULL, mem_size);
	zassert_equal(rc, -MCI_EINVAL, "mci_validate_memory_access_rights returned (%d)", rc);

	rc = mci_validate_platform_specific_component_rights(NULL,
							     platform_specific_component_number);
	zassert_equal(rc, -MCI_EINVAL,
		      "mci_validate_platform_specific_component_rights returned (%d)", rc);

	rc = mci_validate_manifest_parent_child(&unsupported_manifest_class_id, NULL);
	zassert_equal(rc, -MCI_EINVAL, "mci_validate_manifest_parent_child returned (%d)", rc);

	rc = mci_validate_manifest_parent_child(NULL, &unsupported_manifest_class_id);
	zassert_equal(rc, -MCI_EINVAL, "mci_validate_manifest_parent_child returned (%d)", rc);

	rc = mci_get_vendor_id_for_manifest_class_id(NULL, result_uuid);
	zassert_equal(rc, -MCI_EINVAL, "mci_get_vendor_id_for_manifest_class_id returned (%d)", rc);

	rc = mci_get_vendor_id_for_manifest_class_id(&unsupported_manifest_class_id, NULL);
	zassert_equal(rc, -MCI_EINVAL, "mci_get_vendor_id_for_manifest_class_id returned (%d)", rc);
}

static void test_invalid_params(void)
{
	void *mem_address = &mem_address;
	size_t mem_size = sizeof(mem_address);
	int rc = 0;

	rc = mci_validate_memory_access_rights(NULL, mem_address, mem_size);
	zassert_equal(rc, -MCI_EINVAL, "mci_validate_memory_access_rights returned (%d)", rc);
}

static void test_output_buffer_too_small(void)
{
	size_t output_size = OUTPUT_MAX_SIZE;
	size_t supported_manifest_count = 0;
	size_t invokable_manifest_count = 0;
	int rc = 0;

	output_size = OUTPUT_MAX_SIZE;
	rc = mci_get_supported_manifest_class_ids(result_uuid, &output_size);
	zassert_equal(
		rc, 0,
		"mci_get_supported_manifest_class_ids returned (%d), isn't %d manifests enaugh?!",
		rc, OUTPUT_MAX_SIZE);
	supported_manifest_count = output_size;

	output_size = supported_manifest_count;
	rc = mci_get_supported_manifest_class_ids(result_uuid, &output_size);
	zassert_equal(rc, 0, "mci_get_supported_manifest_class_ids returned (%d)", rc);

	if (supported_manifest_count > 1) {
		output_size = supported_manifest_count - 1;
		rc = mci_get_supported_manifest_class_ids(result_uuid, &output_size);
		zassert_equal(rc, -MCI_ESIZE, "mci_get_supported_manifest_class_ids returned (%d)",
			      rc);
	}

	if (supported_manifest_count > 0) {
		output_size = 0;
		rc = mci_get_supported_manifest_class_ids(result_uuid, &output_size);
		zassert_equal(rc, -MCI_ESIZE, "mci_get_supported_manifest_class_ids returned (%d)",
			      rc);
	}

	output_size = OUTPUT_MAX_SIZE;
	rc = mci_get_invoke_order(result_uuid, &output_size);
	zassert_equal(rc, 0, "mci_get_invoke_order returned (%d), isn't %d manifests enaugh?!", rc,
		      OUTPUT_MAX_SIZE);
	invokable_manifest_count = output_size;

	output_size = invokable_manifest_count;
	rc = mci_get_invoke_order(result_uuid, &output_size);
	zassert_equal(rc, 0, "mci_get_invoke_order returned (%d)", rc);

	if (invokable_manifest_count > 1) {
		output_size = invokable_manifest_count - 1;
		rc = mci_get_invoke_order(result_uuid, &output_size);
		zassert_equal(rc, -MCI_ESIZE, "mci_get_invoke_order returned (%d)", rc);
	}

	if (invokable_manifest_count > 0) {
		output_size = 0;
		rc = mci_get_invoke_order(result_uuid, &output_size);
		zassert_equal(rc, -MCI_ESIZE, "mci_get_invoke_order returned (%d)", rc);
	}
}

void test_sanity(void)
{
	ztest_test_suite(test_suit_mci_sanity, ztest_unit_test(test_null_pointers),
			 ztest_unit_test(test_invalid_params),
			 ztest_unit_test(test_output_buffer_too_small));

	ztest_run_test_suite(test_suit_mci_sanity);
}
