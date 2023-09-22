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

typedef struct {
	uint8_t raw[16];
} suit_uuid_t;

typedef suit_uuid_t suit_manifest_class_id_t;

typedef enum {
	DOWNGRADE_PREVENTION_DISABLED = 0,
	DOWNGRADE_PREVENTION_ENABLED = 1
} downgrade_prevention_policy_t;

#define MCI_EINVAL	       200 /**< Invalid parameter value */
#define MCI_ESIZE	       201 /**< Invalid size */
#define MCI_EMANIFESTCLASSID   202 /**< Invalid or unsupported manifest class id */
#define MCI_ENOACCESS	       203 /**< Manifest is not entitled to operate on resource */
#define MCI_EWRONGKEYID	       204 /**< Provided key ID is invalid for desired operation */
#define MCI_ECOMPARISON_FAILED 205 /**< Content of compared suit_uuid_t structures differs */

/**
 * @brief Gets Nordic vendor id
 *
 * @param[out]  vendor_id     Nordic vendor id
 *
 * @return 0 on success
 *              -MCI_EINVAL invalid parameter, i.e. null pointer
 */
int mci_get_nordic_vendor_id(const suit_uuid_t **vendor_id);

/**
 * @brief Gets class id for components which are not associated to specific class id
 *
 * @param[out]  vendor_id     Nordic vendor id
 *
 * @return 0 on success
 *              -MCI_EINVAL invalid parameter, i.e. null pointer
 */
int mci_get_nordic_unspecified_class_id(const suit_uuid_t **class_id);

/**
 * @brief Gets an array of supported manifest class ids.
 *
 * @param[out]     class_id	Array of manifest class ids
 * @param[in,out]  size		as input - maximal amount of elements an array can hold,
 *				as output - amount of stored elements
 *
 * @return 0 on success
 *              -MCI_EINVAL invalid parameter, i.e. null pointer
 *              -MCI_ESIZE too small to store all information
 */
int mci_get_supported_manifest_class_ids(const suit_manifest_class_id_t **class_id, size_t *size);

/**
 * @brief Gets an array of manifest class ids in invocation order.
 *
 * @param[out]     class_id	Array of manifest class ids
 * @param[in,out]  size		as input - maximal amount of elements an array can hold,
 *				as output - amount of stored elements
 *
 * @return 0 on success
 *              -MCI_EINVAL invalid parameter, i.e. null pointer
 *              -MCI_ESIZE too small to store all information
 */
int mci_get_invoke_order(const suit_manifest_class_id_t **class_id, size_t *size);

/**
 * @brief Gets downgrade prevention policy for manifest class id
 *
 * @param[in]   class_id	Manifest class id
 * @param[out]  policy
 *
 * @return 0 on success
 *              -MCI_EINVAL invalid parameter, i.e. null pointer
 *              -MCI_EMANIFESTCLASSID manifest class id unsupported
 */
int mci_get_downgrade_prevention_policy(const suit_manifest_class_id_t *class_id,
					downgrade_prevention_policy_t *policy);

/**
 * @brief Validates if manifest class id is supported in the device.
 *
 * @param[in]  class_id	Manifest class id
 *
 * @return 0 on success
 *              -MCI_EINVAL invalid parameter, i.e. null pointer
 *              -MCI_EMANIFESTCLASSID manifest class id unsupported
 */
int mci_validate_manifest_class_id(const suit_manifest_class_id_t *class_id);

/**
 * @brief Verifying whether specific key_id is valid for signing/checking signature of specific
 *manifest class
 *
 * @param[in]   class_id	Manifest class id
 * @param[in]   key_id		Identifier of key utilized for manifest signing. key_id may be equal
 *				to 0. In that case function returns success if manifest class id
 *				does not require signing.
 *
 * @return 0 on success
 *              -MCI_EINVAL invalid parameter, i.e. null pointer
 *              -MCI_EMANIFESTCLASSID manifest class id unsupported
 *              -MCI_EWRONGKEYID provided key ID is invalid for signing for provided manifest class
 */
int mci_validate_signing_key_id(const suit_manifest_class_id_t *class_id, uint32_t key_id);

