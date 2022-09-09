/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <modem/nrf_modem_lib.h>
#include <modem/nrf_modem_lib_trace.h>
#include <modem/trace_backend.h>
#include <nrf_modem.h>
#include <nrf_modem_at.h>
#include <nrf_modem_os.h>
#include <nrf_modem_trace.h>
#include <nrf_errno.h>

LOG_MODULE_REGISTER(nrf_modem_lib_trace, CONFIG_NRF_MODEM_LIB_LOG_LEVEL);

NRF_MODEM_LIB_ON_INIT(trace_init, trace_init_callback, NULL);

K_SEM_DEFINE(trace_sem, 0, 1);
K_SEM_DEFINE(trace_done_sem, 1, 1);

#if CONFIG_SIZE_OPTIMIZATIONS
#define TRACE_THREAD_STACK_SIZE 768
#else
/* Need more stack when optimizations are disabled */
#define TRACE_THREAD_STACK_SIZE 1024
#endif

#define TRACE_THREAD_PRIORITY                                                                      \
	COND_CODE_1(CONFIG_NRF_MODEM_LIB_TRACE_THREAD_PRIO_OVERRIDE,                               \
		    (CONFIG_NRF_MODEM_LIB_TRACE_THREAD_PRIO), (K_LOWEST_APPLICATION_THREAD_PRIO))

static int trace_init(void);
static int trace_deinit(void);

static double min_bps = DOUBLE_MAX;
static double max_bps;
static double tot_bps;
static int num_bps;

#if CONFIG_NRF_MODEM_LIB_TRACE_LOG_BITRATE
#define LOG_PERIOD_MSEC K_MSEC(CONFIG_NRF_MODEM_LIB_TRACE_LOG_BITRATE_PERIOD_MSEC)

static void log_bitrate_stats(struct k_work *item);

K_WORK_DELAYABLE_DEFINE(log_bitrate_stats_work, log_bitrate_stats);

static void log_bitrate_stats(struct k_work *item)
{
	LOG_INF("Trace avg bitrate (bps): %10.2lf", tot_bps / num_bps);

	k_work_schedule(&log_bitrate_stats_work,
			LOG_PERIOD_MSEC);
}
#endif

static void update_bps(double bps)
{
	if (bps < min_bps) {
		min_bps = bps;
	}

	if (bps > max_bps) {
		max_bps = bps;
	}

	tot_bps += bps;
	num_bps++;
}

int nrf_modem_lib_trace_backend_bitrate_get(double *min_val, double *max_val, double *avg_val)
{
	if (min_val == NULL || max_val == NULL || avg_val == NULL) {
		return -EINVAL;
	}

	*min_val = min_bps;
	*max_val = max_bps;
	*avg_val = tot_bps / num_bps;

	return 0;
}

int nrf_modem_lib_trace_processing_done_wait(k_timeout_t timeout)
{
	int err;

	err = k_sem_take(&trace_done_sem, timeout);
	if (err) {
		return err;
	}

	k_sem_give(&trace_done_sem);

	return 0;
}

static int trace_fragment_write(struct nrf_modem_trace_data *frag)
{
	int ret;
	size_t remaining = frag->len;
	uint32_t start;
	uint32_t end;
	double bps;

	while (remaining) {
		start = k_uptime_get_32();

		ret = trace_backend_write((void *)((uint8_t *)frag->data + frag->len - remaining),
					  remaining);

		end = k_uptime_get_32();

		if (ret < 0) {
			LOG_ERR("trace_backend_write failed with err: %d", ret);

			return ret;
		}

		if (ret == 0) {
			LOG_WRN("trace_backend_write wrote 0 bytes.");
		}

		/* Prevent division by zero (sometimes `end` == `start`) */
		if (end != start) {
			bps = ret / (end - start) * 1000 * 8;

			update_bps(bps);
		}

		remaining -= ret;
	}

	return 0;
}

void trace_thread_handler(void)
{
	int err;
	struct nrf_modem_trace_data *frags;
	size_t n_frags;

trace_reset:
	k_sem_take(&trace_sem, K_FOREVER);

	while (true) {
		err = nrf_modem_trace_get(&frags, &n_frags);
		switch (err) {
		case -NRF_ESHUTDOWN:
			LOG_INF("Modem was turned off, no more traces");
			goto out;
		case -NRF_ENODATA:
			LOG_INF("Modem has faulted, coredump output sent trace");
			goto out;
		case -NRF_EINPROGRESS:
			__ASSERT(0, "Error in transport backend");
			goto out;
		}

		for (size_t i = 0; i < n_frags; i++) {
			err = trace_fragment_write(&frags[i]);
			if (err) {
				goto out;
			}
		}
	}

out:
	err = trace_deinit();
	if (err) {
		LOG_ERR("trace_deinit failed with err: %d", err);
	}

	goto trace_reset;
}

static int trace_init(void)
{
	int err;

	k_sem_take(&trace_done_sem, K_FOREVER);

	err = trace_backend_init(nrf_modem_trace_processed);
	if (err) {
		LOG_ERR("trace_backend_init failed with err: %d", err);

		return err;
	}

	LOG_INF("Trace tread ready");
	k_sem_give(&trace_sem);


#if CONFIG_NRF_MODEM_LIB_TRACE_LOG_BITRATE
	k_work_schedule(&log_bitrate_stats_work,
			LOG_PERIOD_MSEC);
#endif

	return 0;
}

static void trace_init_callback(int err, void *ctx)
{
	if (err) {
		return;
	}

	err = trace_init();
	if (err) {
		LOG_ERR("Failed to initialize trace backend, err: %d", err);
		return;
	}

#if CONFIG_NRF_MODEM_LIB_TRACE_LEVEL_OVERRIDE
	err = nrf_modem_lib_trace_level_set(CONFIG_NRF_MODEM_LIB_TRACE_LEVEL);
	LOG_INF("Trace level override: %d", CONFIG_NRF_MODEM_LIB_TRACE_LEVEL);
	if (err) {
		LOG_ERR("Failed to start tracing, err %d", err);
	}
#else
	LOG_INF("Trace level untouched");
#endif
}

static int trace_deinit(void)
{
	int err;

	err = trace_backend_deinit();
	if (err) {
		LOG_ERR("trace_backend_deinit failed with err: %d", err);

		return err;
	}

	k_sem_give(&trace_done_sem);

	return 0;
}

int nrf_modem_lib_trace_level_set(enum nrf_modem_lib_trace_level trace_level)
{
	int err;
	/* Casting to integer to remove any assumptions on the type of the enum
	 * (could be `char` or `int`) when `printf` expects exactly `int`.
	 */
	int tl = trace_level;

	if (tl) {
		err = nrf_modem_at_printf("AT%%XMODEMTRACE=1,%d", tl);
	} else {
		err = nrf_modem_at_printf("AT%%XMODEMTRACE=0");
	}

	if (err) {
		LOG_ERR("Failed to set trace level, err: %d", err);
		return -ENOEXEC;
	}

	return 0;
}

K_THREAD_DEFINE(trace_thread, TRACE_THREAD_STACK_SIZE, trace_thread_handler,
	       NULL, NULL, NULL, TRACE_THREAD_PRIORITY, 0, 0);
