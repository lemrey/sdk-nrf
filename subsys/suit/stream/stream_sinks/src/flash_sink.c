/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <flash_sink.h>
#include <zephyr/drivers/flash.h>
#include <suit_plat_mem_util.h>

/* Definitions for SOC internal nonvolatile memory */
#if (DT_NODE_EXISTS(DT_NODELABEL(mram10))) /* nrf54H20 */
	#define INTERNAL_NVM_START (DT_REG_ADDR(DT_NODELABEL(mram10)) & 0xFFFFFFUL)
	#define INTERNAL_NVM_SIZE DT_REG_SIZE(DT_NODELABEL(mram10)) + DT_REG_SIZE(DT_NODELABEL(mram11))
#elif (DT_NODE_EXISTS(DT_NODELABEL(flash0))) /* nrf52 or flash simulator */
	#define INTERNAL_NVM_START DT_REG_ADDR(DT_NODELABEL(flash0))
	#define INTERNAL_NVM_SIZE DT_REG_SIZE(DT_NODELABEL(flash0))
#elif (DT_NODE_EXISTS(DT_NODELABEL(rram0))) /* nrf54l15 */
	#define INTERNAL_NVM_START DT_REG_ADDR(DT_NODELABEL(rram0))
	#define INTERNAL_NVM_SIZE DT_REG_SIZE(DT_NODELABEL(rram0))
#else
	#error "No recognizable internal nvm nodes found."
#endif

/* Example of how external nvm could be added
 * #define EXTERNAL_NVM_START
 * #define EXTERNAL_NVM_SIZE
 */

#define INTERNAL_NVM_DEV DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller))
/* For external nvm, device could be defined for example
 * #define INTERNAL_NVM_DEV DT_INST(0, jedec_spi_nor)
 */

#if defined(CONFIG_SOC_FLASH_NRF_MRAM_ONE_BYTE_WRITE_ACCESS)
	#define SWAP_BUFFER_SIZE 4
#else
	#define SWAP_BUFFER_SIZE 16
#endif /* CONFIG_SOC_FLASH_NRF_MRAM_ONE_BYTE_WRITE_ACCESS */

#define WRITE_OFFSET(a) (a->ptr + a->offset)

/* Set to more than one to allow multiple contexts in case of parallel execution */
#define SUIT_MAX_FLASH_COMPONENTS 1

#define IS_COND_TRUE(c)  ((c) ? "True" : "False")

LOG_MODULE_REGISTER(suit_flash_sink, CONFIG_SUIT_LOG_LEVEL);

static suit_plat_err_t erase(void *ctx);
static suit_plat_err_t write(void *ctx, uint8_t *buf, size_t *size);
static suit_plat_err_t seek(void *ctx, size_t offset);
static suit_plat_err_t flush(void *ctx);
static suit_plat_err_t used_storage(void *ctx, size_t *size);
static suit_plat_err_t release(void *ctx);

struct nvm_area {
	uintptr_t na_start;
	size_t na_size;
	const struct device *na_fdev;
};
struct flash_ctx {
	size_t size_used;
	size_t offset;
	size_t offset_limit;
	uintptr_t ptr;
	const struct device *fdev;
	size_t flash_write_size;
	bool in_use;
};

static struct flash_ctx ctx[SUIT_MAX_FLASH_COMPONENTS];

/* List of nonvolatile memories accessible for flash_sink */
static const struct nvm_area nvm_area_map[] = {
	{
		.na_start = INTERNAL_NVM_START,
		.na_size = INTERNAL_NVM_SIZE,
		.na_fdev = INTERNAL_NVM_DEV,
	},
/* { Example how external nvm could be added
 *		.na_start = EXTERNAL_NVM_START,
 *		.na_size = EXTERNAL_NVM_SIZE,
 *		.na_fdev = EXTERNAL_NVM_DEV,
 * },
 */
};
const uint8_t nvm_area_map_size = ARRAY_SIZE(nvm_area_map);


/**
 * @brief Get the new, free ctx object
 *
 * @return struct flash_ctx* or NULL if no free ctx was found
 */
static struct flash_ctx *new_ctx_get()
{
	for (size_t i = 0; i < SUIT_MAX_FLASH_COMPONENTS; i++) {
		if (!ctx[i].in_use) {
			return &ctx[i];
		}
	}

	return NULL; /* No free ctx */
}

/**
 * @brief Register write by updating appropriate offsets and sizes
 *
 * @param flash_ctx Flash sink context pointer
 * @param write_size Size of written data
 * @return SUIT_PLAT_SUCCESS in case of success, otherwise error code
 */
