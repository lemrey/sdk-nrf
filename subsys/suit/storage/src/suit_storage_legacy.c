/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_storage_mpi.h>
#include <suit_storage_internal.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(suit_storage, CONFIG_SUIT_LOG_LEVEL);

/* Update candidate metadata, aligned to the erase block size. */
union suit_update_candidate_entry {
	struct update_candidate_info update;
	uint8_t erase_block[EB_SIZE(struct update_candidate_info)];
};

/* SUIT envelope slot.
 *
 * Each slot contains a CBOR-encoded map with severed SUIT envelope.
 */
typedef uint8_t suit_envelope_encoded_t[CONFIG_SUIT_STORAGE_ENVELOPE_SIZE];

/* SUIT envelope slot, aligned to the erase block size. */
union suit_envelope_entry {
	suit_envelope_encoded_t envelope_encoded;
	uint8_t erase_block[EB_SIZE(suit_envelope_encoded_t)];
};

/* SUIT configuration erea, set by application (i.e. thorough IPC service) and
 * checked using suit-condition-check-content.
 *
 * Currently both the service and the directive is not supported.
 * This definition allows to implement up to three 32-bit configuration values.
 * It is defined here, so the system will not be repartitioned once this feature
 * will be introduced in the future.
 */
typedef uint8_t suit_config_t[CONFIG_SUIT_STORAGE_CONFIG_SIZE];

/* SUIT configuration area, aligned to the erase block size. */
union suit_config_entry {
	suit_config_t config;
	uint8_t erase_block[EB_SIZE(suit_config_t)];
};

/* SUIT storage structure, that can be used to parse the NVM contents. */
struct suit_storage {
	/** Update candidate information. */
	union suit_update_candidate_entry update;
	/** SUIT configuration area */
	union suit_config_entry config;
	/** A copy of the configuration area to protect against random resets. */
	union suit_config_entry config_backup;
	/** The main storage for the SUIT envelopes. */
	union suit_envelope_entry envelopes[CONFIG_SUIT_STORAGE_N_ENVELOPES];
};

