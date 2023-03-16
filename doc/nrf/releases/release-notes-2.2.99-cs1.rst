.. _ncs_release_notes_cs1:

Release notes for nRF54H20 limited customer sampling
####################################################

.. contents::
   :local:
   :depth: 2

|NCS| delivers reference software and supporting libraries for developing low-power wireless applications with Nordic Semiconductor products in the nRF52, nRF53, nRF54, nRF70, and nRF91 Series.

This release of |NCS| is only meant to be used for product development on the nRF54H20 device.
To work with other devices, use the |NCS| 2.3.0 release from the `sdk-nrf`_ repository.
This release notes document only lists additional changes related to the nRF54H20 limited customer sampling.

The SDK includes open source projects (TF-M, MCUboot, OpenThread, Matter, and the Zephyr RTOS), which are continuously integrated and redistributed with the SDK.

Release notes might refer to "experimental" support for features, which indicates that the feature is incomplete in functionality or verification, and can be expected to change in future releases.
To learn more, see :ref:`software_maturity`.

All features delivered as part of nRF54H20 limited sampling are experimental.

Highlights
**********

* Added:

  * Experimental support for Bluetooth Low Energy for the nRF54H20 device.
  * Experimental support for the following drivers on the nRF54H20 device:

    * GPIO
    * SPIM
    * Timer
    * UART

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v2.2.99-cs1**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` and :ref:`gs_updating_repos_examples` for more information.

For information on the included repositories and revisions, see `Repositories and revisions for limited customer sampling v2.2.99-cs1`_.

IDE and tool support
********************

Refer to :ref:`ug_nrf54h20_gs` for installation and building instructions.

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF54H20 limited customer sampling v2.2.99-cs1`_ for the list of issues valid for the latest release.

.. _ncs_release_notes_cs1_changelog:

Changelog
*********

The following sections provide detailed lists of changes by component.

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

nRF Machine Learning (Edge Impulse)
-----------------------------------

* Added support for the nRF54H20 PDK with board target ``nrf54h20dk_nrf54h20_cpuapp@soc1``.

Samples
=======

This section provides detailed lists of changes by :ref:`sample <sample>`, including protocol-related samples.

Bluetooth samples
-----------------

* :ref:`central_and_peripheral_hrs` sample:

  * Added support for the nRF54H20 PDK with board target ``nrf54h20dk_nrf54h20_cpuapp@soc1``.

* :ref:`bluetooth_central_hids` sample:

  * Added support for the nRF54H20 PDK with board target ``nrf54h20dk_nrf54h20_cpuapp@soc1``.

* :ref:`central_uart` sample:

  * Added support for the nRF54H20 PDK with board target ``nrf54h20dk_nrf54h20_cpuapp@soc1``.

* :ref:`peripheral_hids_keyboard` sample:

  * Added support for the nRF54H20 PDK with board target ``nrf54h20dk_nrf54h20_cpuapp@soc1``.

* :ref:`peripheral_hids_mouse` sample:

  * Added support for the nRF54H20 PDK with board target ``nrf54h20dk_nrf54h20_cpuapp@soc1``.

* :ref:`peripheral_uart` sample:

  * Added support for the nRF54H20 PDK with board target ``nrf54h20dk_nrf54h20_cpuapp@soc1``.

* :ref:`ble_throughput` sample:

  * Added support for the nRF54H20 PDK with board target ``nrf54h20dk_nrf54h20_cpuapp@soc1``.

* :ref:`peripheral_rscs` sample:

  * Added support for the nRF54H20 PDK with board target ``nrf54h20dk_nrf54h20_cpuapp@soc1``.

* :ref:`peripheral_lbs` sample:

  * Added support for the nRF54H20 PDK with board target ``nrf54h20dk_nrf54h20_cpuapp@soc1``.

* :ref:`direct_test_mode` sample:

  * Added support for the nRF54H20 PDK with board target ``nrf54h20dk_nrf54h20_cpurad@soc1``.

Multicore samples
-----------------

* :ref:`multicore_hello_world` sample:

  * Renamed this sample from ``nRF5340: Multicore application`` sample to :ref:`multicore_hello_world`.
  * Added support for the nRF54H20 PDK with board target ``nrf54h20dk_nrf54h20_cpuapp@soc1``.

Other samples
-------------

* :ref:`ipc_service_sample` sample:

  * Added support for the nRF54H20 PDK with board targets ``nrf54h20dk_nrf54h20_cpuapp@soc1`` and ``nrf54h20dk_nrf54h20_cpurad@soc1``.

* :ref:`radio_test` sample:

  * Added support for the nRF54H20 PDK with board target ``nrf54h20dk_nrf54h20_cpurad@soc1``.

* :ref:`caf_sensor_manager_sample` sample:

  * Added support for the nRF54H20 PDK with board target ``nrf54h20dk_nrf54h20_cpuapp@soc1``.

* :ref:`event_manager_proxy_sample` sample:

  * Added support for the nRF54H20 PDK with board target ``nrf54h20dk_nrf54h20_cpuapp@soc1``.

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each NCS release.

The Zephyr fork in |NCS| (``sdk-zephyr-next``) contains all commits from the upstream Zephyr repository up to and including ``e1e06d05fa8d1b6ac1b0dffb1712e94e308861f8``, plus some |NCS| specific additions.

For the list of |NCS| specific commits, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline manifest-rev ^e1e06d05fa

The current |NCS| main branch is based on revision ``e1e06d05fa`` of Zephyr.

* Added:

  * Support for the ``nrf54h20dk_nrf54h20_cpuapp@soc1`` board target in the following samples:

    * :ref:`zephyr:blinky-sample`
    * :ref:`zephyr:peripheral_hr`
    * :ref:`zephyr:bluetooth_central_hr`
    * :ref:`zephyr:coremark_sample`

  * Support for the ``nrf54h20dk_nrf54h20_cpurad@soc1`` board target in the following sample:

    * :ref:`zephyr:bluetooth-hci-rpmsg-sample`

Documentation
=============

* Added documentation for :ref:`ug_nrf54h`.

.. |no_changes_yet_note| replace:: No changes since the latest |NCS| release.
