#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

set(PLATFORM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/sl/platform)

set(ZEPHYR_CURRENT_LIBRARY nrf-802154-platform)
zephyr_library_sources_ifdef(CONFIG_NRF_802154_SL_HPTIMER ${PLATFORM_DIR}/nrf_802154_hp_timer.c)
zephyr_library_sources_ifdef(CONFIG_NRF_802154_SL_LPTIMER_RTC ${PLATFORM_DIR}/nrf_802154_platform_sl_lptimer_zephyr.c)
zephyr_library_sources_ifdef(CONFIG_NRF_802154_SL_LPTIMER_GRTC ${PLATFORM_DIR}/nrf_802154_platform_sl_lptimer_grtc_zephyr.c)
zephyr_library_sources_ifdef(CONFIG_NRF_802154_SL_LPTIMER_EVENTS_HALTIUM ${PLATFORM_DIR}/nrf_802154_platform_sl_lptimer_grtc_events_haltium.c)
zephyr_library_sources_ifdef(CONFIG_NRF_802154_SL_LPTIMER_EVENTS_NRF54L ${PLATFORM_DIR}/nrf_802154_platform_sl_lptimer_grtc_events_nrf54l.c)

if (CONFIG_NRF_802154_RADIO_DRIVER OR CONFIG_NRF_802154_SERIALIZATION)
  if (CONFIG_MPSL)
    target_compile_definitions(nrf-802154-platform PUBLIC NRF_802154_VERIFY_PERIPHS_ALLOC_AGAINST_MPSL=1)
  endif()

  if (CONFIG_SOC_SERIES_NRF54HX OR CONFIG_SOC_PLATFORM_NRF54L)
    target_compile_definitions(zephyr-802154-interface
      INTERFACE
        NRF_802154_TOTAL_TIMES_MEASUREMENT_ENABLED=0
      )
  endif ()

  set(NRF53_SERIES  ${CONFIG_SOC_SERIES_NRF53X})
  set(SER_HOST      ${CONFIG_NRF_802154_SER_HOST})
  set(SL_OPENSOURCE ${CONFIG_NRF_802154_SL_OPENSOURCE})

  if (CONFIG_NRF_802154_SOURCE_NRFXLIB)
    add_subdirectory(${ZEPHYR_CURRENT_MODULE_DIR}/nrf_802154 nrf_802154)

    target_link_libraries(nrf-802154-driver-interface INTERFACE zephyr-802154-interface)
    target_link_libraries(nrf-802154-serialization-interface INTERFACE zephyr-802154-interface)
  endif (CONFIG_NRF_802154_SOURCE_NRFXLIB)

  target_compile_definitions(zephyr-802154-interface INTERFACE NRF_802154_ECB_PRIORITY=-1)
  target_compile_definitions(zephyr-802154-interface INTERFACE NRF_802154_SWI_PRIORITY=1)

  if (CONFIG_NRF_802154_ACK_TIMEOUT_CUSTOM_US)
    target_compile_definitions(zephyr-802154-interface
      INTERFACE
        NRF_802154_PRECISE_ACK_TIMEOUT_DEFAULT_TIMEOUT=${CONFIG_NRF_802154_ACK_TIMEOUT_CUSTOM_US})
  endif ()
endif (CONFIG_NRF_802154_RADIO_DRIVER OR CONFIG_NRF_802154_SERIALIZATION)
