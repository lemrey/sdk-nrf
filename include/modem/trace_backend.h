/*
 * Copyright (c) 2022-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef TRACE_BACKEND_H__
#define TRACE_BACKEND_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file trace_backend.h
 *
 * @defgroup trace_backend nRF91 Modem trace backend interface.
 * @{
 */

/** @brief callback to signal the trace module that
 * some amount of trace data has been processed.
 */
typedef int (*trace_backend_processed_cb)(size_t len);

/**
 * @brief The trace backend interface, implemented by the trace backend.
 */
struct trace_backend {
	/**
	 * @brief Initialize the compile-time selected trace backend.
	 *
	 * @param trace_processed_cb Function callback for signaling how much of the trace data has
	 *                           been processed by the backend.
	 *
	 * @return 0 If the operation was successful.
	 *           Otherwise, a (negative) error code is returned.
	 */
	int (*init)(trace_backend_processed_cb trace_processed_cb);

	/**
	 * @brief Deinitialize the compile-time selected trace backend.
	 *
	 * @return 0 If the operation was successful.
	 *           Otherwise, a (negative) error code is returned.
	 */
	int (*deinit)(void);

	/**
	 * @brief Write trace data to the compile-time selected trace backend.
	 *
	 * @param data Memory buffer containing modem trace data.
	 * @param len  Memory buffer length.
	 *
	 * @returns Number of bytes written if the operation was successful.
	 *          Otherwise, a (negative) error code is returned.
	 *          Especially,
	 *          -ENOSPC if no space is available and the backend has to be cleared before
	 *                  tracing can continue.
	 */
	int (*write)(const void *data, size_t len);

	/**
	 * @brief Read trace data from flash storage
	 *
	 * @details Read out the trace data from flash memory.
	 *
	 * @param buf Output buffer
	 * @param len Size of output buffer
	 *
	 * @return 0 on success, negative errno on failure.
	 */
	int (*read)(void *buf, size_t len);

	/**
	 * @brief Erase captured trace data
	 *
	 * @details Erase all captured trace data and prepare the backend for capturing new traces.
	 *
	 * @note This operation is only supported with some trace backends.
	 *
	 * @return 0 on success, negative errno on failure.
	 */
	int (*clear)(void);
};

/**@} */ /* defgroup trace_backend */

#ifdef __cplusplus
}
#endif

#endif /* TRACE_BACKEND_H__ */
