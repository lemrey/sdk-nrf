/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zcbor_decode.h>
#include <suit_plat_decode_util.h>
#include <string.h>

bool suit_plat_decode_component_id(struct zcbor_string *component_id, uint8_t *cpu_id,
				   intptr_t *run_address, size_t *size)
{
	ZCBOR_STATE_D(state, 2, component_id->value, component_id->len,
		      SUIT_PLAT_MAX_NUM_COMPONENT_ID_PARTS);
	struct zcbor_string component_type;
	bool res;

	res = zcbor_list_start_decode(state);
	res = res && zcbor_bstr_decode(state, &component_type);
	res = res && zcbor_bstr_start_decode(state, NULL);
	res = res && zcbor_int_decode(state, cpu_id, 1);
	res = res && zcbor_bstr_end_decode(state);
	res = res && zcbor_bstr_start_decode(state, NULL);
	res = res && zcbor_size_decode(state, (size_t *)run_address);
	res = res && zcbor_bstr_end_decode(state);
	res = res && zcbor_bstr_start_decode(state, NULL);
	res = res && zcbor_size_decode(state, (size_t *)size);
	res = res && zcbor_bstr_end_decode(state);
	res = res && zcbor_list_end_decode(state);

	return res;
}

#ifdef CONFIG_SUIT_PLATFORM
bool suit_plat_decode_component_type(struct zcbor_string *component_id, suit_component_type_t *type)
{
	ZCBOR_STATE_D(state, 2, component_id->value, component_id->len,
		      SUIT_PLAT_MAX_NUM_COMPONENT_ID_PARTS);
	struct zcbor_string tmp;
	bool res;

	res = zcbor_list_start_decode(state);
	res = res && zcbor_bstr_start_decode(state, NULL);
	res = res && zcbor_tstr_decode(state, &tmp);
	res = res && zcbor_bstr_end_decode(state);

	if ((type == NULL) || (!res)) {
		return false;
	}

	if ((tmp.len == strlen("MEM")) && (memcmp(tmp.value, "MEM", tmp.len) == 0)) {
		*type = SUIT_COMPONENT_TYPE_MEM;
	} else if ((tmp.len == strlen("CAND_IMG")) &&
		   (memcmp(tmp.value, "CAND_IMG", tmp.len) == 0)) {
		*type = SUIT_COMPONENT_TYPE_CAND_IMG;
	} else if ((tmp.len == strlen("CAND_MFST")) &&
		   (memcmp(tmp.value, "CAND_MFST", tmp.len) == 0)) {
		*type = SUIT_COMPONENT_TYPE_CAND_MFST;
	} else if ((tmp.len == strlen("INSTLD_MFST")) &&
		   (memcmp(tmp.value, "INSTLD_MFST", tmp.len) == 0)) {
		*type = SUIT_COMPONENT_TYPE_INSTLD_MFST;
	} else if ((tmp.len == strlen("SOC_SPEC")) &&
		   (memcmp(tmp.value, "SOC_SPEC", tmp.len) == 0)) {
		*type = SUIT_COMPONENT_TYPE_SOC_SPEC;
	} else if ((tmp.len == strlen("CACHE_POOL")) &&
		   (memcmp(tmp.value, "CACHE_POOL", tmp.len) == 0)) {
		*type = SUIT_COMPONENT_TYPE_CACHE_POOL;
	} else {
		*type = SUIT_COMPONENT_TYPE_UNSUPPORTED;
		return false;
	}

	return res;
}
#endif /* CONFIG_SUIT_PLATFORM */

bool suit_plat_decode_address_size(struct zcbor_string *component_id, intptr_t *run_address,
				   size_t *size)
{
	ZCBOR_STATE_D(state, 2, component_id->value, component_id->len,
		      SUIT_PLAT_MAX_NUM_COMPONENT_ID_PARTS);
	struct zcbor_string component_type;
	bool res;
	uint8_t cpu_id;

	res = zcbor_list_start_decode(state);
	res = res && zcbor_bstr_decode(state, &component_type);
	res = res && zcbor_bstr_start_decode(state, NULL);
	res = res && zcbor_int_decode(state, &cpu_id, 1);
	res = res && zcbor_bstr_end_decode(state);
	res = res && zcbor_bstr_start_decode(state, NULL);
	res = res && zcbor_size_decode(state, (size_t *)run_address);
	res = res && zcbor_bstr_end_decode(state);
	res = res && zcbor_bstr_start_decode(state, NULL);
	res = res && zcbor_size_decode(state, (size_t *)size);
	res = res && zcbor_bstr_end_decode(state);
	res = res && zcbor_list_end_decode(state);

	return res;
}

bool suit_plat_decode_component_number(struct zcbor_string *component_id, uint32_t *number)
{
	ZCBOR_STATE_D(state, 2, component_id->value, component_id->len,
		      SUIT_PLAT_MAX_NUM_COMPONENT_ID_PARTS);
	struct zcbor_string component_type;

	bool res = zcbor_list_start_decode(state);
	res = res && zcbor_bstr_decode(state, &component_type);
	res = res && zcbor_bstr_start_decode(state, NULL);
	res = res && zcbor_uint32_decode(state, number);

	return res;
}

#ifdef CONFIG_SUIT_MCI
bool suit_plat_decode_manifest_class_id(struct zcbor_string *component_id,
					suit_manifest_class_id_t **class_id)
{
	struct zcbor_string component_type;
	struct zcbor_string class_id_bstr;
	bool res;

	if ((component_id == NULL) || (class_id == NULL)) {
		return false;
	}

	ZCBOR_STATE_D(state, 2, component_id->value, component_id->len,
		      SUIT_PLAT_MAX_NUM_COMPONENT_ID_PARTS);

	res = zcbor_list_start_decode(state);
	res = res && zcbor_bstr_start_decode(state, NULL);
	res = res && zcbor_tstr_decode(state, &component_type);
	res = res && zcbor_bstr_end_decode(state);
	res = res && zcbor_bstr_decode(state, &class_id_bstr);

	if (res) {
		if ((component_type.len != strlen("INSTLD_MFST")) ||
		    (memcmp(component_type.value, "INSTLD_MFST", component_type.len) != 0)) {
			res = false;
		}
	}

	if (res) {
		if (class_id_bstr.len != sizeof(suit_manifest_class_id_t)) {
			res = false;
		} else {
			*class_id = (suit_uuid_t *)class_id_bstr.value;
		}
	}
	return res;
}
#endif /* CONFIG_SUIT_MCI */
