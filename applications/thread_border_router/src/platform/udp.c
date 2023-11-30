/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief OpenThread platform - UDP
 */

#include "tbr.h"

#include <ipv6.h>
#include <openthread/udp.h>

#include <sys/errno.h>

#include <zephyr/logging/log.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/socket.h>

#define MAX_UDP_SIZE CONFIG_NRF_TBR_MAX_UDP_DGRAM_SIZE

LOG_MODULE_DECLARE(nrf_tbr, CONFIG_NRF_TBR_LOG_LEVEL);

K_MUTEX_DEFINE(udp_mutex);

static uint8_t dgram_buffer[MAX_UDP_SIZE];

static void udp_sent_cb(struct net_context *context, int status, void *user_data)
{
	if (status < 0) {
		LOG_DBG("OT socket (%p) - datagram transmission failed, error: %d",
			user_data, status);
	} else {
		LOG_DBG("OT socket (%p) - transmitted %d bytes", user_data, status);
	}
}

otError otPlatUdpSocket(otUdpSocket *aUdpSocket)
{
	struct net_context *net_ctx;
	int res;

	if (!aUdpSocket) {
		return OT_ERROR_INVALID_ARGS;
	}

	res = net_context_get(AF_INET6, SOCK_DGRAM, IPPROTO_UDP, &net_ctx);

	if (res < 0) {
		LOG_ERR("Failed to allocate network context, error: %d", res);
		return OT_ERROR_FAILED;
	}

	aUdpSocket->mHandle = net_ctx;

	LOG_DBG("OT socket (%p) - opened", aUdpSocket);

	return OT_ERROR_NONE;
}

otError otPlatUdpClose(otUdpSocket *aUdpSocket)
{
	if (!aUdpSocket || !aUdpSocket->mHandle) {
		return OT_ERROR_INVALID_ARGS;
	}

	LOG_DBG("OT socket (%p) - closing", aUdpSocket);

	/* Without CONFIG_NET_OFFLOAD this function call always succeeds */
	net_context_put(aUdpSocket->mHandle);

	return OT_ERROR_NONE;
}

otError otPlatUdpBind(otUdpSocket *aUdpSocket)
{
	int res;
	char addrstr[INET6_ADDRSTRLEN];
	struct sockaddr addr;

	if (!aUdpSocket || !aUdpSocket->mHandle) {
		return OT_ERROR_INVALID_ARGS;
	}

	net_sin6(&addr)->sin6_family = AF_INET6;
	net_sin6(&addr)->sin6_port = htons(aUdpSocket->mSockName.mPort);

	net_ipv6_addr_copy_raw((uint8_t *)&net_sin6(&addr)->sin6_addr,
				(uint8_t *)&aUdpSocket->mSockName.mAddress);

	res = net_context_bind(aUdpSocket->mHandle, &addr, sizeof(struct sockaddr_in6));

	if (res < 0) {
		LOG_ERR("OT socket (%p) - failed to bind", aUdpSocket);
		return OT_ERROR_FAILED;
	}

	inet_ntop(net_sin6(&addr)->sin6_family, &net_sin6(&addr)->sin6_addr,
		  addrstr, sizeof(addrstr));

	LOG_DBG("OT socket (%p) - bound to [%s]:%u", aUdpSocket,
		addrstr, ntohs(net_sin6(&addr)->sin6_port));

	return OT_ERROR_NONE;
}

otError otPlatUdpBindToNetif(otUdpSocket *aUdpSocket, otNetifIdentifier aNetifIdentifier)
{
	struct tbr_context *ctx = tbr_get_context();

	if (!aUdpSocket || !aUdpSocket->mHandle) {
		return OT_ERROR_INVALID_ARGS;
	}

	switch(aNetifIdentifier) {
	case OT_NETIF_THREAD:
		if (!ctx->ot->iface) {
			return OT_ERROR_FAILED;
		}

		net_context_set_iface(aUdpSocket->mHandle, ctx->ot->iface);
		break;
	case OT_NETIF_BACKBONE:
		if (!ctx->backbone_iface) {
			return OT_ERROR_FAILED;
		}

		net_context_set_iface(aUdpSocket->mHandle, ctx->backbone_iface);
		break;
	default:
		break;
	}

	return OT_ERROR_NONE;
}

