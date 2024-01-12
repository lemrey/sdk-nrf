/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief mDNS server
 */

#include "mdns_server.h"
#include "mdns_internal.h"

#include <ctype.h>

#include <ipv4.h>
#include <ipv6.h>
#include <udp_internal.h>

#include <zephyr/logging/log.h>
#include <zephyr/net/igmp.h>
#include <zephyr/net/net_core.h>

LOG_MODULE_DECLARE(mdns_server, CONFIG_NRF_TBR_MDNS_SERVER_LOG_LEVEL);

#define MDNS_SERVER_LISTENERS_COUNT CONFIG_NRF_TBR_MDNS_SERVER_NUM_LISTENERS
#define MDNS_SERVER_PORT 5353
#define MDNS_NAME_MAX_LEN 255
#define MDNS_DATA_BUFFER_SIZE MDNS_NAME_MAX_LEN + 2

#define LABEL_POINTER_MARK 0xC0
#define RESPONSE_TYPE_BIT_POS 15
#define PKT_ALLOC_TIMEOUT_MS 100
#define LABEL_PTR_SIZE 2
#define ANSWER_METADATA_SIZE 10

#define COMPRESSION_ENTRIES 64

#define HDR_RESPONSE_FLAG (1 << 15)
#define HDR_OPCODE_MASK (0b1111 << 14)
#define HDR_AA_FLAG (1 << 10)
#define RECORD_CACHE_FLUSH_FLAG (1 << 15)

struct mdns_server_listener listeners[MDNS_SERVER_LISTENERS_COUNT];
static struct in6_addr mdns_server_ipv6_addr;
static struct in_addr mdns_server_ipv4_addr = {{{ 224, 0, 0, 251 }}};

static uint8_t data[MDNS_DATA_BUFFER_SIZE];

static struct compression_entry comp_entries[COMPRESSION_ENTRIES];
static size_t comp_entry_count;

static K_MUTEX_DEFINE(listeners_mutex);

/* Passing the 'void *' let us avoid unaligned access warning */
static inline void unaligned_swap_u16(void *val)
{
	UNALIGNED_PUT(ntohs(UNALIGNED_GET((uint16_t *)val)), (uint16_t *)val);
}

static struct mdns_server_listener *find_unused_listener(void)
{
	struct mdns_server_listener *result = NULL;

	for (int i = 0; i < MDNS_SERVER_LISTENERS_COUNT; ++i) {
		if (!listeners[i].iface) {
			result = &listeners[i];
			break;
		}
	}

	return result;
}

static void free_listener(struct mdns_server_listener *listener)
{
	if (listener->ipv4_handle) {
		net_udp_unregister(listener->ipv4_handle);
	}

	if (listener->ipv6_handle) {
		net_udp_unregister(listener->ipv6_handle);
	}

	listener->iface = NULL;
	listener->ipv4_handle = NULL;
	listener->ipv6_handle = NULL;
}

static int read_name_from_buf_locked(struct net_buf *buf, size_t offset, size_t len, char *dst,
				     k_timeout_t timeout)
{
	size_t res = 0;
	size_t bytes_read = 0;
	uint8_t label_len = 0;

	if (lock_mdns_mutex(timeout) != 0) {
		return -EBUSY;
	}

	while (bytes_read < len) {
		/* Read label length */
		if (net_buf_linearize(&label_len, LABEL_LEN_SIZE, buf, offset + bytes_read,
				      LABEL_LEN_SIZE) == 0) {
			goto fail;
		}

		bytes_read++;

		res = net_buf_linearize(&dst[bytes_read - LABEL_LEN_SIZE], label_len, buf, offset + bytes_read, label_len);
		if (res < label_len) {
			goto fail;
		}
		bytes_read += res;

		if (bytes_read < len - LABEL_LEN_SIZE) {
			dst[bytes_read - LABEL_LEN_SIZE] = '.';
		}
	}

	unlock_mdns_mutex();

	return bytes_read - LABEL_LEN_SIZE;
fail:
	unlock_mdns_mutex();
	return 0;
}

static int read_name_from_rdata(const struct mdns_record_handle *handle, char *name, size_t max_len,
				k_timeout_t timeout)
{
	struct mdns_record *record = FROM_HANDLE(handle);

	if (!handle || !is_record_handle_valid(handle) || !name || !max_len) {
		return -EINVAL;
	}

	return read_name_from_buf_locked(record->rdata, 0, MIN(record->rdata_len, max_len), name,
					 timeout);
}

