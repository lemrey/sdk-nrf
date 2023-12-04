#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(suit_smp_transfer)

target_sources(app PRIVATE src/main.c)

if (CONFIG_MCUMGR_TRANSPORT_BT)
  target_sources(app PRIVATE src/bluetooth.c)
endif()

if (CONFIG_SUIT_BUILD_FLASH_COMPANION)
  foreach (overlay IN LISTS DTC_OVERLAY_FILE)
    if (EXISTS "${overlay}")
      get_filename_component(overlay_path "${overlay}" ABSOLUTE)
      add_overlay_dts(flash_companion "${overlay_path}")
    endif()
  endforeach()

  add_child_image(
    NAME flash_companion
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/../flash_companion
    BOARD ${BOARD}
  )
endif()
