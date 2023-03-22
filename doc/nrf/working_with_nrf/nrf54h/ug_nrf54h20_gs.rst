.. _ug_nrf54h20_gs:

Getting started with the nRF54H20 PDK
#####################################

.. contents::
   :local:
   :depth: 2

This page will get you started with your nRF54H20 PDK using the |NCS|.
It first tells you how to test that your PDK is working correctly by using the preflashed :ref:`zephyr:blinky-sample` sample.
After the test, it will guide you on how to configure, build, and program the :ref:`multicore_hello_world` sample to the development kit and read logs from it.

If you have already set up your nRF54H20 PDK and want to learn more, see the :ref:`ug_nrf54h20_architecture` documentation for an in-depth description of the architecture of the nRF54H20.

.. _ug_nrf54h20_gs_requirements:

Minimum requirements
********************

Make sure you have all the required hardware and that your computer has one of the supported operating systems.

Hardware
========

* nRF54H20 PDK
* USB-C cable

Software
========

On your computer, one of the following operating systems:

* Microsoft Windows
* Ubuntu Linux

|supported OS|

.. note::
   macOS is not supported for the limited sampling release on day one.
   Support will be added in the future.

You also need to install `Git`_ or `Git for Windows`_ (on Linux or Windows, respectively).

.. _ug_nrf54h20_gs_test_blinky:

Testing with the Blinky sample
******************************

In the limited sampling release, the nRF54H20 PDK comes preprogrammed with the :ref:`zephyr:blinky-sample` sample.

Complete the following steps to test that the PDK works correctly:

1. Connect the USB-C end of the USB-C cable to the **IMCU USB** port the nRF54H20 PDK.
#. Connect the other end of the USB-C cable to your PC.
#. Move the **POWER** switch to **On** to turn the nRF54H20 PDK on.
   The switch is located in the top left corner of the PDK PCB.

**LED1** will turn on and start to blink.

If something does not work as expected, contact Nordic Semiconductor support.

.. _nrf54h20_gs_installing_software:

Installing the required software
********************************

To work with the nRF54H20 PDK, you need to install the limited sampling version of the |NCS|.
Follow the instructions in the next sections to install the required tools.

.. _nrf54h20_install_commandline:

Installing the nRF Command Line Tools
=====================================

You need the nRF Command Line Tools 10.21.0 specific to the limited sampling release of the |NCS|.

To install the nRF Command Line Tools, you need to download and install the version corresponding to your system:

* `64-bit Windows, executable`_
* 64-bit Linux:

  * `x86 system, deb format`_
  * `x86 system, RPM`_
  * `x86 system, tar archive`_

  * `ARM64 system, deb format`_
  * `ARM64 system, RPM`_
  * `ARM64 system, tar archive`_

* 32-bit Linux:

  * `ARMHF system, deb format`_
  * `ARMHF system, RPM`_
  * `ARMHF system, tar archive`_

.. _nrf54h20_install_toolchain:

Installing the toolchain
========================

You can install the toolchain for the limited sampling of the |NCS| by running an installation script.

To be able to install the toolchain, you need to have been granted access to the necessary GitHub repositories using an authenticated account that does not have a passphrase key for credentials.
The access is granted as part of the onboarding process for the limited sampling release.
In addition, you need to have Git and curl installed.

To install the toolchain, complete the following steps:

1. Open a terminal window.
   On Windows, open Git Bash.
#. Download and run the :file:`bootstrap-toolchain.sh` script file using the following command:

   .. parsed-literal::
      :class: highlight

      curl --proto '=https' --tlsv1.2 -sSf https://developer.nordicsemi.com/.pc-tools/scripts/bootstrap-toolchain.sh | sh

   Depending on your connection, this might take some time.

#. Run the command provided to you by the script:

   .. tabs::

      .. tab:: Windows

            Run the following command in Git Bash:

            .. parsed-literal::
               :class: highlight

               c:/nordic-lcs/nrfutil.exe toolchain-manager launch --terminal --chdir "c:/nordic-lcs/west_working_dir" --ncs-version v2.2.99-cs1

            This opens a new terminal window with the |NCS| toolchain environment, where west and other development tools are available.
            Alternatively, you can run the following command::

               c:/nordic-lcs/nrfutil.exe toolchain-manager env --as-script'

            This gives all the necessary environmental variables you need to copy-paste and execute in the same terminal window to be able to run west directly there.

            .. caution::
               When working with the limited sampling release, you must always use the terminal window where the west environmental variables have been called.

            If you run into errors during the installation process, delete the :file:`.west` folder inside the :file:`C:\\nordic-lcs` directory, and start over.

            We recommend adding the path where nrfutil is located to your environmental variables.

      .. tab:: Linux

            Run the following command in your terminal:

            .. parsed-literal::
               :class: highlight

               $HOME/nordic-lcs/nrfutil toolchain-manager launch --terminal --chdir "$HOME/nordic-lcs/west_working_dir" --ncs-version v2.2.99-cs1

            This makes west and other development tools in the |NCS| toolchain environment available in the same shell session.

            .. caution::
               When working with west in the limited sampling release version of |NCS|, you must always use this shell window.

            If you run into errors during the installation process, delete the :file:`.west` folder inside the :file:`nordic-lcs` directory, and start over.

            We recommend adding the path where nrfutil is located to your environmental variables.

.. _nrf54h20_install_ncs:

Installing the |NCS|
====================

After you have installed nRF Command Line Tools 10.21.0 and the toolchain, complete the following steps to get the limited sampling version of the |NCS|:

