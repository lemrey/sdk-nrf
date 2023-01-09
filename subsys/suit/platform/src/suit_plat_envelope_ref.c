/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <suit_platform_internal.h>
#include <zcbor_decode.h>
#include <suit_plat_driver_com.h>
#include <suit_types.h>

struct component_instance_meta {
	suit_component_t handle;
	struct zcbor_string payload;
	/* The size of the current contents in the current slot while dry running commands. */
	size_t dry_read_size;
	bool in_use;
};

static struct component_instance_meta
	component_instance_meta[CONFIG_SUIT_MAX_ENVELOPE_REF_COMPONENTS];

static int get_free_slot_index(void)
{
	for (uint8_t i = 0; i < CONFIG_SUIT_MAX_ENVELOPE_REF_COMPONENTS; i++) {
		if (!component_instance_meta[i].in_use) {
			return i;
		}
	}

	return -1;
}

static void remove_entry(suit_component_t handle)
{
	for (uint8_t i = 0; i < CONFIG_SUIT_MAX_ENVELOPE_REF_COMPONENTS; i++) {
		if (component_instance_meta[i].handle == handle) {
			component_instance_meta[i].in_use = false;
			component_instance_meta[i].handle = 0;
			component_instance_meta[i].payload.len = 0;
			component_instance_meta[i].payload.value = NULL;
		}
	}
}

static int init(suit_component_t handle)
{
	int index = get_free_slot_index();

	if (index < 0) {
		/* No free slots */
		return SUIT_ERR_CRASH;
	}

	if (suit_plat_component_impl_data_set(handle, &component_instance_meta[index]) !=
	    SUIT_SUCCESS) {
		return SUIT_ERR_CRASH;
	}

	component_instance_meta[index].in_use = true;

	return SUIT_SUCCESS;
}

static int read(suit_component_t handle, size_t offset, uint8_t *buf, size_t *len)
{
	struct component_instance_meta *meta;

	if (suit_plat_component_impl_data_get(handle, (void **)&meta) == SUIT_SUCCESS) {
		if (len != NULL) {
			size_t read_len = (buf != NULL) ? meta->payload.len : meta->dry_read_size;
			*len = MIN(read_len, *len); // To prevent overflow in the comparison below

			if (offset > read_len - *len) {
				if (offset > read_len) {
					return SUIT_ERR_UNAVAILABLE_PAYLOAD;
				} else {
					*len = read_len - offset;
				}
			}

			if (buf != NULL) {
				memcpy(buf, &meta->payload.value[offset], *len);
			}

			return SUIT_SUCCESS;
		}
	}

	return SUIT_ERR_UNAVAILABLE_PARAMETER;
}

static size_t read_address(suit_component_t handle, uint8_t **read_address)
{
	struct component_instance_meta *meta;

	if (suit_plat_component_impl_data_get(handle, (void **)&meta) != SUIT_SUCCESS) {
		return 0;
	}

	*read_address = (uint8_t *)meta->payload.value;

	return (read_address == NULL ? meta->dry_read_size : meta->payload.len);
}

static int fetch_integrated(suit_component_t handle, struct zcbor_string *payload)
{
	if (payload != NULL) {
		struct component_instance_meta *meta;

		if (suit_plat_component_impl_data_get(handle, (void **)&meta) != SUIT_SUCCESS) {
			return SUIT_ERR_CRASH;
		}

		meta->handle = handle;
		meta->payload.len = payload->len;

		if (payload->value != NULL) { /* normal run */
			meta->payload.value = payload->value;
			meta->dry_read_size = payload->len;
		} else { /* dry run */
			meta->dry_read_size = 0;
		}
	}

	return SUIT_SUCCESS;
}

static struct suit_component_impl impl = {
	.init = init,
	.read = read,
	.write = NULL,
	.invoke = NULL,
	.read_address = read_address,
	.check_vid = plat_com_check_vid,
	.check_cid = plat_com_check_cid,
	.fetch_integrated = fetch_integrated,
	.release = remove_entry,
};

struct suit_component_impl *suit_component_envelope_reference_impl(void)
{
	return &impl;
}
