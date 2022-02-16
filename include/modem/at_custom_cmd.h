/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
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
 * @brief Public API for adding custom AT commands using filters in the Modem library with
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
 * @note For the modem library to accept the response in @c buf as a success, the response must
 *       contain "OK\r\n". If the response is an error, use "ERROR\r\n" or appropriate CMS/CME
 *       responses, e.g. "+CMS ERROR: <errorcode>".
 *
 * @param buf Buffer to put response into.
 * @param buf_size Size of the response buffer.
 * @param content Response format.
 * @param ... Format arguments.
 *
 * @retval 0 on success.
 * @retval -NRF_EFAULT if no buffer provided.
 * @retval -NRF_E2BIG if the provided buffer is too small for the response.
 */
int at_custom_cmd_response_buffer_fill(char *buf, size_t buf_size,
		const char *content, ...);


/** Filter is paused. */
#define PAUSED 1
/** Filter is active, default. */
#define ACTIVE 0

/**
 * @brief Define an AT filter.
 *
 * @param name The filter name.
 * @param _cmd The AT command on which the filter should trigger.
 * @param _callback Filtered AT commands callback function.
 * @param ... Optional initial state (@c PAUSED or @c ACTIVE ).
 *	      The default initial filter state is @c ACTIVE .
 */
#define AT_FILTER(name, _cmd, _callback, ...)                                                      \
	static int _callback(char *buf, size_t len, char *at_cmd);                                 \
	STRUCT_SECTION_ITERABLE(nrf_modem_at_cmd_filter, nrf_modem_at_cmd_filter_##name) = {       \
		.cmd = _cmd,                                                                       \
		.callback = _callback,                                                             \
		COND_CODE_1(__VA_ARGS__, (.paused = __VA_ARGS__,), ())                             \
	}

/**
 * @brief Pause filter.
 *
 * @param filter The filter to pause.
 */
#define at_filter_pause(filter) \
	nrf_modem_at_cmd_filter_##filter.paused = 1

/**
 * @brief Resume filter.
 *
 * @param filter The filter to resume.
 */
#define at_filter_resume(filter) \
	nrf_modem_at_cmd_filter_##filter.paused = 0

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* AT_CUSTOM_CMD_H_ */
