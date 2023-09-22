/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <suit_platform_internal.h>

#ifdef CONFIG_SUIT_STREAM
#include <sink.h>
#include <sink_selector.h>
#endif /* CONFIG_SUIT_STREAM */

#ifdef CONFIG_SUIT_STREAM_SOURCE_CACHE
#include <cache_streamer.h>
#endif /* CONFIG_SUIT_STREAM_SOURCE_CACHE */
#ifdef CONFIG_SUIT_STREAM_SOURCE_MEMPTR
#include <memptr_streamer.h>
#endif /* CONFIG_SUIT_STREAM_SOURCE_MEMPTR */

#include <stdbool.h>
#include <suit_platform.h>
#include <suit_memptr_storage.h>

LOG_MODULE_REGISTER(suit_plat_fetch, CONFIG_SUIT_LOG_LEVEL);

int suit_plat_check_fetch(suit_component_t dst_handle, struct zcbor_string *uri)
{
#ifdef CONFIG_SUIT_STREAM_SOURCE_CACHE
	struct stream_sink sink;

	int ret = select_sink(dst_handle, &sink);

	if (ret != SUCCESS) {
		return ret;
	}

	if (sink.release != NULL) {
		int err = sink.release(&sink.ctx);

		if (err != SUCCESS) {
			LOG_ERR("sink release failed: %i", err);
			return err;
		}
	}

	return SUIT_SUCCESS;
#elif CONFIG_SUIT_STREAM_SOURCE_MEMPTR /* CONFIG_SUIT_STREAM_SOURCE_CACHE */
	return SUIT_SUCCESS;
#else  /* CONFIG_SUIT_STREAM_SOURCE_MEMPTR || CONFIG_SUIT_STREAM_SOURCE_CACHE */
	return SUIT_ERR_UNSUPPORTED_COMMAND;
#endif /* CONFIG_SUIT_STREAM_SOURCE_MEMPTR || CONFIG_SUIT_STREAM_SOURCE_CACHE */
}

int suit_plat_fetch(suit_component_t dst_handle, struct zcbor_string *uri)
{
#ifdef CONFIG_SUIT_STREAM_SOURCE_CACHE
	struct stream_sink sink;

	int ret = select_sink(dst_handle, &sink);

	if (ret != SUCCESS) {
		return ret;
	}

	ret = cache_streamer(uri->value, uri->len, &sink);

	if (sink.release != NULL) {
		int err = sink.release(&sink.ctx);

		if (err != SUCCESS) {
			LOG_ERR("sink release failed: %i", err);
			return err;
		}
	}

	return ret;
#else  /* CONFIG_SUIT_STREAM_SOURCE_CACHE */
	return SUIT_ERR_UNSUPPORTED_COMMAND;
#endif /* CONFIG_SUIT_STREAM_SOURCE_CACHE */
}

int suit_plat_check_fetch_integrated(suit_component_t dst_handle, struct zcbor_string *payload)
{
#ifdef CONFIG_SUIT_STREAM_SOURCE_MEMPTR
	struct stream_sink sink;

	int ret = select_sink(dst_handle, &sink);

	if (ret != SUCCESS) {
		return ret;
	}

	if (sink.release != NULL) {
		int err = sink.release(sink.ctx);

		if (err != SUCCESS) {
			LOG_ERR("sink release failed: %i", err);
			return err;
		}
	}

	return SUIT_SUCCESS;
#else  /* CONFIG_SUIT_STREAM_SOURCE_MEMPTR */
	return SUIT_ERR_UNSUPPORTED_COMMAND;
#endif /* CONFIG_SUIT_STREAM_SOURCE_MEMPTR */
}

int suit_plat_fetch_integrated(suit_component_t dst_handle, struct zcbor_string *payload)
{
#ifdef CONFIG_SUIT_STREAM_SOURCE_MEMPTR
	struct stream_sink sink;

	int ret = select_sink(dst_handle, &sink);

	if (ret != SUCCESS) {
		return ret;
	}

	ret = memptr_streamer(payload->value, payload->len, &sink);

	if (sink.release != NULL) {
		int err = sink.release(sink.ctx);

		if (err != SUCCESS) {
			LOG_ERR("sink release failed: %i", err);
			return err;
		}
	}

	return ret;
#else  /* CONFIG_SUIT_STREAM_SOURCE_MEMPTR */
	return SUIT_ERR_UNSUPPORTED_COMMAND;
#endif /* CONFIG_SUIT_STREAM_SOURCE_MEMPTR */
}
