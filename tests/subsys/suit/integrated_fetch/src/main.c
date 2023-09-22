/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <stdint.h>
#include <sink.h>
#include <memptr_sink.h>
#include <suit_memptr_storage.h>
#include <memptr_streamer.h>

#define TEST_DATA_SIZE 64

static uint8_t test_data[] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
			      16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
			      32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
			      48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63};

ZTEST_SUITE(memptr_streamer_tests, NULL, NULL, NULL, NULL, NULL);

ZTEST(memptr_streamer_tests, test_memptr_streamer_OK)
{
	struct stream_sink memptr_sink;
	memptr_storage_handle handle = NULL;

	int err = get_memptr_storage(&handle);
	zassert_equal(err, 0, "get_memptr_storage failed - error %i", err);

	err = get_memptr_sink(&memptr_sink, handle);
	zassert_equal(err, 0, "get_memptr_sink failed - error %i", err);

	err = memptr_streamer(test_data, TEST_DATA_SIZE, &memptr_sink);
	zassert_equal(err, 0, "memptr_streamer failed - error %i", err);

	err = release_memptr_storage(handle);
	zassert_equal(err, 0, "memptr_storage.release failed - error %i", err);
}

ZTEST(memptr_streamer_tests, test_memptr_streamer_NOK)
{
	struct stream_sink memptr_sink;
	memptr_storage_handle handle = NULL;

	int err = get_memptr_storage(&handle);
	zassert_equal(err, 0, "get_memptr_storage failed - error %i", err);

	err = get_memptr_sink(&memptr_sink, handle);
	zassert_equal(err, 0, "get_memptr_sink failed - error %i", err);

	err = memptr_streamer(NULL, TEST_DATA_SIZE, &memptr_sink);
	zassert_not_equal(err, 0, "memptr_streamer should have failed - payload == NULL");

	err = memptr_streamer(test_data, 0, &memptr_sink);
	zassert_not_equal(err, 0, "memptr_streamer should have failed - size == 0");

	err = memptr_streamer(test_data, TEST_DATA_SIZE, NULL);
	zassert_not_equal(err, 0, "memptr_streamer should have failed - sink == NULL");

	err = release_memptr_storage(handle);
	zassert_equal(err, 0, "memptr_storage.release failed - error %i", err);
}
