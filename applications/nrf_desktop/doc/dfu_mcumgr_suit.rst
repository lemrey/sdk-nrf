.. _nrf_desktop_dfu_mcumgr_suit:

Device Firmware Upgrade MCUmgr SUIT module
##########################################

.. contents::
   :local:
   :depth: 2

The Device Firmware Upgrade MCUmgr SUIT module is responsible for performing a Software Update for Internet of Things (SUIT) Device Firmware Upgrade (DFU) procedure over Simple Management Protocol (SMP).

.. note::
    The SUIT DFU procedure over SMP uses a custom implementation of the MCUmgr image management handlers (:kconfig:option:`CONFIG_MGMT_SUITFU`), which does not support the image management notification hooks used by both :ref:`nrf_desktop_dfu_mcumgr` and :ref:`caf_ble_smp`.
    Therefore, these existing modules cannot be used to handle SUIT DFU procedure over Bluetooth LE SMP.
    When used with Bluetooth LE transport, the DFU MCUmgr SUIT module relies only on SMP command status callbacks to indicate the need for lower Bluetooth LE connection latency.
    This allows the module to be used for SUIT DFU procedure over SMP with Bluetooth LE transport.

Configuration
*************

To enable the module, set the :ref:`CONFIG_DESKTOP_DFU_MCUMGR_SUIT_ENABLE <config_desktop_app_options>` Kconfig option.

The module selects the following configurations:

* :kconfig:option:`CONFIG_MCUMGR` - This option enables the MCUmgr support, which is required for the DFU process.
  For details, see :ref:`zephyr:mcu_mgr` in the Zephyr documentation.
* :kconfig:option:`CONFIG_MGMT_SUITFU` - This option enables the SUIT Firmware Update Management that implements MCUmgr image management handlers required for the DFU process.
* :kconfig:option:`CONFIG_MCUMGR_MGMT_NOTIFICATION_HOOKS` - This option enables the MCUmgr notification hook support, which allows the module to listen for an MCUmgr event.
  For details, see :ref:`zephyr:mcumgr_callbacks` in the Zephyr documentation.

  MCUmgr notifications hooks:

    * :kconfig:option:`CONFIG_MCUMGR_SMP_COMMAND_STATUS_HOOKS` - This option enables the MCUmgr SMP command status callbacks that inform when an SMP command is received or processed.

* MCUmgr dependencies:

  * :kconfig:option:`CONFIG_NET_BUF`
  * :kconfig:option:`CONFIG_ZCBOR`
  * :kconfig:option:`CONFIG_CRC`
  * :kconfig:option:`CONFIG_FLASH_MAP`
  * :kconfig:option:`CONFIG_FLASH`

* SUIT Firmware Update Management dependencies:

  * :kconfig:option:`CONFIG_MCUMGR_SMP_LEGACY_RC_BEHAVIOUR`

To use the module, you must also enable the :kconfig:option:`CONFIG_SUIT` Kconfig option.
See :ref:`ug_nrf54h20_suit_dfu` for more information.

You must also enable the preferred transport for the MCUmgr's SMP protocol (for example, the :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_BT` Kconfig option).
With the :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_BT` Kconfig option enabled, the module enables the :kconfig:option:`CONFIG_CAF_BLE_SMP_TRANSFER_EVENTS` event.
The event can be used to reduce the Bluetooth connection latency during the DFU process.

You cannot use this module with the :ref:`nrf_desktop_dfu_mcumgr` or :ref:`caf_ble_smp`.
In other words, if you enable the :ref:`CONFIG_DESKTOP_DFU_MCUMGR_SUIT_ENABLE <config_desktop_app_options>` Kconfig option, you cannot enable the :ref:`CONFIG_DESKTOP_DFU_MCUMGR_ENABLE <config_desktop_app_options>` or :kconfig:option:`CONFIG_CAF_BLE_SMP` Kconfig option.

Implementation details
**********************

The module uses MCUmgr's SMP command status received callback to periodically submit a :c:struct:`ble_smp_transfer_event` when SMP commands are received over Bluetooth LE.
The event signals to other application modules that the Bluetooth LE connection latency should be lowered to ensure quick SMP data transfer.
The module registers itself as the final subscriber of the event to track the number of submitted events.
If a :c:struct:`ble_smp_transfer_event` was already submitted, but was not yet processed, the module desists from submitting subsequent events.
