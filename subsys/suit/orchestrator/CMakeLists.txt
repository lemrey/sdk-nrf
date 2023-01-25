#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

# SUIT Orchestrator API
zephyr_interface_library_named(suit_orchestrator_interface)
target_include_directories(suit_orchestrator_interface INTERFACE include)

# SUIT Orchestrator implementation for SDFW
zephyr_library()
zephyr_library_sources(src/suit_orchestrator_sdfw.c)

zephyr_library_link_libraries(suit)
zephyr_library_link_libraries(suit_common)
zephyr_library_link_libraries(suit_orchestrator_interface)
