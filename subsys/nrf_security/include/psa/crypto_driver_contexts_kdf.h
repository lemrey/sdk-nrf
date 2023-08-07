/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef PSA_CRYPTO_DRIVER_CONTEXTS_KDF_H
#define PSA_CRYPTO_DRIVER_CONTEXTS_KDF_H

#include "psa/crypto_driver_common.h"

/* Include the context structure definitions for enabled drivers. */

#if defined(PSA_CRYPTO_DRIVER_HAS_KDF_SUPPORT_OBERON)
#include "oberon_kdf.h"
#endif

#if defined(PSA_CRYPTO_DRIVER_ALG_CTR_DRBG_OBERON)
#include "oberon_ctr_drbg.h"
#endif

#if defined(PSA_CRYPTO_DRIVER_ALG_HMAC_DRBG_OBERON)
#include "oberon_hmac_drbg.h"
#endif

#if defined(PSA_CRYPTO_DRIVER_ALG_JPAKE_OBERON)
#include "oberon_jpake.h"
#endif

#if defined(PSA_CRYPTO_DRIVER_ALG_SPAKE2P_OBERON)
#include "oberon_spake2p.h"
#endif

#if defined(PSA_CRYPTO_DRIVER_ALG_SRP_OBERON)
#include "oberon_srp.h"
#endif

/* Define the context to be used for an operation that is executed through the
 * PSA Driver wrapper layer as the union of all possible drivers' contexts.
 *
 * The union members are the driver's context structures, and the member names
 * are formatted as `'drivername'_ctx`.
 */

typedef union {
	unsigned int dummy; /* Make sure this union is always non-empty */
#ifdef PSA_CRYPTO_DRIVER_HAS_KDF_SUPPORT_OBERON
	oberon_key_derivation_operation_t oberon_kdf_ctx;
#endif
} psa_driver_key_derivation_context_t;

typedef union {
	unsigned int dummy; /* Make sure this union is always non-empty */
#if defined(PSA_CRYPTO_DRIVER_ALG_JPAKE_OBERON)
	oberon_jpake_operation_t oberon_jpake_ctx;
#endif
#if defined(PSA_CRYPTO_DRIVER_ALG_SPAKE2P_OBERON)
	oberon_spake2p_operation_t oberon_spake2p_ctx;
#endif
#if defined(PSA_CRYPTO_DRIVER_ALG_SRP_OBERON)
	oberon_srp_operation_t oberon_srp_ctx;
#endif
} psa_driver_pake_context_t;

typedef union {
	unsigned int dummy; /* Make sure this union is always non-empty */

#if defined(PSA_CRYPTO_DRIVER_ALG_CTR_DRBG_OBERON)
	oberon_ctr_drbg_context_t oberon_ctr_drbg_ctx;
#endif
#if defined(PSA_CRYPTO_DRIVER_ALG_HMAC_DRBG_OBERON)
	oberon_hmac_drbg_context_t oberon_hmac_drbg_ctx;
#endif
} psa_driver_random_context_t;

#endif /* PSA_CRYPTO_DRIVER_CONTEXTS_KDF_H */
