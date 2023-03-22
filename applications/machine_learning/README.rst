.. _nrf_machine_learning_app:

nRF Machine Learning
####################

.. contents::
   :local:
   :depth: 2

The nRF Machine Learning application is an out of the box reference design of an `embedded machine learning`_ using `Edge Impulse`_.
The application gathers data from sensors, forwards data to the Edge Impulse platform, and runs the machine learning model.
It also displays results of the machine learning model on LEDs.
The Edge Impulse platform collects data from sensors, trains machine learning model, and deploys the model to your Nordic Semiconductor's device.
To learn more about Edge Impulse support in the |NCS| see :ref:`ug_edge_impulse`.

Requirements
************

The application supports the following development kits:

.. table-from-sample-yaml::

The available configurations use the built-in sensors, sensors connected with additional shield, or the simulated sensor signal.

.. include:: /includes/tfm.txt

Overview
********

To perform its tasks, the nRF Machine Learning application uses components available in Zephyr and the |NCS|, namely the :ref:`lib_caf` modules and :ref:`zephyr:sensor_api` for sampling sensors, :ref:`zephyr:uart_api` or :ref:`nus_service_readme` for forwarding data, and :ref:`peripheral_status` for presenting gesture detection results over Bluetooth® LE.
It also uses the `Edge Impulse's data forwarder`_ protocol.

Sampling sensors
================

The application handles the sensor sampling using the :ref:`caf_sensor_manager`.
This module uses Zephyr's :ref:`zephyr:sensor_api` to handle the sampling.
This approach allows to use any sensor available in Zephyr.

By default, the following sensors are used by the application:

* Thingy:53 - Built-in accelerometer (``ADXL362``).
* nRF52840 Development Kit - Simulated sensor (:ref:`sensor_sim`).
  The simulated sensor generates predefined waves as acceleration.
  This development kit does not have a built-in accelerometer.
* nRF5340 Development Kit - Simulated sensor (:ref:`sensor_sim`).
  The simulated sensor generates predefined waves as acceleration.
  This development kit does not have a built-in accelerometer.
* nRF54H20 :term:`Preview Development Kit (PDK)` - Accelerometer (``ADXL362``) connected to the PDK with the sensor hub shield.
  This PDK does not have a built-in accelerometer.

Sensor HUB shield
-----------------

The sensor hub shield is an extension for the nRF54H20 PDK containing ``ADXL362`` accelerometer.
This enables you to test the multicore application targeting the nRF54H20 PDK.

.. figure:: /images/sensor_hub_shield.png
   :width: 350px
   :align: center
   :alt: Sensor Hub shield

   Sensor Hub shield

Pin assignment
^^^^^^^^^^^^^^

For the exact pin assignment, refer to the following table:

+-------------------------+----------+-----------------+
| nRF54H20 connector pin  | SIGNAL   | Shield function |
+=========================+==========+=================+
| P1.02                   | SPI CS   | Chip Select     |
+-------------------------+----------+-----------------+
| P2.07                   | SPI MOSI | Serial Data In  |
+-------------------------+----------+-----------------+
| P2.10                   | SPI MISO | Serial Data Out |
+-------------------------+----------+-----------------+
| P2.01                   | SPI SCK  | Serial Clock    |
+-------------------------+----------+-----------------+
| P1.08                   | GPIO     | Interrupt 1     |
+-------------------------+----------+-----------------+
| P1.09                   | GPIO     | Interrupt 2     |
+-------------------------+----------+-----------------+

Shield is an add-on that you can attach to the nRF54H20 PDK to extend its features and functionalities.

If you want to run the application on the nRF54H20 PDK, you need to attach the sensor hub shield to it.
To forward the data over Nordic UART Service (NUS), you need an additional development kit that is able to run the :ref:`central_uart` sample or the `nRF Connect for Mobile`_ application on your smartphone or tablet.

The sensor shield must be connected throughout the adapter board to the **PORT_P1** connector of the PDK.
The connector of the shield must be placed above the **PORT_P2**.
The shield must be oriented so its buttons are close to the bottom of the PDK.
The shield must not be rotated.
The orientation of the Nordic logo on the shield and PDK must be the same.

.. figure:: /images/sensor_hub_with_shield.png
   :width: 350px
   :align: center
   :alt: Shield connected to the PDK

   Shield connected to the PDK

Forwarding data
===============

The application uses `Edge Impulse's data forwarder`_ protocol to forward data to `Edge Impulse studio`_.
By default, the following transports are used:

* Thingy:53 uses :ref:`nus_service_readme`.
* nRF52840 Development Kit uses :ref:`zephyr:uart_api`.
* nRF5340 Development Kit uses :ref:`zephyr:uart_api`.
* nRF54H20 PDK uses :ref:`nus_service_readme`.

Machine learning model
======================

