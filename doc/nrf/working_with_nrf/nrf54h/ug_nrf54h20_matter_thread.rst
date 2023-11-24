.. _ug_nrf54h20_matter_thread:

Working with nRF54H20 and Matter and Thread
###########################################

.. contents::
   :local:
   :depth: 2

.. caution::

   The limited customer sampling version of the |NCS| is affected by the OpenThread KeyID Mode 2 Security Vulnerability.
   This vulnerability impacts all Thread devices using OpenThread and allows an attacker in physical proximity to compromise non-router-capable devices and the entire Thread network in the case of router-capable devices.
   The vulnerability allows an attacker in physical proximity to inject arbitrary IPv6 packets into the Thread network via IEEE 802.15.4 frame transmissions.
   Because the Thread Management Framework (TMF) protocol does not have any additional layer of security, the attacker could exploit this vulnerability to update the Thread Network Key and gain full access to the Thread network.
   There is no known exploitation of vulnerability.

   Due to this issue, the Thread certifications for OpenThread libraries in all |NCS| releases up to v2.4.0, which the limited customer sampling is based on, are deprecated.

Starting with the ``v2.2.99-cs1-dev1`` limited customer sampling tag, the nRF54H20 PDK supports the following Matter and Thread samples:

* :ref:`Matter door lock sample <matter_lock_sample>`
* :ref:`Matter template sample <matter_template_sample>`
* :ref:`Thread CLI sample <ot_cli_sample>`

The Matter support is currently limited to Matter over Thread, and support for both Matter and Thread is currently :ref:`experimental <software_maturity>`.
Read the following sections for more information about support in the nRF54H20 PDK and the platform design for the nRF54H20 SoC.

For more information about Matter and Thread in the |NCS|, read the documentation in the :ref:`ug_matter` and :ref:`ug_thread` protocol sections.

Platform designs for the nRF54H20 SoC
*************************************

Matter in the |NCS| supports the *System-on-Chip, multiprotocol* platform design for the nRF54H20 SoC using Matter over Thread.
You can read more about other available platform designs for Matter on the :ref:`Matter platform design page<ug_matter_overview_architecture_integration_designs>`.

Thread in the |NCS| supports the *System-on-Chip, single protocol* and *System-on-Chip, multiprotocol* platform designs for the nRF54H20 SoC.
You can read more about other available platform designs for Thread on the :ref:`OpenThread architectures page<ug_thread_architectures>`.

For more information about the multiprotocol feature, see :ref:`ug_multiprotocol_support`.

System-on-Chip, single protocol
===============================

In this design, the OpenThread stack runs on several cores and domains of a single nRF54H20 SoC.

This platform designs is suitable for the following development kit:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf54h20dk_nrf54h20_cpuapp@soc1

In this design:

* The Application Core runs the OpenThread stack.
* The Radio Core runs the component of the OpenThread stack that is related to the 802.15.4 IEEE Radio Driver.
* The Secure Domain stores all the secure components, including keys and certificates.
  It also manages the access to peripherals, memory, and other components.

  .. note::
     The PSA crypto API level 3 for storing security components is not yet implemented on the Secure Domain.

* The Global Domain manages clocks, power, global RAM, and global NVM.

For more information, see :ref:`ug_nrf54h20_architecture_cpu`.

The following figure demonstrates the architecture.
The Global Domain is not included.

.. figure:: images/thread_platform_design_nRF54h20.svg
   :alt: Multiprotocol Thread and Bluetooth LE architecture (nRF54H20)

   Single protocol Thread architecture on the nRF54H20 SoC

System-on-Chip, multiprotocol
=============================

In this design, the OpenThread stack and the Bluetooth® Low Energy (LE) stack run on several cores and domains of a single nRF54H20 SoC.

This platform design is suitable for the following development kit:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf54h20dk_nrf54h20_cpuapp@soc1

In this design:

* The Application Core runs the OpenThread stack, and a part of the Bluetooth LE Controller.
* The Radio Core runs both the Bluetooth LE Controller and the component of the OpenThread stack that is related to the 802.15.4 IEEE Radio Driver.
* The Secure Domain stores all the secure components, including keys and certificates.
  It also manages the access to peripherals, memory, and other components.

  .. note::
      The PSA crypto API level 3 for storing security components is not yet implemented on the Secure Domain.

* The Global Domain manages clocks, power, global RAM, and global NVM.

For more information, see :ref:`ug_nrf54h20_architecture_cpu`.

The following figure demonstrates the architecture.
The Global Domain is not included.

.. _nrf54h20_platform_multi_figure:

