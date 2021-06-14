.. _at_monitor_sample:

nRF9160: AT monitor
###################

.. contents::
   :local:
   :depth: 2

The AT monitor  sample demonstrates how to use the :ref:`at_monitor_readme` library.

Requirements
************

The sample supports the following development kit:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf9160dk_nrf9160ns

.. include:: /includes/spm.txt

Overview
********

The sample defines two AT monitors, one for network status notifications (``+CEREG``) and one for received signal quality parameters notifications (``+CESQ``) via the :c:func:`AT_MONITOR` macro.
The sample then subscribes to both notifications and switches the modem to function mode one to register to the network.
While the device is registering to the network, the sample uses one of the AT monitors to determine when the registration has happened, and monitors the signal quality with the other.
Once the device has registered with the network, the sample will read the modem PSM mode status, enable it, and read the PSM mode status again.

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/at_monitor`

.. include:: /includes/build_and_run_nrf9160.txt


Testing
=======

After programming the sample to your development kit, test it by performing the following steps:

1. Connect the USB cable and power on or reset your nRF9160 DK.
#. Open a terminal emulator and observe that the sample starts and connects to the LTE network, while printing both the network registration status and signal quality during the process.
#. Observe that sample prints the PSM status twice, once when it is disabled, and once when it is enabled.
#. Observe that sample completes and prints "Bye".

Sample Output
=============

The sample shows the following output:

.. code-block:: console

	AT Monitor sample started
	Subscribing to notifications
	Connecting to network
	Resuming link quality monitor for AT notifications
	Waiting for network
	Link quality: -61 dBm
	Network registration status: searching
	Link quality: -59 dBm
	Network registration status: home
	Network connection ready
	Pausing link quality monitor for AT notifications
	Reading PSM info...
	  PSM: disabled
	Enabling PSM
	Reading PSM info...
	  PSM: enabled
	Periodic TAU string: 00000110
	Active time string: 00100001
	Modem response:
	+CEREG: 1,1
	OK
	Bye

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`at_monitor_readme`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

In addition, it uses the following sample:

* :ref:`secure_partition_manager`