otError otPlatUdpConnect(otUdpSocket *aUdpSocket)
{
	struct sockaddr addr;
	int res;

	if (!aUdpSocket || !aUdpSocket->mHandle) {
		return OT_ERROR_INVALID_ARGS;
	}

	net_sin6(&addr)->sin6_family = AF_INET6;
	net_sin6(&addr)->sin6_port = aUdpSocket->mPeerName.mPort;

	net_ipv6_addr_copy_raw((uint8_t *)&net_sin6(&addr)->sin6_addr,
			       (uint8_t *)&aUdpSocket->mPeerName.mAddress);

	res = net_context_connect(aUdpSocket->mHandle, &addr, sizeof(struct sockaddr_in6),
				  /* callback */ NULL, K_NO_WAIT, aUdpSocket);

	if (res < 0) {
		LOG_ERR("OT socket (%p) - UDP connect failed: %d", aUdpSocket, res);
		return OT_ERROR_FAILED;
	}

	return OT_ERROR_NONE;;
}

otError otPlatUdpSend(otUdpSocket *aUdpSocket, otMessage *aMessage,
		      const otMessageInfo *aMessageInfo)
{
	int res;
	size_t len;
	struct sockaddr addr;
	socklen_t addrlen;

	if (!aUdpSocket || !aUdpSocket->mHandle || !aMessage || !aMessageInfo) {
		return OT_ERROR_INVALID_ARGS;
	}

	len = otMessageGetLength(aMessage);

	if (len > MAX_UDP_SIZE) {
		return OT_ERROR_NO_BUFS;
	}

	k_mutex_lock(&udp_mutex, K_FOREVER);

	if (len != otMessageRead(aMessage, 0, dgram_buffer, len)) {
		k_mutex_unlock(&udp_mutex);
		return OT_ERROR_FAILED;
	}

	LOG_DBG("OT socket (%p) - sending UDP datagram with len = %u, ", aUdpSocket, len);

	net_sin6(&addr)->sin6_family = AF_INET6;
	net_sin6(&addr)->sin6_port = htons(aMessageInfo->mPeerPort);
	addrlen = sizeof(struct sockaddr_in6);

	net_ipv6_addr_copy_raw((uint8_t *)&net_sin6(&addr)->sin6_addr,
			       (uint8_t *)&aMessageInfo->mPeerAddr);

	res = net_context_sendto(aUdpSocket->mHandle, dgram_buffer, len, &addr, addrlen,
				 udp_sent_cb, K_NO_WAIT, aUdpSocket);

	k_mutex_unlock(&udp_mutex);

	if (res < 0) {
		LOG_ERR("OT socket (%p) - failed to send UDP datagram, error: %d",
			aUdpSocket, res);
		return OT_ERROR_FAILED;
	}

	return OT_ERROR_NONE;
}

otError otPlatUdpJoinMulticastGroup(otUdpSocket        *aUdpSocket,
				    otNetifIdentifier   aNetifIdentifier,
				    const otIp6Address *aAddress)
{
	struct tbr_context *ctx = tbr_get_context();
	struct in6_addr maddr;
	char addrstr[INET6_ADDRSTRLEN];
	int res;

	if (!aUdpSocket || !aUdpSocket->mHandle || !aAddress ||
	    aNetifIdentifier != OT_NETIF_BACKBONE) {
		return OT_ERROR_INVALID_ARGS;
	}

	if (!ctx->backbone_iface) {
		return OT_ERROR_FAILED;
	}

	net_ipv6_addr_copy_raw((uint8_t *)&maddr, (uint8_t *)aAddress);

	res = net_ipv6_mld_join(ctx->backbone_iface, &maddr);

	inet_ntop(AF_INET6, &maddr, addrstr, sizeof(addrstr));

	if (res < 0 && res != -EALREADY) {
		LOG_ERR("OT socket (%p) - failed to join multicast group [%s], error:  %d",
			aUdpSocket, addrstr, res);
		return OT_ERROR_FAILED;
	}

	return OT_ERROR_NONE;
}

otError otPlatUdpLeaveMulticastGroup(otUdpSocket        *aUdpSocket,
				     otNetifIdentifier   aNetifIdentifier,
				     const otIp6Address *aAddress)
{
	struct tbr_context *ctx = tbr_get_context();
	struct in6_addr maddr;
	char addrstr[INET6_ADDRSTRLEN];
	int res;

	if (!aUdpSocket || !aUdpSocket->mHandle || !aAddress ||
	    aNetifIdentifier != OT_NETIF_BACKBONE) {
		return OT_ERROR_INVALID_ARGS;
	}

	if (!ctx->backbone_iface) {
		return OT_ERROR_FAILED;
	}

	net_ipv6_addr_copy_raw((uint8_t *)&maddr, (uint8_t *)aAddress);

	res = net_ipv6_mld_leave(ctx->backbone_iface, &maddr);

	inet_ntop(AF_INET6, &maddr, addrstr, sizeof(addrstr));

	if (res < 0) {
		LOG_ERR("OT socket (%p) - failed to leave multicast group [%s], error: %d",
			aUdpSocket, addrstr, res);
		return OT_ERROR_FAILED;
	}

	return OT_ERROR_NONE;
}
