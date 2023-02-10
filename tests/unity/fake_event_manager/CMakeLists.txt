#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_include_directories(.)

zephyr_sources_ifdef(
  CONFIG_FAKE_EVENT_MANAGER
  app_event_manager.c
  )
