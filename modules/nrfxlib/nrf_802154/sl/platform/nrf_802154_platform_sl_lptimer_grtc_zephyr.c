/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "platform/nrf_802154_platform_sl_lptimer.h"

#include <assert.h>
#include <kernel.h>

#include <hal/nrf_ipct.h>
#include <haly/nrfy_dppi.h>
#include <haly/nrfy_grtc.h>
#include "nrfx_missing_pieces.h" /* TODO: remove when nrfx is patched */

#include "platform/nrf_802154_clock.h"
#include "nrf_802154_sl_config.h"
#include "nrf_802154_sl_atomics.h"
#include "nrf_802154_sl_utils.h"
#include "timer/nrf_802154_timer_coord.h"
#include "nrf_802154_sl_periphs.h"

#if defined(HALTIUM_XXAA) && IS_ENABLED(CONFIG_EMULATOR_FPGA)
/* Correction for FPGA clocking being 8x slower than specified in IPS */
#define HALTIUM_FPGA_CLOCKS_DOWNSCALING_BITSHIFT 3 /* (1/8) */
#else
#define HALTIUM_FPGA_CLOCKS_DOWNSCALING_BITSHIFT 0
#endif

#define GRTC_INT_PRIORITY           NRF_802154_SL_RTC_IRQ_PRIORITY

#define GRTC_CALLBACKS_CC_CHANNEL   9 /* value temporarily suitable for RADIOCORE, this may change */
#define GRTC_TIMESTAMPS_CC_CHANNEL  10 /* value temporarily suitable for RADIOCORE, this may change */
#define GRTC_HW_TASKS_CC_CHANNEL    11 /* value temporarily suitable for RADIOCORE, this may change */

#define GRTC_SYS_COUNTER_CMP_EVENT  \
    nrfy_grtc_sys_counter_compare_event_get(GRTC_CALLBACKS_CC_CHANNEL)
#define GRTC_USED_INT_MASK          \
    NRFY_EVENT_TO_INT_BITMASK(GRTC_SYS_COUNTER_CMP_EVENT)
#define GRTC_TASK_CAPTURE_TIMESTAMP \
    NRFX_CONCAT_2(NRF_GRTC_TASK_CAPTURE_, GRTC_TIMESTAMPS_CC_CHANNEL)
#define GRTC_EVENT_HW_TRIGGER       \
    NRFX_CONCAT_2(NRF_GRTC_EVENT_COMPARE_, GRTC_HW_TASKS_CC_CHANNEL)

static bool     m_compare_int_lock_key;
static uint32_t m_critical_section_cnt;

static bool grtc_compare_int_lock(void);
static void grtc_compare_int_unlock(bool key);

ISR_DIRECT_DECLARE(grtc_irq_handler)
{
    uint64_t curr_time = nrfy_grtc_sys_counter_get(NRF_GRTC);
    curr_time <<= HALTIUM_FPGA_CLOCKS_DOWNSCALING_BITSHIFT;

    nrfy_grtc_sys_counter_compare_event_disable(NRF_GRTC, GRTC_CALLBACKS_CC_CHANNEL);

    if (nrfy_grtc_events_process(NRF_GRTC, GRTC_USED_INT_MASK))
    {
        nrf_802154_sl_timer_handler(curr_time);
    }

    ISR_DIRECT_PM(); /* PM done after servicing interrupt for best latency */
    return 1;        /* We should check if scheduling decision should be made */
}

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

#define PPI_TIMESTAMP             NRF_802154_PPI_TIMESTAMP_EVENT_TO_TIMER_CAPTURE

#define PUBLISH_REGISTER_OFFSET   0x100UL          ///< Offset of the publish register relative to its event register
#define PUBLISH_ENABLE_BIT        (0x1UL << 31UL) ///< Bit that enables event publishing to a DPPI channel

