/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <mram_sink.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <suit_plat_mem_util.h>

#define MRAM_WORD_SIZE FLASH_AREA_WRITE_BLOCK_SIZE(suit_storage)
#define MRAM_WORD_MASK 0x0000000F
#define WRITE_OFFSET(a) (suit_plat_get_nvm_offset(a->ptr) + a->offset)

/* Set to more than one to allow multiple contexts in case of parallel execution */
#define SUIT_MAX_MRAM_COMPONENTS 1

LOG_MODULE_REGISTER(suit_mram_sink, CONFIG_SUIT_LOG_LEVEL);

static int write(void *ctx, uint8_t *buf, size_t *size);
static int seek(void *ctx, size_t offset);
static int flush(void *ctx);
static int used_storage(void *ctx, size_t *size);
static int release(void *ctx);

struct mram_ctx {
	size_t size;
	size_t offset;
	size_t offset_limit;
	uint8_t *ptr;
	bool in_use;
};

static struct mram_ctx ctx[SUIT_MAX_MRAM_COMPONENTS];
static const struct device *fdev = NULL;

/**
 * @brief Get the new, free ctx object
 *
 * @return struct mram_ctx* or NULL if no free ctx was found
 */
static struct mram_ctx *get_new_ctx()
{
	for (size_t i = 0; i < SUIT_MAX_MRAM_COMPONENTS; i++) {
		if (!ctx[i].in_use) {
			return &ctx[i];
		}
	}

	return NULL; /* No free ctx */
}

int get_mram_sink(struct stream_sink *sink, uint8_t *dst, size_t size)
{
	if ((dst != NULL) && (size > 0) && (sink != NULL)) {
		struct mram_ctx *ctx = get_new_ctx();

		if (ctx != NULL) {
			if (fdev == NULL) {
				fdev = DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));

				if (!device_is_ready(fdev)) {
					LOG_ERR("Flash device not ready.");
					return FLASH_NOT_READY;
				}
			}

			ctx->offset = 0;
			ctx->offset_limit = (size_t)dst + size;
			ctx->size = 0;
			ctx->ptr = dst;
			ctx->in_use = true;

			sink->write = write;
			sink->seek = seek;
			sink->flush = flush;
			sink->used_storage = used_storage;
			sink->release = release;
			sink->ctx = ctx;

			return SUCCESS; /* SUCCESS */
		}

		LOG_ERR("ERROR - SUIT_MAX_MRAM_COMPONENTS reached.");
		return SUIT_MAX_COMPONENTS_REACHED;
	}

	LOG_ERR("Invalid arguments.");
	return INVALID_ARGUMENT;
}

#if !defined(CONFIG_SOC_FLASH_NRF_MRAM_ONE_BYTE_WRITE_ACCESS) && (MRAM_WORD_SIZE > 1)
static int write_unaligned_start(struct mram_ctx *mram_ctx, size_t *size_left, uint8_t **buf)
{
	uint8_t edit_buffer[MRAM_WORD_SIZE];
	size_t start_offset = 0;
	size_t block_start = 0;
	size_t write_size = 0;

	block_start = ((size_t)(WRITE_OFFSET(mram_ctx) / MRAM_WORD_SIZE)) * MRAM_WORD_SIZE;
	start_offset = WRITE_OFFSET(mram_ctx) - block_start;
	write_size = 0;

	if (flash_read(fdev, block_start, edit_buffer, MRAM_WORD_SIZE) != 0) {
		LOG_ERR("Flash read failed.");
		return READ_1_FAIL;
	}

	/* write_size - how much data from buf will be written */
	write_size = MIN(*size_left, MRAM_WORD_SIZE - start_offset);

	memcpy(edit_buffer + start_offset, *buf, write_size);

	/* Write back edit_buffer that now contains unaligned bytes from the start of buf */
	if (flash_write(fdev, block_start, edit_buffer, MRAM_WORD_SIZE) != 0) {
		LOG_ERR("Writing initial unaligned data failed.");
		return WRITE_1_FAIL;
	}

	/* Move offset for bytes written */
	mram_ctx->offset += write_size;

	/* Move input buffer ptr */
	*buf += write_size;

	/* Decrease size by the number of bytes written */
	*size_left -= write_size;

	return SUCCESS;
}

