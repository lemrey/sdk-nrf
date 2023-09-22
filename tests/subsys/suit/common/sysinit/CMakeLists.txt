# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if (CONFIG_SOC_NRF54H20_CPUSEC AND CONFIG_FLASH)
# Add MPC overrides for MRAM10 and MRAM11
zephyr_sources(
  mram_configure.c
  )
endif()
