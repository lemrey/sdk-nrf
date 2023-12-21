/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <suit_plat_digest_cache.h>
#include <suit_platform.h>
#include <mocks.h>

static uint8_t component_ids_mem[][4] = {
	{0x1, 0x2, 0x3, 0x4},
	{0xA, 0xB, 0xC},
	{0x2, 0x4, 0x8, 0x10},
	{0x4, 0x3, 0x2, 0x1},
	{0x11, 0x22, 0x33, 0x44},
};

static size_t component_id_lengths[] = {4, 3, 4, 4, 4};

static uint8_t component_id_copies_mem[sizeof(component_ids_mem)/sizeof(component_ids_mem[0])][4];

static uint8_t digests_mem[][8] = {
	{0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88},
	{0xAA, 0xBB, 0xDD, 0xCC, 0xDD, 0xEE, 0xFF},
	{0x12, 0x34, 0x56, 0x78, 0x9A, 0xAB, 0xCD, 0xEF},
	{0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02},
	{0x11, 0x22, 0x11, 0x22, 0x11, 0x22, 0x11, 0x22}
};

static size_t digest_lengths[] = {8, 7, 8, 8, 8};

static uint8_t digest_copies_mem[sizeof(digests_mem)/sizeof(digests_mem[0])][8];

static struct zcbor_string m_component_id_for_fake;

static void test_before(void *data)
{
	/* Reset mocks */
	mocks_reset();
	suit_plat_digest_cache_remove_all();
	suit_plat_digest_cache_lock();

	/* Reset common FFF internal structures */
	FFF_RESET_HISTORY();
}

static void *suite_setup(void)
{
	memcpy(component_id_copies_mem, component_ids_mem, sizeof(component_id_copies_mem));
	memcpy(digest_copies_mem, digests_mem, sizeof(digest_copies_mem));

	return NULL;
}

ZTEST_SUITE(suit_plat_digest_cache_tests, NULL, suite_setup, test_before, NULL, NULL);

static int add_digest_for_component(int index)
{
	int ret = 0;
	struct zcbor_string component_id;
	struct zcbor_string digest;

	if (index < sizeof(component_id_lengths)/sizeof(size_t))
	{
		component_id.len = component_id_lengths[index];
		component_id.value = component_ids_mem[index];
		digest.len = digest_lengths[index];
		digest.value = digests_mem[index];

		ret = suit_plat_digest_cache_add(&component_id, suit_cose_sha256, &digest);
	}

	return ret;
}

static int remove_digest(int index)
{
	int ret;
	struct zcbor_string component_id;

	if (index < sizeof(component_id_lengths)/sizeof(size_t))
	{
		component_id.len = component_id_lengths[index];
		component_id.value = component_ids_mem[index];

		ret = suit_plat_digest_cache_remove(&component_id);
	}

	return ret;
}

static void fill_cache(void)
{
	int ret;

	ret = add_digest_for_component(0);
	zassert_equal(SUIT_SUCCESS, ret, "Adding element to cache failed");
	ret = add_digest_for_component(1);
	zassert_equal(SUIT_SUCCESS, ret, "Adding element to cache failed");
	ret = add_digest_for_component(2);
	zassert_equal(SUIT_SUCCESS, ret, "Adding element to cache failed");
}

static void verify_matching_component_in_cache(int index)
{
	int ret;
	/* The data of the component_id and digest is copied to make sure the cache compares
	   the contents of the strings, not their pointers */
	struct zcbor_string component_id_copy;
	struct zcbor_string digest_copy;

	if (index < sizeof(component_id_lengths)/sizeof(size_t))
	{
		component_id_copy.len = component_id_lengths[index];
		component_id_copy.value = component_id_copies_mem[index];
		digest_copy.len = digest_lengths[index];
		digest_copy.value = digest_copies_mem[index];


		ret = suit_plat_digest_cache_compare(&component_id_copy, suit_cose_sha256, &digest_copy);

		zassert_equal(SUIT_SUCCESS, ret, "Matching digest for a given component ID"
						 " was not found");
	}
}

