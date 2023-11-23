.. _ncs_release_notes_cs3:

Release notes for nRF54 customer sampling v2.4.99-cs3
#####################################################

.. contents::
   :local:
   :depth: 2

The |NCS| delivers reference software and supporting libraries for developing low-power wireless applications with Nordic Semiconductor products in the nRF52, nRF53, nRF54, nRF70, and nRF91 Series.

This release of |NCS| is only meant to be used for product development on the nRF54H and nRF54L devices.
To work with other devices, use the |NCS| 2.5.0 release from the `sdk-nrf`_ repository.
This release notes document only lists additional changes related to the nRF54 customer sampling v2.4.99-cs3.

The SDK includes open source projects (TF-M, MCUboot, OpenThread, Matter, and the Zephyr RTOS), which are continuously integrated and redistributed with the SDK.

Release notes might refer to "experimental" support for features, which indicates that the feature is incomplete in functionality or verification, and can be expected to change in future releases.
To learn more, see :ref:`software_maturity`.

All features delivered as part of nRF54 limited sampling are experimental.

Highlights
**********

* Experimental support for the following features for the nRF54H20:

  * :ref:`Enhanced ShockBurst (ESB) <esb_prx_ptx>` on the nRF54H20 PDK.
  * Software Update for Internet of Things (SUIT) in the :ref:`nrf_desktop` application.
  * Hierarchical manifest within :ref:`SUIT device firmware update <ug_nrf54h20_suit_dfu>` (DFU).
  * Low Latency Packet Mode (LLPM) in the :ref:`nrf_desktop` application.
  * :ref:`Matter over Wi-Fi <ug_nrf54h20_matter_thread_matter_wifi>` on the nRF54H20 PDK + nRF7002 EK.
  * Wi-Fi interface based on the nRF7002 for the :ref:`nrf_tbr_app` application.

* Experimental support for the following features for the nRF54L15:

  * :ref:`Matter over Thread <ug_nrf54l15_matter_thread>` on nRF54L15 PDK
  * :ref:`crypto_samples` use HW acceleration
  * :ref:`Enhanced ShockBurst (ESB) <esb_prx_ptx>` on the nRF54L15 PDK.
  * Zephyr :ref:`zephyr:nrf-system-off-sample` supports low power wake from GRTC in system off mode
  * New :ref:`nrf_peripheral_sensor_node_app` application

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v2.4.99-cs3**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` and :ref:`gs_updating_repos_examples` for more information.

For information on the included repositories and revisions, see `Repositories and revisions for nRF54 customer sampling v2.4.99-cs3`_.

IDE and tool support
********************

Refer to the Getting Started guides for installation and building instructions:

* :ref:`ug_nrf54h20_gs`
* :ref:`ug_nrf54l15_gs`

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF54 customer sampling v2.4.99-cs3`_ for the list of issues valid for the latest release.

.. _ncs_release_notes_cs3_changelog:

Migration notes
***************

See the :ref:`migration notes <migration_cs2_to_cs3>` for information on the changes required or recommended when migrating your application to the latest release.

Changelog
*********

The following sections provide detailed lists of changes by component.

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.
See `Samples`_ for lists of changes for the protocol-related samples.

Matter fork
-----------

The Matter fork in the limited sampling version of the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, the ``v1.1.0.1`` tag.
This nRF Connect SDK release uses the same Matter fork distribution as the previous customer sampling release.

ESB
---

* :ref:`ESB module <ug_esb>`:

  * Added:

    * Support for the nRF54H20 and nRF54L15 PDKs.
    * A radio mode with a 4 Mbps rate.

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

nRF Desktop
-----------

* Added support for the Software Update for Internet of Things (SUIT) procedure to the nRF54H20 PDK configurations.

  The ``debug`` and ``release`` configurations use the :ref:`nrf_desktop_config_channel` for SUIT envelope transfer.
  The :ref:`nrf_desktop_config_channel_script` script and :ref:`nrf_desktop_dfu` are used to handle the envelope transfer over the configuration channel protocol.
  The ``smp_suit`` configuration uses the SUITFU (:file:`subsys/mgmt/suitfu`) for the SUIT envelope transfer.
  The :ref:`nrf_desktop_dfu_mcumgr_suit` is used to ensure a proper SUITFU configuration and to lower the Bluetooth LE connection latency, allowing quick data transfer over SMP.

* Enabled Low Latency Packet Mode (LLPM) in the nRF54H20 PDK configurations.

nRF Thread Border Router
------------------------
* Added:

  * Support for the nRF7002 Extension Board as a backbone interface.
  * The following new features of the Thread Border Router:

    * Multicast Forwarding
    * IPv4 support - NAT64 and DNS64
    * ND Proxy
    * Backbone Agent.

nRF Peripheral Sensor node
--------------------------

* Added the :ref:`nrf_peripheral_sensor_node_app` application.
  It periodically acquires data from a Serial Peripheral Interface (SPI) and Two-wire Interface (TWI) devices, advertises, and enables the reading of sensor values from Bluetooth® service in a power-efficient way.

