/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief nRF Thread Border Router's main function
 */

#include <string.h>

#include <openthread/border_agent.h>
#include <openthread/border_routing.h>
#include <openthread/backbone_router_ftd.h>
#include <openthread/thread.h>

#include <zephyr/net/net_config.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/openthread.h>
#include <zephyr/sys/byteorder.h>

#include "backbone_agent.h"
#include "mdns_publisher.h"

LOG_MODULE_DECLARE(nrf_tbr, CONFIG_NRF_TBR_LOG_LEVEL);

#define UPDATE_MESHCOP_FLAGS OT_CHANGED_THREAD_ROLE \
	| OT_CHANGED_THREAD_EXT_PANID		    \
	| OT_CHANGED_THREAD_NETWORK_NAME	    \
	| OT_CHANGED_THREAD_BACKBONE_ROUTER_STATE   \
	| OT_CHANGED_THREAD_NETDATA

#define BA_VENDOR_NAME  CONFIG_NRF_TBR_VENDOR_NAME
#define BA_PRODUCT_NAME CONFIG_NRF_TBR_PRODUCT_NAME
#define BA_SERVICE      "_meshcop"
#define BA_PROTO        "_udp"
#define BA_DOMAIN       "local"
#define BA_DUMMY_PORT   0

#define BA_INSTANCE_NAME BA_VENDOR_NAME "-" BA_PRODUCT_NAME

#define OT_THREAD_VERSION_1_1 2
#define OT_THREAD_VERSION_1_2 3
#define OT_THREAD_VERSION_1_3 4

#define INVALID_FD -1

#define OT_INSTANCE openthread_get_default_instance()

static int service_fd;
static struct mdns_publisher_rec *service_rec;

typedef int (*value_encoder)(char *, size_t);

enum {
	CONNECTION_MODE_DISABLED        = 0,
	CONNECTION_MODE_PSKC            = 1,
	CONNECTION_MODE_PSKD            = 2,
	CONNECTION_MODE_VENDOR          = 3,
	CONNECTION_MODE_X509            = 4,
};

enum {
	INTERFACE_STATIS_NOT_INITIALIZED        = 0,
	INTERFACE_STATIS_INITIALIZED            = 1,
	INTERFACE_STATIS_ACTIVE                 = 2,
};

enum {
	AVAILABILITY_INFREQUENT = 0,
	AVAILABILITY_HIGH       = 1
};

static void close_serv_socket()
{
	if (service_fd != INVALID_FD) {
		close(service_fd);
		service_fd = INVALID_FD;
	}
};

static void handle_port_update()
{
	static struct sockaddr server_addr;
	char addrstr[INET_ADDRSTRLEN];
	int r;

	net_sin(&server_addr)->sin_family = AF_INET;
	net_sin(&server_addr)->sin_addr.s_addr = INADDR_ANY;
	net_sin(&server_addr)->sin_port = service_rec->port;

	r = socket(server_addr.sa_family, SOCK_DGRAM, IPPROTO_UDP);

	if (r == -1) {
		LOG_DBG("socket() failed (%s)", strerror(errno));
		return;
	}

	service_fd = r;

	r = bind(service_fd, &server_addr, sizeof(server_addr));

	if (r == -1) {
		LOG_DBG("bind() failed (%s)", strerror(errno));
		close(service_fd);
		return;
	}

	service_rec->port = net_sin(&server_addr)->sin_port;

	inet_ntop(server_addr.sa_family, &net_sin(&server_addr)->sin_addr,
		  addrstr, sizeof(addrstr));
	LOG_DBG("bound to [%s]:%u",
		addrstr, ntohs(net_sin(&server_addr)->sin_port));
}

static int encode_value(char *buffer, size_t max, const char *key,
			size_t key_len, const char *value, size_t value_len)
{
	int encoded_bytes = 0;
	uint8_t to_encode = key_len + value_len + 1;

	/* len + key + '=' + value */
	if (to_encode + 1 > max) {
		return -ENOBUFS;
	}

	memcpy(&buffer[encoded_bytes], &to_encode, sizeof(to_encode));
	encoded_bytes += sizeof(to_encode);

	memcpy(&buffer[encoded_bytes], key, key_len);
	encoded_bytes += key_len;

	memcpy(&buffer[encoded_bytes], "=", 1);
	encoded_bytes += 1;

	memcpy(&buffer[encoded_bytes], value, value_len);
	encoded_bytes += value_len;

	return encoded_bytes;
}

