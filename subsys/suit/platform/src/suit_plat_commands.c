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

int suit_plat_check_vid(suit_component_t component_handle, struct zcbor_string *vid_uuid)
{
	const struct suit_component_impl *impl = suit_plat_component_impl_get(component_handle);

	if (impl == NULL) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	if (impl->check_vid == NULL) {
		return SUIT_FAIL_CONDITION;
	}

	return impl->check_vid(component_handle, vid_uuid);
}

int suit_plat_check_cid(suit_component_t component_handle, struct zcbor_string *cid_uuid)
{
	const struct suit_component_impl *impl = suit_plat_component_impl_get(component_handle);

	if (impl == NULL) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	if (impl->check_cid == NULL) {
		return SUIT_FAIL_CONDITION;
	}

	return impl->check_cid(component_handle, cid_uuid);
}

int suit_plat_check_did(suit_component_t component_handle, struct zcbor_string *did_uuid)
{
	const struct suit_component_impl *impl = suit_plat_component_impl_get(component_handle);

	if (impl == NULL) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	if (impl->check_did == NULL) {
		return SUIT_FAIL_CONDITION;
	}

	return impl->check_did(component_handle, did_uuid);
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
