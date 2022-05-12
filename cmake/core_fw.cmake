#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

function(build_secdom)
  if (CONFIG_SOC_SERIES_NRF54X)
    include(ExternalProject)
    set(BOARD_REMOTE "nrf54fpga_nrf5420_cpusec")

    ExternalProject_Add(
      secdom
      SOURCE_DIR ${NRF_SDFW}/project/sdfw
      INSTALL_COMMAND ""
      CMAKE_CACHE_ARGS
        -DBOARD:STRING=${BOARD_REMOTE}
        -DCONFIG_RESET_ON_FATAL_ERROR:STRING=n
      BUILD_ALWAYS True
    )
  else()
    message(FATAL_ERROR "Incorrect SoC series")
  endif()
endfunction()

function(create_app_uicr)
  # Adds an uicr to the build.
  #
  # TODO: Add DOMAIN depandent generation.
  #       Right now UICR_CONF_FILE is ment to be used for App core.
  #       Domain could be obtained from ${BOARD}

  if (DEFINED UICR_CONF_FILE AND EXISTS UICR_CONF_FILE)
    set(UICR_CONF_FILE ${UICR_CONF_FILE})
  elseif (EXISTS ${APPLICATION_CONFIG_DIR}/uicr/${BOARD}.yaml)
    set(UICR_CONF_FILE ${APPLICATION_CONFIG_DIR}/uicr/${BOARD}.yaml)
  else()
    message(FATAL_ERROR "UICR file does not exists")
  endif()

  set(DOMAIN "application")

  nrf_regtool_generate_icr_from_yaml(
    CONFIG_FILE_IN ${UICR_CONF_FILE}
    HEX_FILE_NAME_PREFIX app_uicr
    TARGET_DOMAIN ${DOMAIN}
    HEX_FILE_OUT uicr_hex_out
    ICR_TYPE UICR
  )
  add_custom_target(uicr_target ALL DEPENDS ${uicr_hex_out})

endfunction()
