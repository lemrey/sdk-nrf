.. _known_issues_lcs:

Known issues and limitations for the limited sampling
#####################################################

.. contents::
   :local:
   :depth: 2

Known issues and limitations listed on this page *and* tagged with the :ref:`latest customer sampling release version <release_notes>` are valid for the current state of development.

This page only lists the known issues and limitations that are related to the customer sampling (CS) of the nRF54H20 and nRF54L15 SoCs.
``v2.4.99-cs3`` indicates the latest limited sampling release based on |NCS| v2.4.99.

.. raw:: html
   :file: includes/filter.js

.. raw:: html

   Filter by versions:

   <select name="versions" id="versions-select">
     <option value="all">All versions</option>
     <option value="v2-4-99-cs3" selected>v2.4.99-cs3</option>
     <option value="v2-4-99-cs2">v2.4.99-cs2</option>
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

.. rst-class:: v2-4-99-cs3

NCSDK-24846: The nRF54H20 PDK can behave like a bricked device in specific instances.
  In specific instances, after a cold boot, the nRF54H20 PDK can behave like a bricked device (being unreachable via debugger, or non-booting).

  **Workaround**: Briefly tap (for less than 0.5 seconds) the Reset button on the PDK.

.. rst-class:: v2-4-99-cs3

NCSDK-24884: MacOS toolchain installation script for Apple silicon installs libraries for Intel silicon.
  The toolchain installation script for MacOS installs on Apple CPUs libraries for Intel CPUs.

  **Workaround**: After the activation of the toolchain environment, run the following commands:

  .. code-block::

     pip uninstall -y lxml
     pip install lxml

.. rst-class:: v2-4-99-cs3

DRGN-20961: Possible assert when starting the scanner/initiator due to a late and unexpected HFCLK ramp-up when requesting a radio event.
  No workaround for this issue is currently available.

.. rst-class:: v2-4-99-cs3

CVE-2023-2626: OpenThread KeyID Mode 2 Security Vulnerability
  This vulnerability impacts all Thread devices using OpenThread and allows an attacker in physical proximity to compromise non-router-capable devices and the entire Thread network in the case of router-capable devices.
  The vulnerability allows an attacker in physical proximity to inject arbitrary IPv6 packets into the Thread network via IEEE 802.15.4 frame transmissions.
  Because the Thread Management Framework (TMF) protocol does not have any additional layer of security, the attacker could exploit this vulnerability to update the Thread Network Key and gain full access to the Thread network.
  There is no known exploitation of vulnerability.
  Due to this issue, the Thread certifications for OpenThread libraries in all |NCS| releases up to v2.4.0, which the limited customer sampling is based on, are deprecated.

.. rst-class:: v2-4-99-cs3

The PCA10156 v0.2.0 AA0-ES3 and earlier versions of the nRF54L15 PDK do not work with the ``v2-4-99-cs3`` release
  To make sure your device works correctly, use the customer sampling release version compatible with your PDK.
  For details, refer to the compatibility matrix in :ref:`migration_guides`.

.. rst-class:: v2-4-99-cs3

KRKNWK-18160: The :ref:`nrf_tbr_app` could send multiple PING responses during IPv6 multicast forwarding
  When using Wi-Fi, the Wi-Fi interface of the :ref:`nrf_tbr_app` loops back multicast packets to itself, resulting in flooding the network with packets
  No workaround for this issue is currently available.

.. rst-class:: v2-4-99-cs3

KRKNWK-18183: The :ref:`nrf_tbr_app` in debug configuration with Wi-Fi and NAT64 enabled does not build due to a FLASH overflow
  When building the :ref:`nrf_tbr_app` application in debug configuration with WiFi and NAT64 enabled, the build fails due to a FLASH overflow.
  No workaround for this issue is currently available.

.. rst-class:: v2-4-99-cs2

NRFX-2444: The ``ecall`` instruction cannot be executed on the VPR cores when in a trap handler
  Executing the ``ecall`` instruction when in a trap handler (like an interrupt or an exception) halts the execution of Zephyr on the VPR cores.

