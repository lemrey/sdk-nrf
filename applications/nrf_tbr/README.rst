.. _nrf_tbr_app:

nRF Thread Border Router
########################

.. contents::
   :local:
   :depth: 2

The nRF Thread Border Router (nRF TBR) application demonstrates the Thread Border Router functionality for Nordic's SoC.

Application overview
********************

The Thread Border Router is a type of a network device that provides connectivity from the Thread network to adjacent networks on other physical layers - Wi-Fi or Ethernet.
The Thread Border Router provides also services for devices within the IEEE 802.15.4 network, including routing services for off-network operations.

.. figure:: /images/nrf_tbr_app_overview.svg
    :alt: Thread Border Router connections

    Thread Border Router connections

The nRF TBR application is a Nordic Semiconductor's implementation of the Thread Border Router.

Current version of the application supports only IPv6 connectivity between Thread and non-Thread devices over the external network link.

.. 
    note::
   The application can support IPv4 networking for some purposes, e.g., mDNS or name resolution.

Architecture
============

The nRF TBR application runs on a single SoC and utilizes an interface extension board for Internet connection.
The application is built using the |NCS| that includes the Zephyr RTOS and the OpenThread stack.

The nRF TBR architecture is presented on the following diagram:

.. figure:: /images/nrf_tbr_app_arch.svg
    :alt: nRF TBR architecture

    nRF TBR architecture

Features
========

The current version of the nRF TBR application supports the following features:

* Bidirectional IPv6 connectivity
    IPv6 communication between Thread and non-Thread (external) networks.
    It currently supports Ethernet as the external network link.

* mDNS
    A multicast-based discovery of services.

* DHCPv6 and prefix delegation
    A network protocol for configuring IPv6 hosts with IP addresses, IP prefixes, default route, local segment MTU, and other configuration data required to operate in the IPv6 network.

* Border Agent
    A function that relays MeshCoP messages between the Thread Network and an external Thread Commissioner.

Requirements
************

To use the application, a number of hardwarde and software requirements must be met.

The application supports the following preview development kit (PDK):

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf54h20dk_nrf54h20_cpuapp@soc1

Besides the PDK, the application needs also the following hardware modules:

* `PHYTEC link board ETH`_ extension board as an external interface for Internet connection
* nRF54-Style-DK IO adapter board (PCA64161) as SHIM (Shove Hardware in the Middle) from `PHYTEC link board ETH`_ extension board to the nRF54H20 PDK

.. figure:: /images/nrf_tbr_app_hardware.svg
    :alt: Hardware connection

    Hardware setup connection

To perform testing, the following additional devices are required:

* A router with IPv6 connectivity through Ethernet or a network switch

   .. note::
      nRF TBR uses ICMPv6 Router Advertisement messages to share routing information and its prefixes with other devices.
      If non-Thread network nodes are connected through a router, make sure that IPv6 Router Advertisement Guard feature on the router is disabled.
      Otherwise, communication between the networks may be disrupted.

* A Linux host machine
* Optionally, you can also use the :ref:`Thread CLI device <ot_cli_sample>` (running on, for example, the nRF52840 DK).


User interface
**************

nRF TBR application supports commands listed in `OpenThread CLI Reference`_.
OpenThread CLI is integrated into the system shell accessible over serial connection.
To indicate a Thread command, the ``ot`` keyword needs to precede the command.

Moreover, the application provides an additional set of commands related to nRF TBR features.
To indicate a nRF TBR command, the ``tbr`` keyword needs to precede the command.
Run the ``tbr`` command without any parameters to list all available commands.

Configuration
*************

|config|

Building and running
********************

.. |application path| replace:: :file:`applications/thread_border_router`

.. include:: /includes/application_build_and_run.txt

Thread CLI device building
==========================

For testing purposes, optionally the :ref:`ot_cli_sample` sample can be used together with nRF TBR application.
To do so, follow the instructions from the :ref:`ot_cli_sample_building_and_running` section of the Thread CLI sample.

