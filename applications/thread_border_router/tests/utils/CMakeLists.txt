# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf_tbr_utils)

set(NRF_TBR_SRC ${ZEPHYR_NRF_MODULE_DIR}/applications/thread_border_router/src)

target_include_directories(app PRIVATE 
    ${NRF_TBR_SRC}/
    ${ZEPHYR_BASE}/subsys/net/ip
)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE
    ${app_sources}
    # Test file
    ${NRF_TBR_SRC}/utils/utils.c
)
