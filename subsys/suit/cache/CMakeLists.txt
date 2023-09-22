#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_interface_library_named(suit_cache_interface)
target_include_directories(suit_cache_interface INTERFACE include)

zephyr_library()
zephyr_library_sources(src/suit_cache.c)

zephyr_library_sources_ifdef(CONFIG_SUIT_CACHE_RW src/suit_cache_rw.c)

zephyr_library_link_libraries(suit_cache_interface)

zephyr_library_link_libraries_ifdef(CONFIG_SUIT_CACHE_RW suit_utils)