static inline void publish_event_to_channel(uint32_t event_addr, uint8_t channel)
{
    volatile uint32_t * p_reg = (volatile uint32_t *)(event_addr + PUBLISH_REGISTER_OFFSET);

    assert((*p_reg == 0) || (*p_reg == (channel | PUBLISH_ENABLE_BIT)));
    *p_reg = (channel | PUBLISH_ENABLE_BIT);
}

static inline void unpublish_event(uint32_t event_addr, uint8_t channel)
{
    volatile uint32_t * p_reg = (volatile uint32_t *)(event_addr + PUBLISH_REGISTER_OFFSET);

    assert((*p_reg == 0) || (*p_reg == (channel | PUBLISH_ENABLE_BIT)));
    *p_reg = 0;
}

static void setup_dynamic_connections_for_radio_timestamps(uint32_t dppi_ch)
{
    // {a} Connection: RADIO.EVENT_{?} to DPPIC_020[dppi_ch]
    // It is the responsibility of the user of this platform to make the {a} connection
    // and pass the DPPI channel number as a parameter here.

    if (dppi_ch == NRF_802154_SL_HW_TASK_PPI_INVALID)
    {
        return;
    }

    // {b} Connect: DPPIC_020[dppi_ch] to IPCT_radio
    nrf_ipct_subscribe_set(NRF_IPCT, IPCT_L_TASK_SEND, dppi_ch);

    nrfy_dppi_channels_enable(DPPIC_G2_INST, 1UL << DPPIC_G2_TS_CHANNEL);
}

static void setup_static_connections_for_radio_timestamps(void)
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
    nrfy_grtc_subscribe_set(NRF_GRTC, GRTC_TASK_CAPTURE_TIMESTAMP, DPPIC_G2_TS_CHANNEL);

    // Enable: DPPIC_132
    nrfy_dppi_channels_enable(DPPIC_G2_INST, 1UL << DPPIC_G2_TS_CHANNEL);

    nrfy_dppi_channels_include_in_group(DPPIC_G2_INST,
                                        1UL << DPPIC_G2_TS_CHANNEL,
                                        DPPIC_G2_CH_GROUP);

    // Configure automatic DPPI disconnection, triggered by its own conducted signal
    nrfy_dppi_subscribe_set(DPPIC_G2_INST, DPPIC_G2_TASK_DISABLE, DPPIC_G2_TS_CHANNEL);

    nrf_dppi_channels_enable(NRF_DPPIC020, (1UL << PPI_TIMESTAMP));
}

static void setup_static_connections_for_radio_hw_tasks(void);

void nrf_802154_platform_sl_lp_timer_init(void)
{
    m_critical_section_cnt = 0UL;

#if 0 /* TODO: when possible use this code instead of a workaround */

    while (nrfy_grtc_event_check(NRF_GRTC, NRF_GRTC_EVENT_SYSCOUNTERVALID));

#elif 1 /* workaround that replaces the EVENT_SYSCOUNTERVALID */
    NRFX_DELAY_US(20000);
#endif

#if IS_ENABLED(CONFIG_NRF_GRTC_TIMER)
    /* The assumption here is that 802.15.4 is the only user of the GRTC IRQn line, therefore
     * it conflicts with the use of the NRF_GRTC_TIMER.
     */
#error "This platform/sl_lptimer implementation is incompatible with Zephyr's nrf_grtc_timer.c"

#else
    /* TODO: This impelmentation should use the Zephyr's GRTC driver to, among other things,
     *     share an GRTC IRQ line with other GRTC users. When this is done, this file will be
     *     ready to be moved to the zephyr/modules/hal_nordic/nrf_802154/sl_opensource/platform
     */
    IRQ_DIRECT_CONNECT(GRTC_1_IRQn, GRTC_INT_PRIORITY, grtc_irq_handler, IRQ_ZERO_LATENCY);
    irq_enable(GRTC_1_IRQn);
#endif

    nrfy_grtc_event_clear(NRF_GRTC, GRTC_SYS_COUNTER_CMP_EVENT);

    setup_static_connections_for_radio_hw_tasks();
    nrf_802154_platform_sl_lptimer_hw_task_cleanup();

    nrfy_grtc_sys_counter_compare_event_enable(NRF_GRTC, GRTC_CALLBACKS_CC_CHANNEL);
}

