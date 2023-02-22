/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <modem/at_cmd_hook.h>
#include <nrf_errno.h>
#include <nrf_modem_at.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <zephyr/toolchain/common.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(at_cmd_hook, CONFIG_AT_CMD_HOOK_LOG_LEVEL);

#define HOOK_COUNT (_nrf_modem_at_cmd_hook_list_end - _nrf_modem_at_cmd_hook_list_start)

static int at_cmd_hook_sys_init(void)
{
	int err;
	extern struct nrf_modem_at_cmd_hook _nrf_modem_at_cmd_hook_list_start[];
	extern struct nrf_modem_at_cmd_hook _nrf_modem_at_cmd_hook_list_end[];

	err = nrf_modem_at_cmd_hooks_set(_nrf_modem_at_cmd_hook_list_start, HOOK_COUNT,
					 CONFIG_AT_HOOK_POST_CB_LEN_MAX);
	LOG_INF("AT hooks enabled with %d entries", HOOK_COUNT);

	return err;
}

void at_cmd_hook_pause(struct nrf_modem_at_cmd_hook *entry)
{
	entry->paused = true;
	LOG_DBG("%s(%s)", __func__, entry->cmd);
}

void at_cmd_hook_resume(struct nrf_modem_at_cmd_hook *entry)
{
	entry->paused = false;
	LOG_DBG("%s(%s)", __func__, entry->cmd);
}

SYS_INIT(at_cmd_hook_sys_init, APPLICATION, 0);
