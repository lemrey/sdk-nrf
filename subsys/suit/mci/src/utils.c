/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <suit_mci.h>
#include <string.h>

mci_err_t suit_mci_suit_uuid_compare(const suit_uuid_t *uuid1, const suit_uuid_t *uuid2)
{
	if (NULL == uuid1 || NULL == uuid2) {
		return SUIT_PLAT_ERR_INVAL;
	}

	if (0 == memcmp(uuid1->raw, uuid2->raw, sizeof(((suit_uuid_t *)0)->raw))) {
		return SUIT_PLAT_SUCCESS;
	}

	return MCI_ERR_COMPARISON_FAILED;
}

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

	if (SUIT_PLAT_SUCCESS
	    == suit_mci_suit_uuid_compare(parent_class_id, existing_parent_class_id)) {
		return SUIT_PLAT_SUCCESS;
	}

	return MCI_ERR_NOACCESS;
}
