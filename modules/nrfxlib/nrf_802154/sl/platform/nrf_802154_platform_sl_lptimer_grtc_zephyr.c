/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "platform/nrf_802154_platform_sl_lptimer.h"

#include <assert.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/timer/nrf_grtc_timer.h>

#include <haly/nrfy_dppi.h>

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

typedef uint8_t lptimer_state_t;
#define LPTIMER_STATE_DISABLED  0u
#define LPTIMER_STATE_ENABLED   1u

static volatile lptimer_state_t m_lptimer_state = LPTIMER_STATE_DISABLED;

static bool lptimer_state_set(lptimer_state_t new_state)
{
    lptimer_state_t expected_state = (new_state == LPTIMER_STATE_ENABLED) ?
        LPTIMER_STATE_DISABLED : LPTIMER_STATE_ENABLED;

    return nrf_802154_sl_atomic_cas_u8((uint8_t *)&m_lptimer_state,
                                       &expected_state,
                                       new_state);
}

static bool     m_compare_int_lock_key;
static uint32_t m_critical_section_cnt;
static int32_t  m_callbacks_cc_channel;
static int32_t  m_timestamps_cc_channel;
static int32_t  m_hw_tasks_cc_channel;

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

static void timer_compare_handler(int32_t id, uint64_t expire_time, void * user_data)
{
    (void)user_data;
    (void)expire_time;

    assert(id == m_callbacks_cc_channel);

    uint64_t curr_ticks = z_nrf_grtc_timer_read();

    nrf_802154_sl_timer_handler(curr_ticks);
}

void nrf_802154_platform_sl_lp_timer_init(void)
{
    m_critical_section_cnt = 0UL;

    m_callbacks_cc_channel = z_nrf_grtc_timer_chan_alloc();
    assert(m_callbacks_cc_channel >= 0);

    m_timestamps_cc_channel = z_nrf_grtc_timer_chan_alloc();
    assert(m_timestamps_cc_channel >= 0);

    m_hw_tasks_cc_channel = z_nrf_grtc_timer_chan_alloc();
    assert(m_hw_tasks_cc_channel >= 0);

    nrf_802154_platform_sl_lptimer_static_event_for_hw_tasks_set(m_hw_tasks_cc_channel);
    nrf_802154_platform_sl_lptimer_dynamic_event_for_hw_tasks_clear();

    /* Due to `m_lptimer_state == LPTIMER_STATE_DISABLED`, for consistency with 
     * `nrf_802154_platform_sl_lptimer_disable`, the critical section is now entered.
     */
    nrf_802154_platform_sl_lptimer_critical_section_enter();
}

void nrf_802154_platform_sl_lp_timer_deinit(void)
{
    (void)z_nrf_grtc_timer_compare_int_lock(m_callbacks_cc_channel);

    z_nrf_grtc_timer_chan_free(m_callbacks_cc_channel);
    z_nrf_grtc_timer_chan_free(m_timestamps_cc_channel);
    z_nrf_grtc_timer_chan_free(m_hw_tasks_cc_channel);
}

uint64_t nrf_802154_platform_sl_lptimer_current_lpticks_get(void)
{
    return z_nrf_grtc_timer_read();
}

uint64_t nrf_802154_platform_sl_lptimer_us_to_lpticks_convert(uint64_t us, bool round_up)
{
    return us >> HALTIUM_FPGA_CLOCKS_DOWNSCALING_BITSHIFT;
}

uint64_t nrf_802154_platform_sl_lptimer_lpticks_to_us_convert(uint64_t lpticks)
{
    return lpticks << HALTIUM_FPGA_CLOCKS_DOWNSCALING_BITSHIFT;
}

void nrf_802154_platform_sl_lptimer_schedule_at(uint64_t fire_lpticks)
{
    /* This function is not required to be reentrant, hence no critical section. */
    z_nrf_grtc_timer_set(m_callbacks_cc_channel,
                         fire_lpticks,
                         timer_compare_handler,
                         NULL);

    if (lptimer_state_set(LPTIMER_STATE_ENABLED))
    {
        nrf_802154_platform_sl_lptimer_critical_section_exit();
    }
}

void nrf_802154_platform_sl_lptimer_disable(void)
{
    if (lptimer_state_set(LPTIMER_STATE_DISABLED))
    {
        nrf_802154_platform_sl_lptimer_critical_section_enter();
    }

    z_nrf_grtc_timer_abort(m_callbacks_cc_channel);
}

void nrf_802154_timer_coord_init(void)
{
    nrf_802154_platform_sl_lptimer_static_event_for_timestamps_set(m_timestamps_cc_channel);

}

