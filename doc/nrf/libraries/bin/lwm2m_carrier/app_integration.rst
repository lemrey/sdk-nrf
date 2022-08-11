.. _lwm2m_app_int:

Application integration
#######################

.. contents::
   :local:
   :depth: 2

This section describes the needed interaction between the LwM2M carrier library and the user application.

.. figure:: /libraries/bin/lwm2m_carrier/images/lwm2m_carrier_overview.svg
    :alt: Interaction between the LwM2M carrier library and the user application

    Interaction between the LwM2M carrier library and the user application


The LwM2M carrier library has an OS abstraction layer.
This abstraction layer makes the LwM2M carrier library independent of the |NCS| modules and underlying implementation of primitives such as timers, non-volatile storage, and heap allocation.
For more information, see :file:`lwm2m_os.h`.

.. figure:: /libraries/bin/lwm2m_carrier/images/lwm2m_carrier_os_abstraction.svg
    :alt: LwM2M library OS abstraction overview

    LwM2M library OS abstraction overview

It provides an abstraction of the following modules:

* |NCS| modules:

  .. lwm2m_osal_mod_list_start

  * :ref:`at_monitor_readme`
  * :ref:`lte_lc_readme`
  * :ref:`lib_download_client`
  * :ref:`modem_key_mgmt`
  * :ref:`sms_readme`
  * :ref:`pdn_readme`
  * :ref:`lib_dfu_target`

  .. lwm2m_osal_mod_list_end

* Zephyr modules:

  * :ref:`zephyr:kernel_api` (``include/kernel.h``)
  * :ref:`zephyr:nvs_api`

The OS abstraction layer is fully implemented for the |NCS|, and it needs to be ported if used with other RTOS or on other systems.

After calling the :c:func:`lwm2m_carrier_init` function, your application can call the non-returning function :c:func:`lwm2m_carrier_run` in its own thread.
Both these functions are called in :file:`nrf/lib/bin/lwm2m_carrier/os/lwm2m_carrier.c`, which is included into the project when you enable the LwM2M carrier library.

.. _lwm2m_configuration:

Configuration
*************

To run the library in an application, you must implement the application with the API of the library.
Enable the library by setting the :kconfig:option:`CONFIG_LWM2M_CARRIER` Kconfig option to ``y``.

The :ref:`lwm2m_carrier` sample project configuration (:file:`nrf/samples/nrf9160/lwm2m_carrier/prj.conf`) contains all the configurations that are needed by the LwM2M carrier library.

To overwrite the carrier default settings, you can provide the initialization parameter :c:type:`lwm2m_carrier_config_t` with the Kconfig options specified in the following sections.
Alternatively, you can enable the LWM2M carrier shell to overwrite the carrier default settings through the command line.
In the :ref:`lwm2m_carrier` sample, you can enable the LWM2M carrier shell by :ref:`building with the overlay file <lwm2m_carrier_shell_overlay>` :file:`overlay-shell.conf`.
See :ref:`lwm2m_shell` for more information on the shell.

.. _general_options_lwm2m:

General options
===============

Following are some of the general Kconfig options that you can configure:

* :kconfig:option:`CONFIG_LWM2M_CARRIER_BOOTSTRAP_SMARTCARD`:

  * This configuration allows the LwM2M carrier library to use a URI stored on the SIM card.
    The configuration in the SIM will take precedence over any other configuration.
    For example, if a bootstrap server URI is fetched from the SIM, the configuration set by the :kconfig:option:`CONFIG_LWM2M_CARRIER_IS_SERVER_BOOTSTRAP` Kconfig option is ignored.

* :kconfig:option:`CONFIG_LWM2M_CARRIER_SESSION_IDLE_TIMEOUT`:

  * This configuration specifies the session idle timeout (inactivity). Upon timeout, the LwM2M carrier library disconnects from one or more device management servers.
  * The timeout closes the DTLS session. A new DTLS session will be created on the next activity (for example, lifetime trigger).
  * Leaving this configuration empty (0) sets it to a default of 60 seconds.