Testing
=======

This section presents a few ways to test the nRF TBR application functionalities.

The following diagram illustrates a test setup:

.. figure:: /images/nrf_tbr_app_setup.svg
    :alt: Thread BR test setup

    Thread BR test setup

Linux machine configuration
---------------------------

To allow Thread devices to communicate with non-Thread networks, the nRF TBR application provides routing configuration and prefixes by sending Routing Advertisement (RA) messages.
You must configure a Linux host machine so that it can accept incoming RA messages.
To do so, perform the following steps:

1. Enable acceptance of RA messages - set the ``accept_ra`` parameter to ``2`` by running the following command on Linux host terminal:

   .. code-block:: console

      sudo sysctl -w n.ipv6.conf.all.accept_ra=2

#. Allow all kinds of prefix length in RAs messages to be accepted - set the ``accept_ra_rt_info_max_plen`` parameter to ``128`` by running the following command on Linux host terminal:

   .. code-block:: console

      sudo sysctl -w n.ipv6.conf.all.accept_ra_rt_info_max_plen=128

DHCPv6 and prefix delegation
----------------------------

During the nRF TBR startup, the application automatically communicates with the DHCPv6 server to receive an address for a non-Thread interface.
Moreover, it requests a delegated prefix for subnetting.

Use the ``net iface`` command to display and verify information about network interface.
The address assigned by the DHCP server should be listed in `IPv6 Unicast addresses` section for `Ethernet` interface as in the following sample command output:

.. code-block:: console

   Interface 0x2f0024b0 (Ethernet) [2]
   ===================================
   Link addr : 68:27:19:EF:56:01
   MTU       : 1500
   Flags     : AUTO_START,IPv4,IPv6
   Ethernet capabilities supported:
         10 Mbits
         100 Mbits
   IPv6 unicast addresses (max 7):
         fe80::6a27:19ff:feef:5601 autoconf preferred infinite
         2001:db8::1 manual preferred infinite
         2001:dead:beef:cafe:6a27:19ff:feef:5601 autoconf preferred infinite
         2001:dead:beef:cafe::28c DHCP preferred
   IPv6 multicast addresses (max 7):
         ff02::1
         ff02::1:ffef:5601
         ff02::1:ff00:1
         ff02::fb
         ff02::2
         ff02::1:ff00:28c
   IPv6 prefixes (max 6):
         2001:dead:beef:cafe::/64 infinite
   IPv6 hop limit           : 64
   IPv6 base reachable time : 30000
   IPv6 reachable time      : 34339
   IPv6 retransmit timer    : 0
   IPv4 unicast addresses (max 1):
         192.168.2.126 DHCP preferred
   IPv4 multicast addresses (max 2):
         224.0.0.1
         224.0.0.251
   IPv4 gateway : 192.168.2.1
   IPv4 netmask : 255.255.255.0
   DHCPv4 lease time : 43200
   DHCPv4 renew time : 21600
   DHCPv4 server     : 192.168.2.1
   DHCPv4 requested  : 192.168.2.126
   DHCPv4 state      : bound
   DHCPv4 attempts   : 1
   DHCPv6 address requested : 2001:dead:beef:cafe::28c
   DHCPv6 prefix requested  : none
   DHCPv6 state             : bound
   DHCPv6 attempts          : 1

Testing communication with Thread Border Router
-----------------------------------------------

To verify the communication between the nRF TBR and non-Thread network, complete the following steps:

1. Ensure that the nRF54H20 PDK is programmed with the nRF TBR application.
#. Turn on the PDK.
#. |connect_terminal_ANSI|

   .. note::
        |thread_hwfc_enabled|

#. .. include:: /includes/thread_configure_network.txt
#. .. include:: /includes/thread_enable_network.txt
#. Wait until the device becomes a leader of the network.
   To check the current state, use the ``ot state`` command.
   For example:

   .. code-block:: console

      uart:~$ ot state
      leader
      Done