void nrf_802154_timer_coord_uninit(void)
{
    // Not implemented
}

void nrf_802154_timer_coord_start(void)
{
    nrf_dppi_channels_enable(NRF_802154_SL_DPPIC_INSTANCE, (1UL << PPI_TIMESTAMP));
}

void nrf_802154_timer_coord_stop(void)
{
    nrf_dppi_channels_disable(NRF_802154_SL_DPPIC_INSTANCE, (1UL << PPI_TIMESTAMP));
}

void nrf_802154_timer_coord_timestamp_prepare(const nrf_802154_sl_event_handle_t * p_event)
{
    uint32_t ppi_channel = p_event->event_addr;

    if (!p_event->shared)
    {
        publish_event_to_channel(p_event->event_addr, PPI_TIMESTAMP);
        ppi_channel = PPI_TIMESTAMP;
    }

    z_nrf_grtc_timer_capture_prepare(m_timestamps_cc_channel);

    nrf_802154_platform_sl_lptimer_dynamic_event_for_timestamps_set(ppi_channel,
                                                                    m_timestamps_cc_channel);
}

bool nrf_802154_timer_coord_timestamp_get(uint64_t * p_timestamp)
{
    uint64_t cc_value;

    if (z_nrf_grtc_timer_capture_read(m_timestamps_cc_channel, &cc_value) != 0)
    {
        return false;
    }

    *p_timestamp = nrf_802154_platform_sl_lptimer_lpticks_to_us_convert(cc_value);

    return true;
}

typedef uint8_t hw_task_state_t;
#define HW_TASK_STATE_IDLE       0u
#define HW_TASK_STATE_SETTING_UP 1u
#define HW_TASK_STATE_READY      2u
#define HW_TASK_STATE_CLEANING   3u
#define HW_TASK_STATE_UPDATING   4u

static volatile hw_task_state_t m_hw_task_state = HW_TASK_STATE_IDLE;
static uint64_t                 m_hw_task_fire_lpticks;

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
    uint64_t                           syscnt_now;
    bool                               done_on_time = true;
    nrf_802154_sl_mcu_critical_state_t mcu_cs_state;

    if (!hw_task_state_set(HW_TASK_STATE_IDLE, HW_TASK_STATE_SETTING_UP))
    {
        // the only one available set of peripherals is already used
        return NRF_802154_SL_LPTIMER_PLATFORM_NO_RESOURCES;
    }

    nrf_802154_platform_sl_lptimer_dynamic_event_for_hw_tasks_set(ppi_channel,
                                                                  m_hw_tasks_cc_channel);

    m_hw_task_fire_lpticks = fire_lpticks;

    z_nrf_grtc_timer_set(m_hw_tasks_cc_channel, fire_lpticks, NULL, NULL);

    nrf_802154_sl_mcu_critical_enter(mcu_cs_state);

    syscnt_now = z_nrf_grtc_timer_read();

    if (syscnt_now + grtc_cc_minimum_margin >= fire_lpticks)
    {
        // it is too late
        nrf_802154_platform_sl_lptimer_dynamic_event_for_hw_tasks_clear();
        z_nrf_grtc_timer_abort(m_hw_tasks_cc_channel);
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

    z_nrf_grtc_timer_abort(m_hw_tasks_cc_channel);

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

    nrf_802154_platform_sl_lptimer_dynamic_event_for_hw_tasks_set(ppi_channel,
                                                                  m_hw_tasks_cc_channel);

    cc_triggered = z_nrf_grtc_timer_compare_evt_check(m_hw_tasks_cc_channel);
    if (z_nrf_grtc_timer_read() >= m_hw_task_fire_lpticks) {
        cc_triggered = true;
    }

    hw_task_state_set(HW_TASK_STATE_UPDATING, HW_TASK_STATE_READY);

    return cc_triggered ? NRF_802154_SL_LPTIMER_PLATFORM_TOO_LATE :
           NRF_802154_SL_LPTIMER_PLATFORM_SUCCESS;
}

void nrf_802154_platform_sl_lptimer_critical_section_enter(void)
{
    nrf_802154_sl_mcu_critical_state_t state;

    nrf_802154_sl_mcu_critical_enter(state);

    m_critical_section_cnt++;

    if (m_critical_section_cnt == 1UL)
    {
        m_compare_int_lock_key = z_nrf_grtc_timer_compare_int_lock(m_callbacks_cc_channel);
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
        z_nrf_grtc_timer_compare_int_unlock(m_callbacks_cc_channel,
                                            m_compare_int_lock_key);
    }

    m_critical_section_cnt--;

    nrf_802154_sl_mcu_critical_exit(state);
}