static suit_plat_err_t register_write(struct flash_ctx *flash_ctx, size_t write_size)
{
	flash_ctx->offset += write_size;

	if (flash_ctx->offset > flash_ctx->size_used) {
		flash_ctx->size_used = flash_ctx->offset;
	}

	return SUIT_PLAT_SUCCESS;
}

/**
 * @brief Get the nvm area object corresponding to given address
 *
 * @param address Address to look for in registered areas
 * @return const struct flash_area* Pointer to flash area or NULL in case of fail
 */
static const struct nvm_area *nvm_area_get(uint8_t *address)
{
	size_t target_offset = suit_plat_mem_nvm_offset_get(address);

	for (size_t i = 0; i < nvm_area_map_size; i++) {
		LOG_DBG("nvm_area: start_address: 0x%lX,  size: 0x%X,  target offset: 0x%X", nvm_area_map[i].na_start, nvm_area_map[i].na_size, target_offset);

		if ((target_offset >= nvm_area_map[i].na_start) &&
			(target_offset < (nvm_area_map[i].na_start + nvm_area_map[i].na_size))) {
				return &nvm_area_map[i];
			}
	}

	LOG_DBG("nvm_area: Not found");
	return NULL;
}

/**
 * @brief Get the flash write size for given flash driver
 *
 * @param fdev Flash driver to get the size from
 * @return size_t Write block size retrieved from driver
 */
static size_t flash_write_size_get(const struct device *fdev)
{
	const struct flash_parameters *parameters = flash_get_parameters(fdev);

	return parameters->write_block_size;
}

bool suit_flash_sink_is_address_supported(uint8_t *address)
{
	if ((address == NULL) || (nvm_area_get(address) == NULL)) {
		LOG_INF("Failed to find nvm area corresponding to address: %p", address);
		return false;
	}

	return true;
}

suit_plat_err_t erase(void *ctx)
{
	if (ctx != NULL) {
		struct flash_ctx *flash_ctx = (struct flash_ctx *)ctx;
		size_t size = flash_ctx->offset_limit - (size_t)flash_ctx->ptr;

		LOG_DBG("flash_sink_init_mem size %u", size);

		/* Erase requested area in preparation for data. */
		int res = flash_erase(flash_ctx->fdev, flash_ctx->ptr, size);
		if (res != 0) {
			LOG_ERR("Failed to erase requested memory area: %i", res);
			return SUIT_PLAT_ERR_IO;
		}

		return SUIT_PLAT_SUCCESS;
	}

	return SUIT_PLAT_ERR_INVAL;
}

suit_plat_err_t suit_flash_sink_get(struct stream_sink *sink, uint8_t *dst, size_t size)
{
	if ((dst != NULL) && (size > 0) && (sink != NULL)) {
		struct flash_ctx *ctx = new_ctx_get();

		if (ctx != NULL) {
			LOG_DBG("flash_sink requested area: offset: 0x%lX; size: 0x%X",
				suit_plat_mem_nvm_offset_get(dst), size);

			const struct nvm_area *nvm_area = nvm_area_get(dst);

			if (nvm_area == NULL) {
				LOG_ERR("Failed to find nvm area corresponding to requested address.");
				return SUIT_PLAT_ERR_HW_NOT_READY;
			}

			if (!device_is_ready(nvm_area->na_fdev)) {
				LOG_ERR("Flash device not ready.");
				return SUIT_PLAT_ERR_HW_NOT_READY;
			}

			/* Check if requested area fits in found nvm */
			if ((suit_plat_mem_nvm_offset_get(dst) + size)
			    > (nvm_area->na_start + nvm_area->na_size)) {
				LOG_ERR("Requested memory area out of bounds of corresponding nvm");
				return SUIT_PLAT_ERR_OUT_OF_BOUNDS;
			}

			memset(ctx, 0, sizeof(*ctx));
			ctx->flash_write_size = flash_write_size_get(nvm_area->na_fdev);
			ctx->fdev = nvm_area->na_fdev;
			ctx->offset = 0;
			ctx->offset_limit = suit_plat_mem_nvm_offset_get(dst) + size; /* max address */
			ctx->size_used = 0;
			ctx->ptr = suit_plat_mem_nvm_offset_get(dst);
			ctx->in_use = true;

			if ((ctx->flash_write_size >
				(nvm_area->na_start + nvm_area->na_size)) ||
				(ctx->flash_write_size > SWAP_BUFFER_SIZE)) {

				memset(ctx, 0, sizeof(*ctx));
				LOG_ERR("Write block size exceeds set safety limits");
				return SUIT_PLAT_ERR_INVAL;
			}

			memset(sink, 0, sizeof(*sink));
			sink->erase = erase;
			sink->write = write;
			sink->seek = seek;
			sink->flush = flush;
			sink->used_storage = used_storage;
			sink->release = release;
			sink->ctx = ctx;

			return SUIT_PLAT_SUCCESS; /* SUCCESS */
		}

		LOG_ERR("ERROR - SUIT_MAX_FLASH_COMPONENTS reached.");
		return SUIT_PLAT_ERR_NO_RESOURCES;
	}

	LOG_ERR("%s: Invalid arguments: %s, %s, %s", __func__, IS_COND_TRUE(dst != NULL),
		IS_COND_TRUE(size > 0), IS_COND_TRUE(sink != NULL));
	return SUIT_PLAT_ERR_INVAL;
}

