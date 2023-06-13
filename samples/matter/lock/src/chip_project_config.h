/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 *    @file
 *          Example project configuration file for CHIP.
 *
 *          This is a place to put application or project-specific overrides
 *          to the default configuration values for general CHIP features.
 *
 */

#pragma once

#ifdef CONFIG_EMULATOR_FPGA

#define CHIP_CONFIG_MRP_DEFAULT_INITIAL_RETRY_INTERVAL (15000)
#define CHIP_CONFIG_MRP_DEFAULT_ACTIVE_RETRY_INTERVAL (2000_ms32)
#define CHIP_CONFIG_MRP_DEFAULT_IDLE_RETRY_INTERVAL (2000_ms32)
#define CHIP_CONFIG_RMP_DEFAULT_ACK_TIMEOUT (2000_ms32)
#define CHIP_CONFIG_RMP_DEFAULT_MAX_RETRANS (10)
#define CHIP_DEVICE_CONFIG_FAILSAFE_EXPIRY_LENGTH_SEC 240

#endif
