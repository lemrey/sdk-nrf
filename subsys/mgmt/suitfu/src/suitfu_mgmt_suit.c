/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/sys/util.h>
#include <string.h>
#include <stdio.h>

#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>
#include <mgmt/mcumgr/util/zcbor_bulk.h>

#ifdef CONFIG_SSF_SUIT_SERVICE_ENABLED
#include <sdfw_services/suit_service.h>
#endif /* CONFIG_SSF_SUIT_SERVICE_ENABLED */

#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/mgmt/handlers.h>
#include "suitfu_mgmt_priv.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(suitfu_mgmt, CONFIG_MGMT_SUITFU_LOG_LEVEL);

static const struct mgmt_handler suit_mgmt_handlers[] = {

#ifdef CONFIG_MGMT_SUITFU_GRP_SUIT_CAND_ENV_UPLOAD
	[SUIT_MGMT_ID_ENVELOPE_UPLOAD] = {.mh_read = NULL,
					  .mh_write = suitfu_mgmt_suit_envelope_upload},
#endif

#ifdef CONFIG_MGMT_SUITFU_GRP_SUIT_IMAGE_FETCH
	[SUIT_MGMT_ID_MISSING_IMAGE_STATE] = {.mh_read = suitfu_mgmt_suit_missing_image_state_read,
					      .mh_write = NULL},
	[SUIT_MGMT_ID_MISSING_IMAGE_UPLOAD] = {.mh_read = NULL,
					       .mh_write = suitfu_mgmt_suit_missing_image_upload},
#endif
};

static struct mgmt_group suit_mgmt_group = {
	.mg_handlers = (struct mgmt_handler *)suit_mgmt_handlers,
	.mg_handlers_count = ARRAY_SIZE(suit_mgmt_handlers),
	.mg_group_id = CONFIG_MGMT_GROUP_ID_SUIT,
};

void suit_mgmt_register_group(void)
{
#ifdef CONFIG_MGMT_SUITFU_GRP_SUIT_IMAGE_FETCH
	suitfu_mgmt_suit_image_fetch_init();
#endif
	mgmt_register_group(&suit_mgmt_group);
}

void suit_mgmt_unregister_group(void)
{
	mgmt_unregister_group(&suit_mgmt_group);
}

#ifdef CONFIG_MGMT_SUITFU_AUTO_REGISTER_HANDLERS
MCUMGR_HANDLER_DEFINE(suitfu_mgmt_suit, suit_mgmt_register_group);
#endif
