/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <suit_platform_internal.h>
#include <suit_plat_digest_cache.h>
#include <suit_plat_error_convert.h>

#ifdef CONFIG_SUIT_STREAM
#include <sink.h>
#include <sink_selector.h>
#endif /* CONFIG_SUIT_STREAM */

#ifdef CONFIG_SUIT_STREAM_SOURCE_CACHE
#include <dfu_cache_streamer.h>
#endif /* CONFIG_SUIT_STREAM_SOURCE_CACHE */
#ifdef CONFIG_SUIT_STREAM_SOURCE_MEMPTR
#include <memptr_streamer.h>
#include <memptr_sink.h>
#endif /* CONFIG_SUIT_STREAM_SOURCE_MEMPTR */

#include <stdbool.h>
#include <suit_platform.h>
#include <suit_memptr_storage.h>
#include <suit_plat_memptr_size_update.h>

LOG_MODULE_REGISTER(suit_plat_fetch_sdfw, CONFIG_SUIT_LOG_LEVEL);

#ifdef CONFIG_SUIT_STREAM
static suit_plat_err_t release_sink(struct stream_sink *sink)
{
	if (sink != NULL) {
		if (sink->release != NULL) {
			suit_plat_err_t err = sink->release(sink->ctx);

			if (err != SUIT_PLAT_SUCCESS) {
				LOG_ERR("sink release failed.");
			}

			return err;
		}

		return SUIT_PLAT_SUCCESS;
	}

	return SUIT_PLAT_ERR_INVAL;
}

/**
 * @brief Function checks if component type is supported by SDFW fetch command
 *
 * @param component_type Component type be verified
 * @return true If supported
 * @return false If unsupported
 */
static bool is_type_supported(suit_component_type_t component_type)
{
	if ((component_type != SUIT_COMPONENT_TYPE_CAND_IMG) &&
	    (component_type != SUIT_COMPONENT_TYPE_CAND_MFST) &&
	    (component_type != SUIT_COMPONENT_TYPE_SOC_SPEC) &&
	    (component_type != SUIT_COMPONENT_TYPE_MEM)) {
		return false;
	}

	return true;
}

/**
 * @brief Function checks if component type is supported by ipc streaming service
 *
 * @param component_type Component type be verified
 * @return true If supported
 * @return false If unsupported
 */
static bool is_type_supported_by_ipc(suit_component_type_t component_type)
{
	if ((component_type != SUIT_COMPONENT_TYPE_CAND_IMG) &&
	    (component_type != SUIT_COMPONENT_TYPE_CAND_MFST) &&
	    (component_type != SUIT_COMPONENT_TYPE_MEM)) {
		return false;
	}

	return true;
}
#endif /* CONFIG_SUIT_STREAM */

int suit_plat_check_fetch(suit_component_t dst_handle, struct zcbor_string *uri)
{
#ifdef CONFIG_SUIT_STREAM
	suit_component_type_t dst_component_type = SUIT_COMPONENT_TYPE_UNSUPPORTED;
	struct stream_sink dst_sink;

	/* Get component type based on component handle*/
	int ret = suit_plat_component_type_get(dst_handle, &dst_component_type);
	if (ret != SUIT_SUCCESS) {
		LOG_ERR("Failed to decode component type: %i", ret);
		return ret;
	}

	if (!is_type_supported(dst_component_type)) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	/* Get dst_sink - final destination sink */
	ret = suit_sink_select(dst_handle, &dst_sink);
	if (ret != SUIT_SUCCESS) {
		LOG_ERR("Selecting sink failed: %i", ret);
		return ret;
	}

	ret = release_sink(&dst_sink);
	if (ret != SUIT_PLAT_SUCCESS) {
		return suit_plat_err_to_processor_err_convert(ret);
	}

	if (IS_ENABLED(CONFIG_SUIT_STREAM_SOURCE_CACHE) ||
	    (IS_ENABLED(CONFIG_SUIT_STREAM_IPC_REQUESTOR) &&
	     is_type_supported_by_ipc(dst_component_type))) {
		return SUIT_SUCCESS;
	}
#endif /* CONFIG_SUIT_STREAM */

	return SUIT_ERR_UNSUPPORTED_COMMAND;
}

