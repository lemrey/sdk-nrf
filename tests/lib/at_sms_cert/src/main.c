/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <ztest.h>
#include <stdio.h>
#include <string.h>
#include <nrf_modem_at.h>
#include <modem/nrf_modem_lib.h>
#include <nrf_errno.h>

static char response[64];

static void test_at_sms_cert_setup(void)
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

		err = sscanf(response, "+CEREG: %d,%d\r\n", &err, &connected);
		zassert_equal(2, err, "sscanf failed, error: %d", err);
		retries--;
		if (retries == 0) {
			zassert_unreachable("Network connection timed out");
		}
		k_sleep(K_SECONDS(1));
	} while (connected != 1 && retries != 0);

	err = nrf_modem_at_cmd(response, sizeof(response), "AT+CNMI=3,2,0,1");
	zassert_equal(0, err, "nrf_modem_at_printf failed, error: %d", err);
	zassert_mem_equal(response, "OK\r\n", strlen("OK\r\n"), NULL);
}

static void test_at_sms_cert_cpms(void)
{
	int err;

#define CPMS_1_CMD "AT+CPMS=\"TA\",\"TA\""
#define CPMS_1_RES "+CPMS: 0,3,0,3\r\nOK\r\n"

	err = nrf_modem_at_cmd(response, sizeof(response), CPMS_1_CMD);
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, CPMS_1_RES, strlen(CPMS_1_RES), NULL);

#define CPMS_2_CMD "AT+CPMS?"
#define CPMS_2_RES "+CPMS: \"TA\",0,3,\"TA\",0,3\r\nOK\r\n"

	err = nrf_modem_at_cmd(response, sizeof(response), CPMS_2_CMD);
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, CPMS_2_RES, strlen(CPMS_2_RES), NULL);

#define CPMS_3_CMD "AT+CPMS=?"
#define CPMS_3_RES "+CPMS: (\"TA\"),(\"TA\")\r\nOK\r\n"

	err = nrf_modem_at_cmd(response, sizeof(response), CPMS_3_CMD);
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, CPMS_3_RES, strlen(CPMS_3_RES), NULL);
}

static void test_at_sms_cert_csms(void)
{
	int err;

#define CSMS_1_CMD "AT+CSMS=0"
#define CSMS_1_RES "+CSMS: 1,1,0\r\nOK\r\n"

	err = nrf_modem_at_cmd(response, sizeof(response), CSMS_1_CMD);
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, CSMS_1_RES, strlen(CSMS_1_RES), NULL);

#define CSMS_2_CMD "AT+CSMS=1"
#define CSMS_2_RES "+CSMS: 0,0,0\r\nOK\r\n"

	err = nrf_modem_at_cmd(response, sizeof(response), CSMS_2_CMD);
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, CSMS_2_RES, strlen(CSMS_2_RES), NULL);

#define CSMS_3_CMD "AT+CSMS?"
#define CSMS_3_RES "+CSMS: 0,1,1,0\r\nOK\r\n"

	err = nrf_modem_at_cmd(response, sizeof(response), CSMS_3_CMD);
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, CSMS_3_RES, strlen(CSMS_3_RES), NULL);

#define CSMS_4_CMD "AT+CSMS=?"
#define CSMS_4_RES "+CSMS: 0\r\nOK\r\n"

	err = nrf_modem_at_cmd(response, sizeof(response), CSMS_4_CMD);
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, CSMS_4_RES, strlen(CSMS_4_RES), NULL);
}

static void test_at_sms_cert_csca(void)
{
	int err;

#define CSCA_1_CMD "AT+CSCA=\"+358501234567\""
#define CSCA_1_RES "OK\r\n"

	err = nrf_modem_at_cmd(response, sizeof(response), CSCA_1_CMD);
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, CSCA_1_RES, strlen(CSCA_1_RES), NULL);

#define CSCA_2_CMD "AT+CSCA?"
#define CSCA_2_RES "+CSMS: \"+358501234567\""

	err = nrf_modem_at_cmd(response, sizeof(response), CSCA_2_CMD);
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, CSCA_2_RES, strlen(CSCA_2_RES), NULL);
}

static void test_at_sms_cert_cmgd(void)
{
	int err;

#define CMGD_1_CMD "AT+CMGD=2"
#define CMGD_1_RES "OK\r\n"

	err = nrf_modem_at_cmd(response, sizeof(response), CMGD_1_CMD);
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, CMGD_1_RES, strlen(CMGD_1_RES), NULL);

#define CMGD_2_CMD "AT+CMGD=?"
#define CMGD_2_RES "+CMGD: (0-2)\r\nOK\r\n"

	err = nrf_modem_at_cmd(response, sizeof(response), CMGD_2_CMD);
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, CMGD_2_RES, strlen(CMGD_2_RES), NULL);
}

