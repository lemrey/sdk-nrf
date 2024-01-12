/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief mDNS internals
 */

#include "mdns_internal.h"

#include <zephyr/kernel.h>
#include <zephyr/net/buf.h>
#include <zephyr/net/net_core.h>

LOG_MODULE_REGISTER(mdns_server, CONFIG_NRF_TBR_MDNS_SERVER_LOG_LEVEL);

#define MAX_RECORDS_NUM CONFIG_NRF_TBR_MDNS_SERVER_NUM_RECORDS
#define SLAB_ALIGN 4

#define GENERIC_DATA_BUFFER_COUNT CONFIG_NRF_TBR_MDNS_SERVER_DATA_BUFFER_COUNT
#define GENERIC_DATA_BUFFER_SIZE CONFIG_NRF_TBR_MDNS_SERVER_DATA_BUFFER_SIZE

#define MAX_LABEL_LEN 63
#define MIN_LABEL_LEN 1

#define DOMAIN_LOCAL_STR "local"
#define DOMAIN_LOCAL_STR_LEN 5
#define DOMAIN_ARPA_STR "arpa"
#define DOMAIN_ARPA_STR_LEN 4

K_MEM_SLAB_DEFINE_STATIC(records_slab, sizeof(struct mdns_record), MAX_RECORDS_NUM, SLAB_ALIGN);

NET_BUF_POOL_FIXED_DEFINE(generic_rdata, GENERIC_DATA_BUFFER_COUNT, GENERIC_DATA_BUFFER_SIZE, 0,
			  NULL);

static K_MUTEX_DEFINE(mdns_mutex);

static sys_slist_t records_list;

/* Workaround the problem where default allocator provides a size for the allocation
 * which fails because fixed buffer allocates less memory than expected.
 */
static struct net_buf *rdata_fragment_alloc(k_timeout_t timeout, void *user_data)
{
	ARG_UNUSED(timeout);
	ARG_UNUSED(user_data);

	return net_buf_alloc(&generic_rdata, K_NO_WAIT);
}

static bool domain_is_valid(const char *dname, uint8_t len)
{
	uint8_t label_len = 0;
	uint8_t last_label_pos = 0;

	for (int i = 0; i < len; ++i) {
		if (dname[i] == '\0') {
			break;
		}

		if (dname[i] == '.') {
			if (label_len > MAX_LABEL_LEN || label_len < MIN_LABEL_LEN) {
				return false;
			}

			label_len = 0;
			last_label_pos = i;
		} else {
			label_len++;
		}
	}

	return (label_len == DOMAIN_LOCAL_STR_LEN &&
	        strcmp(&dname[last_label_pos + 1], DOMAIN_LOCAL_STR) == 0) ||
	       (label_len == DOMAIN_ARPA_STR_LEN &&
	        strcmp(&dname[last_label_pos + 1], DOMAIN_ARPA_STR) == 0);
}

int free_mdns_record(struct mdns_record *record)
{
	if (!record) {
		return -EINVAL;
	}

	if (!sys_slist_find_and_remove(&records_list, &record->node)) {
		return -EINVAL;
	}

	if (record->rdata) {
		net_buf_unref(record->rdata);
	}

	if (record->name) {
		net_buf_unref(record->name);
	}

	memset(record, 0, sizeof(struct mdns_record));

	k_mem_slab_free(&records_slab, (void **)&record);

	return 0;
}

static inline size_t append_data(struct net_buf *buf, const void *data, uint8_t data_len)
{
	return net_buf_append_bytes(buf, data_len, data, K_NO_WAIT, rdata_fragment_alloc, NULL);
}

static inline size_t append_label(struct net_buf *buf, const char *label, uint8_t len)
{
	return append_data(buf, &len, sizeof(len)) + append_data(buf, label, len);
}

