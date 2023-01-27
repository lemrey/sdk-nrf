/*
 * Copyright (c) 2022-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>
#include <sys/errno.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/kernel.h>
#include <zephyr/fs/fcb.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <modem/trace_backend.h>

LOG_MODULE_REGISTER(modem_trace_backend, CONFIG_MODEM_TRACE_BACKEND_LOG_LEVEL);

#define EXT_FLASH_DEVICE DEVICE_DT_GET(DT_ALIAS(ext_flash))
#define TRACE_OFFSET FLASH_AREA_OFFSET(modem_trace)
#define TRACE_SIZE CONFIG_PM_PARTITION_SIZE_MODEM_TRACE

#define BUF_SIZE CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_FLASH_BUF_SIZE

static trace_backend_processed_cb trace_processed_callback;

static const struct flash_area *modem_trace_area;
static const struct device *flash_dev;
static struct flash_sector trace_flash_sectors[CONFIG_NRF_MODEM_TRACE_FLASH_SECTORS];

struct fcb trace_fcb = {
#if IS_ENABLED(CONFIG_FCB_ALLOW_FIXED_ENDMARKER)
	.f_flags = FCB_FLAGS_CRC_DISABLED,
#endif
};

/* Store in __noinit RAM to perserve in warm boot. */
static __noinit uint32_t magic;
static __noinit size_t stream_buf_written;
static __noinit uint8_t stream_buf[BUF_SIZE];

static bool is_initialized;

static int trace_backend_clear(void);

static size_t buffer_append(const void *data, size_t len)
{
	size_t append_len;

	append_len =  MIN(len, sizeof(stream_buf) - stream_buf_written);

	memcpy(&stream_buf[stream_buf_written], data, append_len);

	stream_buf_written += append_len;

	return append_len;
}

static int buffer_flush(void)
{
	int err;
	struct fcb_entry loc;

	if (!is_initialized) {
		return -EPERM;
	}

	if (!stream_buf_written) {
		return -ENODATA;
	}

	err = fcb_append(&trace_fcb, stream_buf_written, &loc);
	if (err) {
		if (IS_ENABLED(CONFIG_NRF_MODEM_TRACE_FLASH_NOSPACE_ERASE_OLDEST)) {
			/* Erase the oldest sector and append again. */
			err = fcb_rotate(&trace_fcb);
			if (err) {
				LOG_ERR("fcb_rotate failed, err %d", err);
				return err;
			}
			err = fcb_append(&trace_fcb, stream_buf_written, &loc);
		}

		if (err) {
			LOG_ERR("fcb_append failed, err %d", err);
			return -ENOSPC;
		}
	}

	err = flash_area_write(
		trace_fcb.fap, FCB_ENTRY_FA_DATA_OFF(loc), stream_buf, stream_buf_written);
	if (err) {
		LOG_ERR("flash_area_write failed, err %d", err);
		return err;
	}

	err = fcb_append_finish(&trace_fcb, &loc);
	if (err) {
		LOG_ERR("fcb_append_finish failed, err %d", err);
		return err;
	}

	stream_buf_written = 0;

	return 0;
}

static int trace_flash_erase(void)
{
	int err;

	LOG_INF("Erasing external flash");

	err = flash_area_erase(modem_trace_area, 0, TRACE_SIZE);
	if (err) {
		LOG_ERR("flash_erase error: %d", err);
		return err;
	}

	return err;
}

static const uint32_t magic_val = 0x152ac523;

