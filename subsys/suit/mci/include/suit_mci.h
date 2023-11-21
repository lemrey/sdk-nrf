/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUIT_MCI_H__
#define SUIT_MCI_H__

/*
 * SUIT (Software Updates for Internet of Things) installation and booting processes are described
 * in manifests as a sequences of instructions. That enables customization of update and booting
 * process at level of manifest. Aim of this API is to represent set of device constrains which
 * shall be applied to manifest processing.
 *
 * Manifests topology: Device images are described by one or more SUIT manifests, grouped in a tree
 * structures. Each of those manifests supported by the device is identified by UUID based
 * suit_manifest_class_id_t
 *
 * Boot order: SUIT manifest contains booting instructions (suit-invoke command sequence),
 * effectively controlling the booting order for components it covers. As manifests are grouped in
 * tree structures, parent manifest controls the order of invocation of its children suit-invoke
 * command sequences. Having multiple manifests in the device - there should be a mechanism which
 * would allow bootloader to chose a specific manifest or list of manifests to process its
 * suit-invoke in desired order.
 *
 * Downgrade prevention policy: Product requirements may enforce different downgrade prevention
 * policies attached to different components (i.e. cellular and application firmware).
 * Each manifest supported by the device has its downgrade prevention policy attached.
 * When update candidate is delivered to the device, update may be proceeded or be refused based
 * on sequence number in candidate manifest, sequence number of relevant manifest installed
 * in the device and respective downgrade prevention policy
 *
 * Key ID: Key id value can be utilized to encode purpose/signing authority associated with the key.
 * This may be utilized to detect the situation that manifest is signed with valid key, but
 * associated to authority which does not hold rights to sign this manifest. Definition of key id
 * semantic is out of scope of MCI, MCI just enables validation of value of selected bits of key id.
 *
 * Manifest access rights: malicious or unintentionally defective manifests may operate on
 * resources (i.e. memory ranges, cpu and others) they are not entitled to. There should be a
 * mechanism which would allow to check whether manifest operates on resources it is entitled to.
 * Such checking shall be applied before actual manifest processing starts, as detection of access
 * violation in the middle of update procedure may lead to unbootable state of the device. Memory
 * range checks do not differentiate between read and write access, as distinct manifest is
 * typically responsible both for installation and verification and eventual invocation of specific
 * suit component.
 *
 */
#include <zephyr/types.h>
#include <suit_plat_err.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint8_t raw[16];
} suit_uuid_t;

typedef suit_uuid_t suit_manifest_class_id_t;

typedef enum {
	DOWNGRADE_PREVENTION_DISABLED = 0,
	DOWNGRADE_PREVENTION_ENABLED = 1
} downgrade_prevention_policy_t;

typedef int mci_err_t;

 /**< Invalid or unsupported manifest class id */
#define MCI_ERR_MANIFESTCLASSID   1
/**< Manifest is not entitled to operate on resource */
#define MCI_ERR_NOACCESS	      2
/**< Provided key ID is invalid for desired operation */
#define MCI_ERR_WRONGKEYID	      3
/**< Content of compared suit_uuid_t structures differs */
#define MCI_ERR_COMPARISON_FAILED 4

/**
 * @brief Gets Nordic vendor id
 *
 * @param[out]  vendor_id     Nordic vendor id
 *
 * @retval SUIT_PLAT_SUCCESS    on success
 * @retval SUIT_PLAT_ERR_INVAL  invalid parameter, i.e. null pointer
 */
mci_err_t mci_get_nordic_vendor_id(const suit_uuid_t **vendor_id);

/**
 * @brief Gets class id for components which are not associated to specific class id
 *
 * @param[out]  vendor_id     Nordic vendor id
 *
 * @retval SUIT_PLAT_SUCCESS    on success
 * @retval SUIT_PLAT_ERR_INVAL  invalid parameter, i.e. null pointer
 */
mci_err_t mci_get_nordic_unspecified_class_id(const suit_uuid_t **class_id);

