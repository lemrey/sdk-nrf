/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <nrf_errno.h>
#include <nrf_modem_at.h>
#include <zephyr/logging/log.h>

#include <modem/at_custom_cmd.h>

extern char *strtok_r(char *, const char *, char **);

LOG_MODULE_REGISTER(at_sms_cert, CONFIG_AT_SMS_CERT_LOG_LEVEL);

/* Buffer size for SMS messages. */
#define SMS_BUFFER_LIST_SIZE (3)
#define SMS_BUFFER_MAX_PDU_SIZE (352)

/* Buffer size for Service Center Address. */
#define SCA_BUFFER_SIZE (16)

/* Buffer for SMS messages. */
struct sms_buffer {
	size_t pdu_size;
	char data[SMS_BUFFER_MAX_PDU_SIZE];
};

/* Buffer list for SMS messages. */
static struct sms_buffer sms_buffer_list[SMS_BUFFER_LIST_SIZE];

/* Buffer for SCA. */
static char sca_buff[SCA_BUFFER_SIZE + 1];

/* Custom AT function declarations. */
static int at_cmd_callback_cpms(char *buf, size_t len, char *at_cmd);
static int at_cmd_callback_csms(char *buf, size_t len, char *at_cmd);
static int at_cmd_callback_csca(char *buf, size_t len, char *at_cmd);
static int at_cmd_callback_cscs(char *buf, size_t len, char *at_cmd);
static int at_cmd_callback_cmgd(char *buf, size_t len, char *at_cmd);
static int at_cmd_callback_cmss(char *buf, size_t len, char *at_cmd);
static int at_cmd_callback_cmms(char *buf, size_t len, char *at_cmd);
static int at_cmd_callback_cmgw(char *buf, size_t len, char *at_cmd);
static int at_cmd_callback_cmgf(char *buf, size_t len, char *at_cmd);

int at_sms_cert_filter_callback(char *buf, size_t len, char *at_cmd);

/* AT filters
 * Including all commands the filter should check for and the respective
 * functions to be called on detection.
 */

AT_FILTER(GCF1, "AT+CPMS", &at_sms_cert_filter_callback);
AT_FILTER(GCF2, "AT+CSMS", &at_sms_cert_filter_callback);
AT_FILTER(GCF3, "AT+CSCA", &at_sms_cert_filter_callback);
AT_FILTER(GCF4, "AT+CSCS", &at_sms_cert_filter_callback);
AT_FILTER(GCF5, "AT+CMGD", &at_sms_cert_filter_callback);
AT_FILTER(GCF6, "AT+CMSS", &at_sms_cert_filter_callback);
AT_FILTER(GCF7, "AT+CMGW", &at_sms_cert_filter_callback);
/* Be extra strict as other implementation may be in modem. */
AT_FILTER(GCF8, "AT+CMGF=0", &at_sms_cert_filter_callback);
AT_FILTER(GCF9, "AT+CMMS", &at_sms_cert_filter_callback);

/*
 * Internal AT SMS Cert filters
 *
 * note: The reason for having a shared callback for all filtered commands and internal filters,
 * instead of calling the custom command-specific callbacks directly, is that we want to support
 * multiple commands on the same call, e.g. "AT+CMMS=1;+CMSS=0;+CMSS=1".
 */
struct nrf_modem_at_cmd_filter at_sms_cert_filters[] = {
	{ "AT+CPMS", &at_cmd_callback_cpms }, { "AT+CSMS", &at_cmd_callback_csms },
	{ "AT+CSCA", &at_cmd_callback_csca }, { "AT+CSCS", &at_cmd_callback_cscs },
	{ "AT+CMGD", &at_cmd_callback_cmgd }, { "AT+CMSS", &at_cmd_callback_cmss },
	{ "AT+CMMS", &at_cmd_callback_cmms }, { "AT+CMGW", &at_cmd_callback_cmgw },
	{ "AT+CMGF", &at_cmd_callback_cmgf },
};

#define NUM_INTERNAL_FILTERS (sizeof(at_sms_cert_filters) / sizeof(struct nrf_modem_at_cmd_filter))

/* Match string. */
static int match_string(char *str, const char *at_cmd)
{
	return (strncmp(str, at_cmd, strlen(str)) == 0);
}

/* Delete SMS buffer. */
static void delete_sms_buffer(int index)
{
	sms_buffer_list[index].pdu_size = 0;
}

/* Get number of used SMS buffers. */
static int sms_storage_used_get(void)
{
	int sms_storage_used = 0;
	/* Find number of storage in use. */
	for (int i = 0; i < SMS_BUFFER_LIST_SIZE; i++) {
		if (sms_buffer_list[i].pdu_size != 0) {
			sms_storage_used++;
		}
	}
	return sms_storage_used;
}

