/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SINK_SELECTOR_H__
#define SINK_SELECTOR_H__

#include <sink.h>
#include <suit_types.h>

int select_sink(suit_component_t dst_handle, struct stream_sink *sink);

#endif /* SINK_SELECTOR_H__ */
