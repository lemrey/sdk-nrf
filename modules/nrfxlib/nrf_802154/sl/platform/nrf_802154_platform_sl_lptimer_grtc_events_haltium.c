/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <kernel.h>

#include <haly/nrfy_dppi.h>
#include <hal/nrf_ipct.h>
#include "nrfx_missing_pieces.h" /* TODO: remove when nrfx is patched */

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

/* Peripherals used for timestamping - located in local domain (_L_) */
/*   - DPPIC_L : DPPIC020 (RADIOCORE.DPPIC020) */
#define DPPIC_L_INST                 NRF_DPPIC020

/*   - IPCT_L : NRF_IPCT (RADIOCORE.IPCT) */
#define IPCT_L_TS_CHANNEL            0
#define IPCT_L_TASK_SEND             NRFX_CONCAT_2(NRF_IPCT_TASK_SEND_, IPCT_L_TS_CHANNEL)

/* Peripherals used for timestamping - located in global "Main power domain" (_G1_) */
/*   - IPCT_G1 : IPCT130_S */
#define IPCT_G1_INST                 NRF_IPCT130_S
#define IPCT_G1_TS_CHANNEL           0
#define IPCT_G1_SHORTS               IPCT_SHORTS_RECEIVE0_ACK0_Msk
#define IPCT_G1_EVENT_RECEIVE        NRFX_CONCAT_2(NRF_IPCT_EVENT_RECEIVE_, IPCT_G1_TS_CHANNEL)

/*   - DPPIC_G1 : DPPIC130_S */
#define DPPIC_G1_INST                NRF_DPPIC130_S
#define DPPIC_G1_TS_CHANNEL          0 /* must be in the range [0..7] to satisfy the requirements for the dependent macros */

/*  - PPIB_G1 : PPIB130_S */
#define PPIB_G1_TS_CHANNEL           (DPPIC_G1_TS_CHANNEL + 8) /* hw-fixed dependency */

/* Peripherals used for timestamping - located in global "Active power domain" (_G2_) */
/*   - PPIB_G2 : PPIB133_S */
#define PPIB_G2_TS_CHANNEL           (PPIB_G1_TS_CHANNEL - 8) /* hw-fixed dependency */

/*   - DPPIC_G2 : DPPIC132_S */
#define DPPIC_G2_INST                NRF_DPPIC132_S
#define DPPIC_G2_TS_CHANNEL          (PPIB_G2_TS_CHANNEL + 0) /* hw-fixed dependency */
#define DPPIC_G2_CHG_NUM             0
#define DPPIC_G2_CH_GROUP            NRFX_CONCAT_2(NRF_DPPI_CHANNEL_GROUP, DPPIC_G2_CHG_NUM)
#define DPPIC_G2_TASK_DISABLE        NRFX_CONCAT_3(NRF_DPPI_TASK_CHG, DPPIC_G2_CHG_NUM, _DIS)

#if (PPIB_G1_TS_CHANNEL < 8) || (PPIB_G1_TS_CHANNEL > 15)
#error PPIB_G1_TS_CHANNEL is required to be in the [8..15] range
/* The requirement is because only this range of channels can be connected to PPIB133 */
#endif

void nrf_802154_platform_sl_lptimer_dynamic_event_for_timestamps_set(uint32_t dppi_ch,
                                                                     nrf_grtc_task_t cc_channel)
{
    // {a} Connection: RADIO.EVENT_{?} to DPPIC_020[dppi_ch]
    // It is the responsibility of the user of this platform to make the {a} connection
    // and pass the DPPI channel number as a parameter here.

    // {b} Connect: DPPIC_020[dppi_ch] to IPCT_radio
    nrf_ipct_subscribe_set(NRF_IPCT, IPCT_L_TASK_SEND, dppi_ch);

    nrfy_dppi_channels_enable(DPPIC_G2_INST, 1UL << DPPIC_G2_TS_CHANNEL);
}