#. Ping Linux host machine by using the ``ot ping <linux ip address>`` command.
   Following is a sample command output:

   .. code-block:: console

      uart:~$ ot ping fdde:ad00:beef:cafe:e6d3:c0a7:5dbd:6420
      16 bytes from fdde:ad00:beef:cafe:e6d3:c0a7:5dbd:6420: icmp_seq=1 hlim=64 time=10ms
      1 packets transmitted, 1 packets received. Packet loss = 0.0%. Round-trip min/avg/max = 10/10.0/10 ms.
      Done

#. Get OMR IP address for nRF TBR with the ``ot ipaddr`` command.
   Example addresses listed in command output:

   .. code-block:: console

      uart:~$ ot ipaddr
      fd17:4a96:9b59:1:ac19:43e0:b905:c0af
      fdde:ad00:beef:0:0:ff:fe00:fc00
      fdde:ad00:beef:0:0:ff:fe00:d000
      fdde:ad00:beef:0:5f39:9cc8:9c62:d290
      fe80:0:0:0:7c50:d035:6d30:c3b4
      Done

   .. note::
      To communicate with non-Thread networks, the nRF TBR generates an off-mesh route (OMR) prefix that is propagated as a part of the Thread Network Data.
      All devices in the network use it to generate an IPv6 address that is later used for communication with devices from an external network.
      To check the OMR prefix, use the ``ot br omrprefix`` command:

      .. code-block:: console

         uart:~$ ot br omrprefix
         Local: fd17:4a96:9b59:1::/64
         Favored: fd17:4a96:9b59:1::/64 prf:low
         Done

      An example OMR prefix **fd17:4a96:9b59:1::/64** is used to create nRF TBR OMR address that can be reached from Ethernet network.
      Notice the IPv6 address **fd17:4a96:9b59:1:ac19:43e0:b905:c0af** starts with OMR prefix.

#. Ping nRF TBR's OMR address from the Linux host machine by using the ``ping <nRF TBR ip address>`` command.
   Following is a sample linux command output:

   .. code-block:: console

      user@host:~$ ping fd17:4a96:9b59:1:ac19:43e0:b905:c0af -c 3
      PING fd17:4a96:9b59:1:ac19:43e0:b905:c0af(fd17:4a96:9b59:1:ac19:43e0:b905:c0af) 56 data bytes
      64 bytes from fd17:4a96:9b59:1:ac19:43e0:b905:c0af: icmp_seq=1 ttl=64 time=3.97 ms
      64 bytes from fd17:4a96:9b59:1:ac19:43e0:b905:c0af: icmp_seq=2 ttl=64 time=3.85 ms
      64 bytes from fd17:4a96:9b59:1:ac19:43e0:b905:c0af: icmp_seq=3 ttl=64 time=3.88 ms

      --- fd17:4a96:9b59:1:ac19:43e0:b905:c0af ping statistics ---
      3 packets transmitted, 3 received, 0% packet loss, time 2002ms
      rtt min/avg/max/mdev = 3.853/3.900/3.973/0.052 ms

Testing communication with Thread node over nRF TBR
---------------------------------------------------

After performing all steps from `Testing communication with Thread Border Router`_ you can optionally verify the communication between Thread Router or End Device and non-Thread device.

..
  Here we need a link to Thread CLI page under 'Ensure that development kit'

1. Make sure that development kit (DK) intended to work as Thread node is programmed with CLI sample (`Thread CLI device building`_).
#. Turn on the DK.
#. |connect_terminal_ANSI|

   .. note::
        |thread_hwfc_enabled|

#. .. include:: /includes/thread_configure_network.txt
#. .. include:: /includes/thread_enable_network.txt
#. Wait until the device joins to Thread network created by nRF TBR in `Testing communication with Thread Border Router`_.
   To verify whether the device joined to Thread network, use the ``ot state`` command.
   Device should becomes either a `child` or `router`:

   .. code-block:: console

      uart:~$ ot state
      router
      Done

