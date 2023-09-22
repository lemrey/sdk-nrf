#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# SUIT Manifest Configuration Information API
zephyr_interface_library_named(suit_mci)
target_include_directories(suit_mci INTERFACE include)

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_SUIT_MCI_IMPL_NRF54H20_SDFW src/mci_nrf54h20.c)
zephyr_library_sources_ifdef(CONFIG_SUIT_MCI_IMPL_POSIX src/mci_posix.c)
zephyr_library_sources_ifdef(CONFIG_SUIT_MCI_IMPL_NRF52840 src/mci_nrf52840.c)
zephyr_library_sources(src/generic_ids.c)
zephyr_library_sources(src/utils.c)

zephyr_library_link_libraries(suit_mci)
