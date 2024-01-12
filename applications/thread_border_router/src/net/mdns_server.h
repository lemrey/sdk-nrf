/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief mDNS server
 */

#ifndef INCLUDE_MDNS_SERVER_
#define INCLUDE_MDNS_SERVER_

#include <zephyr/sys_clock.h>
#include <zephyr/net/net_ip.h>

/**
 * @brief mDNS records handle
 *
 * Handle used for accessing the record with API functions.
 */
struct mdns_record_handle;

/**
 * @brief Record iteration callback
 *
 * Function pointer to be provided as @ref mdns_records_for_each() argument. It
 * is invoked for each currently allocated record.
 */
typedef void (*mdns_record_cb_t)(struct mdns_record_handle *handle, void *user_data);

/** Supported mDNS record types */
enum mdns_record_type {
	MDNS_RECORD_TYPE_INVALID = 0,
	MDNS_RECORD_TYPE_A = 1,
	MDNS_RECORD_TYPE_NS = 2,
	MDNS_RECORD_TYPE_CNAME = 5,
	MDNS_RECORD_TYPE_PTR = 12,
	MDNS_RECORD_TYPE_TXT = 16,
	MDNS_RECORD_TYPE_AAAA = 28,
	MDNS_RECORD_TYPE_SRV = 33,
	MDNS_RECORD_TYPE_ANY = 255,
};

/**
 * @brief mDNS server config
 *
 * Used as a parameter for @ref mdns_server_listener_start(). It can be used to
 * activate mDNS server for multiple network interfaces. All instances share
 * the same list of allocated records.
 */
struct mdns_server_listener_config {
	/** Network interface */
	struct net_if *iface;

	/** Enables listener of IPv4 queries (address: 224.0.0.251) */
	bool setup_ipv4;

	/** Enables listener of IPv6 queries (address: ff02::fb)*/
	bool setup_ipv6;
};

/**
 * @brief Initialize mDNS server
 *
 * Prepare memory buffers, IP addresses etc.
 */
void mdns_server_init(void);

/**
 * @brief Start mDNS listener
 *
 * Enable listening for mDNS queries based on provided configuration.
 */
int mdns_server_listener_start(struct mdns_server_listener_config *config);

/**
 * @brief Stop mDNS listener
 *
 * Disable listening for an interface.
 */
int mdns_server_listener_stop(struct net_if *iface);

/**
 * @brief Iterate over active mDNS records
 *
 * Invoke a callback for each allocated mDNS record providing a record's handle.
 *
 * @note Function is thread safe as it locks the internal mutex at the beginning.
 *       Within the context of the callback any function that would lock the
 *       mutex (e.g., @ref mdns_record_get_name()) can be called with
 *       K_NO_WAIT parameter.
 *
 * @param callback Callback to be called for each active record
 * @param user_data User data supplied by a caller
 * @param timeout Time of waiting for the internal lock
 *
 * @retval >=0 is a number of processed records
 * @retval -EINVAL if @p callback is NULL
 * @retval -EBUSY if failed to acquire the lock
 */
int mdns_records_for_each(mdns_record_cb_t callback, void *user_data, k_timeout_t timeout);

/**
 * @brief Remove active mDNS record
 *
 * Remove the given mDNS record.
 *
 * @note Function is thread safe as it locks the internal mutex at the beginning.
 *
 * @param handle Record's handle
 * @param timeout Time of waiting for the internal lock
 *
 * @retval 0 on success
 * @retval -EINVAL if @p handle is invalid
 * @retval -EBUSY if failed to acquire the lock
 */
int mdns_record_remove(struct mdns_record_handle *handle, k_timeout_t timeout);

/**
 * @brief Get the name of a record
 *
 * Copy the record's domain name to a buffer.
 *
 * @note Function is thread safe as it locks the internal mutex at the beginning
 *
 * @param handle Record's handle
 * @param buffer Buffer to store the name
 * @param max_length Maximum number of bytes to be copied
 * @param timeout Time of waiting for the internal lock

 * @retval >0 is a number of read bytes
 * @retval -EINVAL if @p handle, @p buffer or @p max_length are invalid
 * @retval -EBUSY if failed to acquire the lock
 * @retval 0 on error while reading internal buffers
 */
int mdns_record_get_name(const struct mdns_record_handle *handle, char *buffer, size_t max_length,
			 k_timeout_t timeout);

