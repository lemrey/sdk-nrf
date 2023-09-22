/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUIT_PLATFORM_INTERNAL_H__
#define SUIT_PLATFORM_INTERNAL_H__

#include <stdint.h>
#include <stdbool.h>
#include <suit_types.h>

/** @brief SUIT component type.
 *
 * @details To provide a better flexibility in the component ID definitions,
 *          it is advised to use the decoding function and the type defined below
 *          to evaluate which type of the component is being processed.
 */
typedef enum {
	/** Reserved value, indicating that component type is not defined by any of the known
	 *  values.
	 */
	SUIT_COMPONENT_TYPE_UNSUPPORTED,

	/** MCU memory-mapped slot (MRAM, RAM, external flash). */
	SUIT_COMPONENT_TYPE_MEM,

	/** Reference to dependency candidate envelope in candidate envelope or in the download
	 *  cache.
	 */
	SUIT_COMPONENT_TYPE_CAND_MFST,

	/** Reference to integrated payload in candidate envelope or payload in the download cache.
	 */
	SUIT_COMPONENT_TYPE_CAND_IMG,

	/** Installed envelope, holding severed manifest and its authentication block. */
	SUIT_COMPONENT_TYPE_INSTLD_MFST,

	/** SOC-Specific component. Necessary in case if installation to or reading from this
	 *  component type goes beyond 'memcpy-like' operations.
	 */
	SUIT_COMPONENT_TYPE_SOC_SPEC,

	/** SUIT Cache pool, where images downloaded during 'payload-fetch' sequence execution can
	 * be stored, ready for installation.
	 */
	SUIT_COMPONENT_TYPE_CACHE_POOL,
} suit_component_type_t;

/** Initialize the component implementation and associate it with the component handle. */
typedef int (*suit_component_init_t)(suit_component_t handle);

/** Release the given component from the component implementation. */
typedef void (*suit_component_release_t)(suit_component_t handle);

/** Read the contents of an active slot */
typedef int (*suit_component_read_t)(suit_component_t handle, size_t offset, uint8_t *buf,
				     size_t *len);

/** If component supports slots, write contents into the inactive slot. Overwrite the active slot
 * otherwise. */
typedef int (*suit_component_write_t)(suit_component_t handle, size_t offset, uint8_t *buf,
				      size_t len);

/** Start executing the code stored inside the component. */
typedef int (*suit_component_invoke_t)(suit_component_t handle, struct zcbor_string *invoke_args);

/** Get the address for direct read operations and return the number of bytes available. */
typedef size_t (*suit_component_read_address_t)(suit_component_t handle, uint8_t **read_address);

/** Check if a vendor/class/device ID matches with the component. */
typedef int (*suit_component_check_uuid_t)(suit_component_t handle, struct zcbor_string *did_uuid);

/** Check if the slot number matches with the active slot. */
typedef int (*suit_component_check_slot_t)(suit_component_t handle, unsigned int slot);

/** Fetch the given integrated payload into the component. */
typedef int (*suit_component_fetch_integrated_t)(suit_component_t handle,
						 struct zcbor_string *payload);

/** Structure that holds the component implementation functions.
 *
 * @note If component does not support certain functions, it should set
 *       the corresponding field to NULL.
 */
struct suit_component_impl {
	suit_component_init_t init;
	suit_component_release_t release;

	suit_component_read_t read;
	suit_component_write_t write;
	suit_component_invoke_t invoke;
	suit_component_read_address_t read_address;

	suit_component_check_uuid_t check_vid;
	suit_component_check_uuid_t check_cid;
	suit_component_check_uuid_t check_did;
	suit_component_check_slot_t check_slot;
	suit_component_fetch_integrated_t fetch_integrated;
};

struct suit_plat_component {
	struct suit_component_impl *
		impl; ///! The pointer to the implementation to use to interface with the component.
	void *impl_data; ///! Slot to store the implementation-specific private data.
	struct zcbor_string component_id; ///! Slot to store the component ID. The implementation
					  /// may use it instead of building full internal context.
	bool in_use;
};

/** Return the implementation, selected to handle the component. */
const struct suit_component_impl *suit_plat_component_impl_get(suit_component_t handle);

/** Set the pointer to the implementation-specific data. */
int suit_plat_component_impl_data_set(suit_component_t handle, void *impl_data);

/** Return the pointer to the implementation-specific data. */
int suit_plat_component_impl_data_get(suit_component_t handle, void **impl_data);

/** Return the full component ID associated with the component. */
int suit_plat_component_id_get(suit_component_t handle, struct zcbor_string **component_id);

/** Select the component implementation, based on component ID.
 *
 *  @param[in]  component_id  The CBOR-encoded component identifier.
 *  @param[in]  component_handle  Assigned component reference for use with other functions.
 */
struct suit_component_impl *suit_component_select_impl(struct zcbor_string *component_id,
						       suit_component_t component_handle);

/**
 * @brief Returns set of implementations specific for MRAM
 *
 * @return struct suit_component_impl*
 */
struct suit_component_impl *suit_component_mram_impl(void);

#endif /* SUIT_PLATFORM_INTERNAL_H__ */
