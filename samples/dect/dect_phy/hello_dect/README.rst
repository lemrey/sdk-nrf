.. _nrf_modem_dect_phy_hello:

nRF91x1: DECT NR+ PHY hello sample
##################################

.. contents::
   :local:
   :depth: 2

The sample demonstrates how to set up a simple DECT NR+ application with the DECT PHY modem firmware.

Requirements
************

The sample needs at least two nRF91x1 Development Kits.

See :ref:`nrf91x1_ug_intro` for more details.

.. note::

   The :ref:`nrf_modem_dect_phy_simple` sample requires the DECT NR+ PHY modem firmware to run on the nRF91x1 modem core. Please contact our sales department to know more.

.. include:: /includes/tfm.txt

Overview
********

The sample shows a simple broadcast and reception of DECT NR+ messages between devices on a hard-coded channel.

After initialization the devices will run a loop, transmitting a counter value before listening for incoming receptions. The time to listen is given by the :kconfig:option:`CONFIG_RX_PERIOD_S` Kconfig option.
The loop is exited after a number of transmissions given by the :kconfig:option:`CONFIG_TX_TRANSMISSIONS` Kconfig option, or continue forever if the :kconfig:option:`CONFIG_TX_TRANSMISSIONS` Kconfig option is ``0``.

Each device can be reset to run the sample again.

Building and running
********************

.. |sample path| replace:: :file:`samples/modem_trace_flash`

.. include:: /includes/build_and_run_ns.txt

.. important::

   DECT NR+ operates on free but regulated radio channels. The regulations and availability of the channels varies by countries and regions.
   It is your responsibility to operate the devices according to the local regulation, both at the development site and the device operating regions.
   If you are in the EU and US, you can use the ``overlay-eu.conf`` and ``overlay-us.conf``Kconfig overlays respectively.
   Otherwise set the carrier using the :kconfig:option:`CONFIG_CARRIER` Kconfig option.

Testing
=======

|test_sample|

After programming the sample to your Development Kits, test it by performing the following steps:

#. |connect_kit|
#. |connect_terminal|
#. Observe that the devices will transmit a counter value that is received by the other devices.
#. After a given number of transmissions, observe that the devices shut down and exit the sample.

Sample output
=============

The sample shows an output similar to the following:

Device 1:

.. code-block:: console

   *** Booting nRF Connect SDK v3.5.99-ncs1 ***
   [00:00:00.378,784] <inf> app: Dect NR+ PHY Hello sample started
   [00:00:00.691,375] <inf> app: Dect NR+ PHY initialized, device ID: 12345
   [00:00:00.691,406] <inf> app: Transmitting 0
   [00:00:05.697,784] <inf> app: Transmitting 1
   [00:00:10.704,193] <inf> app: Transmitting 2
   [00:00:14.186,553] <inf> app: Received header from device ID 67890
   [00:00:14.186,889] <inf> app: Received data (RSSI: -54.5): Hello DECT! 0
   [00:00:15.710,571] <inf> app: Transmitting 3
   [00:00:19.192,932] <inf> app: Received header from device ID 67890
   [00:00:19.193,267] <inf> app: Received data (RSSI: -54.5): Hello DECT! 1
   [00:00:20.716,949] <inf> app: Transmitting 4
   ...
   [00:02:24.352,661] <inf> app: Received header from device ID 67890
   [00:02:24.352,996] <inf> app: Received data (RSSI: -54.5): Hello DECT! 26
   [00:02:25.876,739] <inf> app: Transmitting 29
   [00:02:25.876,831] <inf> app: Reached maximum number of transmissions (30)
   [00:02:25.876,831] <inf> app: Shutting down
   [00:02:25.893,554] <inf> app: Bye!

Device 2:

.. code-block:: console

   *** Booting nRF Connect SDK v3.5.99-ncs1 ***
   [00:00:00.407,287] <inf> app: Dect NR+ PHY Hello sample started
   [00:00:00.719,238] <inf> app: Dect NR+ PHY initialized, device ID: 67890
   [00:00:00.719,268] <inf> app: Transmitting 0
   [00:00:02.254,211] <inf> app: Received header from device ID 12345
   [00:00:02.254,547] <inf> app: Received data (RSSI: -54.5): Hello DECT! 3
   [00:00:05.725,646] <inf> app: Transmitting 1
   [00:00:07.260,620] <inf> app: Received header from device ID 12345
   [00:00:07.260,955] <inf> app: Received data (RSSI: -54.5): Hello DECT! 4
   ...
   [00:02:10.885,284] <inf> app: Transmitting 26
   [00:02:12.420,318] <inf> app: Received header from device ID 12345
   [00:02:12.420,654] <inf> app: Received data (RSSI: -54.5): Hello DECT! 29
   [00:02:15.891,693] <inf> app: Transmitting 27
   [00:02:20.898,071] <inf> app: Transmitting 28
   [00:02:25.904,449] <inf> app: Transmitting 29
   [00:02:25.904,541] <inf> app: Reached maximum number of transmissions (30)
   [00:02:25.904,571] <inf> app: Shutting down
   [00:02:25.921,325] <inf> app: Bye!

Dependencies
************

It uses the following `sdk-nrfxlib`_ libraries:

* :ref:`nrfxlib:nrf_modem`

It uses the following Zephyr libraries:

* :ref:`zephyr:uart_api`


In addition, it uses the following secure firmware components:

* :ref:`Trusted Firmware-M <ug_tfm>`
