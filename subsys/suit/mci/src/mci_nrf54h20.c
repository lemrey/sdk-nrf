/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <suit_mci.h>
#include <drivers/nrfx_common.h>

/* nRF54H20 topology: Root Manifest orchestrating app, radio, sec (with system controller).
 * Summarizing - Single Root Manifest and three local manifests.
 */

/* RFC4122 uuid5(nordic_vid, 'nRF54H20_sample_root')
 */
static const suit_manifest_class_id_t nordic_root_manifest_class_id = {
	{0x3f, 0x6a, 0x3a, 0x4d, 0xcd, 0xfa, 0x58, 0xc5, 0xac, 0xce, 0xf9, 0xf5, 0x84, 0xc4, 0x11,
	 0x24}};

/* RFC4122 uuid5(nordic_vid, 'nRF54H20_sample_app')
 */
static const suit_manifest_class_id_t nordic_app_manifest_class_id = {
	{0x08, 0xc1, 0xb5, 0x99, 0x55, 0xe8, 0x5f, 0xbc, 0x9e, 0x76, 0x7b, 0xc2, 0x9c, 0xe1, 0xb0,
	 0x4d}};

/* RFC4122 uuid5(nordic_vid, 'nRF54H20_sample_rad')
 */
static const suit_manifest_class_id_t nordic_rad_manifest_class_id = {
	{0x81, 0x6a, 0xa0, 0xa0, 0xaf, 0x11, 0x5e, 0xf2, 0x85, 0x8a, 0xfe, 0xb6, 0x68, 0xb2, 0xe9,
	 0xc9}};

/* RFC4122 uuid5(nordic_vid, 'nRF54H20_sec_sys')
 */
static const suit_manifest_class_id_t nordic_sec_sys_manifest_class_id = {
	{0x75, 0x81, 0x1f, 0x10, 0x09, 0x94, 0x57, 0x72, 0xad, 0xf7, 0x71, 0x88, 0xbc, 0x8c, 0x5b,
	 0x73}};

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
	{&nordic_sec_sys_manifest_class_id, &nordic_root_manifest_class_id,
	 DOWNGRADE_PREVENTION_DISABLED,
	 /* signing_key_mask equal to 0 means signing is not required
	  */
	 0x00000000, 0x00000000},
	{&nordic_app_manifest_class_id, &nordic_root_manifest_class_id,
	 DOWNGRADE_PREVENTION_DISABLED,
	 /* signing_key_mask equal to 0 means signing is not required
	  */
	 0x00000000, 0x00000000},
	{&nordic_rad_manifest_class_id, &nordic_root_manifest_class_id,
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

	} else if (0 == mci_compare_suit_uuid(&nordic_sec_sys_manifest_class_id, class_id)) {
		/* Sec manifest
		 */
		if (NRF_PROCESSOR_SYSCTRL == processor_id) {
			/* SysCtrl
			 */
			return 0;
		}

		return -MCI_ENOACCESS;

	} else if (0 == mci_compare_suit_uuid(&nordic_app_manifest_class_id, class_id)) {
		/* App manifest.
		 * TODO - implement verification for NRF_PROCESSOR_ID_PPR(13) and
		 * NRF_PROCESSOR_ID_FLPR(14) support(based on UICR)
		 */
		if (NRF_PROCESSOR_APPLICATION == processor_id) {
			/* Appcore
			 */
			return 0;
		}

		return -MCI_ENOACCESS;

	} else if (0 == mci_compare_suit_uuid(&nordic_rad_manifest_class_id, class_id)) {
		/* Rad manifest
		 * TODO - implement verification for NRF_PROCESSOR_ID_PPR(13) and
		 * NRF_PROCESSOR_ID_FLPR(14) support(based on UICR)
		 */
		if (NRF_PROCESSOR_RADIOCORE == processor_id) {
			/* Radiocore
			 */
			return 0;
		} else if (NRF_PROCESSOR_BBPR == processor_id) {
			/* BBPR
			 */
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

	} else if (0 == mci_compare_suit_uuid(&nordic_sec_sys_manifest_class_id, class_id)) {
		/* Sec and sysctrl manifest - TODO - implement checks based on UICR/SICR
		 */
		return 0;

	} else if (0 == mci_compare_suit_uuid(&nordic_app_manifest_class_id, class_id)) {
		/* App manifest - TODO - implement checks based on UICR
		 */
		return 0;

	} else if (0 == mci_compare_suit_uuid(&nordic_rad_manifest_class_id, class_id)) {
		/* Rad manifest - TODO - implement checks based on UICR
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

	if (0 == mci_compare_suit_uuid(&nordic_sec_sys_manifest_class_id, class_id)) {
		/* The only manifest with ability to control platform specific components is secdom.
		 * 0 - SDFW Firmware
		 * 1 - SDFW Recovery Firmware
		 */
		if (0 == platform_specific_component_number ||
		    1 == platform_specific_component_number) {
			return 0;
		}
		return -MCI_ENOACCESS;
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
