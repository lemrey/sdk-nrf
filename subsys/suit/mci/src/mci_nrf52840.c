/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <suit_mci.h>

/* nRF52840 topology: Root Manifest orchestrating application domain.
 * Summarizing - Single Root Manifest and one local manifest.
 */

/* RFC4122 uuid5(nordic_vid, 'posix_sample_root')
 */
static const suit_manifest_class_id_t nordic_root_manifest_class_id = {
	{0xc8, 0x36, 0x30, 0xf5, 0x9e, 0x0f, 0x5f, 0xe4, 0x8e, 0xff, 0x34, 0x27, 0xd3, 0x64, 0x7d,
	 0x97}};

/* RFC4122 uuid5(nordic_vid, 'posix_sample_app')
 */
static const suit_manifest_class_id_t nordic_app_manifest_class_id = {
	{0x56, 0xdc, 0x9a, 0x14, 0x28, 0xd8, 0x52, 0xd3, 0xbd, 0x62, 0xe7, 0x7a, 0x08, 0xbc, 0x8b,
	 0x91}};

typedef struct {
	const suit_manifest_class_id_t *manifest_class_id;
	const suit_manifest_class_id_t *parent_manifest_class_id;
	downgrade_prevention_policy_t downgrade_prevention_policy;
	uint32_t signing_key_bits;
	uint32_t signing_key_mask;
} manifest_config_t;

static const manifest_config_t supported_manifests[] = {
	{&nordic_root_manifest_class_id, NULL, DOWNGRADE_PREVENTION_DISABLED,
	 /* signing_key_mask equal to 0 means signing is not required
	  */
	 0x00000000, 0x00000000},
	{&nordic_app_manifest_class_id, &nordic_root_manifest_class_id,
	 DOWNGRADE_PREVENTION_DISABLED,
	 /* signing_key_mask equal to 0 means signing is not required
	  */
	 0x00000000, 0x00000000}};

static const manifest_config_t *
find_manifest_config(const suit_manifest_class_id_t *manifest_class_id)
{
	for (int i = 0; i < sizeof(supported_manifests) / sizeof(manifest_config_t); ++i) {
		const manifest_config_t *manifest_config = &supported_manifests[i];

		if (0 ==
		    mci_compare_suit_uuid(manifest_config->manifest_class_id, manifest_class_id)) {
			return manifest_config;
		}
	}
	return NULL;
}

int mci_get_supported_manifest_class_ids(const suit_manifest_class_id_t **class_id, size_t *size)
{
	if (NULL == class_id || NULL == size) {
		return -MCI_EINVAL;
	}

	size_t output_max_size = *size;
	size_t output_size = sizeof(supported_manifests) / sizeof(manifest_config_t);

	if (output_size > output_max_size) {
		return -MCI_ESIZE;
	}

	for (int i = 0; i < output_size; ++i) {
		class_id[i] = supported_manifests[i].manifest_class_id;
	}

	*size = output_size;
	return 0;
}

int mci_get_invoke_order(const suit_manifest_class_id_t **class_id, size_t *size)
{
	if (NULL == class_id || NULL == size) {
		return -MCI_EINVAL;
	}
	size_t output_max_size = *size;

	/* In this implementation the only manifest which shall be utilized to initiate
	 *  system bringup is a root manifest
	 */
	size_t output_size = 1;

	if (output_size > output_max_size) {
		return -MCI_ESIZE;
	}

	class_id[0] = &nordic_root_manifest_class_id;
	*size = output_size;
	return 0;
}

int mci_get_downgrade_prevention_policy(const suit_manifest_class_id_t *class_id,
					downgrade_prevention_policy_t *policy)
{
	if (NULL == class_id || NULL == policy) {
		return -MCI_EINVAL;
	}

	const manifest_config_t *manifest_config = find_manifest_config(class_id);

	if (NULL == manifest_config) {
		return -MCI_EMANIFESTCLASSID;
	}
	*policy = manifest_config->downgrade_prevention_policy;
	return 0;
}

