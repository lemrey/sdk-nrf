/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <string.h>

#include <zephyr/logging/log.h>

#include <zephyr/ipc/ipc_service.h>

#if DT_NODE_EXISTS(DT_NODELABEL(ppr_code))
#define PPR_START_ADDR DT_REG_ADDR(DT_NODELABEL(ppr_code))
#include <hal/nrf_vpr.h>
#endif

#ifdef CONFIG_TEST_EXTRA_STACK_SIZE
#define STACKSIZE	(1024 + CONFIG_TEST_EXTRA_STACK_SIZE)
#else
#define STACKSIZE	(1024)
#endif

LOG_MODULE_REGISTER(host, LOG_LEVEL_INF);

struct payload {
	unsigned long cnt;
	unsigned long size;
	uint8_t data[];
};

static K_SEM_DEFINE(bound_sem, 0, 1);
K_THREAD_STACK_DEFINE(check_task_stack, STACKSIZE);
struct k_thread check_task_data;

static uint32_t payload_data[(CONFIG_APP_IPC_SERVICE_MESSAGE_LEN + 3)/4];
static struct payload *p_payload = (struct payload *) payload_data;

static void ep_bound(void *priv)
{
	k_sem_give(&bound_sem);
}

static void ep_recv(const void *data, size_t len, void *priv)
{
	uint8_t received_val = *((uint8_t *)data);
	static uint8_t expected_val;

	if ((received_val != expected_val) || (len != CONFIG_APP_IPC_SERVICE_MESSAGE_LEN)) {
		printk("Unexpected message received_val: %d , expected_val: %d\n",
			received_val,
			expected_val);
	}

	expected_val++;
}

static struct ipc_ept_cfg ep_cfg = {
	.name = "ep0",
	.cb = {
		.bound    = ep_bound,
		.received = ep_recv,
	},
};

static void check_task(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	unsigned long last_cnt = p_payload->cnt;
	unsigned long delta;

	while (1) {
		k_sleep(K_MSEC(1000));

		delta = p_payload->cnt - last_cnt;

		printk("Δpkt: %ld (%ld B/pkt) | throughput: %ld bit/s\n",
			delta, p_payload->size, delta * CONFIG_APP_IPC_SERVICE_MESSAGE_LEN * 8);

		last_cnt = p_payload->cnt;
	}
}

int main(void)
{
	const struct device *ipc0_instance;
	struct ipc_ept ep;
	int ret;
	printk("IPC-service %s demo started\n", CONFIG_BOARD);

#if DT_NODE_EXISTS(DT_NODELABEL(ppr_code))
	/* Enable PPr core */
	printk("Enabling PPR core\n");
	nrf_vpr_initpc_set(NRF_VPR130, PPR_START_ADDR);
	nrf_vpr_cpurun_set(NRF_VPR130, true);
#endif

	memset(p_payload->data, 0xA5, CONFIG_APP_IPC_SERVICE_MESSAGE_LEN - sizeof(struct payload));

	p_payload->size = CONFIG_APP_IPC_SERVICE_MESSAGE_LEN;
	p_payload->cnt = 0;

	ipc0_instance = DEVICE_DT_GET(DT_NODELABEL(ipc0));

	ret = ipc_service_open_instance(ipc0_instance);
	if ((ret < 0) && (ret != -EALREADY)) {
		LOG_INF("ipc_service_open_instance() failure: %d", ret);
		return ret;
	}

	ret = ipc_service_register_endpoint(ipc0_instance, &ep, &ep_cfg);
	if (ret < 0) {
		printf("ipc_service_register_endpoint() failure");
		return ret;
	}

	k_sem_take(&bound_sem, K_FOREVER);

	(void)k_thread_create(
		&check_task_data, check_task_stack,
		K_THREAD_STACK_SIZEOF(check_task_stack),
		check_task,
		NULL, NULL, NULL,
		-1, 0, K_NO_WAIT);

	while (true) {
		ret = ipc_service_send(&ep, p_payload, CONFIG_APP_IPC_SERVICE_MESSAGE_LEN);
		if (ret == -ENOMEM) {
			/* No space in the buffer. Retry. */
			LOG_DBG("send_message(%ld) failed with ret -ENOMEM\n", p_payload->cnt);
		} else if (ret < 0) {
			LOG_ERR("send_message(%ld) failed with ret %d\n", p_payload->cnt, ret);
			break;
		} else {
			p_payload->cnt++;
		}

		if (CONFIG_APP_IPC_SERVICE_SEND_INTERVAL < (USEC_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC)) {
			k_busy_wait(CONFIG_APP_IPC_SERVICE_SEND_INTERVAL);
		} else {
			k_usleep(CONFIG_APP_IPC_SERVICE_SEND_INTERVAL);
		}
	}

	return 0;
}
