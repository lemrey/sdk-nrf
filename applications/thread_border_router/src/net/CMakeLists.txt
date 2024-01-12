#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_sources_ifdef(CONFIG_NRF_TBR_ROUTE_IPV4 routev4.c)
zephyr_library_sources_ifdef(CONFIG_NRF_TBR_NAT
    nat_core.c
    nat_proto.c
)

zephyr_library_sources(
    mdns_internal.c
    mdns_server.c
)
