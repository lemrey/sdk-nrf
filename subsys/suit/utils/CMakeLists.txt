#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# SUIT utilities
zephyr_interface_library_named(suit_utils)
target_include_directories(suit_utils INTERFACE include)
if(CONFIG_SUIT_MCI)
target_link_libraries(suit_utils INTERFACE suit_mci)
endif() # CONFIG_SUIT_MCI
if(CONFIG_SUIT_PLATFORM)
target_link_libraries(suit_utils INTERFACE suit_platform_interface)
target_link_libraries(suit_utils INTERFACE suit)
endif() # CONFIG_SUIT_PLATFORM
target_link_libraries(suit_utils INTERFACE suit_platform_err)

zephyr_library()
zephyr_library_sources(src/suit_plat_decode_util.c)
zephyr_library_sources(src/suit_plat_mem_util.c)

zephyr_library_link_libraries(suit_utils)
zephyr_library_link_libraries_ifdef(CONFIG_SUIT_MCI suit_mci)