static suit_plat_err_t write_unaligned_start(struct flash_ctx *flash_ctx, size_t *size_left, uint8_t **buf)
{
	LOG_ERR("write_unaligned_start...start");

	uint8_t edit_buffer[SWAP_BUFFER_SIZE];

	size_t start_offset = 0;
	size_t block_start = 0;
	size_t write_size = 0;

	block_start = ((size_t)(WRITE_OFFSET(flash_ctx) / flash_ctx->flash_write_size)) * flash_ctx->flash_write_size;
	start_offset = WRITE_OFFSET(flash_ctx) - block_start;
	write_size = 0;

	LOG_ERR("write_unaligned)_start...1");
	if (flash_read(flash_ctx->fdev, block_start, edit_buffer, flash_ctx->flash_write_size) != 0) {
		LOG_ERR("Flash read failed.");
		return SUIT_PLAT_ERR_IO;
	}

	LOG_ERR("write_unaligned)_start...2");
	/* write_size - how much data from buf will be written */
	write_size = MIN(*size_left, flash_ctx->flash_write_size - start_offset);

	LOG_ERR("write_unaligned)_start...3");
	memcpy(edit_buffer + start_offset, *buf, write_size);

	LOG_ERR("write_unaligned)_start...4");
	/* Write back edit_buffer that now contains unaligned bytes from the start of buf */
	if (flash_write(flash_ctx->fdev, block_start, edit_buffer, flash_ctx->flash_write_size) != 0) {
		LOG_ERR("Writing initial unaligned data failed.");
		return SUIT_PLAT_ERR_IO;
	}

	/* Move offset for bytes written */
	suit_plat_err_t ret = register_write(flash_ctx, write_size);
	if (ret != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Failed to update size after write");
		return ret;
	}

	/* Move input buffer ptr */
	*buf += write_size;

	/* Decrease size by the number of bytes written */
	*size_left -= write_size;

	LOG_ERR("write_unaligned_start...end");
	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t write_aligned(struct flash_ctx *flash_ctx, size_t *size_left, uint8_t **buf,
				     size_t write_size)
{
	size_t block_start = 0;

	/* Write part that is aligned */
	block_start = ((size_t)(WRITE_OFFSET(flash_ctx) / flash_ctx->flash_write_size)) * flash_ctx->flash_write_size;

	if (flash_write(flash_ctx->fdev, block_start, *buf, write_size) != 0) {
		LOG_ERR("Writing aligned blocks failed.");
		return SUIT_PLAT_ERR_IO;
	}

	write_size = *size_left >= write_size ? write_size : *size_left;

	/* Move offset for bytes written */
	suit_plat_err_t ret = register_write(flash_ctx, write_size);
	if (ret != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Failed to update size after write");
		return ret;
	}

	/* Move input buffer ptr */
	*buf += write_size;

	/* Decrease size by the number of bytes written */
	*size_left -= write_size;
	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t write_remaining(struct flash_ctx *flash_ctx, size_t *size_left, uint8_t **buf)
{
	uint8_t edit_buffer[SWAP_BUFFER_SIZE];
	size_t block_start = 0;

	/* Write remaining data */
	block_start = ((size_t)(WRITE_OFFSET(flash_ctx) / flash_ctx->flash_write_size)) * flash_ctx->flash_write_size;

	if (flash_read(flash_ctx->fdev, block_start, edit_buffer, flash_ctx->flash_write_size) == 0) {
		memcpy(edit_buffer, *buf, *size_left);

		/* Write back edit_buffer that now contains unaligned bytes from the start of buf */
		if (flash_write(flash_ctx->fdev, block_start, edit_buffer, flash_ctx->flash_write_size) != 0) {
			LOG_ERR("Writing remaining unaligned data failed.");
			return SUIT_PLAT_ERR_IO;
		}

		/* Move offset for bytes written */
		suit_plat_err_t ret = register_write(flash_ctx, *size_left);
		if (ret != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Failed to update size after write");
		return ret;
	}

		return SUIT_PLAT_SUCCESS;
	}

	LOG_ERR("Flash read failed.");
	return SUIT_PLAT_ERR_IO;
}

static suit_plat_err_t write(void *ctx, uint8_t *buf, size_t *size)
{
	size_t size_left = *size;

	if ((ctx != NULL) && (buf != NULL) && (size_left > 0)) {
		struct flash_ctx *flash_ctx = (struct flash_ctx *)ctx;

		if (!flash_ctx->in_use) {
			LOG_ERR("flash_sink not initialized.");
			return SUIT_PLAT_ERR_INVAL;
		}

		if (flash_ctx->fdev == NULL) {
			LOG_ERR("%s: fdev is NULL.", __func__);
			return SUIT_PLAT_ERR_INVAL;
		}

		if ((flash_ctx->offset_limit - (size_t)flash_ctx->ptr) >= size_left) {
			if (flash_ctx->flash_write_size == 1) {
				int ret = flash_write(flash_ctx->fdev,
						      WRITE_OFFSET(flash_ctx),
						      buf, size_left);

				if (ret == 0) {
					ret = register_write(flash_ctx, size_left);
					if (ret != SUIT_PLAT_SUCCESS) {
						LOG_ERR("Failed to update size after write");
						return ret;
					}
				}
				else
				{
					ret = SUIT_PLAT_ERR_IO;
				}

				return ret;
			}

			size_t write_size = 0;
			suit_plat_err_t err = 0;

			if (WRITE_OFFSET(flash_ctx) % flash_ctx->flash_write_size) {
				/* Write offset is not aligned with start of block */
				err = write_unaligned_start(flash_ctx, &size_left, &buf);

				if (err != SUIT_PLAT_SUCCESS) {
					return err;
				}

				if (size_left == 0) {
					/* All data written */
					return SUIT_PLAT_SUCCESS;
				}
			}

			/* Number of bytes to be written in context of whole blocks */
			write_size = (size_left / flash_ctx->flash_write_size) * flash_ctx->flash_write_size;

			if (write_size > 0) {
				err = write_aligned(flash_ctx, &size_left, &buf, write_size);

				if (err != SUIT_PLAT_SUCCESS) {
					return err;
				}

				if (size_left == 0) {
					/* All data written */
					return SUIT_PLAT_SUCCESS;
				}
			}

			return write_remaining(flash_ctx, &size_left, &buf);

		} else {
			LOG_ERR("Write out of bounds.");
			return SUIT_PLAT_ERR_OUT_OF_BOUNDS;
		}
	}

	LOG_ERR("%s: Invalid arguments. %s, %s, %s", __func__,
	IS_COND_TRUE(ctx != NULL),
	IS_COND_TRUE(buf != NULL),
	IS_COND_TRUE(size_left > 0));
	return SUIT_PLAT_ERR_INVAL;
}

static suit_plat_err_t seek(void *ctx, size_t offset)
{
	if (ctx != NULL) {
		struct flash_ctx *flash_ctx = (struct flash_ctx *)ctx;

		if (offset < (flash_ctx->offset_limit - (size_t)flash_ctx->ptr)) {
			flash_ctx->offset = offset;
			return SUIT_PLAT_SUCCESS;
		}

		LOG_ERR("Invalid argument - offset value out of range");
		return SUIT_PLAT_ERR_INVAL;
	}

	LOG_ERR("%s: Invalid arguments - ctx is NULL", __func__);
	return SUIT_PLAT_ERR_INVAL;
}

static suit_plat_err_t flush(void *ctx)
{
	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t used_storage(void *ctx, size_t *size)
{
	if ((ctx != NULL) && (size != NULL)) {
		struct flash_ctx *flash_ctx = (struct flash_ctx *)ctx;

		*size = flash_ctx->size_used;

		return SUIT_PLAT_SUCCESS;
	}

	LOG_ERR("%s: Invalid arguments. %s, %s", __func__,
	IS_COND_TRUE(ctx != NULL),
	IS_COND_TRUE(size != NULL));
	return SUIT_PLAT_ERR_INVAL;
}

static suit_plat_err_t release(void *ctx)
{
	if (ctx != NULL) {
		struct flash_ctx *flash_ctx = (struct flash_ctx *)ctx;

		flash_ctx->offset = 0;
		flash_ctx->offset_limit = 0;
		flash_ctx->size_used = 0;
		flash_ctx->ptr = 0;
		flash_ctx->fdev = NULL;
		flash_ctx->in_use = false;

		return SUIT_PLAT_SUCCESS;
	}

	LOG_ERR("%s: Invalid arguments - ctx is NULL", __func__);
	return SUIT_PLAT_ERR_INVAL;
}
