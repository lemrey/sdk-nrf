/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"

#include "bolt_lock_manager.h"
#include "led_widget.h"

#ifdef CONFIG_NET_L2_OPENTHREAD
#include "thread_util.h"
#endif

#if CONFIG_EMULATOR_FPGA || CONFIG_SOC_SERIES_NRF54HX
#include "debug_helpers.h"
#endif

#include <platform/CHIPDeviceLayer.h>

#include "board_util.h"
#include <app-common/zap-generated/attribute-id.h>
#include <app-common/zap-generated/attribute-type.h>
#include <app-common/zap-generated/attributes/Accessors.h>
#include <app-common/zap-generated/cluster-id.h>
#include <app/clusters/door-lock-server/door-lock-server.h>
#include <app/server/OnboardingCodesUtil.h>
#include <app/server/Server.h>
#include <credentials/DeviceAttestationCredsProvider.h>
#include <credentials/examples/DeviceAttestationCredsExample.h>
#include <lib/support/CHIPMem.h>
#include <lib/support/CodeUtils.h>
#include <system/SystemError.h>

#ifdef CONFIG_CHIP_WIFI
#include <app/clusters/network-commissioning/network-commissioning.h>
#include <platform/nrfconnect/wifi/NrfWiFiDriver.h>
#endif

#ifdef CONFIG_CHIP_OTA_REQUESTOR
#include "ota_util.h"
#endif

#include <dk_buttons_and_leds.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

#include <algorithm>

using namespace ::chip;
using namespace ::chip::app;
using namespace ::chip::Credentials;
using namespace ::chip::DeviceLayer;

LOG_MODULE_DECLARE(app, CONFIG_MATTER_LOG_LEVEL);

namespace
{
constexpr size_t kAppEventQueueSize = 10;
constexpr uint32_t kFactoryResetTriggerTimeout = 3000;
constexpr uint32_t kFactoryResetCancelWindow = 3000;
#if NUMBER_OF_BUTTONS == 2
constexpr uint32_t kAdvertisingTriggerTimeout = 3000;
#endif
constexpr EndpointId kLockEndpointId = 1;

K_MSGQ_DEFINE(sAppEventQueue, sizeof(AppEvent), kAppEventQueueSize, alignof(AppEvent));
LEDWidget sStatusLED;
LEDWidget sLockLED;
#if NUMBER_OF_LEDS == 4
LEDWidget sUnusedLED;
LEDWidget sUnusedLED_1;
#endif

bool sIsNetworkProvisioned;
bool sIsNetworkEnabled;
bool sHaveBLEConnections;

k_timer sFunctionTimer;
} /* namespace */

AppTask AppTask::sAppTask;

#ifdef CONFIG_CHIP_WIFI
app::Clusters::NetworkCommissioning::Instance
	sWiFiCommissioningInstance(0, &(NetworkCommissioning::NrfWiFiDriver::Instance()));
#endif