* :kconfig:option:`CONFIG_LWM2M_CARRIER_CUSTOM_APN`:

  * This configuration produces different results depending on normal or generic mode of operation.
  * If :kconfig:option:`CONFIG_LWM2M_CARRIER_CUSTOM_URI` is not set (normal), this configuration provides a fallback APN.
    This might be required in your application, depending on the requirements from the carrier.
  * If :kconfig:option:`CONFIG_LWM2M_CARRIER_CUSTOM_URI` is set (generic), :kconfig:option:`CONFIG_LWM2M_CARRIER_CUSTOM_APN` is used instead of the default APN and fallback APN is not provided.

* :kconfig:option:`CONFIG_LWM2M_CARRIER_LG_UPLUS`, :kconfig:option:`CONFIG_LWM2M_CARRIER_LG_UPLUS_SERVICE_CODE`:

  * The :kconfig:option:`CONFIG_LWM2M_CARRIER_LG_UPLUS_SERVICE_CODE` Kconfig option sets the LG U+ service code, which is needed to identify your device in the LG U+ device management.

  .. note::
     Application DFU is needed to enable LG U+ functionality.

Server options
==============

Following are some of the server Kconfig options that you can configure:

The server settings can put the LwM2M carrier library either in the normal mode where it connects to the applicable carriers, or in the generic mode where it can connect to any bootstrap server.

* :kconfig:option:`CONFIG_LWM2M_CARRIER_IS_SERVER_BOOTSTRAP`:

  * This configuration specifies if the custom LwM2M server is an LwM2M Bootstrap Server.

* :kconfig:option:`CONFIG_LWM2M_CARRIER_CUSTOM_URI`:

  * This configuration lets the LwM2M carrier library connect to a custom server other than the normal carrier server, thereby enabling the generic mode.
  * You must set this option during self-testing, or if the end product is not to be certified with the applicable carriers.
    For more information, see :ref:`lwm2m_certification`.

* :kconfig:option:`CONFIG_LWM2M_CARRIER_SERVER_SEC_TAG`:

  * If the :kconfig:option:`CONFIG_LWM2M_CARRIER_CUSTOM_URI` option is configured, and the server requires a secure connection, a security tag for a PSK is required.
    If the configuration is empty (0), it is ignored.
  * The :ref:`modem_key_mgmt` section describes how to store credentials for the security tag specified in :kconfig:option:`CONFIG_LWM2M_CARRIER_SERVER_SEC_TAG` into the nRF9160 modem.

* :kconfig:option:`CONFIG_LWM2M_CARRIER_SERVER_LIFETIME`:

  * This configuration specifies the lifetime of the custom LwM2M server.
  * This configuration is ignored if :kconfig:option:`CONFIG_LWM2M_CARRIER_IS_SERVER_BOOTSTRAP` is set.

  .. note::

     A change of the bootstrap server URI between builds does not trigger a new bootstrap.
     The bootstrap process is intended to happen only once unless it is initiated from the server.
     To redo the bootstrap process, you must erase the flash and then load your application again.

Device options
==============

These values are reported in the Device Object.
They are not expected to change during run time.
If these values are not set by the application, the LwM2M carrier library sets the default values based on the modem and operator network.

Following are the device Kconfig options:

* :kconfig:option:`CONFIG_LWM2M_CARRIER_DEVICE_MANUFACTURER`
* :kconfig:option:`CONFIG_LWM2M_CARRIER_DEVICE_MODEL_NUMBER`
* :kconfig:option:`CONFIG_LWM2M_CARRIER_DEVICE_TYPE`
* :kconfig:option:`CONFIG_LWM2M_CARRIER_DEVICE_HARDWARE_VERSION`
* :kconfig:option:`CONFIG_LWM2M_CARRIER_DEVICE_SOFTWARE_VERSION`

.. _lwm2m_events:

LwM2M carrier library events
****************************

The :c:func:`lwm2m_carrier_event_handler` function must be implemented by your application.
This is shown in the :ref:`lwm2m_carrier` sample.
A weak implementation is included in :file:`nrf/lib/bin/lwm2m_carrier/os/lwm2m_carrier.c`.

Following are the various LwM2M carrier library events which are also listed in :file:`nrf/lib/bin/lwm2m_carrier/include/lwm2m_carrier.h`.



