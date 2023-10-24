/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef RAM_SINK_H__
#define RAM_SINK_H__

#include <sink.h>

/**
 * @brief Get the ram_sink object
 *
 * @param sink Pointer to sink_stream to be filled
 * @param dst Destination address - start of write area
 * @param size Write area size
 * @return int 0 if success otherwise error code.
 */
int ram_sink_get(struct stream_sink *sink, uint8_t *dst, size_t size);

#endif /* RAM_SINK_H__ */