CHIP_ERROR AppTask::Init()
{
	/* Initialize CHIP stack */
	LOG_INF("Init CHIP stack");

	CHIP_ERROR err = chip::Platform::MemoryInit();
	if (err != CHIP_NO_ERROR) {
		LOG_ERR("Platform::MemoryInit() failed");
		return err;
	}

	err = PlatformMgr().InitChipStack();
	if (err != CHIP_NO_ERROR) {
		LOG_ERR("PlatformMgr().InitChipStack() failed");
		return err;
	}

#if defined(CONFIG_NET_L2_OPENTHREAD)
	err = ThreadStackMgr().InitThreadStack();
	if (err != CHIP_NO_ERROR) {
		LOG_ERR("ThreadStackMgr().InitThreadStack() failed");
		return err;
	}

#ifdef CONFIG_OPENTHREAD_MTD_SED
	err = ConnectivityMgr().SetThreadDeviceType(ConnectivityManager::kThreadDeviceType_SleepyEndDevice);
#else
	err = ConnectivityMgr().SetThreadDeviceType(ConnectivityManager::kThreadDeviceType_MinimalEndDevice);
#endif /* CONFIG_OPENTHREAD_MTD_SED */
	if (err != CHIP_NO_ERROR) {
		LOG_ERR("ConnectivityMgr().SetThreadDeviceType() failed");
		return err;
	}

#ifdef CONFIG_OPENTHREAD_DEFAULT_TX_POWER
	err = SetDefaultThreadOutputPower();
	if (err != CHIP_NO_ERROR) {
		LOG_ERR("Cannot set default Thread output power");
		return err;
	}
#endif /* CONFIG_OPENTHREAD_DEFAULT_TX_POWER */
#elif defined(CONFIG_CHIP_WIFI)
	sWiFiCommissioningInstance.Init();
#else
	return CHIP_ERROR_INTERNAL;
#endif /* CONFIG_NET_L2_OPENTHREAD */

	/* Initialize LEDs */
	LEDWidget::InitGpio();
	LEDWidget::SetStateUpdateCallback(LEDStateUpdateHandler);

	sStatusLED.Init(DK_LED1);
	sLockLED.Init(DK_LED2);
	sLockLED.Set(BoltLockMgr().IsLocked());
#if NUMBER_OF_LEDS == 4
	sUnusedLED.Init(DK_LED3);
	sUnusedLED_1.Init(DK_LED4);
#endif

	UpdateStatusLED();

#if CONFIG_EMULATOR_FPGA || CONFIG_SOC_SERIES_NRF54HX
	RegisterDebugCommands();
#endif

	/* Initialize buttons */
#if !(defined(CONFIG_EMULATOR_FPGA) || defined(CONFIG_SOC_SERIES_NRF54HX))
	int ret = dk_buttons_init(ButtonEventHandler);
	if (ret) {
		LOG_ERR("dk_buttons_init() failed");
		return chip::System::MapErrorZephyr(ret);
	}
#endif

#ifdef CONFIG_MCUMGR_SMP_BT
	/* Initialize DFU over SMP */
	GetDFUOverSMP().Init(RequestSMPAdvertisingStart);
	GetDFUOverSMP().ConfirmNewImage();
#endif

	/* Initialize function timer */
	k_timer_init(&sFunctionTimer, &AppTask::TimerEventHandler, nullptr);
	k_timer_user_data_set(&sFunctionTimer, this);

	/* Initialize lock manager */
	BoltLockMgr().Init(LockStateChanged);

	/* Initialize CHIP server */
#if CONFIG_CHIP_FACTORY_DATA
	ReturnErrorOnFailure(mFactoryDataProvider.Init());
	SetDeviceInstanceInfoProvider(&mFactoryDataProvider);
	SetDeviceAttestationCredentialsProvider(&mFactoryDataProvider);
	SetCommissionableDataProvider(&mFactoryDataProvider);
#else
	SetDeviceAttestationCredentialsProvider(Examples::GetExampleDACProvider());
#endif
	static CommonCaseDeviceServerInitParams initParams;
	(void)initParams.InitializeStaticResourcesBeforeServerInit();

	ReturnErrorOnFailure(chip::Server::GetInstance().Init(initParams));
	ConfigurationMgr().LogDeviceConfig();
	PrintOnboardingCodes(chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE));

	/*
	 * Add CHIP event handler and start CHIP thread.
	 * Note that all the initialization code should happen prior to this point to avoid data races
	 * between the main and the CHIP threads.
	 */
	PlatformMgr().AddEventHandler(ChipEventHandler, 0);

	err = PlatformMgr().StartEventLoopTask();
	if (err != CHIP_NO_ERROR) {
		LOG_ERR("PlatformMgr().StartEventLoopTask() failed");
		return err;
	}

	return CHIP_NO_ERROR;
}

CHIP_ERROR AppTask::StartApp()
{
	ReturnErrorOnFailure(Init());

	AppEvent event = {};

	while (true) {
		k_msgq_get(&sAppEventQueue, &event, K_FOREVER);
		DispatchEvent(event);
	}

	return CHIP_NO_ERROR;
}

void AppTask::PostEvent(const AppEvent &event)
{
	if (k_msgq_put(&sAppEventQueue, &event, K_NO_WAIT)) {
		LOG_INF("Failed to post event to app task event queue");
	}
}

