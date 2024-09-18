/*$$$LICENCE_NORDIC_STANDARD<2017>$$$*/

#include "sdk_common.h"
#include "nrf_sdh_ble.h"

#include "nrf_sdh.h"
#include "app_error.h"
#include "nrf_strerror.h"


#include "nrf_log.h"
#if 0
#define NRF_LOG_MODULE_NAME nrf_sdh_ble
#if NRF_SDH_BLE_LOG_ENABLED
    #define NRF_LOG_LEVEL       NRF_SDH_BLE_LOG_LEVEL
    #define NRF_LOG_INFO_COLOR  NRF_SDH_BLE_INFO_COLOR
    #define NRF_LOG_DEBUG_COLOR NRF_SDH_BLE_DEBUG_COLOR
#else
    #define NRF_LOG_LEVEL       0
#endif // NRF_SDH_BLE_LOG_ENABLED
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();
#else
LOG_MODULE_REGISTER(nrf_sdh_ble);
#endif

// Create section set "sdh_ble_observers".
NRF_SECTION_SET_DEF(sdh_ble_observers, nrf_sdh_ble_evt_observer_t,
                CONFIG_NRF_SDH_BLE_OBSERVER_PRIO_LEVELS);


//lint -save -e10 -e19 -e40 -e27 Illegal character (0x24)
#if defined(__ZEPHYR__)
    uint32_t const * const m_ram_start = (uint32_t *const)DT_REG_ADDR(DT_CHOSEN(zephyr_sram));
#elif defined(__CC_ARM)
    extern uint32_t  Image$$RW_IRAM1$$Base;
    uint32_t const * const m_ram_start = &Image$$RW_IRAM1$$Base;
#elif defined(__ICCARM__)
    extern uint32_t  __ICFEDIT_region_RAM_start__;
    uint32_t const * const m_ram_start = &__ICFEDIT_region_RAM_start__;
#elif defined(__SES_ARM)
    extern uint32_t  __app_ram_start__;
    uint32_t const * const m_ram_start = &__app_ram_start__;
#elif defined(__GNUC__)
    extern uint32_t  __data_start__;
    uint32_t const * const m_ram_start = &__data_start__;
#endif
//lint -restore

#define RAM_START       0x20000000
#define APP_RAM_START   (uint32_t)m_ram_start


static bool m_stack_is_enabled;


ret_code_t nrf_sdh_ble_app_ram_start_get(uint32_t * p_app_ram_start)
{
    if (p_app_ram_start == NULL)
    {
        return NRF_ERROR_NULL;
    }

    *p_app_ram_start = APP_RAM_START;

    return NRF_SUCCESS;
}


