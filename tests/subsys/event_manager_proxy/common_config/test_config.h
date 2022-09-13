/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#if defined(BOARD_FPGA)
	/* FPGA boards are slower and executing full scale test is impractical. */
	#define TEST_CONFIG_SIMPLE_BURST_SIZE 100
	#define TEST_CONFIG_DATA_BURST_SIZE 100
	#define TEST_CONFIG_DATA_BIG_BURST_SIZE 10
#else
	#define TEST_CONFIG_SIMPLE_BURST_SIZE 10000
	#define TEST_CONFIG_DATA_BURST_SIZE 10000
	#define TEST_CONFIG_DATA_BIG_BURST_SIZE 1000
#endif

/* Test related timeouts. */
#if defined(BOARD_FPGA)
	/* FPGA boards need more time. */
	#define TEST_TIMEOUT_BASE_S 10
	#define RESPONSE_TIMEOUT_S 10
	#define REMOTE_START_TIMEOUT_S 10
#else
	#define TEST_TIMEOUT_BASE_S 1
	#define RESPONSE_TIMEOUT_S 1
	#define REMOTE_START_TIMEOUT_S 1
#endif
