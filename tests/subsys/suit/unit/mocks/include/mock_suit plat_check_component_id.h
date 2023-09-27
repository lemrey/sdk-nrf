/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOCK_SUIT_PLAT_CHECK_COMPONENT_ID_H__
#define MOCK_SUIT_PLAT_CHECK_COMPONENT_ID_H__

#include <zephyr/fff.h>
#include <mock_globals.h>

#include <suit_plat_check_component_id.h>

FAKE_VALUE_FUNC(int, suit_plat_check_component_id, suit_manifest_class_id_t *, struct zcbor_string *);

static inline void mock_suit_plat_check_component_id_reset(void)
{
	RESET_FAKE(suit_plat_check_component_id);
}

#endif /* MOCK_SUIT_PLAT_CHECK_COMPONENT_ID_H__ */