void nrf_802154_platform_sl_lp_timer_deinit(void)
{
    grtc_compare_int_lock();

    irq_disable(GRTC_1_IRQn);

    nrfy_grtc_sys_counter_compare_event_clear(NRF_GRTC, GRTC_CALLBACKS_CC_CHANNEL);
    nrfy_grtc_sys_counter_compare_event_disable(NRF_GRTC, GRTC_CALLBACKS_CC_CHANNEL);
}

uint64_t nrf_802154_platform_sl_lptimer_current_lpticks_get(void)
{
    return (nrfy_grtc_sys_counter_get(NRF_GRTC) << HALTIUM_FPGA_CLOCKS_DOWNSCALING_BITSHIFT);
}

uint64_t nrf_802154_platform_sl_lptimer_us_to_lpticks_convert(uint64_t us, bool round_up)
{
#if IS_ENABLED(CONFIG_EMULATOR_FPGA)
    return us * 1.001749054f;
#else
    return us;
#endif
}

uint64_t nrf_802154_platform_sl_lptimer_lpticks_to_us_convert(uint64_t lpticks)
{
#if IS_ENABLED(CONFIG_EMULATOR_FPGA)
    return lpticks * 0.998254f;
#else
    return lpticks;
#endif
}

void nrf_802154_platform_sl_lptimer_schedule_at(uint64_t fire_lpticks)
{
    uint64_t rescaled_lpticks = (fire_lpticks >> HALTIUM_FPGA_CLOCKS_DOWNSCALING_BITSHIFT);
    /* This function is not required to be reentrant, hence no critical section. */
    nrfy_grtc_int_enable(NRF_GRTC, GRTC_USED_INT_MASK);
    nrfy_grtc_sys_counter_cc_set(NRF_GRTC, GRTC_CALLBACKS_CC_CHANNEL, rescaled_lpticks);
    nrfy_grtc_sys_counter_compare_event_enable(NRF_GRTC, GRTC_CALLBACKS_CC_CHANNEL);
}

void nrf_802154_platform_sl_lptimer_disable(void)
{
    bool lock_key = grtc_compare_int_lock();

    nrfy_grtc_sys_counter_compare_event_clear(NRF_GRTC, GRTC_CALLBACKS_CC_CHANNEL);
    nrfy_grtc_int_disable(NRF_GRTC, GRTC_USED_INT_MASK);

    grtc_compare_int_unlock(lock_key);
}

void nrf_802154_timer_coord_init(void)
{
    setup_static_connections_for_radio_timestamps();
}

void nrf_802154_timer_coord_uninit(void)
{
    // Not implemented
}

void nrf_802154_timer_coord_start(void)
{
    // Intentionally empty
}

void nrf_802154_timer_coord_stop(void)
{
    // Intentionally empty
}

void nrf_802154_timer_coord_timestamp_prepare(const nrf_802154_sl_event_handle_t * p_event)
{
    uint32_t ppi_channel = p_event->event_addr;

    if (!p_event->shared)
    {
        publish_event_to_channel(p_event->event_addr, PPI_TIMESTAMP);
        ppi_channel = PPI_TIMESTAMP;
    }

    setup_dynamic_connections_for_radio_timestamps(ppi_channel);
    nrfy_grtc_sys_counter_cc_set(NRF_GRTC, GRTC_TIMESTAMPS_CC_CHANNEL, 0ULL);
}

