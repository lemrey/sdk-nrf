/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <ztest.h>
#include <stdio.h>
#include <string.h>
#include <nrf_modem_at.h>
#include <modem/nrf_modem_lib.h>
#include <nrf_errno.h>

static char response[64];

static void test_at_cmd_filter_setup(void)
{
	int err;
	int retries = 50;
	int connected = false;

	err = nrf_modem_at_printf("AT+CEREG=1");
	zassert_equal(0, err, "nrf_modem_at_printf failed, error: %d", err);

	err = nrf_modem_at_printf("AT+CESQ=1");
	zassert_equal(0, err, "nrf_modem_at_printf failed, error: %d", err);

	err = nrf_modem_at_printf("AT+CFUN=1");
	zassert_equal(0, err, "nrf_modem_at_printf failed, error: %d", err);

	/* Wait for network connection. */
	do {
		err = nrf_modem_at_cmd(response, sizeof(response), "AT+CEREG?");
		zassert_equal(0, err, "nrf_modem_at_printf failed, error: %d", err);

		err = sscanf(response, "+CEREG: %d,%d", &err, &connected);
		zassert_equal(2, err, "sscanf failed, error: %d", err);
		retries--;
		if (retries == 0) {
			zassert_unreachable("Network connection timed out");
		}
		k_sleep(K_SECONDS(1));
	} while (connected != 1 && retries != 0);

	err = nrf_modem_at_cmd(response, sizeof(response), "AT+CNMI=3,2,0,1");
	zassert_equal(0, err, "nrf_modem_at_printf failed, error: %d", err);
	zassert_mem_equal(response, "OK", strlen("OK"), NULL);
}

static void test_at_cmd_filter_cmd_cpms(void)
{
	int err;

	err = nrf_modem_at_cmd(response, sizeof(response), "AT+CPMS=\"TA\",\"TA\"");
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, "+CPMS: 0,3,0,3", strlen("+CPMS: 0,3,0,3"), NULL);

	err = nrf_modem_at_cmd(response, sizeof(response), "AT+CPMS?");
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, "+CPMS: \"TA\",0,3,\"TA\",0,3",
			strlen("+CPMS: \"TA\",0,3,\"TA\",0,3"), NULL);

	err = nrf_modem_at_cmd(response, sizeof(response), "AT+CPMS=?");
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, "+CPMS: (\"TA\"),(\"TA\")", strlen("+CPMS: (\"TA\"),(\"TA\")"),
			NULL);
}

static void test_at_cmd_filter_cmd_csms(void)
{
	int err;

	err = nrf_modem_at_cmd(response, sizeof(response), "AT+CSMS=0");
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, "+CSMS: 1,1,0", strlen("+CSMS: 1,1,0"), NULL);

	err = nrf_modem_at_cmd(response, sizeof(response), "AT+CSMS=1");
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, "+CSMS: 0,0,0", strlen("+CSMS: 0,0,0"), NULL);

	err = nrf_modem_at_cmd(response, sizeof(response), "AT+CSMS?");
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, "+CSMS: 0,1,1,0", strlen("+CSMS: 0,1,1,0"), NULL);

	err = nrf_modem_at_cmd(response, sizeof(response), "AT+CSMS=?");
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, "+CSMS: 0", strlen("+CSMS: 0"), NULL);
}

static void test_at_cmd_filter_cmd_csca(void)
{
	int err;

	err = nrf_modem_at_cmd(response, sizeof(response), "AT+CSCA=\"+358501234567\"");
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, "+CSMS: OK", strlen("+CSMS: OK"), NULL);

	err = nrf_modem_at_cmd(response, sizeof(response), "AT+CSCA?");
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, "+CSMS: \"+358501234567\"", strlen("+CSMS: \"+358501234567\""),
			NULL);
}

static void test_at_cmd_filter_cmd_cmgd(void)
{
	int err;

	err = nrf_modem_at_cmd(response, sizeof(response), "AT+CMGD=2");
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, "+CSGD: OK", strlen("+CSGD: OK"), NULL);

	err = nrf_modem_at_cmd(response, sizeof(response), "AT+CMGD=?");
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, "+CMGD: (1-3)", strlen("+CMGD: (1-3)"), NULL);
}

static void test_at_cmd_filter_cmd_cmgw(void)
{
	int err;

	err = nrf_modem_at_cmd(response, sizeof(response), "AT+CMGW=1,9\r010017116031621300\x1a");
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, "+CMGW: 1", strlen("+CMGW: 1"), NULL);

	err = nrf_modem_at_cmd(response, sizeof(response), "AT+CMGW=1,9\r010017116031621300\x1a");
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, "+CMGW: 2", strlen("+CMGW: 2"), NULL);
}

static void test_at_cmd_filter_cmd_cmss(void)
{
	int err;

	err = nrf_modem_at_cmd(response, sizeof(response), "AT+CMSS=1\x1a");
	zassert_mem_equal(response, "+CMS ERROR:", strlen("+CMS ERROR:"), NULL);
}

static void test_at_cmd_filter_cmd_cmgw_cmgd(void)
{
	int err;

	err = nrf_modem_at_cmd(response, sizeof(response), "AT+CMGD=2");
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, "+CSGD: OK", strlen("+CSGD: OK"), NULL);

	err = nrf_modem_at_cmd(response, sizeof(response), "AT+CMGW=1,9\r010017116031621300\x1a");
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, "+CMGW: 2", strlen("+CMGW: 2"), NULL);

	err = nrf_modem_at_cmd(response, sizeof(response), "AT+CMGW=1,9\r010017116031621300\x1a");
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, "+CMGW: 3", strlen("+CMGW: 3"), NULL);

	err = nrf_modem_at_cmd(response, sizeof(response), "AT+CMGW=1,9\r010017116031621300\x1a");
	zassert_equal(1<<16, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, "ERROR", strlen("ERROR"), NULL);
}

static void test_at_cmd_filter_buffer_size(void)
{
	int err;

	err = nrf_modem_at_cmd(response, 1, "AT+CMGD=2");
	zassert_equal(-NRF_E2BIG, err, "nrf_modem_at_cmd failed, error: %d", err);

	err = nrf_modem_at_cmd(NULL, 100, "AT+CMGD=2");
	zassert_equal(-NRF_EFAULT, err, "nrf_modem_at_cmd failed, error: %d", err);
}

void test_main(void)
{
	ztest_test_suite(at_cmd_filter,
		ztest_unit_test(test_at_cmd_filter_setup),
		ztest_unit_test(test_at_cmd_filter_cmd_cpms),
		ztest_unit_test(test_at_cmd_filter_cmd_csms),
		ztest_unit_test(test_at_cmd_filter_cmd_csca),
		ztest_unit_test(test_at_cmd_filter_cmd_cmgd),
		ztest_unit_test(test_at_cmd_filter_cmd_cmgw),
		ztest_unit_test(test_at_cmd_filter_cmd_cmss),
		ztest_unit_test(test_at_cmd_filter_cmd_cmgw_cmgd),
		ztest_unit_test(test_at_cmd_filter_buffer_size)
	);

	ztest_run_test_suite(at_cmd_filter);
}