void AppTask::UpdateClusterState(BoltLockManager::State state, BoltLockManager::OperationSource source)
{
	DlLockState newLockState;

	switch (state) {
	case BoltLockManager::State::kLockingCompleted:
		newLockState = DlLockState::kLocked;
		break;
	case BoltLockManager::State::kUnlockingCompleted:
		newLockState = DlLockState::kUnlocked;
		break;
	default:
		newLockState = DlLockState::kNotFullyLocked;
		break;
	}

	SystemLayer().ScheduleLambda([newLockState, source] {
		chip::app::DataModel::Nullable<chip::app::Clusters::DoorLock::DlLockState> currentLockState;
		chip::app::Clusters::DoorLock::Attributes::LockState::Get(kLockEndpointId, currentLockState);

		if (currentLockState.IsNull()) {
			/* Initialize lock state with start value, but not invoke lock/unlock. */
			chip::app::Clusters::DoorLock::Attributes::LockState::Set(kLockEndpointId, newLockState);
		} else {
			LOG_INF("Updating LockState attribute");

			if (!DoorLockServer::Instance().SetLockState(kLockEndpointId, newLockState, source)) {
				LOG_ERR("Failed to update LockState attribute");
			}
		}
	});
}

#ifdef CONFIG_MCUMGR_SMP_BT
void AppTask::RequestSMPAdvertisingStart(void)
{
	sAppTask.PostEvent(AppEvent{ AppEvent::StartSMPAdvertising });
}
#endif

void AppTask::DispatchEvent(const AppEvent &event)
{
	switch (event.Type) {
	case AppEvent::Lock:
		BoltLockMgr().Lock(event.LockEvent.Source);
		break;
	case AppEvent::Unlock:
		BoltLockMgr().Unlock(event.LockEvent.Source);
		break;
	case AppEvent::Toggle:
		if (BoltLockMgr().IsLocked()) {
			BoltLockMgr().Unlock(event.LockEvent.Source);
		} else {
			BoltLockMgr().Lock(event.LockEvent.Source);
		}
		break;
	case AppEvent::CompleteLockAction:
		BoltLockMgr().CompleteLockAction();
		break;
	case AppEvent::FunctionPress:
		FunctionPressHandler(event.FunctionEvent.ButtonNumber);
		break;
	case AppEvent::FunctionRelease:
		FunctionReleaseHandler(event.FunctionEvent.ButtonNumber);
		break;
	case AppEvent::FunctionTimer:
		FunctionTimerEventHandler();
		break;
	case AppEvent::StartBleAdvertising:
		StartBLEAdvertisingHandler();
		break;
#if CONFIG_EMULATOR_FPGA || CONFIG_SOC_SERIES_NRF54HX
	case AppEvent::StartThread:
		StartThreadHandler();
		break;
#endif
	case AppEvent::UpdateLedState:
		event.UpdateLedStateEvent.LedWidget->UpdateState();
		break;
#ifdef CONFIG_MCUMGR_SMP_BT
	case AppEvent::StartSMPAdvertising:
		GetDFUOverSMP().StartBLEAdvertising();
		break;
#endif
	default:
		LOG_INF("Unknown event received");
		break;
	}
}

#if CONFIG_EMULATOR_FPGA || CONFIG_SOC_SERIES_NRF54HX
void AppTask::StartThreadHandler()
{
	if (!ConnectivityMgr().IsThreadProvisioned()) {
		StartDefaultThreadNetwork(0);
		LOG_INF("Device is not commissioned to a Thread network. Starting with the default configuration.");
	} else {
		LOG_INF("Device is commissioned to a Thread network.");
	}
}
#endif /* CONFIG_EMULATOR_FPGA */

void AppTask::FunctionPressHandler(uint8_t buttonNumber)
{
	if (buttonNumber == DK_BTN1) {
		sAppTask.StartFunctionTimer(kFactoryResetTriggerTimeout);
		sAppTask.mFunction = TimerFunction::SoftwareUpdate;
	}
#if NUMBER_OF_BUTTONS == 2
	else if (buttonNumber == DK_BTN2) {
		sAppTask.StartFunctionTimer(kAdvertisingTriggerTimeout);
		sAppTask.mFunction = TimerFunction::AdvertisingStart;
	}
#endif
}