1. In the terminal window opened as part of :ref:`installing the toolchain <nrf54h20_install_toolchain>`, initialize west with the revision of the nRF Connect SDK from the initial limited sampling by running the following command:

   .. parsed-literal::
      :class: highlight

      west init -m https://github.com/nrfconnect/sdk-nrf-next --mr v2.2.99-cs1

   A window pops up to ask you to select a credential helper.
   You can use any of the options.

#. Set up GitHub authentication:

   ``west update`` requires :ref:`west <zephyr:west>` to fetch from private repositories on GitHub.

   Because the `west manifest file`_ uses ``https://`` URLs instead of ``ssh://``, you may be prompted to type your GitHub username and Personal Access Token multiple times.
   GitHub has a comprehensive `documentation page <https://docs.github.com/en/authentication/keeping-your-account-and-data-secure/about-authentication-to-github>`_ on the subject.
   In many cases (including Windows), the Git installation includes `Git Credential Manager <https://github.com/git-ecosystem/git-credential-manager>`_, which will handle GitHub authentication and is the recommended method for handling GitHub authentication.

   However, if you are already using `SSH-based authentication <https://docs.github.com/en/authentication/connecting-to-github-with-ssh/generating-a-new-ssh-key-and-adding-it-to-the-ssh-agent>`_, you can reuse your SSH setup by adding the following to your :file:`~/.gitconfig` (or :file:`%userprofile%\\.gitconfig` on Windows):

   .. parsed-literal::
      :class: highlight

         [url "ssh://git@github.com"]
               insteadOf = https://github.com

   This will rewrite the URLs on the fly so that Git uses ``ssh://`` for all network operations with GitHub.

   Another option instead is to create a :file:`~/.git-credentials` (or :file:`%userprofile%\\.git-credentials` on Windows) and add this line to it::

      https://<GitHub username>:<Personal Access Token>@github.com

#. Enter the following command to clone the project repositories::

      west update

   Depending on your connection, this might take some time.

#. Export a :ref:`Zephyr CMake package <zephyr:cmake_pkg>`.
   This allows CMake to automatically load the boilerplate code required for building |NCS| applications::

      west zephyr-export

#. As an administrator (or superuser), install ``nrf-regtool`` using the following command::

      pip install modules/lib/nrf-regtool

Your directory structure now looks similar to this::

    nordic-lcs/west_working_dir
    |___ .west
    |___ bootloader
    |___ modules
    |___ nrf
    |___ nrfxlib
    |___ zephyr
    |___ ...


Note that there are additional folders, and that the structure might change.
The full set of repositories and folders is defined in the manifest file.

.. _ug_nrf54h20_gs_sample:

Programming the sample
**********************

The :ref:`multicore_hello_world` sample is a multicore sample running on both the Application core (``cpuapp``) and the Peripheral Processor (PPR, ``cpuppr``).
It uses the ``nrf54h20dk_nrf54h20_cpuapp@soc1`` build target.

.. caution::
   You should use west to program the nRF54H20 PDK during the limited sampling release.
   The ``west`` command is available from the terminal window you opened in Step 3 of `nrf54h20_install_toolchain`_.

   Do not use the ``nrfjprog -e`` command to program the nRF54H20 PDK, as it will brick the device.
   Use the ``west flash --erase-storage`` command instead.

To build and program the sample to the nRF54H20 PDK, complete the following steps:

1. Connect the nRF54H20 PDK to you computer using the IMCU USB port on the PDK.
#. Navigate to the :file:`nrf/samples/multicore/hello_world` folder containing the sample.
#. Build the sample by running the following command::

      west build -b nrf54h20dk_nrf54h20_cpuapp@soc1

#. Program the sample using the standard |NCS| command.
   If you have multiple Nordic Semiconductor devices, make sure that only the nRF54H20 PDK you want to program is connected.

   .. code-block:: console

      west flash

The sample will be automatically built and programmed on both the Application core and the Peripheral Processor (PPR) of the nRF54H20.

.. _nrf54h20_sample_reading_logs:

Reading the logs
****************

With the :ref:`multicore_hello_world` sample programmed, the nRF54H20 PDK outputs logs for the application core and the peripheral processor.
The logs are output over UART.

To read the logs from the :ref:`multicore_hello_world` sample programmed to the nRF54H20 PDK, complete the following steps:

1. Connect to the PDK with a terminal emulator (for example, PuTTY) using the following settings:

   * Baud rate: 115200
   * 8 data bits
   * 1 stop bit
   * No parity
   * HW flow control: None

#. Press the **Reset** button on the PCB to reset the PDK.
#. Observe the console output for both cores:

   * For the application core, the output should be as follows:

     .. code-block:: console

        *** Booting Zephyr OS build v2.7.99-ncs1-2193-gd359a86abf14  ***
        Hello world from nrf54h20dk_nrf54h20_cpuapp

   * For the PPR core, the output should be as follows:

     .. code-block:: console

        *** Booting Zephyr OS build v2.7.99-ncs1-2193-gd359a86abf14  ***
        Hello world from nrf54h20dk_nrf54h20_cpuppr

See the :ref:`ug_nrf54h20_logging` page for more information.

Next steps
**********

You are now all set to use the nRF54H20 PDK.
See the following links for where to go next:

* :ref:`ug_nrf54h20_architecture` for information about the multicore System-on-Chip, such as the responsibilities of the cores and their interprocessor interactions, the memory mapping, and the boot sequence.
* :ref:`ug_nrf54h20_app_samples` to see the available samples for the nRF54H20 PDK for the initial limited sampling.
* The :ref:`introductory documentation <getting_started>` for more information on the |NCS| and the development environment.
