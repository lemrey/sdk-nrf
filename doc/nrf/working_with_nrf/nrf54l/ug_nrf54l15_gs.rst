.. _ug_nrf54l15_gs:

Getting started with the nRF54L15 PDK
#####################################

.. contents::
   :local:
   :depth: 2

.. caution::
   Keep in mind that this page is still a work in progress and should not be referred to for any guidelines.

This page will get you started with your nRF54L15 PDK using the |NCS|.
First, you will test if your PDK is working correctly by running the preflashed :ref:`PWM Blinky <zephyr:blink-led-sample>` sample.
Once successfully completed, the instructions will guide you through how to configure, build, and program the  :ref:`Hello World <zephyr:hello_world>` sample to the development kit, and how to read its logs.

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

You also need to install `Git`_.

.. _ug_nrf54l15_gs_test_sample:

Testing with the Blinky sample
******************************

In the limited sampling release, the nRF54L15 PDK comes preprogrammed with the Blinky sample.

Complete the following steps to test if the PDK works correctly:

1. Connect the USB-C end of the USB-C cable to the **IMCU USB** port the nRF54L15 PDK.
#. Connect the other end of the USB-C cable to your PC.
#. Move the **POWER** switch to **On** to turn the nRF54L15 PDK on.

**LED1** will turn on and start to blink.

If something does not work as expected, contact Nordic Semiconductor support.

.. _nrf54l15_gs_installing_software:

Installing the required software
********************************

To start working with the nRF54L15 PDK, you need to install the limited sampling version of the |NCS|.
See the following instructions to install the required tools.

.. _nrf54l15_install_commandline:

Installing the nRF Command Line Tools
=====================================

For the limited customer sampling, you need the `nRF Command Line Tools 10.22.1`_ release of the |NCS|.
Ensure to download and install the version corresponding to your system.

Installing the toolchain
========================

You can install the toolchain for the limited sampling of the |NCS| by running an installation script.
Before installing it, however, you need to have been granted an access to the necessary GitHub repositories using an authenticated account that does not have a passphrase key for credentials.
The access is granted as part of the onboarding process for the limited sampling release.
Ensure you additionally have Git and curl installed.

Run the installation script:

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

               $HOME/nordic-lcs/nrfutil toolchain-manager launch --shell --chdir "$HOME/nordic-lcs/west_working_dir" --ncs-version v2.2.99-cs1

            This makes west and other development tools in the |NCS| toolchain environment available in the same shell session.

            .. caution::
               When working with west in the limited sampling release version of |NCS|, you must always use this shell window.

            If you run into errors during the installation process, delete the :file:`.west` folder inside the :file:`nordic-lcs` directory, and start over.

            We recommend adding the path where nrfutil is located to your environmental variables.

      .. tab:: macOS

            TBA

.. _nrf5l15_install_ncs:

Installing the |NCS|
====================

After you have installed nRF Command Line Tools 10.22.1 and the toolchain, you need to install the |NCS|:

1. In the terminal window opened during toolchain installation, initialize west with the revision of the |NCS| from the limited sampling by running the following command:

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

   An alternative option is to create a :file:`~/.git-credentials` (or :file:`%userprofile%\\.git-credentials` on Windows) and add the following line to it::

      https://<GitHub username>:<Personal Access Token>@github.com

#. Enter the following command to clone the project repositories::

      west update

   Depending on your connection, this might take some time.

#. Export a :ref:`Zephyr CMake package <zephyr:cmake_pkg>`.
   This allows CMake to automatically load the boilerplate code required for building |NCS| applications::

      west zephyr-export

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

.. _ug_nrf54l15_gs_sample:

Programming the sample
**********************

TBA

.. _nrf54l15_sample_reading_logs:

Reading the logs
****************

TBA

.. _nRF Command Line Tools 10.22.1: https://www.nordicsemi.com/Products/Development-tools/nrf-command-line-tools/download
