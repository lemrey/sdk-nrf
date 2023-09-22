/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SDFW_SINK_H__
#define SDFW_SINK_H__

#include <sink.h>

/**
 * @brief Get the sdfw_sink object
 *
 * @param sink Pointer to sink_stream to be filled
 * @return int 0 if success, error code otherwise
 */
int sdfw_sink_get(struct stream_sink *sink);

#endif /* SDFW_SINK_H__ */
