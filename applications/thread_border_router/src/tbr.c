/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Miscellaneous nRF Thread Border Router related functions
 */

#include "tbr.h"

static struct tbr_context context;

struct tbr_context *tbr_get_context(void)
{
	return &context;
}
