/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief nRF Thread Border Router's shell commands
 */

#include "net/mdns_server.h"

#include <stdio.h>
#include <stdlib.h>

#include <ipv6.h>

#include <zephyr/net/net_if.h>
#include <zephyr/shell/shell.h>

#define BUFFER_SIZE 256
#define MAX_TXT_RDATA_LEN 255
#define RECORD_ACCESS_TIMEOUT 500

typedef int (*type_handler)(const struct shell *sh, int32_t ttl, const char *name, size_t name_len,
			    size_t argc, char *argv[]);

static char buf[BUFFER_SIZE];

struct type_str_parser {
	int type;
	const char *str;
	type_handler handler;
};

static int add_a(const struct shell *sh, int32_t ttl, const char *name, size_t name_len,
		 size_t argc, char *argv[]);
static int add_aaaa(const struct shell *sh, int32_t ttl, const char *name, size_t name_len,
		 size_t argc, char *argv[]);
static int add_cname(const struct shell *sh, int32_t ttl, const char *name, size_t name_len,
		 size_t argc, char *argv[]);
static int add_ns(const struct shell *sh, int32_t ttl, const char *name, size_t name_len,
		 size_t argc, char *argv[]);
static int add_ptr(const struct shell *sh, int32_t ttl, const char *name, size_t name_len,
		 size_t argc, char *argv[]);
static int add_srv(const struct shell *sh, int32_t ttl, const char *name, size_t name_len,
		 size_t argc, char *argv[]);
static int add_txt(const struct shell *sh, int32_t ttl, const char *name, size_t name_len,
		 size_t argc, char *argv[]);

static struct type_str_parser types_map[] = {
	 { MDNS_RECORD_TYPE_A, "A", add_a },
	 { MDNS_RECORD_TYPE_NS, "NS", add_ns },
	 { MDNS_RECORD_TYPE_CNAME, "CNAME", add_cname },
	 { MDNS_RECORD_TYPE_PTR, "PTR", add_ptr },
	 { MDNS_RECORD_TYPE_TXT, "TXT", add_txt },
	 { MDNS_RECORD_TYPE_AAAA, "AAAA", add_aaaa },
	 { MDNS_RECORD_TYPE_SRV, "SRV", add_srv },
	 { MDNS_RECORD_TYPE_ANY, "ANY", NULL },
	 { MDNS_RECORD_TYPE_INVALID, "INVALID", NULL },
};

static const char *type_to_cstr(enum mdns_record_type type)
{
	struct type_str_parser *res = &types_map[0];

	while (res->type != MDNS_RECORD_TYPE_INVALID && res->type != type) {
		res++;
	}

	return res->str;
}

static enum mdns_record_type type_from_cstr(const char *type)
{
	struct type_str_parser *res = &types_map[0];

	while (res->type != MDNS_RECORD_TYPE_INVALID && strcmp(res->str, type) != 0) {
		res++;
	}

	return res->type;
}

static type_handler get_handler_for_type(enum mdns_record_type type)
{
	struct type_str_parser *res = &types_map[0];

	while (res->type != MDNS_RECORD_TYPE_INVALID && res->type != type) {
		res++;
	}

	return res->handler;
}

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

static void print_rdata_addr(const struct shell *sh, struct mdns_record_handle *handle)
{
	enum mdns_record_type type;
	struct in_addr ipv4_addr;
	struct in6_addr ipv6_addr;
	sa_family_t family;
	void *src;

	if (mdns_record_get_type(handle, K_NO_WAIT, &type)) {
		shell_warn(sh, "failed to get the type");
		return;
	}

	if (type == MDNS_RECORD_TYPE_A) {
		family = AF_INET;
		mdns_record_get_rdata_a(handle, &ipv4_addr, K_NO_WAIT);
		src = &ipv4_addr;
	} else if (type == MDNS_RECORD_TYPE_AAAA) {
		family = AF_INET6;
		mdns_record_get_rdata_aaaa(handle, &ipv6_addr, K_NO_WAIT);
		src = &ipv6_addr;
	} else {
		__ASSERT(false, "invalid record type: %u", type);
		return;
	}

	char addr_buf[INET_ADDRSTRLEN];

	shell_print(sh, "                 answer: %s", net_addr_ntop(family, src, addr_buf,
								     sizeof(addr_buf)));
}

