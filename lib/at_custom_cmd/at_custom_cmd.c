/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <device.h>
#include <init.h>
#include <logging/log.h>
#include <modem/at_custom_cmd.h>
#include <modem/at_monitor.h>
#include <nrf_errno.h>
#include <nrf_modem_at.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <toolchain/common.h>
#include <zephyr.h>

LOG_MODULE_REGISTER(at_filter);

int at_custom_cmd_init(void)
{
	static struct nrf_modem_at_cmd_filter *at_cmd_filter_list;
	bool first_list_item = true;
	int num_items = 0;
	int err;

	STRUCT_SECTION_FOREACH(nrf_modem_at_cmd_filter, e) {
		if (first_list_item) {
			at_cmd_filter_list = e;
			first_list_item = false;
		}
		num_items++;
	}

	err = nrf_modem_at_cmd_filter_set(at_cmd_filter_list, num_items);
	LOG_INF("AT filter enabled with %d entries.", num_items);

	return err;
}

int at_custom_cmd_deinit(void)
{
	return nrf_modem_at_cmd_filter_set(NULL, 0);
}

int at_custom_cmd_sys_init(const struct device *unused)
{
	(void)unused;

	return at_custom_cmd_init();
}

/* Fill response buffer without overflowing the buffer. */
int at_custom_cmd_response_buffer_fill(char *buf, size_t len,
		const char *content, ...)
{
	va_list args;

	if (buf == NULL) {
		LOG_ERR("%s called with NULL buffer", __func__);
		return -NRF_EFAULT;
	}

	va_start(args, content);
	if (vsnprintf(buf, len, content, args) > len) {
		LOG_ERR("The buffer passed to %s is too small", __func__);
		return -NRF_E2BIG;
	}
	va_end(args);

	return 0;
}

#if defined(CONFIG_AT_CUSTOM_CMD_SYS_INIT)
/* Initialize during SYS_INIT. */
SYS_INIT(at_custom_cmd_sys_init, APPLICATION, 1);
#endif
