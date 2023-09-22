/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/ztest.h>
#include <suit_mci.h>

void test_generic_ids(void);
void test_sanity(void);
void test_api_positive_scenarios(void);
void test_topology(void);

void test_main(void)
{
	mci_init();
	test_generic_ids();
	test_sanity();
	test_api_positive_scenarios();
	test_topology();
}
