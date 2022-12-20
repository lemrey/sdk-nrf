/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <suit_platform_internal.h>
#include <zcbor_decode.h>
#include <suit_plat_driver_com.h>
#include <drivers/flash.h>

/* Convert absolute address into an offset, reachable through the flash API. */
#define FLASH_OFFSET(address)  (COND_CODE_1(DT_NODE_EXISTS(DT_NODELABEL(mram10)), \
	((address) - (DT_REG_ADDR(DT_NODELABEL(mram10)) & 0xEFFFFFFFUL)), \
	(address)))


static const struct device *fdev;
/* The size of the current contents in the current slot. */
static size_t read_size;
/* The size of the current contents in the current slot while dry running commands. */
static size_t dry_read_size;


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

	read_size = size;
	dry_read_size = size;
	fdev = DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));

	if (!device_is_ready(fdev)) {
		return SUIT_ERR_CRASH;
	}

	return SUIT_SUCCESS;
}

static int read(suit_component_t handle, size_t offset, uint8_t *buf, size_t *len)
{
	if (len != NULL) {
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
			return (flash_read(fdev, FLASH_OFFSET(offset), buf, *len) == 0) ?
				       SUIT_SUCCESS :
				       SUIT_ERR_CRASH;
		}

		return SUIT_SUCCESS;
	}

	return SUIT_ERR_UNAVAILABLE_PARAMETER;
}

static int write(suit_component_t handle, size_t offset, uint8_t *buf, size_t len)
{
	size_t write_block_size = flash_get_write_block_size(fdev);
	struct zcbor_string *component_id;
	uint8_t cpu_id;
	intptr_t run_address;
	size_t size;

	if (suit_plat_component_id_get(handle, &component_id) != SUIT_SUCCESS) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	if (!plat_com_decode_component_id(component_id, &cpu_id, &run_address, &size)) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	if (len > size) {
		return SUIT_ERR_CRASH;
	}

	if (buf != NULL) {
#ifndef SOC_FLASH_NRF_MRAM_ONE_BYTE_WRITE_ACCESS
		/* Check if len is multiple of write_blck size */
		if (len % write_block_size) {
			return SUIT_ERR_CRASH;
		}
#endif

		return (flash_write(fdev, FLASH_OFFSET(offset), buf, len) == 0) ? SUIT_SUCCESS :
										  SUIT_ERR_CRASH;
	} else {
		if (offset + len > dry_read_size) {
			dry_read_size = offset + len;
		}
	}

	return SUIT_SUCCESS;
}

static int invoke(suit_component_t handle, struct zcbor_string *run_args)
{
	return SUIT_SUCCESS;
}

static size_t read_address(suit_component_t handle, uint8_t **read_address)
{
	if (read_address != NULL) {
		struct zcbor_string *component_id;
		uint8_t cpu_id;
		size_t size;

		if (suit_plat_component_id_get(handle, &component_id) != SUIT_SUCCESS) {
			return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
		}

		if (!plat_com_decode_component_id(component_id, &cpu_id, (intptr_t *)read_address,
						  &size)) {
			return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
		}
	}

	return (read_address == NULL ? dry_read_size : read_size);
}

static int fetch_integrated(suit_component_t handle, struct zcbor_string *payload)
{
	struct zcbor_string *component_id;
	uint8_t cpu_id;
	intptr_t run_address;
	size_t size;

	if (suit_plat_component_id_get(handle, &component_id) != SUIT_SUCCESS) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	if (!plat_com_decode_component_id(component_id, &cpu_id, &run_address, &size)) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	return (write(handle, run_address, (uint8_t *)payload->value, payload->len) == 0) ?
		       SUIT_SUCCESS :
		       SUIT_ERR_CRASH;
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

struct suit_component_impl *suit_component_mram_impl(void)
{
	return &impl;
}
