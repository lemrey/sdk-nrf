/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>
#include <string.h>
#include <ipc_streamer.h>
#include <fetch_source_mgr.h>

#define IMG_REQUEST_THREAD_STACK_SIZE 2048
#define IMG_REQUEST_THREAD_PRIORITY   5

static K_THREAD_STACK_DEFINE(img_request_thread_stack_area, IMG_REQUEST_THREAD_STACK_SIZE);
static struct k_work_q img_request_work_q;

typedef enum {
	FREE = 0,
	READY_TO_ENQUEUE = 1,
	ENQUEUED = 2
} buffer_state_t;

typedef struct {
	buffer_state_t buffer_state;
	uint32_t offset_in_image;
	uint32_t chunk_size;
	uint32_t chunk_id;
} buffer_metadata_t;

typedef struct {
	buffer_metadata_t buffer_metadata[CONFIG_SUIT_STREAM_IPC_PROVIDER_BUFFERS];
	uint8_t buffer[CONFIG_SUIT_STREAM_IPC_PROVIDER_BUFFERS]
		      [CONFIG_SUIT_STREAM_IPC_PROVIDER_BUFFER_SIZE];
} buffer_info_t;

typedef struct {
	struct k_work work;
	uint32_t stream_session_id;
	size_t current_image_offset;
	size_t requested_image_offset;
	uint32_t last_chunk_id;

	buffer_info_t buffer_info;
} image_request_info_t;

static image_request_info_t request_info = {
	.stream_session_id = 0,
};

static K_SEM_DEFINE(chunk_status_changed_sem, 0, 1);

static int wait_for_buffer_state_change(image_request_info_t *ri)
{
	int err = 0;
	ipc_streamer_chunk_info_t injected_chunks[CONFIG_SUIT_STREAM_IPC_PROVIDER_BUFFERS];
	size_t chunk_info_count = CONFIG_SUIT_STREAM_IPC_PROVIDER_BUFFERS;

	while (true) {
		err = ipc_streamer_chunk_status_req(ri->stream_session_id, injected_chunks,
						    &chunk_info_count);
		if (-IPC_STREAMER_ENOSPACE == err) {
			/* not enough space in injected_chunks. This is not a problem,
			 *  stream requestor most probably just released its space and
			 *  one of next calls will be successful.
			 */
		} else if (err < 0) {
			return err;
		} else {

			bool chunk_released = false;
			for (int i = 0; i < CONFIG_SUIT_STREAM_IPC_PROVIDER_BUFFERS; i++) {
				buffer_metadata_t *bm = &ri->buffer_info.buffer_metadata[i];

				if (ENQUEUED == bm->buffer_state) {
					bool still_enqueued = false;
					for (int j = 0; j < chunk_info_count; j++) {
						ipc_streamer_chunk_info_t *ci = &injected_chunks[j];
						if (ci->chunk_id == bm->chunk_id &&
						    PENDING == ci->status) {
							still_enqueued = true;
							break;
						}
					}

					if (!still_enqueued) {
						bm->buffer_state = FREE;
						chunk_released = true;
					}
				}
			}

			if (chunk_released) {
				return 0;
			}
		}

		/* timeout is a guard against chunk status notification miss.
		 *  20 ms limits amount of IPC calls and at the same time it does not
		 *  affect operation duration significantly
		 */
		k_sem_take(&chunk_status_changed_sem, K_MSEC(20));
	}

	return err;
}

static int find_buffer_for_enqueue(image_request_info_t *ri, buffer_metadata_t **bm,
				   uint8_t **buffer)
{
	int err = 0;
	*bm = NULL;
	*buffer = NULL;

	for (int i = 0; i < CONFIG_SUIT_STREAM_IPC_PROVIDER_BUFFERS; i++) {
		buffer_metadata_t *current = &ri->buffer_info.buffer_metadata[i];
		if (READY_TO_ENQUEUE == current->buffer_state) {
			/* READY_TO_ENQUEUE- means that new data shall be concatenated to this one
			 */
			*bm = current;
			*buffer = ri->buffer_info.buffer[i];
			return 0;
		}
	}

	while (NULL == *bm) {
		for (int i = 0; i < CONFIG_SUIT_STREAM_IPC_PROVIDER_BUFFERS; i++) {
			buffer_metadata_t *current = &ri->buffer_info.buffer_metadata[i];
			if (FREE == current->buffer_state) {
				*bm = current;
				*buffer = ri->buffer_info.buffer[i];
				break;
			}
		}

		if (NULL == *bm) {
			err = wait_for_buffer_state_change(ri);
			if (err < 0) {
				return err;
			}
		}
	}

	return err;
}

