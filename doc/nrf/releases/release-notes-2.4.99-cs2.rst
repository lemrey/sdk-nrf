.. _ncs_release_notes_cs2:

Release notes for nRF54 customer sampling v2.4.99-cs2
#####################################################

.. contents::
   :local:
   :depth: 2

|NCS| delivers reference software and supporting libraries for developing low-power wireless applications with Nordic Semiconductor products in the nRF52, nRF53, nRF54, nRF70, and nRF91 Series.

This release of |NCS| is only meant to be used for product development on the nRF54H and nRF54L devices.
To work with other devices, use the |NCS| 2.4.0 release from the `sdk-nrf`_ repository.
This release notes document only lists additional changes related to the nRF54 customer sampling v2.4.99-cs2.

The SDK includes open source projects (TF-M, MCUboot, OpenThread, Matter, and the Zephyr RTOS), which are continuously integrated and redistributed with the SDK.

Release notes might refer to "experimental" support for features, which indicates that the feature is incomplete in functionality or verification, and can be expected to change in future releases.
To learn more, see :ref:`software_maturity`.

All features delivered as part of nRF54 limited sampling are experimental.

Highlights
**********

* Added:

  * Experimental support for Bluetooth® Low Energy for the nRF54L15 device.
  * Experimental support for a set of drivers on the nRF54L15 device.
    See the `Drivers`_ section for a full list.
  * Experimental support for Direct Test Mode for the nRF54L15 device.
  * Experimental support for Software Update for the Internet of Things (SUIT) for the nRF54H20 device.
  * Experimental support for Thread and Matter over Thread for the nRF54H20 device.
  * Experimental support for Cracen PRNG on the RF54L15 and for PSA Crypto API.
  * Experimental support for the nRF54L15 in the Find My stack and samples.
    Find My support is delivered through the `sdk-find-my-next repository`_.
  * Experimental support for NFC Type 2 Tag, Type 4 Tag, and NFC shell on the nRF54L15 and nRF54H20.
  * Experimental support for the nRF54H20 with USB High-Speed in the :ref:`nrf_desktop` application.
  * Support for the nRF54L15 and nRF54H20 devices in |nRFVSC|.


Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v2.4.99-cs2**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` and :ref:`gs_updating_repos_examples` for more information.

For information on the included repositories and revisions, see `Repositories and revisions for nRF54 customer sampling v2.4.99-cs2`_.

IDE and tool support
********************

Refer to the Getting Started guides for installation and building instructions:

* :ref:`ug_nrf54h20_gs`
* :ref:`ug_nrf54l15_gs`

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF54 customer sampling v2.4.99-cs2`_ for the list of issues valid for the latest release.

.. _ncs_release_notes_cs2_changelog:

Changelog
*********

The following sections provide detailed lists of changes by component.

Application Development
=======================

* Added support for development on the nRF54L15 and nRF54H20 devices in |nRFVSC|.

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

nRF Desktop
-----------

* Added:

  * Support for the nRF54H20 PDK with the ``nrf54h20dk_nrf54h20_cpuapp@soc1`` board target.
    The nRF Desktop application for this target is configured to act as a gaming mouse.
    It supports both USB High-Speed and the Bluetooth LE transport.
    See the :ref:`nrf_desktop_board_configuration_files` section of the nRF Desktop documentation for details.
  * The :ref:`nrf_desktop_measuring_hid_report_rate` section in the nRF Desktop documentation.
    The USB High-Speed transport on the nRF54H20 PDK is able to achieve 8000 HID reports per second with the shortest USB polling interval.
  * Kconfig options to enable handling of the power management events for the following nRF Desktop modules:

    * :ref:`nrf_desktop_board` - The :ref:`CONFIG_DESKTOP_BOARD_PM_EVENTS <config_desktop_app_options>` Kconfig option.
    * :ref:`nrf_desktop_motion` - The :ref:`CONFIG_DESKTOP_MOTION_PM_EVENTS <config_desktop_app_options>` Kconfig option.
    * :ref:`nrf_desktop_ble_latency` - The :ref:`CONFIG_DESKTOP_BLE_LATENCY_PM_EVENTS <config_desktop_app_options>` Kconfig option.

    All listed Kconfig options are enabled by default and depend on the :kconfig:option:`CONFIG_CAF_PM_EVENTS` Kconfig option.
  * Kconfig option to configure a motion generated per second during a button press (:ref:`CONFIG_DESKTOP_MOTION_BUTTONS_MOTION_PER_SEC <config_desktop_app_options>`) in the :ref:`nrf_desktop_motion`.
    To improve accuracy of the motion data generated when pressing a button, the implementation relies on the hardware clock instead of the system uptime.

* Updated:

  * To improve the accuracy, the generation of simulated movement data in the :ref:`nrf_desktop_motion` now uses a timestamp in microseconds based on the cycle count (either :c:func:`k_cycle_get_32` or :c:func:`k_cycle_get_64` function depending on the :kconfig:option:`CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER` Kconfig option).
  * Aligned Kconfig option names in the :ref:`nrf_desktop_motion` implementation that generates motion from button presses.
    The Kconfig options defining used key IDs are prefixed with ``CONFIG_MOTION_BUTTONS_`` instead of ``CONFIG_MOTION_`` to ensure consistency with configuration of other implementations of the motion module.

nRF Machine Learning (Edge Impulse)
-----------------------------------

* Updated the machine learning models (:kconfig:option:`CONFIG_EDGE_IMPULSE_URI`) used by the application so that they are now hosted by Nordic Semiconductor.

nRF Thread Border Router
------------------------