The application handles the machine learning model using the :ref:`ei_wrapper` library available in the |NCS|.
The model performs the classification task by assigning a label to input data.
The labels that are assigned by the machine learning model are specific to the given model.

By default, the application uses pretrained machine leaning models deployed in `Edge Impulse studio`_:

* Both Thingy:53 and the nRF54H20 PDK use the `NCS hardware accelerometer machine learning model`_.
  The model uses the data from the accelerometer to recognize the following gestures:

  * ``idle`` - The device is placed on a flat surface.
  * ``updown`` - The device is moved in updown direction.
  * ``rotate`` - The device is rotated.
  * ``tap`` - The device is tapped while placed on a flat surface.

  Unknown gestures, such as shaking the device, are recognized as anomaly.
* Both the nRF52840 Development Kit and nRF5340 Development Kit use the `NCS simulated sensor machine learning model`_.
  The model uses simulated sensor data to recognize the following simulated wave types:

  * ``sine``
  * ``triangle``
  * ``idle``

  The ``square`` wave signal can also be generated by the simulated sensor.
  This signal is unknown to the machine learning model and therefore it is marked as anomaly.

The application displays LED effects that correspond to the machine learning results.
For more detailed information, see the `User interface`_ section.

Power management
================

Reducing power consumption is important for every battery-powered device.

In the nRF Machine Learning application, application modules are automatically suspended or turned off if the device is not in use for a predefined period.
The application uses :ref:`caf_power_manager` for this purpose.
This means that Zephyr power management is forced to the :c:enumerator:`PM_STATE_ACTIVE` state when the device is in either the Power management active or the Power management suspended state, but the power off state is forced directly by :ref:`caf_power_manager` as Zephyr's :c:enumerator:`PM_STATE_SOFT_OFF` state.

* In the :c:enumerator:`POWER_MANAGER_LEVEL_ALIVE` state, the device is in working condition, Bluetooth is advertising whenever required and all the connections are maintained.
* In the :c:enumerator:`POWER_MANAGER_LEVEL_SUSPENDED` state, the device maintains the active Bluetooth connection.
* In the :c:enumerator:`POWER_MANAGER_LEVEL_OFF` state, the CPU is switched to the off mode.

In the suspended and OFF states, most of the functionalities are disabled.
For example, LEDs and sensors are turned off and Bluetooth advertising is stopped.

Any button press can wake up the device.

For the Thingy:53, the sensor supports a trigger that can be used for active power management.
As long as the device detects acceleration, the board is kept in the active state.
When the board is in the :c:enumerator:`POWER_MANAGER_LEVEL_SUSPENDED` state, it can be woken up by acceleration threshold by moving the device.

You can define the time interval after which the peripherals are suspended or powered off in the :kconfig:option:`CONFIG_CAF_POWER_MANAGER_TIMEOUT` option.
By default, this period is set to 120 seconds.

.. _nrf_machine_learning_app_architecture:

Firmware architecture
=====================

The nRF Machine Learning application has a modular structure, where each module has a defined scope of responsibility.
The application uses the :ref:`app_event_manager` to distribute events between modules in the system.

The following figure shows the application architecture.
The figure visualizes relations between Application Event Manager, modules, drivers, and libraries.

.. figure:: /images/ml_app_architecture.svg
   :alt: nRF Machine Learning application architecture

   nRF Machine Learning application architecture

Since the application architecture is uniform and the code is shared, the set of modules in use depends on configuration.
In other words, not all of the modules need to be enabled for a given reference design.
For example, the :ref:`caf_ble_state` and :ref:`caf_ble_adv` modules are not enabled if the configuration does not use Bluetooth®.

See :ref:`nrf_machine_learning_app_internal_modules` for detailed information about every module used by the nRF Machine Learning application.

Firmware architecture for nRF54H20 PDK
--------------------------------------

On the nRF54H20 PDK, the architecture of the nRF Machine Learning application is split in two, as part of the application is running on a separate processor (PPR).
PPR handles the sensor sampling and sending the data to the application processor.

The following figure shows the application architecture for the nRF54H20 PDK, visualizing the relations between the Application Event Manager, modules, drivers, and libraries.

.. figure:: /images/ml_app_architecture_hcs.svg
   :alt: nRF Machine Learning application architecture for the nRF54H20 PDK

   The architecture of the nRF Machine Learning application for the nRF54H20 PDK

Programming Thingy:53
=====================

|thingy53_sample_note|

Programming nRF53 DK
====================

If you build this application for the nRF53 DK, it enables additional features similar to the ones that are enabled for Thingy:53:

* MCUboot bootloader with serial recovery and multi-image update.
* Static configuration of :ref:`partition_manager`.
* DFU over-the-air using Simple Management Protocol over Bluetooth.

See :ref:`ug_thingy53` for detailed information about the mentioned features.