ret_code_t nrf_sdh_ble_default_cfg_set(uint8_t conn_cfg_tag, uint32_t * p_ram_start)
{
    uint32_t ret_code;

    ret_code = nrf_sdh_ble_app_ram_start_get(p_ram_start);
    if (ret_code != NRF_SUCCESS)
    {
        return ret_code;
    }

#if defined (S112) || defined(S312)
    STATIC_ASSERT(CONFIG_NRF_SDH_BLE_CENTRAL_LINK_COUNT == 0, "When using s112, NRF_SDH_BLE_CENTRAL_LINK_COUNT must be 0.");
#endif

    // Overwrite some of the default settings of the BLE stack.
    // If any of the calls to sd_ble_cfg_set() fail, log the error but carry on so that
    // wrong RAM settings can be caught by nrf_sdh_ble_enable() and a meaningful error
    // message will be printed to the user suggesting the correct value.
    ble_cfg_t ble_cfg;

#if (CONFIG_NRF_SDH_BLE_TOTAL_LINK_COUNT != 0)
    // Configure the connection count.
    memset(&ble_cfg, 0, sizeof(ble_cfg));
    ble_cfg.conn_cfg.conn_cfg_tag                     = conn_cfg_tag;
    ble_cfg.conn_cfg.params.gap_conn_cfg.conn_count   = CONFIG_NRF_SDH_BLE_TOTAL_LINK_COUNT;
    ble_cfg.conn_cfg.params.gap_conn_cfg.event_length = CONFIG_NRF_SDH_BLE_GAP_EVENT_LENGTH;

    ret_code = sd_ble_cfg_set(BLE_CONN_CFG_GAP, &ble_cfg, *p_ram_start);
    if (ret_code != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("sd_ble_cfg_set() returned %s when attempting to set BLE_CONN_CFG_GAP.",
                      nrf_strerror_get(ret_code));
    }

    // Configure the connection roles.
    memset(&ble_cfg, 0, sizeof(ble_cfg));
#if !defined (S122)
    ble_cfg.gap_cfg.role_count_cfg.periph_role_count  = CONFIG_NRF_SDH_BLE_PERIPHERAL_LINK_COUNT;
#endif // !defined (S122)
#if !defined (S112) && !defined(S312) && !defined(S113)
    ble_cfg.gap_cfg.role_count_cfg.central_role_count = CONFIG_NRF_SDH_BLE_CENTRAL_LINK_COUNT;
    ble_cfg.gap_cfg.role_count_cfg.central_sec_count  = MIN(CONFIG_NRF_SDH_BLE_CENTRAL_LINK_COUNT,
                                                            BLE_GAP_ROLE_COUNT_CENTRAL_SEC_DEFAULT);
#endif // !defined (S112) && !defined(S312) && !defined(S113)

    ret_code = sd_ble_cfg_set(BLE_GAP_CFG_ROLE_COUNT, &ble_cfg, *p_ram_start);
    if (ret_code != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("sd_ble_cfg_set() returned %s when attempting to set BLE_GAP_CFG_ROLE_COUNT.",
                      nrf_strerror_get(ret_code));
    }

    // Configure the maximum ATT MTU.
#if (CONFIG_NRF_SDH_BLE_GATT_MAX_MTU_SIZE != 23)
    memset(&ble_cfg, 0x00, sizeof(ble_cfg));
    ble_cfg.conn_cfg.conn_cfg_tag                 = conn_cfg_tag;
    ble_cfg.conn_cfg.params.gatt_conn_cfg.att_mtu = CONFIG_NRF_SDH_BLE_GATT_MAX_MTU_SIZE;

    ret_code = sd_ble_cfg_set(BLE_CONN_CFG_GATT, &ble_cfg, *p_ram_start);
    if (ret_code != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("sd_ble_cfg_set() returned %s when attempting to set BLE_CONN_CFG_GATT.",
                      nrf_strerror_get(ret_code));
    }
#endif  // NRF_SDH_BLE_GATT_MAX_MTU_SIZE != 23
#endif  // NRF_SDH_BLE_TOTAL_LINK_COUNT != 0

    // Configure number of custom UUIDS.
    memset(&ble_cfg, 0, sizeof(ble_cfg));
    ble_cfg.common_cfg.vs_uuid_cfg.vs_uuid_count = CONFIG_NRF_SDH_BLE_VS_UUID_COUNT;

    ret_code = sd_ble_cfg_set(BLE_COMMON_CFG_VS_UUID, &ble_cfg, *p_ram_start);
    if (ret_code != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("sd_ble_cfg_set() returned %s when attempting to set BLE_COMMON_CFG_VS_UUID.",
                      nrf_strerror_get(ret_code));
    }

    // Configure the GATTS attribute table.
    memset(&ble_cfg, 0x00, sizeof(ble_cfg));
    ble_cfg.gatts_cfg.attr_tab_size.attr_tab_size = CONFIG_NRF_SDH_BLE_GATTS_ATTR_TAB_SIZE;

    ret_code = sd_ble_cfg_set(BLE_GATTS_CFG_ATTR_TAB_SIZE, &ble_cfg, *p_ram_start);
    if (ret_code != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("sd_ble_cfg_set() returned %s when attempting to set BLE_GATTS_CFG_ATTR_TAB_SIZE.",
                      nrf_strerror_get(ret_code));
    }

    // Configure Service Changed characteristic.
    memset(&ble_cfg, 0x00, sizeof(ble_cfg));
    ble_cfg.gatts_cfg.service_changed.service_changed =
            IS_ENABLED(CONFIG_NRF_SDH_BLE_SERVICE_CHANGED);

    ret_code = sd_ble_cfg_set(BLE_GATTS_CFG_SERVICE_CHANGED, &ble_cfg, *p_ram_start);
    if (ret_code != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("sd_ble_cfg_set() returned %s when attempting to set BLE_GATTS_CFG_SERVICE_CHANGED.",
                      nrf_strerror_get(ret_code));
    }

    return NRF_SUCCESS;
}


