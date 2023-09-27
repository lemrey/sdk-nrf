/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DIGEST_SINK_H__
#define DIGEST_SINK_H__

#include <sink.h>
#include <psa/crypto.h>

/**
 * @brief Get the digest_sink object
 *
 * @note This function has to be called prior to calling digest_sink_digest_match function
 *
 * @param[in] sink Pointer to sink_stream to be filled
 * @param[in] algorithm Algorithm to be used for digest calculation
 * @param[in] expected_digest digest to be used for comparison against calculated digest value
 * @return int SUCCESS if success, error code otherwise
 */
int digest_sink_get(struct stream_sink *sink, psa_algorithm_t algorithm,
		    const uint8_t *expected_digest);

/**
 * @brief Check if digest matches expected digest value
 *
 * @note A sink has to be initialized with digest_sink_get function call.
 * @note Then data intended for digest calculation has to be fed in using sink's write API function
 * call(s).
 * @note Finally this function can be called to finilize digest calculation and perform its
 * verification.
 *
 * @param[in] ctx Context of a sink used for digest calculation
 *
 * @return int SUCCESS Digest calculation was successful and it matches expected digest
 * @return int DIGEST_MISMATCH Digest calculation was successful but it does not match expected
 * digest
 * @return int DIGEST_CALCULATION_FAILURE Digest could not be calculated
 * @return int DIGEST_ABORTION_FAILURE Failure during cleanup after error
 * @return int INVALID_ARGUMENT @ctx is NULL
 * @return int NOT_INITIALIZED A sink was not initialized using digest_sink_get function
 */
int digest_sink_digest_match(void *ctx);

#endif /* DIGEST_SINK_H__ */
