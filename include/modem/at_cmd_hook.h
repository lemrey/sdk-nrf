/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef AT_CMD_HOOK_H_
#define AT_CMD_HOOK_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file at_cmd_hook.h
 *
 * @defgroup at_cmd_hook Hooks for AT command calls
 *
 * @{
 *
 * @brief Public API for getting callbacks when sending AT commands to the modem using filters in
 * the Modem library with application callbacks.
 *
 */

#include <stddef.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/toolchain/common.h>
#include <nrf_modem_at.h>

/* AT command hook filter is paused. */
#define AT_CMD_HOOK_PAUSED 1
/* AT command hook filter is active, default. */
#define AT_CMD_HOOK_ACTIVE 0

/**
 * @brief Define an AT command hook.
 *
 * @param entry The entry name.
 * @param _filter The (partial) AT command on which the hook should trigger.
 * @param _pre_callback The AT command callback function to be called before the AT command is sent.
 * @param _post_callback The AT command callback function to be called after the response is
 *                       received.
 * @param ... Optional initial state ( @c AT_CMD_HOOK_PAUSED or @c AT_CMD_HOOK_ACTIVE ).
 *	      The default is @c AT_CMD_HOOK_ACTIVE .
 */
#define AT_CMD_HOOK(entry, _filter, _pre_callback, _post_callback, ...)                            \
	static STRUCT_SECTION_ITERABLE(nrf_modem_at_cmd_hook, entry) = {                           \
		.cmd = _filter,                                                                    \
		.pre_callback = _pre_callback,                                                     \
		.post_callback = _post_callback,                                                   \
		COND_CODE_1(__VA_ARGS__, (.paused = __VA_ARGS__,), ())                             \
	}

/**
 * @brief Pause an AT hook.
 *
 * @param entry Pointer to the AT hook to pause.
 */
void at_cmd_hook_pause(struct nrf_modem_at_cmd_hook *entry);

/**
 * @brief Resume an AT hook.
 *
 * @param entry Pointer to the AT hook to resume.
 */
void at_cmd_hook_resume(struct nrf_modem_at_cmd_hook *entry);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* AT_CMD_HOOK_H_ */
