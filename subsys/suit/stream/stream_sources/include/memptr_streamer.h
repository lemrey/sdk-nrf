/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MEMPTR_STREAMER_H__
#define MEMPTR_STREAMER_H__

#include <sink.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Stream payload from pointer to sink
 *
 * @param payload Pointer to payload - source
 * @param payload_size Size of payload
 * @param sink Pointer to sink that will write payload - target
 * @return SUIT_PLAT_SUCCESS if success otherwise error code
 */
suit_plat_err_t memptr_streamer(const uint8_t *payload, size_t payload_size,
				struct stream_sink *sink);

#ifdef __cplusplus
}
#endif

#endif /* MEMPTR_STREAMER_H__ */