static void print_txt_rdata(const struct shell *sh, struct mdns_record_handle *handle)
{
	int res;
	uint8_t txt_data[MAX_TXT_RDATA_LEN];

	res = mdns_record_get_rdata_txt(handle, txt_data, sizeof(txt_data), K_NO_WAIT);

	if (res > 0) {
		shell_hexdump(sh, txt_data, res);
	} else if (res == 0) {
		shell_print(sh, "no TXT data for the record");
	} else if (res == -EBUSY) {
		shell_warn(sh, "could not acquire the lock within: %u ms, try again",
			   RECORD_ACCESS_TIMEOUT);
	} else {
		shell_warn(sh, "failed to get TXT data");
	}

	return;
}

static void print_domain(const struct shell *sh, struct mdns_record_handle *handle)
{
	enum mdns_record_type type;
	uint16_t srv_priority;
	uint16_t srv_weight;
	uint16_t srv_port;
	int res;

	if (mdns_record_get_type(handle, K_NO_WAIT, &type)) {
		shell_warn(sh, "failed to get the type");
		return;
	}

	switch (type) {
	case MDNS_RECORD_TYPE_NS:
		res = mdns_record_get_rdata_ns(handle, buf, sizeof(buf), K_NO_WAIT);
		break;
	case MDNS_RECORD_TYPE_CNAME:
		res = mdns_record_get_rdata_cname(handle, buf, sizeof(buf), K_NO_WAIT);
		break;
	case MDNS_RECORD_TYPE_PTR:
		res = mdns_record_get_rdata_ptr(handle, buf, sizeof(buf), K_NO_WAIT);
		break;
	case MDNS_RECORD_TYPE_SRV:
		res = mdns_record_get_rdata_srv(handle, &srv_priority, &srv_weight, &srv_port, buf,
					  sizeof(buf), K_NO_WAIT);
		shell_print(sh, "               priority: %u", srv_priority);
		shell_print(sh, "                 weight: %u", srv_weight);
		shell_print(sh, "                   port: %u", srv_port);
		break;
	default:
		return;
	}

	buf[res] = '\0';

	shell_print(sh, "                 answer: %s", buf);
}

static void print_rdata(const struct shell *sh, struct mdns_record_handle *handle)
{
	enum mdns_record_type type;

	if (mdns_record_get_type(handle, K_NO_WAIT, &type)) {
		shell_warn(sh, "failed to get the type");
		return;
	}

	if (type == MDNS_RECORD_TYPE_A || type == MDNS_RECORD_TYPE_AAAA) {
		print_rdata_addr(sh, handle);
	} else if (type == MDNS_RECORD_TYPE_TXT) {
		print_txt_rdata(sh, handle);
	} else {
		print_domain(sh, handle);
	}
}

static void print_record(struct mdns_record_handle *handle, void *user_data) {
	const struct shell *sh = (const struct shell *)user_data;
	size_t read = 0;
	enum mdns_record_type type;
	const char *type_str;
	int32_t ttl;

	read = mdns_record_get_name(handle, buf, sizeof(buf) - 1, K_NO_WAIT);

	if (read < 0) {
		shell_warn(sh, "failed to read the name");
		return;
	}

	buf[read] = '\0';

	if (mdns_record_get_type(handle, K_NO_WAIT, &type)) {
		shell_warn(sh, "failed to get the type");
		return;
	}

	if (mdns_record_get_ttl(handle, K_NO_WAIT, &ttl)) {
		shell_warn(sh, "failed to get the TTL");
		return;
	}

	type_str = type_to_cstr(type);

	shell_print(sh, "record %p: type: %s", handle, type_str);
	shell_print(sh, "                    ttl: %d", ttl);
	shell_print(sh, "                   name: %s", buf);

	print_rdata(sh, handle);

	shell_print(sh, "");
}

static int cmd_tbr_mdns_records_show(const struct shell *sh, size_t argc, char *argv[])
{
	int res;

	res = mdns_records_for_each(print_record, (void *)sh, K_MSEC(RECORD_ACCESS_TIMEOUT));

	if (res < 0) {
		shell_warn(sh, "failed to acquire the lock within %u ms", RECORD_ACCESS_TIMEOUT);
	} else if (res == 0) {
		shell_info(sh, "no mDNS records");
	}

	return 0;
}

static void print_result(const struct shell *sh, int res, const struct mdns_record_handle *handle)
{
	if (res == 0) {
		shell_print(sh, "added record: %p", handle);
	} else if (res == -EBUSY) {
		shell_warn(sh, "failed to acquire the lock, retry");
	} else if (res == -EINVAL) {
		shell_warn(sh, "invalid arguments");
	} else {
		shell_warn(sh, "operation failed");
	}
}