void AppTask::FunctionReleaseHandler(uint8_t buttonNumber)
{
	if (buttonNumber == DK_BTN1) {
		if (sAppTask.mFunction == TimerFunction::SoftwareUpdate) {
			sAppTask.CancelFunctionTimer();
			sAppTask.mFunction = TimerFunction::NoneSelected;

#ifdef CONFIG_MCUMGR_SMP_BT
			GetDFUOverSMP().StartServer();
#else
			LOG_INF("Software update is disabled");
#endif

		} else if (sAppTask.mFunction == TimerFunction::FactoryReset) {
#if NUMBER_OF_LEDS == 4
			sUnusedLED_1.Set(false);
			sUnusedLED.Set(false);
#endif

			/* Set lock status LED back to show state of lock. */
			sLockLED.Set(BoltLockMgr().IsLocked());

			UpdateStatusLED();

			sAppTask.CancelFunctionTimer();
			sAppTask.mFunction = TimerFunction::NoneSelected;
			LOG_INF("Factory Reset has been Canceled");
		}
	}
#if NUMBER_OF_BUTTONS == 2
	else if (buttonNumber == DK_BTN2) {
		if (sAppTask.mFunction == TimerFunction::AdvertisingStart) {
			sAppTask.CancelFunctionTimer();
			sAppTask.mFunction = TimerFunction::NoneSelected;
			GetAppTask().PostEvent(AppEvent{ AppEvent::Toggle, BoltLockManager::OperationSource::kButton });
		}
	}
#endif
}

void AppTask::FunctionTimerEventHandler()
{
	if (sAppTask.mFunction == TimerFunction::SoftwareUpdate) {
		LOG_INF("Factory Reset Triggered. Release button within %ums to cancel.", kFactoryResetCancelWindow);
		sAppTask.StartFunctionTimer(kFactoryResetCancelWindow);
		sAppTask.mFunction = TimerFunction::FactoryReset;

#ifdef CONFIG_STATE_LEDS
		/* Turn off all LEDs before starting blink to make sure blink is co-ordinated. */
		sStatusLED.Set(false);
		sLockLED.Set(false);
#if NUMBER_OF_LEDS == 4
		sUnusedLED_1.Set(false);
		sUnusedLED.Set(false);
#endif

		sStatusLED.Blink(500);
		sLockLED.Blink(500);
#if NUMBER_OF_LEDS == 4
		sUnusedLED.Blink(500);
		sUnusedLED_1.Blink(500);
#endif
#endif
	} else if (sAppTask.mFunction == TimerFunction::FactoryReset) {
		sAppTask.mFunction = TimerFunction::NoneSelected;
		LOG_INF("Factory Reset triggered");
		chip::Server::GetInstance().ScheduleFactoryReset();
	}
#if NUMBER_OF_BUTTONS == 2
	else if (sAppTask.mFunction == TimerFunction::AdvertisingStart) {
		sAppTask.mFunction = TimerFunction::NoneSelected;
		GetAppTask().PostEvent(AppEvent{ AppEvent::StartBleAdvertising });
	}
#endif
}

void AppTask::StartBLEAdvertisingHandler()
{
	if (Server::GetInstance().GetFabricTable().FabricCount() != 0) {
		LOG_INF("Matter service BLE advertising not started - device is already commissioned");
		return;
	}

	if (ConnectivityMgr().IsBLEAdvertisingEnabled()) {
		LOG_INF("BLE advertising is already enabled");
		return;
	}

	if (chip::Server::GetInstance().GetCommissioningWindowManager().OpenBasicCommissioningWindow() !=
	    CHIP_NO_ERROR) {
		LOG_ERR("OpenBasicCommissioningWindow() failed");
	}
}

void AppTask::LEDStateUpdateHandler(LEDWidget &ledWidget)
{
	sAppTask.PostEvent(AppEvent{ AppEvent::UpdateLedState, &ledWidget });
}

void AppTask::LockStateChanged(BoltLockManager::State state, BoltLockManager::OperationSource source)
{
	switch (state) {
	case BoltLockManager::State::kLockingInitiated:
		LOG_INF("Lock action initiated");
		sLockLED.Blink(50, 50);
		break;
	case BoltLockManager::State::kLockingCompleted:
		LOG_INF("Lock action completed");
		sLockLED.Set(true);
		break;
	case BoltLockManager::State::kUnlockingInitiated:
		LOG_INF("Unlock action initiated");
		sLockLED.Blink(50, 50);
		break;
	case BoltLockManager::State::kUnlockingCompleted:
		LOG_INF("Unlock action completed");
		sLockLED.Set(false);
		break;
	}

	/* Handle changing attribute state in the application */
	sAppTask.UpdateClusterState(state, source);
}

