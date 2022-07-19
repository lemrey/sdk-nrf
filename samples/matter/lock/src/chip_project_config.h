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

/* Use a default pairing code if one hasn't been provisioned in flash. */
#define CHIP_DEVICE_CONFIG_USE_TEST_SETUP_PIN_CODE 20202021
#define CHIP_DEVICE_CONFIG_USE_TEST_SETUP_DISCRIMINATOR 0xF00

#define CHIP_DEVICE_CONFIG_SED_IDLE_INTERVAL 2000_ms32

#ifdef CONFIG_FORCE_INCREASE_COMMISSIONING_TIMEOUTS

#define CHIP_CONFIG_MRP_DEFAULT_INITIAL_RETRY_INTERVAL (15000)
#define CHIP_CONFIG_MRP_DEFAULT_ACTIVE_RETRY_INTERVAL (2000_ms32)
#define CHIP_CONFIG_MRP_DEFAULT_IDLE_RETRY_INTERVAL (2000_ms32)
#define CHIP_CONFIG_RMP_DEFAULT_ACK_TIMEOUT (2000_ms32)
#define CHIP_CONFIG_RMP_DEFAULT_MAX_RETRANS (10)
#define CHIP_DEVICE_CONFIG_FAILSAFE_EXPIRY_LENGTH_SEC 240

#endif