static void verify_component_missing_from_cache(int index)
{
	int ret;

	struct zcbor_string component_id;

	if (index < sizeof(component_id_lengths)/sizeof(size_t))
	{
		component_id.len = component_id_lengths[index];
		component_id.value = component_ids_mem[index];

		/* Value of the digest does not matter, it won't be verified due to the missing
		   component ID */
		ret = suit_plat_digest_cache_compare(&component_id, suit_cose_sha256, NULL);

		zassert_equal(SUIT_ERR_MISSING_COMPONENT, ret,
			  "Attempt to retrieve missing does not report correctly");
	}
}


static void verify_cache_is_empty(void)
{
	int ret;
	struct zcbor_string component_id;

	for (size_t i = 0; i < sizeof(component_id_lengths)/sizeof(size_t); i++)
	{
		component_id.len = component_id_lengths[i];
		component_id.value = component_ids_mem[i];

		/* Value of the digest does not matter, it won't be verified due to the missing
		   component ID */
		ret = suit_plat_digest_cache_compare(&component_id, suit_cose_sha256, NULL);
		zassert_equal(SUIT_ERR_MISSING_COMPONENT, ret, "Cache is not empty");
	}
}

static int suit_plat_component_id_get_correct_fake_func(suit_component_t handle,
							struct zcbor_string **component_id)
{
	*component_id = &m_component_id_for_fake;

	return SUIT_SUCCESS;
}

ZTEST(suit_plat_digest_cache_tests, test_cache_unlock)
{
	bool was_enabled;
	suit_plat_digest_cache_unlock();

	was_enabled = suit_plat_digest_cache_is_unlocked();

	zassert_true(was_enabled, "Enabling the SUIT digest cache failed");
}

ZTEST(suit_plat_digest_cache_tests, test_cache_lock)
{
	bool was_enabled;

	suit_plat_digest_cache_unlock();
	suit_plat_digest_cache_lock();
	was_enabled = suit_plat_digest_cache_is_unlocked();

	zassert_false(was_enabled, "Disabling the SUIT digest cache failed");
}

ZTEST(suit_plat_digest_cache_tests, test_cache_reunlock)
{
	bool was_enabled;

	suit_plat_digest_cache_unlock();
	suit_plat_digest_cache_lock();
	suit_plat_digest_cache_unlock();

	was_enabled = suit_plat_digest_cache_is_unlocked();

	zassert_true(was_enabled, "Reenabling the SUIT digest cache failed");
}

ZTEST(suit_plat_digest_cache_tests, test_cache_add_single)
{
	int ret;

	suit_plat_digest_cache_unlock();

	ret = add_digest_for_component(0);
	zassert_equal(SUIT_SUCCESS, ret, "Adding element to cache failed");

	verify_matching_component_in_cache(0);
}

ZTEST(suit_plat_digest_cache_tests, test_cache_fill)
{
	suit_plat_digest_cache_unlock();
	fill_cache();

	verify_matching_component_in_cache(0);
	verify_matching_component_in_cache(1);
	verify_matching_component_in_cache(2);
}

ZTEST(suit_plat_digest_cache_tests, test_cache_add_when_full)
{
	int ret;

	suit_plat_digest_cache_unlock();
	fill_cache();

	ret = add_digest_for_component(3);
	zassert_equal(SUIT_ERR_OVERFLOW, ret,
			  "Adding element to full cache does not return overflow");

	verify_component_missing_from_cache(3);
}

ZTEST(suit_plat_digest_cache_tests, test_get_uncached_element)
{
	suit_plat_digest_cache_unlock();
	fill_cache();

	verify_component_missing_from_cache(3);
}

ZTEST(suit_plat_digest_cache_tests, test_add_null_digest)
{
	int ret;
	struct zcbor_string component_id;

	suit_plat_digest_cache_unlock();
	component_id.len = component_id_lengths[0];
	component_id.value = component_ids_mem[0];

	ret = suit_plat_digest_cache_add(&component_id, suit_cose_sha256, NULL);

	zassert_equal(SUIT_ERR_UNSUPPORTED_PARAMETER, ret,
		      "Passing NULL digest parameter to suit_plat_digest_cache_add"
		      " does not report correctly");
}

ZTEST(suit_plat_digest_cache_tests, test_add_null_component_id)
{
	int ret;
	struct zcbor_string digest;

	suit_plat_digest_cache_unlock();
	digest.len = digest_lengths[0];
	digest.value = digests_mem[0];

	ret = suit_plat_digest_cache_add(NULL, suit_cose_sha256, &digest);

	zassert_equal(SUIT_ERR_UNSUPPORTED_PARAMETER, ret,
		      "Passing NULL component_id parameter to suit_plat_digest_cache_add"
		      " does not report correctly");
}

