/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <suit_mci.h>

mci_err_t suit_mci_manifest_parent_child_validate(const suit_manifest_class_id_t *parent_class_id,
						  const suit_manifest_class_id_t *child_class_id)
{
	if (NULL == parent_class_id || NULL == child_class_id) {
		return SUIT_PLAT_ERR_INVAL;
	}

	const suit_manifest_class_id_t *existing_parent_class_id = NULL;
	mci_err_t ret = suit_mci_manifest_parent_get(child_class_id, &existing_parent_class_id);

	if (ret != SUIT_PLAT_SUCCESS) {
		return ret;
	}

	if (NULL == existing_parent_class_id) {
		return MCI_ERR_NOACCESS;
	}

	if (SUIT_PLAT_SUCCESS ==
	    suit_metadata_uuid_compare(parent_class_id, existing_parent_class_id)) {
		return SUIT_PLAT_SUCCESS;
	}

	return MCI_ERR_NOACCESS;
}
