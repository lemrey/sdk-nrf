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

#ifdef CONFIG_MOCK_SUIT_METADATA
#include <mock_suit_metadata.h>
#endif /* CONFIG_MOCK_SUIT_METADATA */

#ifdef CONFIG_MOCK_SUIT_STORAGE
#include <mock_suit_storage.h>
#endif /* CONFIG_MOCK_SUIT_STORAGE */

#ifdef CONFIG_MOCK_SUIT_PLAT_DECODE_UTIL
#include <mock_suit_plat_decode_util.h>
#endif /* CONFIG_MOCK_SUIT_PLAT_DECODE_UTIL */

#ifdef CONFIG_MOCK_SUIT_CRYPTO
#include <mock_suit_crypto.h>
#endif

#ifdef CONFIG_MOCK_SUIT_KERNEL
#include <mock_suit_kernel.h>
#endif

#ifdef CONFIG_MOCK_SUIT_PLAT_CHECK_COMPONENT_COMPATIBILITY
#include <mock_suit_plat_component_compatibility.h>
#endif /* CONFIG_MOCK_SUIT_PLAT_CHECK_COMPONENT_COMPATIBILITY */

#ifdef CONFIG_MOCK_SUIT_PLATFORM_INTERNAL
#include <mock_suit_platform_internal.h>
#endif

#ifdef CONFIG_MOCK_SUIT_MEMPTR_STORAGE
#include <mock_suit_memptr_storage.h>
#endif /* CONFIG_MOCK_SUIT_MEMPTR_STORAGE */

static inline void mocks_reset(void)
{
#ifdef CONFIG_MOCK_SUIT_PROCESSOR
	mock_suit_processor_reset();
#endif /* CONFIG_MOCK_SUIT_PROCESSOR */

#ifdef CONFIG_MOCK_SUIT_MCI
	mock_suit_mci_reset();
#endif /* CONFIG_MOCK_SUIT_MCI */

#ifdef CONFIG_MOCK_SUIT_METADATA
	mock_suit_metadata_reset();
#endif /* CONFIG_MOCK_SUIT_METADATA */

#ifdef CONFIG_MOCK_SUIT_STORAGE
	mock_suit_storage_reset();
#endif /* CONFIG_MOCK_SUIT_STORAGE */

#ifdef CONFIG_MOCK_SUIT_PLAT_DECODE_UTIL
	mock_suit_plat_decode_util_reset();
#endif /* CONFIG_MOCK_SUIT_PLAT_DECODE_UTIL */

#ifdef CONFIG_MOCK_SUIT_CRYPTO
	mock_suit_crypto_reset();
#endif

#ifdef CONFIG_MOCK_SUIT_KERNEL
	mock_suit_kernel_reset();
#endif

#ifdef CONFIG_MOCK_SUIT_PLAT_CHECK_COMPONENT_COMPATIBILITY
	mock_suit_plat_component_compatibility_check_reset();
#endif

#ifdef CONFIG_MOCK_SUIT_PLATFORM_INTERNAL
	mock_suit_platform_internal_reset();
#endif

#ifdef CONFIG_MOCK_SUIT_MEMPTR_STORAGE
	mock_suit_memptr_storage_reset();
#endif
}
#endif /* MOCK_H__ */
