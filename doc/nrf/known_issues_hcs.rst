.. _known_issues_hcs:

Known issues for HCS
####################

.. contents::
   :local:
   :depth: 2

Known issues listed on this page *and* tagged with the :ref:`latest initial customers sampling release version <release_notes>` are valid for the current state of development.
Use the drop-down filter to see known issues for previous releases and check if they are still valid.

.. raw:: html
   :file: includes/filter.js

.. raw:: html

   Filter by versions:

   <select name="versions" id="versions-select">
     <option value="all">All versions</option>
     <option value="v2-2-99+HCS" selected>v2.2.99+HCS</option>
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

nRF54H20
********

.. rst-class:: v2-2-99+HCS

NRFX-2444: The ``ecall`` instruction cannot be executed on the VPR cores when in a trap handler
  Executing the ``ecall`` instruction when in a trap handler (like an interrupt or an exception) halts the execution of Zephyr on the VPR cores.

.. rst-class:: v2-2-99+HCS

NRFX-3037: The GNU Project Debugger (GDB) is not reliable for debugging the VPR cores.
  GDB could crash when connecting to the PDK through SEGGER J-Link and starting a debug session.

-----

In addition to these known issues, check the following pages:

* The :ref:`known_issues` page, listing the current issues of the public |NCS| release version on which the initial customers sampling version for nRF54H20 is built upon.
* The current issues listed in the `official Zephyr repository`_, since these might apply to the |NCS| fork of the Zephyr repository as well.

To get help and report issues that are not related to Zephyr but to the |NCS|, go to Nordic's `DevZone`_.
