/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <cache_sink.h>
#include <flash_sink.h>
#include <errno.h>
#include <dfu_cache_rw.h>
#include <zephyr/drivers/flash.h>
#include <suit_plat_mem_util.h>

LOG_MODULE_REGISTER(suit_cache_sink, CONFIG_SUIT_LOG_LEVEL);

static suit_plat_err_t write(void *ctx, uint8_t *buf, size_t *size);
static suit_plat_err_t seek(void *ctx, size_t offset);
static suit_plat_err_t flush(void *ctx);
static suit_plat_err_t used_storage(void *ctx, size_t *size);
static suit_plat_err_t release(void *ctx);

struct cache_ctx {
	struct suit_cache_slot slot;
	size_t offset;
	size_t offset_limit;
	bool in_use;
	bool write_enabled;
};

static struct cache_ctx ctx;

suit_plat_err_t dfu_get_cache_sink(struct stream_sink *sink, uint8_t cache_partition_id,
				   const uint8_t *uri, size_t uri_size)
{
	if ((sink != NULL) && (uri != NULL) && (uri_size > 0)) {

		if (ctx.in_use) {
			LOG_ERR("Cache is busy");
			return SUIT_PLAT_ERR_BUSY;
		}

		suit_plat_err_t ret = dfu_create_cache_slot(cache_partition_id, &ctx.slot,
							    uri, uri_size);

		if (ret != SUIT_PLAT_SUCCESS) {
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

		return SUIT_PLAT_SUCCESS;
	}

	LOG_ERR("Invalid argument.");
	return SUIT_PLAT_ERR_INVAL;
}

/* Actual write offset = data_offset + offset */
static suit_plat_err_t write(void *ctx, uint8_t *buf, size_t *size)
{
	if ((ctx != NULL) && (buf != NULL) && (size != NULL)) {
		struct cache_ctx *cache_ctx = (struct cache_ctx *)ctx;
		struct stream_sink sink;

		if (cache_ctx->write_enabled) {
			/* Check if data will fit */
			if ((*size + cache_ctx->offset) < cache_ctx->offset_limit) {
				suit_plat_err_t ret = flash_sink_get(
					&sink,
					suit_plat_mem_nvm_ptr_get(cache_ctx->slot.slot_offset +
								  cache_ctx->slot.data_offset +
								  cache_ctx->offset),
					*size, NULL);
				if (ret != SUIT_PLAT_SUCCESS) {
					LOG_ERR("Getting flash_sink failed. %i", ret);
					return SUIT_PLAT_ERR_IO;
				}

				ret = sink.write(sink.ctx, buf, size);
				if (ret != SUIT_PLAT_SUCCESS) {
					LOG_ERR("Writing data to cache slot failed, Flash err: %d.", ret);

					if (sink.release(sink.ctx) != SUIT_PLAT_SUCCESS) {
						LOG_ERR("Sink release failed");
						return SUIT_PLAT_ERR_INVAL;
					}

					return SUIT_PLAT_ERR_IO;
				}

				cache_ctx->offset += *size;

				if (sink.release(sink.ctx) != SUIT_PLAT_SUCCESS) {
					LOG_ERR("Sink release failed");
					return SUIT_PLAT_ERR_INVAL;
				}

				return SUIT_PLAT_SUCCESS;
			}
		}

		return SUIT_PLAT_ERR_INCORRECT_STATE;
	}

	return SUIT_PLAT_ERR_INVAL;
}

static suit_plat_err_t seek(void *ctx, size_t offset)
{
	if (ctx != NULL) {
		struct cache_ctx *cache_ctx = (struct cache_ctx *)ctx;

		if (cache_ctx->write_enabled) {
			if (offset < cache_ctx->offset_limit) {
				cache_ctx->offset = offset;
				return SUIT_PLAT_SUCCESS;
			}
		}

		return SUIT_PLAT_ERR_INCORRECT_STATE;
	}

	LOG_ERR("Invalid argument.");
	return SUIT_PLAT_ERR_INVAL;
}

static suit_plat_err_t flush(void *ctx)
{
	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t used_storage(void *ctx, size_t *size)
{
	if ((ctx != NULL) && (size != NULL)) {
		struct cache_ctx *cache_ctx = (struct cache_ctx *)ctx;

		*size = cache_ctx->offset;

		return SUIT_PLAT_SUCCESS;
	}

	LOG_ERR("Invalid arguments.");
	return SUIT_PLAT_ERR_INVAL;
}

static suit_plat_err_t release(void *ctx)
{
	if (ctx != NULL) {
		struct cache_ctx *cache_ctx = (struct cache_ctx *)ctx;
		suit_plat_err_t ret = SUIT_PLAT_SUCCESS;

		if (cache_ctx->write_enabled) {
			LOG_INF("Changes were not committed and will be dropped");
			cache_ctx->write_enabled = false;

			ret = dfu_drop_cache_slot(&cache_ctx->slot);

			if (ret != SUIT_PLAT_SUCCESS) {
				return ret;
			}
		}

		cache_ctx->in_use = false;
		return ret;
	}

	return SUIT_PLAT_ERR_INVAL;
}

suit_plat_err_t suit_cache_sink_commit(void *ctx)
{
	if (ctx != NULL) {
		struct cache_ctx *cache_ctx = (struct cache_ctx *)ctx;
		suit_plat_err_t ret = dfu_close_cache_slot(&cache_ctx->slot, cache_ctx->offset);

		if (ret != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Commit to cache failed.");
			return ret;
		}

		cache_ctx->write_enabled = false;
		return SUIT_PLAT_SUCCESS;
	}

	return SUIT_PLAT_ERR_INVAL;
}

suit_plat_err_t suit_cache_sink_drop(void *ctx)
{
	if (ctx != NULL) {
		struct cache_ctx *cache_ctx = (struct cache_ctx *)ctx;
		suit_plat_err_t ret = dfu_drop_cache_slot(&cache_ctx->slot);

		if (ret != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Drop changes to cache failed.");
			return ret;
		}

		cache_ctx->write_enabled = false;

		return SUIT_PLAT_SUCCESS;
	}

	return SUIT_PLAT_ERR_INVAL;
}