The nRF53 DK has a J-Link debug IC that can be used to program the firmware.
Alternatively, firmware can be updated over MCUBoot serial recovery or DFU over-the-air using Simple Management Protocol over Bluetooth.
Keep in mind that if you use bootloader to update firmware, the new firmware must be compatible with used bootloader and partition map.

The nRF53 Development Kit uses RTT as logger's backend.
The RTT logs can be easily accessed, because the Development Kit has a built-in SEGGER chip.

Programming nRF54H20 PDK
========================

The nRF54H20 PDK has a J-Link debug IC that can be used to program the firmware.

When building an application for the nRF54H20 PDK, you are building all domain images at once.
During this process, the following images are built:

* Application core application (:file:`<build_dir>/zephyr/zephyr.hex`)
* PPR core application (:file:`<build_dir>/remote/zephyr/zephyr.hex`)
* Secure Domain firmware (:file:`<build_dir>/secdom/src/secdom-build/zephyr.hex`)
* System Controller firmware (:file:`<build_dir>/sysctrl/src/sysctrl-build/zephyr.hex`)
* Radio core firmware (:file:`<build_dir>/hci_rpmsg/zephyr/zephyr.hex`)

Additionally, the following UICR contents are generated for setup access for domains:

* System Controler UICR (:file:`<build_dir>/sysctrl/src/sysctrl-build/uicr.hex`)
* Application UICR (:file:`<build_dir>/zephyr/uicr.hex`)
* Radio UICR (:file:`<build_dir>/hci_rpmsg/zephyr/uicr.hex`)

All of the HEX files need to be flashed into the device.

See the `Building and running`_ section for more information.

Custom model requirements
=========================

The default application configurations rely on pretrained machine learning models that can be automatically downloaded during the application build.
If you want to train and deploy a custom machine learning model using `Edge Impulse Studio`_, you need a user account for the Edge Impulse Studio web-based tool.
The user account is not needed to perform predictions using the pretrained models.

Data forwarding requirements
============================

To forward the collected data using `Edge Impulse's data forwarder`_, you must install the Edge Impulse CLI.
See `Edge Impulse CLI installation guide`_ for instructions.

Nordic UART Service requirements
--------------------------------

If you want to forward data over Nordic UART Service (NUS), you need an additional development kit that is able to run the :ref:`central_uart` sample.
Check the sample Requirements section for the list of supported development kits.
The sample is used to receive data over NUS and forward it to the host computer over UART.
See `Testing with Thingy:53`_ for how to test this solution.

.. _nrf_machine_learning_app_requirements_build_types:

nRF Machine Learning build types
================================

The nRF Machine Learning application does not use a single :file:`prj.conf` file.
Configuration files are provided for different build types for each supported board.

Each board has its own :file:`prj.conf` file, which represents a ``debug`` build type.
Other build types are covered by dedicated files with the build type added as a suffix to the ``prj`` part, as per the following list.
For example, the ``release`` build type file name is :file:`prj_release.conf`.
If a board has other configuration files, for example associated with partition layout or child image configuration, these follow the same pattern.

.. include:: /gs_modifying.rst
   :start-after: build_types_overview_start
   :end-before: build_types_overview_end

Before you start testing the application, you can select one of the build types supported by nRF Machine Learning application, depending on your development kit and the building method.
The application supports the following build types:

* ``debug`` -- Debug version of the application - can be used to verify if the application works correctly.
* ``release`` -- Release version of the application - can be used to achieve better performance and reduce memory consumption.

Not every board supports both mentioned build types.
The given board can also support some additional configurations of the nRF Machine Learning application.
For example, the nRF52840 Development Kit supports ``nus`` configuration that uses :ref:`nus_service_readme` instead of :ref:`zephyr:uart_api` for data forwarding.
Additionally, the nRF54H20 PDK supports the ``singlecore`` configuration that does not use Peripheral processor (PPR) for data sampling.
Data are collected with the application CPU instead.

.. note::
    `Selecting a build type`_ is optional.
    The ``debug`` build type is used by default if no build type is explicitly selected.

User interface
**************

The application supports a simple user interface.
You can control the application using predefined buttons, while LEDs are used to display information.

Buttons
=======

The application supports a button that is used to switch between data forwarding and running the machine learning model.
You can trigger the change by pressing and holding the button for longer than 5 seconds.

.. note::
   If a given configuration supports Bluetooth, then two sequences of the button press and release in a short time interval, shortly after application boot, removes the Bluetooth bonds.

If the application uses the simulated sensor signal, you can use another button to change signal generated by the simulated sensor.
The change is triggered by any press of the button.

By default, the following buttons are used by the application:

* Thingy:53:

  * The **SW3** button switches between data forwarding and running the machine learning model and removes Bluetooth bonds.

