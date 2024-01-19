#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_interface_library_named(suit_orchestrator_app)
target_include_directories(suit_orchestrator_app INTERFACE include)

zephyr_library()
zephyr_library_sources(src/suit_orchestrator_app.c)

zephyr_library_link_libraries_ifdef(CONFIG_SUIT_PROCESSOR suit)
zephyr_library_link_libraries(suit_orchestrator_app)
zephyr_library_link_libraries(suit_utils)
zephyr_library_link_libraries(suit_envelope_info)
zephyr_library_link_libraries(suit_platform_err)