/**
 * @brief Gets an array of supported manifest class ids.
 *
 * @param[out]     class_id	Array of manifest class ids
 * @param[in,out]  size		as input - maximal amount of elements an array can hold,
 *				as output - amount of stored elements
 *
 * @retval SUIT_PLAT_SUCCESS    on success
 * @retval SUIT_PLAT_ERR_INVAL  invalid parameter, i.e. null pointer
 * @retval SUIT_PLAT_ERR_SIZE   too small to store all information
 */
mci_err_t mci_get_supported_manifest_class_ids(const suit_manifest_class_id_t **class_id, size_t *size);

/**
 * @brief Gets an array of manifest class ids in invocation order.
 *
 * @param[out]     class_id	Array of manifest class ids
 * @param[in,out]  size		as input - maximal amount of elements an array can hold,
 *				as output - amount of stored elements
 *
 * @retval SUIT_PLAT_SUCCESS    on success
 * @retval SUIT_PLAT_ERR_INVAL  invalid parameter, i.e. null pointer
 * @retval SUIT_PLAT_ERR_SIZE   too small to store all information
 */
mci_err_t mci_get_invoke_order(const suit_manifest_class_id_t **class_id, size_t *size);

/**
 * @brief Gets downgrade prevention policy for manifest class id
 *
 * @param[in]   class_id	Manifest class id
 * @param[out]  policy
 *
 * @retval SUIT_PLAT_SUCCESS        on success
 * @retval SUIT_PLAT_ERR_INVAL      invalid parameter, i.e. null pointer
 * @retval MCI_ERR_MANIFESTCLASSID  manifest class id unsupported
 */
mci_err_t mci_get_downgrade_prevention_policy(const suit_manifest_class_id_t *class_id,
					downgrade_prevention_policy_t *policy);

/**
 * @brief Validates if manifest class id is supported in the device.
 *
 * @param[in]  class_id	Manifest class id
 *
 * @retval SUIT_PLAT_SUCCESS        on success
 * @retval SUIT_PLAT_ERR_INVAL      invalid parameter, i.e. null pointer
 * @retval MCI_ERR_MANIFESTCLASSID  manifest class id unsupported
 */
mci_err_t mci_validate_manifest_class_id(const suit_manifest_class_id_t *class_id);

/**
 * @brief Verifying whether specific key_id is valid for signing/checking signature of specific
 *manifest class
 *
 * @param[in]   class_id	Manifest class id
 * @param[in]   key_id		Identifier of key utilized for manifest signing. key_id may be equal
 *				to 0. In that case function returns success if manifest class id
 *				does not require signing.
 *
 * @retval SUIT_PLAT_SUCCESS        on success
 * @retval SUIT_PLAT_ERR_INVAL      invalid parameter, i.e. null pointer
 * @retval MCI_ERR_MANIFESTCLASSID  manifest class id unsupported
 * @retval MCI_ERR_WRONGKEYID       provided key ID is invalid for signing
 *                                  for provided manifest class
 */
mci_err_t mci_validate_signing_key_id(const suit_manifest_class_id_t *class_id, uint32_t key_id);

/**
 * @brief Verifies if manifest with specific class id is entitled to start (invoke) code on specific
 * processor
 *
 * @param[in]   class_id	    Manifest class id
 * @param[in]   processor_id	Processor id. Refer to Product Specification
 *
 * @retval SUIT_PLAT_SUCCESS        on success
 * @retval SUIT_PLAT_ERR_INVAL      invalid parameter, i.e. null pointer
 * @retval MCI_ERR_MANIFESTCLASSID  manifest class id unsupported
 * @retval MCI_ERR_NOACCESS         manifest is not entitled to operate on cpu
 */
mci_err_t mci_validate_processor_start_rights(const suit_manifest_class_id_t *class_id, int processor_id);

