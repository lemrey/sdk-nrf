/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <cache_sink.h>
#include <errno.h>
#include <suit_cache_rw.h>
#include <zephyr/drivers/flash.h>

LOG_MODULE_REGISTER(suit_cache_sink, CONFIG_SUIT_LOG_LEVEL);

static int write(void *ctx, uint8_t *buf, size_t *size);
static int seek(void *ctx, size_t offset);
static int flush(void *ctx);
static int used_storage(void *ctx, size_t *size);
static int release(void *ctx);

struct cache_ctx {
	struct suit_cache_slot slot;
	size_t offset;
	size_t offset_limit;
	bool in_use;
	bool write_enabled;
};

static struct cache_ctx ctx;

int dfu_get_cache_sink(struct stream_sink *sink, uint8_t cache_partition_id, const uint8_t *uri,
		       size_t uri_size)
{
	if ((sink != NULL) && (uri != NULL) && (uri_size > 0)) {

		if (ctx.in_use) {
			LOG_ERR("Cache is busy");
			return -EBUSY;
		}

		int ret = dfu_create_cache_slot(cache_partition_id, &ctx.slot, uri, uri_size);

		if (ret != 0) {
			LOG_ERR("Getting slot in cache failed");
			return ret;
		}

		ctx.offset = 0;
		ctx.offset_limit = ctx.slot.size - ctx.slot.data_offset;
		ctx.in_use = true;
		ctx.write_enabled = true;

		sink->erase = NULL;
		sink->write = write;
		sink->seek = seek;
		sink->flush = flush;
		sink->used_storage = used_storage;
		sink->release = release;
		sink->ctx = &ctx;

		LOG_INF("dfu_get_cache_sink success");
		return SUCCESS;
	}

	LOG_ERR("Invalid argument.");
	return INVALID_ARGUMENT;
}

/* Actual write offset = data_offset + offset */
static int write(void *ctx, uint8_t *buf, size_t *size)
{
	if ((ctx != NULL) && (buf != NULL) && (size != NULL)) {
		struct cache_ctx *cache_ctx = (struct cache_ctx *)ctx;

		if (cache_ctx->write_enabled) {
			/* Check if data will fit */
			if ((*size + cache_ctx->offset) < cache_ctx->offset_limit) {
				if (!device_is_ready(cache_ctx->slot.fdev)) {
					LOG_ERR("Flash device not ready.");
					return -EIO;
				}

				int ret = flash_write(cache_ctx->slot.fdev,
						      cache_ctx->slot.slot_offset +
							      cache_ctx->slot.data_offset +
							      cache_ctx->offset,
						      buf, *size);

				if (ret != 0) {
					LOG_ERR("Writing data to cache slot failed.");
					return ret;
				}

				cache_ctx->offset += *size;
				return SUCCESS;
			}
		}

		return WRITE_DISABLED;
	}

	return INVALID_ARGUMENT;
}

static int seek(void *ctx, size_t offset)
{
	if (ctx != NULL) {
		struct cache_ctx *cache_ctx = (struct cache_ctx *)ctx;

		if (cache_ctx->write_enabled) {
			if (offset < cache_ctx->offset_limit) {
				cache_ctx->offset = offset;
				return SUCCESS;
			}
		}

		return WRITE_DISABLED;
	}

	LOG_ERR("Invalid argument.");
	return INVALID_ARGUMENT;
}

static int flush(void *ctx)
{
	return SUCCESS;
}

static int used_storage(void *ctx, size_t *size)
{
	if ((ctx != NULL) && (size != NULL)) {
		struct cache_ctx *cache_ctx = (struct cache_ctx *)ctx;

		*size = cache_ctx->offset;

		return SUCCESS;
	}

	LOG_ERR("Invalid arguments.");
	return INVALID_ARGUMENT;
}

static int release(void *ctx)
{
	if (ctx != NULL) {
		struct cache_ctx *cache_ctx = (struct cache_ctx *)ctx;
		int ret = SUCCESS;

		if (cache_ctx->write_enabled) {
			LOG_INF("Changes were not committed and will be dropped");
			cache_ctx->write_enabled = false;

			ret = dfu_drop_cache_slot(&cache_ctx->slot, cache_ctx->offset);

			if (ret != SUCCESS) {
				LOG_INF("drop_cache_slot success");
				return ret;
			}
		}

		cache_ctx->in_use = false;
		return ret;
	}

	return INVALID_ARGUMENT;
}

int suit_cache_sink_commit(void *ctx)
{
	if (ctx != NULL) {
		struct cache_ctx *cache_ctx = (struct cache_ctx *)ctx;
		int ret = dfu_close_cache_slot(&cache_ctx->slot, cache_ctx->offset);

		if (ret != 0) {
			LOG_ERR("Commit to cache failed.");
			return ret;
		}

		cache_ctx->write_enabled = false;
		return SUCCESS;
	}

	return INVALID_ARGUMENT;
}

int suit_cache_sink_drop(void *ctx)
{
	if (ctx != NULL) {
		struct cache_ctx *cache_ctx = (struct cache_ctx *)ctx;
		int ret = dfu_drop_cache_slot(&cache_ctx->slot, cache_ctx->offset);

		if (ret != 0) {
			LOG_ERR("Drop changes to cache failed.");
			return ret;
		}

		cache_ctx->write_enabled = false;

		return SUCCESS;
	}

	return INVALID_ARGUMENT;
}
