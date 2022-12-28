.. _ipc_service_sample:

IPC service
###########

.. contents::
   :local:
   :depth: 2

The IPC service sample demonstrates the functionality of the IPC service.

Overview
********

The sample application tests throughput of the IPC service with available backends.
Currently, the sample supports OpenAMP RPMSG and ICMSG backends.

Each core periodically prints out data throughput in bytes per second.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Configuration
*************

|config|

Both application and network core send data to each other in the time interval specified by the :ref:`CONFIG_APP_IPC_SERVICE_SEND_INTERVAL <CONFIG_APP_IPC_SERVICE_SEND_INTERVAL>` option.
You can change the value and observe how the throughput on each core changes.

.. note::
   Increasing the time interval to send data on one core, decreases the reading speed on the other core.

Configuration options
=====================

Check and configure the following Kconfig options:

.. _CONFIG_APP_IPC_SERVICE_SEND_INTERVAL:

CONFIG_APP_IPC_SERVICE_SEND_INTERVAL - Time interval to send data through the IPC service
   The sample configuration defines the time interval to send data packages through IPC service in µs.
   Since the kernel timeout has a 1 ms resolution, this value is rounded off.
   If the value is lesser than 1000 µs, use :c:func:`k_busy_wait` instead of :c:func:`k_msleep` function.

.. _ipc_service_configuration_file:

Configuration files
===================

The sample provides predefined configuration files for typical use cases.
The configuration and devicetree overlay files are placed in the :file:`samples/ipc/ipc_service/boards` and :file:`samples/ipc/ipc_service` folders.

.. important::
   For the nRF54H20 Development Kit (DK), there is no default configuration.
   You must provide configuration and devicetree overlays.

Following are the available configuration files for the nRF54H20 DK:

* :file:`nrf54h20dk_nrf54h20_cpuapp_ppr.conf` is used to test transmission between ``cpuapp`` and ``cpuppr`` build targets with an ICMSG backend.
* :file:`nrf54h20dk_nrf54h20_cpuapp_rad.conf` is used to test transmission between ``cpuapp`` and ``cpurad`` build targets with ICMSG and RPMSG backends.
* :file:`nrf54h20dk_nrf54h20_cpuapp_sys.conf` is used to test transmission between ``cpuapp`` and ``cpusys`` build targets with ICMSG and RPMSG backends.
* :file:`nrf54h20dk_nrf54h20_cpurad_sys.conf` is used to test transmission between ``cpurad`` and ``cpusys`` build targets with ICMSG and RPMSG backends.

For the nRF54H20 DK, select any one of the configuration files.
When using the command line, add any of these file names with the ``-DOVERLAY_CONFIG=boards/`` and ``-Dremote_OVERLAY_CONFIG=boards/`` prefixes to the build command for local and remote applications, respectively.

Following are the available devicetree overlay files for the nRF54H20 DK:

* :file:`nrf54h20dk_nrf54h20_cpuapp_ppr.overlay` is used for ``cpuapp`` and ``cpuppr`` configurations with ICMSG backends.
* :file:`nrf54h20dk_nrf54h20_cpuapp_rad_icmsg.overlay` is used for ``cpuapp`` and ``cpurad`` configurations with ICMSG backends.
* :file:`nrf54h20dk_nrf54h20_cpuapp_rad.overlay` is used for ``cpuapp`` and ``cpurad`` configurations, RPMSG.
* :file:`nrf54h20dk_nrf54h20_cpuapp_sys_icmsg.overlay` is used for ``cpuapp`` and ``cpusys`` configurations with ICMSG backends.
* :file:`nrf54h20dk_nrf54h20_cpuapp_sys.overlay` is used for ``cpuapp`` and ``cpusys`` configurations with RPMSG backends.
* :file:`nrf54h20dk_nrf54h20_cpurad_sys_icmsg.overlay` is used for ``cpurad`` and ``cpusys`` configurations with ICMSG backends.
* :file:`nrf54h20dk_nrf54h20_cpurad_sys.overlay` is used for ``cpurad`` and ``cpusys`` configurations with RPMSG backends.

Select the file with the name corresponding to the configuration overlay and IPC backend you have selected.
When using the command line, along with parameters of configuration files, add the file name with the ``-DDTC_OVERLAY_FILE=boards/`` and ``-Dremote_OVERLAY_CONFIG=boards/-Dremote_DTC_OVERLAY_FILE=boards/`` prefixes to the build command for local and remote applications, respectively.

For ICMSG targets, the :file:`prj_icmsg.conf` file must be selected as a configuration.
When using command line, along with the rest parameters, add the ``-DCONF_FILE=prj_icmsg.conf`` and ``-Dremote_CONF_FILE=prj_icmsg.conf`` parameters to the build command for local and remote applications, respectively.

An example on how to program the sample for the nRF54H20 DK with a configuration overlay, devicetree overlay, and selected IPC backend is shown in the :ref:`Building and running <nRF54H20_sample_build>` section.

Building and running
********************

.. |sample path| replace:: :file:`samples/ipc/ipc_service`

.. include:: /includes/build_and_run.txt

