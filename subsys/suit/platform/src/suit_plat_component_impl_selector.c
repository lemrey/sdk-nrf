/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zcbor_decode.h>
#include <suit_platform_internal.h>

#ifdef CONFIG_SUIT_COMPONENT_RAMBUF
/** Forward declaration of the RAM-based component implementation constructor. */
struct suit_component_impl *suit_component_rambuf_impl(void);
#elif CONFIG_SUIT_COMPONENT_MRAM
/** Forward declaration of the MRAM-based component implementation constructor. */
struct suit_component_impl *suit_component_mram_impl(void);
#endif

struct suit_component_impl *suit_component_envelope_reference_impl(void);

struct suit_component_impl *
suit_component_select_impl(struct zcbor_string *component_id,
			   struct zcbor_string *key_ids[SUIT_MAX_NUM_SIGNERS], size_t num_key_ids,
			   suit_component_t component_handle)
{
	ZCBOR_STATE_D(state, 1, component_id->value, component_id->len,
		      SUIT_MAX_NUM_COMPONENT_ID_PARTS);
	struct zcbor_string component_type;
	bool res;

	res = zcbor_list_start_decode(state);
	res = res && zcbor_bstr_decode(state, &component_type);
	if ((!res) || (component_type.len != 1)) {
		return NULL;
	}

	/* Try loading the component using supported loaders. */
	switch (component_type.value[0]) {
	case 'M':
#ifdef CONFIG_SUIT_COMPONENT_RAMBUF
		return suit_component_rambuf_impl();
#elif CONFIG_SUIT_COMPONENT_MRAM
		return suit_component_mram_impl();
#else
		#error "Configuration invalid no implementation selected for 'M'"
#endif
	case 'D':
		return suit_component_envelope_reference_impl();
	default:
		break;
	}

	return NULL;
}