* :c:macro:`LWM2M_CARRIER_EVENT_INIT`:

  * This event indicates that the carrier library has been successfully initialized.
    :ref:`nrf_modem` is initialized and can be used. See :ref:`req_appln_limitations`).
    The application can now attach to the network.
  * If CA certificates are not already present in the modem, they can be written when receiving this event (and before attaching to the network).
    The CA certificates needed for your device depends on your network operator.
    The LwM2M carrier library will apply these certificates during certain out-of-band FOTA operations.
    See :ref:`lwm2m_carrier` sample for an example of how these certificates are written to the modem using :ref:`modem_key_mgmt` library.

* :c:macro:`LWM2M_CARRIER_EVENT_LTE_LINK_DOWN`, :c:macro:`LWM2M_CARRIER_EVENT_LTE_LINK_UP`:

  * These events indicate that the device must connect to, or disconnect from the LTE network.
    They occur during the bootstrapping process, startup, and during FOTA.

* :c:macro:`LWM2M_CARRIER_EVENT_BOOTSTRAPPED`:

  * This event indicates that the bootstrap sequence is complete, and that the device is ready to be registered.
  * This event is typically seen during the first boot-up.

* :c:macro:`LWM2M_CARRIER_EVENT_REGISTERED`:

  * This event indicates that the device has registered successfully to the carrier's device management servers.

* :c:macro:`LWM2M_CARRIER_EVENT_DEFERRED`:

  * This event indicates that the connection to the device management server has failed.
  * The :c:macro:`LWM2M_CARRIER_EVENT_DEFERRED` event appears instead of the :c:macro:`LWM2M_CARRIER_EVENT_REGISTERED` event.
  * The :c:member:`timeout` parameter supplied with this event determines when the LwM2M carrier library will retry the connection.
  * Following are the various deferred reasons:

    * :c:macro:`LWM2M_CARRIER_DEFERRED_NO_REASON` - The application need not take any special action. If :c:member:`timeout` is 24 hours, the application can proceed with other activities until the retry takes place.

    * :c:macro:`LWM2M_CARRIER_DEFERRED_PDN_ACTIVATE` - This event indicates problem with the SIM card, or temporary network problems. If this persists, contact your carrier.

    * :c:macro:`LWM2M_CARRIER_DEFERRED_BOOTSTRAP_CONNECT` - The DTLS handshake with the bootstrap server has failed. If the application is using a custom PSK, verify that the format is correct.

    * :c:macro:`LWM2M_CARRIER_DEFERRED_BOOTSTRAP_SEQUENCE` - The bootstrap sequence is incomplete. The server failed either to acknowledge the request by the library, or to send objects to the library. Confirm that the carrier is aware of the IMEI.

    * :c:macro:`LWM2M_CARRIER_DEFERRED_SERVER_NO_ROUTE`, :c:macro:`LWM2M_CARRIER_DEFERRED_BOOTSTRAP_NO_ROUTE` - There is a routing problem in the carrier network. If this event persists, contact the carrier.

    * :c:macro:`LWM2M_CARRIER_DEFERRED_SERVER_CONNECT` - This event indicates that the DTLS handshake with the server has failed. This typically happens if the bootstrap sequence has failed on the carrier side.

    * :c:macro:`LWM2M_CARRIER_DEFERRED_SERVER_REGISTRATION` - The server registration has not completed, and the server does not recognize the connecting device. If this event persists, contact the carrier.

    * :c:macro:`LWM2M_CARRIER_DEFERRED_SERVICE_UNAVAILABLE` - The server is unavailable due to maintenance.

* :c:macro:`LWM2M_CARRIER_EVENT_FOTA_START`:

  * This event indicates that the modem update has started.
  * The application must immediately terminate any open TLS sessions.
  * See :ref:`req_appln_limitations`.

* :c:macro:`LWM2M_CARRIER_EVENT_REBOOT`:

  * This event indicates that the LwM2M carrier library will reboot the device.
  * If the application is not ready to reboot, it must return non-zero and then reboot at the earliest convenient time.