static int add_a(const struct shell *sh, int32_t ttl, const char *name, size_t name_len, size_t argc, char *argv[])
{
	struct in_addr addr;
	struct mdns_record_handle *handle = NULL;
	int res;

	if (argc < 1) {
		shell_warn(sh, "missing IPv4 address");
		return -EINVAL;
	}

	if (net_addr_pton(AF_INET, argv[0], &addr)) {
		shell_warn(sh, "invalid IPv4 address: %s", argv[0]);
		return -EINVAL;
	}

	res = mdns_record_add_a(name, name_len, ttl, &addr, K_MSEC(RECORD_ACCESS_TIMEOUT), &handle);

	print_result(sh, res, handle);

	return res;
}

static int add_aaaa(const struct shell *sh, int32_t ttl, const char *name, size_t name_len, size_t argc, char *argv[])
{
	struct in6_addr addr;
	struct mdns_record_handle *handle = NULL;
	int res;

	if (argc < 1) {
		shell_warn(sh, "missing IPv6 address");
		return -EINVAL;
	}

	if (net_addr_pton(AF_INET6, argv[0], &addr)) {
		shell_warn(sh, "invalid IPv6 address: %s", argv[0]);
		return -EINVAL;
	}

	res = mdns_record_add_aaaa(name, name_len, ttl, &addr, K_MSEC(RECORD_ACCESS_TIMEOUT), &handle);

	print_result(sh, res, handle);

	return res;
}

static int add_cname(const struct shell *sh, int32_t ttl, const char *name, size_t name_len, size_t argc, char *argv[])
{
	struct mdns_record_handle *handle = NULL;
	int res;

	if (argc < 1) {
		shell_warn(sh, "missing CNAME");
		return -EINVAL;
	}

	res = mdns_record_add_cname(name, name_len, ttl, argv[0], strlen(argv[0]), K_MSEC(RECORD_ACCESS_TIMEOUT), &handle);

	print_result(sh, res, handle);

	return res;
}

static int add_ns(const struct shell *sh, int32_t ttl, const char *name, size_t name_len, size_t argc, char *argv[])
{
	struct mdns_record_handle *handle = NULL;
	int res;

	if (argc < 1) {
		shell_warn(sh, "missing NS");
		return -EINVAL;
	}

	res = mdns_record_add_ns(name, name_len, ttl, argv[0], strlen(argv[0]), K_MSEC(RECORD_ACCESS_TIMEOUT), &handle);

	print_result(sh, res, handle);

	return res;

}

static int add_ptr(const struct shell *sh, int32_t ttl, const char *name, size_t name_len, size_t argc, char *argv[])
{
	struct mdns_record_handle *handle = NULL;
	int res;

	if (argc < 1) {
		shell_warn(sh, "missing PTR");
		return -EINVAL;
	}

	res = mdns_record_add_ptr(name, name_len, ttl, argv[0], strlen(argv[0]), K_MSEC(RECORD_ACCESS_TIMEOUT), &handle);

	print_result(sh, res, handle);

	return res;
}

static int add_srv(const struct shell *sh, int32_t ttl, const char *name, size_t name_len, size_t argc, char *argv[])
{
	struct mdns_record_handle *handle = NULL;
	int res;
	uint16_t priority;
	uint16_t weight;
	uint16_t port;
	char *endptr;

	if (argc < 4) {
		shell_warn(sh, "missing arguments");
		return -EINVAL;
	}

	priority = strtol(argv[0], &endptr, 10);
	if (*endptr != '\0') {
		shell_warn(sh, "invalid priority: %s", argv[0]);
		return -EINVAL;
	}

	weight = strtol(argv[1], &endptr, 10);
	if (*endptr != '\0') {
		shell_warn(sh, "invalid weight: %s", argv[1]);
		return -EINVAL;
	}

	port = strtol(argv[2], &endptr, 10);
	if (*endptr != '\0') {
		shell_warn(sh, "invalid port: %s", argv[1]);
		return -EINVAL;
	}

	res = mdns_record_add_srv(name, name_len, ttl, priority, weight, port, argv[3], strlen(argv[3]), K_MSEC(RECORD_ACCESS_TIMEOUT), &handle);

	print_result(sh, res, handle);

	return res;
}

static size_t parse_txt_data(size_t argc, char *argv[], uint8_t *data, size_t max_len)
{
	size_t encoded = 0;
	size_t arg_len;

	for (int i = 0; i < argc; ++i) {
		arg_len = strlen(argv[i]);
		data[encoded++] = arg_len;

		memcpy(&data[encoded], argv[i], arg_len);

		encoded += arg_len;
	}

	return encoded;
}

