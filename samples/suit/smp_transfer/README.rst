.. _nrf54h_suit_sample:

SUIT DFU: Update over Bluetooth® Low Energy
###########################################

.. contents::
   :local:
   :depth: 2

The sample demonstrates how to update and boot the nRF54H20 System on Chip (SoC) using the Software Update for Internet of Things (SUIT) procedure on the Application and Radio cores of the SoC.

Requirements
************

The sample supports the following development kit:

+------------------------+----------+--------------------------------+--------------------------------+
| **Hardware platforms** | **PCA**  | **Board name**                 | **Build target**               |
+========================+==========+================================+================================+
| nRF54H20 PDK           | PCA10145 | ``nrf54h20dk_nrf54h20_cpuapp`` | ``nrf54h20dk_nrf54h20_cpuapp`` |
+------------------------+----------+--------------------------------+--------------------------------+

You also need the nRF Connect Device Manager app:

* `nRF Connect Device Manager for Android <https://play.google.com/store/apps/details?id=no.nordicsemi.android.nrfconnectdevicemanager>`__
  (The minimum required version is v1.9.)

* `nRF Connect Device Manager for iOS <https://apps.apple.com/us/app/nrf-connect-device-manager/id1519423539>`__
  (The minimum required version is v1.5.)

Overview
********

The sample uses Bluetooth® Low Energy and the nRF Connect Device Manager app to perform an update on the nRF54H20 SoC, featuring Nordic Semiconductor’s implementation of the SUIT procedure.
The sample demonstrates how to perform a DFU of the Application and Radio firmware.

SUIT is the only DFU procedure supported for the nRF54H20 SoCs.
To read more about the SUIT procedure, see `Device Firmware Update using SUIT <https://res.developer.nordicsemi.com/ncs/doc/latest/nrf/working_with_nrf/nrf54h/ug_suit_dfu_procedure.html>`__.

|NCS|-specific SUIT limitations
===============================

The sample and the SUIT DFU procedure currently only support the Application core and the Radio core firmware updates.
Additionally, this sample is an early version and is only meant to showcase the start of Nordic Semiconductor’s implementation of the SUIT DFU procedure.

User interface
**************

LED 1:
    The number of blinks indicates the application firmware version.
    The default is set to ``1`` to blink once, indicating "Version 1".

Configuration
*************

See `Configuring your application <https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/config_and_build/modifying.html#configure-application>`_ for information about how to permanently or temporarily change the configuration.

Configuration options
=====================

Check and configure the Kconfig option ``CONFIG_APP_VERSION``.
This changes the number of blinks on **LED 1** and sets the `sequence number <https://res.developer.nordicsemi.com/ncs/doc/latest/nrf/working_with_nrf/nrf54h/ug_suit_dfu_procedure.html#suit-manifest-sequence-number>`__ of the SUIT `envelope <https://res.developer.nordicsemi.com/ncs/doc/latest/nrf/working_with_nrf/nrf54h/ug_suit_dfu_procedure.html#envelope>`__’s manifest.

Modify partition sizes
======================

You can also modify the size and location of the partitions.
This is done by modifying the values for the desired location and size of the partition in the devicetree :file:`.overlay` files.

* To modify the Application core’s partition size,  modify the values for ``slot0_partition`` defined in :file:`modules/ic-next/dts/arm/nordic_nrf_next/nrf54h20_cpuapp.dtsi`.

* To modify the DFU partition, modify the values for ``dfu_partition`` defined in :file:`nrf/samples/suit/smp_transfer/boards/nrf54h20dk_nrf54h20_cpuapp.overlay`.

    * ``dfu_partition`` is where the update candidate is stored before the update process begins.

Template manifest file
======================

A manifest file is required to create a SUIT envelope to carry out the DFU.
See the `Manifest <https://res.developer.nordicsemi.com/ncs/doc/latest/nrf/working_with_nrf/nrf54h/ug_suit_dfu_procedure.html#manifest>`__ section of the SUIT documentation to read more about SUIT manifest files.
The template file is automatically created after the sample is built for the first time.

Building and running
********************

To build and program the sample to the nRF54H20 PDK, complete the following steps:

1. Connect the PDK to your computer using a USB cable.

#. Power on the PDK.

#. Open a terminal window in :file:`nrf/samples/suit/smp_transfer`.