void nrf_802154_platform_sl_lptimer_static_event_for_timestamps_set(nrf_grtc_task_t cc_channel)
{
    // {c} Connection: IPCT_radio to IPCT_130
    // It is assumed that this connection has already been made by SECURE-core as a result of
    // configuring of the UICR->IPCMAP[] registers.

    // Enable auto confirmations on destination IPCT_130
    nrfy_ipct_shorts_enable(IPCT_G1_INST, IPCT_G1_SHORTS);

    // {d} Connect: IPCT_130 to DPPIC_130
    nrf_ipct_publish_set(IPCT_G1_INST, IPCT_G1_EVENT_RECEIVE, DPPIC_G1_TS_CHANNEL);

    // {e} Connection: DPPIC_130 to PPIB_130
    // It is assumed that this connection has already been made by SECURE-core as a result of
    // configuring the UICR->DPPI.GLOBAL[].CH.LINK.SOURCE registers.

    // {f} Connection: PPIB_130 to PPIB_133
    // One of hw-fixed connections (channels [8..15] --> [0..7]), so nothing to do.

    // Enable: DPPIC_130
    nrfy_dppi_channels_enable(DPPIC_G1_INST, 1UL << DPPIC_G1_TS_CHANNEL);

    // {g} Connection: PPIB_133 to DPPIC_132
    // It is assumed that this connection has already been made by SECURE-core as a result of
    // configuring of the UICR->DPPI.GLOBAL[].CH.LINK.SINK registers.

    // {h} Connect: DPPIC_132 to GRTC.CC
    nrfy_grtc_subscribe_set(NRF_GRTC, cc_channel, DPPIC_G2_TS_CHANNEL);

    // Enable: DPPIC_132
    nrfy_dppi_channels_enable(DPPIC_G2_INST, 1UL << DPPIC_G2_TS_CHANNEL);

    nrfy_dppi_channels_include_in_group(DPPIC_G2_INST,
                                        1UL << DPPIC_G2_TS_CHANNEL,
                                        DPPIC_G2_CH_GROUP);

    // Configure automatic DPPI disconnection, triggered by its own conducted signal
    nrfy_dppi_subscribe_set(DPPIC_G2_INST, DPPIC_G2_TASK_DISABLE, DPPIC_G2_TS_CHANNEL);
}

/* To trigger RADIO.TASKS_{?} with GRTC.EVENT_COMPARE[#cc] the following connection chain must be
 * created:
 *    - starting from LOCAL (RADIO core) domain:
 *        {a} RADIO.TASKS_{?}  <-- DPPIC_020
 *        {b} DPPIC_020        <-- IPCT_radio
 *    - entering the GLOBAL "Main power" domain (G1):
 *        {c} IPCT_radio       <-- IPCT_130
 *        {d} IPCT_130         <-- DPPIC_130
 *        {e} DPPIC_130        <-- PPIB_130
 *        {f} PPIB_130         <-- PPIB_133
 *    - ending in the GLOBAL "Active power" domain (G2):
 *        {g} PPIB_133         <-- DPPIC_132
 *        {h} DPPIC_132        <-- GRTC.CC
 */

/* Peripherals used for hardware tasks - located in local domain (_L_) */
/*   - DPPIC_L : DPPIC020 (RADIOCORE.DPPIC020) */
/*   - IPCT_L : NRF_IPCT (RADIOCORE.IPCT) */
#define IPCT_L_HT_CHANNEL            1
#define IPCT_L_SHORTS                IPCT_SHORTS_RECEIVE1_ACK1_Msk
#define IPCT_L_EVENT_RECEIVE         NRFX_CONCAT_2(NRF_IPCT_EVENT_RECEIVE_, IPCT_L_HT_CHANNEL)

/* Peripherals used for timestamping - located in global "Main power domain" (_G1_) */
/*   - IPCT_G1 : IPCT130_S */
#define IPCT_G1_HT_CHANNEL           1
#define IPCT_G1_TASK_SEND            NRFX_CONCAT_2(NRF_IPCT_TASK_SEND_, IPCT_G1_HT_CHANNEL)

/*   - DPPIC_G1 : DPPIC130_S */
#define DPPIC_G1_HT_CHANNEL          1 /* must be in the range [0..7] to satisfy the requirements for the dependent macros */

