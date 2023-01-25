#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

# SUIT Common API
zephyr_interface_library_named(suit_common)
target_include_directories(suit_common INTERFACE include)

# SUIT common implementation for SDFW
zephyr_library()
zephyr_library_sources(src/suit_storage.c)
zephyr_library_sources(src/suit_plat_commands.c)
zephyr_library_sources(src/suit_plat_components.c)
zephyr_library_sources(src/suit_plat_crypto_psa.c)
zephyr_library_sources(src/suit_plat_driver_com.c)

zephyr_library_link_libraries(suit)
zephyr_library_link_libraries(suit_common)
zephyr_library_link_libraries(suit_platform_interface)