/**
 * @brief Get the type of a record
 *
 * Copy the record's type.
 *
 * @note Function is thread safe as it locks the internal mutex at the beginning
 *
 * @param handle Record's handle
 * @param timeout Time of waiting for the internal lock
 * @param output Pointer to @ref mdns_record_type for storing the type
 *
 * @retval 0 on success
 * @retval -EINVAL if @p handle or @p output are invalid
 * @retval -EBUSY if failed to acquire the lock
 */
int mdns_record_get_type(const struct mdns_record_handle *handle, k_timeout_t timeout,
			 enum mdns_record_type *output);

/**
 * @brief Get the TTL of a record
 *
 * Copy the record's TTL.
 *
 * @note Function is thread safe as it locks the internal mutex at the beginning
 *
 * @param handle Record's handle
 * @param timeout Time of waiting for the internal lock
 * @param output Pointer for storing the TTL
 *
 * @retval 0 on success
 * @retval -EINVAL if @p handle or @p output are invalid
 * @retval -EBUSY if failed to acquire the lock
 */
int mdns_record_get_ttl(const struct mdns_record_handle *handle, k_timeout_t timeout,
			int32_t *output);

/**
 * @brief Link mDNS records
 *
 * While answering received questions mDNS server may provide more records that
 * were explicitly requested. These answers are added to "Additional RRs"
 * section of the response. This function adds the @p handle at the end of
 * additional record chain. Each record stores a single 'next' pointer so this
 * function should be used with care. Example chain: PTR->SRV->TXT->A<->AAAA.
 *
 * @note Function is thread safe as it locks the internal mutex at the beginning
 *
 * @param parent Target record's handle
 * @param handle Record's handle
 * @param timeout Time of waiting for the internal lock
 *
 * @retval 0 on success
 * @retval -EINVAL if @p parent or @p handle are invalid
 * @retval -EBUSY if failed to acquire the lock
 */
int mdns_link_records(struct mdns_record_handle *parent, const struct mdns_record_handle *handle,
		      k_timeout_t timeout);

/**
 * @brief Add A record
 *
 * Allocate a new A record and get its handle.
 *
 * @note Function is thread safe as it locks the internal mutex at the beginning
 * @note Function copies provided data to internal buffers
 *
 * @param name Record's name
 * @param name_len Record's name length
 * @param ttl TTL
 * @param address IPv4 address
 * @param timeout Time of waiting for the internal lock
 * @param output Pointer for storing allocated record's handle
 *
 * @retval 0 on success
 * @retval -EINVAL if any of provided parameter is invalid
 * @retval -EBUSY if failed to acquire the lock
 */
int mdns_record_add_a(const char *name, uint8_t name_len, int32_t ttl, struct in_addr *address,
		      k_timeout_t timeout, struct mdns_record_handle **output);

/**
 * @brief Add AAAA record
 *
 * Allocate a new AAAA record and get its handle.
 *
 * @note Function is thread safe as it locks the internal mutex at the beginning
 * @note Function copies provided data to internal buffers
 *
 * @param name Record's name
 * @param name_len Record's name length
 * @param ttl TTL
 * @param address IPv6 address
 * @param timeout Time of waiting for the internal lock
 * @param output Pointer for storing allocated record's handle
 *
 * @retval 0 on success
 * @retval -EINVAL if any of provided parameter is invalid
 * @retval -EBUSY if failed to acquire the lock
 */
int mdns_record_add_aaaa(const char *name, uint8_t name_len, int32_t ttl, struct in6_addr *address,
			 k_timeout_t timeout, struct mdns_record_handle **output);

/**
 * @brief Add NS record
 *
 * Allocate a new NS record and get its handle.
 *
 * @note Function is thread safe as it locks the internal mutex at the beginning
 * @note Function copies provided data to internal buffers
 *
 * @param name Record's name
 * @param name_len Record's name length
 * @param ttl TTL
 * @param ns_dname Name server's domain name
 * @param ns_dname_len Name server's domain name's length
 * @param timeout Time of waiting for the internal lock
 * @param output Pointer for storing allocated record's handle
 *
 * @retval 0 on success
 * @retval -EINVAL if any of provided parameter is invalid
 * @retval -EBUSY if failed to acquire the lock
 */
int mdns_record_add_ns(const char *name, uint8_t name_len, int32_t ttl, const char *ns_dname,
		       uint8_t ns_dname_len, k_timeout_t timeout,
		       struct mdns_record_handle **output);