/*  - PPIB_G1 : PPIB130_S */
#define PPIB_G1_HT_CHANNEL           (DPPIC_G1_HT_CHANNEL + 8) /* hw-fixed dependency */

/* Peripherals used for timestamping - located in global "Active power domain" (_G2_) */
/*   - PPIB_G2 : PPIB133_S */
#define PPIB_G2_HT_CHANNEL           (PPIB_G1_HT_CHANNEL - 8) /* hw-fixed dependency */

/*   - DPPIC_G2 : DPPIC132_S */
#define DPPIC_G2_HT_CHANNEL          (PPIB_G2_HT_CHANNEL + 0) /* hw-fixed dependency */

#if (PPIB_G1_HT_CHANNEL < 8) || (PPIB_G1_HT_CHANNEL > 15)
#error PPIB_G1_HT_CHANNEL is required to be in the [8..15] range
/* The requirement is because only this range of channels can be connected to PPIB133 */
#endif

void nrf_802154_platform_sl_lptimer_dynamic_event_for_hw_tasks_set(uint32_t dppi_ch,
                                                                   nrf_grtc_event_t cc_channel)
{
    nrf_ipct_event_clear(NRF_IPCT, IPCT_L_EVENT_RECEIVE);

    // {a} Connection: DPPIC_020[dppi_ch] to RADIO.TASKS_{?}
    // It is the responsibility of the user of this platform to make the {a} connection
    // and pass the DPPI channel number as a parameter here.

    if (dppi_ch == NRF_802154_SL_HW_TASK_PPI_INVALID)
    {
        return;
    }

    // {b} Connect: IPCT_radio to DPPIC_020[dppi_ch]
    nrf_ipct_publish_set(NRF_IPCT, IPCT_L_EVENT_RECEIVE, dppi_ch);

    nrfy_dppi_channels_enable(DPPIC_G2_INST, 1UL << DPPIC_G2_HT_CHANNEL);
}

void nrf_802154_platform_sl_lptimer_static_event_for_hw_tasks_set(nrf_grtc_event_t cc_channel)
{
    // {c} Connection: IPCT_radio to IPCT_130
    // It is assumed that this connection has already been made by SECURE-core as a result of
    // configuring of the UICR->IPCMAP[] registers.

    // Enable auto confirmations on destination IPCT_radio
    nrfy_ipct_shorts_enable(NRF_IPCT, IPCT_L_SHORTS);

    // {d} Connect: IPCT_130 to DPPIC_130
    nrf_ipct_subscribe_set(IPCT_G1_INST, IPCT_G1_TASK_SEND, DPPIC_G1_HT_CHANNEL);

    // {e} Connection: DPPIC_130 to PPIB_130
    // It is assumed that this connection has already been made by SECURE-core as a result of
    // configuring the UICR->DPPI.GLOBAL[].CH.LINK.SOURCE registers.

    // {f} Connection: PPIB_130 to PPIB_133
    // One of hw-fixed connections (channels [8..15] --> [0..7]), so nothing to do.

    // Enable: DPPIC_130
    nrfy_dppi_channels_enable(DPPIC_G1_INST, 1UL << DPPIC_G1_HT_CHANNEL);

    // {g} Connection: PPIB_133 to DPPIC_132
    // It is assumed that this connection has already been made by SECURE-core as a result of
    // configuring of the UICR->DPPI.GLOBAL[].CH.LINK.SINK registers.

    // {h} Connect: DPPIC_132 to GRTC.CC
    nrfy_grtc_publish_set(NRF_GRTC, cc_channel, DPPIC_G2_HT_CHANNEL);
}

void nrf_802154_platform_sl_lptimer_dynamic_event_for_hw_tasks_clear(void)
{
    nrf_ipct_publish_clear(NRF_IPCT, IPCT_L_EVENT_RECEIVE);
    nrfy_dppi_channels_disable(DPPIC_G2_INST, 1UL << DPPIC_G2_HT_CHANNEL);
    nrf_ipct_event_clear(NRF_IPCT, IPCT_L_EVENT_RECEIVE);
}
