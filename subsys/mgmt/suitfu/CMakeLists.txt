#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_include_directories(
    include
)

zephyr_library()

if(CONFIG_SSF_SUIT_SERVICE_ENABLED)
  zephyr_library_link_libraries(suit_utils)
endif()

zephyr_library_sources(
  src/suitfu_mgmt.c
  )
