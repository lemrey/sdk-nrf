/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <stdint.h>
#include <suit_platform.h>
#include <suit_platform_internal.h>

#define ASSIGNED_COMPONENT_HANDLE 0x1E054000

static const uint8_t dummy_data[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};

static void test_impl_selector(void)
{
	suit_component_t component_handle = ASSIGNED_COMPONENT_HANDLE;

	/* [h'MEM', h'02', h'1A00080000', h'1A00000008'] */
	uint8_t valid_value[] = {0x84, 0x44, 0x63, 'M',	 'E',  'M',  0x41, 0x02, 0x45, 0x1A,
				 0x00, 0x08, 0x00, 0x00, 0x45, 0x1A, 0x00, 0x00, 0x00, 0x08};
	struct zcbor_string valid_component_id = {
		.value = valid_value,
		.len = sizeof(valid_value),
	};
	struct suit_component_impl *impl =
		suit_component_select_impl(&valid_component_id, component_handle);

	zassert_not_equal(impl, NULL, "Suit component implementation is NULL");
	zassert_not_equal(impl->init, NULL, "Init implementation is NULL");

	/* [h'SOC_SPEC', h'01'] */
	uint8_t valid_value_invalid_comp_type[] = {0x82, 0x49, 0x68, 'S', 'O',	'C', '_',
						   'S',	 'P',  'E',  'C', 0x41, 0x01};

	struct zcbor_string valid_component_id_invalid_comp_type = {
		.value = valid_value_invalid_comp_type,
		.len = sizeof(valid_value_invalid_comp_type),
	};

	impl = suit_component_select_impl(&valid_component_id_invalid_comp_type, component_handle);
	zassert_equal(
		impl, NULL,
		"Suit component implementation should be NULL for unsupported component type");

	/* [h'MEM', h'02', h'1A00080000', h'1A00000008'] */
	/* Modified to be malformed: the second byte containing type substring length modified: 0x44
	 * => 0x46
	 */
	uint8_t invalid_value[] = {0x84, 0x46, 0x63, 'M',  'E',	 'M',  0x41, 0x02, 0x45, 0x1A,
				   0x00, 0x08, 0x00, 0x00, 0x45, 0x1A, 0x00, 0x00, 0x00, 0x08};
	struct zcbor_string invalid_component_id = {
		.value = invalid_value,
		.len = sizeof(invalid_value),
	};

	impl = suit_component_select_impl(&invalid_component_id, component_handle);
	zassert_equal(impl, NULL,
		      "Suit component implementation should be NULL for malformed component id");
}

static void test_plat_state(void)
{
	int err = 0;

	err = suit_plat_report(0, NULL);
	zassert_equal(err, SUIT_ERR_UNSUPPORTED_COMMAND, "Empty function that should return error");
}

static void test_envelope_ref(void)
{
	/* [h'CAND_IMG', h'02'] */
	uint8_t valid_value[] = {0x82, 0x49, 0x68, 'C', 'A',  'N', 'D',
				 '_',  'I',  'M',  'G', 0x41, 0x02};
	struct zcbor_string valid_component_id = {
		.value = valid_value,
		.len = sizeof(valid_value),
	};

	struct zcbor_string dummy_payload = {.value = dummy_data, .len = sizeof(dummy_data)};

	suit_component_t component_handle;

	int err = suit_plat_create_component_handle(&valid_component_id, &component_handle);
	zassert_equal(err, SUIT_SUCCESS, "Init failed - error %i", err);

	struct suit_plat_component *component = (struct suit_plat_component *)component_handle;
	err = component->impl->fetch_integrated(component_handle, &dummy_payload);
	zassert_equal(err, SUIT_SUCCESS, "Fetch integrated failed - error %i", err);

	uint8_t *dataPtr;
	size_t read_size = component->impl->read_address(component_handle, &dataPtr);
	zassert_equal(dataPtr, dummy_data, "Data address mismatch");

	uint8_t byte;
	size_t len = 1;
	err = component->impl->read(component_handle, 5, &byte, &len);
	zassert_equal(byte, dummy_data[5], "Data after read doesn't match");

	err = component->impl->read(component_handle, dummy_payload.len + read_size, &byte, &len);
	zassert_not_equal(err, SUIT_SUCCESS, "Read should have failed => offset > len");

	err = component->impl->read(0, dummy_payload.len + 1, &byte, &len);
	zassert_not_equal(err, SUIT_SUCCESS, "Read should have failed => invalid handle");

	component->impl->release(component_handle);

	err = suit_plat_release_component_handle(component_handle);
	zassert_equal(err, 0, "component_handle release failed - error %i", err);
}

static void test_mram(void)
{
	/* [h'MEM', h'02', h'1A00080000', h'1A00000008'] */
	uint8_t valid_value[] = {0x84, 0x44, 0x63, 'M',	 'E',  'M',  0x41, 0x02, 0x45, 0x1A,
				 0x00, 0x08, 0x00, 0x00, 0x45, 0x1A, 0x00, 0x00, 0x00, 0x08};
	struct zcbor_string valid_component_id = {
		.value = valid_value,
		.len = sizeof(valid_value),
	};

	struct zcbor_string dummy_payload = {.value = dummy_data, .len = sizeof(dummy_data)};

	suit_component_t component_handle;

	int err = suit_plat_create_component_handle(&valid_component_id, &component_handle);
	zassert_equal(err, SUIT_SUCCESS, "Init failed - error %i", err);

	struct suit_plat_component *component = (struct suit_plat_component *)component_handle;
	err = component->impl->fetch_integrated(component_handle, &dummy_payload);
	zassert_equal(err, SUIT_SUCCESS, "Fetch integrated failed - error %i", err);

	err = component->impl->write(0, 0, (uint8_t *)dummy_payload.value, dummy_payload.len);
	zassert_not_equal(err, SUIT_SUCCESS, "Write should have failed => invalid handle");

	err = component->impl->write(component_handle, 0, (uint8_t *)dummy_payload.value,
				     dummy_payload.len + 1);
	zassert_not_equal(err, SUIT_SUCCESS, "Write should have failed => len > size");

	uint8_t *dataPtr;
	size_t read_size = component->impl->read_address(component_handle, &dataPtr);
	zassert_not_equal(read_size, 0, "read_address failed");

	uint8_t bytes[8];
	size_t len = sizeof(bytes);
	err = component->impl->read(component_handle, 5, bytes, &len);
	zassert_equal(err, SUIT_SUCCESS, "Read failed - error %i", err);

	err = component->impl->read(component_handle, read_size + 1, bytes, &len);
	zassert_not_equal(err, SUIT_SUCCESS, "Read should have failed => offset > len");

	err = component->impl->read(0, 5, bytes, &len);
	zassert_not_equal(err, SUIT_SUCCESS, "Read should have failed => invalid handle");

	/* Dry write */
	err = component->impl->write(component_handle, 0, NULL, dummy_payload.len);
	zassert_equal(err, SUIT_SUCCESS, "Dry write failed");

	component->impl->release(component_handle);

	err = suit_plat_release_component_handle(component_handle);
	zassert_equal(err, 0, "component_handle release failed - error %i", err);
}

void test_main(void)
{
	ztest_test_suite(test_suit, ztest_unit_test(test_impl_selector),
			 ztest_unit_test(test_plat_state), ztest_unit_test(test_mram),
			 ztest_unit_test(test_envelope_ref));

	ztest_run_test_suite(test_suit);
}
