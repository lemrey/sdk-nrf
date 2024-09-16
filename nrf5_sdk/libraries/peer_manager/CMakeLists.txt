#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

FILE(GLOB sources *.c)
zephyr_library_sources(${sources})

zephyr_include_directories(.)
