/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef FLASH_SINK_H__
#define FLASH_SINK_H__

#include <sink.h>
#include <suit_memptr_storage.h>

/**
 * @brief Get the flash_sink object
 *
 * @param sink Pointer to sink_stream to be filled
 * @param dst Destination address - start of write area
 * @param size Write area size
 * @param handle Handle to storage object
 * @return int 0 if success otherwise error code.
 */
int flash_sink_get(struct stream_sink *sink, uint8_t *dst, size_t size, memptr_storage_handle handle);

#endif /* FLASH_SINK_H__ */
