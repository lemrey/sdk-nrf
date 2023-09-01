/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <nrf_errno.h>
#include <nrf_modem_os_rpc.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/ipc/icmsg.h>

struct nrf_modem_os_rpc {
	/** ICMsg internal data. */
	struct icmsg_data_t data;
	/** ICMsg configuration data. */
	struct icmsg_config_t conf;
	/** ICMsg callbacks to nrf_modem. */
	struct ipc_service_cb cb;
};

struct nrf_modem_os_rpc_signal {
	/** MBOX instance data. */
	struct mbox_channel channel;
	/** Callback to nrf_modem. */
	nrf_modem_os_rpc_signal_cb_t recv;
	/** Private context data usable by nrf_modem. */
	void *priv;
};


struct nrf_modem_os_rpc inst_ctrl;
struct nrf_modem_os_rpc inst_data;

struct nrf_modem_os_rpc_signal inst_app_fault;
struct nrf_modem_os_rpc_signal inst_modem_fault;
struct nrf_modem_os_rpc_signal inst_modem_sysoff;

uintptr_t nrf_modem_os_rpc_sigdev_app_get(void)
{
	const struct device *app_bellboard = DEVICE_DT_GET(DT_NODELABEL(bellboard_cpuapp));

	return (uintptr_t)app_bellboard;
}

uintptr_t nrf_modem_os_rpc_sigdev_modem_get(void)
{
	const struct device *modem_bellboard = DEVICE_DT_GET(DT_NODELABEL(bellboard_cpumod));

	return (uintptr_t)modem_bellboard;
}

int nrf_modem_os_rpc_open(struct nrf_modem_os_rpc *instance,
			  const struct nrf_modem_os_rpc_config *config)
{
	instance->conf.tx_shm_addr = config->tx.addr;
	instance->conf.rx_shm_addr = config->rx.addr;
	instance->conf.tx_shm_size = config->tx.size;
	instance->conf.rx_shm_size = config->rx.size;
	instance->conf.mbox_tx.dev = (struct device *)config->tx.sigdev;
	instance->conf.mbox_rx.dev = (struct device *)config->rx.sigdev;
	instance->conf.mbox_tx.id = config->tx.ch;
	instance->conf.mbox_rx.id = config->rx.ch;
	instance->cb.bound = config->cb.bound;
	instance->cb.received = config->cb.received;

	return icmsg_open(&instance->conf, &instance->data, &instance->cb, config->cb.priv);
}

int nrf_modem_os_rpc_send(struct nrf_modem_os_rpc *instance, const void *msg, size_t len)
{
	int ret;

	ret = icmsg_send(&instance->conf, &instance->data, msg, len);
	if (ret < 0) {
		switch (ret) {
		case -EBUSY:
			return -NRF_EBUSY;
		case -ENOBUFS:
		case -ENOMEM:
			return -NRF_ENOMEM;
		default:
			return ret;
		}
	}

	return 0;
}

int nrf_modem_os_rpc_close(struct nrf_modem_os_rpc *instance)
{
	return icmsg_close(&instance->conf, &instance->data);
}

int nrf_modem_os_rpc_rx_suspend(struct nrf_modem_os_rpc *instance, bool enable)
{
	return mbox_set_enabled(&instance->conf.mbox_rx, !enable);
}

static void mbox_common_callback(const struct device *dev, uint32_t ch, void *ctx,
				 struct mbox_msg *data)
{
	struct nrf_modem_os_rpc_signal *inst = (struct nrf_modem_os_rpc_signal *)ctx;

	ARG_UNUSED(dev);
	ARG_UNUSED(data);

	if (inst->recv != NULL) {
		inst->recv(ch, inst->priv);
	}
}

int nrf_modem_os_rpc_signal_init(struct nrf_modem_os_rpc_signal *instance,
				 struct nrf_modem_os_rpc_signal_config *conf)
{
	int err;

	instance->channel.dev = (struct device *)conf->sigdev;
	instance->channel.id = conf->ch;
	instance->priv = conf->priv;
	instance->recv = conf->recv;

	if (instance->recv == NULL) {
		return 0;
	}

	err = mbox_register_callback(&instance->channel, mbox_common_callback, (void *)instance);
	if (err) {
		goto errout;
	}

	err = mbox_set_enabled(&instance->channel, true);
	if (err) {
		goto errout;
	}

	return 0;

errout:
	instance->recv = NULL;
	return err;
}

int nrf_modem_os_rpc_signal_send(struct nrf_modem_os_rpc_signal *instance)
{
	if (instance->recv != NULL) {
		return -ENOSYS;
	}

	return mbox_send(&instance->channel, NULL);
}

int nrf_modem_os_rpc_signal_deinit(struct nrf_modem_os_rpc_signal *instance)
{
	if (instance->recv == NULL) {
		return 0;
	}

	return mbox_set_enabled(&instance->channel, false);
}
