/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MEMPTR_SINK_H__
#define MEMPTR_SINK_H__

#include <sink.h>
#include <suit_memptr_storage.h>

/**
 * @brief Get the memptr sink object
 *
 * @param sink Pointer to sink_stream to be filled
 * @param handle Handle to storage object
 * @return int 0 if success otherwise error code.
 */
int memptr_sink_get(struct stream_sink *sink, memptr_storage_handle handle);

#endif /* MEMPTR_SINK_H__ */
