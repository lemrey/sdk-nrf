/*$$$LICENCE_NORDIC_STANDARD<2017>$$$*/

/**@file
 *
 * @defgroup nrf_sdh_soc SoC support in SoftDevice Handler
 * @{
 * @ingroup  nrf_sdh
 * @brief    This file contains the declarations of types and functions required for SoftDevice Handler
 * SoC support.
 */

#ifndef NRF_SDH_SOC_H__
#define NRF_SDH_SOC_H__

#include "sdk_common.h"
#include "nrf_section_iter.h"
#include "nrf_soc.h"

#ifdef __cplusplus
extern "C" {
#endif


#if !(defined(__LINT__))
/**@brief   Macro for registering @ref nrf_sdh_soc_evt_observer_t. Modules that want to be
 *          notified about SoC events must register the handler using this macro.
 *
 * @details This macro places the observer in a section named "sdh_soc_observers".
 *
 * @param[in]   _name       Observer name.
 * @param[in]   _prio       Priority of the observer event handler.
 *                          The smaller the number, the higher the priority.
 * @param[in]   _handler    SoC event handler.
 * @param[in]   _context    Parameter to the event handler.
 * @hideinitializer
 */
#define NRF_SDH_SOC_OBSERVER(_name, _prio, _handler, _context)                                      \
NRF_SECTION_SET_ITEM_REGISTER(sdh_soc_observers, _prio, nrf_sdh_soc_evt_observer_t, _name) =  \
{                                                                                                   \
    .handler   = _handler,                                                                          \
    .p_context = _context                                                                           \
}

/**@brief   Macro for registering an array of @ref nrf_sdh_soc_evt_observer_t.
 *          Modules that want to be notified about SoC events must register the handler using
 *          this macro.
 *
 * Each observer's handler will be dispatched an event with its relative context from @p _context.
 * This macro places the observer in a section named "sdh_soc_observers".
 *
 * @param[in]   _name       Observer name.
 * @param[in]   _prio       Priority of the observer event handler.
 *                          The smaller the number, the higher the priority.
 * @param[in]   _handler    SoC event handler.
 * @param[in]   _context    An array of parameters to the event handler.
 * @param[in]   _cnt        Number of observers to register.
 * @hideinitializer
 */
#define NRF_SDH_SOC_EVENT_OBSERVERS(_name, _prio, _handler, _context, _cnt)                              \
STATIC_ASSERT(NRF_SDH_SOC_ENABLED, "NRF_SDH_SOC_ENABLED not set!");                                      \
STATIC_ASSERT(_prio < NRF_SDH_SOC_OBSERVER_PRIO_LEVELS, "Priority level unavailable.");                  \
NRF_SECTION_SET_ITEM_REGISTER(sdh_soc_observers, _prio, static nrf_sdh_soc_evt_observer_t _name[_cnt]) = \
{                                                                                                        \
    MACRO_REPEAT_FOR(_cnt, HANDLER_SET, _handler, _context)                                              \
}

#if !(defined(DOXYGEN))
#define HANDLER_SET(_idx, _handler, _context)                                                       \
{                                                                                                   \
    .handler   = _handler,                                                                          \
    .p_context = _context[_idx],                                                                    \
},
#endif

#else // __LINT__

/* Swallow semicolons */
/*lint -save -esym(528, *) -esym(529, *) : Symbol not referenced. */
#define NRF_SDH_SOC_OBSERVER(A, B, C, D)     static int semicolon_swallow_##A
#define NRF_SDH_SOC_OBSERVERS(A, B, C, D, E) static int semicolon_swallow_##A
/*lint -restore */

#endif


/**@brief   SoC event handler. */
typedef void (*nrf_sdh_soc_evt_handler_t) (uint32_t evt_id, void * p_context);

/**@brief   SoC event observer. */
typedef struct
{
    nrf_sdh_soc_evt_handler_t   handler;    //!< SoC event handler.
    void                      * p_context;  //!< A parameter to the event handler.
} const nrf_sdh_soc_evt_observer_t;


#ifdef __cplusplus
}
#endif

#endif // NRF_SDH_SOC_H__

/** @} */
