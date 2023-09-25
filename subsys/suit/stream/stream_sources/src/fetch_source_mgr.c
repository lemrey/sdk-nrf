/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>
#include "fetch_source_mgr.h"

typedef enum {
	STAGE_IDLE,
	STAGE_PENDING_FIRST_RESPONSE,
	STAGE_IN_PROGRESS
} session_stage_t;

typedef struct {
	fetch_source_mgr_fetch_request_fct request_fct;

} fetch_source_t;

typedef struct {
	session_stage_t stage;
	struct stream_sink client_sink;
	uint32_t session_id;

} stream_session_t;

static stream_session_t stream_session = {.stage = STAGE_IDLE};

static fetch_source_t sources[CONFIG_SUIT_STREAM_FETCH_MAX_SOURCES] = {0};

static uint32_t last_used_session_id = 0;

static K_MUTEX_DEFINE(component_state_mutex);

static inline void component_lock()
{
	k_mutex_lock(&component_state_mutex, K_FOREVER);
}

static inline void component_unlock()
{
	k_mutex_unlock(&component_state_mutex);
}

static inline stream_session_t *open_session(const uint8_t *uri, size_t uri_length,
					     struct stream_sink *sink)
{
	component_lock();
	stream_session_t *session = &stream_session;
	if (STAGE_IDLE != session->stage) {
		component_unlock();
		return NULL;
	}

	session->stage = STAGE_PENDING_FIRST_RESPONSE;
	session->client_sink = *sink;
	session->session_id = 0;

	component_unlock();
	return session;
}

static inline void close_session(stream_session_t *session)
{
	component_lock();
	session->stage = STAGE_IDLE;
	session->session_id = 0;
	component_unlock();
}

static stream_session_t *find_session(uint32_t session_id)
{
	if (0 == session_id) {
		return NULL;
	}

	component_lock();
	stream_session_t *session = &stream_session;

	if (STAGE_IDLE == session->stage || session_id != session->session_id) {
		component_unlock();
		return NULL;
	}

	component_unlock();
	return session;
}

static int write_proxy(void *ctx, uint8_t *buf, size_t *size)
{
	uint32_t session_id = (uintptr_t)ctx;

	component_lock();
	stream_session_t *session = find_session(session_id);

	if (NULL == session) {
		component_unlock();
		return -1;
	}

	if (STAGE_PENDING_FIRST_RESPONSE == session->stage) {
		session->stage = STAGE_IN_PROGRESS;
	}

	int (*client_write_fct)(void *ctx, uint8_t *buf, size_t *size) = session->client_sink.write;
	void *clinet_ctx = session->client_sink.ctx;

	int err = client_write_fct(clinet_ctx, buf, size);

	component_unlock();
	return err;
}

static int seek_proxy(void *ctx, size_t offset)
{
	uint32_t session_id = (uintptr_t)ctx;

	component_lock();
	stream_session_t *session = find_session(session_id);

	if (NULL == session) {
		component_unlock();
		return -1;
	}

	if (STAGE_PENDING_FIRST_RESPONSE == session->stage) {
		session->stage = STAGE_IN_PROGRESS;
	}

	int (*client_seek_fct)(void *ctx, size_t offset) = session->client_sink.seek;
	void *clinet_ctx = session->client_sink.ctx;

	int err = client_seek_fct(clinet_ctx, offset);

	component_unlock();
	return err;
}

int fetch_source_register(fetch_source_mgr_fetch_request_fct request_fct)
{
	component_lock();

	for (int i = 0; i < sizeof(sources) / sizeof(fetch_source_t); i++) {
		fetch_source_t *source = &sources[i];
		if (NULL == source->request_fct) {
			source->request_fct = request_fct;
			component_unlock();
			return 0;
		}
	}

	component_unlock();
	return -1;
}

int fetch_source_stream(const uint8_t *uri, size_t uri_length, struct stream_sink *sink)
{

	if (NULL == uri || 0 == uri_length || NULL == sink || NULL == sink->write) {
		return -1;
	}

	stream_session_t *session = open_session(uri, uri_length, sink);
	if (NULL == session) {
		return -1;
	}

	struct stream_sink session_sink = {.write = write_proxy,
					   .seek = NULL,
					   .flush = NULL,
					   .used_storage = NULL,
					   .release = NULL,
					   .ctx = 0};

	if (NULL != sink->seek) {
		session_sink.seek = seek_proxy;
	}

	for (int i = 0; i < sizeof(sources) / sizeof(fetch_source_t); i++) {

		component_lock();

		fetch_source_t *source = &sources[i];
		fetch_source_mgr_fetch_request_fct request_fct = source->request_fct;

		if (0 == ++last_used_session_id) {
			++last_used_session_id;
		}

		session_sink.ctx = (void *)(uintptr_t)last_used_session_id;
		session->session_id = last_used_session_id;

		component_unlock();

		if (NULL != request_fct) {

			int err = request_fct(uri, uri_length, &session_sink);

			if (0 == err) {
				close_session(session);
				return 0;

			} else if (STAGE_PENDING_FIRST_RESPONSE != session->stage) {
				/* error while transfer has arleady started, unrecoverable
				 */
				close_session(session);
				return -1;
			} else {
				/* fetch source signalized an error immediatly, means it does not
				 * support fetching from provided URI, let's try next fetch source
				 */
			}
		}
	}

	return -1;
}
