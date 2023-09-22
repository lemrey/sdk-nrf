/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOCK_H__
#define MOCK_H__

#include <zephyr/fff.h>
#include <mock_globals.h>

#ifdef CONFIG_MOCK_SUIT_PROCESSOR
#include <mock_suit.h>
#endif /* CONFIG_MOCK_SUIT_PROCESSOR */

#ifdef CONFIG_MOCK_SUIT_MCI
#include <mock_suit_mci.h>
#endif /* CONFIG_MOCK_SUIT_MCI */

#ifdef CONFIG_MOCK_SUIT_STORAGE
#include <mock_suit_storage.h>
#endif /* CONFIG_MOCK_SUIT_STORAGE */

#ifdef CONFIG_MOCK_SUIT_PLAT_DECODE_UTIL
#include <mock_suit_plat_decode_util.h>
#endif /* CONFIG_MOCK_SUIT_PLAT_DECODE_UTIL */

static inline void mocks_reset(void)
{
#ifdef CONFIG_MOCK_SUIT_PROCESSOR
	mock_suit_processor_reset();
#endif /* CONFIG_MOCK_SUIT_PROCESSOR */

#ifdef CONFIG_MOCK_SUIT_MCI
	mock_suit_mci_reset();
#endif /* CONFIG_MOCK_SUIT_MCI */

#ifdef CONFIG_MOCK_SUIT_STORAGE
	mock_suit_storage_reset();
#endif /* CONFIG_MOCK_SUIT_STORAGE */

#ifdef CONFIG_MOCK_SUIT_PLAT_DECODE_UTIL
	mock_suit_plat_decode_util_reset();
#endif /* CONFIG_MOCK_SUIT_PLAT_DECODE_UTIL */
}

#endif /* MOCK_H__ */
