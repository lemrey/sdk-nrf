/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>

#include <digest_sink.h>

LOG_MODULE_REGISTER(suit_digest_sink, CONFIG_SUIT_LOG_LEVEL);

struct digest_sink_context {
	bool in_use;
	psa_hash_operation_t operation;
	const uint8_t *expected_digest;
	size_t expected_digest_length;
};

static struct digest_sink_context digest_contexts[CONFIG_SUIT_STREAM_SINK_DIGEST_CONTEXT_COUNT];

static struct digest_sink_context *get_new_context(void)
{
	for (size_t i = 0; i < CONFIG_SUIT_STREAM_SINK_DIGEST_CONTEXT_COUNT; ++i) {
		if (!digest_contexts[i].in_use) {
			return &digest_contexts[i];
		}
	}

	return NULL;
}

static int write(void *ctx, uint8_t *buf, size_t *size)
{
	if ((NULL == ctx) || (NULL == buf) || (NULL == size)) {
		LOG_ERR("Invalid arguments");
		return INVALID_ARGUMENT;
	}

	LOG_DBG("buf: %p", (void *)buf);
	LOG_DBG("size: %d", *size);

	struct digest_sink_context *digest_ctx = (struct digest_sink_context *)ctx;

	if (!digest_ctx->in_use) {
		LOG_ERR("Writing to uninitialized sink");
		return NOT_INITIALIZED;
	}

	psa_status_t status = psa_hash_update(&digest_ctx->operation, buf, *size);
	if (PSA_SUCCESS != status) {
		LOG_ERR("Failed to update digest: %d", status);
		return status;
	}

	return SUCCESS;
}

static int release(void *ctx)
{
	if (NULL == ctx) {
		LOG_ERR("Invalid argument");
		return INVALID_ARGUMENT;
	}

	struct digest_sink_context *digest_ctx = (struct digest_sink_context *)ctx;
	memset(digest_ctx, 0, sizeof(struct digest_sink_context));

	return SUCCESS;
}

int digest_sink_get(struct stream_sink *sink, psa_algorithm_t algorithm,
		    const uint8_t *expected_digest)
{
	if (NULL == sink || NULL == expected_digest) {
		LOG_ERR("Invalid argument");
		return INVALID_ARGUMENT;
	}

	struct digest_sink_context *digest_ctx = get_new_context();

	if (NULL == digest_ctx) {
		LOG_ERR("Failed to get a new context");
		return SUIT_MAX_COMPONENTS_REACHED;
	}

	memset((void *)digest_ctx, 0, sizeof(struct digest_sink_context));

	psa_status_t status = psa_crypto_init();
	if (PSA_SUCCESS != status) {
		LOG_ERR("Failed to init psa crypto: %d", status);
		return status;
	}

	status = psa_hash_setup(&digest_ctx->operation, algorithm);
	if (PSA_SUCCESS != status) {
		LOG_ERR("Failed to setup hash algorithm: %d", status);
		return status;
	}

	digest_ctx->in_use = true;
	digest_ctx->expected_digest = expected_digest;
	digest_ctx->expected_digest_length = PSA_HASH_LENGTH(algorithm);

	sink->write = write;
	sink->seek = NULL;
	sink->flush = NULL;
	sink->used_storage = NULL;
	sink->release = release;
	sink->ctx = digest_ctx;

	return SUCCESS;
}

int digest_sink_digest_match(void *ctx)
{
	if (NULL == ctx) {
		LOG_ERR("Invalid argument");
		return INVALID_ARGUMENT;
	}

	struct digest_sink_context *digest_ctx = (struct digest_sink_context *)ctx;

	if (!digest_ctx->in_use) {
		LOG_ERR("Sink not initialized");
		return NOT_INITIALIZED;
	}

	psa_status_t status = psa_hash_verify(&digest_ctx->operation, digest_ctx->expected_digest,
					      digest_ctx->expected_digest_length);
	int err = SUCCESS;
	if (PSA_SUCCESS == status) {
		/* Digest calculation successful; expected digest matches calculated one */
		err = SUCCESS;
	} else {
		if (PSA_ERROR_INVALID_SIGNATURE == status) {
			/* Digest calculation successful but expected digest does not match
			 * calculated one */
			err = DIGEST_MISMATCH;
		} else {
			LOG_ERR("psa_hash_verify error: %d", status);
			err = DIGEST_CALCULATION_FAILURE;
		}
		/* In both cases psa_hash_verify enters error state and must be aborted */
		status = psa_hash_abort(&digest_ctx->operation);
		if (PSA_SUCCESS != status) {
			LOG_ERR("psa_hash_abort error %d", status);
			err = DIGEST_ABORTION_FAILURE;
		}
	}

	return err;
}
