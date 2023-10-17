/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <memptr_sink.h>

LOG_MODULE_REGISTER(suit_memptr_sink, CONFIG_SUIT_LOG_LEVEL);

static int write(void *ctx, uint8_t *buf, size_t *size);
static int used_storage(void *ctx, size_t *size);

int memptr_sink_get(struct stream_sink *sink, memptr_storage_handle handle)
{
	if ((sink != NULL) && (handle != NULL)) {
		sink->erase = NULL;
		sink->write = write;
		sink->seek = NULL;
		sink->flush = NULL;
		sink->used_storage = used_storage;
		sink->release = NULL;
		sink->ctx = handle;

		return SUCCESS;
	}

	LOG_ERR("Invalid argument.");
	return INVALID_ARGUMENT;
};

static int write(void *ctx, uint8_t *buf, size_t *size)
{
	if ((ctx != NULL) && (buf != NULL) && (size != NULL) && (*size != 0)) {
		return store_memptr_ptr(ctx, buf, *size);
	}

	LOG_ERR("Invalid argument.");
	return INVALID_ARGUMENT;
}

static int used_storage(void *ctx, size_t *size)
{
	if ((ctx != NULL) && (size != NULL)) {
		uint8_t *payload_ptr;
		size_t payload_size;

		if (get_memptr_ptr(ctx, &payload_ptr, &payload_size) == SUCCESS) {
			if (payload_ptr != NULL) {
				*size = payload_size;
			} else {
				*size = 0;
			}

			return SUCCESS;
		}

		LOG_ERR("Storage get failed.");
		return STORAGE_GET_FAILED;
	}

	LOG_ERR("Invalid argument.");
	return INVALID_ARGUMENT;
}