static int encode_vendor_name(char *buffer, size_t max)
{
	return encode_value(buffer, max, "vn", sizeof("vn") - 1, BA_VENDOR_NAME,
			    sizeof(BA_VENDOR_NAME) - 1);
}

static int encode_product_name(char *buffer, size_t max)
{
	return encode_value(buffer, max, "mn", sizeof("mn") - 1, BA_PRODUCT_NAME,
			    sizeof(BA_PRODUCT_NAME) - 1);
}

static int encode_network_name(char *buffer, size_t max)
{
	const char *nn = otThreadGetNetworkName(OT_INSTANCE);

	return encode_value(buffer, max, "nn", sizeof("nn") - 1, nn, strlen(nn));
}

static int encode_ext_pan_id(char *buffer, size_t max)
{
	const otExtendedPanId *extPanId = otThreadGetExtendedPanId(OT_INSTANCE);

	return encode_value(buffer, max, "xp", sizeof("xp") - 1, extPanId->m8,
			    sizeof(extPanId->m8));
}

static int encode_thread_version(char *buffer, size_t max)
{
	const char *version = "invalid";

	switch (otThreadGetVersion()) {
	case OT_THREAD_VERSION_1_1:
		version = "1.1.1";
		break;
	case OT_THREAD_VERSION_1_2:
		version = "1.2.0";
		break;
	case OT_THREAD_VERSION_1_3:
		version = "1.3.0";
		break;
	}

	return encode_value(buffer, max, "tv", sizeof("tv") - 1, version,
			    strlen(version));
}

static int encode_ext_address(char *buffer, size_t max)
{
	const otExtAddress *extAddr = otLinkGetExtendedAddress(OT_INSTANCE);

	return encode_value(buffer, max, "xa", sizeof("xa") - 1, extAddr->m8,
			    sizeof(extAddr->m8));
}

static int encode_state(char *buffer, size_t max)
{
	uint32_t flags = 0;
	int if_status;

#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
	otBackboneRouterState bbr_state = otBackboneRouterGetState(OT_INSTANCE);
#endif

	/* For ot-br-posix compatibility:
	 *   -----------------------------------
	 *   | bits  | flag                    |
	 *   |---------------------------------|
	 *   | 0 - 2 | connection mode         |
	 *   | 3 - 4 | thread interface status |
	 *   | 5 - 6 | availability            |
	 *   |   7   | is BBR active           |
	 *   |   8   | is BBR primary          |
	 *   -----------------------------------
	 */

	flags |= CONNECTION_MODE_PSKC & 0b111;
	flags |= (AVAILABILITY_HIGH & 0b11) << 3;

	switch (otThreadGetDeviceRole(OT_INSTANCE)) {
	case OT_DEVICE_ROLE_DISABLED:
		if_status = INTERFACE_STATIS_NOT_INITIALIZED;
		break;
	case OT_DEVICE_ROLE_DETACHED:
		if_status = INTERFACE_STATIS_INITIALIZED;
		break;
	default:
		if_status = INTERFACE_STATIS_ACTIVE;
	}

	flags |= (if_status & 0b11) << 5;
#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
	flags |= (if_status == INTERFACE_STATIS_ACTIVE &&
		  bbr_state != OT_BACKBONE_ROUTER_STATE_DISABLED) << 7;
	flags |= (if_status == INTERFACE_STATIS_ACTIVE &&
		  bbr_state == OT_BACKBONE_ROUTER_STATE_PRIMARY) << 8;
#endif

	flags = sys_cpu_to_be32(flags);

	return encode_value(buffer, max, "sb", sizeof("sb") - 1,
			    (const char *)&flags, sizeof(flags));
}

static int encode_active_timestamp(char *buffer, size_t max)
{
	otOperationalDataset dataset;
	uint64_t active_ts;

	if ((otDatasetGetActive(OT_INSTANCE, &dataset)) != OT_ERROR_NONE) {
		return 0;
	}

	/* 64 bits Timestamp fields layout
	 * -----48 bits------//-----15 bits-----//-------1 bit-------//
	 *      Seconds      //      Ticks      //  Authoritative    //
	 */
	active_ts = (dataset.mActiveTimestamp.mSeconds << 16) |
		    (uint64_t)(dataset.mActiveTimestamp.mTicks << 1) |
		    (uint64_t)(dataset.mActiveTimestamp.mAuthoritative);

	active_ts = sys_cpu_to_be64(active_ts);

	return encode_value(buffer, max, "at", sizeof("at") - 1, (char *)&active_ts,
			    sizeof(active_ts));
}

