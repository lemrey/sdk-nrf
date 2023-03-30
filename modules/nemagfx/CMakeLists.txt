#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()

zephyr_library_sources(nema_hal.c)
zephyr_library_sources_ifdef(CONFIG_NEMADC_ENABLE nema_dc_hal.c)
