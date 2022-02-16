.. _gcf_sms_lib_readme:

GCF SMS Library
###############

.. contents::
   :local:
   :depth: 2

The GCF SMS Library is using the :ref:`at_custom_cmd` library and adds custom AT commands for the :c:func:`nrf_modem_at_cmd` command to comply with GCF SMS Certification criterias.
The library adds implementations for the following AT commands:
* AT+CPMS
* AT+CSMS
* AT+CSCA
* AT+CSCS
* AT+CMGD
* AT+CMSS
* AT+CMMS
* AT+CMGW
* AT+CMGF

Usage
=====

The GCF SMS Library is enabled by setting the :kconfig:option:`CONFIG_GCF_SMS_LIB` Kconfig option.
If the library should not be initialized on system init, the :kconfig:option:`CONFIG_GCF_SMS_LIB_SYS_INIT` Kconfig option can be disabled. In this case, the library should be initialized by calling :c:func:`gcf_sms_lib_init`.

API documentation
*****************

| Header file: :file:`include/modem/gcf_sms_lib.h`
| Source file: :file:`lib/gcf_sms_lib/src/gcf_sms_lib.c`

.. doxygengroup:: gcf_sms_lib
   :project: nrf
   :members:
