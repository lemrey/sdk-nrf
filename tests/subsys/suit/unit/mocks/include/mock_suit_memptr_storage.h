/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOCK_SUIT_MEMPTR_STORAGE_H__
#define MOCK_SUIT_MEMPTR_STORAGE_H__

#include <zephyr/fff.h>
#include <mock_globals.h>

#include <suit_memptr_storage.h>

FAKE_VALUE_FUNC(int, get_memptr_storage, memptr_storage_handle *);
FAKE_VALUE_FUNC(int, release_memptr_storage, memptr_storage_handle);
FAKE_VALUE_FUNC(int, store_memptr_ptr, memptr_storage_handle, uint8_t *, size_t);
FAKE_VALUE_FUNC(int, get_memptr_ptr, memptr_storage_handle, uint8_t **, size_t *);

static inline void mock_suit_memptr_storage_reset(void)
{
	RESET_FAKE(get_memptr_storage);
	RESET_FAKE(release_memptr_storage);
	RESET_FAKE(store_memptr_ptr);
	RESET_FAKE(get_memptr_ptr);

	get_memptr_storage_fake.return_val = SUIT_PLAT_SUCCESS;
	release_memptr_storage_fake.return_val = SUIT_PLAT_SUCCESS;
	store_memptr_ptr_fake.return_val = SUIT_PLAT_SUCCESS;
	get_memptr_ptr_fake.return_val = SUIT_PLAT_SUCCESS;
}

#endif /* MOCK_SUIT_MEMPTR_STORAGE_H__ */
