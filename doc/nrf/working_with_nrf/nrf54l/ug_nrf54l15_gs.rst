.. _ug_nrf54l15_gs:

Getting started with the nRF54L15 PDK
#####################################

.. contents::
   :local:
   :depth: 2

This page will get you started with your nRF54L15 PDK using the |NCS|.
First, you will test if your PDK is working correctly by running the preflashed :ref:`PWM Blinky <zephyr:blink-led-sample>` sample.
Once successfully completed, the instructions will guide you through how to configure, build, and program the :ref:`Hello World <zephyr:hello_world>` sample to the development kit, and how to read its logs.

.. _ug_nrf54l15_gs_requirements:

Minimum requirements
********************

Make sure you have all the required hardware and that your computer has one of the supported operating systems.

Hardware
========

* nRF54L15 PDK
* USB-C cable

Software
========

On your computer, one of the following operating systems:

* macOS
* Microsoft Windows
* Ubuntu Linux

|supported OS|

You also need to install `Git`_ or `Git for Windows`_ (on Linux and Mac, or Windows, respectively).

.. _ug_nrf54l15_gs_test_sample:

Testing with the Blinky sample
******************************

In the limited sampling release, the nRF54L15 PDK comes preprogrammed with the Blinky sample.

Complete the following steps to test if the PDK works correctly:

1. Connect the USB-C end of the USB-C cable to the **IMCU USB** port the nRF54L15 PDK.
#. Connect the other end of the USB-C cable to your PC.
#. Move the **POWER** switch to **On** to turn the nRF54L15 PDK on.

**LED2** will turn on and start to blink.

If something does not work as expected, contact Nordic Semiconductor support.

.. note::
   An issue in the initial production batch of the nRF54L15 PDKs causes **LED1** to be always on.
   This will be fixed in future batches.

.. _nrf54l15_gs_installing_software:

Installing the required software
********************************

To start working with the nRF54L15 PDK, you need to install the limited sampling version of the |NCS|.
See the following instructions to install the required tools.

.. _nrf54l15_install_commandline:

Installing the nRF Command Line Tools
=====================================

You need the nRF Command Line Tools 10.22.2 specific to the limited sampling release of the |NCS|.

To install the nRF Command Line Tools, you need to download and install the version corresponding to your system:

* `10.22.2_EC 64-bit Windows, executable`_
* `10.22.2_EC macOS, DMG file`_
* 64-bit Linux:

  * `10.22.2_EC x86 system, deb format`_
  * `10.22.2_EC x86 system, RPM`_
  * `10.22.2_EC x86 system, tar archive`_

  * `10.22.2_EC ARM64 system, deb format`_
  * `10.22.2_EC ARM64 system, RPM`_
  * `10.22.2_EC ARM64 system, tar archive`_

* 32-bit Linux:

  * `10.22.2_EC ARMHF system, deb format`_
  * `10.22.2_EC ARMHF system, RPM`_
  * `10.22.2_EC ARMHF system, tar archive`_

Installing the toolchain
========================

You can install the toolchain for the limited sampling of the |NCS| by running an installation script.
Before installing it, however, you need to have been granted an access to the necessary GitHub repositories using an authenticated account that does not have a passphrase key for credentials.
The access is granted as part of the onboarding process for the limited sampling release.
Ensure that you additionally have Git and curl installed.

Run the installation script:

1. Open a terminal window.
   On Windows, open Git Bash.
#. Download and run the :file:`bootstrap-toolchain.sh` script file using the following command:

   .. parsed-literal::
      :class: highlight

      curl --proto '=https' --tlsv1.2 -sSf https://developer.nordicsemi.com/.pc-tools/scripts/bootstrap-toolchain.sh | NCS_TOOLCHAIN_VERSION=v2.4.99-cs2 sh

   Depending on your connection, this might take some time.

   .. note::
      On macOS, the install directory is :file:`/opt/nordic/ncs`.
      This  means that creating the directory requires root access.
      You will be prompted to grant the script admin rights for the creation of the folder on the first install.
      The folder will be created with the necessary access rights to the user, so subsequent installs do not require root access.

      Do not run the toolchain-manager installation as root (for example, using `sudo`), as this would cause the directory to only grant access to root, meaning subsequent installations will also require root access.