.. figure:: images/thread_platform_design_nRF54h20_multi.svg
   :alt: Multiprotocol Thread and Bluetooth LE architecture (nRF54H20)

   Multiprotocol Thread and Bluetooth LE architecture on the nRF54H20 SoC

Matter over Thread
==================

In this design, the Matter stack, the OpenThread stack, and the Bluetooth® Low Energy (LE) stack run on several cores and domains of a single nRF54H20 SoC.

This platform design is suitable for the following development kit:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf54h20dk_nrf54h20_cpuapp@soc1

In this design:

* The Application Core runs the Matter stack, the OpenThread stack, and a part of the Bluetooth LE Controller.
* The Radio Core runs both the Bluetooth LE Controller and the component of the OpenThread stack that is related to the 802.15.4 IEEE Radio Driver.
* The Secure Domain stores all the secure components, including keys and certificates.
  It also manages the access to peripherals, memory, and other components.

  .. note::
      The PSA crypto API level 3 for storing security components is not yet implemented on the Secure Domain.

* The Global Domain manages clocks, power, global RAM, and global NVM.

For more information, see :ref:`ug_nrf54h20_architecture_cpu`.

Refer to the :ref:`nrf54h20_platform_multi_figure` figure to see the architecture of the SoC.
The Global Domain is not included.

Additional requirements on the nRF54H20 PDK
*******************************************

In addition to the standard requirements for the |NCS|, such as the :ref:`ug_matter_gs_tools_gn` for Matter, you need the following to run Matter-enabled or Thread-enabled applications on the nRF54H20 PDK:

* For DFU - J-Link and a USB cable.
* The compatible version of the nrfjprog tool, included in the :ref:`nRF Command Line Tools version specific to the limited customer sampling<nrf54h20_install_commandline>`.

Configuring Matter and Thread on the nRF54H20 PDK
*************************************************

Currently, only the configuration for Matter over Thread is supported for Matter.
Follow the configuration steps on the :ref:`ug_matter_gs_testing` page to configure the Matter environment for the supported Matter samples.

Currently, only the :ref:`ot_cli_sample` sample is supported for Thread.
See the sample documentation for how to configure it.

The Matter and Thread samples included in the limited customer sampling can work on the corresponding networks with standard devices of the same protocol.

Programming Matter and Thread samples on the nRF54H20 PDK
=========================================================

To program the compatible Matter or Thread samples on the nRF54H20 PDK, follow the :ref:`ug_nrf54h20_gs_sample` steps.
Read also programming guides prepared for specific Matter samples: :ref:`Matter door lock sample <matter_lock_sample>`, and :ref:`Matter template sample <matter_template_sample>`.

.. note::
   :ref:`Testing using Bluetooth LE with Nordic UART Service <matter_lock_sample_ble_nus>` on the :ref:`Matter door lock sample <matter_lock_sample>` is disabled by default.

Logging for Matter and Thread samples on the nRF54H20 PDK
=========================================================

To read logs for Matter samples on the nRF54H20 PDK, complete the following steps:

1. Connect to the nRF54H20 PDK using a USB cable.
#. Select the first available port to read the logs from.

For more information, see :ref:`ug_nrf54h20_logging`.

Implementing support for the nRF54H20 PDK
=========================================

If you want to implement support for the nRF54H20 PDK in your Matter-enabled or Thread-enabled application, read the :ref:`ug_nrf54h20_configuration` guide.

.. _ug_nrf54h20_matter_thread_limitations:

Limitations for Matter and Thread on the nRF54H20 PDK
*****************************************************

Matter and Thread support has the following limitations on the nRF54H20 PDK:

* DFU over Bluetooth LE, Matter, or Serial Link is not yet implemented.
* The current implementation is not power-optimized.
* The cryptographic operations related to Matter and Thread are performed on the Application Core, rather than on the Secure Domain.
* The ``west flash --erase`` command is blocked.
  See :ref:`ug_nrf54h20_gs_sample` for more information.
* The factory reset functionality does not work properly.
  After clearing all NVM storage, the device can not reboot automatically and falls into a hard fault.

  As a workaround, press the reset button on the nRF54H20 PDK board after performing a factory reset.
* Matter over Thread commissioning might be unstable due to the lack of true random generator support on nRF54H20.

  After each reboot or factory reset, the device will always have the same Bluetooth LE and IEEE 80215.4 addresses.
  This might impact working within the Thread network because after the second and following connections, Thread Border Router will reject these connections until deleted from the database and commissioning to Matter will take more and more time.

  As a workaround, after each factory reset and before performing the next commissioning to Matter, connect to the device's serial port and run the following command:

    .. parsed-literal::
       :class: highlight

       ot extaddr *address*

  Replace the *address* argument with an 8-byte randomly generated MAC address, for example ``87fb47d5730ac0a0``.