#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
static int encode_bbr_entries(char *buffer, size_t max)
{
	int res;
	int encoded = 0;
	otBackboneRouterConfig config;
	const char *name;
	size_t val_len;
	uint16_t port = htons(BACKBONE_AGENT_BBR_PORT);
	otBackboneRouterState state = otBackboneRouterGetState(OT_INSTANCE);

	if (state != OT_BACKBONE_ROUTER_STATE_DISABLED) {
		otBackboneRouterGetConfig(OT_INSTANCE, &config);

		res = encode_value(buffer, max, "sq", sizeof("sq") - 1,
				   (char *)&config.mSequenceNumber,
				   sizeof(config.mSequenceNumber));
		if (res < 0) {
			return res;
		}

		encoded += res;
		max -= res;

		res = encode_value(buffer, max, "bb", sizeof("bb") - 1,
				   (char *)&port, sizeof(port));

		if (res < 0) {
			return res;
		}

		encoded += res;
		max -= res;
	}

	name = otThreadGetDomainName(OT_INSTANCE);

	val_len = strlen(name);
	val_len = (val_len > 0) ? val_len - 1 : val_len;

	res = encode_value(buffer, max, "dn", sizeof("dn") - 1, name, val_len);

	if (res < 0) {
		return res;
	}

	return encoded + res;
}

#endif

static int encode_omr_entry(char *buffer, size_t max)
{
	otIp6Prefix omrPrefix;
	otRoutePreference preference;
	otError error;
	char prefix[OT_IP6_PREFIX_SIZE + 1];

	error = otBorderRoutingGetFavoredOmrPrefix(OT_INSTANCE, &omrPrefix, &preference);

	if (error != OT_ERROR_NONE) {
		return 0;
	}

	prefix[0] = omrPrefix.mLength;
	memcpy(&prefix[1], omrPrefix.mPrefix.mFields.m8, (omrPrefix.mLength + 7) / 8);

	return encode_value(buffer, max, "omr", sizeof("omr") - 1, prefix, sizeof(prefix));
}

static void update_meshcop_service()
{
	char buff[MDNS_PUBLISHER_TXT_LEN];
	int n = 0;
	int res = 0;
	uint16_t port = BA_DUMMY_PORT;

	if (!service_rec) {
		mdns_publisher_record_alloc(&service_rec);

		if (!service_rec) {
			LOG_ERR("Failed to allocate mDNS record");
			return;
		}
	}

	/* encode elements: */
	value_encoder encoders[] = {
		encode_vendor_name,
		encode_product_name,
		encode_network_name,
		encode_ext_pan_id,
		encode_thread_version,
		encode_ext_address,
		encode_state,
		encode_active_timestamp,
#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
		encode_bbr_entries,
#endif
		encode_omr_entry,
		NULL,
	};

	for (size_t i = 0; encoders[i] != NULL; ++i) {
		res = encoders[i](&buff[n], MDNS_PUBLISHER_TXT_LEN - n);

		if (res < 0) {
			LOG_ERR("Failed to encode TXT record, encoder: %p", &encoders[i]);
			return;
		}

		n += res;
	}

	/* When Thread interface is disabled we keep using dummy port so service
	 * status can be advertised
	 */
	if (otBorderAgentGetState(OT_INSTANCE) != OT_BORDER_AGENT_STATE_STOPPED) {
		port = otBorderAgentGetUdpPort(OT_INSTANCE);
	}

	/* If the port differs we need to bind the new and close the previous
	 * one. The port must be bound or the mDNS responder will drop requests.
	 */
	if (port != service_rec->port || port == 0) {
		handle_port_update();
		port = service_rec->port;
	}

	mdns_publisher_record_update(service_rec, BA_INSTANCE_NAME, BA_SERVICE,
				     BA_PROTO, BA_DOMAIN, buff, n, port);
}

static void on_thread_state_changed(otChangedFlags flags, struct openthread_context *ot_context,
				    void *user_data)
{
	if (flags & (UPDATE_MESHCOP_FLAGS)) {
		update_meshcop_service();
	}
}

static void unpublish_meshcop_service()
{
	close_serv_socket();
	mdns_publisher_record_free(&service_rec);
	service_rec = NULL;
}

static struct openthread_state_changed_cb ot_state_chaged_cb = {
	.state_changed_cb = on_thread_state_changed
};

void border_agent_init(void)
{
	service_fd = INVALID_FD;
	service_rec = NULL;

	mdns_publisher_init();
	update_meshcop_service();
	openthread_state_changed_cb_register(openthread_get_default_context(), &ot_state_chaged_cb);

	LOG_INF("Border Agent initialized");
}
