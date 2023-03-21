:orphan:

.. _known_issues_lcs:

Known issues and limitations for nRF54H20
#########################################

.. contents::
   :local:
   :depth: 2

Known issues and limitations listed on this page *and* tagged with the :ref:`latest customer sampling release version <release_notes>` are valid for the current state of development.

This page only lists the known issues and limitations that are related to the customer sampling (CS) of nRF54H20.
``v2.2.99-cs1`` indicates the first customer sampling based on |NCS| v2.2.99.

.. raw:: html
   :file: includes/filter.js

.. raw:: html

   Filter by versions:

   <select name="versions" id="versions-select">
     <option value="all">All versions</option>
     <option value="v2-2-99-cs1" selected>v2.2.99-cs1</option>
   </select>


.. HOWTO

   When adding a new version, add it to the dropdown list above and move the "selected" option next to it.
   Once "selected" is moved, only issues that are valid for the new version will be displayed when entering the page.

   When updating this file, add entries in the following format:

   .. rst-class:: vXXX vYYY

   JIRA-XXXX: Title of the issue
     Description of the issue.
     Start every sentence on a new line.

     There can be several paragraphs, but they must be indented correctly.

     **Workaround:** The last paragraph contains the workaround.

Known issues
************

.. rst-class:: v2-2-99-cs1

NRFX-2444: The ``ecall`` instruction cannot be executed on the VPR cores when in a trap handler
  Executing the ``ecall`` instruction when in a trap handler (like an interrupt or an exception) halts the execution of Zephyr on the VPR cores.

.. rst-class:: v2-2-99-cs1

NRFX-3037: The GNU Project Debugger (GDB) is not reliable for debugging the VPR cores
  GDB might crash when connecting to the PDK through SEGGER J-Link and starting a debug session.

.. rst-class:: v2-2-99-cs1

NRFX-3222: STM logging backend without ETR data decoding hangs randomly
  When STM logging uses a backend without ETR data decoding, tracing might hang randomly.
  This mode of operation requires nRF Util trace to decode logs and is not a default mode.

.. rst-class:: v2-2-99-cs1

HM-20886: The radio receiver sensitivity is too low when the radio PHY is set to the LE Coded ``S=2`` coding scheme
  When using the ``S=2`` coding scheme, the value of the packet error rate is higher than expected.

.. rst-class:: v2-2-99-cs1

NCSDK-20616: The LE Secure Connection pairing with MITM protection does not work between nRF54H20 devices
  The nRF54H20 devices generate the same public key due to pseudo-random generator being used, which causes pairing to be rejected to protect devices against an attack where the attacker responds with the same public key.
  This issue can be observed when running the :ref:`peripheral_hids_keyboard` or the :ref:`peripheral_hids_mouse` samples against the :ref:`bluetooth_central_hids` sample on the nRF54H20 PDKs.
  
-----

In addition to these known issues, check the following pages:

* The :ref:`known_issues` page, listing the current issues of the public |NCS| release version on which this customer sampling version for nRF54H20 is built upon.
* The current issues listed in the `official Zephyr repository`_, since these might apply to the |NCS| fork of the Zephyr repository as well.

To get help and report issues that are not related to Zephyr but to the |NCS|, go to Nordic's `DevZone`_.

Limitations
***********


* This release of |NCS| is only meant to be used for sampling the nRF54H20 features.
  No other features have been tested for this release.
* macOS is currently not supported.
  Support will be added in the future.
* Using the ``nrfjprog -e`` command to erase the storage on the nRF54H20 PDK will brick the device.
  Use ``west flash --erase-storage`` instead.
* If you cancel a ``west flash`` command before it finishes running, for example by pressing ctrl+C, the device might require a power cycle before it can be flashed again.
