/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <suit_mci.h>

/* RFC4122 uuid5(uuid.NAMESPACE_DNS, 'nordicsemi.com')
 */
static const suit_uuid_t nordic_vendor_id = {{0x76, 0x17, 0xda, 0xa5, 0x71, 0xfd, 0x5a, 0x85, 0x8f,
					      0x94, 0xe2, 0x8d, 0x73, 0x5c, 0xe9, 0xf4}};

/* RFC4122 uuid5(nordic_vid, 'unspecified_class')
 */
static const suit_uuid_t nordic_unspecified_class_id = {{0xca, 0xd8, 0x52, 0x3a, 0xf8, 0x29, 0x5a,
							 0x9a, 0xba, 0x85, 0x2e, 0xa0, 0xb2, 0xf5,
							 0x77, 0xc9}};

mci_err_t mci_get_nordic_vendor_id(const suit_uuid_t **vendor_id)
{
	if (NULL == vendor_id) {
		return SUIT_PLAT_ERR_INVAL;
	}
	*vendor_id = &nordic_vendor_id;
	return SUIT_PLAT_SUCCESS;
}

mci_err_t mci_get_nordic_unspecified_class_id(const suit_uuid_t **class_id)
{
	if (NULL == class_id) {
		return SUIT_PLAT_ERR_INVAL;
	}
	*class_id = &nordic_unspecified_class_id;
	return SUIT_PLAT_SUCCESS;
}