#if defined(CONFIG_SOC_NRF54H20) && !defined(CONFIG_SUIT_MCI_IMPL_CUSTOM)
static const suit_storage_mpi_t mpi_nRF54H20_sample[] = {
	{
		.version = SUIT_MPI_INFO_VERSION,
		.downgrade_prevention_policy = SUIT_MPI_DOWNGRADE_PREVENTION_ENABLED,
		.independent_updateability_policy = SUIT_MPI_INDEPENDENT_UPDATE_ALLOWED,
		.signature_verification_policy =
			SUIT_MPI_SIGNATURE_CHECK_ENABLED_ON_UPDATE_AND_BOOT,
		/* RFC4122 uuid5(uuid.NAMESPACE_DNS, 'nordicsemi.com') */
		.vendor_id = {0x76, 0x17, 0xda, 0xa5, 0x71, 0xfd, 0x5a, 0x85, 0x8f, 0x94, 0xe2,
			      0x8d, 0x73, 0x5c, 0xe9, 0xf4},
		/* RFC4122 uuid5(nordic_vid, 'nRF54H20_nordic_top') */
		.class_id = {0xf0, 0x3d, 0x38, 0x5e, 0xa7, 0x31, 0x56, 0x05, 0xb1, 0x5d, 0x03, 0x7f,
			     0x6d, 0xa6, 0x09, 0x7f},
	},
	{
		.version = SUIT_MPI_INFO_VERSION,
		.downgrade_prevention_policy = SUIT_MPI_DOWNGRADE_PREVENTION_ENABLED,
		.independent_updateability_policy = SUIT_MPI_INDEPENDENT_UPDATE_DENIED,
		.signature_verification_policy =
			SUIT_MPI_SIGNATURE_CHECK_ENABLED_ON_UPDATE_AND_BOOT,
		/* RFC4122 uuid5(uuid.NAMESPACE_DNS, 'nordicsemi.com') */
		.vendor_id = {0x76, 0x17, 0xda, 0xa5, 0x71, 0xfd, 0x5a, 0x85, 0x8f, 0x94, 0xe2,
			      0x8d, 0x73, 0x5c, 0xe9, 0xf4},
		/* RFC4122 uuid5(nordic_vid, 'nRF54H20_sec') */
		.class_id = {0xd9, 0x6b, 0x40, 0xb7, 0x09, 0x2b, 0x5c, 0xd1, 0xa5, 0x9f, 0x9a, 0xf8,
			     0x0c, 0x33, 0x7e, 0xba},
	},
	{
		.version = SUIT_MPI_INFO_VERSION,
		.downgrade_prevention_policy = SUIT_MPI_DOWNGRADE_PREVENTION_ENABLED,
		.independent_updateability_policy = SUIT_MPI_INDEPENDENT_UPDATE_DENIED,
		.signature_verification_policy =
			SUIT_MPI_SIGNATURE_CHECK_ENABLED_ON_UPDATE_AND_BOOT,
		/* RFC4122 uuid5(uuid.NAMESPACE_DNS, 'nordicsemi.com') */
		.vendor_id = {0x76, 0x17, 0xda, 0xa5, 0x71, 0xfd, 0x5a, 0x85, 0x8f, 0x94, 0xe2,
			      0x8d, 0x73, 0x5c, 0xe9, 0xf4},
		/* RFC4122 uuid5(nordic_vid, 'nRF54H20_sys') */
		.class_id = {0xc0, 0x8a, 0x25, 0xd7, 0x35, 0xe6, 0x59, 0x2c, 0xb7, 0xad, 0x43, 0xac,
			     0xc8, 0xd1, 0xd1, 0xc8},
	},
	{
		.version = SUIT_MPI_INFO_VERSION,
		.downgrade_prevention_policy = SUIT_MPI_DOWNGRADE_PREVENTION_DISABLED,
		.independent_updateability_policy = SUIT_MPI_INDEPENDENT_UPDATE_ALLOWED,
		.signature_verification_policy = SUIT_MPI_SIGNATURE_CHECK_DISABLED,
		/* RFC4122 uuid5(uuid.NAMESPACE_DNS, 'nordicsemi.com') */
		.vendor_id = {0x76, 0x17, 0xda, 0xa5, 0x71, 0xfd, 0x5a, 0x85, 0x8f, 0x94, 0xe2,
			      0x8d, 0x73, 0x5c, 0xe9, 0xf4},
		/* RFC4122 uuid5(nordic_vid, 'nRF54H20_sample_root') */
		.class_id = {0x3f, 0x6a, 0x3a, 0x4d, 0xcd, 0xfa, 0x58, 0xc5, 0xac, 0xce, 0xf9, 0xf5,
			     0x84, 0xc4, 0x11, 0x24},
	},
	{
		.version = SUIT_MPI_INFO_VERSION,
		.downgrade_prevention_policy = SUIT_MPI_DOWNGRADE_PREVENTION_DISABLED,
		.independent_updateability_policy = SUIT_MPI_INDEPENDENT_UPDATE_DENIED,
		.signature_verification_policy = SUIT_MPI_SIGNATURE_CHECK_DISABLED,
		/* RFC4122 uuid5(uuid.NAMESPACE_DNS, 'nordicsemi.com') */
		.vendor_id = {0x76, 0x17, 0xda, 0xa5, 0x71, 0xfd, 0x5a, 0x85, 0x8f, 0x94, 0xe2,
			      0x8d, 0x73, 0x5c, 0xe9, 0xf4},
		/* RFC4122 uuid5(nordic_vid, 'nRF54H20_sample_app') */
		.class_id = {0x08, 0xc1, 0xb5, 0x99, 0x55, 0xe8, 0x5f, 0xbc, 0x9e, 0x76, 0x7b, 0xc2,
			     0x9c, 0xe1, 0xb0, 0x4d},
	},
	{
		.version = SUIT_MPI_INFO_VERSION,
		.downgrade_prevention_policy = SUIT_MPI_DOWNGRADE_PREVENTION_DISABLED,
		.independent_updateability_policy = SUIT_MPI_INDEPENDENT_UPDATE_DENIED,
		.signature_verification_policy = SUIT_MPI_SIGNATURE_CHECK_DISABLED,
		/* RFC4122 uuid5(uuid.NAMESPACE_DNS, 'nordicsemi.com') */
		.vendor_id = {0x76, 0x17, 0xda, 0xa5, 0x71, 0xfd, 0x5a, 0x85, 0x8f, 0x94, 0xe2,
			      0x8d, 0x73, 0x5c, 0xe9, 0xf4},
		/* RFC4122 uuid5(nordic_vid, 'nRF54H20_sample_rad') */
		.class_id = {0x81, 0x6a, 0xa0, 0xa0, 0xaf, 0x11, 0x5e, 0xf2, 0x85, 0x8a, 0xfe, 0xb6,
			     0x68, 0xb2, 0xe9, 0xc9},
	}};