ZTEST(suit_plat_digest_cache_tests, test_replace_existing_component)
{
	int ret;
	struct zcbor_string component_id;
	struct zcbor_string digest;
	struct zcbor_string digest_copy;

	suit_plat_digest_cache_unlock();
	fill_cache();

	component_id.len = component_id_lengths[1];
	component_id.value = component_ids_mem[1];
	digest.len = digest_lengths[4];
	digest.value = digests_mem[4];

	// Replace the component digest with a new digest
	ret = suit_plat_digest_cache_add(&component_id, suit_cose_sha256, &digest);
	zassert_equal(SUIT_SUCCESS, ret, "Replacing the component digest failed");

	digest_copy.len = digest_lengths[4];
	digest_copy.value = digest_copies_mem[4];
	ret = suit_plat_digest_cache_compare(&component_id, suit_cose_sha256, &digest_copy);

	zassert_equal(SUIT_SUCCESS, ret, "Verifying the replaced component ID failed");
}

ZTEST(suit_plat_digest_cache_tests, test_not_matching_same_length)
{
	int ret;
	struct zcbor_string mismatched_digest;
	struct zcbor_string component_id;

	component_id.len = component_id_lengths[0];
	component_id.value = component_ids_mem[0];

	suit_plat_digest_cache_unlock();
	ret = add_digest_for_component(0);
	zassert_equal(SUIT_SUCCESS, ret, "Adding element to cache failed");

	mismatched_digest.len = digest_lengths[3];
	mismatched_digest.value = digests_mem[3];

	ret = suit_plat_digest_cache_compare(&component_id, suit_cose_sha256, &mismatched_digest);
	zassert_equal(SUIT_FAIL_CONDITION, ret, "Comparing against a mismatched digest"
						" does not return the correct error code");
}

ZTEST(suit_plat_digest_cache_tests, test_not_matching_same_content_different_length)
{
	int ret;
	struct zcbor_string mismatched_digest;
	struct zcbor_string component_id;

	component_id.len = component_id_lengths[0];
	component_id.value = component_ids_mem[0];

	suit_plat_digest_cache_unlock();
	ret = add_digest_for_component(0);
	zassert_equal(SUIT_SUCCESS, ret, "Adding element to cache failed");

	mismatched_digest.len = digest_lengths[0] - 1;
	mismatched_digest.value = digests_mem[0];

	ret = suit_plat_digest_cache_compare(&component_id, suit_cose_sha256, &mismatched_digest);
	zassert_equal(SUIT_FAIL_CONDITION, ret, "Comparing against a mismatched digest"
						" does not return the correct error code");
}


ZTEST(suit_plat_digest_cache_tests, test_remove_single)
{
	int ret;

	suit_plat_digest_cache_unlock();
	fill_cache();

	ret = remove_digest(1);
	zassert_equal(SUIT_SUCCESS, ret, "Removing element from cache failed");

	verify_component_missing_from_cache(1);
}

ZTEST(suit_plat_digest_cache_tests, test_removing_from_full_cache_frees_slot)
{
	int ret;

	suit_plat_digest_cache_unlock();
	fill_cache();

	ret = remove_digest(1);
	zassert_equal(SUIT_SUCCESS, ret, "Removing element from cache failed");

	ret = add_digest_for_component(3);
	zassert_equal(SUIT_SUCCESS, ret, "Adding element to cache failed");

	verify_matching_component_in_cache(3);
}

ZTEST(suit_plat_digest_cache_tests, test_locked_cache_add)
{
	int ret;

	ret = add_digest_for_component(0);

	zassert_equal(SUIT_ERR_UNSUPPORTED_COMMAND, ret,
			  "Adding to locked cache returns incorrect status");

	verify_component_missing_from_cache(1);
}

ZTEST(suit_plat_digest_cache_tests, test_locked_cache_remove)
{
	int ret;

	suit_plat_digest_cache_unlock();
	fill_cache();
	suit_plat_digest_cache_lock();

	ret = remove_digest(1);

	zassert_equal(SUIT_ERR_UNSUPPORTED_COMMAND, ret,
			  "Removing from locked cache returns incorrect status");

	verify_matching_component_in_cache(1);
}

