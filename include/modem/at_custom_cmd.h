/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef AT_CUSTOM_CMD_H_
#define AT_CUSTOM_CMD_H_

/**
 * @file at_custom_cmd.h
 *
 * @defgroup at_custom_cmd Custom AT commands
 *
 * @{
 *
 * @brief Public API for adding custom AT commands using filters in modem library with
 * application callbacks.
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <kernel.h>
#include <sys/util_macro.h>
#include <toolchain/common.h>

/**
 * @brief Initialize custom AT commands.
 */
int at_custom_cmd_init(void);

/**
 * @brief Deinitialize custom AT commands.
 */
int at_custom_cmd_deinit(void);

/**
 * @brief Fill response buffer without overflowing the buffer.
 *
 * @param buf Buffer to put response into.
 * @param len Buffer length.
 * @param content Response format.
 * @param ... Format arguments.
 *
 * @note For the modem library to accept the response as a success, the response must contain
 *       "OK\r\n". If the response is an error, use "ERROR\r\n" or appropriate CMS/CME
 *       responses, e.g. "+CMS ERROR: <errorcode>".
 *
 * @retval 0 on success.
 * @retval -NRF_EFAULT if no buffer provided.
 * @retval -NRF_E2BIG if the provided buffer is too small for the response.
 */
int at_custom_cmd_response_buffer_fill(char *buf, size_t len,
		const char *content, ...);

/**
 * @brief Define an AT filter.
 *
 * @param name The filter name.
 * @param _cmd The AT command on which the filter should trigger.
 * @param _callback Filtered AT commands callback function.
 */
#define AT_FILTER(name, _cmd, _callback) \
	STRUCT_SECTION_ITERABLE(nrf_modem_at_cmd_filter, nrf_modem_at_cmd_filter_##name) = { \
		.cmd = _cmd, \
		.callback = _callback \
	}

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* AT_CUSTOM_CMD_H_ */
