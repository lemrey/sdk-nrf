.. _migration_cs2_to_cs3:

Migration notes for |NCS| for the nRF54 customer sampling release v2.4.99-cs3
#############################################################################

This document describes the changes required or recommended when migrating your applications from |NCS| v2.4.99-cs2 to |NCS| v2.4.99-cs3.

nRF54L15 revision and compatibility
***********************************

To ensure the nRF54L15 PDK runs its components correctly, verify its compatibility with your release.
Check the version number on your PDK's sticker and refer to the following compatibility matrix:

+----------------------+------------------------------+
| Release version      | PDK version                  |
+======================+==============================+
| ``2.4.99-cs2``       | * PCA10156, v0.2.0 AA0-ES2   |
|                      | * PCA10156, v0.2.0 AA0-ES3   |
+----------------------+------------------------------+
| ``2.4.99-cs3``       | * PCA10056, v0.2.1 AB0-ES5   |
+----------------------+------------------------------+

Updated tools
*************

The following tools have been updated for the release of the |NCS| v2.4.99-cs3:

* nRF Command Line Tools to version 10.22.3
* The |NCS| toolchain

To install the new tools, refer to the getting started guides for your PDK:

* :ref:`ug_nrf54h20_gs`
* :ref:`ug_nrf54l15_gs`

Required changes
****************

The following changes are mandatory to make your application work correctly after the migration.

* nrfx API:
  If your application directly uses the nrfx API, update your code to take into account the following changes:

  * Changed all the functions' parameters in the GPIOTE driver.
    A pointer to the nrfx_gpiote_t structure was added as the first parameter in all the functions.
    Any function without the pointer to the ``nrfx_gpiote_t`` structure as the first argument is deprecated.
    Add a pointer to the ``nrfx_gpiote_t`` structure to any ``nrfx_gpiote_*`` function call.
  * Changed the prototype of an event handler in the WDT driver.
    The new prototype accepts an event type, requests, and a pointer to the context.
    The previous version is now deprecated.
    Update any event handler definition to align with the new function prototype.

Recommended changes
*******************

The following changes are recommended for your application to work optimally after the migration.

* Device Firmware Update (DFU):
  As the SUIT DFU sample has undergone significant changes, including the implementation of a hierarchical manifest, consult the documentation for the :ref:`nrf54h_suit_sample` and related :ref:`ug_nrf54h20_suit_dfu` documentation.

* nrfx API:
  If your application directly uses the nrfx API, familiarize yourself with the following changes:

  * Added a new member to the configuration structure of the QSPI driver.
    The new configuration member determines the time in milliseconds when a timeout of the QSPI operation occurs.
    If you use ``NRFX_QSPI_DEFAULT_CONFIG`` macro, the timeout field is set with the default value of 500 milliseconds.
  * Replaced the deprecated ``NRFX_ERROR_ALREADY_INITIALIZED`` error code with the new ``NRFX_ERROR_ALREADY`` one.
  * Changed the return value in all drivers in case initialization functions have already been executed.
    The ``NRFX_ERROR_ALREADY`` error code is now returned in place of the ``NRFX_ERROR_INVALID_STATE`` one.