static int chunk_enqueue(image_request_info_t *ri, buffer_metadata_t *bm, uint8_t *buffer_address,
			 bool last_chunk)
{
	int err = 0;

	while (true) {
		err = ipc_streamer_chunk_enqueue(ri->stream_session_id, bm->chunk_id,
						 bm->offset_in_image, buffer_address,
						 bm->chunk_size, last_chunk);
		if (-IPC_STREAMER_ENOSPACE == err) {
			/* Not enough space in requestor, try again later
			 */
			err = wait_for_buffer_state_change(ri);
			if (err < 0) {
				return err;
			}
		} else {
			return err;
		}
	}
}

static int end_of_stream(image_request_info_t *ri)
{
	int err = 0;

	buffer_metadata_t *bm = NULL;
	uint8_t *buffer = NULL;
	err = find_buffer_for_enqueue(ri, &bm, &buffer);
	if (err) {
		return err;
	}

	if (FREE == bm->buffer_state) {
		bm->offset_in_image = ri->requested_image_offset;
		bm->chunk_size = 0;
		bm->chunk_id = ++ri->last_chunk_id;
		bm->buffer_state = READY_TO_ENQUEUE;
	}

	if (READY_TO_ENQUEUE != bm->buffer_state) {
		/* This shall not happen. There must be a flaw in design. concurrency issue?
		 */
		return -IPC_STREAMER_ECUNCURRENCY;
	}

	err = chunk_enqueue(ri, bm, buffer, true);
	if (err) {
		return err;
	}
	ri->requested_image_offset = bm->offset_in_image + bm->chunk_size;
	bm->buffer_state = ENQUEUED;

	/* Let's wait till all buffers are given back or there is a
	 * 'unrecognized stream_session_id' error
	 */
	while (true) {
		bm = NULL;
		for (int i = 0; i < CONFIG_SUIT_STREAM_IPC_PROVIDER_BUFFERS; i++) {
			buffer_metadata_t *current = &ri->buffer_info.buffer_metadata[i];
			if (ENQUEUED == current->buffer_state) {
				bm = current;
				break;
			}
		}

		if (NULL == bm) {
			return 0;
		}

		err = wait_for_buffer_state_change(ri);
		if (err < 0) {
			return 0;
		}
	}

	return 0;
}

static int write_proxy(void *ctx, uint8_t *source_bufer, size_t *size)
{

	int err = 0;
	uint32_t stream_session_id = (uintptr_t)ctx;
	image_request_info_t *ri = &request_info;

	size_t source_offset = 0;
	size_t source_remaining = *size;

	while (source_remaining) {

		if (ri->stream_session_id != stream_session_id) {
			return -1;
		}

		buffer_metadata_t *bm = NULL;
		uint8_t *buffer = NULL;
		err = find_buffer_for_enqueue(ri, &bm, &buffer);
		if (err) {
			return err;
		}

		if (FREE == bm->buffer_state) {
			bm->offset_in_image = ri->requested_image_offset;
			bm->chunk_size = 0;
			bm->chunk_id = ++ri->last_chunk_id;
			bm->buffer_state = READY_TO_ENQUEUE;
		}

		if (READY_TO_ENQUEUE != bm->buffer_state) {
			/* This shall not happen. There must be a flaw in design. concurrency issue?
			 */
			return -IPC_STREAMER_ECUNCURRENCY;
		}

		size_t to_be_copied = CONFIG_SUIT_STREAM_IPC_PROVIDER_BUFFER_SIZE - bm->chunk_size;
		if (source_remaining < to_be_copied) {
			to_be_copied = source_remaining;
		}

		memcpy(buffer + bm->chunk_size, source_bufer + source_offset, to_be_copied);
		source_offset += to_be_copied;
		source_remaining -= to_be_copied;
		bm->chunk_size += to_be_copied;

		if (CONFIG_SUIT_STREAM_IPC_PROVIDER_BUFFER_SIZE == bm->chunk_size) {
			/* buffer full, it is time to enqueue
			 */
			err = chunk_enqueue(ri, bm, buffer, false);
			if (err) {
				return err;
			}
			ri->requested_image_offset = bm->offset_in_image + bm->chunk_size;
			bm->buffer_state = ENQUEUED;
		}
	}

	return 0;
}

