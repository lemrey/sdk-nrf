/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <stddef.h>

/** @brief SUIT envelope generated using the manifest/manifest_app_v2.yaml input file.
 *
 * @details This envelope contains 256-byte random numbers attached as an integrated
 *          payload and fetched directly into the memory-mapped region during install step.
 *          The envelope has been signed using the manifest/key_private.pem key.
 *
 * @note Please use the manifest_common/regenerate.sh script for regenerating payloads.
 */
uint8_t manifest_app_v2_buf[] = {
	0xD8, 0x6B, 0xA2, 0x02, 0x58, 0x7A, 0x82, 0x58, 0x24, 0x82, 0x2F, 0x58, 0x20, 0x45, 0xCC,
	0xC3, 0x8F, 0xB7, 0xEA, 0x5A, 0x86, 0xC9, 0xEA, 0xA1, 0x73, 0x44, 0x25, 0xDB, 0x4A, 0x43,
	0x90, 0x66, 0x14, 0x88, 0x16, 0x55, 0x08, 0xEA, 0xB5, 0xEA, 0x94, 0x35, 0xD8, 0x49, 0x32,
	0x58, 0x51, 0xD2, 0x84, 0x4A, 0xA2, 0x01, 0x26, 0x04, 0x45, 0x1A, 0x7F, 0xFF, 0xFF, 0xE0,
	0xA0, 0xF6, 0x58, 0x40, 0x7B, 0x03, 0x3F, 0x04, 0xB8, 0xA5, 0xA8, 0xAE, 0x1F, 0x79, 0xC2,
	0xA5, 0xB6, 0x1D, 0x26, 0xD0, 0xF3, 0x5D, 0xBB, 0x93, 0x8B, 0x5C, 0xDF, 0xE8, 0xCD, 0xAE,
	0x61, 0x4C, 0x89, 0x46, 0xC9, 0x01, 0xCB, 0x63, 0x90, 0x7B, 0xD6, 0x37, 0xFB, 0x1E, 0x35,
	0xA7, 0xF6, 0x39, 0xC6, 0xE3, 0x79, 0x70, 0x49, 0x0C, 0x8D, 0x1C, 0x40, 0x58, 0xAD, 0x9A,
	0x61, 0x7E, 0x38, 0xE6, 0xC0, 0xB7, 0x64, 0x8C, 0x03, 0x58, 0xB6, 0xA7, 0x01, 0x01, 0x02,
	0x02, 0x03, 0x58, 0x70, 0xA2, 0x02, 0x81, 0x84, 0x44, 0x63, 0x4D, 0x45, 0x4D, 0x41, 0x02,
	0x45, 0x1A, 0x0E, 0x0A, 0xA0, 0x00, 0x45, 0x1A, 0x00, 0x07, 0xF8, 0x00, 0x04, 0x58, 0x56,
	0x86, 0x14, 0xA4, 0x01, 0x50, 0x76, 0x17, 0xDA, 0xA5, 0x71, 0xFD, 0x5A, 0x85, 0x8F, 0x94,
	0xE2, 0x8D, 0x73, 0x5C, 0xE9, 0xF4, 0x02, 0x50, 0x08, 0xC1, 0xB5, 0x99, 0x55, 0xE8, 0x5F,
	0xBC, 0x9E, 0x76, 0x7B, 0xC2, 0x9C, 0xE1, 0xB0, 0x4D, 0x03, 0x58, 0x24, 0x82, 0x2F, 0x58,
	0x20, 0x5F, 0xC3, 0x54, 0xBF, 0x8E, 0x8C, 0x50, 0xFB, 0x4F, 0xBC, 0x2C, 0xFA, 0xEB, 0x04,
	0x53, 0x41, 0xC9, 0x80, 0x6D, 0xEA, 0xBD, 0xCB, 0x41, 0x54, 0xFB, 0x79, 0xCC, 0xA4, 0xF0,
	0xC9, 0x8C, 0x12, 0x0E, 0x19, 0x01, 0x00, 0x01, 0x0F, 0x02, 0x0F, 0x07, 0x43, 0x82, 0x03,
	0x0F, 0x09, 0x43, 0x82, 0x17, 0x02, 0x11, 0x52, 0x86, 0x14, 0xA1, 0x15, 0x69, 0x23, 0x66,
	0x69, 0x6C, 0x65, 0x2E, 0x62, 0x69, 0x6E, 0x15, 0x02, 0x03, 0x0F, 0x05, 0x82, 0x4C, 0x6B,
	0x49, 0x4E, 0x53, 0x54, 0x4C, 0x44, 0x5F, 0x4D, 0x46, 0x53, 0x54, 0x50, 0x08, 0xC1, 0xB5,
	0x99, 0x55, 0xE8, 0x5F, 0xBC, 0x9E, 0x76, 0x7B, 0xC2, 0x9C, 0xE1, 0xB0, 0x4D};

const size_t manifest_app_v2_len = sizeof(manifest_app_v2_buf);
