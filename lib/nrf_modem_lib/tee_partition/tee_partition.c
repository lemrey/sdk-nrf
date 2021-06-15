#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <psa/crypto.h>
#include <psa/service.h>
#include <psa_manifest/tfm_tee_partition.h>

#include <tfm_secure_api.h>
#include <tfm_api.h>

#include <iuicc.h>

static uint8_t data[SIM_HAL_MAX_LE];

typedef psa_status_t (*tp_func_t)(psa_msg_t *);

static psa_status_t tfm_tee_softsim_init_ipc(psa_msg_t *msg)
{
	int err;
	size_t data_len;

	if (msg->out_size[0] != SIM_HAL_MAX_LE) {
		/* The input argument size is incorrect */
		return PSA_ERROR_PROGRAMMER_ERROR;
	}

	data_len = 0;

	err = iuicc_card_activate(data, &data_len);

	if (err == 0) {
		if (data_len <= SIM_HAL_MAX_LE) {
			/* Copying activation data
			 * back to the caller
			 * in a secure way
			 */
			psa_write(msg->handle, 0,
				  data, data_len);
			msg->out_size[0] = data_len;
		} else {
			return PSA_ERROR_PROGRAMMER_ERROR;
		}
	} else {
		PSA_ERROR_GENERIC_ERROR;
	}

	return PSA_SUCCESS;
}

static psa_status_t tfm_tee_softsim_apdu_ipc(psa_msg_t *msg)
{
	int err;
	size_t size;
	size_t data_len;

	if (msg->in_size[0] > SIM_HAL_MAX_LE) {
		/* The input argument size is incorrect */
		return PSA_ERROR_PROGRAMMER_ERROR;
	}

	if (msg->out_size[0] != SIM_HAL_MAX_LE) {
		/* The input argument size is incorrect */
		return PSA_ERROR_PROGRAMMER_ERROR;
	}

	size = psa_read(msg->handle, 0, data, msg->in_size[0]);

	if (size != msg->in_size[0]) {
		return PSA_ERROR_PROGRAMMER_ERROR;
	}

	data_len = msg->in_size[0];

	err = iuicc_transport_apdu(data, &data_len);

	if (err == 0) {
		if (data_len <= SIM_HAL_MAX_LE) {
			/* Copying APDU data
			 * back to the caller
			 * in a secure way
			 */
			psa_write(msg->handle, 0,
				  data, data_len);
			msg->out_size[0] = data_len;
		} else {
			return PSA_ERROR_PROGRAMMER_ERROR;
		}
	} else {
		PSA_ERROR_GENERIC_ERROR;
	}

	return PSA_SUCCESS;
}

static psa_status_t tfm_tee_softsim_deinit_ipc(psa_msg_t *msg)
{
	int err = iuicc_card_deactivate();

	/* This shouldn't fail but we check
	 * anyways just to be safe
	 */
	if (err == 0) {
		return PSA_SUCCESS;
	} else {
		return PSA_ERROR_GENERIC_ERROR;
	}
}

static void tp_signal_handle(psa_signal_t signal, tp_func_t pfn)
{
	psa_status_t status;
	psa_msg_t msg;

	status = psa_get(signal, &msg);
	switch (msg.type) {
	case PSA_IPC_CONNECT:
		psa_reply(msg.handle, PSA_SUCCESS);
		break;
	case PSA_IPC_CALL:
		status = pfn(&msg);
		psa_reply(msg.handle, status);
		break;
	case PSA_IPC_DISCONNECT:
		psa_reply(msg.handle, PSA_SUCCESS);
		break;
	default:
		psa_panic();
	}
}

psa_status_t tfm_tp_req_mngr_init(void)
{
	psa_signal_t signals = 0;

	while (1) {
		signals = psa_wait(PSA_WAIT_ANY, PSA_BLOCK);
		if (signals & TFM_TEE_SOFTSIM_INIT_SIGNAL) {
			tp_signal_handle(TFM_TEE_SOFTSIM_INIT_SIGNAL,
					 tfm_tee_softsim_init_ipc);
		} else if (signals & TFM_TEE_SOFTSIM_APDU_SIGNAL) {
			tp_signal_handle(TFM_TEE_SOFTSIM_APDU_SIGNAL,
					 tfm_tee_softsim_apdu_ipc);
		} else if (signals & TFM_TEE_SOFTSIM_DEINIT_SIGNAL) {
			tp_signal_handle(TFM_TEE_SOFTSIM_DEINIT_SIGNAL,
					 tfm_tee_softsim_deinit_ipc);
		} else {
			psa_panic();
		}
	}

	return PSA_ERROR_SERVICE_FAILURE;
}
