/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <stdbool.h>
#include <suit_platform.h>
#include <suit_plat_decode_util.h>
#include <suit_plat_error_convert.h>
#include <suit_platform_internal.h>
#include <suit_plat_digest_cache.h>

#ifdef CONFIG_SUIT_STREAM
#include <sink.h>
#include <sink_selector.h>
#endif /* CONFIG_SUIT_STREAM */

#ifdef CONFIG_SUIT_STREAM_SOURCE_MEMPTR
#include <memptr_streamer.h>
#include <suit_memptr_storage.h>
#endif /* CONFIG_SUIT_STREAM_SOURCE_MEMPTR */

LOG_MODULE_REGISTER(suit_plat_copy, CONFIG_SUIT_LOG_LEVEL);

#ifdef CONFIG_SUIT_STREAM
static int release_sink(struct stream_sink *sink)
{
	if (sink != NULL) {
		if (sink->release != NULL) {
			int err = sink->release(sink->ctx);

			if (err != SUCCESS) {
				LOG_ERR("sink release failed.");
			}

			return err;
		}

		return SUCCESS;
	}

	return INVALID_ARGUMENT;
}
#endif /* CONFIG_SUIT_STREAM */

int suit_plat_check_copy(suit_component_t dst_handle, suit_component_t src_handle)
{
#ifdef CONFIG_SUIT_STREAM
	struct stream_sink dst_sink;
	struct zcbor_string *component_id;
	suit_component_type_t component_type = SUIT_COMPONENT_TYPE_UNSUPPORTED;

	int ret = select_sink(dst_handle, &dst_sink);

	if (ret != SUCCESS) {
		LOG_ERR("select_sink failed - error %i", ret);
		return ret;
	}

	ret = suit_plat_component_id_get(src_handle, &component_id);

	if (ret != SUIT_SUCCESS) {
		LOG_ERR("suit_plat_component_id_get failed: %i", ret);
		release_sink(&dst_sink);

		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	if (!suit_plat_decode_component_type(component_id, &component_type)) {
		LOG_ERR("suit_plat_decode_component_type failed");
		release_sink(&dst_sink);

		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	/* Select dst_sink based on component type */
	switch (component_type) {
#ifdef CONFIG_SUIT_STREAM_SOURCE_MEMPTR
	case SUIT_COMPONENT_TYPE_CAND_IMG:
		/* memptr */
		ret = SUIT_SUCCESS;
		break;
#endif /* CONFIG_SUIT_STREAM_SOURCE_MEMPTR */

	default:
		ret = SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
		break;
	}

	release_sink(&dst_sink);
	return ret;
#else  /* CONFIG_SUIT_STREAM */
	return SUIT_ERR_UNSUPPORTED_COMMAND;
#endif /* CONFIG_SUIT_STREAM */
}

int suit_plat_copy(suit_component_t dst_handle, suit_component_t src_handle)
{
#ifdef CONFIG_SUIT_STREAM
	struct stream_sink dst_sink;
#ifdef CONFIG_SUIT_STREAM_SOURCE_MEMPTR
	uint8_t *payload_ptr;
	size_t payload_size;
#endif /* CONFIG_SUIT_STREAM_SOURCE_MEMPTR */
	struct zcbor_string *component_id;
	suit_component_type_t component_type = SUIT_COMPONENT_TYPE_UNSUPPORTED;

	int ret = select_sink(dst_handle, &dst_sink);

	if (ret != SUCCESS) {
		LOG_ERR("select_sink failed - error %i", ret);
		return ret;
	}

	if (dst_sink.erase != NULL) {
		ret = dst_sink.erase(dst_sink.ctx);

		if (ret) {
			LOG_ERR("Sink mem erase failed");
			return ret;
		}
	}

	ret = suit_plat_component_id_get(src_handle, &component_id);

	if (ret != SUIT_SUCCESS) {
		LOG_ERR("suit_plat_component_id_get failed: %i", ret);
		release_sink(&dst_sink);

		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	if (!suit_plat_decode_component_type(component_id, &component_type)) {
		LOG_ERR("suit_plat_decode_component_type failed");
		release_sink(&dst_sink);

		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

#if CONFIG_SUIT_DIGEST_CACHE
	// Invalidate the cache entry of the digest for the destination.
	suit_plat_digest_cache_unlock();
	(void) suit_plat_digest_cache_remove_by_handle(dst_handle);
	suit_plat_digest_cache_lock();
#endif

	/* Select source based on component type */
	switch (component_type) {
#ifdef CONFIG_SUIT_STREAM_SOURCE_MEMPTR
	case SUIT_COMPONENT_TYPE_CAND_IMG: { /* memptr */
		memptr_storage_handle handle = NULL;
		ret = suit_plat_component_impl_data_get(src_handle, &handle);

		if (ret != SUCCESS) {
			LOG_ERR("suit_plat_component_impl_data_get failed - error %i", ret);

			release_sink(&dst_sink);
			return ret;
		}

		ret = get_memptr_ptr(handle, &payload_ptr, &payload_size);

		if ((ret != SUIT_PLAT_SUCCESS) && (payload_ptr != NULL) && (payload_size > 0)) {
			LOG_ERR("get_memptr_ptr failed - error %i", ret);

			release_sink(&dst_sink);
			return suit_plat_err_to_proccessor_err_convert(ret);
		}

		ret = memptr_streamer(payload_ptr, payload_size, &dst_sink);

		if (ret != SUCCESS) {
			LOG_ERR("memptr_streamer failed - error %i", ret);
		}
	} break;
#endif /* CONFIG_SUIT_STREAM_SOURCE_MEMPTR */

	default:
		ret = SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
		break;
	}

	release_sink(&dst_sink);

	return ret;
#else  /* CONFIG_SUIT_STREAM */
	return SUIT_ERR_UNSUPPORTED_COMMAND;
#endif /* CONFIG_SUIT_STREAM */
}