You can use a set of configurations to get different test scenarios.

To verify the throughput for only one direction of data transmission, configure the asymmetric sending intervals between cores in the following way:

* ``-DCONFIG_APP_IPC_SERVICE_SEND_INTERVAL=20000000`` and ``-Dremote_CONFIG_APP_IPC_SERVICE_SEND_INTERVAL=1`` to get the remote sending performance.
* ``-DCONFIG_APP_IPC_SERVICE_SEND_INTERVAL=1`` and ``-Dremote_CONFIG_APP_IPC_SERVICE_SEND_INTERVAL=20000000`` to get the application sending performance.

.. note::
   To program the sample using the nRF54H20 DK, you must provide different configuration and overlay parameters along with the build command.
   There is no default configuration for the nRF54H20 build targets.

You can use different overlays and configurations to build commands for different testing scenarios.
The overlay and configuration file details are provided in the :ref:`ipc_service_configuration_file` section.

For the nRF54H20 DK, the following examples show the testing scenarios for ``cpuapp`` and ``cpurad`` configurations with ICMSG backends.

* To test the application where only the application core is sending data through the IPC service, use the following command:

   .. tabs::

      .. tab:: nRF54H20 Development Kit

         .. code-block:: console

            west build -p -b nrf54h20dk_nrf54h20_cpuapp@soc1 -- -DCONFIG_APP_IPC_SERVICE_SEND_INTERVAL=1 \
            -Dremote_CONFIG_APP_IPC_SERVICE_SEND_INTERVAL=20000000 -DCONF_FILE=prj_icmsg.conf \
            -DOVERLAY_CONFIG=boards/nrf54h20dk_nrf54h20_cpuapp_rad.conf \
            -DDTC_OVERLAY_FILE=boards/nrf54h20dk_nrf54h20_cpuapp_rad_icmsg.overlay \
            -Dremote_CONF_FILE=prj_icmsg.conf -Dremote_OVERLAY_CONFIG=boards/nrf54h20dk_nrf54h20_cpuapp_rad.conf \
            -Dremote_DTC_OVERLAY_FILE=boards/nrf54h20dk_nrf54h20_cpuapp_rad_icmsg.overlay

      .. tab:: nRF5340 Development Kit

         .. code-block:: console

            west build -p -b nrf5340dk_nrf5340_cpuapp -- -DCONFIG_APP_IPC_SERVICE_SEND_INTERVAL=1 \
            -Dremote_CONFIG_APP_IPC_SERVICE_SEND_INTERVAL=20000000

* To test the application for a scenario where only the network core is sending data through the IPC service, use the following command:

   .. tabs::

      .. tab:: nRF54H20 Development Kit

         .. code-block:: console

            west build -p -b nrf54h20dk_nrf54h20_cpuapp@soc1 -- -DCONFIG_APP_IPC_SERVICE_SEND_INTERVAL=20000000 \
            -Dremote_CONFIG_APP_IPC_SERVICE_SEND_INTERVAL=1 -DCONF_FILE=prj_icmsg.conf \
            -DOVERLAY_CONFIG=boards/nrf54h20dk_nrf54h20_cpuapp_rad.conf \
            -DDTC_OVERLAY_FILE=boards/nrf54h20dk_nrf54h20_cpuapp_rad_icmsg.overlay -Dremote_CONF_FILE=prj_icmsg.conf \
            -Dremote_OVERLAY_CONFIG=boards/nrf54h20dk_nrf54h20_cpuapp_rad.conf \
            -Dremote_DTC_OVERLAY_FILE=boards/nrf54h20dk_nrf54h20_cpuapp_rad_icmsg.overlay

      .. tab:: nRF5340 Development Kit

         .. code-block:: console

            west build -p -b nrf5340dk_nrf5340_cpuapp -- -DCONFIG_APP_IPC_SERVICE_SEND_INTERVAL=20000000 \
            -Dremote_CONFIG_APP_IPC_SERVICE_SEND_INTERVAL=1

* To test the application with the ICMSG backend, specify parameters along with the build command in the following way:

.. _nRF54H20_sample_build:

   .. tabs::

      .. tab:: nRF54H20 Development Kit

         .. code-block:: console

            west build -p -b nrf54h20dk_nrf54h20_cpuapp@soc1 -- -DCONF_FILE=prj_icmsg.conf \
            -DOVERLAY_CONFIG=boards/nrf54h20dk_nrf54h20_cpuapp_rad.conf \
            -DDTC_OVERLAY_FILE=boards/nrf54h20dk_nrf54h20_cpuapp_rad_icmsg.overlay -Dremote_CONF_FILE=prj_icmsg.conf \
            -Dremote_OVERLAY_CONFIG=boards/nrf54h20dk_nrf54h20_cpuapp_rad.conf \
            -Dremote_DTC_OVERLAY_FILE=boards/nrf54h20dk_nrf54h20_cpuapp_rad_icmsg.overlay

      .. tab:: nRF5340 Development Kit

         .. code-block:: console

            west build -p -b nrf5340dk_nrf5340_cpuapp -- -DCONF_FILE=prj_icmsg.conf -DDTC_OVERLAY_FILE=icmsg.overlay \
            -Dremote_CONF_FILE=prj_icmsg.conf -Dremote_DTC_OVERLAY_FILE=icmsg.overlay

  The local and remote applications must use the same backend.