* nRF54H20 PDK:

  * **Button 1** switches between data forwarding and running the machine learning model and removes Bluetooth bonds.

* nRF52840 and nRF5340 Development Kit:

  * **Button 1** switches between data forwarding and running a machine learning model.
  * **Button 3** changes the signal generated by the simulated sensor.

LEDs
====

The application uses one LED to display the application state.
The LED displays either the state of data forwarding or the machine learning prediction results.
You can configure the LED effect in the application configuration files.

If the application uses the simulated sensor signal, it uses another LED to display the effect that represents the signal generated by the simulated sensor.
The application defines common LED effects for both the machine learning results and the simulated sensor signal.

By default, the application uses the following LED effects:

* Thingy:53 displays the application state in the RGB scale using **LED1**.

  * If the device is returning the machine learning prediction results, the LED uses following predefined colors:

    * Red - ``rotate``
    * Green - ``updown``
    * Blue - ``tap``
    * Purple - Anomaly

    If the machine learning model is running, but it has not detected anything yet or the ``idle`` state is detected, the LED is blinking.
    After a successful detection, the LED is set to the predefined color.
    The LED effect is overridden on the next successful detection.
  * If the device forwards data, the LED color turns red and uses the following blinking patterns:

    * LED blinks slowly if it is not connected.
    * LED blinks with an average frequency if it is connected, but is not actively forwarding data.
    * LED blinks rapidly if it is connected and is actively forwarding data.

* Both nRF5340 Development Kit and nRF52840 Development Kit use monochromatic LEDs to display the application state.
  The **LED1** displays the application state and the **LED2** displays the signal generated by the simulated sensor.

  * If the device is returning the machine learning prediction results, the **LED1** blinks for a predefined number of times and then turns off for a period of time.
    Then the sequence is repeated.
    The machine learning result is represented by the number of blinks:

    * 1 blink - ``sine``
    * 2 blinks - ``triangle``
    * 3 blinks - ``square``
    * 4 blinks - ``idle``

    If the machine learning model is running, but it has not detected anything yet or it has detected an anomaly, the **LED1** is breathing.
  * If the device forwards data, the **LED1** uses the following blinking patterns:

    * LED blinks slowly if it is not connected.
    * LED blinks with an average frequency if it is connected, but is not actively forwarding data.
    * LED blinks rapidly if it is connected and is actively forwarding data.

* nRF54H20 PDK uses monochromatic LEDs to display the application state.
  **LED1**, **LED2** and **LED3** display the application state.

  * If the device is returning the machine learning prediction results, a pattern of LEDs are turn on.
    The machine learning result is represented by:

    * **LED1** - ``updown``
    * **LED2** - ``rotate``
    * **LED3** - ``tap``
    * **LED2** and **LED3** - Anomaly

    If the machine learning model is running, but it has not detected anything yet or the ``idle`` state is detected, the **LED1**, **LED2**, and **LED3**, keep blinking.
  * If the device forwards data, **LED1** uses the following blinking patterns:

    * LED blinks slowly if it is not connected.
    * LED blinks with an average frequency if it is connected, but is not actively forwarding data.
    * LED blinks rapidly if it is connected and is actively forwarding data.

.. _nrf_machine_learning_app_configuration:

Configuration
*************

The nRF Machine Learning application is modular and event driven.
You can enable and configure the modules separately for selected board and build type.
See the documentation page of selected module for information about functionalities provided by the module and its configuration.
See :ref:`nrf_machine_learning_app_internal_modules` for list of modules available in the application.

Configuration files
===================

The nRF Machine Learning application uses the following files as configuration sources:

* Devicetree Specification (DTS) files - These reflect the hardware configuration.
  See :ref:`zephyr:dt-guide` for more information about the DTS data structure.
* Kconfig files - These reflect the software configuration.
  See :ref:`kconfig_tips_and_tricks` for information about how to configure them.
* :file:`_def` files - These contain configuration arrays for the application modules.
  The :file:`_def` files are used by the nRF Machine Learning application modules and :ref:`lib_caf` modules.

The application configuration files for a given board must be defined in a board-specific directory in the :file:`applications/machine_learning/configuration/` directory.
For example, the configuration files for the Thingy:53 are defined in the :file:`applications/machine_learning/configuration/thingy53_nrf5340_cpuapp` directory.

The following configuration files can be defined for any supported board:

* :file:`prj_build_type.conf` - Kconfig configuration file for a build type.
  To support a given build type for the selected board, you must define the configuration file with a proper name.
  For example, the :file:`prj_release.conf` defines configuration for ``release`` build type.
  The :file:`prj.conf` without any suffix defines the ``debug`` build type.
* :file:`app.overlay` - DTS overlay file specific for the board.
  Defining the DTS overlay file for a given board is optional.
