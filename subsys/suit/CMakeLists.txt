#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()

zephyr_library_link_libraries(suit)

zephyr_library_include_directories(./)

zephyr_library_sources(suit_plat_state.c)
zephyr_library_sources(suit_plat_component_impl_selector.c)
zephyr_library_sources(common/suit_plat_crypto_psa.c)
zephyr_library_sources(common/suit_plat_commands.c)
zephyr_library_sources(common/suit_plat_components.c)

zephyr_library_sources_ifdef(CONFIG_SUIT_COMPONENT_RAMBUF suit_plat_rambuf.c)
