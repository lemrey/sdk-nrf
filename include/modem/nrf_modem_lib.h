/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <nrf_modem.h>

/**
 * @brief Initialize the Modem library.
 *
 * This function is synchrounous and it could block
 * for a few minutes when the modem firmware is being updated.
 *
 * If you application supports modem firmware updates, consider
 * initializing the library manually to have control of what
 * the application should do while initialization is ongoing.
 *
 * Library has two operation modes, normal and DFU.
 * In normal operation mode, the DFU functionality should is disabled.
 *
 * Library can alternatively be initialized in DFU mode, which means that
 * all shared memory regions are now reserved for DFU operation,
 * and therefore no other modem functionality can be used.
 *
 * To switch between DFU and normal modes, nrf_modem_shutdown() should be
 * called in between.
 *
 * @param[in] mode Library mode.
 *
 * @return int Zero on success, non-zero otherwise.
 */
int nrf_modem_lib_init(enum nrf_modem_mode_t mode);

/**
 * @brief Makes a thread sleep until next time nrf_modem_lib_init() is called.
 *
 * When nrf_modem_lib_shutdown() is called a thread can call this function to be
 * woken up next time nrf_modem_lib_init() is called.
 */
void nrf_modem_lib_shutdown_wait(void);

/**
 * @brief Get the last return value of nrf_modem_lib_init.
 *
 * This function can be used to access the last return value of
 * nrf_modem_lib_init. This can be used to check the state of a modem
 * firmware exchange when the Modem library was initialized at boot-time.
 *
 * @return int The last return value of nrf_modem_lib_init.
 */
int nrf_modem_lib_get_init_ret(void);

/**
 * @brief Shutdown the Modem library, releasing its resources.
 *
 * @return int Zero on success, non-zero otherwise.
 */
int nrf_modem_lib_shutdown(void);

#ifdef __cplusplus
}
#endif
