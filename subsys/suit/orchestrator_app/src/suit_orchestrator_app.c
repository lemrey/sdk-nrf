/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <dfu/suit_dfu.h>

#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>

#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

#include <sdfw_services/suit_service.h>
#include <suit_envelope_info.h>
#include <suit_plat_mem_util.h>

#if CONFIG_SUIT_STREAM_IPC_PROVIDER
#include <ipc_streamer.h>
#endif

#define FIXED_PARTITION_ERASE_BLOCK_SIZE(label)                                                    \
	DT_PROP(DT_GPARENT(DT_NODELABEL(label)), erase_block_size)

#define DFU_PARTITION_LABEL	     dfu_partition
#define DFU_PARTITION_ADDRESS	 suit_plat_mem_nvm_ptr_get(DFU_PARTITION_OFFSET)
#define DFU_PARTITION_OFFSET	 FIXED_PARTITION_OFFSET(DFU_PARTITION_LABEL)
#define DFU_PARTITION_SIZE	     FIXED_PARTITION_SIZE(DFU_PARTITION_LABEL)
#define DFU_PARTITION_EB_SIZE	 FIXED_PARTITION_ERASE_BLOCK_SIZE(DFU_PARTITION_LABEL)
#define DFU_PARTITION_WRITE_SIZE FIXED_PARTITION_WRITE_BLOCK_SIZE(DFU_PARTITION_LABEL)
#define DFU_PARTITION_DEVICE	 FIXED_PARTITION_DEVICE(DFU_PARTITION_LABEL)

LOG_MODULE_REGISTER(suit_dfu, CONFIG_SUIT_LOG_LEVEL);

#define SUIT_PROCESSOR_ERR_TO_ZEPHYR_ERR(err) ((err) == SUIT_SUCCESS ? 0 : -EACCES)

#if CONFIG_SUIT_ORCHESTRATOR_APP_CANDIDATE_PROCESSING

static int dfu_partition_erase(void)
{
	const struct device *fdev = DFU_PARTITION_DEVICE;

	if (!device_is_ready(fdev)) {
		return -ENODEV;
	}

	/* Division is used to round down so that erase_size is aligned to DFU_PARTITION_EB_SIZE  */
	size_t erase_size = (DFU_PARTITION_SIZE / DFU_PARTITION_EB_SIZE) * DFU_PARTITION_EB_SIZE;

	LOG_INF("Erasing dfu partition");

	int rc = flash_erase(fdev, DFU_PARTITION_OFFSET, erase_size);
	if (rc < 0) {
		return -EIO;
	}

	return 0;
}

#endif /* CONFIG_SUIT_ORCHESTRATOR_APP_CANDIDATE_PROCESSING */

int suit_dfu_initialize(void)
{
#if CONFIG_SUIT_STREAM_IPC_PROVIDER
	suit_ipc_streamer_provider_init();
#endif

	/* TODO: add any other needed initialization - Ref: NCSDK-25349, Ref: NCSDK-25701 */

	LOG_DBG("SUIT DFU module init ok");

	return 0;
}

#if CONFIG_SUIT_ORCHESTRATOR_APP_CANDIDATE_PROCESSING

int suit_dfu_cleanup(void)
{
	suit_envelope_info_reset();

	dfu_partition_erase();
	/* TODO purge cache partitions and other cleanup, Ref: NCSDK-25701 */

	return 0;
}

int suit_dfu_candidate_envelope_stored(void)
{
	suit_plat_err_t err;

	err = suit_envelope_info_candidate_stored((const uint8_t*) DFU_PARTITION_ADDRESS,
						  DFU_PARTITION_SIZE);

	if (err != SUIT_PLAT_SUCCESS)
	{
		LOG_INF("Invalid update candidate: %d", err);

		return -ENOTSUP;
	}

	/* TODO: update cache 0, Ref: NCSDK-25701 */

	return 0;
}

int suit_dfu_candidate_preprocess(void)
{
	/* TODO: Run the SUIT processor, Ref: NCSDK-25349 */

	return 0;
}

int suit_dfu_update_start(void)
{
	const uint8_t* candidate_envelope_address;
	size_t candidate_envelope_size;

	int err = suit_envelope_info_get(&candidate_envelope_address, &candidate_envelope_size);

	if (err != SUIT_PLAT_SUCCESS) {
		LOG_INF("Invalid update candidate: %d", err);

		return -ENOTSUP;
	}

	/* TODO: get update regions - caches, Ref: NCSDK-25701 */
	LOG_INF("Reboot the system and trigger the update");

	LOG_PANIC();

#if CONFIG_SUIT_CACHE
	suit_plat_mreg_t update_candidate[CONFIG_SUIT_CACHE_MAX_CACHES + 1];
#else
	suit_plat_mreg_t update_candidate[1];
#endif

	update_candidate[0].mem = candidate_envelope_address;
	update_candidate[0].size = candidate_envelope_size;

	/* TODO: store information about caches */

	return suit_trigger_update(update_candidate, ARRAY_SIZE(update_candidate));
}

#endif /* CONFIG_SUIT_ORCHESTRATOR_APP_CANDIDATE_PROCESSING */
