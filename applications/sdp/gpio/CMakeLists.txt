#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(emulated_gpio)

target_sources_ifdef(CONFIG_GPIO_NRFE_EGPIO_BACKEND_ICMSG app PRIVATE src/nrfe_icmsg.c)
target_sources_ifdef(CONFIG_GPIO_NRFE_EGPIO_BACKEND_MBOX app PRIVATE  src/nrfe_mbox.c)
target_sources(app PRIVATE src/main.c)
