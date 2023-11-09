.. _ncs_release_notes_changelog_lcs:

Changelog for the nRF54 customer sampling
#########################################

.. contents::
   :local:
   :depth: 2

This page tracks the most relevant changes that are present on the branch of the |NCS| for the limited sampling of the nRF54H20 and nRF54L15 SoCs, as compared to the |NCS| version 2.4.99 (commit hash ``0677b0991e4c3ea58efd14e9e33b6c29a1919113``).
You can find the most relevant changes of the |NCS| version 2.4.99 :ref:`here <ncs_release_notes_changelog>`.

.. note::
   This file is a work in progress and might not cover all relevant changes.

.. HOWTO

   When adding a new PR, decide whether it needs an entry in the changelog.
   If it does, update this page.
   Add the sections you need, as only a handful of sections is kept when the changelog is cleaned.
   "Protocols" section serves as a highlight section for all protocol-related changes, including those made to samples, libraries, and so on.

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF54 customer sampling v2.4.99-cs2`_ for the list of issues valid for the latest release.

Changelog
*********

The following sections provide detailed lists of changes by component.

Application development
=======================

No changes since the latest nRF Connect SDK release.

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.
See `Samples`_ for lists of changes for the protocol-related samples.

Bluetooth LE
------------

No changes since the latest nRF Connect SDK release.

For details, see :ref:`nrfxlib:softdevice_controller_changelog`.

Bluetooth mesh
--------------

No changes since the latest nRF Connect SDK release.

See `Bluetooth mesh samples`_ for the list of changes for the Bluetooth mesh samples.

Matter
------

No changes since the latest nRF Connect SDK release.

See `Matter samples`_ for the list of changes for the Matter samples.

Matter fork
+++++++++++

The Matter fork in the limited sampling version of the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, ``41cb8220744f2587413d0723e24847f07d6ac59f``.

The following list summarizes the most important changes inherited from the upstream Matter:

No changes since the latest nRF Connect SDK release.

Thread
------

No changes since the latest nRF Connect SDK release.

See `Thread samples`_ for the list of changes for the Thread samples.

Zigbee
------

No changes since the latest nRF Connect SDK release.

See `Zigbee samples`_ for the list of changes for the Zigbee samples.

ESB
---

No changes since the latest nRF Connect SDK release.

nRF IEEE 802.15.4 radio driver
------------------------------

No changes since the latest nRF Connect SDK release.

RF Front-End Modules
====================

No changes since the latest nRF Connect SDK release.

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

nRF9160: Asset Tracker v2
-------------------------

No changes since the latest nRF Connect SDK release.

nRF9160: Serial LTE modem
-------------------------

No changes since the latest nRF Connect SDK release.

nRF5340 Audio
-------------

No changes since the latest nRF Connect SDK release.

nRF Machine Learning (Edge Impulse)
-----------------------------------

No changes since the latest nRF Connect SDK release.

nRF Desktop
-----------

No changes since the latest nRF Connect SDK release.

Thingy:53 Zigbee weather station
--------------------------------

No changes since the latest nRF Connect SDK release.

Connectivity Bridge
-------------------

No changes since the latest nRF Connect SDK release.

Samples
=======

This section provides detailed lists of changes by :ref:`sample <sample>`, including protocol-related samples.
For lists of protocol-specific changes, see `Protocols`_.

Bluetooth samples
-----------------

No changes since the latest nRF Connect SDK release.

Bluetooth mesh samples
----------------------

No changes since the latest nRF Connect SDK release.

nRF9160 samples
---------------

No changes since the latest nRF Connect SDK release.

Thread samples
--------------

No changes since the latest nRF Connect SDK release.

Matter samples
--------------

* :ref:`matter_lock_sample` sample:

  * Fixed an issue that prevented nRF Toolbox for iOS in version 5.0.9 from controlling the sample using the :ref:`nus_service_readme`.

NFC samples
-----------

No changes since the latest nRF Connect SDK release.

nRF5340 samples
---------------

No changes since the latest nRF Connect SDK release.

Gazell samples
--------------

No changes since the latest nRF Connect SDK release.

Zigbee samples
--------------

No changes since the latest nRF Connect SDK release.

Other samples
-------------

* Added the nRF9230 support for the following samples:

  * :ref:`multicore_hello_world` sample
  * :ref:`app_event_manager` sample
  * :ref:`ipc_service` sample
  * :ref:`event_manager_proxy` sample
  * :ref:`caf_sensor_manager_sample` sample

* :ref:`radio_test` sample:

  * Added two radio modes with 4 Mbit/s rate.

Devicetree configuration
========================

Thingy:91
---------

No changes since the latest nRF Connect SDK release.

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

No changes since the latest nRF Connect SDK release.

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Binary libraries
----------------

No changes since the latest nRF Connect SDK release.

Bluetooth libraries and services
--------------------------------

No changes since the latest nRF Connect SDK release.

Bootloader libraries
--------------------

No changes since the latest nRF Connect SDK release.

Modem libraries
---------------

No changes since the latest nRF Connect SDK release.

Libraries for networking
------------------------

No changes since the latest nRF Connect SDK release.

Libraries for NFC
-----------------

No changes since the latest nRF Connect SDK release.

Other libraries
---------------

No changes since the latest nRF Connect SDK release.


Common Application Framework (CAF)
----------------------------------

No changes since the latest nRF Connect SDK release.

Shell libraries
---------------

No changes since the latest nRF Connect SDK release.

Libraries for Zigbee
--------------------

No changes since the latest nRF Connect SDK release.

sdk-nrfxlib
-----------

See the changelog for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for additional information.

Scripts
=======

This section provides detailed lists of changes by :ref:`script <scripts>`.

No changes since the latest nRF Connect SDK release.

Unity
-----

No changes since the latest nRF Connect SDK release.

MCUboot
=======

The MCUboot fork in the limited sampling version of the |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to, and including, ``e86f575f68fdac2cab1898e0a893c8c6d8fd0fa1``, plus some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

* No changes since the latest nRF Connect SDK release.

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each NCS release.

The Zephyr fork in the limited sampling version of the |NCS| (``fb802fb6c0af80dbd383e744065bcf1745ecbc66``) contains all commits from the |NCS| fork of the Zephyr repository (``sdk-zephyr`` sha ``eff20cfb43eb888511eb13b37cc131150cf5919c``), which is based on the upstream Zephyr Project sha ``e1e06d05fa8d1b6ac1b0dffb1712e94e308861f8``.

For the list of |NCS| specific commits, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline manifest-rev  ^e1e06d05fa8d1b6ac1b0dffb1712e94e308861f8

zcbor
=====

The `zcbor`_ module has been updated from version 0.4.0 to 0.5.1.
Release notes for 0.5.0 and 0.5.1 can be found in :file:`ncs/nrf/modules/lib/zcbor/RELEASE_NOTES.md`.
:ref:`lib_fmfu_fdev` code has been regenerated using zcbor 0.5.1.


Trusted Firmware-M
==================

No changes since the latest nRF Connect SDK release.

cJSON
=====

No changes since the latest nRF Connect SDK release.

Documentation
=============

Added:

* List and description of :ref:`directives <ug_suit_dfu_suit_directives>` used in the SUIT DFU procedure.
