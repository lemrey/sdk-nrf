/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <suit_platform_internal.h>
#include <zcbor_decode.h>
#include <suit_plat_driver_com.h>

static uint8_t membuf[256]; ///! The memory buffer, holding the component contents.
static size_t read_size; ///! The size of the current contents in the current slot.
static size_t
	dry_read_size; ///! The size of the current contents in the current slot while dry running commands.

static int init(suit_component_t handle)
{
	struct zcbor_string *component_id;
	uint8_t cpu_id;
	intptr_t invoke_address;
	size_t size;

	/* Verify that the component ID has the correct format. */
	if (suit_plat_component_id_get(handle, &component_id) != SUIT_SUCCESS) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}
	if (!plat_com_decode_component_id(component_id, &cpu_id, &invoke_address, &size)) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	read_size = 0;
	dry_read_size = 0;
	memset(membuf, 0, sizeof(membuf));

	return SUIT_SUCCESS;
}

static int read(suit_component_t handle, size_t offset, uint8_t *buf, size_t *len)
{
	size_t read_len = (buf != NULL) ? read_size : dry_read_size;
	*len = MIN(read_len, *len); // To prevent overflow in the comparison below

	if (offset > read_len - *len) {
		if (offset > read_len) {
			return SUIT_ERR_UNAVAILABLE_PAYLOAD;
		} else {
			*len = read_len - offset;
		}
	}

	if (buf != NULL) {
		memcpy(buf, &membuf[offset], *len);
	}

	return SUIT_SUCCESS;
}

static int write(suit_component_t handle, size_t offset, uint8_t *buf, size_t len)
{
	if ((offset > sizeof(membuf)) || (len > (sizeof(membuf) - offset))) {
		return SUIT_ERR_CRASH;
	}

	if (buf != NULL) {
		memcpy(&membuf[offset], buf, len);
		if (offset + len > read_size) {
			read_size = offset + len;
		}
	} else {
		if (offset + len > dry_read_size) {
			dry_read_size = offset + len;
		}
	}

	return SUIT_SUCCESS;
}

static int invoke(suit_component_t handle, struct zcbor_string *invoke_args)
{
	return SUIT_SUCCESS;
}

static size_t read_address(suit_component_t handle, uint8_t **read_address)
{
	if (read_address != NULL) {
		*read_address = membuf;
	}

	return (read_address == NULL ? dry_read_size : read_size);
}

static int fetch_integrated(suit_component_t handle, struct zcbor_string *payload)
{
	return write(handle, 0, (uint8_t *)payload->value, payload->len);
}

static struct suit_component_impl impl = {
	.init = init,
	.read = read,
	.write = write,
	.invoke = invoke,
	.read_address = read_address,
	.check_vid = plat_com_check_vid,
	.check_cid = plat_com_check_cid,
	.fetch_integrated = fetch_integrated,
};

struct suit_component_impl *suit_component_rambuf_impl(void)
{
	return &impl;
}
