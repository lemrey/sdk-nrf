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

int suit_plat_check_image_match(suit_component_t image_handle, enum suit_cose_alg alg_id,
				struct zcbor_string *digest, size_t image_size)
{
	const struct suit_component_impl *impl = suit_plat_component_impl_get(image_handle);
	uint8_t *read_address = NULL;
	size_t read_size = 0;
	struct zcbor_string payload;

	/* Check the component implementation. */
	if (impl == NULL) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	/* Check if the driver supports direct read mode. */
	if (impl->read_address == NULL) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	/* Get the pointer to the readable data. */
	read_size = impl->read_address(image_handle, &read_address);
	if (read_size < image_size) {
		return SUIT_ERR_UNAVAILABLE_PAYLOAD;
	}

	payload.value = read_address;
	payload.len = image_size;

	return suit_plat_check_digest(alg_id, digest, &payload);
}

/** File a report on a command result. */
int suit_plat_report(unsigned int rep_policy, struct suit_report *report)
{
	return SUIT_ERR_UNSUPPORTED_COMMAND;
}
