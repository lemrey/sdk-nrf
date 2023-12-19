#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# SUIT SDFW Platform API
zephyr_interface_library_named(suit_sdfw_platform)
target_include_directories(suit_sdfw_platform INTERFACE include)
target_link_libraries(suit_sdfw_platform INTERFACE suit)

zephyr_library()

zephyr_library_sources(src/plat_swap.c)
zephyr_library_sources(src/plat_fetch.c)
zephyr_library_sources(src/plat_copy.c)
zephyr_library_sources(src/plat_retrieve_manifest.c)
zephyr_library_sources_ifdef(CONFIG_SUIT_CHECK_IMAGE_MATCH src/plat_check_image_match.c)
zephyr_library_sources(src/plat_check_content.c)

zephyr_library_sources(src/plat_invoke.c)
if(${CONFIG_ZTEST})
  zephyr_library_sources(src/runners/run_posix.c)
else ()
  zephyr_library_sources_ifdef(CONFIG_SOC_NRF54H20_CPUSEC src/runners/run_nrf54h20.c)
endif ()

zephyr_library_link_libraries_ifdef(CONFIG_SUIT_STREAM suit_stream_sinks_interface)
zephyr_library_link_libraries_ifdef(CONFIG_SUIT_STREAM suit_stream_sources_interface)
zephyr_library_link_libraries_ifdef(CONFIG_SUIT_SINK_SELECTOR suit_sink_selector_interface)
zephyr_library_link_libraries_ifdef(CONFIG_SUIT_STORAGE suit_storage_interface)
zephyr_library_link_libraries_ifdef(CONFIG_SUIT_STREAM_SOURCE_MEMPTR suit_stream_sources_interface)


zephyr_library_link_libraries(suit_sdfw_platform)
zephyr_library_link_libraries(suit_utils)
zephyr_library_link_libraries(suit_platform_interface)
zephyr_library_link_libraries(suit_memptr_storage_interface)
