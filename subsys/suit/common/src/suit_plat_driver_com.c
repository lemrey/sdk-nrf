/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-4-Clause
 */
#include <zcbor_decode.h>
#include <suit_platform_internal.h>
#include <suit_plat_driver_com.h>

/*
 * It is assumed that in future, sets of vid_uuid_raw and cid_uuid_raw will be stored securely
 * and selected for comparision based on component "type".
 * In such case check_vid and check_cid should be common for all drivers.
 * Also current defined in code vid_uuid_raw and cid_uuid_raw can be considered common.
 */

/** The value of UUID5("nordicsemi.no"). */
static const uint8_t vid_uuid_raw[] = {
	0x2B, 0xDC, 0x1C, 0x07, 0xE0, 0xD1, 0x54, 0x84,
	0xBE, 0x50, 0x63, 0x17, 0x4D, 0x5A, 0x74, 0xC3,
};

/** The value of UUID5("nRF5420_cpuapp"). */
static const uint8_t cid_uuid_raw[] = {
	0x85, 0x20, 0xEA, 0x9C, 0x51, 0x5E, 0x57, 0x79,
	0x8B, 0x5F, 0xBD, 0xAD, 0x67, 0xDE, 0xC7, 0xD9,
};

int plat_com_check_vid(suit_component_t handle, struct zcbor_string *vid_uuid)
{
	static const struct zcbor_string exp_vid_uuid = {
		.value = vid_uuid_raw,
		.len = sizeof(vid_uuid_raw),
	};

	return (suit_compare_zcbor_strings(&exp_vid_uuid, vid_uuid) ? SUIT_SUCCESS :
								      SUIT_FAIL_CONDITION);
}

int plat_com_check_cid(suit_component_t handle, struct zcbor_string *cid_uuid)
{
	static const struct zcbor_string exp_cid_uuid = {
		.value = cid_uuid_raw,
		.len = sizeof(cid_uuid_raw),
	};

	return (suit_compare_zcbor_strings(&exp_cid_uuid, cid_uuid) ? SUIT_SUCCESS :
								      SUIT_FAIL_CONDITION);
}

bool plat_com_decode_component_id(struct zcbor_string *component_id, uint8_t *cpu_id,
				  intptr_t *run_address, size_t *size)
{
	ZCBOR_STATE_D(state, 2, component_id->value, component_id->len,
		      SUIT_MAX_NUM_COMPONENT_ID_PARTS);
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
