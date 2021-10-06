#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
zephyr_library()
zephyr_library_sources(nrf_modem_lib.c)
zephyr_library_sources(nrf_modem_os.c)
zephyr_library_sources_ifdef(CONFIG_NET_SOCKETS nrf91_sockets.c)
zephyr_library_sources_ifdef(CONFIG_NRF_MODEM_LIB_AT_CMD_FILTER nrf_modem_at_cmd_filter.c)
zephyr_library_sources(shmem_sanity.c)