bool nrf_802154_timer_coord_timestamp_get(uint64_t * p_timestamp)
{
    uint64_t cc_value;

    cc_value = nrfy_grtc_sys_counter_cc_get(NRF_GRTC, GRTC_TIMESTAMPS_CC_CHANNEL);

    if (cc_value == 0ULL)
    {
        return false;
    }
    else
    {
        *p_timestamp = nrf_802154_platform_sl_lptimer_lpticks_to_us_convert(cc_value << HALTIUM_FPGA_CLOCKS_DOWNSCALING_BITSHIFT);
        return true;
    }
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

typedef uint8_t hw_task_state_t;
#define HW_TASK_STATE_IDLE       0u
#define HW_TASK_STATE_SETTING_UP 1u
#define HW_TASK_STATE_READY      2u
#define HW_TASK_STATE_CLEANING   3u
#define HW_TASK_STATE_UPDATING   4u

static volatile hw_task_state_t m_hw_task_state = HW_TASK_STATE_IDLE;

static bool hw_task_state_set(hw_task_state_t expected_state,
                              hw_task_state_t new_state)
{
    return nrf_802154_sl_atomic_cas_u8((uint8_t *)&m_hw_task_state,
                                       &expected_state,
                                       new_state);
}

static void setup_dynamic_connections_for_radio_hw_tasks(uint32_t dppi_ch)
{
    // {a} Connection: DPPIC_020[dppi_ch] to RADIO.TASKS_{?}
    // It is the responsibility of the user of this platform to make the {a} connection
    // and pass the DPPI channel number as a parameter here.

    // {b} Connect: IPCT_radio to DPPIC_020[dppi_ch]
    nrf_ipct_publish_set(NRF_IPCT, IPCT_L_EVENT_RECEIVE, dppi_ch);
}

static void setup_static_connections_for_radio_hw_tasks(void)
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
    nrfy_grtc_publish_set(NRF_GRTC, GRTC_EVENT_HW_TRIGGER, DPPIC_G2_HT_CHANNEL);
}

static void clear_dynamic_connections_for_radio_hw_tasks(void)
{
    nrf_ipct_publish_clear(NRF_IPCT, IPCT_L_EVENT_RECEIVE);
    nrfy_dppi_channels_disable(DPPIC_G2_INST, 1UL << DPPIC_G2_HT_CHANNEL);
}

nrf_802154_sl_lptimer_platform_result_t nrf_802154_platform_sl_lptimer_hw_task_prepare(
    uint64_t fire_lpticks,
    uint32_t ppi_channel)
{
    const uint64_t                     grtc_cc_minimum_margin = 1uLL;
    uint64_t                           rescaled_lpticks =
        fire_lpticks >> HALTIUM_FPGA_CLOCKS_DOWNSCALING_BITSHIFT;
    uint64_t                           syscnt_now;
    bool                               done_on_time;
    nrf_802154_sl_mcu_critical_state_t mcu_cs_state;

    if (!hw_task_state_set(HW_TASK_STATE_IDLE, HW_TASK_STATE_SETTING_UP))
    {
        // the only one available set of peripherals is already used
        return NRF_802154_SL_LPTIMER_PLATFORM_NO_RESOURCES;
    }

    nrfy_grtc_event_clear(NRF_GRTC, GRTC_EVENT_HW_TRIGGER);
    nrf_ipct_event_clear(NRF_IPCT, IPCT_L_EVENT_RECEIVE);

    setup_dynamic_connections_for_radio_hw_tasks(ppi_channel);

    nrfy_grtc_sys_counter_cc_set(NRF_GRTC, GRTC_HW_TASKS_CC_CHANNEL, rescaled_lpticks);
    nrfy_grtc_sys_counter_compare_event_enable(NRF_GRTC, GRTC_HW_TASKS_CC_CHANNEL);

    nrf_802154_sl_mcu_critical_enter(mcu_cs_state);

    syscnt_now = nrfy_grtc_sys_counter_get(NRF_GRTC);
    if (syscnt_now + grtc_cc_minimum_margin >= rescaled_lpticks)
    {
        // it is too late
        clear_dynamic_connections_for_radio_hw_tasks();
        nrfy_grtc_event_clear(NRF_GRTC, GRTC_EVENT_HW_TRIGGER);
        nrf_ipct_event_clear(NRF_IPCT, IPCT_L_EVENT_RECEIVE);
        done_on_time = false;
    }
    else
    {
        nrfy_dppi_channels_enable(DPPIC_G2_INST, 1UL << DPPIC_G2_HT_CHANNEL);
        done_on_time = true;
    }

    nrf_802154_sl_mcu_critical_exit(mcu_cs_state);

    hw_task_state_set(HW_TASK_STATE_SETTING_UP,
                      done_on_time ? HW_TASK_STATE_READY : HW_TASK_STATE_IDLE);

    return done_on_time ? NRF_802154_SL_LPTIMER_PLATFORM_SUCCESS : NRF_802154_SL_LPTIMER_PLATFORM_TOO_LATE;
}

