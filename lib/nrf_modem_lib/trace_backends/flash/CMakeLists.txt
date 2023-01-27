#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if (CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_FLASH)
  zephyr_library_sources(flash.c)
endif()