/* Get internal callback for custom AT SMS cert commands. */
static nrf_modem_at_cmd_handler_t at_sms_cert_filter_callback_get(char *at_cmd)
{
	bool match;
	int match_len;

	if (!at_cmd) {
		LOG_ERR("filter_callback_get: AT command is empty");
		return NULL;
	}

	if ((sizeof(at_sms_cert_filters) == 0) || (at_sms_cert_filters == NULL)) {
		LOG_ERR("filter_callback_get: No filters set");
		return NULL;
	}

	/* We expect at least "AT+XXXX". */
	if (strlen(at_cmd) < 8) {
		LOG_ERR("filter_callback_get: AT command is too short: %s", log_strdup(at_cmd));
		return NULL;
	}

	for (size_t i = 0; i < NUM_INTERNAL_FILTERS; i++) {
		match_len = MIN(strlen(at_sms_cert_filters[i].cmd), strlen(at_cmd));
		match = strncmp(at_sms_cert_filters[i].cmd, at_cmd, match_len) == 0;
		if (match) {
			return at_sms_cert_filters[i].callback;
		}
	}

	return NULL;
}

/* Internal filter with multiple command support. */
int at_sms_cert_filter_callback(char *buf, size_t len, char *at_cmd)
{
	int err = 0;
	nrf_modem_at_cmd_handler_t callback;
	char *msg, *msg_rest;
	char *buf_left = buf;
	size_t len_left = len;
	int len_tmp = 0;
	int cmss_command_count = 0;
	int num_cmss_command = 0;

	/* Count number of CMSS' and GMGS' in the concatenated AT command. */
	const char *tmp = at_cmd;

	while ((tmp = strstr(tmp, "+CMSS="))) {
		num_cmss_command++;
		tmp++;
	}

	tmp = at_cmd;
	while ((tmp = strstr(tmp, "+CMGS="))) {
		num_cmss_command++;
		tmp++;
	}

	/* Split and loop on multiple commands. */
	msg = strtok_r(at_cmd, ";", &msg_rest);

	if (!msg) {
		err = -NRF_EFAULT;
		return err;
	}

	/* Loop through AT commands. */
	while (msg) {
		/* Count CMSS and CMGS commands and send CMMS=0 before the last. */
		if (match_string("AT+CMSS=", msg) || match_string("AT+CMGS=", msg)) {
			cmss_command_count++;
		}
		if ((num_cmss_command != 0) && (cmss_command_count == num_cmss_command)) {
			LOG_DBG("Sending CMMS");
			nrf_modem_at_printf("AT+CMMS=0");
		}

		callback = at_sms_cert_filter_callback_get(msg);

		if (callback != NULL) {
			/* AT command is filtered. */
			err = (*callback)(buf_left, len_left, msg);
			if (err < 0) {
				return err;
			}
		} else {
			err = nrf_modem_at_cmd(buf_left, len_left, msg);
		}

		if (err != 0) {
			return err;
		}

		/* Get the next command. */
		msg = strtok_r(msg_rest, ";", &msg_rest);
		/* Add AT for concatenated AT-commands. */
		if ((msg != NULL) && (msg - 2 * sizeof(char) >= at_cmd)) {
			msg = msg - 2 * sizeof(char);
			msg[0] = 'A';
			msg[1] = 'T';
		}

		/* Move head of response buffer after current reply. */
		len_tmp = strlen(buf_left);
		buf_left = &(buf_left[len_tmp]);
		len_left = len_left - len_tmp;
	}

	return err;
}

/*
 * AT Filter callback functions.
 */

static int at_cmd_callback_cpms(char *buf, size_t len, char *at_cmd)
{
	int sms_storage_used = sms_storage_used_get();

	/* Test */
	if (match_string("AT+CPMS=?", at_cmd)) {
		return at_custom_cmd_response_buffer_fill(buf, len,
							  "+CPMS: (\"TA\"),(\"TA\")\r\nOK\r\n");
	}

	/* Set */
	if (match_string("AT+CPMS=\"TA\",\"TA\"", at_cmd)) {
		return at_custom_cmd_response_buffer_fill(buf, len, "+CPMS: %d,%d,%d,%d\r\nOK\r\n",
							  sms_storage_used, SMS_BUFFER_LIST_SIZE,
							  sms_storage_used, SMS_BUFFER_LIST_SIZE);
	}
	if (match_string("AT+CPMS=\"SM\",\"SM\",\"MT\"", at_cmd)) {
		return at_custom_cmd_response_buffer_fill(buf, len,
							  "+CPMS: \"SM\",%d,%d,"
							  "\"SM\",%d,%d,\"MT\",%d,%d\r\nOK\r\n",
							  sms_storage_used, SMS_BUFFER_LIST_SIZE,
							  sms_storage_used, SMS_BUFFER_LIST_SIZE,
							  sms_storage_used, SMS_BUFFER_LIST_SIZE);
	}

	/* Read */
	if (match_string("AT+CPMS?", at_cmd)) {
		return at_custom_cmd_response_buffer_fill(
			buf, len, "+CPMS: \"TA\",%d,%d,\"TA\",%d,%d\r\nOK\r\n", sms_storage_used,
			SMS_BUFFER_LIST_SIZE, sms_storage_used, SMS_BUFFER_LIST_SIZE);
	}

	return at_custom_cmd_response_buffer_fill(buf, len, "ERROR\r\n");
}

