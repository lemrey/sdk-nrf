/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CACHE_SINK_H__
#define CACHE_SINK_H__

#include <sink.h>

/**
 * @brief Get cache sink object
 *
 * @param sink Pointer to sink_stream to be filled
 * @param cache_partition_id SUIT cache partition index (suit_cache_2 => id == 2)
 * @param uri Pointer to URI that will be used as a key in cache
 * @param uri_size Size of URI
 * @return int 0 if success otherwise error code
 */
int dfu_get_cache_sink(struct stream_sink *sink, uint8_t cache_partition_id, const uint8_t *uri,
		       size_t uri_size);

/**
 * @brief Commits written data to cache
 *
 * @param ctx Pointer to sink context
 * @return int 0 in case of success, otherwise error code
 */
int suit_cache_sink_commit(void *ctx);

/**
 * @brief Created slot is removed along with any data written to it.
 *
 * @param ctx Pointer to sink context
 * @return int 0 in case of success, otherwise error code
 */
int suit_cache_sink_drop(void *ctx);

/* No release for cache */
#endif /* CACHE_SINK_H__ */
