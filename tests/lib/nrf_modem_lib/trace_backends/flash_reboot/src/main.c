/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <stdbool.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>

#include <time.h>
#include <stdlib.h>

#include <nrf_modem_trace.h>
#include <modem/trace_backend.h>

extern struct trace_backend trace_backend;

static int __noinit has_booted;

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

	/* Be explicit as has_booted is __noinit */
	if (has_booted != true) {
		printk("Preparing data\n");
		for (int i = 0; i < 100; i++) {
			err = trace_backend.write(&buf_write[i], BUF_SIZE);
			zassert_equal(BUF_SIZE, err,
				      "write failed on iteration %d, err %d", i, err);
		}

		err = trace_backend.write(buf_write, 50);
		zassert_equal(50, err, "write failed err %d", err);

		has_booted = true;
		sys_reboot(SYS_REBOOT_WARM);
	}

	return NULL;
}

ZTEST(trace_flash_boot, test_trace_flash_write_read_warm)
{
	int ret;

	for (int i = 0; i < 100; i++) {
		ret = trace_backend.read((void *)buf_read, sizeof(buf_read));
		zassert_equal(BUF_SIZE, ret, "read failed on iteration %d, err %d", i, ret);
		zassert_mem_equal(&buf_write[i], buf_read, ret);
	}

	ret = trace_backend.read((void *)buf_read, sizeof(buf_read));
	zassert_equal(50, ret, "read failed, err %d", ret);
	zassert_mem_equal(buf_write, buf_read, ret);

	ret = trace_backend.read((void *)buf_read, sizeof(buf_read));
	zassert_equal(-ENODATA, ret, "read unexpected return %d", ret);

	/* Write new data */
	for (int i = 0; i < 100; i++) {
		ret = trace_backend.write(&buf_write[i], BUF_SIZE);
		zassert_equal(BUF_SIZE, ret, "write failed on iteration %d, err %d", i, ret);
	}
}

ZTEST_SUITE(trace_flash_boot, NULL, suite_setup, NULL, NULL, NULL);
