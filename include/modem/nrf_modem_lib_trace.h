/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_MODEM_LIB_TRACE_H__
#define NRF_MODEM_LIB_TRACE_H__

#include <zephyr.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file nrf_modem_lib_trace.h
 *
 * @defgroup nrf_modem_lib_trace nRF91 Modem trace module
 * @{
 */

/** @brief Trace level
 *
 * The trace level can be used to filter the traces.
 */
enum nrf_modem_lib_trace_level {
	NRF_MODEM_LIB_TRACE_LEVEL_COREDUMP_ONLY = 1,	/**< Coredump only. */
	NRF_MODEM_LIB_TRACE_LEVEL_FULL = 2,		/**< LTE, IP, GNSS, and coredump. */
	NRF_MODEM_LIB_TRACE_LEVEL_IP_ONLY = 4,		/**< IP. */
	NRF_MODEM_LIB_TRACE_LEVEL_LTE_AND_IP = 5,	/**< LTE and IP. */
};

/** @brief Wait for trace to have finished processing after coredump or shutdown.
 *
 * This function blocks until the trace module has finished processing data after
 * a modem fault (coredump) or modem shutdown.
 *
 * @param timeout Time to wait for trace processing to be done.
 *
 * @return Zero on success, non-zero otherwise.
 */
int nrf_modem_lib_trace_processing_done_wait(k_timeout_t timeout);

/** @brief Set trace level and start/resume a trace session.
 *
 * This function sends an AT command that sets the trace level and starts/resumes tracing.
 *
 * @param trace_level Trace level
 *
 * @return Zero on success, non-zero otherwise.
 */
int nrf_modem_lib_trace_level_set(enum nrf_modem_lib_trace_level trace_level);

/** @brief Resume a trace session.
 *
 * This function resumes an ongoing trace session.
 *
 * @return Zero on success, non-zero otherwise.
 */
int nrf_modem_lib_trace_resume(void);

/** @brief Pause an ongoing trace session.
 *
 * This function pauses an ongoing trace session.
 *
 * @return Zero on success, non-zero otherwise.
 */
int nrf_modem_lib_trace_pause(void);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* NRF_MODEM_LIB_TRACE_H__ */