#endif /* CONFIG_SOC_NRF54H20 && !CONFIG_SUIT_MCI_IMPL_CUSTOM */

#ifdef CONFIG_SUIT_MCI_IMPL_CUSTOM
static const suit_storage_mpi_t mpi_test_sample[] = {
	{
		.version = SUIT_MPI_INFO_VERSION,
		.downgrade_prevention_policy = SUIT_MPI_DOWNGRADE_PREVENTION_DISABLED,
		.independent_updateability_policy = SUIT_MPI_INDEPENDENT_UPDATE_ALLOWED,
		.signature_verification_policy = SUIT_MPI_SIGNATURE_CHECK_DISABLED,
		/* RFC4122 uuid5(uuid.NAMESPACE_DNS, 'nordicsemi.com') */
		.vendor_id = {0x76, 0x17, 0xda, 0xa5, 0x71, 0xfd, 0x5a, 0x85, 0x8f, 0x94, 0xe2,
			      0x8d, 0x73, 0x5c, 0xe9, 0xf4},
		/* RFC4122 uuid5(nordic_vid, 'test_sample_root') */
		.class_id = {0x97, 0x05, 0x48, 0x23, 0x4c, 0x3d, 0x59, 0xa1, 0x89, 0x86, 0xa5, 0x46,
			     0x60, 0xa1, 0x4b, 0x0a},
	},
	{
		.version = SUIT_MPI_INFO_VERSION,
		.downgrade_prevention_policy = SUIT_MPI_DOWNGRADE_PREVENTION_DISABLED,
		.independent_updateability_policy = SUIT_MPI_INDEPENDENT_UPDATE_DENIED,
		.signature_verification_policy = SUIT_MPI_SIGNATURE_CHECK_DISABLED,
		/* RFC4122 uuid5(uuid.NAMESPACE_DNS, 'nordicsemi.com') */
		.vendor_id = {0x76, 0x17, 0xda, 0xa5, 0x71, 0xfd, 0x5a, 0x85, 0x8f, 0x94, 0xe2,
			      0x8d, 0x73, 0x5c, 0xe9, 0xf4},
		/* RFC4122 uuid5(nordic_vid, 'test_sample_app') */
		.class_id = {0x5b, 0x46, 0x9f, 0xd1, 0x90, 0xee, 0x53, 0x9c, 0xa3, 0x18, 0x68, 0x1b,
			     0x03, 0x69, 0x5e, 0x36},
	}};
#endif /* CONFIG_SUIT_MCI_IMPL_CUSTOM */