/* Note: While called, packet cursor has to be set right after UDP header */
static bool read_header(struct net_pkt *pkt, struct mdns_msg_hdr *hdr)
{
	if (net_pkt_read(pkt, hdr, sizeof(*hdr)) < 0) {
		return false;
	}

	unaligned_swap_u16(&hdr->tid);
	unaligned_swap_u16(&hdr->flags);
	unaligned_swap_u16(&hdr->questions);
	unaligned_swap_u16(&hdr->answer_rrs);
	unaligned_swap_u16(&hdr->authority_rrs);
	unaligned_swap_u16(&hdr->additional_rrs);

	return true;
}

static bool write_header(struct net_pkt *pkt, struct mdns_msg_hdr *hdr)
{
	unaligned_swap_u16(&hdr->tid);
	unaligned_swap_u16(&hdr->flags);
	unaligned_swap_u16(&hdr->questions);
	unaligned_swap_u16(&hdr->answer_rrs);
	unaligned_swap_u16(&hdr->authority_rrs);
	unaligned_swap_u16(&hdr->additional_rrs);

	return net_pkt_write(pkt, hdr, sizeof(*hdr)) == 0;
}


static void finalize_mdns_message(struct net_pkt *pkt, struct mdns_msg_hdr *hdr)
{
	net_pkt_cursor_init(pkt);
	net_pkt_set_overwrite(pkt, true);

	/* Jump to mDNS message HDR */
	net_pkt_skip(pkt, net_pkt_family(pkt) == AF_INET ? NET_IPV4UDPH_LEN : NET_IPV6UDPH_LEN);
	write_header(pkt, hdr);
	net_pkt_set_overwrite(pkt, false);

	net_pkt_cursor_init(pkt);

	if (net_pkt_family(pkt) == AF_INET) {
		net_ipv4_finalize(pkt, IPPROTO_UDP);
	} else {
		net_ipv6_finalize(pkt, IPPROTO_UDP);
	}

	net_pkt_trim_buffer(pkt);
}

static struct net_pkt *create_mdns_message(struct net_if *iface, size_t size, sa_family_t family,
					   void *addr, uint16_t port)
{
	struct net_pkt *pkt;
	struct mdns_msg_hdr hdr = { 0 };

	pkt = net_pkt_alloc_with_buffer(iface, size + sizeof(struct mdns_msg_hdr), family,
					IPPROTO_UDP, K_MSEC(PKT_ALLOC_TIMEOUT_MS));

	if (!pkt) {
		goto fail;
	}

	if (family == AF_INET) {
		/* Set TTL according to the RFC */
		net_pkt_set_ipv4_ttl(pkt, 255);

		if (net_ipv4_create(pkt, net_if_ipv4_select_src_addr(iface, (struct in_addr *)addr),
				    (struct in_addr *)addr) < 0) {
			goto fail;
		}
	} else if (family == AF_INET6) {
		/* Set hop limit according to the RFC */
		net_pkt_set_ipv6_hop_limit(pkt, 255);

		if (net_ipv6_create(pkt, net_if_ipv6_get_ll(iface, NET_ADDR_ANY_STATE),
				 (struct in6_addr *)addr) < 0) {
			goto fail;
		}
	} else {
		__ASSERT(false, "Invalid family");
	}

	if (net_udp_create(pkt, htons(MDNS_SERVER_PORT), port) < 0) {
		goto fail;
	}

	/* Reserve zeroed bytes for the header - they'll be filled during finalization */
	net_pkt_write(pkt, &hdr, sizeof(hdr));

	return pkt;
fail:
	LOG_DBG("Failed to create mDNS response");

	if (pkt) {
		net_pkt_unref(pkt);
	}

	return NULL;
}

static inline bool is_comp_entry_matching(struct compression_entry *entry, uint8_t *data,
					  size_t to_compare, bool is_unicast)
{
	return entry->for_unicast == is_unicast && entry->data_len == to_compare &&
	       net_buf_data_match(entry->buffer, entry->buffer_offset, data,
				  to_compare) == to_compare;
}

static struct compression_entry *find_compression_entry(uint8_t *data, size_t to_compare,
							bool is_unicast)
{
	struct compression_entry *entry = NULL;

	for (int i = 0; i < comp_entry_count; ++i) {
		if (is_comp_entry_matching(&comp_entries[i], data, to_compare, is_unicast)) {
			entry = &comp_entries[i];
			break;
		}
	}

	return entry;
}

static void add_compression_entries(uint8_t *name, size_t labels_len, size_t total_len,
				    struct net_buf *buf, size_t buf_offset,
				    bool is_unicast, size_t pkt_offset)
{
	size_t stored = 0;
	struct compression_entry *entry;

	LOG_DBG("Adding compression entries from pkt offset: %u", pkt_offset);

	while (stored < labels_len) {
		if (comp_entry_count == COMPRESSION_ENTRIES) {
			return;
		}

		entry = &comp_entries[comp_entry_count];
		entry->for_unicast = is_unicast;
		entry->buffer = buf;
		entry->buffer_offset = buf_offset + stored;
		entry->data_len = total_len - stored;
		entry->pkt_offset = pkt_offset + buf_offset + stored;

		stored += name[stored] + LABEL_LEN_SIZE;
		comp_entry_count++;
	}
}

