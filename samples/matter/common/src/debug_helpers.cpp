/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "debug_helpers.h"

#include "app_event.h"
#include "app_task.h"
#include "thread_util.h"

#include <platform/CHIPDeviceLayer.h>
#include <app/server/Server.h>

#include <zephyr/logging/log.h>

#ifndef CONFIG_CHIP_LIB_SHELL
#error Enable CONFIG_CHIP_LIB_SHELL to use debug_helpers!
#endif

LOG_MODULE_DECLARE(app, CONFIG_MATTER_LOG_LEVEL);

using namespace chip;
using namespace chip::app;

using Shell::Engine;
using Shell::shell_command_t;
using Shell::streamer_get;
using Shell::streamer_printf;

Engine sShellDebugSubCommands;
Engine sShellDebugCommissionSubCommands;

static CHIP_ERROR DebugHelpHandler(int argc, char **argv)
{
	sShellDebugSubCommands.ForEachCommand(Shell::PrintCommandHelp, nullptr);
	return CHIP_NO_ERROR;
}

static CHIP_ERROR DebugCommandHandler(int argc, char **argv)
{
	if (argc == 0) {
		return DebugHelpHandler(argc, argv);
	}
	return sShellDebugSubCommands.ExecCommand(argc, argv);
}

CHIP_ERROR LockCommandHandler(int argc, char **argv)
{
	LockSampleHelpers::LockUnlock *data = Platform::New<LockSampleHelpers::LockUnlock>();
	data->mState = BoltLockManager::State::kLockingInitiated;

	DeviceLayer::PlatformMgr().ScheduleWork(LockSampleHelpers::LockUnlockHandler, reinterpret_cast<intptr_t>(data));
	return CHIP_NO_ERROR;
}

CHIP_ERROR UnlockCommandHandler(int argc, char **argv)
{
	LockSampleHelpers::LockUnlock *data = Platform::New<LockSampleHelpers::LockUnlock>();
	data->mState = BoltLockManager::State::kUnlockingInitiated;

	DeviceLayer::PlatformMgr().ScheduleWork(LockSampleHelpers::LockUnlockHandler, reinterpret_cast<intptr_t>(data));
	return CHIP_NO_ERROR;
}

CHIP_ERROR FactoryResetCommandHandler(int argc, char **argv)
{
	chip::Server::GetInstance().ScheduleFactoryReset();
	return CHIP_NO_ERROR;
}

CHIP_ERROR DefaultCommissionCommandHandler(int argc, char **argv)
{
	DeviceLayer::PlatformMgr().ScheduleWork(LockSampleHelpers::CommissionHandler, 0);
	return CHIP_NO_ERROR;
}

CHIP_ERROR BleAdvertisementStart(int argc, char **argv)
{
	DeviceLayer::PlatformMgr().ScheduleWork(LockSampleHelpers::BleAdvertisementStartHandler, 0);
	return CHIP_NO_ERROR;
}

void RegisterDebugCommands()
{
	static const shell_command_t sDebugSubCommands[] = {
		{ &DebugHelpHandler, "help", "Helpers commands" },
		{ &LockCommandHandler, "lock", "Locking a door lock" },
		{ &UnlockCommandHandler, "unlock", "unlocking a door lock" },
		{ &FactoryResetCommandHandler, "factoryreset", "Perform the factory reset" },
		{ &DefaultCommissionCommandHandler, "thread", "commission to the default thread network" },
		{ &BleAdvertisementStart, "bleadv", "Start BLE advertisement"}
	};

	static const shell_command_t sDebugCommand = { &DebugCommandHandler, "command",
						      "Helpers commands. Usage: command [lock|unlock|thread|bleadv]" };

	sShellDebugSubCommands.RegisterCommands(sDebugSubCommands, ArraySize(sDebugSubCommands));

	Engine::Root().RegisterCommands(&sDebugCommand, 1);
}


void LockSampleHelpers::LockUnlockHandler(intptr_t aContext)
{
	LockSampleHelpers::LockUnlock *data = reinterpret_cast<LockSampleHelpers::LockUnlock *>(aContext);
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

void LockSampleHelpers::CommissionHandler(intptr_t aContext)
{
	LOG_INF("Starting default OpenThread Network...");
	GetAppTask().PostEvent(AppEvent{ AppEvent::StartThread });
}

void LockSampleHelpers::BleAdvertisementStartHandler(intptr_t aContext)
{
	GetAppTask().PostEvent(AppEvent{ AppEvent::StartBleAdvertising });
}


void StartDefaultThreadNetwork(uint64_t datasetTimestamp)
{
	using namespace chip::DeviceLayer;

	constexpr uint8_t masterkey[] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
					  0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF };

	chip::Thread::OperationalDataset dataset{};

	uint8_t xpanid[8];
	net_bytes_from_str(xpanid, sizeof(xpanid), CONFIG_OPENTHREAD_XPANID);

	dataset.SetChannel(CONFIG_OPENTHREAD_CHANNEL);
	dataset.SetExtendedPanId(xpanid);
	dataset.SetMasterKey(masterkey);
	dataset.SetNetworkName(CONFIG_OPENTHREAD_NETWORK_NAME);
	dataset.SetPanId(CONFIG_OPENTHREAD_PANID);

	if (datasetTimestamp != 0) {
		dataset.SetActiveTimestamp(datasetTimestamp);
	}

	ThreadStackMgr().SetThreadEnabled(false);
	ThreadStackMgr().SetThreadProvision(dataset.AsByteSpan());
	ThreadStackMgr().SetThreadEnabled(true);

#if CHIP_DEVICE_CONFIG_ENABLE_DNSSD
	chip::app::DnssdServer::Instance().StartServer();
#endif
}