static int at_cmd_callback_csms(char *buf, size_t len, char *at_cmd)
{
	/* Test */
	if (match_string("AT+CSMS=?", at_cmd)) {
		return at_custom_cmd_response_buffer_fill(buf, len, "+CSMS: 0\r\nOK\r\n");
	}

	/* Set */
	if (match_string("AT+CSMS=0", at_cmd)) {
		return at_custom_cmd_response_buffer_fill(buf, len, "+CSMS: 1,1,0\r\nOK\r\n");
	} else if (match_string("AT+CSMS=1", at_cmd)) {
		return at_custom_cmd_response_buffer_fill(buf, len, "+CSMS: 0,0,0\r\nOK\r\n");
	}

	/* Read */
	if (match_string("AT+CSMS?", at_cmd)) {
		return at_custom_cmd_response_buffer_fill(buf, len, "+CSMS: 0,1,1,0\r\nOK\r\n");
	}

	return at_custom_cmd_response_buffer_fill(buf, len, "ERROR\r\n");
}

static int at_cmd_callback_csca(char *buf, size_t len, char *at_cmd)
{
	/* Set */
	if (match_string("AT+CSCA=", at_cmd)) {
		strncpy(sca_buff, &(at_cmd[8]), SCA_BUFFER_SIZE);
		sca_buff[sizeof(sca_buff) - 1] = '\0';
		return at_custom_cmd_response_buffer_fill(buf, len, "OK\r\n");
	}

	/* Read */
	if (match_string("AT+CSCA?", at_cmd)) {
		return at_custom_cmd_response_buffer_fill(buf, len, "+CSMS: %s\r\nOK\r\n",
							  sca_buff);
	}

	return at_custom_cmd_response_buffer_fill(buf, len, "ERROR\r\n");
}

static int at_cmd_callback_cscs(char *buf, size_t len, char *at_cmd)
{
	/* Set */
	if (match_string("AT+CSCS=", at_cmd)) {
		return at_custom_cmd_response_buffer_fill(buf, len, "OK\r\n");
	}

	return at_custom_cmd_response_buffer_fill(buf, len, "ERROR\r\n");
}

static int at_cmd_callback_cmgd(char *buf, size_t len, char *at_cmd)
{
	char *s1 = NULL;
	int index;
	int flag;

	/* Test */
	if (match_string("AT+CMGD=?", at_cmd)) {
		return at_custom_cmd_response_buffer_fill(buf, len, "+CMGD: (0-2)\r\nOK\r\n");
	}

	/* Set */
	if (match_string("AT+CMGD=", at_cmd)) {
		/* Check flags first. */
		s1 = strstr(at_cmd, ",");
		if (s1) {
			flag = strtol(&(s1[1]), NULL, 10);
			if (flag == 4) {
				/* Delete all buffers. */
				for (int i = 0; i < SMS_BUFFER_LIST_SIZE; i++) {
					delete_sms_buffer(i);
				}
				return at_custom_cmd_response_buffer_fill(buf, len, "OK\r\n");
			}
		}

		/* No supported flag, delete only the buffer. */
		index = strtol(&(at_cmd[8]), NULL, 10);

		if (index >= 0 && index < SMS_BUFFER_LIST_SIZE) {
			delete_sms_buffer(index);
			return at_custom_cmd_response_buffer_fill(buf, len, "OK\r\n");
		}
	}

	return at_custom_cmd_response_buffer_fill(buf, len, "ERROR\r\n");
}

