#include <zephyr.h>

#include <pm_config.h>

#include <nrf_modem.h>
#include <nrf_modem_platform.h>
#include <nrf_modem_at.h>

static const nrf_modem_init_params_t init_params = {
	.ipc_irq_prio = NRF_MODEM_NETWORK_IRQ_PRIORITY,
	.shmem.ctrl = {
		.base = PM_NRF_MODEM_LIB_CTRL_ADDRESS,
		.size = CONFIG_NRF_MODEM_LIB_SHMEM_CTRL_SIZE,
	},
	.shmem.tx = {
		.base = PM_NRF_MODEM_LIB_TX_ADDRESS,
		.size = CONFIG_NRF_MODEM_LIB_SHMEM_TX_SIZE,
	},
	.shmem.rx = {
		.base = PM_NRF_MODEM_LIB_RX_ADDRESS,
		.size = CONFIG_NRF_MODEM_LIB_SHMEM_RX_SIZE,
	},
#if CONFIG_NRF_MODEM_LIB_TRACE_ENABLED
	.shmem.trace = {
		.base = PM_NRF_MODEM_LIB_TRACE_ADDRESS,
		.size = CONFIG_NRF_MODEM_LIB_SHMEM_TRACE_SIZE,
	},
#endif
};


void main(void)
{
	int32_t err;

    printk("SoftSIM sample started.\n");

	err = nrf_modem_init(&init_params, NORMAL_MODE);

	err = nrf_modem_at_init();
	if (err) {
		printk("Failed to initialize AT, err %d\n", err);
		return;
	}

	err = nrf_modem_at_printf("AT+CFUN=41");
	if (err) {
		printk("Failed to write AT command, err %d\n", err);
		return;
	}

	while (1) {
		k_sleep(K_MSEC(1));
	}
}
