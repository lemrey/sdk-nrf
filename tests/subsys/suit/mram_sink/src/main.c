/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <stdint.h>
#include <mram_sink.h>
#include <sink.h>
#include <platform_mem_util.h>
#include <zephyr/drivers/flash.h>

#define TEST_DATA_SIZE 64
#define WRITE_ADDR     0x80000

#define SUIT_STORAGE_ADDRESS FLASH_ADDRESS(SUIT_STORAGE_OFFSET)
#define SUIT_STORAGE_OFFSET  FIXED_PARTITION_OFFSET(suit_storage)
#define SUIT_STORAGE_SIZE    FIXED_PARTITION_SIZE(suit_storage)

static uint8_t test_data[] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
			      16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
			      32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
			      48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63};

#ifdef CONFIG_FLASH_SIMULATOR
static uint8_t *f_base_address = NULL;
#endif /* CONFIG_FLASH_SIMULATOR */

static void test_setup_flash(void *arg)
{
	/* Erase the area, to met the preconditions in the next test. */
	const struct device *fdev = DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));
	zassert_not_null(fdev, "Unable to find a driver to erase area");

#ifdef CONFIG_FLASH_SIMULATOR
	if (f_base_address == NULL) {
		size_t f_size;

		f_base_address = flash_simulator_get_memory(fdev, &f_size);
	}
#endif /* CONFIG_FLASH_SIMULATOR */

	int rc = flash_erase(fdev, SUIT_STORAGE_OFFSET, SUIT_STORAGE_SIZE);
	zassert_equal(rc, 0, "Unable to erase memory before test execution");
}

ZTEST_SUITE(mram_sink_tests, NULL, NULL, test_setup_flash, NULL, NULL);

ZTEST(mram_sink_tests, test_get_mram_sink_OK)
{
	struct stream_sink mram_sink;

	int err = get_mram_sink(&mram_sink, (uint8_t *)WRITE_ADDR, TEST_DATA_SIZE);

	zassert_equal(err, 0, "get_mram_sink failed - error %i", err);
	zassert_not_equal(mram_sink.ctx, NULL, "get_mram_sink failed - ctx is NULL");

	err = mram_sink.release(mram_sink.ctx);
	zassert_equal(err, 0, "mram_sink.release failed - error %i", err);
}

ZTEST(mram_sink_tests, test_get_mram_sink_NOK)
{
	struct stream_sink mram_sink;

	int err = get_mram_sink(&mram_sink, NULL, TEST_DATA_SIZE);
	zassert_not_equal(err, 0, "get_mram_sink should have failed - dst == NULL");

	err = get_mram_sink(&mram_sink, (uint8_t *)WRITE_ADDR, 0);
	zassert_not_equal(err, 0, "get_mram_sink should have failed - offset_limit == 0");
}

ZTEST(mram_sink_tests, test_mram_sink_release_NOK)
{
	struct stream_sink mram_sink;

	int err = get_mram_sink(&mram_sink, (uint8_t *)WRITE_ADDR, TEST_DATA_SIZE);
	zassert_equal(err, 0, "get_mram_sink failed - error %i", err);

	err = mram_sink.release(NULL);
	zassert_not_equal(err, 0, "mram_sink.release should have failed - ctx == NULL");

	err = mram_sink.release(mram_sink.ctx);
	zassert_equal(err, 0, "mram_sink.release failed - error %i", err);
}

ZTEST(mram_sink_tests, test_mram_sink_seek_OK)
{
	struct stream_sink mram_sink;

	int err = get_mram_sink(&mram_sink, (uint8_t *)WRITE_ADDR, TEST_DATA_SIZE);
	zassert_equal(err, 0, "get_mram_sink failed - error %i", err);

	err = mram_sink.seek(mram_sink.ctx, 0);
	zassert_equal(err, 0, "mram_sink.seek failed - error %i", err);

	err = mram_sink.seek(mram_sink.ctx, 9);
	zassert_equal(err, 0, "mram_sink.seek failed - error %i", err);

	err = mram_sink.seek(mram_sink.ctx, 63);
	zassert_equal(err, 0, "mram_sink.seek failed - error %i", err);

	err = mram_sink.release(mram_sink.ctx);
	zassert_equal(err, 0, "mram_sink.release failed - error %i", err);
}