static size_t compress_name(struct net_buf *src, size_t src_len, size_t src_offset,
			    uint8_t *dst, size_t dst_len, bool is_unicast, size_t pkt_offset)
{
	struct compression_entry *entry = NULL;
	size_t dst_pos = 0;
	size_t to_compare;

	net_buf_linearize(dst, dst_len, src, src_offset, src_len);

	/* Consecutively try to find the longest match in compression entries by starting from the
	 * next label, e.g. for 'foo.bar.local' try with 'foo.bar.local', 'bar.local', and 'local'.
	 */
	while (dst_pos < src_len) {
		to_compare = src_len - dst_pos;

		entry = find_compression_entry(&dst[dst_pos], to_compare, is_unicast);
		if (entry) {
			LOG_DBG("Compressing name from pos: %u, pkt offset: %u", dst_pos, entry->pkt_offset);

			*((uint16_t *)&dst[dst_pos]) = htons(LABEL_POINTER_MARK << 8 |
							     entry->pkt_offset);
			dst_pos += LABEL_PTR_SIZE;

			break;
		}

		/* jump to the next label */
		dst_pos += dst[dst_pos] + LABEL_LEN_SIZE;
	}

	add_compression_entries(dst, entry ? dst_pos : src_len, src_len, src, src_offset,
				is_unicast, pkt_offset);

	if (!entry) {
		/* Name was not compressed - add name termination byte */
		dst[dst_pos++] = '\0';
	}

	return dst_pos;
}

static size_t get_current_label_offset(struct net_pkt *pkt)
{
	return net_pkt_get_current_offset(pkt) - (net_pkt_family(pkt) == AF_INET ?
						  			 NET_IPV4UDPH_LEN :
									 NET_IPV6UDPH_LEN);
}

static void append_response(struct mdns_record *record, struct net_pkt *pkt, bool is_unicast)
{
	size_t to_write = 0;
	size_t rdata_offset = 0;
	uint16_t final_data_len = 0;
	struct net_pkt_cursor rdata_len_curs;
	struct net_pkt_cursor answer_end_curs;
	struct srv_rdata *srv_data;
	const size_t max_payload_len = record->name_len + record->rdata_len + ANSWER_METADATA_SIZE;

	net_pkt_set_overwrite(pkt, false);

	NET_DBG("appending record: %p to the response", record);

	NET_DBG("pkt len before alloc: %u", net_pkt_get_len(pkt));

	/* As we do not know the number and final size of the payload allocate memory in the fly
	 * if the potential result will not fit into unused space. We could use a bigger `data`
	 * buffer but it could result in a lot of small buffer fragments after the compression.
	 */
	if (net_buf_tailroom(net_buf_frag_last(pkt->buffer)) < max_payload_len) {
		NET_DBG("allocating new buffer with len: %u", max_payload_len);
		net_pkt_append_buffer(pkt, net_buf_alloc_len(net_buf_pool_get(pkt->buffer->pool_id),
							     max_payload_len,
							     K_FOREVER));
	}

	NET_DBG("pkt len after alloc: %u", net_pkt_get_len(pkt));

	to_write = compress_name(record->name, record->name_len, 0, data, sizeof(data), is_unicast,
				 get_current_label_offset(pkt));

	net_pkt_write(pkt, data, to_write);

	net_pkt_write_be16(pkt, record->type);
	net_pkt_write_be16(pkt, RECORD_CLASS_IN | RECORD_CACHE_FLUSH_FLAG);
	net_pkt_write_be32(pkt, record->ttl);

	/* Record data length will be updated at the end */
	net_pkt_cursor_backup(pkt, &rdata_len_curs);
	net_pkt_write(pkt, (uint8_t *)&final_data_len, sizeof(final_data_len));

	if (record->type == MDNS_RECORD_TYPE_A || record->type == MDNS_RECORD_TYPE_AAAA ||
	    record->type == MDNS_RECORD_TYPE_TXT) {
		final_data_len = record->rdata_len;

		net_buf_linearize(data, sizeof(data), record->rdata, 0, final_data_len);
		net_pkt_write(pkt, data, record->rdata_len);

		/* A, AAAA, and TXT record has no other data to be written */
		goto finish;
	} else if (record->type == MDNS_RECORD_TYPE_SRV) {
		net_buf_linearize(data, sizeof(data), record->rdata, 0, sizeof(struct srv_rdata));

		srv_data = (struct srv_rdata *)data;

		net_pkt_write_be16(pkt, srv_data->priority);
		net_pkt_write_be16(pkt, srv_data->weight);
		net_pkt_write_be16(pkt, srv_data->port);

		rdata_offset = sizeof(struct srv_rdata);
		final_data_len = rdata_offset;
	}

	to_write = compress_name(record->rdata, record->rdata_len - rdata_offset, rdata_offset,
				 &data[rdata_offset], sizeof(data) - rdata_offset, is_unicast,
				 get_current_label_offset(pkt));

	final_data_len += to_write;

	net_pkt_write(pkt, &data[rdata_offset], to_write);

finish:
	/* Set data length after compression */
	net_pkt_cursor_backup(pkt, &answer_end_curs);
	net_pkt_cursor_restore(pkt, &rdata_len_curs);

	net_pkt_set_overwrite(pkt, true);
	net_pkt_write_be16(pkt, final_data_len);
	net_pkt_set_overwrite(pkt, false);

	/* Jump back at the answer's end so the cursor is prepared for the next message */
	net_pkt_cursor_restore(pkt, &answer_end_curs);

	if (is_unicast) {
		record->in_unicast_response = true;
	} else {
		record->in_mcast_response = true;
	}
}

