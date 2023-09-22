/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <memptr_streamer.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(suit_memptr_streamer, CONFIG_SUIT_LOG_LEVEL);

int memptr_streamer(const uint8_t *payload, size_t payload_size, struct stream_sink *sink)
{
	if ((payload != NULL) && (sink != NULL) && (sink->write != NULL) && (payload_size > 0)) {
		return sink->write(sink->ctx, (uint8_t *)payload, &payload_size);
	}

	LOG_ERR("payload is %s", payload == NULL ? "NULL" : "OK");
	LOG_ERR("sink is %s", sink == NULL ? "NULL" : "OK");

	if (sink != NULL) {
		LOG_ERR("sink->write is %s", sink->write == NULL ? "NULL" : "OK");
	}

	LOG_ERR("payload_size: %u", payload_size);

	return INVALID_ARGUMENT;
}
