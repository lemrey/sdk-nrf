/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#if defined(CONFIG_EMULATOR_FPGA)

#include "bolt_lock_manager.h"

class FPGA_NRF54 {
    public:

    constexpr static size_t sMaxThreadNetworkName = 15;
    constexpr static size_t sExtendedPanIdSize = 23;

    struct LockUnlock {
        BoltLockManager::State mState;
    };
    
    static void FpgaLockUnlockHandler(intptr_t aContext);
    static void BleAdvertisementStartHandler(intptr_t aContext);
    static void FpgaCommissionHandler(intptr_t aContext);


};

#endif
