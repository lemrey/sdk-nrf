/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <cache_streamer.h>
#include <suit_cache.h>
#include <zcbor_decode.h>

suit_plat_err_t cache_streamer(const uint8_t *uri, size_t uri_size,
							   struct stream_sink *sink)
{
	if ((uri != NULL) && (sink != NULL) && (sink->write != NULL) && (uri_size > 0)) {
		int err = 0;
		struct zcbor_string payload;
		struct zcbor_string uri_tmp = {
			.value = uri,
			.len = uri_size,
		};

		// TODO: convert error code when err codes in cache are refactored
		err = suit_cache_search(&uri_tmp, &payload);

		if (err == SUIT_PLAT_SUCCESS) {
			return sink->write(sink->ctx, (uint8_t *)payload.value, &payload.len);
		}

		return SUIT_PLAT_ERR_NOT_FOUND;
	}

	return SUIT_PLAT_ERR_INVAL;
}
