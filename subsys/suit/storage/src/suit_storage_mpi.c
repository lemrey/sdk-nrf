/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_storage_mpi.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(suit_storage_mpi, CONFIG_SUIT_LOG_LEVEL);

typedef struct {
	suit_manifest_role_t role;
	const uint8_t *addr;
	size_t size;
} suit_storage_mpi_entry_t;

static suit_storage_mpi_entry_t entries[CONFIG_SUIT_STORAGE_N_ENVELOPES];
static size_t entries_len;

suit_plat_err_t suit_storage_mpi_init(void)
{
	memset(entries, 0, sizeof(entries));
	entries_len = 0;

	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_storage_mpi_configuration_load(suit_manifest_role_t role, const uint8_t *addr,
						    size_t size)
{
	const suit_manifest_class_id_t *new_class_id = NULL;
	suit_storage_mpi_t *mpi = (suit_storage_mpi_t *)addr;

	if ((role == SUIT_MANIFEST_UNKNOWN) || (addr == NULL) || (size == 0)) {
		return SUIT_PLAT_ERR_INVAL;
	}

	if (mpi->version != SUIT_MPI_INFO_VERSION) {
		return SUIT_PLAT_ERR_OUT_OF_BOUNDS;
	}

	new_class_id = (const suit_manifest_class_id_t *)mpi->class_id;

	for (size_t i = 0; i < entries_len; i++) {
		const suit_storage_mpi_t *ex_mpi = (suit_storage_mpi_t *)entries[i].addr;
		const suit_manifest_class_id_t *ex_class_id =
			(const suit_manifest_class_id_t *)ex_mpi->class_id;

		if (entries[i].role == role) {
			LOG_ERR("Manifest with given role already configured at index %d", i);
			return SUIT_PLAT_ERR_EXISTS;
		}

		if (suit_metadata_uuid_compare(new_class_id, ex_class_id) == SUIT_PLAT_SUCCESS) {
			LOG_ERR("Manifest with given class ID already configured with different "
				"role at index %d",
				i);
			return SUIT_PLAT_ERR_EXISTS;
		}
	}

	if (ARRAY_SIZE(entries) <= entries_len) {
		LOG_ERR("Too small Manifest Provisioning Information array.");
		return SUIT_PLAT_ERR_SIZE;
	}

	LOG_DBG("Add manifest class ID at index %d:", entries_len);
	LOG_DBG("\t%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
		new_class_id->raw[0], new_class_id->raw[1], new_class_id->raw[2],
		new_class_id->raw[3], new_class_id->raw[4], new_class_id->raw[5],
		new_class_id->raw[6], new_class_id->raw[7], new_class_id->raw[8],
		new_class_id->raw[9], new_class_id->raw[10], new_class_id->raw[11],
		new_class_id->raw[12], new_class_id->raw[13], new_class_id->raw[14],
		new_class_id->raw[15]);

	entries[entries_len].role = role;
	entries[entries_len].addr = addr;
	entries[entries_len].size = size;
	entries_len++;

	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_storage_mpi_role_get(const suit_manifest_class_id_t *class_id,
					  suit_manifest_role_t *role)
{
	if ((class_id == NULL) || (role == NULL)) {
		return SUIT_PLAT_ERR_INVAL;
	}

	for (size_t i = 0; i < entries_len; i++) {
		if (entries[i].role != SUIT_MANIFEST_UNKNOWN) {
			suit_storage_mpi_t *mpi = (suit_storage_mpi_t *)entries[i].addr;

			if (suit_metadata_uuid_compare(
				    (const suit_manifest_class_id_t *)mpi->class_id, class_id) ==
			    SUIT_PLAT_SUCCESS) {
				*role = entries[i].role;
				return SUIT_PLAT_SUCCESS;
			}
		}
	}

	return SUIT_PLAT_ERR_NOT_FOUND;
}

suit_plat_err_t suit_storage_mpi_class_get(suit_manifest_role_t role,
					   const suit_manifest_class_id_t **class_id)
{
	if ((role == SUIT_MANIFEST_UNKNOWN) || (class_id == NULL)) {
		return SUIT_PLAT_ERR_INVAL;
	}

	for (size_t i = 0; i < entries_len; i++) {
		if (entries[i].role == role) {
			suit_storage_mpi_t *mpi = (suit_storage_mpi_t *)entries[i].addr;

			*class_id = (const suit_manifest_class_id_t *)mpi->class_id;

			return SUIT_PLAT_SUCCESS;
		}
	}

	return SUIT_PLAT_ERR_NOT_FOUND;
}

suit_plat_err_t suit_storage_mpi_get(const suit_manifest_class_id_t *class_id,
				     suit_storage_mpi_t **mpi)
{
	if ((class_id == NULL) || (mpi == NULL)) {
		return SUIT_PLAT_ERR_INVAL;
	}

	for (size_t i = 0; i < entries_len; i++) {
		if (entries[i].role != SUIT_MANIFEST_UNKNOWN) {
			*mpi = (suit_storage_mpi_t *)entries[i].addr;

			if (suit_metadata_uuid_compare(
				    (const suit_manifest_class_id_t *)(*mpi)->class_id, class_id) ==
			    SUIT_PLAT_SUCCESS) {
				return SUIT_PLAT_SUCCESS;
			}
		}
	}

	return SUIT_PLAT_ERR_NOT_FOUND;
}

suit_plat_err_t suit_storage_mpi_class_ids_get(suit_manifest_class_info_t *class_info, size_t *size)
{
	if ((class_info == NULL) || (size == NULL)) {
		return SUIT_PLAT_ERR_INVAL;
	}

	if (*size < entries_len) {
		return SUIT_PLAT_ERR_SIZE;
	}

	for (size_t i = 0; i < entries_len; i++) {
		suit_storage_mpi_t *mpi = (suit_storage_mpi_t *)entries[i].addr;
		class_info[i].class_id = (const suit_manifest_class_id_t *)mpi->class_id;
		class_info[i].role = entries[i].role;
	}

	*size = entries_len;

	return SUIT_PLAT_SUCCESS;
}