ZTEST(mram_sink_tests, test_mram_sink_seek_NOK)
{
	struct stream_sink mram_sink;

	int err = get_mram_sink(&mram_sink, (uint8_t *)WRITE_ADDR, TEST_DATA_SIZE);
	zassert_equal(err, 0, "get_mram_sink failed - error %i", err);

	err = mram_sink.seek(mram_sink.ctx, 64);
	zassert_not_equal(err, 0, "mram_sink.seek should have failed - passed arg == offset_limit");

	err = mram_sink.seek(mram_sink.ctx, 65);
	zassert_not_equal(err, 0, "mram_sink.seek should have failed - passed arg > offset_limit");

	err = mram_sink.release(mram_sink.ctx);
	zassert_equal(err, 0, "mram_sink.release failed - error %i", err);
}

ZTEST(mram_sink_tests, test_mram_sink_used_storage_OK)
{
	struct stream_sink mram_sink;
	size_t used_storage = 0;

	int err = get_mram_sink(&mram_sink, (uint8_t *)WRITE_ADDR, TEST_DATA_SIZE);
	zassert_equal(err, 0, "get_mram_sink failed - error %i", err);

	err = mram_sink.used_storage(mram_sink.ctx, &used_storage);
	zassert_equal(err, 0, "mram_sink.use_storage failed - error %i", err);
	zassert_equal(used_storage, 0, "mram_sink.use_storage failed - not initialized to 0");

	err = mram_sink.release(mram_sink.ctx);
	zassert_equal(err, 0, "mram_sink.release failed - error %i", err);
}

ZTEST(mram_sink_tests, test_mram_sink_used_storage_NOK)
{
	struct stream_sink mram_sink;

	int err = get_mram_sink(&mram_sink, (uint8_t *)WRITE_ADDR, TEST_DATA_SIZE);
	zassert_equal(err, 0, "get_mram_sink failed - error %i", err);

	err = mram_sink.used_storage(mram_sink.ctx, NULL);
	zassert_not_equal(err, 0, "mram_sink.use_storage should have failed - arg size == NULL");

	err = mram_sink.release(mram_sink.ctx);
	zassert_equal(err, 0, "mram_sink.release failed - error %i", err);
}

ZTEST(mram_sink_tests, test_mram_sink_write_OK)
{
	struct stream_sink mram_sink;
	size_t used_storage = 0;
	size_t input_size = 21; /* Arbitrary value, chosen to be unaligned */

	int err = get_mram_sink(&mram_sink, (uint8_t *)(SUIT_STORAGE_OFFSET), TEST_DATA_SIZE);
	zassert_equal(err, 0, "get_mram_sink failed - error %i", err);

	err = mram_sink.write(mram_sink.ctx, test_data, &input_size);
	zassert_equal(err, 0, "mram_sink.write failed - error %i", err);

	err = mram_sink.used_storage(mram_sink.ctx, &used_storage);
	zassert_equal(err, 0, "mram_sink.use_storage failed - error %i", err);
	zassert_equal(used_storage, input_size, "mram_sink.use_storage failed - value %d",
		      used_storage);

	err = mram_sink.seek(mram_sink.ctx, input_size + 7);
	zassert_equal(err, 0, "mram_sink.seek failed - error %i", err);

	err = mram_sink.used_storage(mram_sink.ctx, &used_storage);
	zassert_equal(err, 0, "mram_sink.use_storage failed - error %i", err);

	err = mram_sink.write(mram_sink.ctx, &test_data[input_size], &input_size);
	zassert_equal(err, 0, "mram_sink.write failed - error %i", err);

	err = mram_sink.release(mram_sink.ctx);
	zassert_equal(err, 0, "mram_sink.release failed - error %i", err);
}

ZTEST(mram_sink_tests, test_mram_sink_write_NOK)
{
	struct stream_sink mram_sink;
	size_t input_size = 0;

	int err = get_mram_sink(&mram_sink, (uint8_t *)WRITE_ADDR, TEST_DATA_SIZE);
	zassert_equal(err, 0, "get_mram_sink failed - error %i", err);

	err = mram_sink.write(mram_sink.ctx, test_data, &input_size);
	zassert_not_equal(err, 0, "mram_sink.write should have failed - size == 0");

	input_size = 8;
	err = mram_sink.write(NULL, test_data, &input_size);
	zassert_not_equal(err, 0, "mram_sink.write should have failed - ctx == NULL");

	err = mram_sink.write(mram_sink.ctx, NULL, &input_size);
	zassert_not_equal(err, 0, "mram_sink.write should have failed - buf == NULL");

	err = mram_sink.release(mram_sink.ctx);
	zassert_equal(err, 0, "mram_sink.release failed - error %i", err);
}