/**@brief   Function for finding the end address of the RAM. */
static uint32_t ram_end_address_get(void)
{
    uint32_t ram_total_size;

#ifdef NRF51
    uint32_t block_size = NRF_FICR->SIZERAMBLOCKS;
    ram_total_size      = block_size * NRF_FICR->NUMRAMBLOCK;
#else
    ram_total_size      = NRF_FICR->INFO.RAM * 1024;
#endif

    return RAM_START + ram_total_size;
}


ret_code_t nrf_sdh_ble_enable(uint32_t * const p_app_ram_start)
{
    // Start of RAM, obtained from linker symbol.
    uint32_t const app_ram_start_link = *p_app_ram_start;

    ret_code_t ret_code = sd_ble_enable(p_app_ram_start);
    if (*p_app_ram_start > app_ram_start_link)
    {
        NRF_LOG_WARNING("Insufficient RAM allocated for the SoftDevice.");

        NRF_LOG_WARNING("Change the RAM start location from 0x%x to 0x%x.",
                        app_ram_start_link, *p_app_ram_start);
        NRF_LOG_WARNING("Maximum RAM size for application is 0x%x.",
                        ram_end_address_get() - (*p_app_ram_start));
    }
    else
    {
        NRF_LOG_DEBUG("RAM starts at 0x%x", app_ram_start_link);
        if (*p_app_ram_start != app_ram_start_link)
        {
            NRF_LOG_DEBUG("RAM start location can be adjusted to 0x%x.", *p_app_ram_start);

            NRF_LOG_DEBUG("RAM size for application can be adjusted to 0x%x.",
                          ram_end_address_get() - (*p_app_ram_start));
        }
    }

    if (ret_code == NRF_SUCCESS)
    {
        m_stack_is_enabled = true;
    }
    else
    {
        NRF_LOG_ERROR("sd_ble_enable() returned %s.", nrf_strerror_get(ret_code));
    }

    return ret_code;
}


/**@brief       Function for polling BLE events.
 *
 * @param[in]   p_context   Context of the observer.
 */
static void nrf_sdh_ble_evts_poll(void * p_context)
{
    UNUSED_VARIABLE(p_context);

    ret_code_t ret_code;

    if (!m_stack_is_enabled)
    {
        return;
    }

    while (true)
    {
        /*lint -save -e(587) */
        __ALIGN(4) uint8_t evt_buffer[NRF_SDH_BLE_EVT_BUF_SIZE];
        /*lint -restore */

        ble_evt_t * p_ble_evt;
        uint16_t    evt_len = (uint16_t)sizeof(evt_buffer);

        ret_code = sd_ble_evt_get(evt_buffer, &evt_len);
        if (ret_code != NRF_SUCCESS)
        {
            break;
        }

        p_ble_evt = (ble_evt_t *)evt_buffer;

        NRF_LOG_DEBUG("BLE event: 0x%x.", p_ble_evt->header.evt_id);

        // Forward the event to BLE observers.
        TYPE_SECTION_FOREACH(nrf_sdh_ble_evt_observer_t, sdh_ble_observers, p_observer)
        {
            p_observer->handler(p_ble_evt, p_observer->p_context);
        }
    }

    if (ret_code != NRF_ERROR_NOT_FOUND)
    {
        APP_ERROR_HANDLER(ret_code);
    }
}


NRF_SDH_STACK_OBSERVER(m_nrf_sdh_ble_evts_poll, CONFIG_NRF_SDH_BLE_STACK_OBSERVER_PRIO) =
{
    .handler   = nrf_sdh_ble_evts_poll,
    .p_context = NULL,
};