/**
 * @brief Add CNAME record
 *
 * Allocate a new CNAME record and get its handle.
 *
 * @note Function is thread safe as it locks the internal mutex at the beginning
 * @note Function copies provided data to internal buffers
 *
 * @param name Record's name
 * @param name_len Record's name length
 * @param ttl TTL
 * @param cname Canonical name
 * @param cname_len Canonical name's length
 * @param timeout Time of waiting for the internal lock
 * @param output Pointer for storing allocated record's handle
 *
 * @retval 0 on success
 * @retval -EINVAL if any of provided parameter is invalid
 * @retval -EBUSY if failed to acquire the lock
 */
int mdns_record_add_cname(const char *name, uint8_t name_len, int32_t ttl, const char *cname,
			  uint8_t cname_len, k_timeout_t timeout,
			  struct mdns_record_handle **output);

/**
 * @brief Add PTR record
 *
 * Allocate a new PTR record and get its handle.
 *
 * @note Function is thread safe as it locks the internal mutex at the beginning
 * @note Function copies provided data to internal buffers
 *
 * @param name Record's name
 * @param name_len Record's name length
 * @param ttl TTL
 * @param ptr_dname Domain name
 * @param ptr_dname_len Domain name's length
 * @param timeout Time of waiting for the internal lock
 * @param output Pointer for storing allocated record's handle
 *
 * @retval 0 on success
 * @retval -EINVAL if any of provided parameter is invalid
 * @retval -EBUSY if failed to acquire the lock
 */
int mdns_record_add_ptr(const char *name, uint8_t name_len, int32_t ttl, const char *ptr_dname,
			uint8_t ptr_dname_len, k_timeout_t timeout,
			struct mdns_record_handle **output);

/**
 * @brief Add TXT record
 *
 * Allocate a new TXT record and get its handle.
 *
 * @note Function is thread safe as it locks the internal mutex at the beginning
 * @note Function copies provided data to internal buffers
 *
 * @param name Record's name
 * @param name_len Record's name length
 * @param ttl TTL
 * @param data Data buffer
 * @param data_length Data buffer's length
 * @param timeout Time of waiting for the internal lock
 * @param output Pointer for storing allocated record's handle
 *
 * @retval 0 on success
 * @retval -EINVAL if any of provided parameter is invalid
 * @retval -EBUSY if failed to acquire the lock
 */
int mdns_record_add_txt(const char *name, uint8_t name_len, int32_t ttl, const char *data,
			size_t data_length, k_timeout_t timeout,
			struct mdns_record_handle **output);

/**
 * @brief Add SRV record
 *
 * Allocate a new SRV record and get its handle.
 *
 * @note Function is thread safe as it locks the internal mutex at the beginning
 * @note Function copies provided data to internal buffers
 *
 * @param name Record's name
 * @param name_len Record's name length
 * @param ttl TTL
 * @param priority SRV priority
 * @param weight SRV weight
 * @param port SRV port
 * @param target SRV target
 * @param target_len SRV target's length
 * @param timeout Time of waiting for the internal lock
 * @param output Pointer for storing allocated record's handle
 *
 * @retval 0 on success
 * @retval -EINVAL if any of provided parameter is invalid
 * @retval -EBUSY if failed to acquire the lock
 */
int mdns_record_add_srv(const char *name, uint8_t name_len, int32_t ttl, uint16_t priority,
			uint16_t weight, uint16_t port, const char *target, size_t target_len,
			k_timeout_t timeout, struct mdns_record_handle **output);

/**
 * @brief Get A record's data
 *
 * Copy record's data to a provided buffer
 *
 * @note Function is thread safe as it locks the internal mutex at the beginning.
 *       When called from an already locked context (e.g. @ref mdns_records_for_each())
 *       the provided timeout can be set to K_NO_WAIT.
 *
 * @param handle Record's handle
 * @param address Buffer for IPv4 address
 * @param timeout Time of waiting for the internal lock
 *
 * @retval  >0 on success
 * @retval -EINVAL if any of provided parameter is invalid
 * @retval -EBUSY if failed to acquire the lock
 */
int mdns_record_get_rdata_a(const struct mdns_record_handle *handle, struct in_addr *address,
			    k_timeout_t timeout);

/**
 * @brief Get AAAA record's data
 *
 * Copy record's data to a provided buffer
 *
 * @note Function is thread safe as it locks the internal mutex at the beginning.
 *       When called from an already locked context (e.g. @ref mdns_records_for_each())
 *       the provided timeout can be set to K_NO_WAIT.
 *
 * @param handle Record's handle
 * @param address Buffer for IPv6 address
 * @param timeout Time of waiting for the internal lock
 *
 * @retval  >0 on success
 * @retval -EINVAL if any of provided parameter is invalid
 * @retval -EBUSY if failed to acquire the lock
 */
