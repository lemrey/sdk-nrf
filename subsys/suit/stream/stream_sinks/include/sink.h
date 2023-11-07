/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SINK_H__
#define SINK_H__

#include <stdio.h>
#include <stdint.h>
#include <suit_plat_err.h>

#define SUCCESS			       0
#define INVALID_ARGUMENT	       -1
#define SUIT_MAX_COMPONENTS_REACHED    -2
#define FLASH_NOT_READY		       -3
#define WRITE_1_FAIL		       -4
#define WRITE_2_FAIL		       -5
#define WRITE_3_FAIL		       -6
#define READ_1_FAIL		       -7
#define READ_2_FAIL		       -8
#define WRITE_OUT_OF_BOUNDS	       -9
#define STORAGE_GET_FAILED	       -10
#define SOURCE_NOT_FOUND	       -11
#define WRITE_DISABLED		       -12
#define MRAM_ERASE_FAILED	       -13
#define UNSUPPORTED_SDFW_UPDATE_STATUS -14
#define SDFW_UPDATE_FAILURE	       -15
#define MULTIPLE_WRITE_CALLS	       -16
#define NOT_FOUND		       -17
#define CANDIDATE_INCONSISTENT	       -18
#define DIGEST_CALCULATION_FAILURE     -19
#define DIGEST_ABORTION_FAILURE	       -20
#define DIGEST_MISMATCH		       -21
#define NOT_INITIALIZED		       -22

typedef int (*erase_ptr)(void *ctx);
typedef int (*write_ptr)(void *ctx, uint8_t *buf, size_t *size);
typedef int (*seek_ptr)(void *ctx, size_t offset);
typedef int (*flush_ptr)(void *ctx);
typedef int (*used_storage_ptr)(void *ctx, size_t *size);
typedef int (*release_ptr)(void *ctx);


/**
 * @brief Structure represents node that is a target for data.
 *    The idea is to be able to create chain of such links/filters to process and write data.
 *    Structure can represent for example:
 *    - decrypting or decompressing entity and being an intermediate block.
 *    - a writer, presumably a final block.
 */
struct stream_sink {
	erase_ptr erase;
	write_ptr write;
	seek_ptr seek;
	flush_ptr flush;
	used_storage_ptr used_storage;
	release_ptr release;

	void *ctx; /* context used by specific sink implementation */
};

#endif /* SINK_H__ */