* :c:macro:`LWM2M_CARRIER_EVENT_ERROR`:

  * This event indicates an error.
  * The event data struct :c:type:`lwm2m_carrier_event_error_t` contains the information about the error (:c:member:`code` and :c:member:`value`).
  * Following are the valid error codes:

    * :c:macro:`LWM2M_CARRIER_ERROR_CONNECT_FAIL` - This error is generated from the :c:func:`lte_lc_init_and_connect` function. It indicates possible problems with the SIM card, or insufficient network coverage. See :c:member:`value` field of the event.

    * :c:macro:`LWM2M_CARRIER_ERROR_DISCONNECT_FAIL` - This error is generated from the :c:func:`lte_lc_offline` function. See :c:member:`value` field of the event.

    * :c:macro:`LWM2M_CARRIER_ERROR_BOOTSTRAP` - This error is generated during the bootstrap procedure.

      +--------------------------------------------------------+--------------------------------------------------------------------------------------+--------------------------------------------------+
      | Errors                                                 | More information                                                                     | Recovery                                         |
      |                                                        |                                                                                      |                                                  |
      +========================================================+======================================================================================+==================================================+
      | Retry limit for connecting to the bootstrap            | Common reason for this failure can be incorrect URI or PSK,                          | Library retries after next device reboot.        |
      | server has been reached (``-ETIMEDOUT``).              | or the server is unavailable (for example, temporary network issues).                |                                                  |
      |                                                        | If this error persists, contact your carrier.                                        |                                                  |
      +--------------------------------------------------------+--------------------------------------------------------------------------------------+--------------------------------------------------+
      | Failure to provision the PSK                           | If the LTE link is up while the modem attempts to write keys to the modem,           | Library retries after 24 hours.                  |
      | needed for the bootstrap procedure.                    | the error will be ``-EACCES``. Verify that the application prioritizes the           |                                                  |
      |                                                        | ``LWM2M_CARRIER_EVENT_LTE_LINK_UP`` and ``LWM2M_CARRIER_EVENT_LTE_LINK_DOWN`` events.|                                                  |
      +--------------------------------------------------------+--------------------------------------------------------------------------------------+--------------------------------------------------+
      | Failure to read MSISDN or ICCID values (``-EFAULT``).  | ICCID is fetched from SIM, while MSISDN is received from the network for             | Library retries upon next network connection.    |
      |                                                        | some carriers. If it has not been issued yet, the bootstrap process cannot proceed.  |                                                  |
      +--------------------------------------------------------+--------------------------------------------------------------------------------------+--------------------------------------------------+

    * :c:macro:`LWM2M_CARRIER_ERROR_FOTA_PKG` - This error indicates that the update package has been rejected. The integrity check has failed because of a wrong package sent from the server, or a wrong package received by client. The :c:member:`value` field will have an error of type :c:type:`nrf_dfu_err_t` from the file :file:`nrfxlib/nrf_modem/include/nrf_socket.h`.

    * :c:macro:`LWM2M_CARRIER_ERROR_FOTA_PROTO` - This error indicates a protocol error. There might be unexpected HTTP header contents. The server might not support partial content requests.

    * :c:macro:`LWM2M_CARRIER_ERROR_FOTA_CONN` - This error indicates a connection problem. Either the server host name could not be resolved, or the remote server refused the connection.

    * :c:macro:`LWM2M_CARRIER_ERROR_FOTA_CONN_LOST` - This error indicates a loss of connection, or an unexpected closure of connection by the server.

    * :c:macro:`LWM2M_CARRIER_ERROR_FOTA_FAIL` - This error indicates a failure in applying a valid update. If this error persists, create a ticket in `DevZone`_ with the modem trace.

    * :c:macro:`LWM2M_CARRIER_ERROR_CONFIGURATION` - This error indicates that an illegal object configuration was detected.

    * :c:macro:`LWM2M_CARRIER_ERROR_INTERNAL` - This error indicates an irrecoverable error between the modem and carrier library. The LwM2M carrier library recovers only upon reboot.

.. _lwm2m_shell:

LwM2M carrier shell configuration
*********************************

To enable and configure the LwM2M carrier shell, set the :kconfig:option:`CONFIG_LWM2M_CARRIER_SHELL` Kconfig option to ``y``.
The :kconfig:option:`CONFIG_LWM2M_CARRIER_SHELL` Kconfig option has the following dependencies:

