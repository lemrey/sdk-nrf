#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

function(internal_build_secdom board_rev extra_arg)
    include(ExternalProject)

    # Select a matching secdom board.
    if(CONFIG_SOC_NRF5420)
      if(CONFIG_EMULATOR_FPGA)
        set(BOARD_REMOTE "nrf54fpga_nrf5420_cpusec${board_rev}")
      elseif(CONFIG_EMULATOR_Z1)
        set(BOARD_REMOTE "nrf54emu_nrf5420_cpusec${board_rev}")
      endif()
    elseif(CONFIG_SOC_NRF5420_SOC1)
      if(CONFIG_EMULATOR_FPGA)
        set(BOARD_REMOTE "nrf54fpga_nrf5420soc1_cpusec${board_rev}")
      elseif(CONFIG_EMULATOR_Z1)
        set(BOARD_REMOTE "nrf54emu_nrf5420soc1_cpusec${board_rev}")
      endif()
    elseif(CONFIG_SOC_NRF9230)
      if(CONFIG_EMULATOR_Z1)
        set(BOARD_REMOTE "nrf92emu_nrf9230_cpusec${board_rev}")
      endif()
    endif()

    if (NOT DEFINED BOARD_REMOTE)
      message(FATAL_ERROR "SDFW cannot be built for this SoC")
    endif()

    ExternalProject_Add(
      secdom
      SOURCE_DIR ${NRF_SDFW}/project/sdfw
      INSTALL_COMMAND ""
      CMAKE_CACHE_ARGS
        -DBOARD:STRING=${BOARD_REMOTE}
	${extra_arg}
	-DOVERLAY_CONFIG:STRING=${OVERLAY_CONFIGS}
        -DCONFIG_SDFW_MAP_GRTC_APPLICATION_PROGRAMMABLE_IRQ_TO_PPR:STRING=y
      BUILD_ALWAYS True
    )
endfunction()

function(build_secdom)
	internal_build_secdom("" "")
endfunction()

function(build_secdom_stm_logging_frontend)
	set(OVERLAY_CONFIGS "stm_etr_logging_base.conf;logging.conf;stm_logging.conf")
	internal_build_secdom(@uart_bellboard -DCONFIG_ETR_DUMP_DECODE:STRING=y)
endfunction()

function(build_sysctrl)
    include(ExternalProject)

    # Select a matching sysctrl board.
    if(CONFIG_SOC_NRF5420)
      if(CONFIG_EMULATOR_FPGA)
        set(BOARD_REMOTE "nrf54fpga_nrf5420_cpusys")
      elseif(CONFIG_EMULATOR_Z1)
        set(BOARD_REMOTE "nrf54emu_nrf5420_cpusys")
      endif()
    elseif(CONFIG_SOC_NRF5420_SOC1)
      if(CONFIG_EMULATOR_FPGA)
        set(BOARD_REMOTE "nrf54fpga_nrf5420soc1_cpusys")
      elseif(CONFIG_EMULATOR_Z1)
        set(BOARD_REMOTE "nrf54emu_nrf5420soc1_cpusys")
      endif()
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
      BUILD_ALWAYS True
    )
endfunction()