ZTEST(suit_plat_digest_cache_tests, test_remove_all)
{
	int ret;
	suit_plat_digest_cache_unlock();
	fill_cache();

	ret = suit_plat_digest_cache_remove_all();
	zassert_equal(SUIT_SUCCESS, ret, "Removing all elements from cache failed");

	verify_cache_is_empty();
}

ZTEST(suit_plat_digest_cache_tests, test_locked_cache_remove_all)
{
	int ret;

	suit_plat_digest_cache_unlock();
	fill_cache();
	suit_plat_digest_cache_lock();

	ret = suit_plat_digest_cache_remove_all();

	zassert_equal(SUIT_ERR_UNSUPPORTED_COMMAND, ret,
			  "Removing all elements from locked cache returns incorrect status");

	verify_matching_component_in_cache(0);
	verify_matching_component_in_cache(1);
	verify_matching_component_in_cache(2);
}

ZTEST(suit_plat_digest_cache_tests, test_remove_all_one_by_one)
{
	int ret;
	suit_plat_digest_cache_unlock();
	fill_cache();

	ret = remove_digest(0);
	zassert_equal(SUIT_SUCCESS, ret, "Removing element from cache failed");

	ret = remove_digest(1);
	zassert_equal(SUIT_SUCCESS, ret, "Removing element from cache failed");

	ret = remove_digest(2);
	zassert_equal(SUIT_SUCCESS, ret, "Removing element from cache failed");

	verify_cache_is_empty();
}

ZTEST(suit_plat_digest_cache_tests, test_suit_plat_digest_cache_remove_by_handle_invalid_handle)
{
	int ret;
	int dummy = 0;
	suit_component_t handle = (intptr_t) &dummy; // just ensure that it's a valid address

	suit_plat_digest_cache_unlock();
	fill_cache();

	suit_plat_component_id_get_fake.return_val = SUIT_ERR_MISSING_COMPONENT;

	ret = suit_plat_digest_cache_remove_by_handle(handle);

	zassert_equal(SUIT_ERR_MISSING_COMPONENT, ret,
			  "Removing based on unavailable handle does not report correctly");
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	zassert_equal(suit_plat_component_id_get_fake.arg0_val, handle,
			  "Incorrect handle passed to suit_plat_component_id_get()");
}

ZTEST(suit_plat_digest_cache_tests, test_suit_plat_digest_cache_remove_by_handle)
{
	int ret;
	int dummy = 0;
	suit_component_t handle = (intptr_t) &dummy; // just ensure that it's a valid address

	suit_plat_digest_cache_unlock();
	fill_cache();

	suit_plat_component_id_get_fake.custom_fake = suit_plat_component_id_get_correct_fake_func;

	m_component_id_for_fake.len = component_id_lengths[0];
	m_component_id_for_fake.value = component_ids_mem[0];

	ret = suit_plat_digest_cache_remove_by_handle(handle);

	zassert_equal(SUIT_SUCCESS, ret,
			  "Removing based on unavailable handle does not report correctly");
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	zassert_equal(suit_plat_component_id_get_fake.arg0_val, handle,
			  "Incorrect handle passed to suit_plat_component_id_get()");

	verify_component_missing_from_cache(0);
	verify_matching_component_in_cache(1); // Verify other components were not deleted
}

ZTEST(suit_plat_digest_cache_tests, test_suit_plat_digest_cache_remove_by_handle_2)
{
	int ret;
	int dummy = 0;
	suit_component_t handle = (intptr_t) &dummy; // just ensure that it's a valid address

	suit_plat_digest_cache_unlock();
	fill_cache();

	suit_plat_component_id_get_fake.custom_fake = suit_plat_component_id_get_correct_fake_func;

	m_component_id_for_fake.len = component_id_lengths[1];
	m_component_id_for_fake.value = component_ids_mem[1];

	ret = suit_plat_digest_cache_remove_by_handle(handle);

	zassert_equal(SUIT_SUCCESS, ret,
			  "Removing based on unavailable handle does not report correctly");
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	zassert_equal(suit_plat_component_id_get_fake.arg0_val, handle,
			  "Incorrect handle passed to suit_plat_component_id_get()");

	verify_component_missing_from_cache(1);
}
