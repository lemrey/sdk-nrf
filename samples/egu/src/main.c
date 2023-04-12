/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/logging/log.h>
#include <nrf.h>
#include <drivers/egu.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>

LOG_MODULE_REGISTER(egu_sample, 4);

struct k_sem egu_sem;

K_SEM_DEFINE(egu_sem, 0, 1);

static void egu_callback(const struct device *dev, int channel, void *ctx)
{
	k_sem_give(&egu_sem);
}

void main(void)
{
	LOG_INF("Egu sample");

	const struct device *egu = DEVICE_DT_GET(DT_NODELABEL(dut));

	if (!device_is_ready(egu)) {
		return;
	}

	int egu_channel_num = egu_channel_number_get(egu);
	int err;

	for (int i = 0; i < egu_channel_num; i++) {
		err = egu_channel_callback_set(egu, i, egu_callback, NULL);

		if (err != 0) {
			LOG_ERR("Error in setting channel %d callback", i);
			return;
		}
	}

	/* Trigger tasks and wait for callback to be called. */
	for (int i = 0; i < egu_channel_num; i++) {
		err = egu_channel_task_trigger(egu, i);

		if (err != 0) {
			LOG_ERR("Error in triggering channel %d task", i);
			return;
		}

		err = k_sem_take(&egu_sem, K_MSEC(100));

		if (err == 0) {
			LOG_INF("Channel number %d interrupt called", i);
		} else {
			LOG_ERR("Channel number %d interrupt timeout", i);
		}
	}
}