static int add_txt(const struct shell *sh, int32_t ttl, const char *name, size_t name_len, size_t argc, char *argv[])
{
	struct mdns_record_handle *handle = NULL;
	uint8_t txt_data[MAX_TXT_RDATA_LEN];
	size_t txt_data_len = 0;
	int res;

	txt_data_len = parse_txt_data(argc, argv, txt_data, sizeof(txt_data));
	res = mdns_record_add_txt(name, name_len, ttl, txt_data, txt_data_len, K_MSEC(RECORD_ACCESS_TIMEOUT), &handle);

	print_result(sh, res, handle);

	return 0;
}

static int cmd_tbr_mdns_records_add(const struct shell *sh, size_t argc, char *argv[])
{
	enum mdns_record_type type;
	int32_t ttl;
	char *endptr;
	type_handler handler;

	if (argc < 4) {
		shell_warn(sh, "missing arguments");
		return -EINVAL;
	}

	type = type_from_cstr(argv[1]);
	if (type == MDNS_RECORD_TYPE_INVALID || type == MDNS_RECORD_TYPE_ANY) {
		shell_warn(sh, "invalid record type: %s", argv[1]);
		return -EINVAL;
	}

	handler = get_handler_for_type(type);

	__ASSERT(handler, "invalid mDNS record type handler");

	ttl = strtol(argv[2], &endptr, 10);
	if (*endptr != '\0') {
		shell_warn(sh, "invalid TTL: %s", argv[2]);
		return -ENOENT;
	}

	if (handler(sh, ttl, argv[3], strlen(argv[3]), argc - 4, &argv[4]) != 0) {
		shell_warn(sh, "failed to add mDNS record");
		return -EFAULT;
	}

	return 0;
}

static int cmd_tbr_mdns_records_remove(const struct shell *sh, size_t argc, char *argv[])
{
	struct mdns_record_handle *handle;
	int res;
	char *endptr;

	if (argc < 2) {
		shell_warn(sh, "missing record handler");
		return -EINVAL;
	}

	handle = (struct mdns_record_handle *)strtoul(argv[1], &endptr, 16);

	if (*endptr != '\0') {
		shell_warn(sh, "invalid record handler: %s", argv[1]);
		return -EINVAL;
	}

	res = mdns_record_remove(handle, K_MSEC(RECORD_ACCESS_TIMEOUT));

	if (res == -EINVAL) {
		shell_warn(sh, "record: %s not found", argv[1]);
	} else if (res == -EBUSY) {
		shell_warn(sh, "could not acquire the lock within: %u ms, try again",
			   RECORD_ACCESS_TIMEOUT);
	} else {
		shell_info(sh, "record %s removed", argv[1]);
	}

	return res;
}

SHELL_STATIC_SUBCMD_SET_CREATE(tbr_cmd_nd,
			       SHELL_CMD(send_rs, NULL, "Send Router Solicitation\n"
					 "  Usage: tbr nd send_rs <interface's index>",
					 cmd_tbr_nd_send_rs),
			       SHELL_SUBCMD_SET_END
			       );

SHELL_STATIC_SUBCMD_SET_CREATE(tbr_cmd_mdns_records,
			       SHELL_CMD(show, NULL, "Show the list of mDNS records",
					 cmd_tbr_mdns_records_show),
			       SHELL_CMD(add, NULL, "Add mDNS record",
					 cmd_tbr_mdns_records_add),
			       SHELL_CMD(remove, NULL, "Remove mDNS record",
					 cmd_tbr_mdns_records_remove),
			       SHELL_SUBCMD_SET_END
			       );

SHELL_STATIC_SUBCMD_SET_CREATE(tbr_cmd_mdns,
			       SHELL_CMD(records, &tbr_cmd_mdns_records, "Manage mDNS records",
					 NULL),
			       SHELL_SUBCMD_SET_END
			       );

SHELL_STATIC_SUBCMD_SET_CREATE(tbr_commands,
			       SHELL_CMD(nd, &tbr_cmd_nd, "Neighbor Discovery commands", NULL),
			       SHELL_CMD(mdns, &tbr_cmd_mdns, "mDNS server commands", NULL),
			       SHELL_SUBCMD_SET_END
			       );

SHELL_CMD_REGISTER(tbr, &tbr_commands, "Thread Border Router commands", NULL);
