:orphan:

.. _migration_2.6:

Migration guide for |NCS| v2.6.0 (Working draft)
################################################

This document describes the changes required or recommended when migrating your application from |NCS| v2.5.0 to |NCS| v2.6.0.

Required changes
****************

The following changes are mandatory to make your application work in the same way as in previous releases.

* For the Serial LTE Modem (SLM) application:

  * The Zephyr settings backend has been changed from :ref:`FCB <fcb_api>` to :ref:`NVS <nvs_api>`.
    Because of this, one setting is restored to its default value after the switch if you are using the :ref:`liblwm2m_carrier_readme` library.
    The setting controls whether the SLM connects automatically to the network on startup.
    You can read and write it using the ``AT#XCARRIER="auto_connect"`` command.
  
  * The handling of Release Assistance Indication (RAI) socket options has been updated in the ``#XSOCKETOPT`` command.
    The individual RAI-related socket options have been consolidated into a single ``SO_RAI`` option.
    You must modify your application to use the new ``SO_RAI`` option with the corresponding value to specify the RAI behavior.
    The changes are as follows:

    The ``SO_RAI_NO_DATA``, ``SO_RAI_LAST``, ``SO_RAI_ONE_RESP``, ``SO_RAI_ONGOING``, and ``SO_RAI_WAIT_MORE`` options have been replaced by the ``SO_RAI`` option with values from ``1`` to ``5``.

    Here are the changes you need to make in your application code:

    * If you previously used ``AT#XSOCKETOPT=1,50,`` replace it with ``AT#XSOCKETOPT=1,61,1`` to indicate ``RAI_NO_DATA``.
    * If you previously used ``AT#XSOCKETOPT=1,51,`` replace it with ``AT#XSOCKETOPT=1,61,2`` to indicate ``RAI_LAST``.
    * If you previously used ``AT#XSOCKETOPT=1,52,`` replace it with ``AT#XSOCKETOPT=1,61,3`` to indicate ``RAI_ONE_RESP``.
    * If you previously used ``AT#XSOCKETOPT=1,53,`` replace it with ``AT#XSOCKETOPT=1,61,4`` to indicate ``RAI_ONGOING``.
    * If you previously used ``AT#XSOCKETOPT=1,54,`` replace it with ``AT#XSOCKETOPT=1,61,5`` to indicate ``RAI_WAIT_MORE``.

* For applications using :ref:`nrf_modem_lib_readme`:

  * The Release Assistance Indication (RAI) socket options have been deprecated and replaced with a new consolidated socket option.
    If your application uses :c:macro:`SO_RAI_*` socket options, you need to update your socket configuration as follows:

    #. Replace the deprecated socket options :c:macro:`SO_RAI_NO_DATA`, :c:macro:`SO_RAI_LAST`, :c:macro:`SO_RAI_ONE_RESP`, :c:macro:`SO_RAI_ONGOING`, and :c:macro:`SO_RAI_WAIT_MORE` with the new :c:macro:`SO_RAI` option.
    #. Set the optval parameter of the :c:macro:`SO_RAI` socket option to one of the new values :c:macro:`RAI_NO_DATA`, :c:macro:`RAI_LAST`, :c:macro:`RAI_ONE_RESP`, :c:macro:`RAI_ONGOING`, or :c:macro:`RAI_WAIT_MORE` to specify the desired indication.
    
    Example of migration:

    .. code-block:: c

      /* Before migration. */
      setsockopt(socket_fd, SOL_SOCKET, SO_RAI_LAST, NULL, 0);

      /* After migration. */
      int rai_option = RAI_LAST;
      setsockopt(socket_fd, SOL_SOCKET, SO_RAI, &rai_option, sizeof(rai_option));

* For the Matter samples and applications using Intermittently Connected Devices configuration (formerly called Sleepy End Devices):

  * The naming convention for the energy-optimized devices has been changed from Sleepy End Devices (SED) to Intermittently Connected Devices (ICD).
    Because of this, the Kconfig options used to manage this configuration have been aligned as well.
    If your application uses the following Kconfig options, they require name changes:

      * The ``CONFIG_CHIP_ENABLE_SLEEPY_END_DEVICE_SUPPORT`` Kconfig option was renamed to :kconfig:option:`CONFIG_CHIP_ENABLE_ICD_SUPPORT`.
      * The ``CONFIG_CHIP_SED_IDLE_INTERVAL`` Kconfig option was renamed to :kconfig:option:`CONFIG_CHIP_ICD_SLOW_POLL_INTERVAL`.
      * The ``CONFIG_CHIP_SED_ACTIVE_INTERVAL`` Kconfig option was renamed to :kconfig:option:`CONFIG_CHIP_ICD_FAST_POLLING_INTERVAL`.
      * The ``CONFIG_CHIP_SED_ACTIVE_THRESHOLD`` Kconfig option was renamed to :kconfig:option:`CONFIG_CHIP_ICD_ACTIVE_MODE_THRESHOLD`.

* For Matter over Thread samples, starting from this release, the cryptography backend enabled by default is PSA Crypto API instead of mbedTLS.
  Be aware of the change and consider the following when migrating to |NCS| v2.6.0:

  * You can keep using mbedTLS API as the cryptography backend by disabling PSA Crypto API.
    You can disable it by setting the :kconfig:option:`CONFIG_CHIP_CRYPTO_PSA` Kconfig option to ``n``.
  * When the Device Attestation Certificate (DAC) private key exists in the factory data set, it can migrate to the PSA ITS secure storage.

    You can also have the DAC private key replaced by zeros in the factory data partition by setting the :kconfig:option:`CONFIG_CHIP_CRYPTO_PSA_MIGRATE_DAC_PRIV_KEY` Kconfig option to ``y``.
    This functionality is experimental.

* For samples using Wi-Fi features:

  * A few Kconfig options related to scan operations have been removed in the current release.

     If your application uses scan operations, they need to be updated to remove the dependency on the following options:

      * ``CONFIG_WIFI_MGMT_SCAN_BANDS``
      * ``CONFIG_WIFI_MGMT_SCAN_DWELL_TIME_ACTIVE``
      * ``CONFIG_WIFI_MGMT_SCAN_DWELL_TIME_PASSIVE``
      * ``CONFIG_WIFI_MGMT_SCAN_SSID_FILT``
      * ``CONFIG_WIFI_MGMT_SCAN_MAX_BSS_CNT``
      * ``CONFIG_WIFI_MGMT_SCAN_CHAN``

  * Instead of the ``CONFIG_WIFI_MGMT_SCAN_MAX_BSS_CNT`` Kconfig option, a new :kconfig:option:`CONFIG_NRF_WIFI_SCAN_MAX_BSS_CNT` Kconfig option is added.

Recommended changes
*******************

The following changes are recommended for your application to work optimally after the migration.

.. HOWTO

   Add changes in the following format:

.. * Change1 and description
.. * Change2 and description
