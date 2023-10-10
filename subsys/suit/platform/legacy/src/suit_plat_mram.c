/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/devicetree.h>
#include <suit_platform_internal.h>
#include <zcbor_decode.h>
#include <suit_plat_decode_util.h>
#include <suit_plat_driver_com.h>
#include <zephyr/drivers/flash.h>
#include <suit_plat_mem_util.h>
#ifdef CONFIG_SDFW_RESET_HANDLING_ENABLED
#include <reset_mgr.h>
#endif /* CONFIG_SDFW_RESET_HANDLING_ENABLED */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(suit_plat_mram, CONFIG_FLASH_LOG_LEVEL);

struct component_instance_meta {
	suit_component_t handle;
	/* The size of the current contents in the current slot. */
	size_t read_size;
	/* The size of the current contents in the current slot while dry running commands. */
	size_t dry_read_size;
	bool in_use;
};

static struct component_instance_meta component_instance_meta[CONFIG_SUIT_MAX_MRAM_COMPONENTS];
static const struct device *fdev;

static int get_free_slot_index(void)
{
	for (uint8_t i = 0; i < CONFIG_SUIT_MAX_MRAM_COMPONENTS; i++) {
		if (!component_instance_meta[i].in_use) {
			return i;
		}
	}

	return -1;
}

static void remove_entry(suit_component_t handle)
{
	for (uint8_t i = 0; i < CONFIG_SUIT_MAX_MRAM_COMPONENTS; i++) {
		if (component_instance_meta[i].handle == handle) {
			component_instance_meta[i].in_use = false;
			component_instance_meta[i].handle = 0;
			component_instance_meta[i].read_size = 0;
			component_instance_meta[i].dry_read_size = 0;
		}
	}
}

static int init(suit_component_t handle)
{
	struct zcbor_string *component_id;
	uint8_t cpu_id;
	intptr_t invoke_address;
	size_t size;
	uint8_t index = get_free_slot_index();

	if (index < 0) {
		return SUIT_ERR_CRASH;
	}

	/* Verify that the component ID has the correct format. */
	if (suit_plat_component_id_get(handle, &component_id) != SUIT_SUCCESS) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	if (!suit_plat_decode_component_id(component_id, &cpu_id, &invoke_address, &size)) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	if (suit_plat_component_impl_data_set(handle, &component_instance_meta[index]) !=
	    SUIT_SUCCESS) {
		return SUIT_ERR_CRASH;
	}

	component_instance_meta[index].in_use = true;
	component_instance_meta[index].handle = handle;
	component_instance_meta[index].read_size = size;
	component_instance_meta[index].dry_read_size = size;
	fdev = DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));

	if (!device_is_ready(fdev)) {
		return SUIT_ERR_CRASH;
	}

	return SUIT_SUCCESS;
}

static int read(suit_component_t handle, size_t offset, uint8_t *buf, size_t *len)
{
	struct component_instance_meta *cs;
	struct zcbor_string *component_id;
	uint8_t cpu_id;
	intptr_t run_address;
	size_t size;

	if (suit_plat_component_impl_data_get(handle, (void **)&cs) != SUIT_SUCCESS) {
		return SUIT_ERR_UNAVAILABLE_PARAMETER;
	}

	if (suit_plat_component_id_get(handle, &component_id) != SUIT_SUCCESS) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	if (!suit_plat_decode_component_id(component_id, &cpu_id, &run_address, &size)) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	if (len != NULL) {
		size_t read_len = (buf != NULL) ? cs->read_size : cs->dry_read_size;
		*len = MIN(read_len, *len); /* To prevent overflow in the comparison below. */

		if (offset > read_len - *len) {
			if (offset > read_len) {
				return SUIT_ERR_UNAVAILABLE_PAYLOAD;
			} else {
				*len = read_len - offset;
			}
		}

		if (buf != NULL) {
			return flash_read(fdev, suit_plat_get_nvm_offset((uint8_t *)(run_address + offset)), buf, *len);
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
	struct component_instance_meta *cs;

	if (suit_plat_component_impl_data_get(handle, (void **)&cs) != SUIT_SUCCESS) {
		return SUIT_ERR_UNAVAILABLE_PARAMETER;
	}

	if (suit_plat_component_id_get(handle, &component_id) != SUIT_SUCCESS) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	if (!suit_plat_decode_component_id(component_id, &cpu_id, &run_address, &size)) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	if (len > size) {
		return SUIT_ERR_CRASH;
	}

	if (buf != NULL) {
		if (!IS_ENABLED(CONFIG_SOC_FLASH_NRF_MRAM_ONE_BYTE_WRITE_ACCESS)) {
			/* Check if len is multiple of write_blck size */
			if (len % write_block_size) {
				return SUIT_ERR_CRASH;
			}
		}

		return flash_write(fdev, suit_plat_get_nvm_offset((uint8_t *)(run_address + offset)), buf, len);
	} else {
		if (offset + len > cs->dry_read_size) {
			cs->dry_read_size = offset + len;
		}
	}

	return SUIT_SUCCESS;
}

static int invoke(suit_component_t handle, struct zcbor_string *run_args)
{
#ifdef CONFIG_SDFW_RESET_HANDLING_ENABLED
	struct zcbor_string *component_id;
	uint8_t cpu_id;
	intptr_t run_address;
	size_t size;

	if (suit_plat_component_id_get(handle, &component_id) != SUIT_SUCCESS) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	if (!suit_plat_decode_component_id(component_id, &cpu_id, &run_address, &size)) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	return reset_mgr_start_cpu(cpu_id, run_address, run_address);
#else
	return SUIT_SUCCESS;
#endif /* CONFIG_SDFW_RESET_HANDLING_ENABLED */
}

static size_t read_address(suit_component_t handle, uint8_t **read_address)
{
	struct component_instance_meta *cs;

	if (suit_plat_component_impl_data_get(handle, (void **)&cs) != SUIT_SUCCESS) {
		return 0;
	}

	if (read_address != NULL) {
		struct zcbor_string *component_id;
		uint8_t cpu_id;
		size_t size;

		if (suit_plat_component_id_get(handle, &component_id) != SUIT_SUCCESS) {
			read_address = NULL;
			return 0;
		}

		if (!suit_plat_decode_component_id(component_id, &cpu_id, (intptr_t *)read_address,
						   &size)) {
			read_address = NULL;
			return 0;
		}

	}

	return (read_address == NULL ? cs->dry_read_size : cs->read_size);
}

static int fetch_integrated(suit_component_t handle, struct zcbor_string *payload)
{
	if (payload != NULL) {
		if (payload->value != NULL) {
			int ret = write(handle, 0, (uint8_t *)payload->value, payload->len);

			if (ret != SUIT_SUCCESS) {
				LOG_ERR("write failed: %i", ret);
			}

			return ret;
		} else {
			return SUIT_SUCCESS;
		}
	}

	LOG_ERR("payload NULL");
	return SUIT_ERR_CRASH;
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
	.release = remove_entry,
};

struct suit_component_impl *suit_component_mram_impl(void)
{
	return &impl;
}