* Added new :ref:`nrf_tbr_app` application with support for the nRF54H20 PDK with board target ``nrf54h20dk_nrf54h20_cpuapp@soc1``.

Samples
=======

This section provides detailed lists of changes by :ref:`sample <sample>`, including protocol-related samples.

Bluetooth samples
-----------------

* Added support for the nRF54L15 PDK with board target ``nrf54l15dk_nrf54l15_cpuapp@soc1`` in the following samples:

  * :ref:`bluetooth_central_hids`
  * :ref:`central_uart`
  * :ref:`peripheral_uart`
  * :ref:`peripheral_lbs`
  * :ref:`direct_test_mode`

Cryptography samples
--------------------

* Added:

  * Support for software-enabled crypto samples (using PSA crypto drivers from Oberon) for the nRF54L15 and nRF54H20 devices.
  * Support for hardware-accelerated TRNG+PRNG cryptography using CRACEN for the nRF54L15.

  * Support for the nRF54L15 PDK with board target ``nrf54l15dk_nrf54l15_cpuapp@soc1`` in the following samples:

    * :ref:`crypto_aes_cbc`
    * :ref:`crypto_aes_ccm`
    * :ref:`crypto_aes_ctr`
    * :ref:`crypto_aes_gcm`
    * :ref:`crypto_chacha_poly`
    * :ref:`crypto_ecdh`
    * :ref:`crypto_ecdsa`
    * :ref:`crypto_ecjpake`
    * :ref:`crypto_hkdf`
    * :ref:`crypto_hmac`
    * :ref:`crypto_persistent_key`
    * :ref:`crypto_rng`
    * :ref:`crypto_sha256`

  * Support for the nRF54H20 PDK with board target ``nrf54h20dk_nrf54h20_cpuapp@soc1`` in the following samples:

    * :ref:`crypto_aes_cbc`
    * :ref:`crypto_aes_ccm`
    * :ref:`crypto_aes_ctr`
    * :ref:`crypto_aes_gcm`
    * :ref:`crypto_chacha_poly`
    * :ref:`crypto_ecdh`
    * :ref:`crypto_ecdsa`
    * :ref:`crypto_hkdf`
    * :ref:`crypto_hmac`
    * :ref:`crypto_rng`
    * :ref:`crypto_rsa`
    * :ref:`crypto_sha256`

DFU samples
-----------

* Added new :ref:`SUIT DFU: Update over Bluetooth Low Energy <nrf54h_suit_sample>` sample that demonstrates the Software Update for the Internet of Things (SUIT) procedure on the nRF54H20 with board target ``nrf54h20dk_nrf54h20_cpuapp@soc1``.

Matter samples
--------------

* Added support for the nRF54H20 PDK with board target ``nrf54h20dk_nrf54h20_cpuapp@soc1`` in the following samples:

  * :ref:`matter_lock_sample`
  * :ref:`matter_template_sample`

NFC samples
-----------

* Added support for the nRF54L15 PDK with board target ``nrf54l15dk_nrf54l15_cpuapp@soc1`` in the following samples:

  * :ref:`record_text`
  * :ref:`writable_ndef_msg`
  * :ref:`nrf-nfc-system-off-sample`
  * :ref:`nfc_shell`
  * :ref:`record_launch_app`
  * :ref:`nfc_tnep_tag`

* Added support for the nRF54H20 PDK with board target ``nrf54h20dk_nrf54h20_cpuapp@soc1`` in the following samples:

  * :ref:`record_text`
  * :ref:`writable_ndef_msg`
  * :ref:`nfc_shell`
  * :ref:`record_launch_app`

Thread samples
--------------

* :ref:`ot_cli_sample` sample

  * Added support for the nRF54H20 PDK with board target ``nrf54h20dk_nrf54h20_cpuapp@soc1``.

Other samples
-------------

* :ref:`radio_test` sample:

  * Added support for the nRF54L15 PDK with board target ``nrf54l15dk_nrf54l15_cpuapp@soc1``.

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

* Added:

  * Support for the following peripheral drivers on the nRF54H20 PDK:

    * TWIM
    * QDEC
    * COMP

  * Support for the following peripheral drivers on the nRF54L15 PDK:

    * QDEC
    * TIMER
    * SAADC
    * COMP
    * WDT
    * I2S
    * GRTC
    * TWIM
    * SPIM
    * UARTE
    * GPIO
    * GPIOTE
    * CLOCK
    * RRAMC

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each NCS release.

The Zephyr fork in |NCS| (``sdk-zephyr-next``) contains all commits from the upstream Zephyr repository up to and including ``4bbd91a9083a588002d4397577863e0c54ba7038``, plus some |NCS| specific additions.

For the list of |NCS| specific commits, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline manifest-rev ^4bbd91a908

The current |NCS| main branch is based on revision ``4bbd91a908`` of Zephyr.

* Added support for the nRF54L15 PDK with board target ``nrf54l15dk_nrf54l15_cpuapp@soc1`` in the following samples:

  * :ref:`zephyr:blinky-sample`
  * :ref:`zephyr:hello_world`
  * :ref:`zephyr:settings_subsys_sample`

* :ref:`zephyr:qdec_sensor` sample:

  * Added support for the nRF54H20 PDK with board target ``nrf54h20dk_nrf54h20_cpuapp@soc1``.

Documentation
=============

* Added the following documentation sections:

  * :ref:`ug_nrf54l`
  * :ref:`ug_nrf54h20_suit_dfu`
  * :ref:`ug_nrf54h20_matter_thread`
  * :ref:`ug_nrf54h20_custom_pcb`