#. Run the command provided to you by the script:

   .. tabs::

      .. tab:: Windows

            Run the following command in Git Bash:

            .. parsed-literal::
               :class: highlight

               c:/nordic-lcs/nrfutil.exe toolchain-manager launch --terminal --chdir "c:/nordic-lcs/west_working_dir" --ncs-version v2.4.99-cs2

            This opens a new terminal window with the |NCS| toolchain environment, where west and other development tools are available.
            Alternatively, you can run the following command::

               c:/nordic-lcs/nrfutil.exe toolchain-manager env --as-script

            This gives all the necessary environmental variables you need to copy-paste and execute in the same terminal window to be able to run west directly there.

            .. caution::
               When working with the limited sampling release, you must always use the terminal window where the west environmental variables have been called.

            If you run into errors during the installation process, delete the :file:`.west` folder inside the :file:`C:\\nordic-lcs` directory, and start over.

            We recommend adding the path where nrfutil is located to your environmental variables.

      .. tab:: Linux

            Run the following command in your terminal:

            .. parsed-literal::
               :class: highlight

               $HOME/nordic-lcs/nrfutil toolchain-manager launch --shell --chdir "$HOME/nordic-lcs/west_working_dir" --ncs-version v2.4.99-cs2

            This makes west and other development tools in the |NCS| toolchain environment available in the same shell session.

            .. caution::
               When working with west in the limited sampling release version of |NCS|, you must always use this shell window.

            If you run into errors during the installation process, delete the :file:`.west` folder inside the :file:`nordic-lcs` directory, and start over.

            We recommend adding the path where nrfutil is located to your environmental variables.

      .. tab:: macOS

            To install the required tools, complete the following steps:

            .. ncs-include:: develop/getting_started/index.rst
               :docset: zephyr
               :dedent: 6
               :start-after: .. _install_dependencies_macos:
               :end-before: group-tab:: Windows

            Ensure that these dependencies are installed with their versions as specified in the :ref:`Required tools table <req_tools_table>`.
            To check the installed versions, run the following command:

            .. parsed-literal::
               :class: highlight

                brew list --versions

.. _nrf5l15_install_ncs:

Installing the |NCS|
====================

After you have installed nRF Command Line Tools 10.22.2 and the toolchain, you need to install the |NCS|:

1. In the terminal window opened during toolchain installation, initialize west with the revision of the |NCS| from the limited sampling by running the following command:

   .. parsed-literal::
      :class: highlight

      west init -m https://github.com/nrfconnect/sdk-nrf-next --mr v2.4.99-cs2

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

   An alternative option is to create a :file:`~/.git-credentials` (or :file:`%userprofile%\\.git-credentials` on Windows) and add the following line to it::

      https://<GitHub username>:<Personal Access Token>@github.com

#. Enter the following command to clone the project repositories::

      west update

   Depending on your connection, this might take some time.

#. On Windows, export a :ref:`Zephyr CMake package <zephyr:cmake_pkg>`.
   This allows CMake to automatically load the boilerplate code required for building |NCS| applications::

      west zephyr-export

   On MacOS and Linux, skip this step.

Your directory structure now looks similar to this::

    nordic-lcs/west_working_dir
    |___ .west
    |___ bootloader
    |___ modules
    |___ nrf
    |___ nrfxlib
    |___ zephyr
    |___ ...

This is a simplified structure preview.
There are additional folders, and the structure might change over time.

.. _ug_nrf54l15_gs_sample:

Programming the Hello World! sample
***********************************

The :ref:`zephyr:hello_world_user` sample is a simple Zephyr sample.
It uses the ``nrf54l15dk_nrf54l15_cpuapp@soc1`` build target.

To build and program the sample to the nRF54L15 PDK, complete the following steps:

1. Connect the nRF54L15 PDK to you computer using the IMCU USB port on the PDK.
#. Navigate to the :file:`zephyr/samples/hello_world` folder containing the sample.
#. Build the sample by running the following command::

      west build -b nrf54l15dk_nrf54l15_cpuapp@soc1

#. Program the sample using the standard |NCS| command.
   If you have multiple Nordic Semiconductor devices, make sure that only the nRF54L15 PDK you want to program is connected.

   .. code-block:: console

      west flash

.. _nrf54l15_sample_reading_logs:

Reading the logs
****************

With the :ref:`zephyr:hello_world_user` sample programmed, the nRF54L15 PDK outputs logs over UART.

To read the logs from the :ref:`zephyr:hello_world_user` sample programmed to the nRF54L15 PDK, complete the following steps:

1. Connect to the PDK with a terminal emulator (for example, PuTTY) using the following settings:

   * Baud rate: 115200
   * 8 data bits
   * 1 stop bit
   * No parity
   * HW flow control: None

#. Press the **Reset** button on the PCB to reset the PDK.
#. Observe the console output (similar to the following):

  .. code-block:: console

   *** Booting Zephyr OS build 06af494ba663  ***
   Hello world! nrf54l15dk_nrf54l15_cpuapp@soc1

Next steps
**********

You are now all set to use the nRF54L15 PDK.
See the following links for where to go next:

* The `nRF54L15 Objective Product Specification`_ (OPS) PDF document for the nRF54L15.
* The :ref:`ug_nrf54l15_samples` page to see the available samples for the nRF54L15 PDK for the initial limited sampling.
* The :ref:`introductory documentation <getting_started>` for more information on the |NCS| and the development environment.
