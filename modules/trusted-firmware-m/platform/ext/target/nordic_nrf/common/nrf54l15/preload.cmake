#-------------------------------------------------------------------------------
# Copyright (c) 2020, Nordic Semiconductor ASA.
# Copyright (c) 2020, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

# preload.cmake is used to set things that related to the platform that are both
# immutable and global, which is to say they should apply to any kind of project
# that uses this platform. In practise this is normally compiler definitions and
# variables related to hardware.

# Set architecture and CPU
set(TFM_SYSTEM_PROCESSOR cortex-m33)
set(TFM_SYSTEM_ARCHITECTURE armv8-m.main)
set(TFM_SYSTEM_DSP OFF)
set(CONFIG_TFM_FP_ARCH "fpv5-sp-d16")

add_compile_definitions(
  MOONLIGHT_XXAA # required by nrfx_config.h
  NRF54L15_ENGA_XXAA # Required by nrf.h
  NRF_APPLICATION
  # SKIP configuring the SAU from the MDK as it does not fit TF-M's needs
  NRF_SKIP_SAU_CONFIGURATION
)