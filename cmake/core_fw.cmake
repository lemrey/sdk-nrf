#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

function(internal_build_secdom extra_arg)
    include(ExternalProject)

    if (BOARD_REVISION)
      set(board_rev "@${BOARD_REVISION}")
    endif()

    # If the application is build for FPGA, detect that and forward to sdfw
    if (CONFIG_EMULATOR_FPGA)
      list(APPEND extra_arg "-DCONFIG_EMULATOR_FPGA:STRING=y")
    endif()

    # Select a matching secdom board.
    if(CONFIG_SOC_NRF54H20)
        set(BOARD_REMOTE "nrf54h20dk_nrf54h20_cpusec${board_rev}")
    elseif(CONFIG_SOC_NRF9230)
        set(BOARD_REMOTE "nrf9230dk_nrf9230_cpusec${board_rev}")
    endif()

    if (NOT DEFINED BOARD_REMOTE)
      message(FATAL_ERROR "SDFW cannot be built for this SoC")
    endif()

    if (OVERLAY_CONFIGS)
      set(overlay_configs_opt "-DOVERLAY_CONFIG:STRING=${OVERLAY_CONFIGS}")
    endif()

    if (DTC_OVERLAY_FILES)
      set(dtc_overlay_files_opt "-DDTC_OVERLAY_FILE:STRING=${DTC_OVERLAY_FILES}")
    endif()

    ExternalProject_Add(
      secdom
      SOURCE_DIR ${NRF_SDFW}/project/sdfw
      INSTALL_COMMAND ""
      CMAKE_CACHE_ARGS
        -DBOARD:STRING=${BOARD_REMOTE}
        ${extra_arg}
        ${overlay_configs_opt}
        ${dtc_overlay_files_opt}
        -DCONFIG_SDFW_MAP_GRTC_APPLICATION_PROGRAMMABLE_IRQ_TO_PPR:STRING=y
      BUILD_ALWAYS True
    )
endfunction()

function(build_secdom)
  internal_build_secdom("")
endfunction()

function(build_secdom_stm_logging_frontend)
  set(DTC_OVERLAY_FILES "nrf54h20.overlay;serial_cpusec.overlay")
  set(OVERLAY_CONFIGS "stm_trace_host_etr.conf;log_common.conf;stm_logging.conf")
  internal_build_secdom(-DCONFIG_ETR_DUMP_DECODE:STRING=y)
endfunction()

function(build_sysctrl)
    include(ExternalProject)

    if (BOARD_REVISION)
      set(board_rev "@${BOARD_REVISION}")
    endif()

    # If the application is build for FPGA, detect that and forward to scfw
    if (CONFIG_EMULATOR_FPGA)
      list(APPEND extra_arg "-DCONFIG_EMULATOR_FPGA:STRING=y")
    endif()

    # Select a matching sysctrl board.
    if(CONFIG_SOC_NRF54H20)
        set(BOARD_REMOTE "nrf54h20dk_nrf54h20_cpusys${board_rev}")
    endif()

    if (NOT DEFINED BOARD_REMOTE)
      message(FATAL_ERROR "SCFW cannot be built for this SoC")
    endif()

    ExternalProject_Add(
      sysctrl
      SOURCE_DIR ${NRF_DIR}/../core_fw/sysctrl/sysctrl/src
      INSTALL_COMMAND ""
      CMAKE_CACHE_ARGS
        -DBOARD:STRING=${BOARD_REMOTE}
        -DBUILD_TYPE:STRING=Release
        ${extra_arg}
      BUILD_ALWAYS True
    )
endfunction()
