/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_storage_internal.h>
#include <zephyr/drivers/flash.h>
#ifdef CONFIG_FLASH_SIMULATOR
#include <zephyr/drivers/flash/flash_simulator.h>
#endif /* CONFIG_FLASH_SIMULATOR */
#include <zephyr/logging/log.h>

#define UPDATE_MAGIC_VALUE_AVAILABLE_RAW  0x5555AAAA
#define UPDATE_MAGIC_VALUE_AVAILABLE_CBOR 0x55AA55AA
#define UPDATE_MAGIC_VALUE_EMPTY	  0xFFFFFFFF

LOG_MODULE_REGISTER(suit_storage_update, CONFIG_SUIT_LOG_LEVEL);

#ifdef CONFIG_FLASH_SIMULATOR
static uint8_t *f_base_address = NULL;
#endif /* CONFIG_FLASH_SIMULATOR */
static const struct device *fdev;

/** @brief Update the candidate info structure.
 *
 * @param[in]  info  Update candidate info to write.
 *
 * @return 0 on success, negative value otherwise.
 */
static int save_update_info(struct update_candidate_info *info)
{
	if (fdev == NULL) {
		return -ENXIO;
	}

	int err = flash_erase(fdev, SUIT_STORAGE_OFFSET + offsetof(struct suit_storage, update),
			      sizeof(((struct suit_storage *)0)->update.erase_block));
	if (err != 0) {
		return err;
	}

	return flash_write(fdev, SUIT_STORAGE_OFFSET + offsetof(struct suit_storage, update), info,
			   sizeof(*info));
}

int suit_storage_update_init(void)
{
	fdev = DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));

	if (!device_is_ready(fdev)) {
		fdev = NULL;
		return -ENXIO;
	}

#ifdef CONFIG_FLASH_SIMULATOR
	size_t f_size = 0;
	f_base_address = flash_simulator_get_memory(fdev, &f_size);
#endif /* CONFIG_FLASH_SIMULATOR */

	return 0;
}

int suit_storage_update_cand_get(const suit_plat_mreg_t **regions, size_t *len)
{
	struct suit_storage *storage = (struct suit_storage *)SUIT_STORAGE_ADDRESS;
	struct update_candidate_info *info = &(storage->update.update);

	if (fdev == NULL) {
		return -ENXIO;
	}

	if ((regions == NULL) || (len == NULL)) {
		return -EINVAL;
	}

	if ((info->update_magic_value != UPDATE_MAGIC_VALUE_AVAILABLE_CBOR) ||
	    (info->update_regions_len < 1) || (info->update_regions[0].mem == NULL) ||
	    (info->update_regions[0].size == 0)) {
		return -ENOENT;
	}

	*len = info->update_regions_len;
	*regions = (const suit_plat_mreg_t *)&(info->update_regions);

	return 0;
}

int suit_storage_update_cand_set(suit_plat_mreg_t *regions, size_t len)
{
	struct update_candidate_info info;

	memset(&info, 0, sizeof(info));

	if (((len != 0) && (regions == NULL)) || (len > CONFIG_SUIT_STORAGE_N_UPDATE_REGIONS)) {
		return -EINVAL;
	}

	if ((regions != NULL) && ((regions[0].mem == NULL) || (regions[0].size == 0))) {
		return -EINVAL;
	}

	info.update_magic_value = UPDATE_MAGIC_VALUE_AVAILABLE_CBOR;
	info.update_regions_len = len;
	if (len != 0) {
		memcpy(&info.update_regions, regions, sizeof(regions[0]) * len);
	}

	return save_update_info(&info);
}
