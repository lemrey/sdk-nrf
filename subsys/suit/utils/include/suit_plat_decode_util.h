/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUIT_PLAT_DECODE_UTIL_H__
#define SUIT_PLAT_DECODE_UTIL_H__

#include <zcbor_common.h>
#ifdef CONFIG_SUIT_MCI
#include <suit_mci.h>
#endif /* CONFIG_SUIT_MCI */
#ifdef CONFIG_SUIT_PLATFORM
#include <suit_platform_internal.h>
#endif /* CONFIG_SUIT_PLATFORM*/

#define SUIT_PLAT_MAX_NUM_COMPONENT_ID_PARTS                                                       \
	5 ///! The maximum number of bytestrings in a component ID.

/**
 * @brief Decode params from component_id
 *
 * @param component_id Input component id
 * @param cpu_id Decoded CPU id
 * @param run_address Decoded start/run address
 * @param size Decoded component size
 * @return true Operation succeeded
 * @return false Operation failed
 */
bool suit_plat_decode_component_id(struct zcbor_string *component_id, uint8_t *cpu_id,
				   intptr_t *run_address, size_t *size);

#ifdef CONFIG_SUIT_PLATFORM
/**
 * @brief Decode component type from component_id
 *
 * @param component_id Input component_id
 * @param type         Decoded component type
 * @return true Operation succeeded
 * @return false Operation failed
 */
bool suit_plat_decode_component_type(struct zcbor_string *component_id,
				     suit_component_type_t *type);
#endif /* CONFIG_SUIT_PLATFORM */

/**
 * @brief Decode params from component_id
 *
 * @param component_id Input component id
 * @param run_address Decoded start/run address
 * @param size Decoded component size
 * @return true Operation succeeded
 * @return false Operation failed
 */
bool suit_plat_decode_address_size(struct zcbor_string *component_id, intptr_t *run_address,
				   size_t *size);

/**
 * @brief Decode number from component_id
 *
 * @param component_id Input component id
 * @param number Decoded component number
 * @return true Operation succeeded
 * @return false Operation failed
 */
bool suit_plat_decode_component_number(struct zcbor_string *component_id, uint32_t *number);

#ifdef CONFIG_SUIT_MCI
/**
 * @brief Decode manifest class ID from component_id
 *
 * @param component_id Input component id
 * @param class_id Decoded class ID
 *
 * @return true Operation succeeded
 * @return false Operation failed
 */
bool suit_plat_decode_manifest_class_id(struct zcbor_string *component_id,
					suit_manifest_class_id_t **class_id);
#endif /* CONFIG_SUIT_MCI */

#endif /* SUIT_PLAT_DECODE_UTIL_H__ */
