.. _nrf54h_suit_sample:

SUIT DFU: Firmware update on the nRF54H20 SoC
#############################################

.. contents::
   :local:
   :depth: 2

The sample demonstrates how to update and boot the nRF54H20 System on Chip (SoC) using the Software Update for Internet of Things (SUIT) procedure on the Application and Radio Cores of the SoC.
The update on the nRF54H20 SoC can be done over Bluetooth® Low Energy or UART.

Requirements
************

The sample supports the following development kit:

+------------------------+----------+--------------------------------+------------------------------------+
| **Hardware platforms** | **PCA**  | **Board name**                 | **Build target**                   |
+========================+==========+================================+====================================+
| nRF54H20 PDK           | PCA10145 | ``nrf54h20dk_nrf54h20_cpuapp`` | ``nrf54h20dk_nrf54h20_cpuapp@soc1``|
+------------------------+----------+--------------------------------+------------------------------------+

You need the nRF Connect Device Manager app for SUIT update over Bluetooth Low Energy:

* `nRF Device Manager mobile app for Android`_
  (The minimum required version is v1.9.)

* `nRF Device Manager mobile app for iOS`_
  (The minimum required version is v1.5.)

For a SUIT update over UART, you need to install MCUmgr.
MCUmgr is a tool that can be used to upload SUIT envelopes through the SMP protocol.
For more information on MCUmgr installation, see :ref:`zephyr:mcu_mgr`.

Overview
********

The sample uses one of the following to perform an update on the nRF54H20 SoC, featuring Nordic Semiconductor’s implementation of the SUIT procedure:

* Bluetooth Low Energy and the nRF Connect Device Manager app
* UART and MCUmgr

The sample demonstrates how to perform a DFU of the Application and Radio firmware.

SUIT is the only DFU procedure supported for the nRF54H20 SoCs.
To read more about the SUIT procedure, see :ref:`ug_nrf54h20_suit_intro`.

|NCS|-specific SUIT limitations
===============================

The sample and the SUIT DFU procedure currently only support the Application Core and the Radio Core firmware updates.
Additionally, this sample is an early version and is only meant to showcase the start of Nordic Semiconductor’s implementation of the SUIT DFU procedure.

User interface
**************

LED 1:
    The number of blinks indicates the application firmware version.
    The default is set to ``1`` to blink once, indicating "Version 1".

Configuration
*************

|config|

The default configuration uses UART with Version 1.
To change the version of the application, configure the :ref:`CONFIG_APP_VERSION <CONFIG_APP_VERSION>` Kconfig option.
This also changes the number of blinks on **LED 1** and sets the :ref:`sequence number <ug_suit_dfu_suit_manifest_elements>` of the SUIT :ref:`envelope <ug_suit_dfu_suit_concepts>`’s manifest.

To use this configuration, include the ``-DCONFIG_APP_VERSION=x`` option with the build command, where x is the version number.

For example:

.. code-block:: console

   west build -p -b nrf54h20dk_nrf54h20_cpuapp@soc1 -- -DCONFIG_APP_VERSION=2

If you do not specify this configuration, the sample is built with Version 1.

Configuration options
=====================

Check and configure the following configuration option for the sample:

.. _CONFIG_APP_VERSION:

CONFIG_APP_VERSION - Configuration for the application version
   The sample configuration updates the version of the application.
   The default value is ``1``.

Modify partition sizes
======================

You can also modify the size and location of the partitions.
This is done by modifying the values for the desired location and size of the partition in the devicetree :file:`.overlay` files.

* To modify the Application Core’s partition size,  modify the values for ``slot0_partition`` defined in :file:`modules/ic-next/dts/arm/nordic_nrf_next/nrf54h20_cpuapp.dtsi`.

* To modify the DFU partition, modify the values for ``dfu_partition`` defined in :file:`samples/suit/smp_transfer/boards/nrf54h20dk_nrf54h20_cpuapp.overlay`.

    * ``dfu_partition`` is where the update candidate is stored before the update process begins.

Manifest template files
=======================

Manifest template files are automatically created after the sample is built for the first time.
These are required to create a SUIT envelope to carry out the DFU.

If you want to make modifications to how the DFU is executed in this sample, you can do so by editing the manifest templates.
See the :ref:`ug_nrf54h20_suit_customize_dfu` user guide for instructions and examples.

Building and running
********************

.. |sample path| replace:: :file:`samples/suit/smp_transfer`

This sample can be found under |sample path| in the |NCS| folder structure.

Building and programming using the command line
===============================================

To build and program the sample to the nRF54H20 PDK, complete the following steps:

.. tabs::

   .. group-tab:: Over Bluetooth Low Energy

      1. Open a terminal window in |sample path|.
      #. Build the sample using the following ``west`` command, with the following Kconfig options set:

         .. code-block:: console

            west build -p -b nrf54h20dk_nrf54h20_cpuapp@soc1 -- -DOVERLAY_CONFIG=overlay-bt.conf -DCONFIG_APP_VERSION=1

         .. note::

            If you are compiling in Windows and the build is unsuccessful due to the maximum path length limitation, use the following command:

            .. code-block:: console

               west build -p -b nrf54h20dk_nrf54h20_cpuapp@soc1 -d C:\ncs-lcs\west_working_dir\build\ -- -DOVERLAY_CONFIG=overlay-bt.conf -DCONFIG_APP_VERSION=1

         If you want to further configure your sample, see :ref:`configure_application` for additional information.

         After running the ``west build`` command, the output build files can be found in the :file:`build/zephyr` directory.
         The following build artifacts are found with both firmware binaries embedded as integrated payloads:

          * :file:`root.suit` - This file is the most important envelope, containing embedded :file:`app.suit` and :file:`hci_rpmsg_subimage.suit` as an integrated dependencies.
            These files are used to execute DFU.
          * :file:`app.suit` - This file is the envelope for the Application Core, containing embedded Application Core firmware as an integrated payload.
          * :file:`hci_rpmsg_subimage.suit` - This file is the envelop for the Radio Core, containing embedded radio core firmware as an integrated payload.

         For more information about files generated as output of the build process, see :ref:`app_build_output_files`.
         For more information on the contents of the build directory, see :ref:`zephyr:build-directory-contents` in the Zephyr documentation.

      #. Connect the PDK to your computer using a USB cable.
      #. Power on the PDK.
      #. Program the sample to the kit using the following command:

         .. code-block:: console

            west flash

         .. note::

            If you are compiling in Windows and the build is unsuccessful due to the maximum path length limitation, use the following command:

            .. code-block:: console

               west flash -d C:\ncs-lcs\west_working_dir\build\

   .. group-tab:: Over UART

      1. Open a terminal window in |sample path|.
      #. Build the sample:

         .. code-block:: console

             west build -p -b nrf54h20dk_nrf54h20_cpuapp@soc1

         .. note::

            If you are compiling in Windows and the build is unsuccessful due to the maximum path length limitation, use the following command:

            .. code-block:: console

               west build -p -b nrf54h20dk_nrf54h20_cpuapp@soc1 -d C:\ncs-lcs\west_working_dir\build\

         If you want to further configure your sample, see :ref:`configure_application` for additional information.

         After running the ``west build`` command, the output build files can be found in the :file:`build/zephyr` directory.
         The following build artifacts are found with both firmware binaries embedded as integrated payloads:

          * :file:`root.suit` - This file is the most important envelope, containing embedded :file:`app.suit` as an integrated dependency.
            This file is used to execute DFU.
          * :file:`app.suit` - This file is the envelope for the Application Core, containing embedded Application Core firmware as an integrated payload.

         For more information about files generated as output of the build process, see :ref:`app_build_output_files`.
         For more information on the contents of the build directory, see :ref:`zephyr:build-directory-contents` in the Zephyr documentation.

      #. Connect the PDK to your computer using a USB cable.
      #. Power on the PDK.
      #. Program the sample to the kit using the following command:

         .. code-block:: console

            west flash

         .. note::

            If you are compiling in Windows and the build is unsuccessful due to the maximum path length limitation, use the following command:

            .. code-block:: console

               west flash -d C:\ncs-lcs\west_working_dir\build\

Building and programming using |nRFVSC|
=======================================

To build the sample using |VSC|, follow the steps listed on the `How to build an application`_ page in the |nRFVSC| documentation.

Testing
=======

After programming the sample to your development kit, complete the following steps to test it.

