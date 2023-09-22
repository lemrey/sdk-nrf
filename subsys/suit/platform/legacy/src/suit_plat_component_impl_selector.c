/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zcbor_decode.h>
#include <suit_platform_internal.h>
#include <suit_plat_decode_util.h>

#ifdef CONFIG_SUIT_COMPONENT_RAMBUF
/** Forward declaration of the RAM-based component implementation constructor. */
struct suit_component_impl *suit_component_rambuf_impl(void);
#elif CONFIG_SUIT_COMPONENT_MRAM
/** Forward declaration of the MRAM-based component implementation constructor. */
struct suit_component_impl *suit_component_mram_impl(void);
#endif

struct suit_component_impl *suit_component_envelope_reference_impl(void);

struct suit_component_impl *suit_component_select_impl(struct zcbor_string *component_id,
						       suit_component_t component_handle)
{
	suit_component_type_t component_type = SUIT_COMPONENT_TYPE_UNSUPPORTED;

	if (!suit_plat_decode_component_type(component_id, &component_type)) {
		return NULL;
	}

	/* Try loading the component using supported loaders. */
	switch (component_type) {
	case SUIT_COMPONENT_TYPE_MEM:
#ifdef CONFIG_SUIT_COMPONENT_RAMBUF
		return suit_component_rambuf_impl();
#elif CONFIG_SUIT_COMPONENT_MRAM
		return suit_component_mram_impl();
#else
#error "Configuration invalid no implementation selected for 'M'"
#endif
	case SUIT_COMPONENT_TYPE_CAND_IMG:
		return suit_component_envelope_reference_impl();
	default:
		break;
	}

	return NULL;
}
