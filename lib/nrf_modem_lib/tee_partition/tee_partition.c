#include <stdbool.h>
#include <stdint.h>

#include <psa/crypto.h>

#include "tfm_secure_api.h"
#include "tfm_api.h"
#include "tfm_memory_utils.h"

#define NRF_SOFTSIM_PAYLOAD_SIZE 8
#define NRF_SOFTSIM_APDU_PAYLOAD_SIZE 6

static psa_status_t tfm_tp_init(void)
{
	return TFM_SUCCESS;
}

typedef void (*psa_write_callback_t)(void *handle, uint8_t *digest,
				uint32_t digest_size);

static psa_status_t softsim_init_impl(uint8_t *digest,
				      size_t digest_size,
				      size_t *p_out_payload_size)
{
	/* Custom implementation here */

	static uint8_t atr[NRF_SOFTSIM_PAYLOAD_SIZE] =
		{ 0, 1, 2, 3, 4, 5, 6, 7 };

	memcpy(digest, atr, sizeof(atr));
	*p_out_payload_size = sizeof(atr);

	return PSA_SUCCESS;
}

static psa_status_t softsim_apdu_impl(uint8_t *in_payload,
				      size_t in_payload_size,
				      uint8_t *digest,
				      size_t digest_size,
				      size_t *p_out_payload_size)
{
	/* Custom implementation here */

	static uint8_t apdu[NRF_SOFTSIM_APDU_PAYLOAD_SIZE] =
		{ 8, 9, 10, 11, 12, 13 };

	memcpy(digest, apdu, sizeof(apdu));
	*p_out_payload_size = sizeof(apdu);

	return PSA_SUCCESS;
}

static psa_status_t softsim_deinit_impl()
{
	/* Custom implementation here */

	return PSA_SUCCESS;
}

static psa_status_t tfm_tee_softsim_init(void *out_payload, size_t out_payload_size,
					 size_t *p_out_payload_size,
					 psa_write_callback_t callback)
{
	uint8_t digest[NRF_SOFTSIM_PAYLOAD_SIZE] = { 0 };
	psa_status_t status = PSA_SUCCESS;

	/* Check that digest_size is valid. */
	if (out_payload_size < sizeof(digest)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	status = softsim_init_impl(digest, sizeof(digest), p_out_payload_size);

	if (status != PSA_SUCCESS) {
		return status;
	}

	if (*p_out_payload_size > out_payload_size){
		return PSA_ERROR_PROGRAMMER_ERROR;
	}

	callback(out_payload, digest, *p_out_payload_size);

	return status;
}

static psa_status_t tfm_tee_softsim_apdu(void *in_payload, size_t in_payload_size,
					 void *out_payload, size_t out_payload_size,
					 size_t *p_out_payload_size,
					 psa_write_callback_t callback)
{
	uint8_t digest[NRF_SOFTSIM_APDU_PAYLOAD_SIZE] = { 0 };
	psa_status_t status = PSA_SUCCESS;

	/* Check that digest_size is valid. */
	if (out_payload_size < sizeof(digest)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	status = softsim_apdu_impl(in_payload, in_payload_size,
				   digest, sizeof(digest), p_out_payload_size);

	if (status != PSA_SUCCESS) {
		return status;
	}

	if (*p_out_payload_size > out_payload_size){
		return PSA_ERROR_PROGRAMMER_ERROR;
	}

	callback(out_payload, digest, *p_out_payload_size);

	return status;
}

static psa_status_t tfm_tee_softsim_deinit()
{
	return softsim_deinit_impl();
}

/*
 * \brief Indicates whether TP (TEE Partition) has been initialised.
 */
static bool tp_is_init = false;

/*
 * \brief Initialises TP, if not already initialised.
 *
 * \note In library mode, initialisation is delayed until the first secure
 *	   function call, as calls to the Crypto service are required for
 *	   initialisation.
 *
 * \return PSA_SUCCESS if TP is initialised, PSA_ERROR_GENERIC_ERROR
 *		 otherwise.
 */
static psa_status_t tp_check_init(void)
{
	if (!tp_is_init) {
		if (tfm_tp_init() != PSA_SUCCESS) {
			return PSA_ERROR_GENERIC_ERROR;
		}
		tp_is_init = true;
	}

	return PSA_SUCCESS;
}


void psa_write_digest(void *handle, uint8_t *digest, uint32_t digest_size)
{
	tfm_memcpy(handle, digest, digest_size);
}

psa_status_t tfm_tee_softsim_init_req(psa_invec *in_vec, size_t in_len,
				      psa_outvec *out_vec, size_t out_len)
{
	if (tp_check_init() != PSA_SUCCESS) {
		return PSA_ERROR_GENERIC_ERROR;
	}

	if ((in_len != 1) || (out_len != 1)) {
		/* The number of arguments are incorrect */
		return PSA_ERROR_PROGRAMMER_ERROR;
	}

	if (out_vec[0].len != NRF_SOFTSIM_PAYLOAD_SIZE) {
		/* The input argument size is incorrect */
		return PSA_ERROR_PROGRAMMER_ERROR;
	}

	return tfm_tee_softsim_init(
		(void *)out_vec[0].base, out_vec[0].len,
		&out_vec[0].len,
		psa_write_digest);
}

psa_status_t tfm_tee_softsim_apdu_req(psa_invec *in_vec, size_t in_len,
				      psa_outvec *out_vec, size_t out_len)
{
	if (tp_check_init() != PSA_SUCCESS) {
		return PSA_ERROR_GENERIC_ERROR;
	}

	if ((in_len != 1) || (out_len != 1)) {
		/* The number of arguments are incorrect */
		return PSA_ERROR_PROGRAMMER_ERROR;
	}

	if (in_vec[0].len != NRF_SOFTSIM_PAYLOAD_SIZE) {
		/* The input argument size is incorrect */
		return PSA_ERROR_PROGRAMMER_ERROR;
	}

	if (out_vec[0].len != NRF_SOFTSIM_PAYLOAD_SIZE) {
		/* The input argument size is incorrect */
		return PSA_ERROR_PROGRAMMER_ERROR;
	}

	return tfm_tee_softsim_apdu(
		(void *)in_vec[0].base, in_vec[0].len,
		(void *)out_vec[0].base, out_vec[0].len,
		&out_vec[0].len,
		psa_write_digest);
}

psa_status_t tfm_tee_softsim_deinit_req(psa_invec *in_vec, size_t in_len,
				      psa_outvec *out_vec, size_t out_len)
{
	if (tp_check_init() != PSA_SUCCESS) {
		return PSA_ERROR_GENERIC_ERROR;
	}

	if ((in_len != 1) || (out_len != 1)) {
		/* The number of arguments are incorrect */
		return PSA_ERROR_PROGRAMMER_ERROR;
	}

	return tfm_tee_softsim_deinit();
}

psa_status_t tfm_tp_req_mngr_init(void)
{
	/* In library mode, initialisation is delayed until the first secure
	 * function call, as calls to the Crypto service are required for
	 * initialisation.
	 */
	return PSA_SUCCESS;
}