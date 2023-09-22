/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MEMPTR_STREAMER_H__
#define MEMPTR_STREAMER_H__

#include <sink.h>

/**
 * @brief Stream payload from pointer to sink
 *
 * @param payload Pointer to payload - source
 * @param payload_size Size of payload
 * @param sink Pointer to sink that will write payload - target
 * @return int 0 if success otherwise error code
 */
int memptr_streamer(const uint8_t *payload, size_t payload_size, struct stream_sink *sink);

#endif /* MEMPTR_STREAMER_H__ */