nrf_802154_sl_lptimer_platform_result_t nrf_802154_platform_sl_lptimer_hw_task_cleanup(void)
{
    if (!hw_task_state_set(HW_TASK_STATE_READY, HW_TASK_STATE_CLEANING))
    {
        return NRF_802154_SL_LPTIMER_PLATFORM_WRONG_STATE;
    }

    clear_dynamic_connections_for_radio_hw_tasks();
    nrfy_grtc_sys_counter_compare_event_disable(NRF_GRTC, GRTC_HW_TASKS_CC_CHANNEL);
    nrfy_grtc_event_clear(NRF_GRTC, GRTC_EVENT_HW_TRIGGER);
    nrf_ipct_event_clear(NRF_IPCT, IPCT_L_EVENT_RECEIVE);
    hw_task_state_set(HW_TASK_STATE_CLEANING, HW_TASK_STATE_IDLE);

    return NRF_802154_SL_LPTIMER_PLATFORM_SUCCESS;
}

nrf_802154_sl_lptimer_platform_result_t nrf_802154_platform_sl_lptimer_hw_task_update_ppi(
    uint32_t ppi_channel)
{
    bool cc_triggered;

    if (!hw_task_state_set(HW_TASK_STATE_READY, HW_TASK_STATE_UPDATING)) {
        return NRF_802154_SL_LPTIMER_PLATFORM_WRONG_STATE;
    }

    setup_dynamic_connections_for_radio_hw_tasks(ppi_channel);

    cc_triggered = nrf_ipct_event_check(NRF_IPCT, IPCT_L_EVENT_RECEIVE);

    hw_task_state_set(HW_TASK_STATE_UPDATING, HW_TASK_STATE_READY);

    return cc_triggered ? NRF_802154_SL_LPTIMER_PLATFORM_TOO_LATE :
           NRF_802154_SL_LPTIMER_PLATFORM_SUCCESS;
}

static bool grtc_compare_int_lock(void)
{
    uint32_t prev = nrfy_grtc_int_enable_check(NRF_GRTC, GRTC_USED_INT_MASK);

    nrfy_grtc_int_disable(NRF_GRTC, GRTC_USED_INT_MASK);

    return (prev != 0);
}

static void grtc_compare_int_unlock(bool key)
{
    if (key)
    {
        nrfy_grtc_int_enable(NRF_GRTC, GRTC_USED_INT_MASK);
    }
}

void nrf_802154_platform_sl_lptimer_critical_section_enter(void)
{
    nrf_802154_sl_mcu_critical_state_t state;

    nrf_802154_sl_mcu_critical_enter(state);

    m_critical_section_cnt++;

    if (m_critical_section_cnt == 1UL)
    {
        m_compare_int_lock_key = grtc_compare_int_lock();
    }

    nrf_802154_sl_mcu_critical_exit(state);
}

void nrf_802154_platform_sl_lptimer_critical_section_exit(void)
{
    nrf_802154_sl_mcu_critical_state_t state;

    nrf_802154_sl_mcu_critical_enter(state);

    assert(m_critical_section_cnt > 0UL);

    if (m_critical_section_cnt == 1UL)
    {
        grtc_compare_int_unlock(m_compare_int_lock_key);
    }

    m_critical_section_cnt--;

    nrf_802154_sl_mcu_critical_exit(state);
}
