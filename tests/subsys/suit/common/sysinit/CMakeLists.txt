# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

if (CONFIG_SOC_NRF54H20 AND CONFIG_FLASH)
# Add MPC overrides for MRAM10 and MRAM11
zephyr_sources(
  mram_configure.c
  )
endif()
