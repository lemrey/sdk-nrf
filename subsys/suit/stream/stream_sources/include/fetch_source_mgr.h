/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef FETCH_SOURCE_MGR_H__
#define FETCH_SOURCE_MGR_H__

#include <sink.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Fetch source manager allows for registration of multiple data sources. When fetch_source_stream
 * is called, fetch source manager iterates over registered fetch sources, calling respective
 * fetch_source_mgr_fetch_request_fn. Iteration will be broken if one of registered fetch sources
 * start to call any of stream_sink function pointers - see fetch_source_mgr_fetch_request_fn
 */

/**
 * @brief Fetch source callback function.
 *
 * @param[in]   resource_id		Resource identifier, typically in form of URI. Fetch source
 *interprets it. In case if fetch source is unable to retrieve requested resource, i.e. due to
 *unsupported protocol, it shall just fail, avoiding any prior call to function pointers provided in
 *sink.
 *
 * @param[in]   resource_id_length	Length of resource_id, in bytes
 *
 * @param[in]   sink			Function pointers to pass image chunk to next chunk
 *processing element. Non-'null' write_ptr is always required. Non-'null' seek_ptr may be required
 *by selected fetch sources.
 *
 * @return SUIT_PLAT_SUCCESS on success, error code otherwise
 */
typedef suit_plat_err_t (*fetch_source_mgr_fetch_request_fn)(const uint8_t *uri, size_t uri_length,
						 struct stream_sink *sink);

/**
 * @brief Registers fetch source
 *
 * @param[in]   request_fn		Fetch source callback function. Fetch source manager will
 *iterate over registered callback functions as result of fetch_source_stream call. Attention! Order
 *of execution of callback functions is not guaranteed!
 *
 *
 * @return SUIT_PLAT_SUCCESS on success, error code otherwise
 */

suit_plat_err_t fetch_source_register(fetch_source_mgr_fetch_request_fn request_fn);

/**
 * @brief Streams an image from source to sink
 *
 * @param[in]   resource_id		Resource identifier, typically in form of URI.
 *
 * @param[in]   resource_id_length	Length of resource_id, in bytes
 *
 * @param[in]   sink			Function pointers to pass image chunk to next chunk
 *processing element. Non-'null' write_ptr is always required. Non-'null' seek_ptr may be required
 *by selected fetch sources
 *
 * @return SUIT_PLAT_SUCCESS on success, error code otherwise
 */
suit_plat_err_t fetch_source_stream(const uint8_t *resource_id, size_t resource_id_length,
			struct stream_sink *sink);

#ifdef __cplusplus
}
#endif

#endif /* FETCH_SOURCE_MGR_H__ */
