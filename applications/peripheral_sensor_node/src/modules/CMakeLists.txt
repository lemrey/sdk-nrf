#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources(app PRIVATE nsms.c)

target_sources_ifdef(CONFIG_APP_SENSORS app PRIVATE sensors.c)
