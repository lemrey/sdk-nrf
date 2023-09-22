#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# SUIT legacy platform implementation
zephyr_library()

zephyr_library_sources(src/suit_plat_config.c)
zephyr_library_sources(src/suit_plat_envelope_ref.c)
zephyr_library_sources(src/suit_plat_component_impl_selector.c)
zephyr_library_sources(src/suit_plat_commands.c)
zephyr_library_sources(src/suit_plat_driver_com.c)
zephyr_library_sources(src/plat_components.c)

zephyr_library_sources_ifdef(CONFIG_SUIT_COMPONENT_RAMBUF src/suit_plat_rambuf.c)
zephyr_library_sources_ifdef(CONFIG_SUIT_COMPONENT_MRAM src/suit_plat_mram.c)
zephyr_library_sources_ifdef(CONFIG_SUIT_CRYPTO src/suit_plat_crypto_psa.c)

zephyr_library_link_libraries(suit)
zephyr_library_link_libraries(suit_utils)
zephyr_library_link_libraries(suit_platform_interface)
zephyr_library_link_libraries_ifdef(CONFIG_SUIT_STORAGE suit_storage_interface)
