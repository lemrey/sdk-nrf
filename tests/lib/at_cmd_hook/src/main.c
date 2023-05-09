/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <stdio.h>
#include <string.h>
#include <nrf_modem_at.h>
#include <modem/nrf_modem_lib.h>
#include <modem/at_cmd_hook.h>
#include <nrf_errno.h>

static bool pre_cfun_cb_called;
static bool post_cfun_cb_called;

static void at_callback_pre_cfun(const char *at_cmd);
static void at_callback_post_cfun(const char *at_cmd, int err);
static void at_callback_pre_cereg(const char *at_cmd);
static void at_callback_post_invalid(const char *at_cmd, int err);

AT_CMD_HOOK(cereg_hook, "AT+CEREG", at_callback_pre_cereg, NULL);
AT_CMD_HOOK(cfun_hook, "AT+CFUN", at_callback_pre_cfun, at_callback_post_cfun);
AT_CMD_HOOK(invalid_hook, "AT+INVALID", NULL, at_callback_post_invalid);

static void at_callback_pre_cfun(const char *at_cmd)
{
	pre_cfun_cb_called = true;
}

static void at_callback_post_cfun(const char *at_cmd, int err)
{
	zassert_ok(err);
	post_cfun_cb_called = true;
}

static void at_callback_pre_cereg(const char *at_cmd)
{
	zassert_unreachable(NULL);
}

static void at_callback_post_invalid(const char *at_cmd, int err)
{
	zassert_equal(65536, err);
}

ZTEST(at_cmd_hook, test_nrf_modem_at_hooks)
{
	int err;

	at_cmd_hook_pause(&cfun_hook);

	err = nrf_modem_at_printf("AT+CFUN?");
	zassert_ok(err, "nrf_modem_at_printf(AT+CFUN?) failed, err %d", err);

	zassert_false(pre_cfun_cb_called, "Pre callback not called as expected");
	zassert_false(post_cfun_cb_called, "Post callback not called as expected");

	at_cmd_hook_resume(&cfun_hook);

	err = nrf_modem_at_printf("AT+CFUN?");
	zassert_ok(err, "nrf_modem_at_printf(AT+CFUN?) failed, err %d", err);

	zassert_true(pre_cfun_cb_called, "Pre callback not called as expected");
	zassert_true(post_cfun_cb_called, "Post callback not called as expected");

	err = nrf_modem_at_printf("AT+INVALID?");
	zassert_equal(65536, err,
		      "nrf_modem_at_printf(AT+INVALID) did not fail as expected, err %d", err);
}

ZTEST_SUITE(at_cmd_hook, NULL, NULL, NULL, NULL, NULL);
