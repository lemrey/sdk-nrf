/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-4-Clause
 */
#include <suit_types.h>

/**
 * @brief Validate component vid against expected value
 * 
 * @param handle Component handle
 * @param vid_uuid Component vid
 * @return int Error code
 */
int plat_com_check_vid(suit_component_t handle, struct zcbor_string *vid_uuid);

/**
 * @brief Validate component cid against expected value
 * 
 * @param handle Component handle
 * @param cid_uuid Component cid
 * @return int Error code
 */
int plat_com_check_cid(suit_component_t handle, struct zcbor_string *cid_uuid);

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
bool plat_com_decode_component_id(struct zcbor_string *component_id, uint8_t *cpu_id,
				  intptr_t *run_address, size_t *size);
