/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOCK_SUIT_MCI_H__
#define MOCK_SUIT_MCI_H__

#include <zephyr/fff.h>
#include <mock_globals.h>

#include <suit_mci.h>

/* generic_ids.c */
#ifdef CONFIG_MOCK_SUIT_MCI_GENERIC_IDS
FAKE_VALUE_FUNC(int, mci_get_nordic_vendor_id, const suit_uuid_t **);
FAKE_VALUE_FUNC(int, mci_get_nordic_unspecified_class_id, const suit_uuid_t **);
#endif /* CONFIG_MOCK_SUIT_MCI_GENERIC_IDS */

/* utils.c */
#ifdef CONFIG_MOCK_SUIT_MCI_UTILS
FAKE_VALUE_FUNC(int, mci_compare_suit_uuid, const suit_uuid_t *, const suit_uuid_t *);
FAKE_VALUE_FUNC(int, mci_validate_manifest_parent_child, const suit_manifest_class_id_t *,
		const suit_manifest_class_id_t *);
#endif /* CONFIG_MOCK_SUIT_MCI_UTILS */

/* mci_<soc>.c */
FAKE_VALUE_FUNC(int, mci_get_supported_manifest_class_ids, const suit_manifest_class_id_t **,
		size_t *);
FAKE_VALUE_FUNC(int, mci_get_invoke_order, const suit_manifest_class_id_t **, size_t *);
FAKE_VALUE_FUNC(int, mci_get_downgrade_prevention_policy, const suit_manifest_class_id_t *,
		downgrade_prevention_policy_t *);
FAKE_VALUE_FUNC(int, mci_validate_manifest_class_id, const suit_manifest_class_id_t *);
FAKE_VALUE_FUNC(int, mci_validate_signing_key_id, const suit_manifest_class_id_t *, uint32_t);
FAKE_VALUE_FUNC(int, mci_validate_processor_start_rights, const suit_manifest_class_id_t *, int);
FAKE_VALUE_FUNC(int, mci_validate_memory_access_rights, const suit_manifest_class_id_t *, void *,
		size_t);
FAKE_VALUE_FUNC(int, mci_validate_platform_specific_component_rights,
		const suit_manifest_class_id_t *, int);
FAKE_VALUE_FUNC(int, mci_get_manifest_parent, const suit_manifest_class_id_t *,
		const suit_manifest_class_id_t **);
FAKE_VALUE_FUNC(int, mci_get_vendor_id_for_manifest_class_id, const suit_manifest_class_id_t *,
		const suit_uuid_t **);
FAKE_VALUE_FUNC(int, mci_init);

static inline void mock_suit_mci_reset(void)
{
#ifdef CONFIG_MOCK_SUIT_MCI_GENERIC_IDS
	RESET_FAKE(mci_get_nordic_vendor_id);
	RESET_FAKE(mci_get_nordic_unspecified_class_id);
#endif /* CONFIG_MOCK_SUIT_MCI_GENERIC_IDS */

#ifdef CONFIG_MOCK_SUIT_MCI_UTILS
	RESET_FAKE(mci_compare_suit_uuid);
	RESET_FAKE(mci_validate_manifest_parent_child);
#endif /* CONFIG_MOCK_SUIT_MCI_UTILS */

	RESET_FAKE(mci_get_supported_manifest_class_ids);
	RESET_FAKE(mci_get_invoke_order);
	RESET_FAKE(mci_get_downgrade_prevention_policy);
	RESET_FAKE(mci_validate_manifest_class_id);
	RESET_FAKE(mci_validate_signing_key_id);
	RESET_FAKE(mci_validate_processor_start_rights);
	RESET_FAKE(mci_validate_memory_access_rights);
	RESET_FAKE(mci_validate_platform_specific_component_rights);
	RESET_FAKE(mci_get_manifest_parent);
	RESET_FAKE(mci_get_vendor_id_for_manifest_class_id);
	RESET_FAKE(mci_init);
}

#endif /* MOCK_SUIT_MCI_H__ */
