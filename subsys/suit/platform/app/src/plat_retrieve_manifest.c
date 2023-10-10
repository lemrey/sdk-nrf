/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_platform_internal.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(suit_plat_retr_mfst, CONFIG_SUIT_LOG_LEVEL);

int suit_plat_retrieve_manifest(suit_component_t component_handle, uint8_t **envelope_str,
				size_t *envelope_len)
{
	return SUIT_ERR_UNSUPPORTED_COMMAND;
}