static void test_at_sms_cert_cmgw(void)
{
	int err;

#define CMGW_CMD "AT+CMGW=1,9<CR>010017116031621300<CR><ESC>"
#define CMGW_1_RES "+CMGW: 0"
#define CMGW_2_RES "+CMGW: 1"

	err = nrf_modem_at_cmd(response, sizeof(response),
		CMGW_CMD);
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, CMGW_1_RES, strlen(CMGW_1_RES), NULL);

	err = nrf_modem_at_cmd(response, sizeof(response),
		CMGW_CMD);
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, CMGW_2_RES, strlen(CMGW_2_RES), NULL);

}

static void test_at_sms_cert_cmss(void)
{
	int err;

#define CMSS_1_CMD "AT+CMSS=1"
#define CMSS_1_RES "+CMSS: "

	/* Require a SIM supporting SMS and mfw 1.3.2(?) or newer. */
	err = nrf_modem_at_cmd(response, sizeof(response), CMSS_1_CMD);
	//zassert_mem_equal(response, "+CMSS: ", strlen("+CMSS: "), NULL); //TODO
}

static void test_at_sms_cert_cmgw_cmgd(void)
{
	int err;

#define CMGW_CMGD_CMD "AT+CMGW=1,9<CR>010017116031621300<CR><ESC>"

#define CMGW_CMGD_1_CMD "AT+CMGD=1"
#define CMGW_CMGD_1_RES "OK\r\n"

	err = nrf_modem_at_cmd(response, sizeof(response), CMGW_CMGD_1_CMD);
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, CMGW_CMGD_1_RES, strlen(CMGW_CMGD_1_RES), NULL);

#define CMGW_CMGD_2_RES "+CMGW: 1"

	err = nrf_modem_at_cmd(response, sizeof(response), CMGW_CMGD_CMD);
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, CMGW_CMGD_2_RES, strlen(CMGW_CMGD_2_RES), NULL);

#define CMGW_CMGD_3_RES "+CMGW: 2"

	err = nrf_modem_at_cmd(response, sizeof(response), CMGW_CMGD_CMD);
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, CMGW_CMGD_3_RES, strlen(CMGW_CMGD_3_RES), NULL);

#define CMGW_CMGD_4_RES "ERROR\r\n"

	err = nrf_modem_at_cmd(response, sizeof(response), CMGW_CMGD_CMD);
	zassert_equal(1<<16, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, CMGW_CMGD_4_RES, strlen(CMGW_CMGD_4_RES), NULL);
}

static void test_at_sms_cert_concatenated(void)
{
	int err;

#define CONCAT_1_CMD "AT+CMGD=1;+CMGD=2"
#define CONCAT_1_RES "OK\r\nOK\r\n"

	err = nrf_modem_at_cmd(response, sizeof(response), CONCAT_1_CMD);
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, CONCAT_1_RES, strlen(CONCAT_1_RES), NULL);

#define CONCAT_2_CMD "AT+CPMS=\"TA\",\"TA\""
#define CONCAT_2_RES "+CPMS: 1,3,1,3\r\nOK\r\n"

	err = nrf_modem_at_cmd(response, sizeof(response), CONCAT_2_CMD);
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	zassert_mem_equal(response, CONCAT_2_RES, strlen(CONCAT_2_RES), NULL);

#define CONCAT_3_CMD "AT+CMGD=?;+CEREG?"
#define CONCAT_3_RES "+CMGD: (0-2)\r\nOK\r\n+CEREG:"

	err = nrf_modem_at_cmd(response, sizeof(response), CONCAT_3_CMD);
	zassert_equal(0, err, "nrf_modem_at_cmd failed, error: %d", err);
	printk("%s", response);
	zassert_mem_equal(response, CONCAT_3_RES, strlen(CONCAT_3_RES), NULL);
}

static void test_at_sms_cert_shutdown(void)
{
	int err;

	err = nrf_modem_at_printf("AT+CFUN=0");
	zassert_equal(0, err, "nrf_modem_at_printf failed, error: %d", err);
}

void test_main(void)
{
	ztest_test_suite(at_cmd_filter,
		ztest_unit_test(test_at_sms_cert_setup),

		ztest_unit_test(test_at_sms_cert_cpms),
		ztest_unit_test(test_at_sms_cert_csms),
		ztest_unit_test(test_at_sms_cert_csca),
		ztest_unit_test(test_at_sms_cert_cmgd),
		ztest_unit_test(test_at_sms_cert_cmgw),
		ztest_unit_test(test_at_sms_cert_cmss),
		ztest_unit_test(test_at_sms_cert_cmgw_cmgd),
		ztest_unit_test(test_at_sms_cert_concatenated),

		ztest_unit_test(test_at_sms_cert_shutdown)
	);

	ztest_run_test_suite(at_cmd_filter);
}
