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
#include <modem/at_custom_cmd.h>
#include <nrf_errno.h>

static char response[64];

/* AT filter function declarations. */
static int at_cmd_callback_cmd1(char *buf, size_t len, char *at_cmd)
{
	zassert_mem_equal("AT+CMD1", at_cmd, strlen("AT+CMD1"), NULL);
	return at_custom_cmd_response_buffer_fill(buf, len, "\r\n+CMD1: OK\r\n");
}

static int at_cmd_callback_cmd2(char *buf, size_t len, char *at_cmd)
{
	zassert_mem_equal("AT+CMD2", at_cmd, strlen("AT+CMD2"), NULL);
	return at_custom_cmd_response_buffer_fill(buf, len,
	"\r\n+CPMS: \"TA\",%d,%d,\"TA\",%d,%d\r\nOK\r\n", 0, 3, 0, 3);
}

/* AT filter list
 * Including all comands the filter should check for and function ptr
 * to functions to be called on detection.
 */

AT_FILTER(GCF1, "AT+CMD1", &at_cmd_callback_cmd1);
AT_FILTER(GCF2, "AT+CMD2", &at_cmd_callback_cmd2);

static void test_at_custom_cmd_setup(void)
{
	int err;
	int modem_err;
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
		err = nrf_modem_at_scanf("AT+CEREG?", "\r\n+CEREG: %d,%d", &modem_err, &connected);
		zassert_equal(2, err, "nrf_modem_at_scanf failed, error: %d", err);

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

static void test_at_custom_cmd_response_success(void)
{
	int err;

	err = nrf_modem_at_cmd(response, sizeof(response), "AT+CMD1");
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, "\r\n+CMD1: OK", strlen("\r\n+CMD1: OK"), NULL);

	err = nrf_modem_at_cmd(response, sizeof(response), "AT+CMD2");
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, "\r\n+CPMS: \"TA\",0,3,\"TA\",0,3",
			strlen("\r\n+CPMS: \"TA\",0,3,\"TA\",0,3"), NULL);
}

static void test_at_custom_cmd_buffer_size(void)
{
	int err;

	err = nrf_modem_at_cmd(response, 1, "AT+CMD1");
	zassert_equal(-NRF_E2BIG, err, "nrf_modem_at_cmd failed, error: %d", err);
}

static void test_at_custom_cmd_command_fault_on_NULL_buffer(void)
{
	int err;

	err = nrf_modem_at_cmd(NULL, 100, "AT+CMD1");
	zassert_equal(-NRF_EFAULT, err, "nrf_modem_at_cmd failed, error: %d", err);
}

static void test_at_custom_cmd_shutdown(void)
{
	int err;

	err = at_custom_cmd_deinit();
	zassert_equal(0, err, "custom_at_cmd_deinit failed, error: %d", err);

	err = nrf_modem_at_printf("AT+CFUN=0");
	zassert_equal(0, err, "nrf_modem_at_printf failed, error: %d", err);
}

void test_main(void)
{
	ztest_test_suite(at_cmd_filter,
		ztest_unit_test(test_at_custom_cmd_setup),

		ztest_unit_test(test_at_custom_cmd_response_success),
		ztest_unit_test(test_at_custom_cmd_buffer_size),
		ztest_unit_test(test_at_custom_cmd_command_fault_on_NULL_buffer),

		ztest_unit_test(test_at_custom_cmd_shutdown)
	);

	ztest_run_test_suite(at_cmd_filter);
}
