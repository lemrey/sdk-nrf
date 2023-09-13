/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <openthread/ip6.h>
#include <openthread/error.h>
#include <common/message.hpp>
#include <common/as_core_type.hpp>
#include <zephyr/sys/printk.h>

using namespace ot;

extern "C" otError __real_otIp6Send(otInstance *aInstance, otMessage *aMessage);

/* This function replaces original otIp6Send() implementation from OpenThread's
 * API - it is mandatory to enable multicast loop so such messages are handled
 * properly by OT. However, it cannot be done with a current API version so here
 * we set the flag and call the original function.
 *
 * Note: 'OPENTHREAD_PROJECT_CORE_CONFIG_FILE' define with
 * openthread-core-zephyr-config.h as the value has to be set for this file.
 * Otherwise, there will be a mismatch between 'Message' class layout between
 * this function and the OT stack.
 */
extern "C" otError __wrap_otIp6Send(otInstance *aInstance, otMessage *aMessage)
{
	AsCoreType(aMessage).SetMulticastLoop(true);
	return __real_otIp6Send(aInstance, aMessage);
}