* :file:`_def` files - These files are defined separately for modules used by the application.
  You must define a :file:`_def` file for every module that requires it and enable it in the configuration for the given board.
  The :file:`_def` files that are common for all the boards and build types are located in the :file:`applications/machine_learning/configuration/common` directory.

Advertising configuration
-------------------------

If a given build type enables Bluetooth, the :ref:`caf_ble_adv` is used to control the Bluetooth advertising.
This CAF module relies on :ref:`bt_le_adv_prov_readme` to manage advertising data and scan response data.
The nRF Machine Learning application configures the data providers in :file:`src/util/Kconfig`.
By default, the application enables a set of data providers available in the |NCS| and adds a custom provider that appends UUID128 of Nordic UART Service (NUS) to the scan response data if the NUS is enabled in the configuration and the Bluetooth local identity in use has no bond.

Multi-image builds
------------------

The Thingy:53, nRF53 DK and, nRF54H20 PDK can use the multi-image building with the following child images:

* MCUboot bootloader
* Bluetooth HCI RPMsg
* Peripheral processor (only for the nRF54H20 PDK)

You can define the application-specific configuration for the mentioned child images in the board-specific directory in the :file:`applications/machine_learning/configuration/` directory.
The Kconfig configuration file should be located in subdirectory :file:`child_image/child_image_name` and its name should match the application Kconfig file name, that is contain the build type if necessary
For example, the :file:`applications/machine_learning/configuration/thingy53_nrf5340_cpuapp/child_image/hci_rpmsg/prj.conf` file defines configuration of Bluetooth HCI RPMsg for ``debug`` build type on ``thingy53_nrf5340_cpuapp`` board, while the :file:`applications/machine_learning/configuration/thingy53_nrf5340_cpuapp/child_image/hci_rpmsg/prj_release.conf` file defines configuration of Bluetooth HCI RPMsg for ``release`` build type.
See :ref:`ug_multi_image` for detailed information about multi-image builds and child image configuration.

The nRF54H20 PDK uses a multi-image build to build an additional image for the PPR core.
The source files, devicetree, and Kconfig configuration files are located in the :file:`applications/machine_learning/remote` directory.

.. _nrf_machine_learning_app_configuration_build_types:

Building and running
********************

.. |sample path| replace:: :file:`applications/machine_learning`

The nRF machine learning application is built the same way to any other |NCS| application or sample.
Building the default configurations requires an Internet connection, because the machine learning model source files are downloaded from web during the application build.

.. tabs::

   .. tab:: nRF54H20 PDK

      When building for the nRF54H20 PDK for the single-core variant and invoking ``west build`` or ``cmake`` in the Machine Learning application, add the ``-DCONF_FILE=prj_singlecore.conf`` and ``-DSHIELD=sensor_hub`` command line options.

      When building for the nRF54H20 board with an additional PPR core, add the ``-DSHIELD=sensor_hub`` and ``-Dremote_SHIELD=sensor_hub`` command line options.

      To build the application for the nRF54H20 PDK with the sensor sampling done by the PPR core (dual-core application), run the following command:

      .. code-block:: console

         west build -b nrf54h20dk_nrf54h20_cpuapp@soc1 -- -DSHIELD=sensor_hub -Dremote_SHIELD=sensor_hub

      To build the application for the nRF54H20 DK with the sensor sampling done by the Application core (single-core application), run the following command:

      .. code-block:: console

         west build -b nrf54h20dk_nrf54h20_cpuapp@soc1 -- -DCONF_FILE=prj_singlecore.conf -DSHIELD=sensor_hub

      Flash the sample using the standard |NCS| flash command:

      .. code-block:: console

         west flash

      The command flashes all required binaries to the PDK from the build directory, then resets and runs the application.

      For more information about working with the nRF54H20 PDK, see the :ref:`ug_nrf54h` documentation.

   .. tab:: nRF53 DK and Thingy:53

      .. include:: /includes/build_and_run_ns.txt

Selecting a build type
======================

Before you start testing the application, you can select one of the :ref:`nrf_machine_learning_app_requirements_build_types`, depending on your development kit and building method.

Selecting a build type in |VSC|
-------------------------------

.. include:: /gs_modifying.rst
   :start-after: build_types_selection_vsc_start
   :end-before: build_types_selection_vsc_end

Selecting a build type from command line
----------------------------------------

.. include:: /gs_modifying.rst
   :start-after: build_types_selection_cmd_start
   :end-before: build_types_selection_cmd_end

.. note::
   If the selected board does not support the selected build type, the build is interrupted.
   For example, if the ``nus`` build type is not supported by the selected board, the following notification appears:

   .. code-block:: console

      Configuration file for build type ``nus`` is missing.

Providing API key
=================