int trace_backend_init(trace_backend_processed_cb trace_processed_cb)
{
	int err;
	const struct flash_parameters *fparam;

	if (trace_processed_cb == NULL) {
		return -EFAULT;
	}

	trace_processed_callback = trace_processed_cb;

	err = flash_area_open(FIXED_PARTITION_ID(MODEM_TRACE), &modem_trace_area);
	if (err) {
		LOG_ERR("flash_area_open error:  %d", err);
		return -ENODEV;
	}

	err = flash_area_has_driver(modem_trace_area);
	if (err == -ENODEV) {
		LOG_ERR("flash_area_has_driver error: %d\n", err);
	}

	flash_dev = flash_area_get_device(modem_trace_area);
	if (flash_dev == NULL) {
		LOG_ERR("Failed to get flash device\n");
		return -ENODEV;
	}

	/* After a cold boot the magic will contain random values. */
	if (magic != magic_val) {
		LOG_DBG("Initializing");
		magic = magic_val;
		stream_buf_written = 0;
		trace_flash_erase();
	}

	uint32_t f_sector_cnt = sizeof(trace_flash_sectors) / sizeof(struct flash_sector);

	err = flash_area_get_sectors(
		FIXED_PARTITION_ID(MODEM_TRACE), &f_sector_cnt, trace_flash_sectors);
	if (err) {
		LOG_ERR("flash_area_get_sectors error: %d", err);
		goto out;
	}

	fparam = flash_get_parameters(flash_dev);

	trace_fcb.f_magic = magic_val;
	trace_fcb.f_erase_value = fparam->erase_value;
	trace_fcb.f_sector_cnt = f_sector_cnt;
	trace_fcb.f_sectors = trace_flash_sectors;

	LOG_DBG("Sectors: %d, first sector: %p, size: %d",
		f_sector_cnt, trace_flash_sectors, trace_flash_sectors[0].fs_size);

	err = fcb_init(FIXED_PARTITION_ID(MODEM_TRACE), &trace_fcb);
	if (err) {
		LOG_ERR("fcb_init error: %d", err);
		goto out;
	}

	is_initialized = true;

	LOG_DBG("Modem trace flash storage initialized\n");

out:
	return 0;
}

static struct fcb_entry loc = { 0 };
static struct flash_sector *sector;

int trace_backend_read(void *buf, size_t len)
{
	int err;

	if (!is_initialized) {
		return -EPERM;
	}

	if (!buf || len < BUF_SIZE) {
		return -EINVAL;
	}

	err = fcb_getnext(&trace_fcb, &loc);
	if (err) {
		if (err == -ENOTSUP) {
			/* No data in FCB */
			if (stream_buf_written) {
				memcpy(buf, stream_buf, stream_buf_written);
				len = stream_buf_written;
				stream_buf_written = 0;
				fcb_rotate(&trace_fcb);
				return len;
			} else {
				loc.fe_sector = 0;
				loc.fe_elem_off = 0;
				sector = NULL;
				return -ENODATA;
			}
		}
		return err;
	}

	err = flash_area_read(
		trace_fcb.fap, FCB_ENTRY_FA_DATA_OFF(loc), buf, loc.fe_data_len);
	if (err) {
		LOG_ERR("Flash_area_read failed, err %d", err);
		goto err;
	}

	/* Erase if done with previous sector. */
	if (sector && (sector != loc.fe_sector)) {
		printk("Rotating\r\n");
		fcb_rotate(&trace_fcb);
	}

	sector = loc.fe_sector;

	return loc.fe_data_len;

err:
	return err;
}

static int stream_write(const void *buf, size_t len)
{
	int err, written;
	size_t to_write = len;
	const uint8_t *bytes = buf;

	if (!is_initialized) {
		return -EPERM;
	}

	while (to_write) {
		written = buffer_append(&bytes[len - to_write], to_write);
		if (written != to_write) {
			err = buffer_flush();
			if (err) {
				LOG_ERR("buffer_flush error %d", err);
				goto out;
			}
		}
		to_write -= written;
	}

out:
	return err;
}

int trace_backend_write(const void *data, size_t len)
{
	int write_ret = stream_write(data, len);

	if (write_ret < 0) {
		LOG_ERR("write failed: %d", write_ret);
	}

	int cb_ret = trace_processed_callback(len);

	if (cb_ret < 0) {
		LOG_ERR("trace_processed_callback failed: %d", cb_ret);
		return cb_ret;
	}

	if (write_ret < 0) {
		return write_ret;
	}

	return (int)len;
}

int trace_backend_clear(void)
{
	int err;

	if (!is_initialized) {
		return -EPERM;
	}

	LOG_DBG("Clearing trace storage");
	stream_buf_written = 0;
	err = fcb_clear(&trace_fcb);

	loc.fe_sector = 0;
	loc.fe_elem_off = 0;
	sector = NULL;

	return err;
}

int trace_backend_deinit(void)
{
	return 0;
}

struct trace_backend trace_backend = {
	.init = trace_backend_init,
	.deinit = trace_backend_deinit,
	.write = trace_backend_write,
	.read = trace_backend_read,
	.clear = trace_backend_clear,
};