static void process_matched(struct mdns_record *record, struct match_opts *opts)
{
	bool is_unicast;
	size_t payload_size;
	void *dst_addr = opts->dst_addr;
	struct mdns_response *resp;

	is_unicast = opts->flags & (1 << RESPONSE_TYPE_BIT_POS);
	resp = is_unicast ? &opts->unicast_resp : &opts->mcast_resp;

	if (resp->pkt == NULL) {
		if (!is_unicast) {
			dst_addr = (opts->family == AF_INET) ? (void *)&mdns_server_ipv4_addr :
							       (void *)&mdns_server_ipv6_addr;
		}

		payload_size = record->name_len + record->rdata_len;
		resp->pkt = create_mdns_message(opts->iface, payload_size, opts->family, dst_addr,
						opts->port);
	}

	if (resp->pkt && ((is_unicast && !record->in_unicast_response) ||
			  (!is_unicast && !record->in_mcast_response))) {
		append_response(record, resp->pkt, is_unicast);
		resp->header.answer_rrs++;
	}
}

static bool compare_lowercased(const struct net_buf *buf, uint8_t *data, size_t data_len)
{
	size_t pos = 0;
	size_t to_compare;

	__ASSERT(buf, "Record without a name buffer");

	while (buf && pos < data_len) {
		to_compare = MIN(data_len, buf->len);

		for (size_t i = 0; i < to_compare; ++i) {
			/* Question's chars are already in lowercase */
			if (data[pos++] != tolower(buf->data[i])) {
				return false;
			}
		}
		buf = buf->frags;
	}

	return true;
}

/* Callback function that compares a given record with a question */
static void match_record(struct mdns_record_handle *handle, void *user_data)
{
	struct mdns_record *record = FROM_HANDLE(handle);
	struct match_opts *opts = (struct match_opts*)user_data;

	if ((opts->type != MDNS_RECORD_TYPE_ANY && (record->type != opts->type)) ||
	     (record->name_len != opts->name_len)) {
		return;
	}

	if (compare_lowercased(record->name, opts->name, opts->name_len)) {
		process_matched(record, opts);
	}
}

/* Append all items from additional records chain that were not yet added to a given response */
static size_t append_additional_rrs(struct mdns_record *record, struct net_pkt *pkt,
				    bool is_unicast)
{
	size_t result = 0;

	while (record->next_add_rr) {
		record = record->next_add_rr;

		if ((is_unicast && !record->in_unicast_response) || (!is_unicast &&
								     !record->in_mcast_response)) {
			append_response(record, pkt, is_unicast);
			result++;
		}
	}

	return result;
}

/* Callback function for preparing record for matching */
static void prepare_records(struct mdns_record_handle *handle, void *user_data)
{
	ARG_UNUSED(user_data);

	struct mdns_record *record = FROM_HANDLE(handle);

	record->in_mcast_response = false;
	record->in_unicast_response = false;
}

/* Callback function for appending all additional records if needed */
static void process_additional_rrs(struct mdns_record_handle *handle, void *user_data)
{
	struct mdns_record *record = FROM_HANDLE(handle);
	struct match_opts *opts = (struct match_opts*)user_data;
	size_t appended;

	/* Check if there are additional records related to this one and if they
	 * are not yet present in the response - append them. Single list of records
	 * may be processed multiple times if a record added as an additional one
	 * few iterations ago will be hit again as its on the list of all records.
	 * However, items on its list will skipped so there is no side effect of this
	 * behavior.
	 */
	if (record->next_add_rr) {
		if (record->in_unicast_response) {
			appended = append_additional_rrs(record, opts->unicast_resp.pkt, true);
			opts->unicast_resp.header.additional_rrs += appended;
		}

		if (record->in_mcast_response) {
			appended = append_additional_rrs(record, opts->mcast_resp.pkt, false);
			opts->mcast_resp.header.additional_rrs += appended;
		}
	}
}

