/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <ipc_streamer.h>
#include <fetch_source_mgr.h>

static uint32_t received_bytes = 0;
static uint32_t received_checksum = 0;
static uint32_t expected_bytes = 0;
static uint32_t expected_checksum = 0;

/* Intentionally missaligned buffer size
 */
static uint8_t test_buf[32 * 1024 - 3];
#define ITERATIONS 16

static const char *requested_resource_id = "ExampleImageName.img";
static void *stream_sink_requested_ctx = (void *)0xabcd0001;

static K_SEM_DEFINE(delay_simulator_sem_1, 0, 1);
static K_SEM_DEFINE(delay_simulator_sem_2, 0, 1);
static suit_plat_err_t ipc_stream_write_chunk(void *ctx, uint8_t *buf, size_t *size)
{
	received_bytes += *size;

	for (int i = 0; i < *size; ++i) {
		received_checksum += buf[i];
	}

	/* let's simulate 10 ms lasting operation.
	 *  cannot use k_sleep in posix tests!
	 */
	k_sem_take(&delay_simulator_sem_1, K_MSEC(10));
	return SUIT_PLAT_SUCCESS;
}

typedef struct {
	size_t chunk_size;
	uint32_t sleep_ms;

} access_pattern_t;

static suit_plat_err_t fetch_request_fn(const uint8_t *uri, size_t uri_length,
					struct stream_sink *sink)
{
	zassert_equal(uri_length, strlen(requested_resource_id), "uri_length (%d)", uri_length);
	zassert_mem_equal(uri, requested_resource_id, strlen(requested_resource_id));

	zassert_not_null(sink);
	zassert_not_null(sink->write);

	access_pattern_t ap_table[] = {
		{.chunk_size = 16000, .sleep_ms = 40}, {.chunk_size = 3, .sleep_ms = 10},
		{.chunk_size = 7000, .sleep_ms = 50},  {.chunk_size = 120, .sleep_ms = 10},
		{.chunk_size = 8192, .sleep_ms = 100}, {.chunk_size = 4096, .sleep_ms = 200},
		{.chunk_size = 4096, .sleep_ms = 10},  {.chunk_size = 4096, .sleep_ms = 10}};

	suit_plat_err_t rc = SUIT_PLAT_SUCCESS;
	int pattern_idx = 0;

	for (int buf_iter = 0; buf_iter < ITERATIONS; buf_iter++) {

		size_t offset_in_buffer = 0;

		while (offset_in_buffer < sizeof(test_buf)) {
			access_pattern_t *ap = &ap_table[pattern_idx];

			size_t to_be_copied = sizeof(test_buf) - offset_in_buffer;
			if (ap->chunk_size < to_be_copied) {
				to_be_copied = ap->chunk_size;
			}

			rc = sink->write(sink->ctx, test_buf + offset_in_buffer, &to_be_copied);
			if (rc != SUIT_PLAT_SUCCESS) {
				return rc;
			}

			offset_in_buffer += to_be_copied;

			/* cannot use k_sleep in posix tests!
			 */
			k_sem_take(&delay_simulator_sem_2, K_MSEC(ap->sleep_ms));

			pattern_idx++;
			pattern_idx = pattern_idx % (sizeof(ap_table) / sizeof(access_pattern_t));
		}
	}

	return SUIT_PLAT_SUCCESS;
}

void test_full_stack(void)
{

	for (int i = 0; i < sizeof(test_buf); ++i) {
		test_buf[i] = (uint8_t)i;
	}

	for (int buf_iter = 0; buf_iter < ITERATIONS; buf_iter++) {

		for (int i = 0; i < sizeof(test_buf); ++i) {
			expected_bytes++;
			expected_checksum += test_buf[i];
		}
	}

	suit_ipc_streamer_chunk_status_evt_unsubscribe();
	suit_ipc_streamer_missing_image_evt_unsubscribe();

	suit_plat_err_t rc = SUIT_PLAT_SUCCESS;

	rc = suit_ipc_streamer_provider_init();
	zassert_equal(rc, SUIT_PLAT_SUCCESS, "suit_ipc_streamer_provider_init returned (%d)", rc);

	rc = suit_fetch_source_register(fetch_request_fn);
	zassert_equal(rc, SUIT_PLAT_SUCCESS, "fetch_source_register returned (%d)", rc);

	struct stream_sink test_sink = {
		.write = ipc_stream_write_chunk,
		.seek = NULL,
		.flush = NULL,
		.used_storage = NULL,
		.release = NULL,
		.ctx = stream_sink_requested_ctx,
	};

	uint32_t inter_chunk_timeout_ms = 10000;
	uint32_t requesting_period_ms = 1000;
	rc = suit_ipc_streamer_stream(requested_resource_id, strlen(requested_resource_id),
				      &test_sink, inter_chunk_timeout_ms, requesting_period_ms);

	zassert_equal(expected_bytes, received_bytes, "%d vs %d", expected_bytes, received_bytes);
	zassert_equal(expected_checksum, received_checksum, "%d vs %d", expected_checksum,
		      received_checksum);
	zassert_equal(rc, SUIT_PLAT_SUCCESS, "suit_ipc_streamer_stream returned (%d)", rc);
}