If the URI of the Edge Impulse zip file requires providing an additional API key, you can provide it using the following CMake definition: :c:macro:`EI_API_KEY_HEADER`.
This definition is set in a similar way as selected build type.
For more detailed information about building the machine learning model in the |NCS|, see :ref:`ug_edge_impulse`.

.. tip::
   The nRF Machine Learning application configurations available in the |NCS| do not require providing an API key to download the model.
   The model is downloaded from the web, but no authentication is required.

Testing
=======

After programming the application to your development kit, you can test the nRF Machine Learning application.
You can test running the machine learning model on an embedded device and forwarding data to `Edge Impulse studio`_.
The detailed test steps for the Development Kits and the Thingy:53 are described in the following subsections.

Application logs
----------------

In most of the provided debug configurations, the application provides logs through the RTT.
See :ref:`testing_rtt_connect` for detailed instructions about accessing the logs.

.. note::
   The Thingy:53 in the ``debug`` configuration provides logs through the USB CDC ACM serial.
   See :ref:`ug_thingy53` for detailed information about working with the Thingy:53.

   You can also use ``rtt`` configuration to have the Thingy:53 use RTT for logs.

.. note::
   The nRF54H20 PDK in the ``debug`` configuration provides logs through the UART.
   For detailed information on working with the nRF54H20 PDK, see the :ref:`ug_nrf54h20_gs` documentation.

Testing with Thingy:53
----------------------

After programming the application, perform the following steps to test the nRF Machine Learning application on the Thingy:

1. Turn on the Thingy.
   The application starts in a mode that runs the machine learning model.
   The RGB LED is blinking, because no gesture has been recognized by the machine learning model yet.
#. Tap the device.
   The ``tap`` gesture is recognized by the machine learning model.
   The LED color changes to blue and the LED stays turned on.
#. Move the device up and down.
   The ``updown`` gesture is recognized by the machine learning model.
   The LED color changes to green and the LED stays turned on.
#. Rotate the device.
   The ``rotate`` gesture is recognized by the machine learning model.
   The LED color changes to red and the LED stays turned on.
#. Shake the device.
   The machine learning model detects an anomaly.
   The LED color changes to purple and the LED stays turned on.
#. Press and hold the button for more than 5 seconds to switch to the data forwarding mode.
   After the mode is switched, the LED color changes to red and the LED starts blinking very slowly.
#. Program the :ref:`central_uart` sample to a compatible development kit, for example the nRF52840 Development Kit.
#. Turn on the programmed device.
   After a brief delay, the Bluetooth® connection between the sample and the Thingy is established.
   The Thingy forwards the sensor readouts over NUS.
   The LED on the Thingy starts to blink rapidly.
#. Connect to the Bluetooth® Central UART sample with a terminal emulator (for example, PuTTY).
   See :ref:`putty` for the required settings.
#. Observe the sensor readouts represented as comma-separated values.
   Every line represents a single sensor readout.
   The Thingy forwards sensor readouts over NUS to the Central UART sample.
   The sample forwards the data to the host over UART.
#. Turn off PuTTY to ensure that only one program has access to data on UART.

Optionally, you can also connect to the device using `Edge Impulse's data forwarder`_ and forward data to `Edge Impulse studio`_ (after logging in).
See `Forwarding data to Edge Impulse studio`_ for details.

Testing with the nRF52840 or nRF53 DK
-------------------------------------

After programming the application, perform the following steps to test the nRF Machine Learning application on the Development Kit:

1. Turn on the development kit.
   The application starts in a mode that runs the machine learning model.
   Initially, **LED2** displays the LED effect representing ``sine`` wave (1 blink) and **LED1** is breathing, because the signal was not yet recognized by the machine learning model.
   After a brief delay, the machine learning model recognizes the simulated signal.
   **LED1** and **LED2** display the same LED effect.
#. Press **Button 3** to change the generated acceleration signal.
   Right after the signal change, effects displayed by LEDs are different.
   After a brief delay, the machine learning model recognizes the ``triangle`` wave and the same effect (2 blinks) is displayed by both LEDs.
#. Press **Button 3** to again change generated acceleration signal.
   The ``square`` wave (3 blinks) is displayed only by the **LED2**.
   This signal is marked as anomaly by the machine learning model and **LED1** starts breathing.
#. Press and hold **Button 1** for more than 5 seconds to switch to the data forwarding mode.
   After the mode is switched, **LED1** starts to blink rapidly.
#. Connect to the development kit with a terminal emulator (for example, PuTTY).
   See :ref:`putty` for the required settings.
#. Observe the sensor readouts represented as comma-separated values.
   Every line represents a single sensor readout.
#. Turn off PuTTY to ensure that only one program will access data on UART.

Optionally, you can also connect to the device using `Edge Impulse's data forwarder`_ and forward data to `Edge Impulse studio`_ (after logging in).
See `Forwarding data to Edge Impulse studio`_ for details.

Testing with the nRF54H20 PDK
-----------------------------

