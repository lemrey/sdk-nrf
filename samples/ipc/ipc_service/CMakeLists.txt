#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.20.0)

if (CONF_FILE)
  if(${CONF_FILE} MATCHES "prj_(.*).conf")
    set(CONF_FILE_BUILD_TYPE ${CMAKE_MATCH_1})
  endif()
endif()

set(ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_LIST_DIR})

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ipc_service_host)

target_sources(app PRIVATE src/main.c)

if (CONFIG_SOC_PLATFORM_HALTIUM)
  if(NOT((${BOARD} MATCHES ".*_cpusys$") OR (${CONFIG_APP_REMOTE_BOARD} MATCHES ".*_cpusys($|@)")))
    build_sysctrl()
  else()
    message(WARNING "Remote or App set as sysctrl")
  endif()
  build_secdom()
endif()
