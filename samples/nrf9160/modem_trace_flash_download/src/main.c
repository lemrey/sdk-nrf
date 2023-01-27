/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <modem/lte_lc.h>
#include <modem/nrf_modem_lib.h>
#include <modem/nrf_modem_lib_trace.h>
#include <nrf_modem.h>
#include <nrf_modem_at.h>
#include <dk_buttons_and_leds.h>

#include <zephyr/sys/reboot.h>

#include <string.h>
#include <stdlib.h>
#include <modem/modem_key_mgmt.h>
#include <net/download_client.h>
#include <zephyr/shell/shell.h>

#define URL CONFIG_SAMPLE_FILE_URL
#define SEC_TAG CONFIG_SAMPLE_SEC_TAG

#define PROGRESS_WIDTH 50
#define STARTING_OFFSET 0

static int64_t ref_time;
static struct download_client downloader;
static struct download_client_cfg config = {
#if CONFIG_SAMPLE_SECURE_SOCKET
	.sec_tag = SEC_TAG,
#else
	.sec_tag = -1,
#endif
};

LOG_MODULE_REGISTER(modem_trace_flash_sample, CONFIG_MODEM_TRACE_FLASH_SAMPLE_LOG_LEVEL);

#define UART1_DT_NODE DT_NODELABEL(uart1)

static const struct device *const uart_dev = DEVICE_DT_GET(UART1_DT_NODE);

LTE_LC_ON_CFUN(cfun_hook, on_cfun, NULL);

struct k_sem download_sem;

/* Callback for when modem functional mode is changed */
static void on_cfun(enum lte_lc_func_mode mode, void *context)
{
	LOG_INF("LTE mode changed to %d\n", mode);
}

static void print_uart1(char *buf, int len)
{
	if (!device_is_ready(uart_dev)) {
		LOG_ERR("uart1 device not found/ready!");
		return;
	}
	for (int i = 0; i < len; i++) {
		uart_poll_out(uart_dev, buf[i]);
	}
}

static void print_traces(void)
{
	const size_t READ_BUF_SIZE = 4096;
	uint8_t read_buf[READ_BUF_SIZE];
	int ret = READ_BUF_SIZE;
	size_t read_offset = 0;

	/* Read out the trace data from flash */
	while (ret > 0) {
		ret = nrf_modem_lib_trace_read(read_buf, READ_BUF_SIZE);
		if (ret < 0) {
			if (ret == -ENODATA) {
				break;
			}
			LOG_ERR("Error reading modem traces: %d", ret);
			break;
		}
		if (ret == 0) {
			LOG_DBG("No more traces to read from flash");
			break;
		}
		printk("r: %d\n", ret);
		read_offset += ret;
		print_uart1(read_buf, ret);
	}
	LOG_INF("Total trace bytes read from flash: %d", read_offset);
}

static int shell_print_trace(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(shell, "Read traces to uart");
	print_traces();

	return 0;
}

SHELL_CMD_REGISTER(read, NULL, "Read traces from flash to uart", shell_print_trace);

static void button_handler(uint32_t button_states, uint32_t has_changed)
{
	if (has_changed & button_states & DK_BTN1_MSK) {
		printk("Button 1 pressed - dumping traces to uart1\n");
		print_traces();
	}

	if (has_changed & button_states & DK_BTN2_MSK) {
		printk("Button 2 pressed - restarting application\n");
		sys_reboot(SYS_REBOOT_WARM);
	}
}


static void progress_print(size_t downloaded, size_t file_size)
{
	const int percent = (downloaded * 100) / file_size;
	size_t lpad = (percent * PROGRESS_WIDTH) / 100;
	size_t rpad = PROGRESS_WIDTH - lpad;

	printk("\r[ %3d%% ] |", percent);
	for (size_t i = 0; i < lpad; i++) {
		printk("=");
	}
	for (size_t i = 0; i < rpad; i++) {
		printk(" ");
	}
	printk("| (%d/%d bytes)", downloaded, file_size);
}