After programming the application, perform the following steps to test the nRF Machine Learning application on the PDK:

1. Turn on your PDK.
   The application starts in a mode that runs the machine learning model.
   **LED1**, **LED2**, and **LED3** are blinking because no gesture has been recognized by the machine learning model yet.
#. Move the device up and down.
   The ``updown`` gesture is recognized by the machine learning model.
   **LED1** turns on for some time.
#. Rotate the device.
   The ``rotate`` gesture is recognized by the machine learning model.
   **LED2** turns on for some time.
#. Tap the device.
   The ``tap`` gesture is recognized by the machine learning model.
   **LED3** turns on for some time.
#. Shake the device.
   The machine learning model detects an anomaly.
   **LED2** and **LED3** turn on for some time.
#. Gesture detection results can also be read using the Nordic Status Message Service.
   Start the `nRF Connect for Mobile`_ application on your smartphone or tablet.
#. Connect to the device from the application.
   The device is advertising as ``Sensor HUB``.
   The services of the connected device are shown.
   If the device can not be found - the PDK might be connected to the other device.
   Terminate the connection with the other device, refresh the scanning and connect to the PDK.
   If the device disconnects shortly after being connected - the PDK might have saved bonding data from previous connection.
   The bonding data can be erased if you double-click the **Button 1** within 3 seconds after the application reboots.
#. Find **Nordic Status Message Service** by its name or UUID (57a70001-9350-11ed-a1eb-0242ac120002).
#. Read its **Characteristic User Description** to see that it is ``Gesture``.
#. Read the **Nordic Status Message Service** message characteristic to check the initial status.
   It presents the status string that describes the previously detected gesture.
#. Enable notification for the characteristic.
#. Move the device up and down.
   The ``updown`` gesture is recognized by the machine learning model and **Nordic Status Message Service** is updated.
   **LED1** turns on for some time.
#. Rotate the device.
   The ``rotate`` gesture is recognized by the machine learning model and **Nordic Status Message Service** is updated.
   **LED2** turns on for some time.
#. Tap the device.
   The ``tap`` gesture is recognized by the machine learning model and **Nordic Status Message Service** is updated.
   **LED3** turns on for some time.
#. Disconnect the `nRF Connect for Mobile`_ application from the device.
#. Press and hold **Button 1** for more than 5 seconds to switch to the data forwarding mode.
   After the mode is switched, **LED1**, **LED2**, and **LED3** start blinking very slowly.
#. You can test the data forwarding mode of the application with the `nRF Connect for Mobile`_.

   a. Open the `nRF Connect for Mobile`_ application and connect to the device again.
   #. Bond with the device from the `nRF Connect for Mobile`_ application on your smartphone or tablet.
   #. Find **Nordic UART Service** and enable notification of its "TX Charcteristic".
   #. Observe the sensor readouts represented as comma-separated values.
      Every new value represents a single sensor readout.

#. You can also test the data forwarding mode with use of additional DK.

   a. Program the :ref:`central_uart` sample to a compatible development kit, for example the nRF52840 Development Kit.
   #. Turn on the programmed device.
      After a brief delay, the Bluetooth® connection between the sample and the PDK is established.
      The nRF54H20 PDK forwards the sensor readouts over NUS to the Central UART sample.
      The **LED1**, **LED2**, and **LED3** on the nRF54H20 PDK start to blink rapidly.
   #. Connect to the Bluetooth® Central UART sample with a terminal emulator (for example, PuTTY).
      See :ref:`putty` for the required settings.
   #. Observe the sensor readouts represented as comma-separated values.
      Every line represents a single sensor readout.
      The Central UART sample forwards the data to the host over UART.
   #. Turn off PuTTY to ensure that only one program has access to data on UART.

Forwarding data to Edge Impulse studio
--------------------------------------

To start forwarding data to Edge Impulse studio:

1. Make sure you meet the `Data forwarding requirements`_ before forwarding data to Edge Impulse studio.
#. Run the ``edge-impulse-data-forwarder`` Edge Impulse command line tool.
#. Log in to `Edge Impulse studio`_ and perform the following steps:

   a. Select the :guilabel:`Data acquisition` tab.
   #. In the **Record new data** panel, set the desired values and click :guilabel:`Start sampling`.

      .. figure:: ../../doc/nrf/images/ei_data_acquisition.png
         :scale: 50%
         :alt: Sampling under Data acquisition in Edge Impulse studio

         Sampling under Data acquisition in Edge Impulse studio

   #. Observe the received sample data on the raw data graph under the panel.
      The observed signal depends on the acceleration readouts.

      .. figure:: ../../doc/nrf/images/ei_start_sampling.png
         :scale: 50%
         :alt: Sampling example

         Sampling example

Porting guide
*************

