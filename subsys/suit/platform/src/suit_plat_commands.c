/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_platform.h>
#include <suit_platform_internal.h>

int suit_plat_check_slot(suit_component_t component_handle, unsigned int slot)
{
	const struct suit_component_impl *impl = suit_plat_component_impl_get(component_handle);

	if (impl == NULL) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	if (impl->check_slot == NULL) {
		return SUIT_FAIL_CONDITION;
	}

	return impl->check_slot(component_handle, slot);
}

int suit_plat_retrive_manifest(suit_component_t component_handle, uint8_t **envelope_str,
			       size_t *envelope_len)
{
	return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
}

/** File a report on a command result. */
int suit_plat_report(unsigned int rep_policy, struct suit_report *report)
{
	return SUIT_ERR_UNSUPPORTED_COMMAND;
}

int suit_plat_override_image_size(suit_component_t handle, size_t size)
{
	return SUIT_SUCCESS;
}