/**
 * @brief Verifies if manifest with specific class id is entitled to start (invoke) code on specific
 * processor
 *
 * @param[in]   class_id	Manifest class id
 * @param[in]   processor_id	Processor id. Refer to Product Specification
 *
 * @return 0 on success
 *              -MCI_EINVAL invalid parameter, i.e. null pointer
 *              -MCI_EMANIFESTCLASSID manifest class id unsupported
 *              -MCI_ENOACCESS manifest is not entitled to operate on cpu
 */
int mci_validate_processor_start_rights(const suit_manifest_class_id_t *class_id, int processor_id);

/**
 * @brief Verifies if manifest with specific class id is entitled to operate on memory range
 *
 * @param[in]   class_id	Manifest class id
 * @param[in]   address
 * @param[in]   mem_size	size of access area
 *
 * @return 0 on success
 *              -MCI_EINVAL invalid parameter, i.e. null pointer
 *              -MCI_EMANIFESTCLASSID manifest class id unsupported
 *              -MCI_ENOACCESS manifest is not entitled to operate on memory range
 */
int mci_validate_memory_access_rights(const suit_manifest_class_id_t *class_id, void *address,
				      size_t mem_size);

/**
 * @brief Verifies if manifest with specific class id is entitled to operate on non-memory platform
 * component
 *
 * @param[in]   class_id	Manifest class id
 * @param[in]   platform_specific_component_number
 *
 * @return 0 on success
 *              -MCI_EINVAL invalid parameter, i.e. null pointer
 *              -MCI_EMANIFESTCLASSID manifest class id unsupported
 *              -MCI_ENOACCESS manifest is not entitled to operate on non-memory platform component
 */
int mci_validate_platform_specific_component_rights(const suit_manifest_class_id_t *class_id,
						    int platform_specific_component_number);

/**
 * @brief Verifies whether parent-child relationship for selected manifests is valid
 *
 * @param[in]   parent_class_id	Parent manifest class id
 * @param[in]   child_class_id	Child manifest class id
 *
 * @return 0 on success
 *              -MCI_EINVAL invalid parameter, i.e. null pointer
 *              -MCI_EMANIFESTCLASSID manifest class id unsupported
 *              -MCI_ENOACCESS parent-child relation for selected manifests is invalid
 */
int mci_validate_manifest_parent_child(const suit_manifest_class_id_t *parent_class_id,
				       const suit_manifest_class_id_t *child_class_id);

/**
 * @brief Gets parent manifest class id for provided manifest class id
 *
 * @param[in]   child_class_id	Child manifest class id
 * @param[out]  parent_class_id	Parent manifest class id,
 *		NULL in case if input manifest does not have a parent
 *
 * @return 0 on success,
 *              -MCI_EINVAL invalid parameter, i.e. null pointer
 *              -MCI_EMANIFESTCLASSID child manifest class id unsupported
 */
int mci_get_manifest_parent(const suit_manifest_class_id_t *child_class_id,
			    const suit_manifest_class_id_t **parent_class_id);

/**
 * @brief Gets vendor id for provided manifest class id
 *
 * @param[in]   class_id      Component/manifest class id
 * @param[out]  vendor_id     Vendor id for the class id
 *
 * @return 0 on success
 *              -MCI_EINVAL invalid parameter, i.e. null pointer
 *              -MCI_EMANIFESTCLASSID manifest class id unsupported
 */
int mci_get_vendor_id_for_manifest_class_id(const suit_manifest_class_id_t *class_id,
					    const suit_uuid_t **vendor_id);

/**
 * @brief Checks if two suit_uuid_t structures hold the same uuid value
 *
 * @param[in]   uuid1, uuid2
 *
 * @return 0 on success
 *              -MCI_EINVAL invalid parameter, i.e. null pointer
 *              -MCI_ECOMPARISON_FAILED manifest class id unsupported
 */
int mci_compare_suit_uuid(const suit_uuid_t *uuid1, const suit_uuid_t *uuid2);

/**
 * @brief Initializes MCI
 *
 * @return 0 on success, -MCI_EINVAL otherwise
 *
 */
int mci_init(void);

#endif /* SUIT_MCI_H__ */