You can port the nRF Machine Learning application to any board available in the |NCS| or Zephyr.
To do so, create the board-specific directory in :file:`applications/machine_learning/configuration/` and add the application configuration files there.
See the :ref:`nrf_machine_learning_app_configuration` for detailed information about the nRF Machine Learning application configuration.

.. _nrf_machine_learning_app_internal_modules:

Application internal modules
****************************

The nRF Machine Learning application uses modules available in :ref:`lib_caf` (CAF), a set of generic modules based on Application Event Manager and available to all applications and a set of dedicated internal modules.
See `Firmware architecture`_ for more information.

The nRF Machine Learning application uses the following modules available in CAF:

* :ref:`caf_ble_adv`
* :ref:`caf_ble_bond`
* :ref:`caf_ble_smp`
* :ref:`caf_ble_state`
* :ref:`caf_buttons`
* :ref:`caf_click_detector`
* :ref:`caf_leds`
* :ref:`caf_power_manager`
* :ref:`caf_sensor_manager`

See the module pages for more information about the modules and their configuration.

The nRF Machine Learning application also uses the following dedicated application modules:

``ei_data_forwarder_bt_nus``
  The module forwards the sensor readouts over NUS to the connected Bluetooth Central.
  The sensor data is forwarded only if the connection is secured and connection interval is within the limit defined by :kconfig:option:`CONFIG_BT_PERIPHERAL_PREF_MAX_INT` and :kconfig:option:`CONFIG_BT_PERIPHERAL_PREF_MAX_INT`.

``ei_data_forwarder_uart``
  The module forwards the sensor readouts over UART.

``led_state``
  The module displays the application state using LEDs.
  The LED effects used to display the state of data forwarding, the machine learning results, and the state of the simulated signal are defined in :file:`led_state_def.h` file located in the application configuration directory.
  The common LED effects are used to represent the machine learning results and the simulated sensor signal.

``ml_runner``
  The module uses :ref:`ei_wrapper` API to control running the machine learning model.
  It provides the prediction results using :c:struct:`ml_result_event`.

``ml_app_mode``
  The module controls Application mode. It switches between running the machine learning model and forwarding the data.
  The change is triggered by a long press of the button defined in the module's configuration.

``sensor_sim_ctrl``
  The module controls parameters of the generated simulated sensor signal.
  It switches between predefined sets of parameters for the simulated signal.
  The parameters of the generated signals are defined by the :file:`sensor_sim_ctrl_def.h` file located in the application configuration directory.

``usb_state``
  The module enables USB.

.. note::
   The ``ei_data_forwarder_bt_nus`` and ``ei_data_forwarder_uart`` modules stop forwarding the sensor readouts if they receive a :c:struct:`sensor_event` that cannot be forwarded and needs to be dropped.
   This could happen, for example, if the selected sensor sampling frequency is too high for the used implementation of the Edge Impulse data forwarder.
   Data forwarding is stopped to make sure that dropping samples is noticed by the user.
   If you switch to running the machine learning model and then switch back to data forwarding, the data will be again forwarded to the host.

Dependencies
************

The application uses the following Zephyr drivers and libraries:

* :ref:`zephyr:sensor_api`
* :ref:`zephyr:uart_api`
* :ref:`zephyr:mcu_mgr`

The application uses the following |NCS| libraries and drivers:

* :ref:`app_event_manager`
* :ref:`lib_caf`
* :ref:`ei_wrapper`
* :ref:`nus_service_readme`

The sample also uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`

In addition, you can use the :ref:`central_uart` sample together with the application.
The sample is used to receive data over NUS and forward it to the host over UART.

API documentation
*****************

Following are the API elements used by the application.

Edge Impulse Data Forwarder Event
=================================

| Header file: :file:`applications/machine_learning/src/events/ei_data_forwarder_event.h`
| Source file: :file:`applications/machine_learning/src/events/ei_data_forwarder_event.c`

.. doxygengroup:: ei_data_forwarder_event
   :project: nrf
   :members:

Machine Learning Application Mode Event
=======================================

| Header file: :file:`applications/machine_learning/src/events/ml_app_mode_event.h`
| Source file: :file:`applications/machine_learning/src/events/ml_app_mode_event.c`

.. doxygengroup:: ml_app_mode_event
   :project: nrf
   :members:

Machine Learning Result Event
=============================

| Header file: :file:`applications/machine_learning/src/events/ml_result_event.h`
| Source file: :file:`applications/machine_learning/src/events/ml_result_event.c`

.. doxygengroup:: ml_result_event
   :project: nrf
   :members:

Sensor Simulator Event
======================

| Header file: :file:`applications/machine_learning/src/events/sensor_sim_event.h`
| Source file: :file:`applications/machine_learning/src/events/sensor_sim_event.c`

.. doxygengroup:: sensor_sim_event
   :project: nrf
   :members:
