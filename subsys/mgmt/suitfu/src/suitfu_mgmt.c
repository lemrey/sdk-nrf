/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/sys/util.h>
#include <string.h>
#include <stdio.h>

#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>
#include <mgmt/mcumgr/util/zcbor_bulk.h>

#ifdef CONFIG_SSF_SUIT_SERVICE_ENABLED
#include <sdfw_services/suit_service.h>
#endif /* CONFIG_SSF_SUIT_SERVICE_ENABLED */

#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/mgmt/handlers.h>
#include "suitfu_mgmt_priv.h"
#include <suit_plat_mem_util.h>

#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(suitfu_mgmt, CONFIG_MGMT_SUITFU_LOG_LEVEL);

#if (!(DT_NODE_EXISTS(DT_NODELABEL(dfu_partition))))
#error DFU Partition not defined in devicetree
#endif

#define FIXED_PARTITION_ERASE_BLOCK_SIZE(label)                                                    \
	DT_PROP(DT_GPARENT(DT_NODELABEL(label)), erase_block_size)
#define FIXED_PARTITION_WRITE_BLOCK_SIZE(label)                                                    \
	DT_PROP(DT_GPARENT(DT_NODELABEL(label)), write_block_size)

#define DFU_PARTITION_LABEL	 dfu_partition
#define DFU_PARTITION_ADDRESS	 suit_plat_get_nvm_ptr(DFU_PARTITION_OFFSET)
#define DFU_PARTITION_OFFSET	 FIXED_PARTITION_OFFSET(DFU_PARTITION_LABEL)
#define DFU_PARTITION_SIZE	 FIXED_PARTITION_SIZE(DFU_PARTITION_LABEL)
#define DFU_PARTITION_EB_SIZE	 FIXED_PARTITION_ERASE_BLOCK_SIZE(DFU_PARTITION_LABEL)
#define DFU_PARTITION_WRITE_SIZE FIXED_PARTITION_WRITE_BLOCK_SIZE(DFU_PARTITION_LABEL)
#define DFU_PARTITION_DEVICE     FIXED_PARTITION_DEVICE(DFU_PARTITION_LABEL)

#define ENCODE_FLAG(zse, flag, value) (zcbor_tstr_put_lit(zse, flag) && zcbor_bool_put(zse, value))

static size_t image_size = 0;

#ifdef CONFIG_SSF_SUIT_SERVICE_ENABLED
static struct k_work_delayable system_update_work;

static void schedule_system_update(struct k_work *item)
{
	suit_plat_mreg_t update_candidate[] = {
		{
			.mem = DFU_PARTITION_ADDRESS,
			.size = image_size,
		},
	};

	(void)item;
	(void)suit_trigger_update(update_candidate, ARRAY_SIZE(update_candidate));
}
#endif /* CONFIG_SSF_SUIT_SERVICE_ENABLED */

static int img_mgmt_suit_erase_partition(const struct device *fdev, size_t num_bytes)
{
	size_t erase_size = DIV_ROUND_UP(num_bytes, DFU_PARTITION_EB_SIZE) * DFU_PARTITION_EB_SIZE;
	if (erase_size > DFU_PARTITION_SIZE) {
		return MGMT_ERR_ENOMEM;
	}

	LOG_INF("Erasing %p - %p (0x%x bytes)", DFU_PARTITION_ADDRESS,
		(uint8_t *)((size_t)DFU_PARTITION_ADDRESS + erase_size), erase_size);

	int rc = flash_erase(fdev, DFU_PARTITION_OFFSET, erase_size);
	if (rc < 0) {
		return MGMT_ERR_EUNKNOWN;
	}

	return MGMT_ERR_EOK;
}

