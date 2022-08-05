#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

set(OT_NCP_VENDOR_HOOK_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

if (CONFIG_FPGA_TIMING_COMPATIBILITY)
    set(OVERLAY_CONFIG "overlay-fpga.conf")
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(openthread_coprocessor)

# NORDIC SDK APP START
target_sources(app PRIVATE src/main.c src/nrf_802154_radio_wrapper.c)
# NORDIC SDK APP END

if (CONFIG_FPGA_TIMING_COMPATIBILITY)
    message(WARNING "Building the example with NRF54 on FPGA TIMING COMPATIBILITY enabled!
    This example can work with FPGA targets and can not be used to work with other targets such as NRF52 or NRF53.")
    target_include_directories(nrf-802154-driver-interface BEFORE INTERFACE src)
endif ()