static size_t append_name(struct net_buf *buf, const char *name, size_t name_len)
{
	uint8_t label_len = 0;
	size_t written = 0;
	size_t res = 0;
	size_t pos;

	for (pos = 0; pos < name_len; pos++) {
		if (name[pos] == '.') {
			res = append_label(buf, &name[pos - label_len], label_len);
			if (res < label_len + LABEL_LEN_SIZE) {
				return 0;
			}

			label_len = 0;
			written += res;
			continue;
		}

		label_len++;
	}

	return written + append_label(buf, &name[pos - label_len], label_len);
}

void init_mdns_internals(void)
{
	struct mdns_record *record;

	/* Memory slab uses buffer's free memory to store pointers to next
	 * available block so we cannot simply clear the buffer. Also, we
	 * cannot use the name's pointer to verify if handle is valid.
	 */
	for (int i = 0; i < records_slab.num_blocks; ++i) {
		record = (struct mdns_record *)(records_slab.buffer +
					       (i * records_slab.block_size));
		record->name_len = 0;
	};

	sys_slist_init(&records_list);
}

int iterate_mdns_records(mdns_record_cb_t callback, void *user_data)
{
	struct mdns_record *current;
	struct mdns_record *next;
	int res = 0;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&records_list, current, next, node) {
		callback(TO_HANDLE(current), user_data);
		res++;
	}

	return res;
}

int lock_mdns_mutex(k_timeout_t timeout)
{
	return k_mutex_lock(&mdns_mutex, timeout);
}

int unlock_mdns_mutex(void)
{
	return k_mutex_unlock(&mdns_mutex);
}

bool is_record_handle_valid(const struct mdns_record_handle *handle)
{
	const size_t last_item_pos = records_slab.block_size * (records_slab.num_blocks - 1);
	const size_t handle_offset = (uint32_t)handle - (uint32_t)records_slab.buffer;

	/* Verify handle's address and that it's used */
	return (handle) && ((void *)handle >= (void *)records_slab.buffer) &&
	       ((void *)handle <= (void *)(records_slab.buffer + last_item_pos)) &&
	       ((handle_offset % records_slab.block_size) == 0) &&
	       (FROM_HANDLE(handle)->name_len);
}

struct mdns_record *alloc_mdns_record(const char* name, uint8_t name_len, int32_t ttl,
				      uint16_t type, const void *data, size_t data_len,
				      const char *dname, uint8_t dname_len)
{
	struct mdns_record *record = NULL;
	size_t written = 0;

	if (!domain_is_valid(name, name_len)) {
		LOG_DBG("Invalid record name");
		return NULL;
	}

	if (dname && dname_len && !domain_is_valid(dname, dname_len)) {
		LOG_DBG("Invalid rdata domain");
		return NULL;
	}

	if (k_mem_slab_alloc(&records_slab, (void **)&record, K_NO_WAIT) < 0) {
		LOG_DBG("Failed to allocate mDNS record handle");
		return NULL;
	}

	memset(record, 0, sizeof(struct mdns_record));

	record->name = net_buf_alloc(&generic_rdata, K_NO_WAIT);

	if (!record->name) {
		LOG_DBG("Failed to allocate the name buffer");
		goto fail;
	}

	written = append_name(record->name, name, name_len);
	if (written == 0) {
		goto fail;
	}

	record->name_len = written;
	record->ttl = ttl;
	record->type = type;

	if (data || dname) {
		record->rdata = net_buf_alloc(&generic_rdata, K_NO_WAIT);
		if (!record->rdata) {
			LOG_DBG("Failed to allocate the data buffer");
			goto fail;
		}
	}

	if (data && data_len && !append_data(record->rdata, data, data_len)) {
		goto fail;
	}

	if (dname && dname_len && !append_name(record->rdata, dname, dname_len)) {
		goto fail;
	}

	record->rdata_len = data_len + (dname_len ? dname_len + LABEL_LEN_SIZE : 0);

	sys_slist_append(&records_list, &record->node);

	return record;

fail:
	free_mdns_record(record);
	return NULL;
}