int suit_plat_fetch(suit_component_t dst_handle, struct zcbor_string *uri)
{
#ifdef CONFIG_SUIT_STREAM
	struct stream_sink dst_sink;
	suit_component_type_t dst_component_type = SUIT_COMPONENT_TYPE_UNSUPPORTED;

	/* Get component type based on component handle*/
	int ret = suit_plat_component_type_get(dst_handle, &dst_component_type);
	if (ret != SUIT_SUCCESS) {
		LOG_ERR("Failed to decode component type: %i", ret);
		return ret;
	}

	if (!is_type_supported(dst_component_type)) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	if (!IS_ENABLED(CONFIG_SUIT_STREAM_IPC_REQUESTOR) &&
	    !IS_ENABLED(CONFIG_SUIT_STREAM_SOURCE_CACHE)) {
		return SUIT_ERR_UNSUPPORTED_COMMAND;
	}

	/* Invalidate the cache entry of the digest for the destination. */
#if CONFIG_SUIT_DIGEST_CACHE
	(void) suit_plat_digest_cache_remove_by_handle(dst_handle);
#endif

	/* Get dst_sink - final destination sink */
	ret = suit_sink_select(dst_handle, &dst_sink);
	if (ret != SUIT_SUCCESS) {
		LOG_ERR("Selecting sink failed: %i", ret);
		return ret;
	}

	/* Here other parts of pipe will be instantiated.
	 *	Like decryption and/or decompression sinks.
	 */

	if (dst_sink.erase != NULL) {
		ret = dst_sink.erase(dst_sink.ctx);
		if (ret) {
			LOG_ERR("Sink mem erase failed: %i", ret);
			return ret;
		}
	}

	/* If cache is disabled, act as thou uri was not found in cache */
	ret = SUIT_PLAT_ERR_NOT_FOUND;

#ifdef CONFIG_SUIT_STREAM_SOURCE_CACHE
	/* Check if requested uri exists in cache and get streamer */
	ret = suit_dfu_cache_streamer_stream(uri->value, uri->len, &dst_sink);
#endif /* CONFIG_SUIT_STREAM_SOURCE_CACHE */

#ifdef CONFIG_SUIT_STREAM_IPC_REQUESTOR
	if ((ret == SUIT_PLAT_ERR_NOT_FOUND) &&		/* URI was not found in cache */
	    is_type_supported_by_ipc(dst_component_type)) { /* component type is supported */
		/* Request uri through ipc streamer */
		ret = suit_ipc_streamer_stream(uri->value, uri->len, &dst_sink,
					       CONFIG_SUIT_STREAM_IPC_STREAMER_CHUNK_TIMEOUT,
					       CONFIG_SUIT_STREAM_IPC_STREAMER_REQUESTING_PERIOD);
	}
#endif /* CONFIG_SUIT_STREAM_IPC_REQUESTOR */

	if (ret == SUIT_PLAT_SUCCESS) {
		/* Update size in memptr for MEM component */
		if (dst_component_type == SUIT_COMPONENT_TYPE_MEM) {
			size_t new_size = 0;

			ret = dst_sink.used_storage(dst_sink.ctx, &new_size);
			if (ret != SUIT_PLAT_SUCCESS) {
				LOG_ERR("Getting used storage on destination sink failed");
				release_sink(&dst_sink);

				return suit_plat_err_to_processor_err_convert(ret);
			}

			ret = suit_plat_memptr_size_update(dst_handle, new_size);
			if (ret != SUIT_PLAT_SUCCESS) {
				LOG_ERR("Failed to update destination MEM component size: %i", ret);
				release_sink(&dst_sink);

				return ret;
			}
		}

		ret = release_sink(&dst_sink);
		return suit_plat_err_to_processor_err_convert(ret);
	}

	release_sink(&dst_sink);
	return suit_plat_err_to_processor_err_convert(ret);
#else
	return SUIT_ERR_UNSUPPORTED_COMMAND;
#endif /* CONFIG_SUIT_STREAM */
}