#. Build the application using the following ``west`` command, with the following Kconfig options set:

    .. code-block:: console

        west build -p -b nrf54h20dk_nrf54h20_cpuapp@soc1 -- -DOVERLAY_CONFIG=overlay-bt.conf -DCONFIG_APP_VERSION=1


    Note

     If you are compiling in Windows and the build is unsuccessful due to the maximum path length limitation, use the following command:

        .. code-block:: console

            west build -p -b nrf54h20dk_nrf54h20_cpuapp@soc1 -d C:\nordic-lcs\west_working_dir\build\ -- -DOVERLAY_CONFIG=overlay-bt.conf -DCONFIG_APP_VERSION=1

    After running the ``west build`` command, the output build files can be found in the :file:`build/zephyr` directory.
    One of the build artifacts is the SUIT envelope :file:`envelope.suit` file with both firmware binaries embedded as integrated payloads.

    If you want to further configure your application, see `Configuring your application <https://res.developer.nordicsemi.com/ncs/doc/latest/nrf/getting_started/modifying.html#configure-application>`__ for additional information.

    For more information about files generated as output of the build process, see `Output build files <https://res.developer.nordicsemi.com/ncs/doc/latest/nrf/app_dev/build_and_config_system/index.html#app-build-output-files>`__.
    For more information on the contents of the build directory, see `Build Directory Contents <https://res.developer.nordicsemi.com/ncs/doc/latest/zephyr/develop/application/index.html#build-directory-contents>`__ in the Zephyr documentation.

#.  Run the following command to flash the HEX files onto the PDK:

    .. code-block:: console

        west flash


    Note

    If you are compiling in Windows and the build is unsuccessful due to the maximum path length limitation, use the following command:

    .. code-block:: console

        west flash -d C:\nordic-lcs\west_working_dir\build\

For more information on building and programming with the command line, see the Zephyr documentation on `Building, Flashing and Debugging <https://res.developer.nordicsemi.com/ncs/doc/latest/zephyr/develop/west/build-flash-debug.html#west-build-flash-debug>`__.
See `nRF54H20 debugging <https://res.developer.nordicsemi.com/ncs/doc/latest/nrf/working_with_nrf/nrf54h/ug_nrf54h20_debugging.html#ug-nrf54h20-debugging>`__ and `nRF54H20 logging <https://res.developer.nordicsemi.com/ncs/doc/latest/nrf/working_with_nrf/nrf54h/ug_nrf54h20_logging.html#ug-nrf54h20-logging>`__ for information about debugging and logging on the nRF54H20.

Testing
=======

After programming the sample to your development kit, complete the following steps to test it.

Update the application version
------------------------------

Build the sample application with an updated version number:

.. code-block:: console

    west build -p -b nrf54h20dk_nrf54h20_cpuapp@soc1 -- -DOVERLAY_CONFIG=overlay-bt.conf -DCONFIG_APP_VERSION=2


Note

If you are compiling in Windows and the build is unsuccessful due to the maximum path length limitation, use the following command:

.. code-block:: console

    west build -p -b nrf54h20dk_nrf54h20_cpuapp@soc1 -d C:\nordic-lcs\west_working_dir\build\ -- -DOVERLAY_CONFIG=overlay-bt.conf -DCONFIG_APP_VERSION=2

Another :file:`envelope.suit` file is created after running this command, that contains the updated firmware.


Upload the signed envelope onto your mobile phone
-------------------------------------------------

1. Open the nRF Device Manager app on your mobile phone.

#. Select the device **SUIT SMP Sample**. You should see the following:

    .. figure:: images/suit_smp_select_suit_smp_sample.png
        :alt: Select SUIT SMP Sample

#. From the **SUIT SMP Sample** screen, click on :guilabel:`ADVANCED` in the upper right corner of the app to open a new section called **Images**.

    .. figure:: images/suit_smp_select_advanced.png
        :alt: Select ADVANCED

#. Click on the :guilabel:`READ` button within the **Images** section.

    .. figure:: images/suit_smp_select_image_read.png
        :alt: Select READ from Images

    Observe "Version: 1" printed in the **Images** section of the mobile app.

#. From the **Firmware Upload** section, click on :guilabel:`SELECT FILE` and select the :file:`envelope.suit` file from your mobile device.

    .. figure:: images/suit_smp_select_firmware_select_file.png
        :alt: Select Firmware Upload and Select File

#. The nRF Connect Device Manager app reopens. Click on :guilabel:`UPLOAD` to reveal the **Select Image** menu.

#. From the **Select Image** menu, select :guilabel:`Application Core (0)` and click the :guilabel:`OK` button to upload the :file:`envelope.suit` file.

    Observe an upload progress bar below the "UPLOADING…" text in the **Firmware Upload** section.

    .. figure:: images/suit_smp_firmware_uploading.png
        :alt: Firmware UPLOADING


   The text "UPLOAD COMPLETE" appears in the **Firmware Upload** section once completed.

    .. figure:: images/suit_smp_firmware_upload_complete.png
        :alt: Firmware UPLOAD COMPLETE

#. Reconnect your device.

#. Select the device **SUIT SMP Sample** once again.

    .. figure:: images/suit_smp_images_v2.png
        :alt: Images Version 2

#. Under the **Images** section, click on :guilabel:`READ`.

   Observe "Version: 2" printed in the **Images** section of the mobile app.

   Observe that **LED 1** flashes twice now to indicate Version 2 of the firmware.