nRF Machine Learning (Edge Impulse)
-----------------------------------

* Added support for the :ref:`zephyr:pm-device-runtime` (PM) framework from Zephyr.

Samples
=======

This section provides detailed lists of changes by :ref:`sample <sample>`, including protocol-related samples.
For lists of protocol-specific changes, see `Protocols`_.

Bluetooth samples
-----------------

* Added support for the nRF54L15 PDK to the :ref:`central_and_peripheral_hrs` sample.

DFU samples
-----------

* Updated the :ref:`nrf54h_suit_sample` sample to have support for UART.
  The documentation is renamed from *SUIT DFU: Update over Bluetooth® Low Energy* to :ref:`nrf54h_suit_sample`.

Thread samples
--------------

* Added support for the nRF54L15 PDK with board target ``nrf54l15dk_nrf54l15_cpuapp@soc1`` in the following samples:

  * :ref:`ot_cli_sample`
  * :ref:`ot_coprocessor_sample`

Matter samples
--------------
* Added:

  * Support for the nRF7002 EK for the nRF54H20 PDK with board target ``nrf54h20dk_nrf54h20_cpuapp@soc1`` in the following samples:

    * :ref:`matter_lock_sample`
    * :ref:`matter_template_sample`

  * Support for the nRF54L15 PDK with board target ``nrf54l15dk_nrf54l15_cpuapp@soc1`` in the following samples:

    * :ref:`matter_light_bulb_sample`
    * :ref:`matter_light_switch_sample`
    * :ref:`matter_template_sample`
    * :ref:`matter_window_covering_sample`

  * Experimental support for DFU and SUIT build for the nRF54H20 PDK with board target ``nrf54h20dk_nrf54h20_cpuapp@soc1`` in the :ref:`matter_lock_sample` sample.

* Aligned MRAM memory layout for the nRF54H20 PDK in :file:`.dts` files to fit single-bank Bluetooth LE SMP firmware update.
* Fixed an issue in the :ref:`matter_lock_sample` sample that would prevent nRF Toolbox for iOS in version 5.0.9 from controlling the sample using the :ref:`nus_service_readme`.

Cryptography samples
--------------------

* Updated all the crypto samples for the nRF54L15 PDK with the ``nrf54l15dk_nrf54l15_cpuapp@soc1`` board target to use CRACEN as the default for cryptographic purposes.

Other samples
-------------

* Added support for the nRF54H20 PDK with the ``nrf54h20dk_nrf54h20_cpuapp@soc1`` board target and the nRF54L15 PDK with the ``nrf54l15dk_nrf54l15_cpuapp@soc1`` board target to the :ref:`esb_prx_ptx` sample.
* Updated the :ref:`radio_test` sample with two radio modes with a 4 Mbps rate.

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Common Application Framework (CAF)
----------------------------------

* Added support for the :ref:`zephyr:pm-device-runtime` (PM) framework from Zephyr.

sdk-nrfxlib
-----------

See the changelog for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for additional information.

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each nRF Connect SDK release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``4bbd91a9083a588002d4397577863e0c54ba7038``, with some |NCS| specific additions.

For the list of |NCS| specific commits, including commits cherry-picked from upstream, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline manifest-rev ^4bbd91a908

The current |NCS| main branch is based on revision ``4bbd91a908`` of Zephyr.

.. note::
   For possible breaking changes and changes between the latest Zephyr release and the current Zephyr version, refer to the :ref:`Zephyr release notes <zephyr_release_notes>`.

Zephyr samples
--------------

* Added support for the nRF54L15 PDK with the ``nrf54l15dk_nrf54l15_cpuapp@soc1`` board target in the following samples:

  * :ref:`zephyr:peripheral_hr`
  * :ref:`zephyr:bluetooth_central_hr`
  * :ref:`zephyr:bluetooth-observer-sample`
  * :ref:`zephyr:adc-sample`
  * :ref:`zephyr:nrf-system-off-sample`

Documentation
=============

* Added the :ref:`migration_cs2_to_cs3`.
* Added the :ref:`ug_gpio_pin` userguide on how to drive a GPIO pin.
* Updated the :ref:`nrf_security_drivers` and :ref:`nrf_security_driver_config` pages with information about the :ref:`nrf_security_drivers_cracen`.
* Updated the :ref:`ug_nrf54h20_suit_dfu` with the following changes:

  * List and description of :ref:`directives <ug_suit_dfu_suit_directives>` used in the SUIT DFU procedure.
  * A page on :ref:`ug_nrf54h20_suit_components` in the :ref:`ug_nrf54h20_suit_dfu` user guide.
  * A page on :ref:`ug_nrf54h20_suit_hierarchical_manifests` in the :ref:`ug_nrf54h20_suit_dfu` user guide.
  * A page on :ref:`ug_nrf54h20_suit_customize_dfu` in the :ref:`ug_nrf54h20_suit_dfu` user guide.