#. Ping the Linux host machine by using the ``ot ping <linux ip address>`` command.
   Following is a sample command output:

   .. code-block:: console

      uart:~$ ot ping fdde:ad00:beef:cafe:e6d3:c0a7:5dbd:6420
      16 bytes from fdde:ad00:beef:cafe:e6d3:c0a7:5dbd:6420: icmp_seq=2 hlim=64 time=18ms
      1 packets transmitted, 1 packets received. Packet loss = 0.0%. Round-trip min/avg/max = 18/18.0/18 ms.
      Done

#. Get the IP address for the device with the ``ot ipaddr`` command.
   Example addresses listed in command output:

   .. code-block:: console

      uart:~$ ot ipaddr
      fdde:ad00:beef:0:0:ff:fe00:4400
      fd17:4a96:9b59:1:6684:f56d:c4ed:65a0
      fdde:ad00:beef:0:35f3:129:a24f:412d
      fe80:0:0:0:4c0b:ed8d:2b5:915b
      Done

#. Ping Thread node OMR address from the Linux host machine by using the ``ping <node ip address>`` command.
   Following is a sample command output:

   .. code-block:: console

      user@host:~$ ping fd17:4a96:9b59:1:6684:f56d:c4ed:65a0 -c 3
      PING fd17:4a96:9b59:1:6684:f56d:c4ed:65a0(fd17:4a96:9b59:1:6684:f56d:c4ed:65a0) 56 data bytes
      64 bytes from fd17:4a96:9b59:1:6684:f56d:c4ed:65a0: icmp_seq=1 ttl=64 time=25.8 ms
      64 bytes from fd17:4a96:9b59:1:6684:f56d:c4ed:65a0: icmp_seq=2 ttl=64 time=20.4 ms
      64 bytes from fd17:4a96:9b59:1:6684:f56d:c4ed:65a0: icmp_seq=3 ttl=64 time=18.8 ms

      --- fd17:4a96:9b59:1:6684:f56d:c4ed:65a0 ping statistics ---
      3 packets transmitted, 3 received, 0% packet loss, time 2003ms
      rtt min/avg/max/mdev = 18.809/21.691/25.817/2.993 ms

Border Agent
------------

The nRF TBR always advertises the Border Agent service.
It provides the following information:

* Vendor name
* Product name
* Network name
* Extended PAN identifier
* Thread version
* Extended address of the device
* Current state
* Active timestamp
* Backbone router state
* Thread Domain name
* Thread Off-Mesh route prefix

The Border Agent service can be discovered by any software supporting the mDNS protocol.
For example, to discover the service, use the `Avahi`_ open-source tool and run the ``avahi-browse -rt _meshcop._udp`` command.

.. code-block:: console

   user@host:~$ avahi-browse -rt _meshcop._udp
   + enx00133bb15706 IPv4 nordicsemi-tbr                                _meshcop._udp        local
   = enx00133bb15706 IPv4 nordicsemi-tbr                                _meshcop._udp        local
      hostname = [nordicsemi-tbr.local]
      address = [192.168.1.154]
      port = [52861]
      txt = ["omr=@\253\023J\150\155Y\000\001" "at=\000\000\000\000\000\000\000\000" "sb=\000\000\000I" "xa=~P\2085m0\195\180" "tv=1.3.0" "xp=\222\173\000\190\239\000\202\254" "nn=OpenThread" "mn=tbr" "vn=nordicsemi"]

Related samples
===============

You can use the :ref:`ot_cli_sample` sample for testing purposes.

Dependencies
************

This application uses the :ref:`ug_thread` protocol.

The application uses the following Zephyr libraries:

* :ref:`zephyr:logging_api`
* :ref:`zephyr:kernel_api`