int suit_plat_check_fetch_integrated(suit_component_t dst_handle, struct zcbor_string *payload)
{
#ifdef CONFIG_SUIT_STREAM
	struct stream_sink dst_sink;
	suit_component_type_t dst_component_type = SUIT_COMPONENT_TYPE_UNSUPPORTED;

	/* Get component type based on component handle*/
	int ret = suit_plat_component_type_get(dst_handle, &dst_component_type);
	if (ret != SUIT_SUCCESS) {
		LOG_ERR("Failed to decode component type: %i", ret);
		return ret;
	}

	if (!is_type_supported(dst_component_type)) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

#ifndef CONFIG_SUIT_STREAM_SOURCE_MEMPTR
	return SUIT_ERR_UNSUPPORTED_COMMAND;
#endif /* CONFIG_SUIT_STREAM_SOURCE_MEMPTR */

	/* Get dst_sink - final destination sink */
	ret = suit_sink_select(dst_handle, &dst_sink);
	if (ret != SUIT_SUCCESS) {
		LOG_ERR("Selecting sink failed: %i", ret);
		return ret;
	}

	/* Here other parts of pipe will be instantiated.
	 *	Like decryption and/or decompression sinks.
	 */

	ret = release_sink(&dst_sink);
	return suit_plat_err_to_processor_err_convert(ret);
#else
	return SUIT_ERR_UNSUPPORTED_COMMAND;
#endif /* CONFIG_SUIT_STREAM */
}

int suit_plat_fetch_integrated(suit_component_t dst_handle, struct zcbor_string *payload)
{
#ifdef CONFIG_SUIT_STREAM
	struct stream_sink dst_sink;
	suit_component_type_t dst_component_type = SUIT_COMPONENT_TYPE_UNSUPPORTED;

	/* Get component type based on component handle*/
	int ret = suit_plat_component_type_get(dst_handle, &dst_component_type);
	if (ret != SUIT_SUCCESS) {
		LOG_ERR("Failed to decode component type: %i", ret);
		return ret;
	}

	if (!is_type_supported(dst_component_type)) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

#ifndef CONFIG_SUIT_STREAM_SOURCE_MEMPTR
	return SUIT_ERR_UNSUPPORTED_COMMAND;
#endif /* CONFIG_SUIT_STREAM_SOURCE_MEMPTR */

	/* Get dst_sink - final destination sink */
	ret = suit_sink_select(dst_handle, &dst_sink);
	if (ret != SUIT_SUCCESS) {
		LOG_ERR("Selecting sink failed: %i", ret);
		return ret;
	}

	/* Here other parts of pipe will be instantiated.
	 *	Like decryption and/or decompression sinks.
	 */

	if (dst_sink.erase != NULL) {
		ret = dst_sink.erase(dst_sink.ctx);

		if (ret != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Sink mem erase failed, err code: %d", ret);
			return suit_plat_err_to_processor_err_convert(ret);
		}
	}

	/* Invalidate the cache entry of the digest for the destination. */
#if CONFIG_SUIT_DIGEST_CACHE
	(void) suit_plat_digest_cache_remove_by_handle(dst_handle);
#endif

#ifdef CONFIG_SUIT_STREAM_SOURCE_MEMPTR
	ret = suit_memptr_streamer_stream(payload->value, payload->len, &dst_sink);
#endif /* CONFIG_SUIT_STREAM_SOURCE_MEMPTR */

	if (ret == SUIT_PLAT_SUCCESS) {
		/* Update size in memptr for MEM component */
		if (dst_component_type == SUIT_COMPONENT_TYPE_MEM) {
			size_t new_size = 0;

			ret = dst_sink.used_storage(dst_sink.ctx, &new_size);
			if (ret != SUIT_PLAT_SUCCESS) {
				LOG_ERR("Getting used storage on destination sink failed");
				release_sink(&dst_sink);

				return suit_plat_err_to_processor_err_convert(ret);
			}

			ret = suit_plat_memptr_size_update(dst_handle, new_size);
			if (ret != SUIT_PLAT_SUCCESS) {
				LOG_ERR("Failed to update destination MEM component size: %i", ret);
				release_sink(&dst_sink);

				return ret;
			}
		}

		ret = release_sink(&dst_sink);
		return suit_plat_err_to_processor_err_convert(ret);
	}

	release_sink(&dst_sink);
	return suit_plat_err_to_processor_err_convert(ret);
#else  /* CONFIG_SUIT_STREAM */
	return SUIT_ERR_UNSUPPORTED_COMMAND;
#endif /* CONFIG_SUIT_STREAM */
}
