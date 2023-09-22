/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_platform.h>
#include <suit_plat_decode_util.h>
#ifdef CONFIG_SUIT_STORAGE
#include <suit_storage.h>
#endif /* CONFIG_SUIT_STORAGE */
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(plat_config, CONFIG_SUIT_LOG_LEVEL);

int suit_plat_sequence_completed(enum suit_command_sequence seq_name,
				 struct zcbor_string *manifest_component_id,
				 const uint8_t *envelope_str, size_t envelope_len)
{
	int err = SUIT_SUCCESS;

	if ((manifest_component_id == NULL) || (manifest_component_id->value == NULL)) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

#ifdef CONFIG_SUIT_STORAGE
	if (seq_name == SUIT_SEQ_INSTALL) {
		suit_manifest_class_id_t *class_id;

		if (!suit_plat_decode_manifest_class_id(manifest_component_id, &class_id)) {
			LOG_ERR("Unable to decode manifest class ID");
			return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
		}

		err = suit_storage_install_envelope(class_id, (uint8_t *)envelope_str,
						    envelope_len);
		if (err != SUIT_SUCCESS) {
			LOG_ERR("Failed to save envelope: %d", err);
		} else {
			LOG_DBG("Envelope saved");
		}
	}
#elif !defined(CONFIG_ZTEST)
	/* Allow to skip envelope installation only in SUIT tests. */
	err = SUIT_ERR_UNSUPPORTED_COMMAND;
#endif /* CONFIG_SUIT_STORAGE */

	return err;
}

int suit_plat_authorize_sequence_num(enum suit_command_sequence seq_name,
				     struct zcbor_string *manifest_component_id,
				     unsigned int seq_num)
{
	return SUIT_SUCCESS;
}

int suit_plat_authorize_unsigned_manifest(struct zcbor_string *manifest_component_id)
{
#ifdef CONFIG_SUIT_PLATFORM_ACCEPT_UNSIGNED_MANIFESTS
	return SUIT_SUCCESS;
#else  /* CONFIG_SUIT_PLATFORM_ACCEPT_UNSIGNED_MANIFESTS */
	return SUIT_ERR_AUTHENTICATION;
#endif /* CONFIG_SUIT_PLATFORM_ACCEPT_UNSIGNED_MANIFESTS */
}

int suit_plat_authorize_component_id(struct zcbor_string *manifest_component_id,
				     struct zcbor_string *component_id)
{
	/* If component is of type D (or C ?) release attached memptr_record */
	suit_component_type_t component_type = SUIT_COMPONENT_TYPE_UNSUPPORTED;

	if (!suit_plat_decode_component_type(component_id, &component_type)) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	return SUIT_SUCCESS;
}
