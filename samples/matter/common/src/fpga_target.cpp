/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "fpga_target.h"
#include "app_event.h"
#include "app_task.h"
#include "thread_util.h"

LOG_MODULE_DECLARE(app, CONFIG_MATTER_LOG_LEVEL);

#if !(defined(CONFIG_EMULATOR_FPGA))
#error "You try to build en example for FPGA without FPGA SUPPORT!"
#endif

void FPGA_NRF54::FpgaLockUnlockHandler(intptr_t aContext)
{
	FPGA_NRF54::LockUnlock *data = reinterpret_cast<FPGA_NRF54::LockUnlock *>(aContext);
	switch (data->mState) {
	case BoltLockManager::State::kLockingInitiated:
		LOG_INF("FPGA Locking initiated...");
		GetAppTask().PostEvent(AppEvent{ AppEvent::Lock, BoltLockManager::OperationSource::kRemote });
		break;
	case BoltLockManager::State::kUnlockingInitiated:
		LOG_INF("FPGA Unlocking initiated...");
		GetAppTask().PostEvent(AppEvent{ AppEvent::Unlock, BoltLockManager::OperationSource::kRemote });
		break;
	default:
		break;
	}
}

void FPGA_NRF54::FpgaCommissionHandler(intptr_t aContext)
{
	LOG_INF("Starting default OpenThread Network...");
	GetAppTask().PostEvent(AppEvent{ AppEvent::StartThread });
}

void FPGA_NRF54::BleAdvertisementStartHandler(intptr_t aContext)
{
	GetAppTask().PostEvent(AppEvent{ AppEvent::StartBleAdvertising });
}
