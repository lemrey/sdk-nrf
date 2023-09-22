/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <stddef.h>

/** @brief SUIT envelope generated using the manifest/manifest_root_v2.yaml input file.
 *
 * @details This envelope contains 256-byte random numbers attached as an integrated
 *          payload and fetched directly into the memory-mapped region during install step.
 *          The envelope has been signed using the manifest/key_private.pem key.
 *
 * @note Please use the manifest_common/regenerate.sh script for regenerating payloads.
 */
uint8_t manifest_root_v2_buf[] = {
	0xd8, 0x6b, 0xa4, 0x02, 0x58, 0x73, 0x82, 0x58, 0x24, 0x82, 0x2f, 0x58, 0x20, 0x12, 0x8d,
	0x6f, 0xcd, 0xbd, 0xe5, 0xdd, 0x42, 0xf0, 0x16, 0x50, 0x15, 0x8c, 0xb2, 0x6a, 0x36, 0xc7,
	0xd2, 0x02, 0x73, 0x9d, 0xf5, 0x7d, 0xa3, 0x67, 0x44, 0xd4, 0x2a, 0x3c, 0x21, 0xe1, 0x53,
	0x58, 0x4a, 0xd2, 0x84, 0x43, 0xa1, 0x01, 0x26, 0xa0, 0xf6, 0x58, 0x40, 0xab, 0x31, 0x33,
	0x2d, 0xf6, 0x53, 0xd4, 0xef, 0x1c, 0xca, 0xeb, 0x09, 0xfe, 0x28, 0xce, 0x3b, 0x2d, 0xda,
	0x0f, 0xc3, 0x18, 0x21, 0x79, 0xda, 0x95, 0x74, 0x54, 0xb0, 0xd5, 0x77, 0x5c, 0x17, 0xd4,
	0x83, 0xdf, 0x1a, 0xea, 0x7e, 0x33, 0x12, 0x99, 0x0a, 0x0e, 0x6a, 0xcd, 0x39, 0x0e, 0x53,
	0x9a, 0x55, 0x96, 0x28, 0xe5, 0xa9, 0x88, 0x3d, 0x2b, 0x92, 0xe3, 0x91, 0x60, 0x5b, 0x31,
	0xc7, 0x03, 0x59, 0x01, 0x77, 0xa7, 0x01, 0x01, 0x02, 0x02, 0x03, 0x58, 0xb3, 0xa3, 0x02,
	0x83, 0x82, 0x4a, 0x69, 0x43, 0x41, 0x4e, 0x44, 0x5f, 0x4d, 0x46, 0x53, 0x54, 0x41, 0x00,
	0x82, 0x4c, 0x6b, 0x49, 0x4e, 0x53, 0x54, 0x4c, 0x44, 0x5f, 0x4d, 0x46, 0x53, 0x54, 0x50,
	0x81, 0x6a, 0xa0, 0xa0, 0xaf, 0x11, 0x5e, 0xf2, 0x85, 0x8a, 0xfe, 0xb6, 0x68, 0xb2, 0xe9,
	0xc9, 0x82, 0x4c, 0x6b, 0x49, 0x4e, 0x53, 0x54, 0x4c, 0x44, 0x5f, 0x4d, 0x46, 0x53, 0x54,
	0x50, 0x08, 0xc1, 0xb5, 0x99, 0x55, 0xe8, 0x5f, 0xbc, 0x9e, 0x76, 0x7b, 0xc2, 0x9c, 0xe1,
	0xb0, 0x4d, 0x04, 0x58, 0x59, 0x8e, 0x0c, 0x01, 0x14, 0xa2, 0x01, 0x50, 0x76, 0x17, 0xda,
	0xa5, 0x71, 0xfd, 0x5a, 0x85, 0x8f, 0x94, 0xe2, 0x8d, 0x73, 0x5c, 0xe9, 0xf4, 0x02, 0x50,
	0x81, 0x6a, 0xa0, 0xa0, 0xaf, 0x11, 0x5e, 0xf2, 0x85, 0x8a, 0xfe, 0xb6, 0x68, 0xb2, 0xe9,
	0xc9, 0x0c, 0x02, 0x14, 0xa2, 0x01, 0x50, 0x76, 0x17, 0xda, 0xa5, 0x71, 0xfd, 0x5a, 0x85,
	0x8f, 0x94, 0xe2, 0x8d, 0x73, 0x5c, 0xe9, 0xf4, 0x02, 0x50, 0x08, 0xc1, 0xb5, 0x99, 0x55,
	0xe8, 0x5f, 0xbc, 0x9e, 0x76, 0x7b, 0xc2, 0x9c, 0xe1, 0xb0, 0x4d, 0x0c, 0x82, 0x01, 0x02,
	0x01, 0x0f, 0x02, 0x0f, 0x01, 0xa3, 0x00, 0xa0, 0x01, 0xa0, 0x02, 0xa0, 0x07, 0x49, 0x86,
	0x0c, 0x82, 0x01, 0x02, 0x07, 0x0f, 0x0b, 0x0f, 0x09, 0x49, 0x86, 0x0c, 0x82, 0x01, 0x02,
	0x07, 0x0f, 0x0b, 0x0f, 0x11, 0x58, 0x83, 0x96, 0x0c, 0x00, 0x14, 0xa3, 0x15, 0x69, 0x23,
	0x72, 0x61, 0x64, 0x2e, 0x73, 0x75, 0x69, 0x74, 0x03, 0x58, 0x24, 0x82, 0x2f, 0x58, 0x20,
	0xad, 0x59, 0x18, 0x4d, 0xe9, 0x28, 0xcf, 0xc8, 0x7f, 0x31, 0xf6, 0xb0, 0xda, 0x13, 0x9a,
	0xb8, 0xeb, 0x4f, 0xf5, 0xad, 0xb0, 0x8f, 0x48, 0xf8, 0x58, 0xd0, 0x88, 0x1f, 0x7f, 0x36,
	0xb0, 0xca, 0x0e, 0x19, 0x01, 0x32, 0x15, 0x02, 0x03, 0x0f, 0x07, 0x0f, 0x0b, 0x0f, 0x14,
	0xa3, 0x15, 0x69, 0x23, 0x61, 0x70, 0x70, 0x2e, 0x73, 0x75, 0x69, 0x74, 0x03, 0x58, 0x24,
	0x82, 0x2f, 0x58, 0x20, 0x7d, 0x36, 0x64, 0x1f, 0xe6, 0xce, 0x9d, 0xeb, 0xa8, 0xb9, 0x25,
	0x7c, 0xf5, 0x0a, 0xa1, 0x30, 0xf5, 0x90, 0x71, 0xdb, 0x4c, 0x54, 0xac, 0xf9, 0x92, 0x37,
	0x11, 0x89, 0x18, 0x00, 0x15, 0x15, 0x0e, 0x19, 0x01, 0x32, 0x15, 0x02, 0x03, 0x0f, 0x07,
	0x0f, 0x0b, 0x0f, 0x05, 0x82, 0x4c, 0x6b, 0x49, 0x4e, 0x53, 0x54, 0x4c, 0x44, 0x5f, 0x4d,
	0x46, 0x53, 0x54, 0x50, 0x3f, 0x6a, 0x3a, 0x4d, 0xcd, 0xfa, 0x58, 0xc5, 0xac, 0xce, 0xf9,
	0xf5, 0x84, 0xc4, 0x11, 0x24, 0x69, 0x23, 0x72, 0x61, 0x64, 0x2e, 0x73, 0x75, 0x69, 0x74,
	0x59, 0x01, 0x32, 0xd8, 0x6b, 0xa2, 0x02, 0x58, 0x73, 0x82, 0x58, 0x24, 0x82, 0x2f, 0x58,
	0x20, 0x52, 0xda, 0xdd, 0xdf, 0xda, 0x18, 0xff, 0x08, 0x14, 0xaa, 0x45, 0x84, 0xc2, 0xe6,
	0x71, 0x3b, 0x5f, 0xd5, 0x8e, 0x23, 0xf3, 0x7c, 0xe8, 0xbb, 0x8a, 0xf6, 0x72, 0xee, 0x4a,
	0xe6, 0x3c, 0x89, 0x58, 0x4a, 0xd2, 0x84, 0x43, 0xa1, 0x01, 0x26, 0xa0, 0xf6, 0x58, 0x40,
	0x63, 0x76, 0x2e, 0xce, 0x52, 0x32, 0x1d, 0xc4, 0x20, 0x85, 0x8c, 0xe7, 0x59, 0x41, 0xdd,
	0x07, 0xca, 0x97, 0x75, 0x00, 0xa4, 0x42, 0x55, 0x8a, 0xcc, 0x16, 0x35, 0xfb, 0xcd, 0xd9,
	0x42, 0x25, 0xec, 0x44, 0x02, 0xb4, 0x75, 0x1c, 0x43, 0xbf, 0x5d, 0x61, 0xb8, 0xaf, 0x20,
	0x9a, 0x37, 0xe3, 0xd2, 0xc2, 0xb0, 0xab, 0x42, 0x00, 0x9b, 0x9f, 0x31, 0x28, 0x1a, 0x14,
	0x6c, 0x75, 0x57, 0xe9, 0x03, 0x58, 0xb6, 0xa7, 0x01, 0x01, 0x02, 0x02, 0x03, 0x58, 0x70,
	0xa2, 0x02, 0x81, 0x84, 0x44, 0x63, 0x4d, 0x45, 0x4d, 0x41, 0x03, 0x45, 0x1a, 0x0e, 0x05,
	0x40, 0x00, 0x45, 0x1a, 0x00, 0x05, 0x58, 0x00, 0x04, 0x58, 0x56, 0x86, 0x14, 0xa4, 0x01,
	0x50, 0x76, 0x17, 0xda, 0xa5, 0x71, 0xfd, 0x5a, 0x85, 0x8f, 0x94, 0xe2, 0x8d, 0x73, 0x5c,
	0xe9, 0xf4, 0x02, 0x50, 0x81, 0x6a, 0xa0, 0xa0, 0xaf, 0x11, 0x5e, 0xf2, 0x85, 0x8a, 0xfe,
	0xb6, 0x68, 0xb2, 0xe9, 0xc9, 0x03, 0x58, 0x24, 0x82, 0x2f, 0x58, 0x20, 0x5f, 0xc3, 0x54,
	0xbf, 0x8e, 0x8c, 0x50, 0xfb, 0x4f, 0xbc, 0x2c, 0xfa, 0xeb, 0x04, 0x53, 0x41, 0xc9, 0x80,
	0x6d, 0xea, 0xbd, 0xcb, 0x41, 0x54, 0xfb, 0x79, 0xcc, 0xa4, 0xf0, 0xc9, 0x8c, 0x12, 0x0e,
	0x19, 0x01, 0x00, 0x01, 0x0f, 0x02, 0x0f, 0x07, 0x43, 0x82, 0x03, 0x0f, 0x09, 0x43, 0x82,
	0x17, 0x02, 0x11, 0x52, 0x86, 0x14, 0xa1, 0x15, 0x69, 0x23, 0x66, 0x69, 0x6c, 0x65, 0x2e,
	0x62, 0x69, 0x6e, 0x15, 0x02, 0x03, 0x0f, 0x05, 0x82, 0x4c, 0x6b, 0x49, 0x4e, 0x53, 0x54,
	0x4c, 0x44, 0x5f, 0x4d, 0x46, 0x53, 0x54, 0x50, 0x81, 0x6a, 0xa0, 0xa0, 0xaf, 0x11, 0x5e,
	0xf2, 0x85, 0x8a, 0xfe, 0xb6, 0x68, 0xb2, 0xe9, 0xc9, 0x69, 0x23, 0x61, 0x70, 0x70, 0x2e,
	0x73, 0x75, 0x69, 0x74, 0x59, 0x01, 0x32, 0xd8, 0x6b, 0xa2, 0x02, 0x58, 0x73, 0x82, 0x58,
	0x24, 0x82, 0x2f, 0x58, 0x20, 0x45, 0xcc, 0xc3, 0x8f, 0xb7, 0xea, 0x5a, 0x86, 0xc9, 0xea,
	0xa1, 0x73, 0x44, 0x25, 0xdb, 0x4a, 0x43, 0x90, 0x66, 0x14, 0x88, 0x16, 0x55, 0x08, 0xea,
	0xb5, 0xea, 0x94, 0x35, 0xd8, 0x49, 0x32, 0x58, 0x4a, 0xd2, 0x84, 0x43, 0xa1, 0x01, 0x26,
	0xa0, 0xf6, 0x58, 0x40, 0x89, 0x6e, 0xd5, 0xde, 0xe9, 0x3d, 0x54, 0x51, 0xd9, 0x5e, 0x23,
	0xda, 0x3f, 0x2b, 0x32, 0x6a, 0x76, 0xd9, 0x8f, 0x2e, 0x70, 0x4c, 0xf1, 0xda, 0x34, 0x3c,
	0x8c, 0x7a, 0xcb, 0xef, 0x3d, 0x16, 0xa8, 0xc5, 0x86, 0xe9, 0x61, 0xeb, 0xbc, 0xb0, 0x7d,
	0x93, 0xa7, 0x92, 0xb8, 0xee, 0x44, 0xb1, 0xb7, 0x85, 0x82, 0x31, 0x77, 0x1f, 0xd1, 0x4b,
	0x77, 0xf8, 0x78, 0x5d, 0x3c, 0xe4, 0x36, 0x83, 0x03, 0x58, 0xb6, 0xa7, 0x01, 0x01, 0x02,
	0x02, 0x03, 0x58, 0x70, 0xa2, 0x02, 0x81, 0x84, 0x44, 0x63, 0x4d, 0x45, 0x4d, 0x41, 0x02,
	0x45, 0x1a, 0x0e, 0x0a, 0xa0, 0x00, 0x45, 0x1a, 0x00, 0x07, 0xf8, 0x00, 0x04, 0x58, 0x56,
	0x86, 0x14, 0xa4, 0x01, 0x50, 0x76, 0x17, 0xda, 0xa5, 0x71, 0xfd, 0x5a, 0x85, 0x8f, 0x94,
	0xe2, 0x8d, 0x73, 0x5c, 0xe9, 0xf4, 0x02, 0x50, 0x08, 0xc1, 0xb5, 0x99, 0x55, 0xe8, 0x5f,
	0xbc, 0x9e, 0x76, 0x7b, 0xc2, 0x9c, 0xe1, 0xb0, 0x4d, 0x03, 0x58, 0x24, 0x82, 0x2f, 0x58,
	0x20, 0x5f, 0xc3, 0x54, 0xbf, 0x8e, 0x8c, 0x50, 0xfb, 0x4f, 0xbc, 0x2c, 0xfa, 0xeb, 0x04,
	0x53, 0x41, 0xc9, 0x80, 0x6d, 0xea, 0xbd, 0xcb, 0x41, 0x54, 0xfb, 0x79, 0xcc, 0xa4, 0xf0,
	0xc9, 0x8c, 0x12, 0x0e, 0x19, 0x01, 0x00, 0x01, 0x0f, 0x02, 0x0f, 0x07, 0x43, 0x82, 0x03,
	0x0f, 0x09, 0x43, 0x82, 0x17, 0x02, 0x11, 0x52, 0x86, 0x14, 0xa1, 0x15, 0x69, 0x23, 0x66,
	0x69, 0x6c, 0x65, 0x2e, 0x62, 0x69, 0x6e, 0x15, 0x02, 0x03, 0x0f, 0x05, 0x82, 0x4c, 0x6b,
	0x49, 0x4e, 0x53, 0x54, 0x4c, 0x44, 0x5f, 0x4d, 0x46, 0x53, 0x54, 0x50, 0x08, 0xc1, 0xb5,
	0x99, 0x55, 0xe8, 0x5f, 0xbc, 0x9e, 0x76, 0x7b, 0xc2, 0x9c, 0xe1, 0xb0, 0x4d};

const size_t manifest_root_v2_len = sizeof(manifest_root_v2_buf);
