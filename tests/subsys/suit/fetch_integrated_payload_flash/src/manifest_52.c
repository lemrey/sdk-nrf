/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#if defined(CONFIG_SOC_NRF52840) || defined(CONFIG_BOARD_NATIVE_POSIX)
#include <suit_types.h>

/** @brief SUIT envelope generated using the manifest/manifest_52.yaml input file.
 *
 * @details This envelope contains 256-byte random numbers attached as an integrated
 *          payload and fetched directly into the memory-mapped region during install step.
 *          The envelope has been signed using the manifest/key_private.pem key.
 *
 * @note Please use the manifest_common/regenerate.sh script for regenerating payloads.
 */
uint8_t manifest_buf[] = {
	0xd8, 0x6b, 0xa4, 0x02, 0x58, 0x73, 0x82, 0x58, 0x24, 0x82, 0x2f, 0x58, 0x20, 0x2a, 0x5f,
	0xe2, 0xff, 0xac, 0x00, 0x46, 0x26, 0xa8, 0x30, 0x1f, 0xa7, 0xb7, 0xe3, 0x32, 0xfe, 0x32,
	0x20, 0x76, 0xcb, 0x50, 0xe5, 0xb1, 0x88, 0x49, 0x5e, 0x42, 0x00, 0x91, 0xcb, 0x40, 0xeb,
	0x58, 0x4a, 0xd2, 0x84, 0x43, 0xa1, 0x01, 0x26, 0xa0, 0xf6, 0x58, 0x40, 0x03, 0xfc, 0x40,
	0xe3, 0x4a, 0xc8, 0x77, 0xb7, 0xb5, 0xff, 0x2c, 0xbe, 0x9b, 0x06, 0x46, 0x84, 0xb5, 0x17,
	0x78, 0x09, 0x31, 0x1d, 0xe1, 0xee, 0x70, 0xa5, 0xdf, 0x35, 0x95, 0x9b, 0x72, 0x1f, 0x83,
	0x03, 0xba, 0xa1, 0x87, 0x0f, 0xd3, 0xc5, 0x64, 0x70, 0xae, 0x0f, 0xce, 0xee, 0xd8, 0xa9,
	0x2c, 0xff, 0xac, 0x03, 0x33, 0x71, 0x4e, 0xce, 0x29, 0x45, 0x42, 0x70, 0x6b, 0x95, 0xbb,
	0x92, 0x03, 0x58, 0xd9, 0xa8, 0x01, 0x01, 0x02, 0x01, 0x03, 0x58, 0x6e, 0xa2, 0x02, 0x81,
	0x84, 0x44, 0x63, 0x4d, 0x45, 0x4d, 0x41, 0x00, 0x45, 0x1a, 0x00, 0x08, 0x00, 0x00, 0x43,
	0x19, 0x10, 0x00, 0x04, 0x58, 0x56, 0x86, 0x14, 0xa4, 0x01, 0x50, 0x76, 0x17, 0xda, 0xa5,
	0x71, 0xfd, 0x5a, 0x85, 0x8f, 0x94, 0xe2, 0x8d, 0x73, 0x5c, 0xe9, 0xf4, 0x02, 0x50, 0x56,
	0xdc, 0x9a, 0x14, 0x28, 0xd8, 0x52, 0xd3, 0xbd, 0x62, 0xe7, 0x7a, 0x08, 0xbc, 0x8b, 0x91,
	0x03, 0x58, 0x24, 0x82, 0x2f, 0x58, 0x20, 0x5f, 0xc3, 0x54, 0xbf, 0x8e, 0x8c, 0x50, 0xfb,
	0x4f, 0xbc, 0x2c, 0xfa, 0xeb, 0x04, 0x53, 0x41, 0xc9, 0x80, 0x6d, 0xea, 0xbd, 0xcb, 0x41,
	0x54, 0xfb, 0x79, 0xcc, 0xa4, 0xf0, 0xc9, 0x8c, 0x12, 0x0e, 0x19, 0x01, 0x00, 0x01, 0x0f,
	0x02, 0x0f, 0x07, 0x43, 0x82, 0x03, 0x0f, 0x09, 0x43, 0x82, 0x17, 0x02, 0x11, 0x52, 0x86,
	0x14, 0xa1, 0x15, 0x69, 0x23, 0x66, 0x69, 0x6c, 0x65, 0x2e, 0x62, 0x69, 0x6e, 0x15, 0x02,
	0x03, 0x0f, 0x17, 0x82, 0x2f, 0x58, 0x20, 0xd6, 0x60, 0x44, 0xf7, 0x35, 0x1c, 0x96, 0x0e,
	0xe3, 0x78, 0x47, 0x18, 0xa8, 0x14, 0xff, 0xc0, 0xd0, 0x48, 0x3b, 0x22, 0x72, 0x5e, 0xe6,
	0x96, 0x6a, 0x38, 0x6b, 0x4a, 0x7d, 0x45, 0x4f, 0xcb, 0x05, 0x82, 0x4c, 0x6b, 0x49, 0x4e,
	0x53, 0x54, 0x4c, 0x44, 0x5f, 0x4d, 0x46, 0x53, 0x54, 0x50, 0x56, 0xdc, 0x9a, 0x14, 0x28,
	0xd8, 0x52, 0xd3, 0xbd, 0x62, 0xe7, 0x7a, 0x08, 0xbc, 0x8b, 0x91, 0x17, 0x58, 0x84, 0xa1,
	0x84, 0x44, 0x63, 0x4d, 0x45, 0x4d, 0x41, 0x00, 0x45, 0x1a, 0x00, 0x08, 0x00, 0x00, 0x43,
	0x19, 0x10, 0x00, 0xa6, 0x01, 0x78, 0x18, 0x4e, 0x6f, 0x72, 0x64, 0x69, 0x63, 0x20, 0x53,
	0x65, 0x6d, 0x69, 0x63, 0x6f, 0x6e, 0x64, 0x75, 0x63, 0x74, 0x6f, 0x72, 0x20, 0x41, 0x53,
	0x41, 0x02, 0x65, 0x70, 0x6f, 0x73, 0x69, 0x78, 0x03, 0x6e, 0x6e, 0x6f, 0x72, 0x64, 0x69,
	0x63, 0x73, 0x65, 0x6d, 0x69, 0x2e, 0x63, 0x6f, 0x6d, 0x04, 0x75, 0x54, 0x68, 0x65, 0x20,
	0x50, 0x4f, 0x53, 0x49, 0x58, 0x20, 0x61, 0x70, 0x70, 0x6c, 0x69, 0x63, 0x61, 0x74, 0x69,
	0x6f, 0x6e, 0x05, 0x78, 0x1c, 0x53, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x20, 0x61, 0x70, 0x70,
	0x6c, 0x69, 0x63, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x66, 0x6f, 0x72, 0x20, 0x50, 0x4f,
	0x53, 0x49, 0x58, 0x06, 0x66, 0x76, 0x31, 0x2e, 0x30, 0x2e, 0x30, 0x69, 0x23, 0x66, 0x69,
	0x6c, 0x65, 0x2e, 0x62, 0x69, 0x6e, 0x59, 0x01, 0x00, 0xc7, 0x9c, 0xab, 0x9d, 0xe8, 0x33,
	0x7f, 0x30, 0x14, 0xeb, 0xac, 0x02, 0xaf, 0x26, 0x01, 0x5e, 0x80, 0x6d, 0x88, 0xa1, 0xdb,
	0x11, 0xa7, 0x31, 0xdf, 0xa6, 0xec, 0xcb, 0x9b, 0x48, 0x0d, 0xc8, 0x34, 0x40, 0x6d, 0x30,
	0x86, 0x7d, 0xe8, 0x1b, 0xec, 0x3c, 0xf5, 0x40, 0xd0, 0x48, 0x18, 0x82, 0x11, 0x9d, 0x7c,
	0x3f, 0x6c, 0xe5, 0x8f, 0xf1, 0xd3, 0x5d, 0xe1, 0x51, 0xf7, 0x6a, 0x0f, 0xaf, 0x0b, 0xbd,
	0x4c, 0x5f, 0xa5, 0x34, 0x1a, 0x66, 0xdb, 0x22, 0xec, 0x63, 0xed, 0x4b, 0xab, 0xc7, 0xc8,
	0xf7, 0x59, 0xd8, 0xd6, 0x9e, 0xec, 0x71, 0x1b, 0x24, 0x20, 0xb9, 0xae, 0xe1, 0x3b, 0xfc,
	0xae, 0xb8, 0x77, 0xac, 0xa4, 0x57, 0x34, 0x97, 0x84, 0x4f, 0x58, 0xd5, 0x68, 0x08, 0x6f,
	0xe3, 0x9c, 0x7e, 0x1b, 0xd7, 0x38, 0x22, 0x98, 0x48, 0xf8, 0x7a, 0x67, 0xb2, 0xd9, 0xac,
	0xc5, 0x34, 0xc1, 0x27, 0x82, 0x8e, 0x42, 0x79, 0x84, 0x21, 0x37, 0x4c, 0x41, 0x4a, 0x0f,
	0xe2, 0x7f, 0xa0, 0x6a, 0x19, 0x13, 0x3d, 0x52, 0x22, 0x7f, 0xd6, 0x2f, 0x71, 0x12, 0x76,
	0xab, 0x25, 0x9c, 0xfc, 0x67, 0x08, 0x03, 0x7c, 0xdb, 0x18, 0xe6, 0x45, 0xf8, 0x99, 0xc2,
	0x9e, 0x2c, 0xe3, 0x9b, 0x25, 0xa9, 0x7b, 0x09, 0xff, 0x00, 0x57, 0x26, 0x08, 0x0a, 0x11,
	0x42, 0xcf, 0x82, 0xa2, 0x6b, 0x2a, 0x99, 0xf9, 0x71, 0x9d, 0x14, 0x19, 0x5c, 0x5c, 0x78,
	0x31, 0x60, 0x42, 0x4a, 0x18, 0x1f, 0xec, 0x78, 0x6a, 0x9a, 0x7c, 0x4f, 0xcf, 0xe8, 0x5a,
	0x29, 0x65, 0xcd, 0x01, 0x3b, 0x6d, 0x53, 0xbb, 0xc6, 0xdb, 0xda, 0xd5, 0x8f, 0xf7, 0xf4,
	0xd9, 0xb9, 0x0a, 0x03, 0x4b, 0xff, 0x33, 0xab, 0x3b, 0xc5, 0xaf, 0xd0, 0xb8, 0x2c, 0x0f,
	0x6a, 0xa9, 0x11, 0xb0, 0xe8, 0x57, 0x8c, 0x92, 0x53, 0x81};

const size_t manifest_len = sizeof(manifest_buf);
#endif /* CONFIG_SOC_NRF52840 || CONFIG_BOARD_NATIVE_POSIX */