/**
 * This function processes a next question and appends the matched response to
 * unicast or multicast response packet (a given packet is allocated only when
 * needed).
 *
 * Requirement - the packet's cursor has to be set properly at the first label
 *               of a question to be processed.
 */
static bool process_question(struct net_pkt *pkt, struct net_pkt_cursor *qstart,
			     struct match_opts *opts)
{
	uint8_t name[MDNS_NAME_MAX_LEN];
	int name_pos = 0;
	uint8_t label_len = 0;
	uint16_t label_ptr = 0;
	struct net_pkt_cursor next_label_pos = { 0 };

	if (net_pkt_read_u8(pkt, &label_len) != 0) {
		return false;
	}

	/* In the memory, mDNS records' names are stored with labels prepended by their lengths,
	 * and as lowercases (mDNS is case insensitive), e.g. 'Foo.bAr.zepHyr.local' is kept in
	 * an internal buffer as '<3>foo<3>bar<6>zephyr<5>local'.
	 */
	while (label_len && name_pos < MDNS_NAME_MAX_LEN) {
		/* Check for a pointer to another label */
		if ((label_len & LABEL_POINTER_MARK) == LABEL_POINTER_MARK) {
			label_ptr = ((label_len & (~LABEL_POINTER_MARK)) << 8);

			if (net_pkt_read_u8(pkt, &label_len) != 0) {
				return false;
			}

			label_ptr += label_len;

			if (!next_label_pos.buf) {
				/* If it's a first pointer store the cursor to be restored later */
				net_pkt_cursor_backup(pkt, &next_label_pos);
			}

			net_pkt_cursor_restore(pkt, qstart);
			net_pkt_skip(pkt, label_ptr);
		} else {
			name[name_pos++] = label_len;

			if (net_pkt_read(pkt, &name[name_pos], label_len) < 0) {
				return false;
			}

			name_pos += label_len;
		}

		if (net_pkt_read_u8(pkt, &label_len) != 0) {
			return false;
		}
	}

	if (next_label_pos.buf) {
		net_pkt_cursor_restore(pkt, &next_label_pos);
	}

	if (net_pkt_read_be16(pkt, &opts->type) != 0) {
		return false;
	}

	if (net_pkt_read_be16(pkt, &opts->flags) != 0) {
		return false;
	}

	opts->name = name;
	opts->name_len = name_pos;

	/* At this point mutex is already locked */
	mdns_records_for_each(match_record, opts, K_NO_WAIT);

	return true;
}

static bool send_mdns_response(struct mdns_response *response)
{
	if (!response->pkt) {
		/* No need of sending the packet */
		return true;
	}

	finalize_mdns_message(response->pkt, &response->header);

	if (net_send_data(response->pkt) < 0) {
		net_pkt_unref(response->pkt);

		LOG_DBG("Failed to send mDNS response");

		return false;
	}

	return true;
}

static void init_mdns_header(struct mdns_msg_hdr *src, struct mdns_msg_hdr *dst,
			     bool is_mcast)
{
	/* Transaction ID should be provided only for unicast responses */
	dst->tid = is_mcast ? src->tid : 0;

	/* Copy OPCODE from the query, set response indication bit and
	 * Authoritative Answer bit. Note - at this point all fields of `src`
	 * header are already byte-swapped for reading and all fields of `dst`
	 * will be swapped during response finalization.
	 */
	dst->flags |= src->flags & HDR_OPCODE_MASK;
	dst->flags |= HDR_RESPONSE_FLAG;
	dst->flags |= HDR_AA_FLAG;
}

