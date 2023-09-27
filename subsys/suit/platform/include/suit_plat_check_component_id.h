/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CHECK_COMPONENT_ID
#define CHECK_COMPONENT_ID

#include <suit_mci.h>
#include <zcbor_common.h>

/**
 * @brief Check component ID and manifest class ID against supported manifests
 *
 * @param class_id Class ID to be checked
 * @param component_id Component ID to be checked
 * @return int ) in case of success, otherwise error code
 */
int suit_plat_check_component_id(suit_manifest_class_id_t *class_id,
				 struct zcbor_string *component_id);

#endif /* CHECK_COMPONENT_ID */