static suit_plat_err_t find_mpi_area(suit_manifest_role_t role, uint8_t **addr, size_t *size)
{
	int index = -1;

#if defined(CONFIG_SOC_NRF54H20) && !defined(CONFIG_SUIT_MCI_IMPL_CUSTOM)
	const suit_storage_mpi_t *mpi_config = mpi_nRF54H20_sample;

	switch (role) {
	case SUIT_MANIFEST_APP_ROOT:
		index = 0;
		break;
	case SUIT_MANIFEST_APP_LOCAL_1:
		index = 1;
		break;
	case SUIT_MANIFEST_RAD_LOCAL_1:
		index = 2;
		break;
	case SUIT_MANIFEST_SEC_TOP:
		index = 3;
		break;
	case SUIT_MANIFEST_SEC_SDFW:
		index = 4;
		break;
	case SUIT_MANIFEST_SEC_SYSCTRL:
		index = 5;
		break;
	default:
		index = -1;
		break;
	}
#endif /* CONFIG_SOC_NRF54H20 && !CONFIG_SUIT_MCI_IMPL_CUSTOM */

#ifdef CONFIG_SUIT_MCI_IMPL_CUSTOM
	const suit_storage_mpi_t *mpi_config = mpi_test_sample;

	switch (role) {
	case SUIT_MANIFEST_APP_ROOT:
		index = 0;
		break;
	case SUIT_MANIFEST_APP_LOCAL_1:
		index = 1;
		break;
	default:
		index = -1;
		break;
	}
#endif /* CONFIG_SUIT_MCI_IMPL_CUSTOM */

	if (index == -1) {
		return SUIT_PLAT_ERR_OUT_OF_BOUNDS;
	}

	*addr = (uint8_t *)&mpi_config[index];
	*size = sizeof(mpi_config[index]);

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t find_manifest_area(suit_manifest_role_t role, uint8_t **addr, size_t *size)
{
	struct suit_storage *storage = (struct suit_storage *)SUIT_STORAGE_ADDRESS;
	int index = -1;

#if defined(CONFIG_SOC_NRF54H20) && !defined(CONFIG_SUIT_MCI_IMPL_CUSTOM)
	switch (role) {
	case SUIT_MANIFEST_SEC_TOP:
		index = 0;
		break;
	case SUIT_MANIFEST_SEC_SDFW:
		index = 1;
		break;
	case SUIT_MANIFEST_SEC_SYSCTRL:
		index = 2;
		break;
	case SUIT_MANIFEST_APP_ROOT:
		index = 3;
		break;
	case SUIT_MANIFEST_APP_RECOVERY:
		index = 4;
		break;
	case SUIT_MANIFEST_APP_LOCAL_1:
		index = 5;
		break;
	case SUIT_MANIFEST_APP_LOCAL_2:
		index = 6;
		break;
	case SUIT_MANIFEST_RAD_LOCAL_1:
		index = 7;
		break;
	default:
		index = -1;
		break;
	}
#endif /* CONFIG_SOC_NRF54H20 && !CONFIG_SUIT_MCI_IMPL_CUSTOM */

#ifdef CONFIG_SUIT_MCI_IMPL_CUSTOM
	switch (role) {
	case SUIT_MANIFEST_APP_ROOT:
		index = 0;
		break;
	case SUIT_MANIFEST_APP_LOCAL_1:
		index = 1;
		break;
	default:
		index = -1;
		break;
	}
#endif /* CONFIG_SUIT_MCI_IMPL_CUSTOM */

	if (index == -1) {
		return SUIT_PLAT_ERR_OUT_OF_BOUNDS;
	}

	*addr = storage->envelopes[index].erase_block;
	*size = sizeof(storage->envelopes[index].erase_block);

	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_storage_init(void)
{
	suit_plat_err_t err = SUIT_PLAT_SUCCESS;
	suit_manifest_role_t roles[] = {
		SUIT_MANIFEST_SEC_TOP,	SUIT_MANIFEST_SEC_SDFW,	   SUIT_MANIFEST_SEC_SYSCTRL,
		SUIT_MANIFEST_APP_ROOT, SUIT_MANIFEST_APP_LOCAL_1, SUIT_MANIFEST_RAD_LOCAL_1,
	};

	if (sizeof(struct suit_storage) > SUIT_STORAGE_SIZE) {
		return SUIT_PLAT_ERR_NOMEM;
	}

	if (CEIL_DIV(SUIT_STORAGE_SIZE, SUIT_STORAGE_EB_SIZE) * SUIT_STORAGE_EB_SIZE !=
	    SUIT_STORAGE_SIZE) {
		return SUIT_PLAT_ERR_CRASH;
	}

	err = suit_storage_mpi_init();
	if (err != SUIT_PLAT_SUCCESS) {
		return err;
	}

	err = suit_storage_update_init();
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Unable to initialize update candidate staging area (err: %d)", err);
	}

	/* Bootstrap MPI entries. */
	for (size_t i = 0; i < ARRAY_SIZE(roles); i++) {
		uint8_t *mpi_addr;
		size_t mpi_size;

		err = find_mpi_area(roles[i], &mpi_addr, &mpi_size);
		if (err != SUIT_PLAT_SUCCESS) {
			continue;
		}

		err = suit_storage_mpi_configuration_load(roles[i], mpi_addr, mpi_size);
		if (err != SUIT_PLAT_SUCCESS) {
			return err;
		}
	}

	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_storage_update_cand_get(const suit_plat_mreg_t **regions, size_t *len)
{
	struct suit_storage *storage = (struct suit_storage *)SUIT_STORAGE_ADDRESS;
	uint8_t *addr = storage->update.erase_block;
	size_t size = sizeof(storage->update.erase_block);

	return suit_storage_update_get(addr, size, regions, len);
}

suit_plat_err_t suit_storage_update_cand_set(suit_plat_mreg_t *regions, size_t len)
{
	struct suit_storage *storage = (struct suit_storage *)SUIT_STORAGE_ADDRESS;
	uint8_t *addr = storage->update.erase_block;
	size_t size = sizeof(storage->update.erase_block);

	return suit_storage_update_set(addr, size, regions, len);
}

suit_plat_err_t suit_storage_installed_envelope_get(const suit_manifest_class_id_t *id,
						    uint8_t **addr, size_t *size)
{
	suit_manifest_role_t role;

	if ((id == NULL) || (addr == NULL) || (size == 0)) {
		return SUIT_PLAT_ERR_INVAL;
	}

	suit_plat_err_t err = suit_storage_mpi_role_get(id, &role);
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_INF("Unable to find role for given class ID.");
		return err;
	}

	err = find_manifest_area(role, addr, size);
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_INF("Unable to find area for envelope with role %d.", role);
		return err;
	}

	LOG_DBG("Decode envelope with role: %d address: 0x%lx", role, (intptr_t)(*addr));

	err = suit_storage_envelope_get(*addr, *size, id, addr, size);
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Unable to parse envelope with role %d", role);
		return err;
	}

	LOG_INF("Valid envelope with given class ID and role %d found", role);

	return err;
}

suit_plat_err_t suit_storage_install_envelope(const suit_manifest_class_id_t *id, uint8_t *addr,
					      size_t size)
{
	suit_plat_err_t err;
	suit_manifest_role_t role;
	uint8_t *area_addr = NULL;
	size_t area_size = 0;

	err = suit_storage_mpi_role_get(id, &role);
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_INF("Unable to find role for given class ID.");
		return err;
	}

	if ((addr == NULL) || (size == 0)) {
		return SUIT_PLAT_ERR_INVAL;
	}

	err = find_manifest_area(role, &area_addr, &area_size);
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_INF("Unable to find area for envelope with role %d.", role);
		return err;
	}

	err = suit_storage_envelope_install(area_addr, area_size, id, addr, size);
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_INF("Failed to install envelope with role %d.", role);
		return err;
	}

	LOG_INF("Envelope with role %d saved.", role);

	return err;
}