static enum net_verdict mdns_input(struct net_conn *conn, struct net_pkt *pkt,
				   union net_ip_header *ip_hdr, union net_proto_header *proto_hdr,
				   void *user_data)
{
	struct net_if *iface;
	struct mdns_msg_hdr header;
	struct net_pkt_cursor qstart;
	struct match_opts opts = { 0 };

	if (!conn) {
		LOG_ERR("Invalid connection");
		return NET_DROP;
	}

	if (!pkt) {
		LOG_ERR("Invalid packet");
		return NET_DROP;
	}

	iface = net_pkt_iface(pkt);
	if (!iface) {
		LOG_ERR("No interface");
		return NET_DROP;
	}

	/* Backup the cursor, so it can be used for name decompression */
	net_pkt_cursor_backup(pkt, &qstart);

	if (!read_header(pkt, &header)) {
		LOG_WRN("Failed to parse mDNS message header");
		return NET_DROP;
	}

	LOG_DBG("Received %u questions", header.questions);

	opts.family = pkt->family;
	opts.port = proto_hdr->udp->src_port;
	opts.iface = pkt->iface;
	opts.dst_addr = (opts.family == AF_INET) ? ip_hdr->ipv4->src : ip_hdr->ipv6->src;

	init_mdns_header(&header, &opts.unicast_resp.header, true);
	init_mdns_header(&header, &opts.mcast_resp.header, false);

	memset(comp_entries, 0, sizeof(comp_entries));
	comp_entry_count = 0;

	lock_mdns_mutex(K_FOREVER);

	iterate_mdns_records(prepare_records, NULL);

	for (int i = 0; i < header.questions; ++i) {
		process_question(pkt, &qstart, &opts);
	}

	if (opts.mcast_resp.header.answer_rrs || opts.unicast_resp.header.answer_rrs) {
		iterate_mdns_records(process_additional_rrs, &opts);
	}

	unlock_mdns_mutex();

	if (!send_mdns_response(&opts.unicast_resp) || !send_mdns_response(&opts.mcast_resp)) {
		return NET_DROP;
	}

	net_pkt_unref(pkt);

	return NET_OK;
}

static int get_rdata_addr_locked(const struct mdns_record_handle *handle, void *address,
				 size_t size, k_timeout_t timeout)
{
	int res;

	if (!handle || !is_record_handle_valid(handle) || !address) {
		return -EINVAL;
	}

	if (lock_mdns_mutex(timeout) != 0) {
		return -EBUSY;
	}

	res = net_buf_linearize(address, size, FROM_HANDLE(handle)->rdata, 0, size);

	unlock_mdns_mutex();

	return res;
}

void mdns_server_init(void)
{
	memset(&listeners, 0, sizeof(listeners));

	init_mdns_internals();

	net_ipv6_addr_create(&mdns_server_ipv6_addr,
			     0xff02, 0, 0, 0, 0, 0, 0, 0xfb);
}

int mdns_server_listener_start(struct mdns_server_listener_config *config)
{
	int ret;
	struct sockaddr local_addr_ipv4 = { 0 };
	struct sockaddr local_addr_ipv6 = { 0 };
	struct mdns_server_listener *listener;

	if (!config || !config->iface || (!config->setup_ipv4 && !config->setup_ipv6)) {
		return -EINVAL;
	}

	k_mutex_lock(&listeners_mutex, K_FOREVER);

	for (int i = 0; i < MDNS_SERVER_LISTENERS_COUNT; ++i) {
		if (config->iface == listeners[i].iface) {
			unlock_mdns_mutex();

			return -EALREADY;
		}
	}

	listener = find_unused_listener();

	if (!listener) {
		unlock_mdns_mutex();

		return -ENOMEM;
	}

	if (config->setup_ipv4) {
		LOG_DBG("Starting mDNS IPv4 listener for iface %p", config->iface);

		net_ipaddr_copy(&net_sin(&local_addr_ipv4)->sin_addr, &mdns_server_ipv4_addr);
		local_addr_ipv4.sa_family = AF_INET;

		ret = net_udp_register(AF_INET, NULL, &local_addr_ipv4, 0, MDNS_SERVER_PORT,
			       NULL, mdns_input, NULL, &listener->ipv4_handle);
		if (ret) {
			LOG_ERR("Failed to register mDNS IPv4 listener");
			goto fail;
		}

		ret = net_ipv4_igmp_join(config->iface, &mdns_server_ipv4_addr);
		if (ret) {
			LOG_ERR("Failed to join multicast group");
			goto fail;
		}
	}

	if (config->setup_ipv6) {
		LOG_DBG("Starting mDNS IPv6 listener for iface %p", config->iface);

		net_ipaddr_copy(&net_sin6(&local_addr_ipv6)->sin6_addr, &mdns_server_ipv6_addr);
		local_addr_ipv6.sa_family = AF_INET6;

		ret = net_udp_register(AF_INET6, NULL, &local_addr_ipv6, 0, MDNS_SERVER_PORT,
			       NULL, mdns_input, NULL, &listener->ipv6_handle);

		if (ret) {
			LOG_ERR("Failed to register mDNS IPv6 listener");
			goto fail;
		}

		ret = net_ipv6_mld_join(config->iface, &mdns_server_ipv6_addr);
		if (ret) {
			LOG_ERR("Failed to join multicast group");
			goto fail;
		}
	}

	listener->iface = config->iface;

	k_mutex_unlock(&listeners_mutex);

	return 0;
fail:
	free_listener(listener);
	k_mutex_unlock(&listeners_mutex);

	return -EFAULT;
}

