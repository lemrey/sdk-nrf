#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources_ifdef(CONFIG_ML_APP_EVENT_PROXY
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/event_proxy.c)
