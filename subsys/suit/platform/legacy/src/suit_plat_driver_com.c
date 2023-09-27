/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <suit_plat_driver_com.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(suit_plat_driver_com, CONFIG_SUIT_LOG_LEVEL);

/*
 * It is assumed that in future, sets of vid_uuid_raw and cid_uuid_raw will be stored securely
 * and selected for comparison based on component "type".
 * In such case check_vid and check_cid should be common for all drivers.
 * Also current defined in code vid_uuid_raw and cid_uuid_raw can be considered common.
 */

/** The value of UUID5("nordicsemi.com"). */
static const uint8_t vid_uuid_raw[] = {
	0x76, 0x17, 0xDA, 0xA5, 0x71, 0xFD, 0x5A, 0x85,
	0x8F, 0x94, 0xE2, 0x8D, 0x73, 0x5C, 0xE9, 0xF4,
};

#ifdef CONFIG_SOC_NRF54H20
/** The value of UUID5("nRF54H20_sample_app"). */
static const uint8_t cid_uuid_raw[] = {0x08, 0xc1, 0xb5, 0x99, 0x55, 0xe8, 0x5f, 0xbc,
				       0x9e, 0x76, 0x7b, 0xc2, 0x9c, 0xe1, 0xb0, 0x4d};
#else  /* CONFIG_SOC_NRF54H20 */
/* The value of UUID5("posix_sample_app"). */
static const uint8_t cid_uuid_raw[] = {0x56, 0xdc, 0x9a, 0x14, 0x28, 0xd8, 0x52, 0xd3,
				       0xbd, 0x62, 0xe7, 0x7a, 0x08, 0xbc, 0x8b, 0x91};
#endif /* CONFIG_SOC_NRF54H20 */

int plat_com_check_vid(suit_component_t handle, struct zcbor_string *vid_uuid)
{
	static const struct zcbor_string exp_vid_uuid = {
		.value = vid_uuid_raw,
		.len = sizeof(vid_uuid_raw),
	};

	return (suit_compare_zcbor_strings(&exp_vid_uuid, vid_uuid) ? SUIT_SUCCESS
								    : SUIT_FAIL_CONDITION);
}

int plat_com_check_cid(suit_component_t handle, struct zcbor_string *cid_uuid)
{
	static const struct zcbor_string exp_cid_uuid = {
		.value = cid_uuid_raw,
		.len = sizeof(cid_uuid_raw),
	};

	return (suit_compare_zcbor_strings(&exp_cid_uuid, cid_uuid) ? SUIT_SUCCESS
								    : SUIT_FAIL_CONDITION);
}
