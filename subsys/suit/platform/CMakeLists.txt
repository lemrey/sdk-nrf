#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

# SUIT Platform API
zephyr_interface_library_named(suit_platform_interface)
target_include_directories(suit_platform_interface INTERFACE include)

# SUIT Platform implementation for SDFW
zephyr_library()

zephyr_library_sources(src/suit_plat_state.c)
zephyr_library_sources(src/suit_plat_envelope_ref.c)
zephyr_library_sources(src/suit_plat_component_impl_selector.c)


zephyr_library_sources_ifdef(CONFIG_SUIT_COMPONENT_RAMBUF src/suit_plat_rambuf.c)
zephyr_library_sources_ifdef(CONFIG_SUIT_COMPONENT_MRAM src/suit_plat_mram.c)

zephyr_library_link_libraries(suit)
zephyr_library_link_libraries(suit_common)
zephyr_library_link_libraries(suit_platform_interface)