int mdns_server_listener_stop(struct net_if *iface)
{
	if (!iface) {
		return -EINVAL;
	}

	k_mutex_lock(&listeners_mutex, K_FOREVER);

	for (int i = 0; i < MDNS_SERVER_LISTENERS_COUNT; ++i) {
		if (listeners->iface == listeners[i].iface) {
			free_listener(&listeners[i]);
			k_mutex_unlock(&listeners_mutex);

			return 0;
		}
	}

	k_mutex_unlock(&listeners_mutex);

	return -EALREADY;
}

static int alloc_locked(const char* name, uint8_t name_len, int32_t ttl, uint16_t type,
			const void *data, size_t data_len, const char *dname, uint8_t dname_len,
			k_timeout_t timeout, struct mdns_record_handle **handle)
{
	if (!handle || !name || (!data && !dname)) {
		return -EINVAL;
	}

	if (lock_mdns_mutex(timeout) != 0) {
		return -EBUSY;
	}

	*handle = TO_HANDLE(alloc_mdns_record(name, name_len, ttl, type, data, data_len, dname,
					      dname_len));

	unlock_mdns_mutex();

	return *handle ? 0 : -EFAULT;
}

int mdns_record_add_a(const char *name, uint8_t name_len, int32_t ttl, struct in_addr *address,
		      k_timeout_t timeout, struct mdns_record_handle **output)
{
	return alloc_locked(name, name_len, ttl, MDNS_RECORD_TYPE_A, address,
					   sizeof(*address), NULL, 0, timeout, output);
}

int mdns_record_add_aaaa(const char *name, uint8_t name_len, int32_t ttl, struct in6_addr *address,
			 k_timeout_t timeout, struct mdns_record_handle **output)
{
	return alloc_locked(name, name_len, ttl, MDNS_RECORD_TYPE_AAAA, address,sizeof(*address),
			    NULL, 0, timeout, output);
}

int mdns_record_add_ns(const char *name, uint8_t name_len, int32_t ttl, const char *ns_dname,
		       uint8_t ns_dname_len, k_timeout_t timeout,
		       struct mdns_record_handle **output)
{
	return alloc_locked(name, name_len, ttl, MDNS_RECORD_TYPE_NS, NULL, 0, ns_dname,
			    ns_dname_len, timeout, output);
}

int mdns_record_add_cname(const char *name, uint8_t name_len, int32_t ttl, const char *cname,
			  uint8_t cname_len, k_timeout_t timeout,
			  struct mdns_record_handle **output)
{
	return alloc_locked(name, name_len, ttl, MDNS_RECORD_TYPE_CNAME, NULL, 0, cname, cname_len,
			    timeout, output);
}

int mdns_record_add_ptr(const char *name, uint8_t name_len, int32_t ttl, const char *ptr_dname,
			uint8_t ptr_dname_len, k_timeout_t timeout,
			struct mdns_record_handle **output)
{
	return alloc_locked(name, name_len, ttl, MDNS_RECORD_TYPE_PTR, NULL, 0, ptr_dname,
			    ptr_dname_len, timeout, output);
}

int mdns_record_add_txt(const char *name, uint8_t name_len, int32_t ttl, const char *data,
			size_t data_length, k_timeout_t timeout, struct mdns_record_handle **output)
{
	return alloc_locked(name, name_len, ttl, MDNS_RECORD_TYPE_TXT, data, data_length, NULL, 0,
			    timeout, output);
}

int mdns_record_add_srv(const char *name, uint8_t name_len, int32_t ttl, uint16_t priority,
			uint16_t weight, uint16_t port, const char *target, size_t target_len,
			k_timeout_t timeout, struct mdns_record_handle **output)
{
	struct srv_rdata data;

	data.priority = priority;
	data.weight = weight;
	data.port = port;

	return alloc_locked(name, name_len, ttl, MDNS_RECORD_TYPE_SRV, &data, sizeof(data), target,
			    target_len, timeout, output);
}

int mdns_records_for_each(mdns_record_cb_t cb, void *user_data, k_timeout_t timeout)
{
	int res = 0;

	if (!cb) {
		return -EINVAL;
	}

	if (lock_mdns_mutex(timeout) != 0) {
		return -EBUSY;
	}

	res = iterate_mdns_records(cb, user_data);

	unlock_mdns_mutex();

	return res;
}

int mdns_record_remove(struct mdns_record_handle *handle, k_timeout_t timeout)
{
	int res;

	if (!handle || !is_record_handle_valid(handle)) {
		return -EINVAL;
	}

	if (lock_mdns_mutex(timeout) != 0) {
		return -EBUSY;
	}

	res = free_mdns_record(FROM_HANDLE(handle));

	unlock_mdns_mutex();

	return res;
}