static int seek_proxy(void *ctx, size_t offset)
{

	int err = 0;
	uint32_t stream_session_id = (uintptr_t)ctx;
	image_request_info_t *ri = &request_info;

	if (ri->stream_session_id != stream_session_id) {
		return -1;
	}

	buffer_metadata_t *bm = NULL;
	uint8_t *buffer = NULL;
	err = find_buffer_for_enqueue(ri, &bm, &buffer);
	if (err) {
		return err;
	}

	if (READY_TO_ENQUEUE == bm->buffer_state) {

		if (bm->offset_in_image + bm->chunk_size == offset) {
			/* this operation moves write pointer exactly to location next to
			 *  last byte of this chunk, so there is no need to do anything
			 */
			return 0;

		} else {
			err = chunk_enqueue(ri, bm, buffer, false);
			if (err) {
				return err;
			}
			ri->requested_image_offset = bm->offset_in_image + bm->chunk_size;
			bm->buffer_state = ENQUEUED;
		}
	}

	if (ri->requested_image_offset != offset) {
		err = find_buffer_for_enqueue(ri, &bm, &buffer);
		if (err) {
			return err;
		}

		if (FREE != bm->buffer_state) {
			/* This shall not happen. There must be a flaw in design. concurrency issue?
			 *  as max one buffer can be READY_TO_ENQUEUE
			 */
			return -IPC_STREAMER_ECUNCURRENCY;
		}

		bm->offset_in_image = offset;
		bm->chunk_size = 0;
		bm->chunk_id = ++ri->last_chunk_id;
		bm->buffer_state = READY_TO_ENQUEUE;
	}

	return 0;
}

static void image_request_worker(struct k_work *item)
{
	image_request_info_t *ri = CONTAINER_OF(item, image_request_info_t, work);

	struct stream_sink sink = {.write = write_proxy,
				   .seek = seek_proxy,
				   .flush = NULL,
				   .used_storage = NULL,
				   .release = NULL,
				   .ctx = (void *)(uintptr_t)ri->stream_session_id};

	uint8_t *resource_id = ri->buffer_info.buffer[0];
	int err = fetch_source_stream(resource_id, strlen(resource_id), &sink);

	if (0 == err) {
		err = end_of_stream(ri);
	}

	ri->stream_session_id = 0;
}

static int missing_image_notify_fct(const uint8_t *resource_id, size_t resource_id_length,
				    uint32_t stream_session_id, void *context)
{
	image_request_info_t *ri = &request_info;

	if (0 == ri->stream_session_id) {

		ri->current_image_offset = 0;
		ri->requested_image_offset = 0;
		ri->last_chunk_id = 0;
		memset(&ri->buffer_info, 0, sizeof(buffer_info_t));

		/* Buffer 0 is utilized to pass the URI
		 */
		strncpy(ri->buffer_info.buffer[0], resource_id,
			(resource_id_length < CONFIG_SUIT_STREAM_IPC_PROVIDER_BUFFER_SIZE
				 ? resource_id_length
				 : CONFIG_SUIT_STREAM_IPC_PROVIDER_BUFFER_SIZE - 1));

		ri->stream_session_id = stream_session_id;
		k_work_submit_to_queue(&img_request_work_q, &ri->work);
		return 0;
	}

	return -1;
}

static int chunk_status_notify_fct(uint32_t stream_session_id, void *context)
{
	k_sem_give(&chunk_status_changed_sem);
	return 0;
}

int ipc_streamer_provider_init()
{

	k_work_queue_init(&img_request_work_q);
	k_work_queue_start(&img_request_work_q, img_request_thread_stack_area,
			   K_THREAD_STACK_SIZEOF(img_request_thread_stack_area),
			   IMG_REQUEST_THREAD_PRIORITY, NULL);

	image_request_info_t *ri = &request_info;
	k_work_init(&ri->work, image_request_worker);

	int err = ipc_streamer_requestor_init();
	if (err != 0) {
		return err;
	}

	err = ipc_streamer_chunk_status_evt_subscribe(chunk_status_notify_fct, (void *)NULL);
	if (err != 0) {
		return err;
	}

	err = ipc_streamer_missing_image_evt_subscribe(missing_image_notify_fct, (void *)NULL);
	if (err != 0) {
		return err;
	}

	return 0;
}
