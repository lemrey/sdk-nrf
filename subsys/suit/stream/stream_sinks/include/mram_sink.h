/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MRAM_SINK_H__
#define MRAM_SINK_H__

#include <sink.h>

/**
 * @brief Get the mram_sink object
 *
 * @param sink Pointer to sink_stream to be filled
 * @param dst Destination address - start of write area
 * @param size Write area size
 * @return int 0 if success otherwise error code.
 */
int get_mram_sink(struct stream_sink *sink, uint8_t *dst, size_t size);

#endif /* MRAM_SINK_H__ */