int mdns_record_get_name(const struct mdns_record_handle *handle, char *buffer, size_t max_length,
			    k_timeout_t timeout)
{
	struct mdns_record *record = FROM_HANDLE(handle);

	if (!handle || !is_record_handle_valid(handle) || !buffer || !max_length) {
		return -EINVAL;
	}

	return read_name_from_buf_locked(record->name, 0, MIN(record->name_len, max_length),
					 buffer, timeout);
}

int mdns_record_get_type(const struct mdns_record_handle *handle, k_timeout_t timeout,
			 enum mdns_record_type *output)
{
	if (!handle || !is_record_handle_valid(handle) || !output) {
		return -EINVAL;
	}

	if (lock_mdns_mutex(timeout) != 0) {
		return -EBUSY;
	}

	*output = FROM_HANDLE(handle)->type;

	unlock_mdns_mutex();

	return 0;
}

int32_t mdns_record_get_ttl(const struct mdns_record_handle *handle, k_timeout_t timeout,
			    int32_t *output)
{
	if (!handle || !is_record_handle_valid(handle) || !output) {
		return -EINVAL;
	}

	if (lock_mdns_mutex(timeout) != 0) {
		return -EBUSY;
	}

	*output = FROM_HANDLE(handle)->ttl;

	unlock_mdns_mutex();

	return 0;
}

int mdns_record_get_rdata_a(const struct mdns_record_handle *handle, struct in_addr *address,
			    k_timeout_t timeout)
{
	return get_rdata_addr_locked(handle, (void *)address, sizeof(*address), timeout);
}

int mdns_record_get_rdata_aaaa(const struct mdns_record_handle *handle, struct in6_addr *address,
			       k_timeout_t timeout)
{
	return get_rdata_addr_locked(handle, (void *)address, sizeof(*address), timeout);
}

int mdns_record_get_rdata_ns(const struct mdns_record_handle *handle, char *nsdname, size_t max_len,
			     k_timeout_t timeout)
{
	return read_name_from_rdata(handle, nsdname, max_len, timeout);
}

int mdns_record_get_rdata_cname(const struct mdns_record_handle *handle, char *cname, size_t max_len,
				k_timeout_t timeout)
{
	return read_name_from_rdata(handle, cname, max_len, timeout);
}

int mdns_record_get_rdata_ptr(const struct mdns_record_handle *handle, char *ptrdname,
			      size_t max_len, k_timeout_t timeout)
{
	return read_name_from_rdata(handle, ptrdname, max_len, timeout);
}

int mdns_record_get_rdata_txt(const struct mdns_record_handle *handle, uint8_t *txt_data,
				 size_t max_len, k_timeout_t timeout)
{
	int res;

	if (!handle || !is_record_handle_valid(handle) || !txt_data) {
		return -EINVAL;
	}

	if (lock_mdns_mutex(timeout) != 0) {
		return -EBUSY;
	}

	res = net_buf_linearize(txt_data, max_len, FROM_HANDLE(handle)->rdata, 0,
				FROM_HANDLE(handle)->rdata_len);

	unlock_mdns_mutex();

	return res;
}

int mdns_record_get_rdata_srv(const struct mdns_record_handle *handle, uint16_t *priority,
			      uint16_t *weight, uint16_t *port, char *target, size_t max_len,
			      k_timeout_t timeout)
{
	struct mdns_record *record = FROM_HANDLE(handle);
	struct srv_rdata srv_data;
	size_t to_read;
	int res;

	if (!handle || !is_record_handle_valid(handle) || !priority || !weight || !port ||
	    !target || !max_len) {
		return -EINVAL;
	}

	if (lock_mdns_mutex(timeout) != 0) {
		return -EBUSY;
	}

	net_buf_linearize(&srv_data, sizeof(srv_data), record->rdata, 0, sizeof(srv_data));

	*priority = srv_data.priority;
	*weight = srv_data.weight;
	*port = srv_data.port;

	to_read = MIN(record->rdata_len - sizeof(srv_data), max_len);
	res = read_name_from_buf_locked(record->rdata, sizeof(srv_data), to_read, target,
					K_NO_WAIT);

	unlock_mdns_mutex();

	return res;
}

int mdns_link_records(struct mdns_record_handle *parent, const struct mdns_record_handle *handle,
		      k_timeout_t timeout)
{
	struct mdns_record *current = FROM_HANDLE(parent);

	if (!parent || !handle || !is_record_handle_valid(parent) ||
	    !is_record_handle_valid(handle)) {
		return -EINVAL;
	}

	if (lock_mdns_mutex(timeout) != 0) {
		return -EBUSY;
	}

	while (current->next_add_rr) {
		current = current->next_add_rr;
	}

	current->next_add_rr = FROM_HANDLE(handle);

	unlock_mdns_mutex();

	return 0;
}
