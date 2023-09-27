/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_platform.h>
#include <suit_platform_internal.h>

int suit_plat_check_fetch(suit_component_t dst_handle, struct zcbor_string *uri)
{
	const struct suit_component_impl *impl = suit_plat_component_impl_get(dst_handle);

	if (impl == NULL) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	/* During SUIT-based boot, all fetch commands become non-integrated, since all
	 * severable parts of envelope are removed.
	 * In such case, the manifest should be considered as valid during dry run,
	 * even if the component does not support fetching external parts.
	 *
	 * Due to the fact, that this check is executed for both: boot and upgrade scenarios,
	 * it is valid to assume that if an install step contains fetch directive, the component
	 * must implement either fetch or fetch_integrated APIs.
	 */
	if (impl->fetch_integrated == NULL) {
		return SUIT_ERR_UNSUPPORTED_COMMAND;
	}

	return SUIT_SUCCESS;
}

int suit_plat_fetch(suit_component_t dst_handle, struct zcbor_string *uri)
{
	const struct suit_component_impl *impl = suit_plat_component_impl_get(dst_handle);

	if (impl == NULL) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	return SUIT_ERR_UNSUPPORTED_COMMAND;
}

static int fetch_integrated(suit_component_t dst_handle, struct zcbor_string *payload, bool dry_run)
{
	struct zcbor_string fetch_payload;
	const struct suit_component_impl *impl = suit_plat_component_impl_get(dst_handle);

	if (impl == NULL) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	if (impl->fetch_integrated == NULL) {
		return SUIT_ERR_UNSUPPORTED_COMMAND;
	}

	fetch_payload.value = (dry_run ? NULL : payload->value);
	fetch_payload.len = payload->len;
	return impl->fetch_integrated(dst_handle, &fetch_payload);
}

int suit_plat_check_fetch_integrated(suit_component_t dst_handle, struct zcbor_string *payload)
{
	return fetch_integrated(dst_handle, payload, true);
}

int suit_plat_fetch_integrated(suit_component_t dst_handle, struct zcbor_string *payload)
{
	return fetch_integrated(dst_handle, payload, false);
}

static int copy(suit_component_t dst_handle, suit_component_t src_handle, bool dry_run)
{
	const struct suit_component_impl *src_impl = suit_plat_component_impl_get(src_handle);
	const struct suit_component_impl *dst_impl = suit_plat_component_impl_get(dst_handle);
	size_t read_size = 0;
	uint8_t *read_address = NULL;

	if ((src_impl == NULL) || (dst_impl == NULL)) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	if ((src_impl->read_address == NULL) || (dst_impl->write == NULL)) {
		return SUIT_ERR_UNSUPPORTED_COMMAND;
	}

	read_size = src_impl->read_address(src_handle, (dry_run ? NULL : &read_address));
	return dst_impl->write(dst_handle, 0, (dry_run ? NULL : read_address), read_size);
}

int suit_plat_check_copy(suit_component_t dst_handle, suit_component_t src_handle)
{
	return copy(dst_handle, src_handle, true);
}

int suit_plat_copy(suit_component_t dst_handle, suit_component_t src_handle)
{
	return copy(dst_handle, src_handle, false);
}

int suit_plat_check_swap(suit_component_t dst_handle, suit_component_t src_handle)
{
	return SUIT_ERR_UNSUPPORTED_COMMAND;
}

int suit_plat_swap(suit_component_t dst_handle, suit_component_t src_handle)
{
	return SUIT_ERR_UNSUPPORTED_COMMAND;
}

int suit_plat_check_invoke(suit_component_t image_handle, struct zcbor_string *invoke_args)
{
	const struct suit_component_impl *impl = suit_plat_component_impl_get(image_handle);

	if (impl == NULL) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	if (impl->invoke == NULL) {
		return SUIT_ERR_UNSUPPORTED_COMMAND;
	}

	return SUIT_SUCCESS;
}

int suit_plat_invoke(suit_component_t image_handle, struct zcbor_string *invoke_args)
{
	const struct suit_component_impl *impl = suit_plat_component_impl_get(image_handle);

	if (impl == NULL) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	if (impl->invoke == NULL) {
		return SUIT_ERR_UNSUPPORTED_COMMAND;
	}

	return impl->invoke(image_handle, invoke_args);
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