static size_t c_downloaded;
static int callback(const struct download_client_evt *event)
{
	static size_t file_size;
	uint32_t speed;
	int64_t ms_elapsed;

	if (c_downloaded == 0) {
		download_client_file_size_get(&downloader, &file_size);
		c_downloaded += STARTING_OFFSET;
	}

	switch (event->id) {
	case DOWNLOAD_CLIENT_EVT_FRAGMENT:
		c_downloaded += event->fragment.len;
		if (file_size) {
			progress_print(c_downloaded, file_size);
		} else {
			printk("\r[ %d bytes ] ", c_downloaded);
		}

#if CONFIG_SAMPLE_COMPUTE_HASH
		mbedtls_sha256_update(&sha256_ctx,
			event->fragment.buf, event->fragment.len);
#endif
		return 0;

	case DOWNLOAD_CLIENT_EVT_DONE:
		ms_elapsed = k_uptime_delta(&ref_time);
		speed = ((float)file_size / ms_elapsed) * MSEC_PER_SEC;
		printk("\nDownload completed in %lld ms @ %d bytes per sec, total %d bytes\n",
		       ms_elapsed, speed, c_downloaded);

#if CONFIG_SAMPLE_COMPUTE_HASH
		uint8_t hash[32];
		uint8_t hash_str[64 + 1];

		mbedtls_sha256_finish(&sha256_ctx, hash);
		mbedtls_sha256_free(&sha256_ctx);

		bin2hex(hash, sizeof(hash), hash_str, sizeof(hash_str));

		printk("SHA256: %s\n", hash_str);

#if CONFIG_SAMPLE_COMPARE_HASH
		if (strcmp(hash_str, CONFIG_SAMPLE_SHA256_HASH)) {
			printk("Expect: %s\n", CONFIG_SAMPLE_SHA256_HASH);
			printk("SHA256 mismatch!\n");
		}
#endif /* CONFIG_SAMPLE_COMPARE_HASH */
#endif /* CONFIG_SAMPLE_COMPUTE_HASH */

		lte_lc_power_off();
		k_sem_give(&download_sem);
		return 0;

	case DOWNLOAD_CLIENT_EVT_ERROR:
		printk("Error %d during download\n", event->error);
		lte_lc_power_off();
		/* Stop download */
		return -1;
	}

	return 0;
}

void main(void)
{
	int err;

	LOG_INF("Modem trace backend sample started\n");

	err = dk_buttons_init(button_handler);
	if (err) {
		LOG_ERR("Failed to initialize DK buttons library, error: %d\n", err);
	}

	if (!device_is_ready(uart_dev)) {
		LOG_ERR("uart1 device not found/ready!");
	}

	k_sem_init(&download_sem, 0, 1);

	nrf_modem_lib_init(NORMAL_MODE);

	err = nrf_modem_lib_trace_level_set(NRF_MODEM_LIB_TRACE_LEVEL_FULL);
	if (err) {
		LOG_ERR("Failed to enable modem traces");
	}

	for (int i = 0; i < 5; i++) {

		c_downloaded = 0;
		LOG_INF("Connecting to network\n");

		err = lte_lc_init_and_connect();
		if (err) {
			printk("Failed to connect to the LTE network, err %d\n", err);
			return;
		}

		printk("Initializing download client\n", URL);
		err = download_client_init(&downloader, callback);
		if (err) {
			printk("Failed to initialize the client, err %d", err);
			return;
		}

	#if CONFIG_SAMPLE_COMPUTE_HASH
		mbedtls_sha256_init(&sha256_ctx);
		mbedtls_sha256_starts(&sha256_ctx, false);
	#endif

		printk("Connecting to download client\n", URL);

		err = download_client_connect(&downloader, URL, &config);
		if (err) {
			printk("Failed to connect, err %d", err);
			return;
		}

		ref_time = k_uptime_get();

		err = download_client_start(&downloader, URL, STARTING_OFFSET);
		if (err) {
			printk("Failed to start the downloader, err %d", err);
			return;
		}


		printk("Downloading %s\n", URL);

		k_sem_take(&download_sem, K_SECONDS(120));

		download_client_disconnect(&downloader);

		err = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_POWER_OFF);
		if (err) {
			LOG_ERR("Failed to change LTE mode, err %d\n", err);
			return;
		}
	}

	/* Give the modem some time to turn off and receive traces */
	k_sleep(K_SECONDS(5));

	err = nrf_modem_lib_trace_level_set(NRF_MODEM_LIB_TRACE_LEVEL_OFF);
	if (err) {
		LOG_ERR("Failed to turn off modem traces\n");
	} else {
		LOG_INF("Turned off modem traces\n");
	}

	/* Changing the trace level to off will produce some traces, so sleep long enough to
	 * receive those as well.
	 */
	k_sleep(K_SECONDS(1));

	LOG_INF("Press button 1 or enter 'read' to print traces to UART");


	k_sleep(K_FOREVER);
}
