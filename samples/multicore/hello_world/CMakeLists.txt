#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

set(ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_LIST_DIR})

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(app)

target_sources(app PRIVATE src/main.c)

if (CONFIG_SOC_SERIES_NRF54HX)
  if (CONFIG_APP_STM_LOGGING_FSC_FRONTEND)
    build_secdom_stm_logging_frontend()
  else()
    build_secdom()
  endif()

  build_sysctrl()
endif()
