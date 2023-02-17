/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <zephyr/kernel.h>

#include <haly/nrfy_dppi.h>
#include <hal/nrf_ppib.h>

#include "nrf_802154_platform_sl_lptimer_grtc_events.h"
#include "platform/nrf_802154_platform_sl_lptimer.h"

/* To trigger GRTC.TASKS_CAPTURE[#cc] with RADIO.EVENT_{?}, the following connection chain must be
 * created:
 *    - starting from LOCAL (RADIO core) domain:
 *        {a} RADIO.EVENT_{?}  --> DPPIC_020
 *        {b} DPPIC_020        --> IPCT_radio
 *    - entering the GLOBAL "Main power" domain (G1):
 *        {c} IPCT_radio       --> IPCT_130
 *        {d} IPCT_130         --> DPPIC_130
 *        {e} DPPIC_130        --> PPIB_130
 *        {f} PPIB_130         --> PPIB_133
 *    - ending in the GLOBAL "Active power" domain (G2):
 *        {g} PPIB_133         --> DPPIC_132
 *        {h} DPPIC_132        --> GRTC.CC
 */

/* Peripherals used for timestamping - located in radio power domain (_R_) */
/*   - DPPIC_L : DPPIC10 */
#define DPPIC_R_INST                 NRF_DPPIC10
#define PPIB_R_INST                  NRF_PPIB11

/* Peripherals used for timestamping - located in peripheral power domain (_P_) */

/*   - DPPIC_P : DPPIC20 */
#define DPPIC_P_INST                NRF_DPPIC20
#define DPPIC_P_TS_CHANNEL          0 /* must be in the range [0..7] to satisfy the requirements for the dependent macros */

/*  - PPIB_P : PPIB21 */
#define PPIB_P_INST                 NRF_PPIB21

static uint32_t m_hw_task_ppib_ch = NRF_802154_SL_HW_TASK_PPI_INVALID;

void nrf_802154_platform_sl_lptimer_dynamic_event_for_timestamps_set(uint32_t dppi_ch,
                                                                     nrf_grtc_task_t cc_channel)
{
    // {a} Connection: RADIO.EVENT_{?} to DPPIC_020[dppi_ch]
    // It is the responsibility of the user of this platform to make the {a} connection
    // and pass the DPPI channel number as a parameter here.

    // {b} Connect: DPPIC_020[dppi_ch] to IPCT_radio
    nrf_ppib_subscribe_set(PPIB_R_INST, nrf_ppib_send_task_get(dppi_ch), dppi_ch);
    nrf_ppib_publish_set(PPIB_P_INST, nrf_ppib_receive_event_get(dppi_ch), dppi_ch);

    nrfy_grtc_subscribe_set(NRF_GRTC, cc_channel, dppi_ch);

    nrfy_dppi_channels_enable(DPPIC_P_INST, 1UL << dppi_ch);
}

void nrf_802154_platform_sl_lptimer_static_event_for_timestamps_set(nrf_grtc_task_t cc_channel)
{

}

void nrf_802154_platform_sl_lptimer_dynamic_event_for_hw_tasks_set(uint32_t dppi_ch,
                                                                   nrf_grtc_event_t cc_channel)
{
    // {a} Connection: DPPIC_020[dppi_ch] to RADIO.TASKS_{?}
    // It is the responsibility of the user of this platform to make the {a} connection
    // and pass the DPPI channel number as a parameter here.

    if (dppi_ch == NRF_802154_SL_HW_TASK_PPI_INVALID)
    {
        return;
    }

    m_hw_task_ppib_ch = dppi_ch;

    nrf_ppib_subscribe_set(PPIB_R_INST, nrf_ppib_send_task_get(dppi_ch), dppi_ch);
    nrf_ppib_publish_set(PPIB_P_INST, nrf_ppib_receive_event_get(dppi_ch), dppi_ch);
    nrfy_grtc_publish_set(NRF_GRTC, cc_channel, dppi_ch);
    nrfy_dppi_channels_enable(DPPIC_P_INST, 1UL << dppi_ch);
}

void nrf_802154_platform_sl_lptimer_static_event_for_hw_tasks_set(nrf_grtc_event_t cc_channel)
{
    // Intentionally empty
}

void nrf_802154_platform_sl_lptimer_dynamic_event_for_hw_tasks_clear(void)
{
    nrf_ppib_subscribe_clear(PPIB_R_INST, nrf_ppib_send_task_get(m_hw_task_ppib_ch));
    nrf_ppib_publish_clear(PPIB_P_INST, nrf_ppib_receive_event_get(m_hw_task_ppib_ch));
    nrfy_dppi_channels_disable(DPPIC_P_INST, 1UL << m_hw_task_ppib_ch);
}
