#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_sources_ifdef(CONFIG_NRF_FSTORAGE nrf_fstorage.c)
zephyr_library_sources_ifdef(CONFIG_NRF_FSTORAGE_SD nrf_fstorage_sd.c)
#zephyr_library_sources(nrf_fstorage_nvmc.c)
zephyr_linker_sources_ifdef(CONFIG_NRF_FSTORAGE SECTIONS nrf_fstorage.ld)