static int img_mgmt_suit_write_image_data(const struct device *fdev, unsigned int req_offset,
					  const void *addr, unsigned int size, bool flush)
{
	static uint8_t write_buf[DFU_PARTITION_WRITE_SIZE];
	static uint8_t buf_fill_level = 0;
	static size_t offset = 0;
	int err = 0;

	/* Allow to reset the write procedure if the offset is equal to zero. */
	if (req_offset == 0) {
		buf_fill_level = 0;
		offset = 0;
	}

	LOG_DBG("Writing %d bytes (cache fill: %d)", size, buf_fill_level);

	/* Check if cache is continuous with the input buffer. */
	if ((offset + buf_fill_level) != req_offset) {
		LOG_ERR("Write request corrupted. Last offset: %p requested "
			"offset: %p",
			(void *)(DFU_PARTITION_OFFSET + offset + buf_fill_level),
			(void *)(DFU_PARTITION_OFFSET + req_offset));
		return MGMT_ERR_EUNKNOWN;
	}

	/* Fill the write buffer to flush non-aligned bytes from the previous
	 * call. */
	if (buf_fill_level) {
		size_t len = sizeof(write_buf) - buf_fill_level;
		len = MIN(len, size);
		memcpy(&write_buf[buf_fill_level], addr, len);

		buf_fill_level += len;
		addr = ((uint8_t *)addr) + len;
		size -= len;

		/* If write buffer is full - write it into the memory. */
		if (buf_fill_level == sizeof(write_buf)) {
			LOG_DBG("Write continuous %d cache bytes (address: %p)", sizeof(write_buf),
				(void *)(DFU_PARTITION_OFFSET + offset));
			err = flash_write(fdev, DFU_PARTITION_OFFSET + offset, write_buf,
					  sizeof(write_buf));

			buf_fill_level = 0;
			offset += sizeof(write_buf);
		}
	}

	/* Write aligned data directly from input buffer. */
	if ((err == 0) && (size >= sizeof(write_buf))) {
		size_t write_size = ((size / sizeof(write_buf)) * sizeof(write_buf));

		LOG_DBG("Write continuous %d image bytes (address: %p)", write_size,
			(void *)(DFU_PARTITION_OFFSET + offset));
		err = flash_write(fdev, DFU_PARTITION_OFFSET + offset, addr, write_size);

		size -= write_size;
		offset += write_size;
		addr = ((uint8_t *)addr + write_size);
	}

	LOG_DBG("Cache %d bytes (address: %p)", size, (void *)(DFU_PARTITION_OFFSET + offset));

	/* Store remaining bytes into the write buffer. */
	if ((err == 0) && (size > 0)) {
		memcpy(&write_buf[0], addr, size);
		buf_fill_level += size;
	}

	/* Flush buffer if requested. */
	if ((err == 0) && (flush) && (buf_fill_level > 0)) {
		/* Do not leak information about the previous requests. */
		memset(&write_buf[buf_fill_level], 0xFF, sizeof(write_buf) - buf_fill_level);

		LOG_DBG("Flush %d bytes (address: %p)", sizeof(write_buf),
			(void *)(DFU_PARTITION_OFFSET + offset));
		err = flash_write(fdev, DFU_PARTITION_OFFSET + offset, write_buf,
				  sizeof(write_buf));

		buf_fill_level = 0;
		offset += sizeof(write_buf);
	}

	if (flush) {
		const uint8_t *addr = DFU_PARTITION_ADDRESS;
		LOG_INF("Last Chunk Written");
		LOG_INF("Partition address: %p (size: 0x%x), data: %02X %02X "
			"%02X "
			"%02X ...",
			(void *)addr, offset, addr[0], addr[1], addr[2], addr[3]);
	}

	return (err == 0 ? MGMT_ERR_EOK : MGMT_ERR_EUNKNOWN);
}

