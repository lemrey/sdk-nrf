/*$$$LICENCE_NORDIC_STANDARD<2016>$$$*/
#ifndef NRF_SECTION_H__
#define NRF_SECTION_H__

#include <zephyr/kernel.h>
#include "nordic_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup section_vars Section variables
 * @ingroup app_common
 * @{
 *
 * @brief Section variables.
 */

//lint -save -e27 -esym(526,*)

#if defined(__ICCARM__)
// Enable IAR language extensions
#pragma language=extended
#endif

/**@brief   Macro for obtaining the address of the beginning of a section.
 *
 * param[in]    section_name    Name of the section.
 * @hideinitializer
 */
#if defined(__ZEPHYR__)
#define NRF_SECTION_START_ADDR(section_name)       TYPE_SECTION_START(section_name)

#elif defined(__CC_ARM)
#define NRF_SECTION_START_ADDR(section_name)       &CONCAT_2(section_name, $$Base)

#elif defined(__GNUC__)
#define NRF_SECTION_START_ADDR(section_name)       &CONCAT_2(__start_, section_name)

#elif defined(__ICCARM__)
#define NRF_SECTION_START_ADDR(section_name)       __section_begin(STRINGIFY(section_name))
#endif


/**@brief    Macro for obtaining the address of the end of a section.
 *
 * @param[in]   section_name    Name of the section.
 * @hideinitializer
 */
#if defined(__ZEPHYR__)
#define NRF_SECTION_END_ADDR(section_name)         TYPE_SECTION_END(section_name)

#elif defined(__CC_ARM)
#define NRF_SECTION_END_ADDR(section_name)         &CONCAT_2(section_name, $$Limit)

#elif defined(__GNUC__)
#define NRF_SECTION_END_ADDR(section_name)         &CONCAT_2(__stop_, section_name)

#elif defined(__ICCARM__)
#define NRF_SECTION_END_ADDR(section_name)         __section_end(STRINGIFY(section_name))
#endif


/**@brief   Macro for retrieving the length of a given section, in bytes.
 *
 * @param[in]   section_name    Name of the section.
 * @hideinitializer
 */
#define NRF_SECTION_LENGTH(section_name)                        \
    ((size_t)NRF_SECTION_END_ADDR(section_name) -               \
     (size_t)NRF_SECTION_START_ADDR(section_name))


/**@brief   Macro for creating a section.
 *
 * @param[in]   section_name    Name of the section.
 * @param[in]   data_type       Data type of the variables to be registered in the section.
 *
 * @warning Data type must be word aligned to prevent padding.
 * @hideinitializer
 */
#if defined(__ZEPHYR__)
#define NRF_SECTION_DEF(section_name, data_type)                \
    TYPE_SECTION_START_EXTERN(data_type, section_name);         \
    TYPE_SECTION_END_EXTERN(data_type, section_name)

#elif defined(__CC_ARM)
#define NRF_SECTION_DEF(section_name, data_type)                \
    extern data_type * CONCAT_2(section_name, $$Base);          \
    extern void      * CONCAT_2(section_name, $$Limit)

#elif defined(__GNUC__)
#define NRF_SECTION_DEF(section_name, data_type)                \
    extern data_type * CONCAT_2(__start_, section_name);        \
    extern void      * CONCAT_2(__stop_,  section_name)

#elif defined(__ICCARM__)
#define NRF_SECTION_DEF(section_name, data_type)                \
    _Pragma(STRINGIFY(section = STRINGIFY(section_name)));

#endif


/**@brief   Macro for declaring a variable and registering it in a section.
 *
 * @details Declares a variable and registers it in a named section. This macro ensures that the
 *          variable is not stripped away when using optimizations.
 *
 * @note The order in which variables are placed in a section is dependent on the order in
 *       which the linker script encounters the variables during linking.
 *
 * @param[in]   section_name    Name of the section.
 * @param[in]   section_var     Variable to register in the given section.
 * @hideinitializer
 */
#if defined(__ZEPHYR__)
#define NRF_SECTION_ITEM_REGISTER(section_name, section_var) \
    TYPE_SECTION_ITERABLE(section_name, section_var, section_name, _)

#elif defined(__CC_ARM)
#define NRF_SECTION_ITEM_REGISTER(section_name, section_var) \
    section_var __attribute__ ((section(STRINGIFY(section_name)))) __attribute__((used))

#elif defined(__GNUC__)
#define NRF_SECTION_ITEM_REGISTER(section_name, section_var) \
    section_var __attribute__ ((section("." STRINGIFY(section_name)))) __attribute__((used))

#elif defined(__ICCARM__)
#define NRF_SECTION_ITEM_REGISTER(section_name, section_var) \
    __root section_var @ STRINGIFY(section_name)
#endif


/**@brief   Macro for retrieving a variable from a section.
 *
 * @warning     The stored symbol can only be resolved using this macro if the
 *              type of the data is word aligned. The operation of acquiring
 *              the stored symbol relies on the size of the stored type. No
 *              padding can exist in the named section in between individual
 *              stored items or this macro will fail.
 *
 * @param[in]   section_name    Name of the section.
 * @param[in]   data_type       Data type of the variable.
 * @param[in]   i               Index of the variable in section.
 * @hideinitializer
 */
#define NRF_SECTION_ITEM_GET(section_name, data_type, i) \
    ((data_type*)NRF_SECTION_START_ADDR(section_name) + (i))


/**@brief   Macro for getting the number of variables in a section.
 *
 * @param[in]   section_name    Name of the section.
 * @param[in]   data_type       Data type of the variables in the section.
 * @hideinitializer
 */
#define NRF_SECTION_ITEM_COUNT(section_name, data_type) \
    NRF_SECTION_LENGTH(section_name) / sizeof(data_type)

/** @} */

//lint -restore

#ifdef __cplusplus
}
#endif

#endif // NRF_SECTION_H__
