/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "app_event.h"
#include "bolt_lock_manager.h"
#include "led_widget.h"

#include <platform/CHIPDeviceLayer.h>

#if CONFIG_CHIP_FACTORY_DATA
#include <platform/nrfconnect/FactoryDataProvider.h>
#endif

#ifdef CONFIG_MCUMGR_SMP_BT
#include "dfu_over_smp.h"
#endif

struct k_timer;
struct Identify;

class AppTask {
public:
	static AppTask &Instance()
	{
		static AppTask sAppTask;
		return sAppTask;
	};

	CHIP_ERROR StartApp();

	void UpdateClusterState(BoltLockManager::State state, BoltLockManager::OperationSource source);

	static void PostEvent(const AppEvent &event);
	static void IdentifyStartHandler(Identify *);
	static void IdentifyStopHandler(Identify *);

private:
	CHIP_ERROR Init();

	void CancelTimer();
	void StartTimer(uint32_t timeoutInMs);

	void DispatchEvent(const AppEvent &event);
	void FunctionPressHandler(uint8_t buttonNumber);
	void FunctionReleaseHandler(uint8_t buttonNumber);
	void FunctionTimerEventHandler();
#if CONFIG_EMULATOR_FPGA || CONFIG_SOC_SERIES_NRF54HX
	void StartThreadHandler();
#endif
	void StartBLEAdvertisingHandler();

	static void ChipEventHandler(const chip::DeviceLayer::ChipDeviceEvent *event, intptr_t arg);
	static void ButtonEventHandler(uint32_t buttonState, uint32_t hasChanged);
	static void LEDStateUpdateHandler(LEDWidget &ledWidget);
	static void FunctionTimerTimeoutCallback(k_timer *timer);
	static void UpdateStatusLED();

	static void LockStateChanged(BoltLockManager::State state, BoltLockManager::OperationSource source);

#ifdef CONFIG_MCUMGR_SMP_BT
	static void RequestSMPAdvertisingStart(void);
#endif

#ifdef CONFIG_THREAD_WIFI_SWITCHING
	static void SwitchImagesDone();
	static void SwitchImagesTriggerHandler(const AppEvent &event);
	static void SwitchImagesTimerTimeoutCallback(k_timer *timer);
	static void SwitchImagesEventHandler(const AppEvent &event);

	bool mSwitchImagesTimerActive = false;
#endif

	enum class TimerFunction {
		NoneSelected = 0,
		SoftwareUpdate,
		FactoryReset,
#if CONFIG_BOARD_NRF7002DK_NRF5340_CPUAPP || CONFIG_SOC_SERIES_NRF54HX
		AdvertisingStart,
#endif
	};

	FunctionEvent mFunction = FunctionEvent::NoneSelected;
	bool mFunctionTimerActive = false;

#if CONFIG_CHIP_FACTORY_DATA
	chip::DeviceLayer::FactoryDataProvider<chip::DeviceLayer::InternalFlashFactoryData> mFactoryDataProvider;
#endif
};