static int img_mgmt_upload(struct smp_streamer *ctx)
{
	zcbor_state_t *zsd = ctx->reader->zs;
	zcbor_state_t *zse = ctx->writer->zs;
	size_t decoded = 0;
	struct img_mgmt_upload_req req = {
		.off = SIZE_MAX,
		.size = SIZE_MAX,
		.img_data = {0},
		.data_sha = {0},
		.upgrade = false,
		.image = 0,
	};
	int rc = MGMT_ERR_EOK;

	const struct device *fdev = DFU_PARTITION_DEVICE;

	if (!device_is_ready(fdev)) {
		fdev = NULL;
		return MGMT_ERR_ECORRUPT;
	}

	struct zcbor_map_decode_key_val image_upload_decode[] = {
		ZCBOR_MAP_DECODE_KEY_VAL(image, zcbor_uint32_decode, &req.image),
		ZCBOR_MAP_DECODE_KEY_VAL(data, zcbor_bstr_decode, &req.img_data),
		ZCBOR_MAP_DECODE_KEY_VAL(len, zcbor_size_decode, &req.size),
		ZCBOR_MAP_DECODE_KEY_VAL(off, zcbor_size_decode, &req.off),
		ZCBOR_MAP_DECODE_KEY_VAL(sha, zcbor_bstr_decode, &req.data_sha),
		ZCBOR_MAP_DECODE_KEY_VAL(upgrade, zcbor_bool_decode, &req.upgrade)};

	if (zcbor_map_decode_bulk(zsd, image_upload_decode, ARRAY_SIZE(image_upload_decode),
				  &decoded) != 0) {
		LOG_ERR("Decoding upload rerquest failed");
		return MGMT_ERR_EINVAL;
	}

	if (req.off == 0) {
		LOG_INF("New image transfer started (image number: %d)", req.image);
		if (req.image != CONFIG_MGMT_SUITFU_IMAGE_NUMBER) {
			image_size = 0;
			LOG_ERR("Incorrect image number");
			return MGMT_ERR_EACCESSDENIED;
		}

		image_size = req.size;
		if (img_mgmt_suit_erase_partition(fdev, image_size)) {
			LOG_ERR("Erasing DFU partition failed");
			return MGMT_ERR_EUNKNOWN;
		}
	}

	bool last = false;

	if ((image_size == 0) || (req.off + req.img_data.len > image_size)) {
		LOG_ERR("Image boundaries reached");
		return MGMT_ERR_ENOMEM;
	}

	if ((req.off + req.img_data.len) == image_size) {
		last = true;
	}

	if (img_mgmt_suit_write_image_data(fdev, req.off, req.img_data.value, req.img_data.len,
					   last)) {
		rc = MGMT_ERR_ETIMEOUT;
	} else {
		req.off += req.img_data.len;
	}

#ifdef CONFIG_SSF_SUIT_SERVICE_ENABLED
	if ((rc == MGMT_ERR_EOK) && (last)) {
		LOG_INF("Schedule system reboot");
		k_work_init_delayable(&system_update_work, schedule_system_update);

		int ret = k_work_schedule(
			&system_update_work,
			K_MSEC(CONFIG_MGMT_SUITFU_TRIGGER_UPDATE_RESET_DELAY_MS));
		if (ret < 0) {
			LOG_ERR("Unable to reboot the system");
			rc = MGMT_ERR_EBUSY;
		}
	}
#endif /* CONFIG_SSF_SUIT_SERVICE_ENABLED */

	if (zcbor_tstr_put_lit(zse, "rc") && zcbor_int32_put(zse, rc) &&
	    zcbor_tstr_put_lit(zse, "off") && zcbor_size_put(zse, req.off)) {
		return MGMT_ERR_EOK;
	}

	return MGMT_ERR_EMSGSIZE;
}

