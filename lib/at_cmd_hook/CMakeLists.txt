#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources(at_cmd_hook.c)

zephyr_linker_sources(RWDATA at_cmd_hook.ld)
