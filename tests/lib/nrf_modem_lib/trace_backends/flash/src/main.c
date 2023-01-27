/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <stdbool.h>
#include <string.h>
#include <zephyr/kernel.h>

#include <time.h>
#include <stdlib.h>

#include <nrf_modem_trace.h>
#include <modem/trace_backend.h>

extern struct trace_backend trace_backend;

#define BUF_SIZE CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_FLASH_BUF_SIZE
static uint8_t buf_write[2*BUF_SIZE];
static uint8_t buf_read[2*BUF_SIZE];

static int nrf_modem_trace_processed_test_cb(size_t len)
{
	return 0;
}

static void *suite_setup(void)
{
	int err;

	srand(time(NULL));

	err = trace_backend.init(nrf_modem_trace_processed_test_cb);
	zassert_ok(err);

	return NULL;
}

static void test_before(void *f)
{
	for (int i = 0; i < ARRAY_SIZE(buf_write); i++) {
		buf_write[i] = rand();
	}
}

static void test_after(void *f)
{
	trace_backend.clear();
}

ZTEST(trace_flash, test_trace_flash_write_read_buff_size)
{
	int ret;

	for (int i = 0; i < 100; i++) {
		ret = trace_backend.write(&buf_write[i], BUF_SIZE);
		zassert_equal(BUF_SIZE, ret, "write failed on iteration %d, err %d", i, ret);
	}

	for (int i = 0; i < 100; i++) {
		ret = trace_backend.read((void *)buf_read, sizeof(buf_read));
		zassert_equal(BUF_SIZE, ret, "read failed on iteration %d, err %d", i, ret);
		zassert_mem_equal(&buf_write[i], buf_read, ret);
	}

	ret = trace_backend.read((void *)buf_read, sizeof(buf_read));
	zassert_equal(-ENODATA, ret, "read unexpected return %d", ret);
}

ZTEST(trace_flash, test_trace_flash_write_read_random_sizes)
{
	int ret;
	size_t written = 0;
	size_t to_write;
	size_t read = 0;

	for (int i = 0; i < 128; i++) {
		to_write = rand() % 2049;
		ret = trace_backend.write(&buf_write[i], to_write);
		zassert_true(ret > 0, "write failed on iteration %d, err %d", i, ret);
		written += ret;
	}

	while (read < written) {
		ret = trace_backend.read((void *)buf_read, sizeof(buf_read));
		zassert_true(ret > 0, "read failed, err %d", ret);
		read += ret;
	}

	ret = trace_backend.read((void *)buf_read, sizeof(buf_read));
	zassert_equal(-ENODATA, ret, "read unexpected return %d", ret);
}

ZTEST(trace_flash, test_trace_flash_write_read_more_than_buf_size)
{
	int ret;

	ret = trace_backend.write(buf_write, 1224);
	zassert_equal(1224, ret, "write failed, err %d", ret);

	ret = trace_backend.read(buf_read, sizeof(buf_read));
	zassert_equal(1024, ret, "read failed, %d", ret);
	zassert_mem_equal(buf_write, buf_read, ret);

	ret = trace_backend.read(buf_read, sizeof(buf_read));
	zassert_equal(200, ret, "read failed, %d", ret);
	zassert_mem_equal(&buf_write[1024], buf_read, ret);

	ret = trace_backend.write(&buf_write[200], 1124);
	zassert_equal(1124, ret, "write failed, err %d", ret);

	ret = trace_backend.write(buf_write, 300);
	zassert_equal(300, ret, "write failed, err %d", ret);

	ret = trace_backend.read(buf_read, sizeof(buf_read));
	zassert_equal(1024, ret, "read failed, %d", ret);
	zassert_mem_equal(&buf_write[200], buf_read, ret);

	ret = trace_backend.read(buf_read, sizeof(buf_read));
	zassert_equal(400, ret, "read failed, %d", ret);
	zassert_mem_equal(&buf_write[1224], buf_read, 100);
	zassert_mem_equal(buf_write, &buf_read[100], 300);
}

ZTEST(trace_flash, test_trace_flash_read_no_data)
{
	int ret;

	ret = trace_backend.read((void *)buf_read, sizeof(buf_read));
	zassert_equal(-ENODATA, ret, "read unexpected return %d", ret);

	/* Write and read. No data should be read afterwards. */
	ret = trace_backend.write(buf_write, 1224);
	zassert_equal(1224, ret, "write failed, err %d", ret);

	ret = trace_backend.read(buf_read, sizeof(buf_read));
	zassert_equal(1024, ret, "read failed, %d", ret);

	ret = trace_backend.read(buf_read, sizeof(buf_read));
	zassert_equal(200, ret, "read failed, %d", ret);

	ret = trace_backend.read((void *)buf_read, sizeof(buf_read));
	zassert_equal(-ENODATA, ret, "read unexpected return %d", ret);

	/* Make sure the first failed call does not reset read parameters so we read data again. */
	ret = trace_backend.read((void *)buf_read, sizeof(buf_read));
	zassert_equal(-ENODATA, ret, "read unexpected return %d", ret);
}

ZTEST(trace_flash, test_trace_flash_clear)
{
	int ret;
	int i = 0;

	while(i < 1100) {
		ret = trace_backend.write(&buf_write[i], BUF_SIZE);
		if (ret == -ENOSPC) {
			printk("written %d iterations\r\n", i);
			break;
		}
		zassert_equal(BUF_SIZE, ret, "write failed on iteration %d, err %d", i, ret);
		i++;
	}

	zassert_equal(-ENOSPC, ret);

	/* Write should still return -ENOSPC */
	ret = trace_backend.write(&buf_write[i], BUF_SIZE);
	zassert_equal(-ENOSPC, ret);

	ret = trace_backend.clear();
	zassert_ok(ret);

	/* Write should be able to write more. */
	ret = trace_backend.write(&buf_write[i], BUF_SIZE);
	zassert_equal(1024, ret, "unexpected write error %d", ret);
}

ZTEST(trace_flash, test_trace_flash_write_full)
{
	int ret;
	int i = 0;

	while(i < 1100) {
		ret = trace_backend.write(&buf_write[i % 1024], BUF_SIZE);
		if (ret == -ENOSPC) {
			printk("written %d iterations\r\n", i);
			break;
		}
		zassert_equal(BUF_SIZE, ret, "write failed on iteration %d, err %d", i, ret);
		i++;
	}

	zassert_equal(-ENOSPC, ret);

	/* Write should still return -ENOSPC */
	ret = trace_backend.write(&buf_write[i], BUF_SIZE);
	zassert_equal(-ENOSPC, ret);

	/* Read back at least one sector. */
	for (int i = 0; i < 64; i++) {
		ret = trace_backend.read((void *)buf_read, sizeof(buf_read));
		zassert_equal(1024, ret, "read unexpected return %d", ret);
	}

	/* Write should be able to write more. */
	ret = trace_backend.write(&buf_write[i], BUF_SIZE);
	zassert_equal(1024, ret, "unexpected write error %d", ret);
}

ZTEST_SUITE(trace_flash, NULL, suite_setup, test_before, test_after, NULL);