static int img_mgmt_state_read(struct smp_streamer *ctx)
{
	zcbor_state_t *zse = ctx->writer->zs;
	char installed_vers_str[IMG_MGMT_VER_MAX_STR_LEN] = "<\?\?\?>";
	uint8_t hash[IMG_MGMT_HASH_LEN] = {0};
	struct zcbor_string zhash = {.value = hash, .len = IMG_MGMT_HASH_LEN};
	bool ok;

	/* Let's assume we have 1 'image' with slot 0 that represents installed
	 * envelope.
	 */
	ok = zcbor_tstr_put_lit(zse, "images") && zcbor_list_start_encode(zse, 2);

#ifdef CONFIG_SSF_SUIT_SERVICE_ENABLED
	unsigned int seq_num = 0;
	int alg_id = 0;
	/* ["INSTLD_MFST", RFC4122 uuid5(nordic_vid, 'nRF54H20_sample_root')] */
	const uint8_t component_id[] = {0x82, 0x4c, 0x6b, 'I',	'N',  'S',  'T',  'L',
					'D',  '_',  'M',  'F',	'S',  'T',  0x50, 0x3f,
					0x6a, 0x3a, 0x4d, 0xcd, 0xfa, 0x58, 0xc5, 0xac,
					0xce, 0xf9, 0xf5, 0x84, 0xc4, 0x11, 0x24};

	suit_plat_mreg_t hash_mreg = {.mem = hash, .size = IMG_MGMT_HASH_LEN};
	suit_plat_mreg_t manifest_component_id = {
		.mem = component_id,
		.size = sizeof(component_id),
	};

	if (ok) {
		/* Let's proceed installed envelope */
		int err = suit_get_installed_manifest_info(&manifest_component_id, &seq_num,
							   &alg_id, &hash_mreg);
		if (err != 0) {
			LOG_ERR("Unable to read the current manifest data: %d", err);
			ok = false;
		}

		zhash.value = hash_mreg.mem;
		zhash.len = hash_mreg.size;
	}

	if (ok) {
		int err = snprintf(installed_vers_str, sizeof(installed_vers_str), "%d", seq_num);
		if (err < 0) {
			LOG_ERR("Unable to create manifest version string: %d", err);
			ok = false;
		}
	}
#endif /* CONFIG_SSF_SUIT_SERVICE_ENABLED */

	if (ok) {
		ok = zcbor_map_start_encode(zse, MAX_IMG_CHARACTERISTICS) &&
		     zcbor_tstr_put_lit(zse, "slot") && zcbor_int32_put(zse, 0) &&
		     zcbor_tstr_put_lit(zse, "version");
		LOG_DBG("Manifest slot encoded: %d", ok);
	}

	if (ok) {
		installed_vers_str[sizeof(installed_vers_str) - 1] = '\0';
		ok = zcbor_tstr_put_term(zse, installed_vers_str);
		LOG_DBG("Manifest version encoded: %d", ok);
	}

	if (ok) {
		ok = zcbor_tstr_put_term(zse, "hash") && zcbor_bstr_encode(zse, &zhash);
		LOG_DBG("Manifest hash encoded: %d", ok);
	}

	if (ok) {
		ok = ENCODE_FLAG(zse, "bootable", 1) && ENCODE_FLAG(zse, "confirmed", 1) &&
		     ENCODE_FLAG(zse, "active", 1) && ENCODE_FLAG(zse, "permanent", 1);
		LOG_DBG("Manifest flags encoded: %d", ok);
	}

	if (ok) {
		ok = zcbor_map_end_encode(zse, MAX_IMG_CHARACTERISTICS);
		LOG_DBG("Image map encoded: %d", ok);
	}

	ok = ok && zcbor_list_end_encode(zse, 2);

	return ok ? MGMT_ERR_EOK : MGMT_ERR_EMSGSIZE;
}

static int img_mgmt_erase(struct smp_streamer *ctx)
{
	zcbor_state_t *zse = ctx->writer->zs;
	int rc = 0;
	const struct device *fdev = DFU_PARTITION_DEVICE;

	if (!device_is_ready(fdev)) {
		fdev = NULL;
		rc = MGMT_ERR_ECORRUPT;
	}

	if (rc == 0) {
		if (img_mgmt_suit_erase_partition(fdev, DFU_PARTITION_SIZE)) {
			rc = MGMT_ERR_EUNKNOWN;
		}
	}

	if (zcbor_tstr_put_lit(zse, "rc") && zcbor_int32_put(zse, rc)) {
		return MGMT_ERR_EOK;
	}

	return MGMT_ERR_EMSGSIZE;
}

static const struct mgmt_handler img_mgmt_handlers[] = {
	[IMG_MGMT_ID_STATE] = {.mh_read = img_mgmt_state_read, .mh_write = NULL},
	[IMG_MGMT_ID_UPLOAD] = {.mh_read = NULL, .mh_write = img_mgmt_upload},
	[IMG_MGMT_ID_ERASE] = {.mh_read = NULL, .mh_write = img_mgmt_erase},
};

static struct mgmt_group img_mgmt_group = {
	.mg_handlers = (struct mgmt_handler *)img_mgmt_handlers,
	.mg_handlers_count = ARRAY_SIZE(img_mgmt_handlers),
	.mg_group_id = MGMT_GROUP_ID_IMAGE,
};

void img_mgmt_register_group(void)
{
	mgmt_register_group(&img_mgmt_group);
}

void img_mgmt_unregister_group(void)
{
	mgmt_unregister_group(&img_mgmt_group);
}

#ifdef CONFIG_MGMT_SUITFU_AUTO_REGISTER_HANDLERS
MCUMGR_HANDLER_DEFINE(suitfu_mgmt, img_mgmt_register_group);
#endif
