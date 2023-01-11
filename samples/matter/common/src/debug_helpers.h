/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <lib/core/CHIPError.h>
#include <lib/shell/Engine.h>
#include <lib/shell/commands/Help.h>

#include "bolt_lock_manager.h"

void RegisterDebugCommands();
void StartDefaultThreadNetwork(uint64_t datasetTimestamp);

class LockSampleHelpers {
    public:

    constexpr static size_t sMaxThreadNetworkName = 15;
    constexpr static size_t sExtendedPanIdSize = 23;

    struct LockUnlock {
        BoltLockManager::State mState;
    };
    
    static void LockUnlockHandler(intptr_t aContext);
    static void BleAdvertisementStartHandler(intptr_t aContext);
    static void CommissionHandler(intptr_t aContext);
};
