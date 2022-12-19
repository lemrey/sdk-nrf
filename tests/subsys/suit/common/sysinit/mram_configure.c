/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <init.h>
#include <zephyr/devicetree.h>
#include <hal/nrf_mpc.h>
#include <hal/nrf_cache.h>


/**
 * @brief Structure containing relevant settings for an MPC OVERRIDE.
 */
struct mpc_region_override {
	nrf_mpc_override_config_t config;
	nrf_owner_t owner_id;
	uintptr_t start_address;
	uint32_t perm;
	uint32_t permmask;
	uint32_t offset;
	size_t size;
	size_t index;
};


static void mramc_enable_direct_write(NRF_MRAMC_Type *mramc)
{
	uint32_t config = MRAMC_CONFIG_WEN_EnableDirectWrite << MRAMC_CONFIG_WEN_Pos;

	mramc->CONFIG = config;
	mramc->READYNEXTTIMEOUT = (128UL << MRAMC_READYNEXTTIMEOUT_VALUE_Pos) |
			 (true << MRAMC_READYNEXTTIMEOUT_DW_Pos);
}

static void mpc_configure_override(NRF_MPC_Type *mpc, struct mpc_region_override *override)
{
	nrf_mpc_override_startaddr_set(mpc, override->index, override->start_address);
	nrf_mpc_override_endaddr_set(mpc, override->index, override->start_address + override->size);
	nrf_mpc_override_offset_set(mpc, override->index, override->offset);
	nrf_mpc_override_perm_set(mpc, override->index, override->perm);
	nrf_mpc_override_permmask_set(mpc, override->index, override->permmask);
	nrf_mpc_override_ownerid_set(mpc, override->index, override->owner_id);
	nrf_mpc_override_config_set(mpc, override->index, &override->config);
}


/**
 * @brief Enable direct writes to MRAM10 and MRAM11 for use by local domains.
 */
int configure_mram_direct_write(const struct device *dev)
{
	uint32_t address = DT_REG_ADDR(DT_NODELABEL(mram10));
	uint32_t size = DT_REG_SIZE(DT_NODELABEL(mram10)) + DT_REG_SIZE(DT_NODELABEL(mram11));
	struct mpc_region_override override = {
		.config =
			(nrf_mpc_override_config_t){
				.slave_number = 0,
				.lock = false,
				.enable = true,
				.secdom_enable = false,
				.secure_mask = true,
			},
		.perm = (MPC_OVERRIDE_PERM_READ_Msk | MPC_OVERRIDE_PERM_WRITE_Msk),
		.permmask = (MPC_OVERRIDE_PERM_READ_Msk | MPC_OVERRIDE_PERM_WRITE_Msk),
		.index = (size_t)1,
		.owner_id = NRF_OWNER_SYSCTRL,
	};

	/* Ensure L2CACHE is enabled for domain accesses to MRAM. */
	nrf_cache_enable(NRF_SYSCTRL_L2CACHE);

	/* Enable updating MRAM through direct writes. */
	mramc_enable_direct_write(NRF_MRAMC111);
	mramc_enable_direct_write(NRF_MRAMC110);

	/* Allow for accessing MRAM by configuring MPC override on AXI1 for SYSCTRL domain. */
	override.start_address = address & 0xEFFFFFFF;
	override.size = size;
	mpc_configure_override(NRF_MPC111, &override);

        return 0;
}


SYS_INIT(configure_mram_direct_write, PRE_KERNEL_1, 99);
