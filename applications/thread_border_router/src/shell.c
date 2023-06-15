/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief nRF Thread Border Router's shell commands
 */

#include <stdio.h>
#include <stdlib.h>

#include <ipv6.h>

#include <zephyr/net/net_if.h>
#include <zephyr/shell/shell.h>

static int get_iface_idx(const struct shell *sh, char *index_str)
{
	char *endptr;
	int idx;

	if (!index_str) {
		shell_warn(sh, "Interface index is missing.\n");
		return -EINVAL;
	}

	idx = strtol(index_str, &endptr, 10);
	if (*endptr != '\0') {
		shell_warn(sh, "Invalid index %s\n", index_str);
		return -ENOENT;
	}

	if (!IN_RANGE(idx, 1, 255)) {
		shell_warn(sh, "Invalid index %d\n", idx);
		return -ERANGE;
	}

	return idx;
}

static int cmd_tbr_nd_send_rs(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface;
	int idx;

	if (argc < 2) {
		return -EINVAL;
	}

	idx = get_iface_idx(sh, argv[1]);
	if (idx < 0) {
		return -EINVAL;
	}

	iface = net_if_get_by_index(idx);

	if (!iface) {
		shell_warn(sh, "No such interface in index %d\n", idx);
		return -ENOEXEC;
	}

	shell_print(sh, "Sending Router Solicitation to iface: %p/%u", iface, idx);

	return net_ipv6_send_rs(iface);
}

SHELL_STATIC_SUBCMD_SET_CREATE(tbr_cmd_nd,
			       SHELL_CMD(send_rs, NULL, "Send Router Solicitation\n"
					 "  Usage: tbr nd send_rs <interface's index>",
					 cmd_tbr_nd_send_rs),
			       SHELL_SUBCMD_SET_END
			       );

SHELL_STATIC_SUBCMD_SET_CREATE(tbr_commands,
			       SHELL_CMD(nd, &tbr_cmd_nd, "Neighbor Discovery commands", NULL),
			       SHELL_SUBCMD_SET_END
			       );

SHELL_CMD_REGISTER(tbr, &tbr_commands, "Thread Border Router commands", NULL);
