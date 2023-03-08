.. _ncs_release_notes_changelog_hcs:

Changelog for |NCS| v2.2.99 + HCS
#################################

.. contents::
   :local:
   :depth: 2

This page tracks the most relevant changes that are present on the branch of the initial limited sampling for nRF54H20 of the |NCS|, as compared to the |NCS| version 2.2.99 (commit hash ``d69930b57ff79433c2fded560c6e7ecdb65b9886``).
You can find the most relevant changes of the |NCS| version 2.2.99 :ref:`here <ncs_release_notes_changelog>`.

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
See `known issues for nRF Connect SDK v2.2.0`_ for the list of issues valid for the latest release.

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

No changes since the latest nRF Connect SDK release.

NFC samples
-----------

No changes since the latest nRF Connect SDK release.

nRF5340 samples
---------------

* Updated:

  * Moved ``nRF5340: Multicore application`` sample to :ref:`multicore_hello_world`.

Gazell samples
--------------

No changes since the latest nRF Connect SDK release.

Zigbee samples
--------------

No changes since the latest nRF Connect SDK release.

Other samples
-------------

:ref:`ipc_service_sample` sample:

* Added support for the nRF54H20 DK with board targets nrf54h20dk_nrf54h20_cpuapp@soc1 and nrf54h20dk_nrf54h20_cpurad@soc1.

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

The documentation was updated to add information regarding nRF54H20 and the initial limited sampling.
The main updates are contained in the :ref:`ug_nrf54h` section.