void AppTask::UpdateStatusLED()
{
#ifdef CONFIG_STATE_LEDS
	/* Update the status LED.
	 *
	 * If thread and service provisioned, keep the LED On constantly.
	 *
	 * If the system has ble connection(s) uptill the stage above, THEN blink the LED at an even
	 * rate of 100ms.
	 *
	 * Otherwise, blink the LED On for a very short time. */
	if (sIsNetworkProvisioned && sIsNetworkEnabled) {
		sStatusLED.Set(true);
	} else if (sHaveBLEConnections) {
		sStatusLED.Blink(100, 100);
	} else {
		sStatusLED.Blink(50, 950);
	}
#endif
}

void AppTask::ChipEventHandler(const ChipDeviceEvent *event, intptr_t /* arg */)
{
	switch (event->Type) {
	case DeviceEventType::kCHIPoBLEAdvertisingChange:
#ifdef CONFIG_CHIP_NFC_COMMISSIONING
		if (event->CHIPoBLEAdvertisingChange.Result == kActivity_Started) {
			if (NFCMgr().IsTagEmulationStarted()) {
				LOG_INF("NFC Tag emulation is already started");
			} else {
				ShareQRCodeOverNFC(
					chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE));
			}
		} else if (event->CHIPoBLEAdvertisingChange.Result == kActivity_Stopped) {
			NFCMgr().StopTagEmulation();
		}
#endif
		sHaveBLEConnections = ConnectivityMgr().NumBLEConnections() != 0;
		UpdateStatusLED();
		break;
#if defined(CONFIG_NET_L2_OPENTHREAD)
	case DeviceEventType::kDnssdPlatformInitialized:
#if CONFIG_CHIP_OTA_REQUESTOR
		InitBasicOTARequestor();
#endif /* CONFIG_CHIP_OTA_REQUESTOR */
		break;
	case DeviceEventType::kThreadStateChange:
		sIsNetworkProvisioned = ConnectivityMgr().IsThreadProvisioned();
		sIsNetworkEnabled = ConnectivityMgr().IsThreadEnabled();
#elif defined(CONFIG_CHIP_WIFI)
	case DeviceEventType::kWiFiConnectivityChange:
		sIsNetworkProvisioned = ConnectivityMgr().IsWiFiStationProvisioned();
		sIsNetworkEnabled = ConnectivityMgr().IsWiFiStationEnabled();
#if CONFIG_CHIP_OTA_REQUESTOR
		if (event->WiFiConnectivityChange.Result == kConnectivity_Established) {
			InitBasicOTARequestor();
		}
#endif /* CONFIG_CHIP_OTA_REQUESTOR */
#endif
		UpdateStatusLED();
		break;
	default:
		break;
	}
}

void AppTask::ButtonEventHandler(uint32_t buttonState, uint32_t hasChanged)
{
	if (DK_BTN1_MSK & buttonState & hasChanged) {
		GetAppTask().PostEvent(AppEvent{ AppEvent::FunctionPress, DK_BTN1 });
	} else if (DK_BTN1_MSK & hasChanged) {
		GetAppTask().PostEvent(AppEvent{ AppEvent::FunctionRelease, DK_BTN1 });
	}

/* nRF7002DK has only two buttons, so it needs to use Button 2 for control and advertising purposes */
#if NUMBER_OF_BUTTONS == 2
	if (DK_BTN2_MSK & buttonState & hasChanged) {
		GetAppTask().PostEvent(AppEvent{ AppEvent::FunctionPress, DK_BTN2 });
	} else if (DK_BTN2_MSK & hasChanged) {
		GetAppTask().PostEvent(AppEvent{ AppEvent::FunctionRelease, DK_BTN2 });
	}
#else
	if (DK_BTN2_MSK & buttonState & hasChanged) {
		GetAppTask().PostEvent(AppEvent{ AppEvent::Toggle, BoltLockManager::OperationSource::kButton });
	}

	if (DK_BTN4_MSK & buttonState & hasChanged) {
		GetAppTask().PostEvent(AppEvent{ AppEvent::StartBleAdvertising });
	}
#endif /* NUMBER_OF_BUTTONS */
}

void AppTask::CancelFunctionTimer()
{
	k_timer_stop(&sFunctionTimer);
}

void AppTask::StartFunctionTimer(uint32_t timeoutInMs)
{
	k_timer_start(&sFunctionTimer, K_MSEC(timeoutInMs), K_NO_WAIT);
}

void AppTask::TimerEventHandler(k_timer *timer)
{
	GetAppTask().PostEvent(AppEvent{ AppEvent::FunctionTimer, 0 });
}