/**
 * @brief Verifies if manifest with specific class id is entitled to operate on memory range
 *
 * @param[in]   class_id	Manifest class id
 * @param[in]   address
 * @param[in]   mem_size	size of access area
 *
 * @retval SUIT_PLAT_SUCCESS        on success
 * @retval SUIT_PLAT_ERR_INVAL      invalid parameter, i.e. null pointer
 * @retval MCI_ERR_MANIFESTCLASSID  manifest class id unsupported
 * @retval MCI_ERR_NOACCESS         manifest is not entitled to operate on
 *                                  memory range
 */
mci_err_t mci_validate_memory_access_rights(const suit_manifest_class_id_t *class_id, void *address,
				      size_t mem_size);

/**
 * @brief Verifies if manifest with specific class id is entitled to operate on non-memory platform
 * component
 *
 * @param[in]   class_id	Manifest class id
 * @param[in]   platform_specific_component_number
 *
 * @retval SUIT_PLAT_SUCCESS        on success
 * @retval SUIT_PLAT_ERR_INVAL      invalid parameter, i.e. null pointer
 * @retval MCI_ERR_MANIFESTCLASSID  manifest class id unsupported
 * @retval MCI_ERR_NOACCESS         manifest is not entitled to operate on
 *                                  non-memory platform component
 */
mci_err_t mci_validate_platform_specific_component_rights(const suit_manifest_class_id_t *class_id,
						    int platform_specific_component_number);

/**
 * @brief Verifies whether parent-child relationship for selected manifests is valid
 *
 * @param[in]   parent_class_id	Parent manifest class id
 * @param[in]   child_class_id	Child manifest class id
 *
 * @retval SUIT_PLAT_SUCCESS        on success
 * @retval SUIT_PLAT_ERR_INVAL      invalid parameter, i.e. null pointer
 * @retval MCI_ERR_MANIFESTCLASSID  manifest class id unsupported
 * @retval MCI_ERR_NOACCESS         parent-child relation for selected
 *                                  manifests is invalid
 */
mci_err_t mci_validate_manifest_parent_child(const suit_manifest_class_id_t *parent_class_id,
				       const suit_manifest_class_id_t *child_class_id);

/**
 * @brief Gets parent manifest class id for provided manifest class id
 *
 * @param[in]   child_class_id	Child manifest class id
 * @param[out]  parent_class_id	Parent manifest class id,
 *		NULL in case if input manifest does not have a parent
 *
 * @retval SUIT_PLAT_SUCCESS        on success
 * @retval SUIT_PLAT_ERR_INVAL      invalid parameter, i.e. null pointer
 * @retval MCI_ERR_MANIFESTCLASSID  manifest class id unsupported
 */
mci_err_t mci_get_manifest_parent(const suit_manifest_class_id_t *child_class_id,
			    const suit_manifest_class_id_t **parent_class_id);

/**
 * @brief Gets vendor id for provided manifest class id
 *
 * @param[in]   class_id      Component/manifest class id
 * @param[out]  vendor_id     Vendor id for the class id
 *
 * @retval SUIT_PLAT_SUCCESS        on success
 * @retval SUIT_PLAT_ERR_INVAL      invalid parameter, i.e. null pointer
 * @retval MCI_ERR_MANIFESTCLASSID  manifest class id unsupported
 */
mci_err_t mci_get_vendor_id_for_manifest_class_id(const suit_manifest_class_id_t *class_id,
					    const suit_uuid_t **vendor_id);

/**
 * @brief Checks if two suit_uuid_t structures hold the same uuid value
 *
 * @param[in]   uuid1, uuid2
 *
 * @retval SUIT_PLAT_SUCCESS         on success
 * @retval SUIT_PLAT_ERR_INVAL       invalid parameter, i.e. null pointer
 * @retval MCI_ERR_COMPARISON_FAILED content of UUIDs differs
 */
mci_err_t mci_compare_suit_uuid(const suit_uuid_t *uuid1, const suit_uuid_t *uuid2);

/**
 * @brief Initializes MCI
 *
 * @return SUIT_PLAT_SUCCESS
 *
 */
mci_err_t mci_init(void);

#ifdef __cplusplus
}
#endif

#endif /* SUIT_MCI_H__ */