static int write_aligned(struct mram_ctx *mram_ctx, size_t *size_left, uint8_t **buf,
			 size_t write_size)
{
	size_t block_start = 0;

	/* Write part that is aligned */
	block_start = ((size_t)(WRITE_OFFSET(mram_ctx) / MRAM_WORD_SIZE)) * MRAM_WORD_SIZE;

	if (flash_write(fdev, block_start, *buf, write_size) != 0) {
		LOG_ERR("Writing aligned blocks failed.");
		return WRITE_2_FAIL;
	}

	write_size = *size_left >= write_size ? write_size : *size_left;

	/* Move offset for bytes written */
	mram_ctx->offset += write_size;

	/* Move input buffer ptr */
	*buf += write_size;

	/* Decrease size by the number of bytes written */
	*size_left -= write_size;

	return SUCCESS;
}

static int write_remaining(struct mram_ctx *mram_ctx, size_t *size_left, uint8_t **buf)
{
	uint8_t edit_buffer[MRAM_WORD_SIZE];
	size_t block_start = 0;

	/* Write remaining data */
	block_start = ((size_t)(WRITE_OFFSET(mram_ctx) / MRAM_WORD_SIZE)) * MRAM_WORD_SIZE;

	if (flash_read(fdev, block_start, edit_buffer, MRAM_WORD_SIZE) == 0) {
		memcpy(edit_buffer, *buf, *size_left);

		/* Write back edit_buffer that now contains unaligned bytes from the start of buf */
		if (flash_write(fdev, block_start, edit_buffer, MRAM_WORD_SIZE) != 0) {
			LOG_ERR("Writing remaining unaligned data failed.");
			return WRITE_3_FAIL;
		}

		/* Move offset for bytes written */
		mram_ctx->offset += *size_left;

		return SUCCESS;
	}

	LOG_ERR("Flash read failed.");
	return READ_2_FAIL;
}
#endif /* CONFIG_SOC_FLASH_NRF_MRAM_ONE_BYTE_WRITE_ACCESS */

static int write(void *ctx, uint8_t *buf, size_t *size)
{
	size_t size_left = *size;

	if ((ctx != NULL) && (buf != NULL) && (size_left > 0)) {
		struct mram_ctx *mram_ctx = (struct mram_ctx *)ctx;

		if ((mram_ctx->offset_limit - (size_t)mram_ctx->ptr) >= size_left) {
#if defined(CONFIG_SOC_FLASH_NRF_MRAM_ONE_BYTE_WRITE_ACCESS) || (MRAM_WORD_SIZE == 1)
			int ret = flash_write(fdev, WRITE_OFFSET(mram_ctx), buf, size_left);

			if (ret == SUCCESS) {
				mram_ctx->offset += size_left;
			}

			return ret;

#else /* WRITE_SIZE == MRAM_WORD_SIZE */
			size_t write_size = 0;
			int err = 0;

			if (WRITE_OFFSET(mram_ctx) & MRAM_WORD_MASK) {
				/* Write offset is not aligned with start of block */
				err = write_unaligned_start(mram_ctx, &size_left, &buf);

				if (err != 0) {
					return err;
				}

				if (size_left == 0) {
					/* All data written */
					return SUCCESS;
				}
			}

			/* Number of bytes to be written in context of whole blocks */
			write_size = (size_left / MRAM_WORD_SIZE) * MRAM_WORD_SIZE;

			if (write_size > 0) {
				err = write_aligned(mram_ctx, &size_left, &buf, write_size);

				if (err != 0) {
					return err;
				}

				if (size_left == 0) {
					/* All data written */
					return SUCCESS;
				}
			}

			return write_remaining(mram_ctx, &size_left, &buf);
#endif
		} else {
			LOG_ERR("Write out of bounds.");
			return WRITE_OUT_OF_BOUNDS;
		}
	}

	LOG_ERR("Invalid arguments.");
	return INVALID_ARGUMENT;
}

static int seek(void *ctx, size_t offset)
{
	if (ctx != NULL) {
		struct mram_ctx *mram_ctx = (struct mram_ctx *)ctx;

		if (offset < (mram_ctx->offset_limit - (size_t)mram_ctx->ptr)) {
			mram_ctx->offset = offset;
			return SUCCESS;
		}
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
		struct mram_ctx *mram_ctx = (struct mram_ctx *)ctx;

		*size = mram_ctx->offset;

		return SUCCESS;
	}

	LOG_ERR("Invalid arguments.");
	return INVALID_ARGUMENT;
}

static int release(void *ctx)
{
	if (ctx != NULL) {
		struct mram_ctx *mram_ctx = (struct mram_ctx *)ctx;

		mram_ctx->offset = 0;
		mram_ctx->offset_limit = 0;
		mram_ctx->size = 0;
		mram_ctx->ptr = NULL;
		mram_ctx->in_use = false;

		return SUCCESS;
	}

	LOG_ERR("Invalid arguments.");
	return INVALID_ARGUMENT;
}
