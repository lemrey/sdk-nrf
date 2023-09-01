/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <modem/nrf_modem_lib.h>
#include <nrf_modem_at.h>

static void *suite_setup(void)
{
	int err;

	err = nrf_modem_lib_init();
	zassert_ok(err, "Unable to initialize modem, err %d", err);

	return NULL;
}

static void suite_teardown(void *f)
{
	int err;

	err = nrf_modem_lib_shutdown();
	zassert_ok(err, "Unable to shutdown modem, err %d", err);
}

ZTEST(nrf92_devel_app, test_nrf_modem_lib_send_at_cmds)
{
	int err;
	char buf[32];

	for (int i = 0; i < 5; ++i) {
		err = nrf_modem_at_cmd(buf, sizeof(buf), "AT");
		zassert_ok(err, "Error when sending \"AT\", err %d", err);
	}

	err = nrf_modem_at_cmd(buf, sizeof(buf), "AT+CGMR");
	zassert_ok(err, "Error when sending \"AT+CGMR\", err %d", err);
	
	/* Ensure we don't print from outside the buffer. */
	buf[sizeof(buf)-1] = '\0';
	printk("\"AT+CGMR\" response: %s", (char *)buf);
}

ZTEST(nrf92_devel_app, test_nrf_modem_lib_reinitialize)
{
	int err;

	k_msleep(50);

	err = nrf_modem_lib_shutdown();
	zassert_ok(err, "Failed to shutdown modem, err %d", err);

	printk("Shutdown modem complete");

	k_msleep(50);

	err = nrf_modem_lib_init();
	zassert_ok(err, "Unable to reinitialize modem, err %d", err);

	printk("Modem reinitialized");
}

ZTEST_SUITE(nrf92_devel_app, NULL, suite_setup, NULL, NULL, suite_teardown);
