/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "platform/nrf_802154_platform_sl_lptimer.h"

#include <assert.h>
#include <zephyr/kernel.h>

#include <haly/nrfy_dppi.h>
#include <haly/nrfy_grtc.h>

#include "nrf_802154_sl_config.h"
#include "nrf_802154_sl_atomics.h"
#include "nrf_802154_sl_utils.h"
#include "timer/nrf_802154_timer_coord.h"
#include "nrf_802154_sl_periphs.h"

#include "nrf_802154_platform_sl_lptimer_grtc_events.h"

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

void nrf_802154_platform_sl_lp_timer_init(void)
{
    m_critical_section_cnt = 0UL;

#ifdef MOONLIGHT_XXAA
    nrfy_grtc_rt_counter_start(NRF_GRTC, true);
    nrfy_grtc_sys_counter_start(NRF_GRTC, false);
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
    IRQ_DIRECT_CONNECT(GRTC_IRQn, GRTC_INT_PRIORITY, grtc_irq_handler, IRQ_ZERO_LATENCY);
    irq_enable(GRTC_IRQn);
#endif

    nrfy_grtc_event_clear(NRF_GRTC, GRTC_SYS_COUNTER_CMP_EVENT);

    nrf_802154_platform_sl_lptimer_static_event_for_hw_tasks_set(GRTC_EVENT_HW_TRIGGER);
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
#if NRFY_GRTC_HAS_SYSCOUNTER_ARRAY
    nrfy_grtc_sys_counter_active_set(NRF_GRTC, true);

    while (!nrfy_grtc_sys_conter_ready_check(NRF_GRTC))
    {
        // Intentionally empty.
    }
#endif

    uint64_t counter = nrfy_grtc_sys_counter_get(NRF_GRTC);

#if NRFY_GRTC_HAS_SYSCOUNTER_ARRAY
    nrfy_grtc_sys_counter_active_set(NRF_GRTC, false);
#endif

    return counter << HALTIUM_FPGA_CLOCKS_DOWNSCALING_BITSHIFT;
}

uint64_t nrf_802154_platform_sl_lptimer_us_to_lpticks_convert(uint64_t us, bool round_up)
{
    return us;
}

uint64_t nrf_802154_platform_sl_lptimer_lpticks_to_us_convert(uint64_t lpticks)
{
    return lpticks;
}

void nrf_802154_platform_sl_lptimer_schedule_at(uint64_t fire_lpticks)
{
    uint64_t rescaled_lpticks = (fire_lpticks >> HALTIUM_FPGA_CLOCKS_DOWNSCALING_BITSHIFT);

    /* This function is not required to be reentrant, hence no critical section. */
    nrfy_grtc_int_enable(NRF_GRTC, GRTC_USED_INT_MASK);
    nrfy_grtc_sys_counter_cc_set(NRF_GRTC, GRTC_CALLBACKS_CC_CHANNEL, rescaled_lpticks);
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
    nrf_802154_platform_sl_lptimer_static_event_for_timestamps_set(GRTC_TASK_CAPTURE_TIMESTAMP);
    nrf_dppi_channels_enable(NRF_802154_SL_DPPIC_INSTANCE, (1UL << PPI_TIMESTAMP));
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

    nrf_802154_platform_sl_lptimer_dynamic_event_for_timestamps_set(ppi_channel, GRTC_TASK_CAPTURE_TIMESTAMP);
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

nrf_802154_sl_lptimer_platform_result_t nrf_802154_platform_sl_lptimer_hw_task_prepare(
    uint64_t fire_lpticks,
    uint32_t ppi_channel)
{
    const uint64_t                     grtc_cc_minimum_margin = 1uLL;
    uint64_t                           rescaled_lpticks =
        fire_lpticks >> HALTIUM_FPGA_CLOCKS_DOWNSCALING_BITSHIFT;
    uint64_t                           syscnt_now;
    bool                               done_on_time = true;
    nrf_802154_sl_mcu_critical_state_t mcu_cs_state;

    if (!hw_task_state_set(HW_TASK_STATE_IDLE, HW_TASK_STATE_SETTING_UP))
    {
        // the only one available set of peripherals is already used
        return NRF_802154_SL_LPTIMER_PLATFORM_NO_RESOURCES;
    }

    nrfy_grtc_event_clear(NRF_GRTC, GRTC_EVENT_HW_TRIGGER);

    nrf_802154_platform_sl_lptimer_dynamic_event_for_hw_tasks_set(ppi_channel, GRTC_EVENT_HW_TRIGGER);

    nrfy_grtc_sys_counter_cc_set(NRF_GRTC, GRTC_HW_TASKS_CC_CHANNEL, rescaled_lpticks);
    nrfy_grtc_sys_counter_compare_event_enable(NRF_GRTC, GRTC_HW_TASKS_CC_CHANNEL);

    nrf_802154_sl_mcu_critical_enter(mcu_cs_state);

    syscnt_now = nrfy_grtc_sys_counter_get(NRF_GRTC);
    if (syscnt_now + grtc_cc_minimum_margin >= rescaled_lpticks)
    {
        // it is too late
        nrf_802154_platform_sl_lptimer_dynamic_event_for_hw_tasks_clear();
        nrfy_grtc_event_clear(NRF_GRTC, GRTC_EVENT_HW_TRIGGER);
        done_on_time = false;
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

    nrf_802154_platform_sl_lptimer_dynamic_event_for_hw_tasks_clear();
    nrfy_grtc_sys_counter_compare_event_disable(NRF_GRTC, GRTC_HW_TASKS_CC_CHANNEL);
    nrfy_grtc_event_clear(NRF_GRTC, GRTC_EVENT_HW_TRIGGER);
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

    nrf_802154_platform_sl_lptimer_dynamic_event_for_hw_tasks_set(ppi_channel, GRTC_EVENT_HW_TRIGGER);

    cc_triggered = nrfy_grtc_event_check(
        NRF_GRTC,
        nrf_grtc_sys_counter_compare_event_get(GRTC_HW_TASKS_CC_CHANNEL));

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
