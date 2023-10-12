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
	0xD8, 0x6B, 0xA4, 0x02, 0x58, 0x7A, 0x82, 0x58, 0x24, 0x82, 0x2F, 0x58, 0x20, 0x98, 0x80,
	0x48, 0xC4, 0xB6, 0x61, 0xB3, 0xC4, 0xE5, 0x02, 0xF9, 0x76, 0x7B, 0xBD, 0xB9, 0xBB, 0xC4,
	0x22, 0xEF, 0x22, 0x06, 0x5D, 0x38, 0xE7, 0x10, 0xA0, 0x1B, 0xA3, 0x17, 0x85, 0x9F, 0x74,
	0x58, 0x51, 0xD2, 0x84, 0x4A, 0xA2, 0x01, 0x26, 0x04, 0x45, 0x1A, 0x7F, 0xFF, 0xFF, 0xE0,
	0xA0, 0xF6, 0x58, 0x40, 0x0D, 0x68, 0x77, 0x63, 0xE7, 0xBD, 0x6C, 0xE9, 0x68, 0x54, 0x55,
	0x8F, 0x07, 0x03, 0x9F, 0x24, 0x70, 0x92, 0x86, 0x6A, 0x3B, 0x44, 0x82, 0x5B, 0x01, 0xFA,
	0x81, 0xBD, 0xFB, 0xE2, 0xCA, 0xC2, 0x0C, 0xAC, 0x6E, 0xA3, 0xC4, 0x8C, 0xB0, 0x36, 0x94,
	0x3C, 0xD6, 0xC8, 0xBA, 0xA9, 0x31, 0x6D, 0x77, 0xF0, 0x1B, 0x88, 0x7E, 0x77, 0xF7, 0x71,
	0x5C, 0x4E, 0xCD, 0xAE, 0xD9, 0x35, 0x4A, 0xC0, 0x03, 0x58, 0xDE, 0xA7, 0x01, 0x01, 0x02,
	0x01, 0x03, 0x58, 0x69, 0xA2, 0x02, 0x81, 0x82, 0x49, 0x68, 0x43, 0x41, 0x4E, 0x44, 0x5F,
	0x49, 0x4D, 0x47, 0x41, 0x01, 0x04, 0x58, 0x56, 0x86, 0x14, 0xA4, 0x01, 0x50, 0x76, 0x17,
	0xDA, 0xA5, 0x71, 0xFD, 0x5A, 0x85, 0x8F, 0x94, 0xE2, 0x8D, 0x73, 0x5C, 0xE9, 0xF4, 0x02,
	0x50, 0x56, 0xDC, 0x9A, 0x14, 0x28, 0xD8, 0x52, 0xD3, 0xBD, 0x62, 0xE7, 0x7A, 0x08, 0xBC,
	0x8B, 0x91, 0x03, 0x58, 0x24, 0x82, 0x2F, 0x58, 0x20, 0x5F, 0xC3, 0x54, 0xBF, 0x8E, 0x8C,
	0x50, 0xFB, 0x4F, 0xBC, 0x2C, 0xFA, 0xEB, 0x04, 0x53, 0x41, 0xC9, 0x80, 0x6D, 0xEA, 0xBD,
	0xCB, 0x41, 0x54, 0xFB, 0x79, 0xCC, 0xA4, 0xF0, 0xC9, 0x8C, 0x12, 0x0E, 0x19, 0x01, 0x00,
	0x01, 0x0F, 0x02, 0x0F, 0x07, 0x52, 0x86, 0x14, 0xA1, 0x15, 0x69, 0x23, 0x66, 0x69, 0x6C,
	0x65, 0x2E, 0x62, 0x69, 0x6E, 0x15, 0x02, 0x03, 0x0F, 0x11, 0x52, 0x86, 0x14, 0xA1, 0x15,
	0x69, 0x23, 0x66, 0x69, 0x6C, 0x65, 0x2E, 0x62, 0x69, 0x6E, 0x15, 0x02, 0x03, 0x0F, 0x17,
	0x82, 0x2F, 0x58, 0x20, 0xE7, 0x2B, 0x21, 0xED, 0x99, 0x77, 0x6D, 0xB5, 0x03, 0xCF, 0xEA,
	0xD2, 0x5F, 0x30, 0x35, 0x4F, 0x65, 0x1F, 0x0C, 0x2B, 0x74, 0xE6, 0xDF, 0x0C, 0xFE, 0xF2,
	0xC7, 0xEF, 0x72, 0x3F, 0xC3, 0x81, 0x05, 0x82, 0x4C, 0x6B, 0x49, 0x4E, 0x53, 0x54, 0x4C,
	0x44, 0x5F, 0x4D, 0x46, 0x53, 0x54, 0x50, 0x56, 0xDC, 0x9A, 0x14, 0x28, 0xD8, 0x52, 0xD3,
	0xBD, 0x62, 0xE7, 0x7A, 0x08, 0xBC, 0x8B, 0x91, 0x17, 0x58, 0x7F, 0xA1, 0x82, 0x49, 0x68,
	0x43, 0x41, 0x4E, 0x44, 0x5F, 0x49, 0x4D, 0x47, 0x41, 0x01, 0xA6, 0x01, 0x78, 0x18, 0x4E,
	0x6F, 0x72, 0x64, 0x69, 0x63, 0x20, 0x53, 0x65, 0x6D, 0x69, 0x63, 0x6F, 0x6E, 0x64, 0x75,
	0x63, 0x74, 0x6F, 0x72, 0x20, 0x41, 0x53, 0x41, 0x02, 0x65, 0x70, 0x6F, 0x73, 0x69, 0x78,
	0x03, 0x6E, 0x6E, 0x6F, 0x72, 0x64, 0x69, 0x63, 0x73, 0x65, 0x6D, 0x69, 0x2E, 0x63, 0x6F,
	0x6D, 0x04, 0x75, 0x54, 0x68, 0x65, 0x20, 0x50, 0x4F, 0x53, 0x49, 0x58, 0x20, 0x61, 0x70,
	0x70, 0x6C, 0x69, 0x63, 0x61, 0x74, 0x69, 0x6F, 0x6E, 0x05, 0x78, 0x1C, 0x53, 0x61, 0x6D,
	0x70, 0x6C, 0x65, 0x20, 0x61, 0x70, 0x70, 0x6C, 0x69, 0x63, 0x61, 0x74, 0x69, 0x6F, 0x6E,
	0x20, 0x66, 0x6F, 0x72, 0x20, 0x50, 0x4F, 0x53, 0x49, 0x58, 0x06, 0x66, 0x76, 0x31, 0x2E,
	0x30, 0x2E, 0x30, 0x69, 0x23, 0x66, 0x69, 0x6C, 0x65, 0x2E, 0x62, 0x69, 0x6E, 0x59, 0x01,
	0x00, 0xC7, 0x9C, 0xAB, 0x9D, 0xE8, 0x33, 0x7F, 0x30, 0x14, 0xEB, 0xAC, 0x02, 0xAF, 0x26,
	0x01, 0x5E, 0x80, 0x6D, 0x88, 0xA1, 0xDB, 0x11, 0xA7, 0x31, 0xDF, 0xA6, 0xEC, 0xCB, 0x9B,
	0x48, 0x0D, 0xC8, 0x34, 0x40, 0x6D, 0x30, 0x86, 0x7D, 0xE8, 0x1B, 0xEC, 0x3C, 0xF5, 0x40,
	0xD0, 0x48, 0x18, 0x82, 0x11, 0x9D, 0x7C, 0x3F, 0x6C, 0xE5, 0x8F, 0xF1, 0xD3, 0x5D, 0xE1,
	0x51, 0xF7, 0x6A, 0x0F, 0xAF, 0x0B, 0xBD, 0x4C, 0x5F, 0xA5, 0x34, 0x1A, 0x66, 0xDB, 0x22,
	0xEC, 0x63, 0xED, 0x4B, 0xAB, 0xC7, 0xC8, 0xF7, 0x59, 0xD8, 0xD6, 0x9E, 0xEC, 0x71, 0x1B,
	0x24, 0x20, 0xB9, 0xAE, 0xE1, 0x3B, 0xFC, 0xAE, 0xB8, 0x77, 0xAC, 0xA4, 0x57, 0x34, 0x97,
	0x84, 0x4F, 0x58, 0xD5, 0x68, 0x08, 0x6F, 0xE3, 0x9C, 0x7E, 0x1B, 0xD7, 0x38, 0x22, 0x98,
	0x48, 0xF8, 0x7A, 0x67, 0xB2, 0xD9, 0xAC, 0xC5, 0x34, 0xC1, 0x27, 0x82, 0x8E, 0x42, 0x79,
	0x84, 0x21, 0x37, 0x4C, 0x41, 0x4A, 0x0F, 0xE2, 0x7F, 0xA0, 0x6A, 0x19, 0x13, 0x3D, 0x52,
	0x22, 0x7F, 0xD6, 0x2F, 0x71, 0x12, 0x76, 0xAB, 0x25, 0x9C, 0xFC, 0x67, 0x08, 0x03, 0x7C,
	0xDB, 0x18, 0xE6, 0x45, 0xF8, 0x99, 0xC2, 0x9E, 0x2C, 0xE3, 0x9B, 0x25, 0xA9, 0x7B, 0x09,
	0xFF, 0x00, 0x57, 0x26, 0x08, 0x0A, 0x11, 0x42, 0xCF, 0x82, 0xA2, 0x6B, 0x2A, 0x99, 0xF9,
	0x71, 0x9D, 0x14, 0x19, 0x5C, 0x5C, 0x78, 0x31, 0x60, 0x42, 0x4A, 0x18, 0x1F, 0xEC, 0x78,
	0x6A, 0x9A, 0x7C, 0x4F, 0xCF, 0xE8, 0x5A, 0x29, 0x65, 0xCD, 0x01, 0x3B, 0x6D, 0x53, 0xBB,
	0xC6, 0xDB, 0xDA, 0xD5, 0x8F, 0xF7, 0xF4, 0xD9, 0xB9, 0x0A, 0x03, 0x4B, 0xFF, 0x33, 0xAB,
	0x3B, 0xC5, 0xAF, 0xD0, 0xB8, 0x2C, 0x0F, 0x6A, 0xA9, 0x11, 0xB0, 0xE8, 0x57, 0x8C, 0x92,
	0x53, 0x81};

const size_t manifest_len = sizeof(manifest_buf);
#endif /* CONFIG_SOC_NRF52840 || CONFIG_BOARD_NATIVE_POSIX */
