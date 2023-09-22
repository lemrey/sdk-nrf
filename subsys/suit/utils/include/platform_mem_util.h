/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUIT_PLAT_MEM_UTIL_H__
#define SUIT_PLAT_MEM_UTIL_H__

#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

#ifdef CONFIG_FLASH_SIMULATOR
#include <zephyr/drivers/flash/flash_simulator.h>

#define FLASH_ADDRESS(offset) (f_base_address + offset)
#define FLASH_OFFSET(address) (((uint8_t *)address) - f_base_address)
#else /* CONFIG_FLASH_SIMULATOR */
#define FLASH_ADDRESS(offset)                                                                      \
	(COND_CODE_1(                                                                              \
		DT_NODE_EXISTS(DT_NODELABEL(mram10)),                                              \
		(((offset)&0xEFFFFFFFUL) + (DT_REG_ADDR(DT_NODELABEL(mram10)) & 0xEFFFFFFFUL)),    \
		(offset)))
#define FLASH_OFFSET(address)                                                                      \
	(COND_CODE_1(                                                                              \
		DT_NODE_EXISTS(DT_NODELABEL(mram10)),                                              \
		(((address)&0xEFFFFFFFUL) - (DT_REG_ADDR(DT_NODELABEL(mram10)) & 0xEFFFFFFFUL)),   \
		(address)))
#endif /* CONFIG_FLASH_SIMULATOR */

#define FLASH_AREA_ERASE_BLOCK_SIZE(label)                                                         \
	DT_PROP(DT_GPARENT(DT_NODELABEL(label)), erase_block_size)
#define FLASH_AREA_WRITE_BLOCK_SIZE(label)                                                         \
	DT_PROP(DT_GPARENT(DT_NODELABEL(label)), write_block_size)

#ifdef CONFIG_SDFW_MRAM_ERASE_VALUE
#define EMPTY_STORAGE_VALUE                                                                        \
	((((uint32_t)(CONFIG_SDFW_MRAM_ERASE_VALUE)) << 24) |                                      \
	 (((uint32_t)(CONFIG_SDFW_MRAM_ERASE_VALUE)) << 16) |                                      \
	 (((uint32_t)(CONFIG_SDFW_MRAM_ERASE_VALUE)) << 8) |                                       \
	 (((uint32_t)(CONFIG_SDFW_MRAM_ERASE_VALUE))))
#else
#define EMPTY_STORAGE_VALUE 0xFFFFFFFF
#endif

#endif /* SUIT_PLAT_MEM_UTIL_H__ */
