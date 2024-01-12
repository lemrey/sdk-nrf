/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief mDNS internal structures
 */

#ifndef INCLUDE_MDNS_INTERNAL_
#define INCLUDE_MDNS_INTERNAL_

#include "mdns_server.h"
#include <zephyr/net/net_ip.h>
#include <zephyr/sys/slist.h>

#define FROM_HANDLE(record) ((struct mdns_record *)record)
#define TO_HANDLE(record) ((struct mdns_record_handle *)record)

#define RECORD_CLASS_IN 1

#define LABEL_LEN_SIZE 1

/** Internal structure for keeping active mDNS listener info */
struct mdns_server_listener {
	/** Interface listening on */
	struct net_if *iface;

	/** IPv4 connection handler */
	struct net_conn_handle *ipv4_handle;

	/** IPv6 connection handler */
	struct net_conn_handle *ipv6_handle;
};

/**
 * Internal mDNS record structure not directly accessible by a user.
 * As it is used in memory slabs it must be allocated to word's boundary.
 */
struct mdns_record {
	/** Fixed network buffer for hostname */
	struct net_buf *name;

	/** Name length */
	uint8_t name_len;

	/** Type */
	uint16_t type;

	/** Time To Live */
	int32_t ttl;

	/** Fixed network buffer for record data */
	struct net_buf *rdata;

	/** Record data length */
	uint16_t rdata_len;

	/** Indicates if the record is already in a unicast response */
	bool in_unicast_response;

	/** Indicates if the record is already in a multicast response */
	bool in_mcast_response;

	/** Next record to be added to "Addtional RRs" section */
	struct mdns_record *next_add_rr;

	/** Linked list node */
	sys_snode_t node;
} __aligned(4);

/** mDNS message header */
struct mdns_msg_hdr {
	/** Transaction ID */
	uint16_t tid;

	/** Flags */
	uint16_t flags;

	/** Number of question */
	uint16_t questions;

	/** Number of answers */
	uint16_t answer_rrs;

	/** Number of authoritative nameservers' records */
	uint16_t authority_rrs;

	/** Number of additional records */
	uint16_t additional_rrs;
} __packed;

/** Helper internal structure with info for unicast/mcast response */
struct mdns_response {
	struct net_pkt *pkt;
	struct mdns_msg_hdr header;
};

/** Information used for matching mDNS records */
struct match_opts {
	/** Queried record type */
	uint16_t type;

	/** Queried record name */
	uint8_t *name;

	/** Queried name's length */
	uint16_t name_len;

	/** Query's flags */
	uint16_t flags;

	/** Unicast destination address for a response */
	uint8_t *dst_addr;

	/** Interface to use for packet transmission */
	struct net_if *iface;

	/** IPv4/IPv6 */
	sa_family_t family;

	/** Destination port */
	uint16_t port;

	/** Unicast response info */
	struct mdns_response unicast_resp;

	/** Multicast response info */
	struct mdns_response mcast_resp;
};

/** Internal structure for name compression */
struct compression_entry {
	/** Buffer containing a record's name or name in record's data */
	struct net_buf *buffer;

	/** Offset of a first label within the buffer */
	size_t buffer_offset;

	/** Number of bytes that can be skipped */
	size_t data_len;

	/** Offset of labels within the packet */
	uint16_t pkt_offset;

	/** Flag indicating if the entry is used for unicast response */
	bool for_unicast;
};

/** Internal structure with SRV record data */
struct srv_rdata {
	/** SRV priority */
	uint16_t priority;

	/** SRV weight */
	uint16_t weight;

	/** SRV port */
	uint16_t port;
} __packed;

/**
 * @brief Initialize mDNS internals
 *
 * Initialize memory buffers, lists, etc.
 */
void init_mdns_internals(void);

/**
 * @brief Allocate new mDNS record
 *
 * Allocate a new record and all data buffers nedded for storage.
 *
 * @note Function is not thread safe by itself. The caller has to first acquire
 *       the internal lock with @ref lock_mdns_mutex() and free it with
 *       @ref unlock_mdns_mutex().
 *
 * @param name Name
 * @param name_len Name length
 * @param ttl TTL
 * @param type Type
 * @param data Data stored at the start of record's data buffer (can be NULL)
 * @param data_len Data's length (can be 0)
 * @param dname Domain name stored in the buffer (can be NULL)
 * @param dname_len Domain name's length (can be 0)
 *
 * @returns pointer to the new record or NULL on failure
 */
struct mdns_record *alloc_mdns_record(const char* name, uint8_t name_len, int32_t ttl,
				      uint16_t type, const void *data, size_t data_len,
				      const char *dname, uint8_t dname_len);

/**
 * @brief Iterate over mDNS recods
 *
 * Invoke a callback for each allocated mDNS record providing a record's handle.
 *
 * @note Function is not thread safe by itself. The caller has to first acquire
 *       the internal lock with @ref lock_mdns_mutex() and free it with
 *       @ref unlock_mdns_mutex().
 *
 * @param callback Callback to be called for each active record
 * @param user_data User data supplied by a caller
 *
 * @returns number of iterated records
 */
int iterate_mdns_records(mdns_record_cb_t callback, void *user_data);

/**
 * @brief Free mDNS record
 *
 * @note Function is not thread safe by itself. The caller has to first acquire
 *       the internal lock with @ref lock_mdns_mutex() and free it with
 *       @ref unlock_mdns_mutex().
 *
 * @param record record to be freed
 *
 * @returns 0 if succeeded, -EINVAL if @p record is NULL or not found
 */
int free_mdns_record(struct mdns_record *record);

/**
 * @brief Lock internal mutex
 *
 * Ensure exclusive access to mDNS records. The lock and unlock have to be used
 * explicitly as there are several use cases where a caller may want to hold the
 * lock for some time, e.g. to add multiple records at once or to access them
 * several times without waiting again.
 *
 * @note This function can be safely called multiple times by the same thread
 *       without locking. However, each lock must have a matching call of
 *       @ref unlock_mdns_mutex().
 *
 * @param timeout Time of waiting for the mutex
 *
 * @returns 0 if succeeded, -EBUSY on timeout
 */
int lock_mdns_mutex(k_timeout_t timeout);

/**
 * @brief Unlock internal mutex
 *
 * Releases exclusive access to mDNS records.
 *
 * @note Each call of this function must be preceded with corresponding call
 *       of @ref lock_mdns_mutex().
 */
int unlock_mdns_mutex(void);


/**
 * @brief Verifies if the record's handle is valid
 *
 * This function verifies if the provided handle is valid without iterating
 * through the list of records.
 *
 * @note Function is not thread safe by itself. The caller has to first acquire
 *       the internal lock with @ref lock_mdns_mutex() and free it with
 *       @ref unlock_mdns_mutex().
 */
bool is_record_handle_valid(const struct mdns_record_handle *handle);

#endif