int mci_validate_manifest_class_id(const suit_manifest_class_id_t *class_id)
{
	if (NULL == class_id) {
		return -MCI_EINVAL;
	}

	const manifest_config_t *manifest_config = find_manifest_config(class_id);

	if (NULL == manifest_config) {
		return -MCI_EMANIFESTCLASSID;
	}
	return 0;
}

int mci_validate_signing_key_id(const suit_manifest_class_id_t *class_id, uint32_t key_id)
{
	if (NULL == class_id) {
		return -MCI_EINVAL;
	}

	const manifest_config_t *manifest_config = find_manifest_config(class_id);

	if (NULL == manifest_config) {
		return -MCI_EMANIFESTCLASSID;
	}

	if ((manifest_config->signing_key_bits & manifest_config->signing_key_mask) !=
	    (key_id & manifest_config->signing_key_mask)) {
		return MCI_EWRONGKEYID;
	}

	return 0;
}

int mci_validate_processor_start_rights(const suit_manifest_class_id_t *class_id, int processor_id)
{
	if (NULL == class_id) {
		return -MCI_EINVAL;
	}

	const manifest_config_t *manifest_config = find_manifest_config(class_id);

	if (NULL == manifest_config) {
		return -MCI_EMANIFESTCLASSID;
	}

	if (0 == mci_compare_suit_uuid(&nordic_root_manifest_class_id, class_id)) {
		/* Root manifest - ability to start any cpu are intentionally blocked
		 */
		return -MCI_ENOACCESS;

	} else if (0 == mci_compare_suit_uuid(&nordic_app_manifest_class_id, class_id)) {
		/* Application manifest. Use "0" as CPU ID on POSIX
		 */
		if (0 == processor_id) {
			return 0;
		}

		return -MCI_ENOACCESS;
	}

	return -MCI_ENOACCESS;
}

int mci_validate_memory_access_rights(const suit_manifest_class_id_t *class_id, void *address,
				      size_t mem_size)
{
	if (NULL == class_id || NULL == address || 0 == mem_size) {
		return -MCI_EINVAL;
	}

	const manifest_config_t *manifest_config = find_manifest_config(class_id);

	if (NULL == manifest_config) {
		return -MCI_EMANIFESTCLASSID;
	}

	if (0 == mci_compare_suit_uuid(&nordic_root_manifest_class_id, class_id)) {
		/* Root manifest - ability to operate on memory ranges intentionally blocked
		 */
		return -MCI_ENOACCESS;

	} else if (0 == mci_compare_suit_uuid(&nordic_app_manifest_class_id, class_id)) {
		/* Application manifest - allow to overwrite any address
		 */
		return 0;
	}

	return -MCI_ENOACCESS;
}

int mci_validate_platform_specific_component_rights(const suit_manifest_class_id_t *class_id,
						    int platform_specific_component_number)
{
	if (NULL == class_id) {
		return -MCI_EINVAL;
	}

	const manifest_config_t *manifest_config = find_manifest_config(class_id);

	if (NULL == manifest_config) {
		return -MCI_EMANIFESTCLASSID;
	}

	return -MCI_ENOACCESS;
}

int mci_get_manifest_parent(const suit_manifest_class_id_t *child_class_id,
			    const suit_manifest_class_id_t **parent_class_id)
{
	if (NULL == child_class_id || NULL == parent_class_id) {
		return -MCI_EINVAL;
	}

	const manifest_config_t *child_manifest_config = find_manifest_config(child_class_id);

	if (NULL == child_manifest_config) {
		return -MCI_EMANIFESTCLASSID;
	}

	*parent_class_id = child_manifest_config->parent_manifest_class_id;

	return 0;
}

int mci_get_vendor_id_for_manifest_class_id(const suit_manifest_class_id_t *class_id,
					    const suit_uuid_t **vendor_id)
{
	if (NULL == class_id || NULL == vendor_id) {
		return -MCI_EINVAL;
	}

	const manifest_config_t *manifest_config = find_manifest_config(class_id);

	if (NULL == manifest_config) {
		return -MCI_EMANIFESTCLASSID;
	}

	return mci_get_nordic_vendor_id(vendor_id);
}

int mci_init(void)
{
	return 0;
}
