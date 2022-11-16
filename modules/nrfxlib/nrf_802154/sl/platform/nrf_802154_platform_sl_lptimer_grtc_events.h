/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Helper module for abstracting DPPI connections with GRTC compare/capture channels.
 */

#ifndef NRF_802154_PLATFORM_SL_LPTIMER_EVENTS_H_
#define NRF_802154_PLATFORM_SL_LPTIMER_EVENTS_H_

#include <stdint.h>
#include <haly/nrfy_grtc.h>

#ifdef __cplusplus
extern "C" {
#endif

/**@brief Sets up DPPI connections for capturing a timestamp.
 *
 * @param[in] dppi_ch    DPPI channel to which timestamped event shall publish.
 * @param[in] cc_channel The GRTC capture channel that will capture the timestmap.
 */
void nrf_802154_platform_sl_lptimer_dynamic_event_for_timestamps_set(uint32_t dppi_ch,
                                                                     nrf_grtc_task_t cc_channel);

/**@brief Sets up static DPPI connections for capturing a timestamp.
 *
 * This function configures static DPPI connections for capturing a timestamp.
 * These connections never change.
 *
 * @param[in] cc_channel The GRTC capture channel that will capture the timestmap.
 */
void nrf_802154_platform_sl_lptimer_static_event_for_timestamps_set(nrf_grtc_task_t cc_channel);

/**@brief Sets up DPPI connections for triggering a hardware task using a GRTC event.
 *
 * @param[in] dppi_ch    DPPI channel to which GRTC compare channel shall publish its event.
 * @param[in] cc_channel The GRTC compare channel that will generate the event.
 */
void nrf_802154_platform_sl_lptimer_dynamic_event_for_hw_tasks_set(uint32_t dppi_ch,
                                                                   nrf_grtc_event_t cc_channel);

/**@brief Sets up static DPPI connections for triggering a hardware task using a GRTC event.
 *
 * This function configures static DPPI connections for capturing a timestamp.
 * These connections never change.
 *
 * @param[in] cc_channel The GRTC compare channel that will generate the event.
 */
void nrf_802154_platform_sl_lptimer_static_event_for_hw_tasks_set(nrf_grtc_event_t cc_channel);

/**@brief Clears the DPPI connections for triggering a hardware task using a GRTC event.
 *
 * @param[in] dppi_ch    DPPI channel to which GRTC compare channel shall publish its event.
 * @param[in] cc_channel The GRTC compare channel that will generate the event.
 */
void nrf_802154_platform_sl_lptimer_dynamic_event_for_hw_tasks_clear(void);

#ifdef __cplusplus
}
#endif

#endif // NRF_802154_PLATFORM_SL_LPTIMER_EVENTS_H_