.. rst-class:: v2-4-99-cs2

NRFX-3037: The GNU Project Debugger (GDB) is not reliable for debugging the VPR cores
  GDB might crash when connecting to the PDK through SEGGER J-Link and starting a debug session.

.. rst-class:: v2-4-99-cs2

NRFX-3222: STM logging backend without ETR data decoding hangs randomly
  When STM logging uses a backend without ETR data decoding, tracing might hang randomly.
  This mode of operation requires nRF Util trace to decode logs and is not a default mode.

.. rst-class:: v2-4-99-cs2

HM-20886: The radio receiver sensitivity is too low when the radio PHY is set to the LE Coded ``S=2`` coding scheme
  When using the ``S=2`` coding scheme, the value of the packet error rate is higher than expected.

.. rst-class:: v2-4-99-cs3 v2-4-99-cs2

NCSDK-20616: The LE Secure Connection pairing with MITM protection does not work between nRF54H20 devices
  The nRF54H20 devices generate the same public key due to pseudo-random generator being used, which causes pairing to be rejected to protect devices against an attack where the attacker responds with the same public key.
  This issue can be observed when running the :ref:`peripheral_hids_keyboard` or the :ref:`peripheral_hids_mouse` samples against the :ref:`bluetooth_central_hids` sample on the nRF54H20 PDKs.

.. rst-class:: v2-4-99-cs2

KRKNWK-17225: Matter over Thread commissioning might be unstable due to lack of true random generator support on nRF54H20
  After each reboot or factory reset, the device will always have the same Bluetooth LE and IEEE 80215.4 addresses.
  This might impact working within the Thread network because after the second and following connections, Thread Border Router will reject these connections until deleted from the database and commissioning to Matter will take more and more time.

  **Workaround:** After each factory reset and before performing the next commissioning to Matter, connect to the device's serial port and run the following command:

    .. parsed-literal::
       :class: highlight

       ot extaddr *address*

  Replace the *address* argument with an 8-byte randomly generated MAC address, for example ``87fb47d5730ac0a0``.

.. rst-class:: v2-4-99-cs2

An issue in the initial production batch of the nRF54L15 PDKs causes **LED1** to be always lit
  In the ``2.4.99-cs2`` release, applications and samples running on the nRF54L15 PDK use **LED2**, **LED3**, and **LED4** in place of **LED1**, **LED2**, **LED3**, respectively, while information normally indicated by **LED4** is not indicated at all.
  This will be fixed in future batches.

.. rst-class:: v2-4-99-cs2

The initial production batches of the nRF54L15 PDK have **Button 3** and **Button 4** connected to a GPIO port that does not support interrupts (GPIO port 2)
  This is an error in the initial PDK design

  **Workaround:** To make **Button 3** and **Button 4** functional on the nRF54L15 PDK, enable the :kconfig:option:`CONFIG_DK_LIBRARY_BUTTON_NO_ISR` Kconfig option in the configuration for the sample.
  The DK Buttons and LEDs library, with the :kconfig:option:`CONFIG_DK_LIBRARY_BUTTON_NO_ISR` Kconfig option enabled, polls the PDK button state periodically (50 ms by default) and reports its status according to the poll results.

  Using the :kconfig:option:`CONFIG_DK_LIBRARY_BUTTON_NO_ISR` Kconfig option increases the overall power consumption of the system.
  When measuring power consumption, disable this option.

.. rst-class:: v2-4-99-cs2

When using the SAADC driver on the nRF54L15 PDK, it returns incorrect values for the gain setting different from ``1``.
  The fix for this issue will be introduced in the next release.

.. rst-class:: v2-4-99-cs2