* :kconfig:option:`CONFIG_FLASH_MAP`
* :kconfig:option:`CONFIG_SHELL`
* :kconfig:option:`CONFIG_SETTINGS`

In the :ref:`lwm2m_carrier` sample, the LWM2M carrier shell can be enabled by using the configuration overlay file :file:`overlay-shell.conf`.

.. figure:: /libraries/bin/lwm2m_carrier/images/lwm2m_carrier_os_abstraction_shell.svg
    :alt: LwM2M carrier shell

    LwM2M carrier shell

carrier_init
============

The initialization parameter :c:type:`lwm2m_carrier_config_t` can be overwritten with custom settings through the LwM2M carrier shell command group ``carrier_init``.
Use the ``print`` command to display the configurations that are written with ``carrier_init``:

.. code-block::

    > carrier_init print
    Custom carrier server settings
      Enable custom settings         No
      Bootstrap from smartcard       No
      Session idle timeout           0
      APN
      Service code

    Custom carrier server settings
      Enable custom server settings  Yes
      Is bootstrap server            Yes  (Not used without server URI)
      Server URI
      PSK security tag               0
      Server lifetime                0  (Not used when bootstrap server)

    Custom carrier device settings
      Enable custom device settings  No
      Manufacturer
      Model number
      Device type
      Hardware version
      Software version

These settings are applied by the function :c:func:`lwm2m_carrier_custom_init`.
This function is implemented in :file:`nrf/lib/bin/lwm2m_carrier/os/lwm2m_settings.c` that is included in the project when you enable the LwM2M carrier shell.
The weak implementation of :c:func:`lwm2m_carrier_event_handler` calls :c:func:`lwm2m_carrier_custom_init` on receiving the :c:macro:`LWM2M_CARRIER_EVENT_INIT` event.

carrier_api
===========

The LwM2M carrier shell command group ``carrier_api`` allows you to access the public LwM2M API as shown in :file:`nrf/lib/bin/lwm2m_carrier/include/lwm2m_carrier.h`.

For example, to indicate the battery level of the device to the carrier, the function :c:func:`lwm2m_carrier_battery_level_set` is used.
However, this can also be done through the ``carrier_api`` command:

.. code-block::

   > carrier_api device battery_level 20
   Battery level updated successfully


help
====

To display help for all available shell commands, pass the following command to shell:

.. parsed-literal::
   :class: highlight

   > [*group*] help

If the optional argument is not provided, the command displays help for all command groups.

If the optional argument is provided, it displays help for subcommands of the specified command group.
For example, ``carrier_init help`` displays help for all ``carrier_init`` commands.

Objects
*******

The objects enabled depend on the carrier network.
When connecting to a generic LwM2M server, the following objects are enabled:

* Security
* Server
* Access Control
* Device
* Connectivity Monitoring
* Firmware Update
* Location
* Connectivity Statistics
* Cellular Connectivity
* APN Connection Profile

Resources
=========

The following values that reflect the state of the device must be kept up to date by the application:

* Available Power Sources - Defaults to ``0`` if not set (DC Power).
* Power Source Voltage - Defaults to ``0`` if not set.
* Power Source Current - Defaults to ``0`` if not set.
* Battery Level - Defaults to ``0`` if not set.
* Battery Status - Defaults to ``5`` if not set (Not Installed).
* Memory Total - Defaults to ``0`` if not set.
* Error Code - Defaults to ``0`` if not set (No Error).
* Device Type - Defaults to ``Smart Device`` if not set.
* Software Version - Defaults to ``LwM2M <libversion>``. For example, ``LwM2M 0.30.2`` for release 0.30.2.
* Hardware Version - Defaults to ``1.0`` if not set.
* Location - Defaults to 0 if not set.

The following values are read from the modem by default but can be overwritten:

* Manufacturer
* Model Number
* UTC Offset
* Time zone
* Current Time

For example, the carrier device management platform can observe the battery level of your device.
The application uses the :c:func:`lwm2m_carrier_battery_level_set` function to indicate the current battery level of the device to the carrier.