static int at_cmd_callback_cmss(char *buf, size_t len, char *at_cmd)
{
	int err;
	int sms_buffer_index;

	/* Set */
	if (!match_string("AT+CMSS=", at_cmd)) {
		return at_custom_cmd_response_buffer_fill(buf, len, "ERROR\r\n");
	}

	sms_buffer_index = strtol(&(at_cmd[8]), NULL, 10);
	if (sms_buffer_index < 0 || sms_buffer_index >= SMS_BUFFER_LIST_SIZE) {
		return -NRF_EINVAL;
	}
	if (sms_buffer_list[sms_buffer_index].pdu_size == 0) {
		return -NRF_EINVAL;
	}

	err = nrf_modem_at_printf("AT+CNMI=3,2,0,0");
	if (err) {
		LOG_ERR("nrf_modem_at_printf failed on AT+CNMI=3,2,0,0, buf: %s", log_strdup(buf));
		return err;
	}

	/* Send AT+CMGS command to modem. */
	err = nrf_modem_at_cmd(buf, len, "AT+CMGS=%d\r%s\x1a\0",
			       sms_buffer_list[sms_buffer_index].pdu_size,
			       sms_buffer_list[sms_buffer_index].data);
	if (err) {
		LOG_ERR("nrf_modem_at_cmd failed on %s, buf: %s", __func__, log_strdup(buf));
		LOG_DBG("PDU size: %d", sms_buffer_list[sms_buffer_index].pdu_size);
		LOG_HEXDUMP_DBG(sms_buffer_list[sms_buffer_index].data, 340, "pdu (hex):");
		return err;
	}

	if (match_string("+CMGS:", buf)) {
		buf[3] = 'S';
		return 0;
	}

	return err;
}

static int at_cmd_callback_cmms(char *buf, size_t len, char *at_cmd)
{
	int err;

	/* Send to modem without buffer. */
	err = nrf_modem_at_printf(at_cmd);
	if (err) {
		if (err > 0) {
			LOG_ERR("%s failed, error_type: %d, error_value: %d", log_strdup(at_cmd),
				nrf_modem_at_err_type(err), nrf_modem_at_err(err));
		}
		return at_custom_cmd_response_buffer_fill(buf, len, "ERROR\r\n", err);
	}

	return at_custom_cmd_response_buffer_fill(buf, len, "OK\r\n");
}

static int at_cmd_callback_cmgw(char *buf, size_t len, char *at_cmd)
{
	char *s1 = NULL;
	char *s2 = NULL;

	/* Set */
	if (!match_string("AT+CMGW=", at_cmd)) {
		return at_custom_cmd_response_buffer_fill(buf, len, "ERROR\r\n");
	}

	s1 = strstr(at_cmd, "<CR>");
	if (s1 != NULL) {
		/* Set special char after length to use strtol to convert to string. */
		s1[0] = '\r';
		/* Move s1 to after <CR>. */
		s1 = &s1[4];
		s2 = strstr(at_cmd, "<ESC>");
	}

	if (s2 != NULL) {
		/* Find first available SMS buffer. */
		for (int i = 0; i < SMS_BUFFER_LIST_SIZE; i++) {
			if (sms_buffer_list[i].pdu_size != 0) {
				continue;
			}
			/* Store size. */
			sms_buffer_list[i].pdu_size = strtol(&(at_cmd[8]), NULL, 10);
			/* Veryfy that the buffer is large enough. */
			if ((int)(s2 - s1) + 1 > SMS_BUFFER_MAX_PDU_SIZE) {
				LOG_ERR("PDU buffer size is too small (%d > %d).",
					(int)(s2 - s1) + 1, SMS_BUFFER_MAX_PDU_SIZE);
			}
			/* Store in SMS buffer. */
			strncpy(sms_buffer_list[i].data, s1, (size_t)(s2 - s1));
			sms_buffer_list[i].data[(int)(s2 - s1)] = '\0';
			/* Make AT response. */
			return at_custom_cmd_response_buffer_fill(buf, len, "+CMGW: %d\r\nOK\r\n",
								  i);
		}
	}

	LOG_ERR("Could not parse command: %s", log_strdup(at_cmd));

	return at_custom_cmd_response_buffer_fill(buf, len, "ERROR\r\n");
}

static int at_cmd_callback_cmgf(char *buf, size_t len, char *at_cmd)
{
	/* Set */
	if (match_string("AT+CMGF=0", at_cmd)) {
		strncpy(sca_buff, &(at_cmd[8]), SCA_BUFFER_SIZE);
		sca_buff[sizeof(sca_buff) - 1] = '\0';
		return at_custom_cmd_response_buffer_fill(buf, len, "OK\r\n");
	}

	return at_custom_cmd_response_buffer_fill(buf, len, "ERROR\r\n");
}

/* Initialize the library. */
int at_sms_cert_init(const struct device *unused)
{
	(void)unused;

	for (int i = 0; i < SMS_BUFFER_LIST_SIZE; i++) {
		delete_sms_buffer(i);
	}

	return 0;
}

#if defined(CONFIG_AT_SMS_CERT_SYS_INIT)
/* Initialize during SYS_INIT */
SYS_INIT(at_sms_cert_init, APPLICATION, 1);
#endif