The UART output from the nRF54L15 PDK v0.2.1 and later version is corrupted when using the ``2.4.99-cs2`` release
  **Workaround:** The peripheral calibration data in FICR is not used by default in the ``2.4.99-cs2`` release.
  You can change this by defining NRF_APPLY_FICR_TRIMS in the :file:`modules/hal/nordic_haltium/nrfx/mdk/system_nrf54l.c` file.
  This solution resolves the UART baudrate issue.

.. rst-class:: v2-4-99-cs2

NCSDK-22925: Sampling in the Machine Learning application does not work in multicore configuration on the nRF54H20
  No workaround for this issue at this point.

.. rst-class:: v2-4-99-cs2

NRFX-4563: Shell does not work when running a sample that uses the UARTE SHIM and enables :kconfig:option:`CONFIG_SHELL_BACKEND_SERIAL_INTERRUPT_DRIVEN` on the nRF54L15
  **Workaround:** Disable :kconfig:option:`CONFIG_SHELL_BACKEND_SERIAL_INTERRUPT_DRIVEN` and use polling shell

.. rst-class:: v2-4-99-cs2

NRFX-3886: UARTE async API may be unstable at high throughput on the nRF54L15
  No workaround for this issue at this point.

.. rst-class:: v2-4-99-cs3 v2-4-99-cs2

NRFX-4567: UARTE API does not implement power management on the nRF54L15
  No workaround for this issue at this point.

.. rst-class:: v2-4-99-cs3 v2-4-99-cs2

NRFX-4568: DMA RX events might not arrive on the nRF54L15.
  **Workaround:** Enable ``CONFIG_SOC_NRF_FORCE_CONSTLAT`` and force constant latency mode.

.. rst-class:: v2-4-99-cs2

NRFX-4460: GPIOTE can be used with only one GPIO port on nRF54L15 PDK
  No workaround for this issue at this point.

-----

In addition to these known issues, check the following pages:

* The :ref:`known_issues` page, listing the current issues of the public |NCS| release version on which this customer sampling version for nRF54 is built upon.
* The current issues listed in the `official Zephyr repository`_, since these might apply to the |NCS| fork of the Zephyr repository as well.

To get help and report issues that are not related to Zephyr but to the |NCS|, go to Nordic's `DevZone`_.

Limitations
***********


* This release of |NCS| is only meant to be used for sampling the features of the nRF54H20 and nRF54L15 SoCs.
  No other features have been tested for this release.
* If you cancel a ``west flash`` command before it finishes running, for example by pressing ``Ctrl+C``, the device might require a power cycle before it can be flashed again.
* TIMER, SPIM, and UARTE in MCU PD are not functional when the MCU is running at 128 MHz on the nRF54L15 PDK.
* nRF54H20 USB device controller driver limitations:

  * An automatic fallback to full speed is not supported when the controller is connected to the FS bus (current stack limitation).
  * Isochronous transfers (transactions) are not yet supported (for example, there is no support for the USB audio).
  * Handling of the ``connect``, ``disconnect``, ``suspend``, and ``resume`` bus events is not yet supported due to the design of the driver and the nRF54H20 hardware.
  * Driver and controller can only be initialized and used after the application initialization level.
  * The number of user-usable endpoints is limited to 4 IN and 3 OUT.

* The Bluetooth LE Controller on the nRF54L15 PDK and nRF54H20 PDK currently uses the test PRNG module as its source for entropy (see the :kconfig:option:`CONFIG_ENTROPY_TEST_PRNG` Kconfig option).
* For Matter and Thread limitations, see :ref:`Matter and Thread limitations on nRF54H20 <ug_nrf54h20_matter_thread_limitations>` and :ref:`Matter and Thread limitations on nRF54L15 <ug_nRF54l15_matter_thread_limitations>`.
* Cryptography limitations:

  * Hardware-accelerated cryptography through CRACEN is not supported in this release.
    All crypto functionality in this release utilizes software-enabled cryptography (except for RNG support in nRF54L15 devices).
  * TF-M is not supported for existing and new nRF devices in this limited sampling release.
    Use the regular nRF Connect SDK release for TF-M support in nRF5340 and nRF91 devices.