.. tabs::

   .. group-tab:: Over Bluetooth Low Energy

      1. **Update the application version:**

         Build the sample with an updated version number:

         .. code-block:: console

            west build -p -b nrf54h20dk_nrf54h20_cpuapp@soc1 -- -DOVERLAY_CONFIG=overlay-bt.conf -DCONFIG_APP_VERSION=2


         .. note::

            If you are compiling in Windows and the build is unsuccessful due to the maximum path length limitation, use the following command:

            .. code-block:: console

               west build -p -b nrf54h20dk_nrf54h20_cpuapp@soc1 -d C:\ncs-lcs\west_working_dir\build\ -- -DOVERLAY_CONFIG=overlay-bt.conf -DCONFIG_APP_VERSION=2

         Another :file:`root.suit` file is created after running this command, that contains the updated firmware.


      #. **Upload the signed envelope onto your mobile phone:**

         a. Open the nRF Device Manager app on your mobile phone.
         #. Select the device **SUIT SMP Sample**. You should see the following:

            .. figure:: images/suit_smp_select_suit_smp_sample.png
               :alt: Select SUIT SMP Sample

         #. From the **SUIT SMP Sample** screen, on the **Images** tab at the bottom of the screen, click on :guilabel:`ADVANCED` in the upper right corner of the app to open a new section called **Images**.

            .. figure:: images/suit_smp_select_advanced.png
               :alt: Select ADVANCED

         #. Click on the :guilabel:`READ` button within the **Images** section.

            .. figure:: images/suit_smp_select_image_read.png
               :alt: Select READ from Images

            Observe "Version: 1" printed in the **Images** section of the mobile app.

         #. From the **Firmware Upload** section, click on :guilabel:`SELECT FILE` and select the :file:`root.suit` file from your mobile device.

            .. figure:: images/suit_smp_select_firmware_select_file.png
               :alt: Select Firmware Upload and Select File

         #. Click on :guilabel:`UPLOAD` to reveal the **Select Image** menu.
         #. From the **Select Image** menu, select :guilabel:`Application Core (0)` and click the :guilabel:`OK` button to upload the :file:`root.suit` file.

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

            Observe that **LED 1** flashes twice now to indicate "Version 2" of the firmware.

   .. group-tab:: Over UART

      1. **Update the application version:**

         Build the sample with an updated version number:

         .. code-block:: console

            west build -p -b nrf54h20dk_nrf54h20_cpuapp@soc1 -- -DCONFIG_APP_VERSION=2

         .. note::

            If you are compiling in Windows and the build is unsuccessful due to the maximum path length limitation, use the following command:

            .. code-block:: console

               west build -p -b nrf54h20dk_nrf54h20_cpuapp@soc1 -d C:\ncs-lcs\west_working_dir\build\ -- -DCONFIG_APP_VERSION=2

         Another :file:`root.suit` file is created after running this command, that contains the updated firmware.

      #. **Upload the signed envelope:**

         a. Read the version and digest of the installed root manifest with MCUmgr:

            .. code-block:: console

               mcumgr --conntype serial --connstring "dev=/dev/ttyACM0,baud=115200,mtu=512" image list

            Observe an output similar to the following is logged on UART:

            .. parsed-literal::
               :class: highlight

               image=0 slot=0
                  version: 1
                  bootable: true
                  flags: active confirmed permanent
                  hash: d496cdc8fa4969d271204e8c42c86c7499ae8632f131e098e2e0fb5c7bbe3a5f
               Split status: N/A (0)

         #. Upload the image with MCUmgr:

            .. code-block:: console

               mcumgr --conntype serial --connstring "dev=/dev/ttyACM0,baud=115200,mtu=512" image upload root.suit

            Observe an output similar to the following is logged on UART:

            .. parsed-literal::
               :class: highlight

               0 / 250443 [---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------]   0.00%
               18.99 KiB / 244.57 KiB [============>-------------------------------------------------------------------------------------------------------------------------------------------------]   7.76% 11.83 KiB/s 00m19s
               66.56 KiB / 244.57 KiB [==========================================>-------------------------------------------------------------------------------------------------------------------]  27.21% 18.44 KiB/s 00m09s
               112.12 KiB / 244.57 KiB [=======================================================================>-------------------------------------------------------------------------------------]  45.84% 19.97 KiB/s 00m06s
               154.08 KiB / 244.57 KiB [==================================================================================================>----------------------------------------------------------]  63.00% 20.22 KiB/s 00m04s
               197.40 KiB / 244.57 KiB [==============================================================================================================================>------------------------------]  80.71% 20.51 KiB/s 00m02s
               241.16 KiB / 244.57 KiB [=================================================================================================================================================================>--]  98.60% 20.74 KiB/s
               Done

         #. Read the version and digest of the uploaded root manifest with MCUmgr:

            .. code-block:: console

               mcumgr --conntype serial --connstring "dev=/dev/ttyACM0,baud=115200,mtu=512" image list


            Observe an output similar to the following is logged on UART:

            .. parsed-literal::
               :class: highlight

               image=0 slot=0
                  version: 2
                  bootable: true
                  flags: active confirmed permanent
                  hash: 707efbd3e3dfcbda1c0ce72f069a55f35c30836b79ab8132556ed92ce609f943
               Split status: N/A (0)

            Observe that **LED 1** flashes twice now to indicate "Version 2" of the firmware.
