.. _lwm2m_carrier_provisioning:

Preparing for production
########################

.. contents::
   :local:
   :depth: 2

Before deploying devices that use the :ref:`liblwm2m_carrier_readme` library, consider the following prerequisites.

.. _lwm2m_root_ca_certs:

Root CA certificates
********************

You can optimize your application by removing or adding certificates based on your product needs.
Refer to the respective documentation of one or more of your operators for more information on which certificates apply.

The :ref:`lwm2m_carrier` sample contains a couple of root CA certificates that are typically used for out-of-band firmware updates.
The carrier with which you plan to certify your product might not require the included certificates.
Also, the certificates needed by the carrier might not be present in the sample.
Hence it might become necessary to program the required certificates.

Pre-programming the certificates
================================

The certificates are typically stored in the modem and read by the application.
The LwM2M carrier sample writes the certificates to the modem if they are not already present.
Alternatively, you can also write the certificates beforehand using the :ref:`at_client_sample` sample.

To pre-program the certificates, use one of the following methods:

* Build and flash the :ref:`at_client_sample` sample and provision the certificates using AT commands and the `LTE Link Monitor`_ application. See `Managing credentials`_ for more information.
* Build and flash the LwM2M carrier sample, while making sure that the appropriate certificates (:file:`*.pem`) are included in the :file:`certs` folder and referenced in :file:`carrier_certs.c`.

Other samples and applications like :ref:`asset_tracker_v2` and :ref:`modem_shell_application` with the carrier library integration do not write any certificates in the application.

Pre-shared Key (PSK)
********************

The PSK must only be provided using a security tag using the :kconfig:option:`CONFIG_LWM2M_CARRIER_SERVER_SEC_TAG` Kconfig option.

The sample allows you to set a PSK that is written to a modem security tag using the  :kconfig:option:`CONFIG_CARRIER_APP_PSK` and :kconfig:option:`CONFIG_LWM2M_CARRIER_SERVER_SEC_TAG` Kconfig options.
This is convenient for developing and debugging but must be avoided in the final product.

To set a PSK, complete the following steps:

1. Build and flash the sample once with provided values for :kconfig:option:`CONFIG_CARRIER_APP_PSK` and :kconfig:option:`CONFIG_LWM2M_CARRIER_SERVER_SEC_TAG`.
#. Build the sample again, by providing value for only :kconfig:option:`CONFIG_LWM2M_CARRIER_SERVER_SEC_TAG` and not for :kconfig:option:`CONFIG_CARRIER_APP_PSK`.

Alternatively, you can also set the PSK beforehand using the :ref:`at_client_sample` sample before flashing your application.

Carrier configuration and testing
*********************************

* The library supports carriers which have specific device management requirements.
  Not all requirements are documented here.
  You must contact your carrier for more information as mentioned in the :ref:`lwm2m_certification` documentation.

  * With a few exceptions, if you leave the :c:func:`config_init` function empty, it configures the carrier library to behave correctly in the operator network it is currently connected to.

* The settings required to test and certify your product with the carrier will be different from the settings needed for mass deployment.

  * When :kconfig:option:`CONFIG_LWM2M_CARRIER_CUSTOM_URI` is not set (when :c:func:`config_init` is empty), the URI is predetermined to connect to the live (production) device management server of the currently connected operator network. 
  * During certification process, the :kconfig:option:`CONFIG_LWM2M_CARRIER_CUSTOM_URI` and :kconfig:option:`CONFIG_LWM2M_CARRIER_CUSTOM_PSK` Kconfig options must be set accordingly to connect to the carrier's certification servers instead of the live (production) servers.
    See :ref:`lwm2m_app_int` for more information on the required configurations.