* Combine the above options and test maximal core to core throughput with the ICMSG backend.
  To do so, build the application with the following commands:

   .. tabs::

      .. tab:: nRF54H20 Development Kit

         .. code-block:: console

            west build -p -b nrf54h20dk_nrf54h20_cpuapp@soc1 -- -DCONFIG_APP_IPC_SERVICE_SEND_INTERVAL=1 \
            -Dremote_CONFIG_APP_IPC_SERVICE_SEND_INTERVAL=20000000 -DCONF_FILE=prj_icmsg.conf \
            -DOVERLAY_CONFIG=boards/nrf54h20dk_nrf54h20_cpuapp_rad.conf \
            -DDTC_OVERLAY_FILE=boards/nrf54h20dk_nrf54h20_cpuapp_rad_icmsg.overlay \
            -Dremote_CONF_FILE=prj_icmsg.conf -Dremote_OVERLAY_CONFIG=boards/nrf54h20dk_nrf54h20_cpuapp_rad.conf \
            -Dremote_DTC_OVERLAY_FILE=boards/nrf54h20dk_nrf54h20_cpuapp_rad_icmsg.overlay

            west build -p -b nrf54h20dk_nrf54h20_cpuapp@soc1 -- -DCONFIG_APP_IPC_SERVICE_SEND_INTERVAL=20000000 \
            -Dremote_CONFIG_APP_IPC_SERVICE_SEND_INTERVAL=1 -DCONF_FILE=prj_icmsg.conf \
            -Dremote_CONF_FILE=prj_icmsg.conf -DOVERLAY_CONFIG=boards/nrf54h20dk_nrf54h20_cpuapp_rad.conf \
            -DDTC_OVERLAY_FILE=boards/nrf54h20dk_nrf54h20_cpuapp_rad_icmsg.overlay \
            -Dremote_OVERLAY_CONFIG=boards/nrf54h20dk_nrf54h20_cpuapp_rad.conf \
            -Dremote_DTC_OVERLAY_FILE=boards/nrf54h20dk_nrf54h20_cpuapp_rad_icmsg.overlay

      .. tab:: nRF5340 Development Kit

         .. code-block:: console

            west build -p -b nrf5340dk_nrf5340_cpuapp -- -DCONFIG_APP_IPC_SERVICE_SEND_INTERVAL=1 \
            -Dremote_CONFIG_APP_IPC_SERVICE_SEND_INTERVAL=20000000 -DCONF_FILE=prj_icmsg.conf \
            -DDTC_OVERLAY_FILE=icmsg.overlay -Dremote_CONF_FILE=prj_icmsg.conf -Dremote_DTC_OVERLAY_FILE=icmsg.overlay

            west build -p -b nrf5340dk_nrf5340_cpuapp -- -DCONFIG_APP_IPC_SERVICE_SEND_INTERVAL=20000000 \
            -Dremote_CONFIG_APP_IPC_SERVICE_SEND_INTERVAL=1 -DCONF_FILE=prj_icmsg.conf -DDTC_OVERLAY_FILE=icmsg.overlay \
            -Dremote_CONF_FILE=prj_icmsg.conf -Dremote_DTC_OVERLAY_FILE=icmsg.overlay

Testing
=======

In the default configuration, both application and network cores periodically print out the receiving speed of data that was sent by the other core.

After programming the sample to your development kit, test it by performing the following steps:

1. |connect_terminal|
#. Reset the kit.
#. Observe the console output for both cores:

  * For the application core, the output is similar to the following one:

    .. code-block:: console

       *** Booting Zephyr OS build v3.0.99-ncs1  ***
       IPC-service nrf5340dk_nrf5340_cpuapp demo started
       Δpkt: 9391 (100 B/pkt) | throughput: 7512800 bit/s
       Δpkt: 9389 (100 B/pkt) | throughput: 7511200 bit/s
       Δpkt: 9388 (100 B/pkt) | throughput: 7510400 bit/s
       Δpkt: 9390 (100 B/pkt) | throughput: 7512000 bit/s
       Δpkt: 9396 (100 B/pkt) | throughput: 7516800 bit/s

  * For the network core, the output is similar to the following one:

    .. code-block:: console

       *** Booting Zephyr OS build v3.0.99-ncs1  ***
       IPC-service nrf5340dk_nrf5340_cpunet demo started
       Δpkt: 6665 (100 B/pkt) | throughput: 5332000 bit/s
       Δpkt: 6664 (100 B/pkt) | throughput: 5331200 bit/s
       Δpkt: 6658 (100 B/pkt) | throughput: 5326400 bit/s
       Δpkt: 6665 (100 B/pkt) | throughput: 5332000 bit/s
       Δpkt: 6671 (100 B/pkt) | throughput: 5336800 bit/s

Dependencies
************

The sample uses the following Zephyr subsystems:

* ``include/ipc/ipc_service.h``
* :ref:`zephyr:logging_api`