int mdns_record_get_rdata_aaaa(const struct mdns_record_handle *handle, struct in6_addr *address,
			       k_timeout_t timeout);

/**
 * @brief Get NS record's data
 *
 * Copy record's data to a provided buffer
 *
 * @note Function is thread safe as it locks the internal mutex at the beginning.
 *       When called from an already locked context (e.g. @ref mdns_records_for_each())
 *       the provided timeout can be set to K_NO_WAIT.
 *
 * @param handle Record's handle
 * @param nsdname Buffer for name server's domain
 * @param max_len Maximum number of bytes to copy
 * @param timeout Time of waiting for the internal lock
 *
 * @retval  >0 on success - number of bytes written to @p nsdname
 * @retval -EINVAL if any of provided parameter is invalid
 * @retval -EBUSY if failed to acquire the lock
 */
int mdns_record_get_rdata_ns(const struct mdns_record_handle *handle, char *nsdname, size_t max_len,
			     k_timeout_t timeout);
/**
 * @brief Get CNAME record's data
 *
 * Copy record's data to a provided buffer
 *
 * @note Function is thread safe as it locks the internal mutex at the beginning.
 *       When called from an already locked context (e.g. @ref mdns_records_for_each())
 *       the provided timeout can be set to K_NO_WAIT.
 *
 * @param handle Record's handle
 * @param cname Buffer for canonical name
 * @param max_len Maximum number of bytes to copy
 * @param timeout Time of waiting for the internal lock
 *
 * @retval  >0 on success - number of bytes written to @p cname
 * @retval -EINVAL if any of provided parameter is invalid
 * @retval -EBUSY if failed to acquire the lock
 */
int mdns_record_get_rdata_cname(const struct mdns_record_handle *handle, char *cname, size_t max_len,
				k_timeout_t timeout);

/**
 * @brief Get PTR record's data
 *
 * Copy record's data to a provided buffer
 *
 * @note Function is thread safe as it locks the internal mutex at the beginning.
 *       When called from an already locked context (e.g. @ref mdns_records_for_each())
 *       the provided timeout can be set to K_NO_WAIT.
 *
 * @param handle Record's handle
 * @param ptrdname Buffer for canonical name
 * @param max_len Maximum number of bytes to copy
 * @param timeout Time of waiting for the internal lock
 *
 * @retval  >0 on success - number of bytes written to @p ptrdname
 * @retval -EINVAL if any of provided parameter is invalid
 * @retval -EBUSY if failed to acquire the lock
 */
int mdns_record_get_rdata_ptr(const struct mdns_record_handle *handle, char *ptrdname, size_t max_len,
			      k_timeout_t timeout);

/**
 * @brief Get TXT record's data
 *
 * Copy record's data to a provided buffer
 *
 * @note Function is thread safe as it locks the internal mutex at the beginning.
 *       When called from an already locked context (e.g. @ref mdns_records_for_each())
 *       the provided timeout can be set to K_NO_WAIT.
 *
 * @param handle Record's handle
 * @param txt_data Buffer for TXT data
 * @param max_len Maximum number of bytes to copy
 * @param timeout Time of waiting for the internal lock
 *
 * @retval  >0 on success - number of bytes written to @p txt_data
 * @retval -EINVAL if any of provided parameter is invalid
 * @retval -EBUSY if failed to acquire the lock
 */
int mdns_record_get_rdata_txt(const struct mdns_record_handle *handle, uint8_t *txt_data, size_t max_len,
			      k_timeout_t timeout);

/**
 * @brief Get SRV record's data
 *
 * Copy record's data to a provided buffer
 *
 * @note Function is thread safe as it locks the internal mutex at the beginning.
 *       When called from an already locked context (e.g. @ref mdns_records_for_each())
 *       the provided timeout can be set to K_NO_WAIT.
 *
 * @param handle Record's handle
 * @param priority Pointer for storing the SRV priority
 * @param weight Pointer for storing the SRV weight
 * @param port Pointer for storing the SRV port
 * @param target Buffer for SRV target
 * @param max_len Maximum number of bytes to copy
 * @param timeout Time of waiting for the internal lock
 *
 * @retval  >0 on success - number of bytes written to @p target
 * @retval -EINVAL if any of provided parameter is invalid
 * @retval -EBUSY if failed to acquire the lock
 */
int mdns_record_get_rdata_srv(const struct mdns_record_handle *handle, uint16_t *priority,
			      uint16_t *weight, uint16_t *port, char *target, size_t max_len,
			      k_timeout_t timeout);

#endif
