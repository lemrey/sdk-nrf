/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <suit_mci.h>
#include <string.h>

int mci_compare_suit_uuid(const suit_uuid_t *uuid1, const suit_uuid_t *uuid2)
{
	if (NULL == uuid1 || NULL == uuid2) {
		return -MCI_EINVAL;
	}

	if (0 == memcmp(uuid1->raw, uuid2->raw, sizeof(((suit_uuid_t *)0)->raw))) {
		return 0;
	}

	return -MCI_EMANIFESTCLASSID;
}

int mci_validate_manifest_parent_child(const suit_manifest_class_id_t *parent_class_id,
				       const suit_manifest_class_id_t *child_class_id)
{
	if (NULL == parent_class_id || NULL == child_class_id) {
		return -MCI_EINVAL;
	}

	const suit_manifest_class_id_t *existing_parent_class_id = NULL;
	int ret = mci_get_manifest_parent(child_class_id, &existing_parent_class_id);

	if (0 != ret) {
		return ret;
	}

	if (NULL == existing_parent_class_id) {
		return -MCI_ENOACCESS;
	}

	if (0 == mci_compare_suit_uuid(parent_class_id, existing_parent_class_id)) {
		return 0;
	}

	return -MCI_ENOACCESS;
}
