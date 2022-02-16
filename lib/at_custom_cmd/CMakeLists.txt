#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources(at_custom_cmd.c)

zephyr_linker_sources(RODATA at_custom_cmd.ld)
