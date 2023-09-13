#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(thread_border_router)

set(OPENTHREAD_DIR ${PROJECT_SOURCE_DIR}/../../../modules/lib/openthread)

target_include_directories(app PRIVATE
	src
	${ZEPHYR_BASE}/subsys/net/ip
	${OPENTHREAD_DIR}/src/core
)

# These configurations are required in order to inject our own implementation of
# OpenThread's API function `otIp6Send()`
zephyr_ld_options("-Wl,--wrap=otIp6Send")
set (CMAKE_CXX_FLAGS
	"-DOPENTHREAD_PROJECT_CORE_CONFIG_FILE=\\\"openthread-core-zephyr-config.h\\\""
	${CMAKE_CXX_FLAGS})

target_sources(app PRIVATE
	src/border_agent.c
	src/main.c
	src/mdns_publisher.c
	src/tbr.c
	src/shell.c
	src/api_extensions/ip6_ext.cpp
	src/backbone/mcast_routing.c
	src/platform/infra_if.c
	src/platform/udp.c
	src/utils/utils.c
)
