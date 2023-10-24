/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_mci.h>
#include <suit_plat_decode_util.h>
#include <suit_platform_internal.h>

int suit_plat_check_component_compatibility(const suit_manifest_class_id_t *class_id,
					    struct zcbor_string *component_id)
{
	suit_manifest_class_id_t *decoded_class_id;
	suit_component_type_t type = SUIT_COMPONENT_TYPE_UNSUPPORTED;
	intptr_t address;
	uint32_t number;
	uint8_t cpu_id;
	size_t size;

	if ((class_id == NULL) || (component_id == NULL) || (component_id->value == NULL) ||
	    (component_id->len == 0)) {
		return SUIT_ERR_DECODING;
	}

	/* Validate manifest class ID against supported manifests */
	int ret = mci_validate_manifest_class_id(class_id);
	if (ret != 0) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	if (!suit_plat_decode_component_type(component_id, &type)) {
		return SUIT_ERR_DECODING;
	}

	switch (type) {
	case SUIT_COMPONENT_TYPE_MEM:
		/* Decode component_id */
		if (!suit_plat_decode_component_id(component_id, &cpu_id, &address, &size)) {
			return SUIT_ERR_DECODING;
		}

		ret = mci_validate_processor_start_rights(class_id, cpu_id);
		if (ret != SUIT_SUCCESS) {
			return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
		}

		ret = mci_validate_memory_access_rights(class_id, (void *)address, size);
		if (ret != SUIT_SUCCESS) {
			return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
		}
		break;
	case SUIT_COMPONENT_TYPE_SOC_SPEC:
		if (!suit_plat_decode_component_number(component_id, &number)) {
			return SUIT_ERR_DECODING;
		}

		ret = mci_validate_platform_specific_component_rights(class_id, number);
		if (ret != SUIT_SUCCESS) {
			return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
		}
		break;
	case SUIT_COMPONENT_TYPE_CAND_MFST:
	case SUIT_COMPONENT_TYPE_CAND_IMG:
	case SUIT_COMPONENT_TYPE_CACHE_POOL:
		if (!suit_plat_decode_component_number(component_id, &number)) {
			return SUIT_ERR_DECODING;
		}
		break;

	case SUIT_COMPONENT_TYPE_INSTLD_MFST:
		/* Decode manifest class id */
		if (!suit_plat_decode_manifest_class_id(component_id, &decoded_class_id)) {
			return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
		}

		/* Validate manifest class ID against supported manifests */
		ret = mci_validate_manifest_class_id(decoded_class_id);
		if (ret != 0) {
			return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
		}

		break;
	default:
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	return SUIT_SUCCESS;
}
