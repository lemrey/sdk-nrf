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

#define WRITE_ITERATIONS 1100
ZTEST(trace_flash_continous, test_trace_flash_c_write_read)
{
	int ret;

	for (int i = 0;i <= WRITE_ITERATIONS;i++) {
		ret = trace_backend.write(&buf_write[i % 1024], BUF_SIZE);
		zassert_equal(BUF_SIZE, ret, "write failed on iteration %d, err %d", i, ret);
	}

	ret = trace_backend.read((void *)buf_read, sizeof(buf_read));
	zassert_equal(BUF_SIZE, ret, "read failed, err %d", ret);

	while (ret > 0) {
		ret = trace_backend.read((void *)buf_read, sizeof(buf_read));
		if (ret == -ENODATA) {
			break;
		}
		zassert_equal(BUF_SIZE, ret, "read failed, err %d", ret);
	}

	/* Make sure last data is intact. */
	zassert_mem_equal(&buf_write[WRITE_ITERATIONS % 1024], buf_read, BUF_SIZE);
}

ZTEST_SUITE(trace_flash_continous, NULL, suite_setup, test_before, test_after, NULL);
