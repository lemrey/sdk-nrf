/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "suit_plat_digest_cache.h"
#include <suit_platform_internal.h>

typedef struct
{
    struct zcbor_string component_id;
    struct zcbor_string digest;
} suit_plat_digest_cache_entry_t;

static bool m_unlocked;
static suit_plat_digest_cache_entry_t m_cache[CONFIG_SUIT_DIGEST_CACHE_SIZE];

/**
 * @brief Get the cache entry index for the given component id
 */
static int find_entry(const struct zcbor_string *component_id, suit_plat_digest_cache_entry_t **p_entry)
{
	size_t i;

	for (i = 0; i < CONFIG_SUIT_DIGEST_CACHE_SIZE; i++)
	{
		if (suit_compare_zcbor_strings(component_id, &m_cache[i].component_id))
		{
			*p_entry = &m_cache[i];
			return SUIT_SUCCESS;
		}
	}

	return SUIT_ERR_MISSING_COMPONENT;
}

void suit_plat_digest_cache_unlock(void)
{
	m_unlocked = true;
}

void suit_plat_digest_cache_lock(void)
{
	m_unlocked = false;
}

bool suit_plat_digest_cache_is_unlocked(void)
{
	return m_unlocked;
}

int suit_plat_digest_cache_add(struct zcbor_string *component_id, struct zcbor_string *digest)
{
	size_t i;

	if (!m_unlocked)
	{
		return SUIT_ERR_UNSUPPORTED_COMMAND;
	}

	if (component_id == NULL || digest == NULL)
	{
		return SUIT_ERR_UNSUPPORTED_PARAMETER;
	}

	for (i = 0; i < CONFIG_SUIT_DIGEST_CACHE_SIZE; i++)
	{
		if (m_cache[i].component_id.value == NULL
		    || suit_compare_zcbor_strings(component_id, &m_cache[i].component_id))
		{
			m_cache[i].component_id.len = component_id->len;
			m_cache[i].component_id.value = component_id->value;
			m_cache[i].digest.len = digest->len;
			m_cache[i].digest.value = digest->value;

			return SUIT_SUCCESS;
		}
	}

	return SUIT_ERR_OVERFLOW;
}

int suit_plat_digest_cache_remove(struct zcbor_string *component_id)
{
	suit_plat_digest_cache_entry_t *entry;

	if (!m_unlocked)
	{
		return SUIT_ERR_UNSUPPORTED_COMMAND;
	}

	if (find_entry(component_id, &entry) == SUIT_SUCCESS)
	{
		entry->component_id.len = 0;
		entry->component_id.value = NULL;
		entry->digest.len = 0;
		entry->digest.value = NULL;
	}

	return SUIT_SUCCESS;
}

int suit_plat_digest_cache_get(const struct zcbor_string *component_id,
			       const struct zcbor_string **digest_out)
{
	int ret;
	suit_plat_digest_cache_entry_t *entry;

	if (digest_out == NULL)
	{
		return SUIT_ERR_UNSUPPORTED_PARAMETER;
	}

	ret = find_entry(component_id, &entry);

	if (ret == SUIT_SUCCESS)
	{
		*digest_out = &entry->digest;
	}

	return ret;
}

int suit_plat_digest_cache_remove_all(void)
{
	size_t i;

	if (!m_unlocked)
	{
		return SUIT_ERR_UNSUPPORTED_COMMAND;
	}

	for (i = 0; i < CONFIG_SUIT_DIGEST_CACHE_SIZE; i++)
	{
		m_cache[i].component_id.len = 0;
		m_cache[i].component_id.value = NULL;
		m_cache[i].digest.len = 0;
		m_cache[i].digest.value = NULL;
	}

	return SUIT_SUCCESS;
}

int suit_plat_digest_cache_remove_by_handle(suit_component_t handle)
{
	struct zcbor_string *component_id;

	int ret = suit_plat_component_id_get(handle, &component_id);

	if (ret != SUIT_SUCCESS) {
		return ret;
	}

	return suit_plat_digest_cache_remove(component_id);
}
