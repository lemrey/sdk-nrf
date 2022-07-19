/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "fpga_shell_commands.h"
#include "fpga_target.h"

#include <platform/CHIPDeviceLayer.h>
#include <app/server/Server.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_MATTER_LOG_LEVEL);

using namespace chip;
using namespace chip::app;

using Shell::Engine;
using Shell::shell_command_t;
using Shell::streamer_get;
using Shell::streamer_printf;

Engine sShellFpgaSubCommands;
Engine sShellFpgaCommissionSubCommands;

static CHIP_ERROR FpgaHelpHandler(int argc, char **argv)
{
	sShellFpgaSubCommands.ForEachCommand(Shell::PrintCommandHelp, nullptr);
	return CHIP_NO_ERROR;
}

static CHIP_ERROR FpgaCommandHandler(int argc, char **argv)
{
	if (argc == 0) {
		return FpgaHelpHandler(argc, argv);
	}
	return sShellFpgaSubCommands.ExecCommand(argc, argv);
}

CHIP_ERROR LockCommandHandler(int argc, char **argv)
{
	FPGA_NRF54::LockUnlock *data = Platform::New<FPGA_NRF54::LockUnlock>();
	data->mState = BoltLockManager::State::kLockingInitiated;

	DeviceLayer::PlatformMgr().ScheduleWork(FPGA_NRF54::FpgaLockUnlockHandler, reinterpret_cast<intptr_t>(data));
	return CHIP_NO_ERROR;
}

CHIP_ERROR UnlockCommandHandler(int argc, char **argv)
{
	FPGA_NRF54::LockUnlock *data = Platform::New<FPGA_NRF54::LockUnlock>();
	data->mState = BoltLockManager::State::kUnlockingInitiated;

	DeviceLayer::PlatformMgr().ScheduleWork(FPGA_NRF54::FpgaLockUnlockHandler, reinterpret_cast<intptr_t>(data));
	return CHIP_NO_ERROR;
}

CHIP_ERROR FactoryResetCommandHandler(int argc, char **argv)
{
	chip::Server::GetInstance().ScheduleFactoryReset();
	return CHIP_NO_ERROR;
}

CHIP_ERROR DefaultCommissionCommandHandler(int argc, char **argv)
{
	DeviceLayer::PlatformMgr().ScheduleWork(FPGA_NRF54::FpgaCommissionHandler, 0);
	return CHIP_NO_ERROR;
}

CHIP_ERROR BleAdvertisementStart(int argc, char **argv)
{
	DeviceLayer::PlatformMgr().ScheduleWork(FPGA_NRF54::BleAdvertisementStartHandler, 0);
	return CHIP_NO_ERROR;
}

void RegisterFpgaCommands()
{
	static const shell_command_t sFpgaSubCommands[] = {
		{ &FpgaHelpHandler, "help", "FPGA Lock commands" },
		{ &LockCommandHandler, "lock", "Locking a door lock" },
		{ &UnlockCommandHandler, "unlock", "unlocking a door lock" },
		{ &FactoryResetCommandHandler, "factoryreset", "Perform the factory reset" },
		{ &DefaultCommissionCommandHandler, "commission", "commission to the default thread network" },
		{ &BleAdvertisementStart, "bleadv", "Start BLE advertisement"}
	};

	static const shell_command_t sFpgaCommand = { &FpgaCommandHandler, "fpga",
						      "FPGA lock commands. Usage: fpgalock [lock|unlock|commission]" };

	sShellFpgaSubCommands.RegisterCommands(sFpgaSubCommands, ArraySize(sFpgaSubCommands));

	Engine::Root().RegisterCommands(&sFpgaCommand, 1);
}
