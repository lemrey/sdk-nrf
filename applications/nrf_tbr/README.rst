.. _nrf_tbr_app:

nRF Thread Border Router
########################

.. contents::
   :local:
   :depth: 2

Thread Border Router (BR) is a device that enables connectivity and integration of nodes (devices) in the Thread Network(s) with other devices and services located in external networks, such as the wider Internet or local IP networks.
It also enhances security and offers management capabilities for your Thread network.
Additionally, Thread BR can handle one of the following roles: Router, Leader, or REED.
By using the Thread BR, you can extend the capabilities of your Thread devices.

nRF Thread BR application demonstrates Thread BR functionality developed with the use of Nordic Semiconductor’s products: nRF54H20 PDK and |NCS|.
The application also utilizes the Ethernet interface (`PHYTEC link board ETH`_) as an external network interface.

.. figure:: /images/nrf_tbr_app_overview.svg
    :alt: Thread BR connections

    Thread BR connections

Application overview
********************

Architecture
============

nRF Thread BR application is based on the nRF54H20 PDK and an external Ethernet interface (for example, provided by `PHYTEC link board ETH`_ that is supported by Zephyr OS).
The application is built using |NCS| that includes Zephyr OS and OpenThread stack.

The application is running on a single nRF54H20 SoC with no need to have additional external MCU to be involved.

nRF TBR architecture is presented on the following diagram:

.. figure:: /images/nrf_tbr_app_arch.svg
    :alt: Thread BR architecture

    Thread BR architecture

Features
========

The current version of nRF TBR application supports the following features:

* Bidirectional IPv6 connectivity
    IPv6 communication between Thread and non-Thread (external) networks.
    It currently supports Ethernet as the backbone link.

* mDNS & DNS-SD
    Zero-configuration discovery of services either offered by Thread BR or added by customers to BR.

* DHCPv6 + prefix delegation
    Network protocol for configuring IPv6 hosts with IP addresses, IP prefixes, default route, local segment MTU, and other configuration data required to operate in an IPV6 network.
    Prefix Delegation is one of the methods available in IPv6 for sending information to a device about what networks are available for subnetting.

* Border Agent
    This feature handles the Mesh Commissioning Protocol (MeshCoP) messages used for device commissioning.

Requirements
************

Hardware
========

The application supports the following development kit:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf54h20dk_nrf54h20_cpuapp@soc1

The application also requires the following boards:

* `PHYTEC link board ETH`_
* Daughterboard

Network
=======

Current application release supports only IPv6 network on backbone link, hence you will need the Internet backbone connection with IPv6 support for testing purposes.

Additionally, you must disable IPv6 Router Advertisement Guard on the router.

Testing prerequisites
=====================

To perform testing, the following devices are required:

* An Ethernet router with IPv6 connectivity.
  Make sure that IPv6 Router Advertisement Guard is disabled on the router.
* An nRF Thread BR devkit.
* A Thread CLI device (e.g., running on nRF52840 DK).
* A Linux host machine.

User interface
**************

nRF TBR application supports commands listed in `OpenThread CLI Reference`_.
OpenThread CLI is integrated into the system shell accessible over serial connection.
To indicate a Thread command, the ``ot`` keyword needs to precede the command.

Configuration
*************

|config|

Linux machine configuration
===========================

To let a Thread device communicate with neighbors in Wi-Fi network to which nRF Thread BR is connected, the router performs routing configuration by sending Routing Advertisement (RA) messages.
Linux host machine needs to be configured to accept RA messages sent by nRF Thread BR device.

1. Enable RAs messages to be accepted - set accept_ra parameter to ``2`` by running the following command on Linux host machine command line:

.. code-block:: console

   sudo sysctl -w n.ipv6.conf.all.accept_ra=2

#. Allow all kinds of prefix length in RAs messages to be accepted - set the accept_ra_rt_info_max_plen parameter to ``128`` by running the following command on Linux host machine's command line:

.. code-block:: console

   sudo sysctl -w n.ipv6.conf.all.accept_ra_rt_info_max_plen=128

Building and running
********************

.. |application path| replace:: :file:`applications/nrf_tbr`

.. include:: /includes/application_build_and_run.txt

For testing purposes you must build both nRF Thread BR application and Thread CLI device.

Building nRF Thread BR application
==================================

Lorem ipsum.

Ethernet-based nRF TBR application
----------------------------------

Lorem ipsum.

Building Thread CLI device
==========================

Lorem ipsum.

Testing
=======

After programming the nRF TBR and Thread CLI applications, perform the following steps to test the Thread Border Router functionality.

|test_application|

#. |connect_kit|
#. |connect_terminal|

The following diagram presents a test setup.

.. figure:: /images/nrf_tbr_app_setup.svg
    :alt: Thread BR test setup

    Thread BR test setup

Starting Thread network
-----------------------

Lorem ipsum.

DHCPv6 + Prefix Delegation
--------------------------

Lorem ipsum.

Bidirectional IPv6 connectivity
-------------------------------

Lorem ipsum.

mDNS & DNS-SD
-------------

Lorem ipsum.

Border Agent
------------

Lorem ipsum.

Dependencies
============

Lorem ipsum.

