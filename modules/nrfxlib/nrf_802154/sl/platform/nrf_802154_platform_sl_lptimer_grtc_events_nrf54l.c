/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/timer/nrf_grtc_timer.h>

#include <haly/nrfy_dppi.h>
#include <haly/nrfy_grtc.h>
#include <hal/nrf_ppib.h>

#include "nrf_802154_platform_sl_lptimer_grtc_events.h"
#include "platform/nrf_802154_platform_sl_lptimer.h"

/* To trigger GRTC.TASKS_CAPTURE[#cc] with RADIO.EVENT_{?}, the following connection chain must be
 * created:
 *    - starting from RADIO domain (_R_):
 *        {a} RADIO.EVENT_{?}  --> DPPIC_10
 *        {b} DPPIC_10         --> PPIB_11
 *    - crossing domain boundaries
 *        {c} PPIB_11          --> PPIB_21
 *    - ending in the PERI domain (_P_):
 *        {d} PPIB_21          --> DPPIC_20
 *        {e} DPPIC_20         --> GRTC.CC
 */

/* Peripherals used for timestamping - located in radio power domain (_R_) */
/*   - DPPIC_L : DPPIC10 */
#define DPPIC_R_INST                NRF_DPPIC10
#define PPIB_R_INST                 NRF_PPIB11

/* Peripherals used for timestamping - located in peripheral power domain (_P_) */

/*   - DPPIC_P : DPPIC20 */
#define DPPIC_P_INST                NRF_DPPIC20

/*  - PPIB_P : PPIB21 */
#define PPIB_P_INST                 NRF_PPIB21

static uint32_t m_hw_task_ppib_ch = NRF_802154_SL_HW_TASK_PPI_INVALID;

void nrf_802154_platform_sl_lptimer_dynamic_event_for_timestamps_set(uint32_t dppi_ch,
                                                                     uint32_t cc_channel)
{
    // {a} Connection: RADIO.EVENT_{?} to DPPIC_10
    // It is the responsibility of the user of this platform to make the {a} connection
    // and pass the DPPI channel number as a parameter here.

    // {b} Connect: DPPIC_10 to PPIB_11
    nrf_ppib_subscribe_set(PPIB_R_INST, nrf_ppib_send_task_get(dppi_ch), dppi_ch);

    // {c} Connection: PPIB_11 to PPIB_21: One of hw-fixed connections, so nothing to do.

    // {d} Connect: PPIB_21 to DPPIC_20
    nrf_ppib_publish_set(PPIB_P_INST, nrf_ppib_receive_event_get(dppi_ch), dppi_ch);

    // {e} Connect: DPPIC_20[dppi_ch] to GRTC.CC[cc_channel]
    nrf_grtc_task_t capture_task = nrfy_grtc_sys_counter_capture_task_get(cc_channel);
    NRF_DPPI_ENDPOINT_SETUP(nrfy_grtc_task_address_get(NRF_GRTC, capture_task), dppi_ch);

    nrfy_dppi_channels_enable(DPPIC_P_INST, 1UL << dppi_ch);
}

void nrf_802154_platform_sl_lptimer_static_event_for_timestamps_clear(uint32_t cc_channel)
{
    nrf_grtc_task_t capture_task = nrfy_grtc_sys_counter_capture_task_get(cc_channel);
    NRF_DPPI_ENDPOINT_CLEAR(nrfy_grtc_task_address_get(NRF_GRTC, capture_task));
}

void nrf_802154_platform_sl_lptimer_static_event_for_timestamps_set(uint32_t cc_channel)
{
    /* Intentionally empty */
}

void nrf_802154_platform_sl_lptimer_dynamic_event_for_hw_tasks_set(uint32_t dppi_ch,
                                                                   uint32_t cc_channel)
{
    // {a} Connection: DPPIC_10[dppi_ch] to RADIO.TASKS_{?}
    // It is the responsibility of the user of this platform to make the {a} connection
    // and pass the DPPI channel number as a parameter here.

    if (dppi_ch == NRF_802154_SL_HW_TASK_PPI_INVALID)
    {
        return;
    }

    m_hw_task_ppib_ch = dppi_ch;

    nrf_ppib_publish_set(PPIB_R_INST, nrf_ppib_receive_event_get(dppi_ch), dppi_ch);
    nrf_ppib_subscribe_set(PPIB_P_INST, nrf_ppib_send_task_get(dppi_ch), dppi_ch);

    NRF_DPPI_ENDPOINT_SETUP(z_nrf_grtc_timer_compare_evt_address_get(cc_channel),
                              dppi_ch);

    nrfy_dppi_channels_enable(DPPIC_P_INST, 1UL << dppi_ch);
}

void nrf_802154_platform_sl_lptimer_static_event_for_hw_tasks_set(uint32_t cc_channel)
{
    // Intentionally empty
}

void nrf_802154_platform_sl_lptimer_dynamic_event_for_hw_tasks_clear(void)
{
    uint32_t dppi_ch = m_hw_task_ppib_ch;

    if (dppi_ch != NRF_802154_SL_HW_TASK_PPI_INVALID)
    {
        nrf_ppib_subscribe_clear(PPIB_R_INST, nrf_ppib_send_task_get(dppi_ch));
        nrf_ppib_publish_clear(PPIB_P_INST, nrf_ppib_receive_event_get(dppi_ch));
        nrfy_dppi_channels_disable(DPPIC_P_INST, 1UL << dppi_ch);
    }
}
