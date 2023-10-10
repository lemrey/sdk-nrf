/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_platform.h>
#include <suit_platform_internal.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(suit_plat_check_image_match, CONFIG_SUIT_LOG_LEVEL);

int suit_plat_check_image_match(suit_component_t component, enum suit_cose_alg alg_id,
				struct zcbor_string *digest, size_t image_size)
{
	return SUIT_ERR_UNSUPPORTED_COMMAND;
}
