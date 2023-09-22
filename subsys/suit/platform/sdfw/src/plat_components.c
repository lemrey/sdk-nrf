/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_platform_internal.h>
#include <suit_plat_decode_util.h>
#include <suit_memptr_storage.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(plat_components, CONFIG_SUIT_LOG_LEVEL);

/** Platform component list, populated as a result of successful SUIT manifest validation. */
static struct suit_plat_component components[SUIT_MAX_NUM_COMPONENTS];

/** Resolve pointer to the component structure into component handle.
 */
static inline suit_component_t handle_from_component(const struct suit_plat_component *component)
{
	return (suit_component_t)component;
}

/** Resolve component handle into component structure.
 *
 * @details The component handle is considered as valid if its value
 *	    points to the component array element and the element has
 *	    an implementation assigned.
 */
static struct suit_plat_component *component_from_handle(suit_component_t handle)
{
	const suit_component_t first_component = (suit_component_t)(&components[0]);
	struct suit_plat_component *component = (struct suit_plat_component *)handle;

	if ((handle < (intptr_t)&components[0]) ||
	    (handle > (intptr_t)&components[SUIT_MAX_NUM_COMPONENTS - 1]) ||
	    ((handle - first_component) % (sizeof(struct suit_plat_component)) != 0) ||
	    (!component->in_use)) {
		return NULL;
	}

	return component;
}

int suit_plat_release_component_handle(suit_component_t handle)
{
	struct suit_plat_component *component = component_from_handle(handle);

	if (component == NULL) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	/* If component is of type D (or C ?) release attached memptr_record */
	suit_component_type_t component_type = SUIT_COMPONENT_TYPE_UNSUPPORTED;

	if (!suit_plat_decode_component_type(&component->component_id, &component_type)) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	if (component_type == SUIT_COMPONENT_TYPE_CAND_IMG) {
		int ret = release_memptr_storage(component->impl_data);

		if (ret != 0) {
			return ret;
		}

		component->impl_data = NULL;
	}

	component->in_use = false;

	return SUIT_SUCCESS;
}

int suit_plat_create_component_handle(struct zcbor_string *component_id,
				      suit_component_t *component_handle)
{
	struct suit_plat_component *component = NULL;

	for (size_t i = 0; i < SUIT_MAX_NUM_COMPONENTS; i++) {
		if (components[i].in_use == false) {
			component = &components[i];
			break;
		}
	}

	if (component == NULL) {
		LOG_ERR("component is NULL");
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	*component_handle = (intptr_t)component;
	memset(component, 0, sizeof(struct suit_plat_component));

	/* Store component ID and keys, so the selected implementation can use it in the future. */
	component->component_id.value = component_id->value;
	component->component_id.len = component_id->len;
	component->in_use = true;

	/* If component is of type D (or C ?) get and attach memptr_record */
	suit_component_type_t component_type = SUIT_COMPONENT_TYPE_UNSUPPORTED;

	if (!suit_plat_decode_component_type(component_id, &component_type)) {
		LOG_ERR("Error decoding component type");
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	if (component_type == SUIT_COMPONENT_TYPE_CAND_IMG) {
		return get_memptr_storage(&component->impl_data);
	}

	return SUIT_SUCCESS;
}

int suit_plat_component_impl_data_set(suit_component_t handle, void *impl_data)
{
	struct suit_plat_component *component = component_from_handle(handle);

	if (component == NULL) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	component->impl_data = impl_data;
	return SUIT_SUCCESS;
}

int suit_plat_component_impl_data_get(suit_component_t handle, void **impl_data)
{
	const struct suit_plat_component *component = component_from_handle(handle);

	if (component == NULL) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	*impl_data = component->impl_data;
	return SUIT_SUCCESS;
}

int suit_plat_component_id_get(suit_component_t handle, struct zcbor_string **component_id)
{
	struct suit_plat_component *component = component_from_handle(handle);

	if (component == NULL) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	*component_id = &(component->component_id);
	return SUIT_SUCCESS;
}
