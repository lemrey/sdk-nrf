.. _nrf_tbr_app:

nRF54H20: nRF Thread Border Router
##################################

.. contents::
   :local:
   :depth: 2

The nRF Thread Border Router (nRF TBR) demonstrates the Thread Border Router functionality for the nRF54H20 PDK.

Application overview
********************

The Thread Border Router is a type of a network device that provides connectivity from the Thread network to adjacent networks on other physical layers - Wi-Fi or Ethernet.
The Thread Border Router provides also services for devices within the IEEE 802.15.4 network, including routing services for off-network operations.

.. figure:: /images/nrf_tbr_app_overview.svg
    :alt: Thread Border Router connections

    Thread Border Router connections

The nRF TBR application is a Nordic Semiconductor's implementation of the Thread Border Router.

Current version of the application supports only IPv6 connectivity between Thread and non-Thread devices over the backbone link.

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
    It currently supports Ethernet as the backbone link.

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

* `PHYTEC link board ETH`_ as an external interface for Internet connection
* nRF54-Style-DK IO adapter board (PCA64161) as a daughterboard

.. figure:: /images/nrf_tbr_app_hardware.svg
    :alt: Hardware connection

    Hardware connection

To perform testing, the following additional devices are required:

* A router with IPv6 connectivity through Ethernet or a network switch
* A Linux host machine

Optionally, you can also use the :ref:`Thread CLI device <ot_cli_sample>` (running on, for example, the nRF52840 DK).

.. note::
   nRF TBR uses ICMPv6 Router Advertisement messages to share routing information and its prefixes with other devices.
   If non-Thread network nodes are connected through a router, make sure that IPv6 Router Advertisement Guard feature on the router is disabled.
   Otherwise, communication between the networks may be disrupted.

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

For testing purposes, the :ref:`ot_cli_sample` sample can be used together with nRF TBR application.

Thread CLI device building
==========================

Optionally, you can also build the Thread CLI device.
To do so, follow the instructions from the :ref:`ot_cli_sample_building_and_running` section .

Testing
=======

This section presents a few ways to test the nRF TBR application functionalites.

The following diagram illustrates a test setup:

.. figure:: /images/nrf_tbr_app_setup.svg
    :alt: Thread BR test setup

    Thread BR test setup

Starting Thread network
-----------------------

.. include:: /includes/thread_enable_network.txt

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

..
  Note on PuTTY - In Thread docs there are links to a guide how to use it, maybe we could add this too?

Testing connection with Thread Border Router
--------------------------------------------

To verify the communication between the nRF Thread Border Router and non-Thread network, complete the following steps:

1. Ensure that the nRF54H20 PDK is programmed with the nRF TBR application.
#. Turn on the PDK.
#. Connect to the PDK with a terminal emulator, e.g., PuTTY.
#. .. include:: /includes/thread_configure_network.txt
#. .. include:: /includes/thread_enable_network.txt
#. Wait until the device becomes a leader of the network.
   To check the current state, use the ``ot state`` command.
#. Ping Linux host machine by using the ``ot ping <ip address>`` command.
   Following is a sample command output:

   .. code-block:: console

      uart:~$ ot ping fdde:ad00:beef:cafe:5569:2ae8:30b6:b25b
      fdde:ad00:beef:cafe:5569:2ae8:30b6:b25b: icmp_seq=3 hlim=64 time=5ms
      1 packets transmitted, 1 packets received. Packet loss = 0.0%. Round-trip min/avg/max = 5/5.0/5 ms.

#. Ping nRF TBR's OMR address from the Linux host machine by using the ``ping <ip address>`` command.

   .. note::
      To communicate with non-Thread networks, the nRF TBR generates an off-mesh route (OMR) prefix that is propagated as a part of the Thread Network Data.
      All devices in the network use it to generate an IPv6 address that is later used for communication with devices from an external network.
      To check the OMR prefix, use the ``ot br omrprefix`` command.

   Following is a sample command output:

   .. code-block:: console

      user@host:~$ ping fd17:4a96:9b59:1:ac19:43e0:b905:c0af -c 4
      PING fd17:4a96:9b59:1:ac19:43e0:b905:c0af(fd17:4a96:9b59:1:ac19:43e0:b905:c0af) 56 data bytes
      64 bytes from fd17:4a96:9b59:1:ac19:43e0:b905:c0af: icmp_seq=1 ttl=64 time=4.51 ms
      64 bytes from fd17:4a96:9b59:1:ac19:43e0:b905:c0af: icmp_seq=2 ttl=64 time=4.50 ms
      64 bytes from fd17:4a96:9b59:1:ac19:43e0:b905:c0af: icmp_seq=3 ttl=64 time=4.54 ms
      64 bytes from fd17:4a96:9b59:1:ac19:43e0:b905:c0af: icmp_seq=4 ttl=64 time=4.39 ms
      --- fd17:4a96:9b59:1:ac19:43e0:b905:c0af ping statistics ---
      4 packets transmitted, 4 received, 0% packet loss, time 3005ms
      rtt min/avg/max/mdev = 4.391/4.483/4.537/0.055 ms

Testing connection with End Device
----------------------------------

To verify the communication between Thread End Device and non-Thread device, complete the following steps:

..
  Here we need a link to Thread CLI page under 'Ensure that development kit'

1. Perform steps described in `Testing connection with Thread Border Router`_ to establish connection with an external network.
#. Ensure that End Device's development kit is programmed with the :ref:`ot_cli_sample` sample.
#. Turn on the End Device's development kit.
#. Connect to the development kit with a terminal emulator, e.g., PuTTY.
#. Bring up the Thread interface on the End Device (`Starting Thread network`_).
#. .. include:: /includes/thread_configure_network.txt
#. .. include:: /includes/thread_enable_network.txt
#. Wait until the device becomes either a Child or Router.
   To check the current state, use the ``ot state`` command.
#. Ping the Linux host machine by using the ``ot ping <ip address>`` command.
   Following is a sample command output:

   .. code-block:: console

      uart:~$ ot ping fd0b:3fe0:f9b4:0:3477:7647:4943:842a
      16 bytes from fd0b:3fe0:f9b4:0:3477:7647:4943:842a: icmp_seq=1 hlim=64 time=18ms
      1 packets transmitted, 1 packets received. Packet loss = 0.0%. Round-trip min/avg/max = 18/18.0/18 ms.

#. Ping End Device's OMR address from the Linux host machine by using the ``ping <ip address>`` command.
   Following is a sample command output:

   .. code-block:: console

      user@host:~$ ping -c 4 fd17:4a96:9b59:1:bca2:8690:269c:9684
      PING fd17:4a96:9b59:1:bca2:8690:269c:9684(fd17:4a96:9b59:1:bca2:8690:269c:9684) 56 data bytes
      64 bytes from fd17:4a96:9b59:1:bca2:8690:269c:9684: icmp_seq=1 ttl=64 time=36.3 ms
      64 bytes from fd17:4a96:9b59:1:bca2:8690:269c:9684: icmp_seq=2 ttl=64 time=21.6 ms
      64 bytes from fd17:4a96:9b59:1:bca2:8690:269c:9684: icmp_seq=3 ttl=64 time=20.2 ms
      64 bytes from fd17:4a96:9b59:1:bca2:8690:269c:9684: icmp_seq=4 ttl=64 time=19.5 ms
      --- fd17:4a96:9b59:1:bca2:8690:269c:9684 ping statistics ---
      4 packets transmitted, 4 received, 0% packet loss, time 3005ms
      rtt min/avg/max/mdev = 19.472/24.395/36.261/6.893 ms

DHCPv6 and prefix delegation
----------------------------

During the nRF TBR startup, the application automatically communicates with the DHCPv6 server to receive an address for a non-Thread interface.
Moreover, it requests a delegated prefix for subnetting.

Use the ``net iface`` command to display and verify information about network interface.
The address assigned by the DHCP server should be listed in `IPv6 Unicast addresses` section as in the following sample command output:

.. code-block:: console

   Interface 0x2f002480 (Ethernet) [2]
   ===================================
   Link addr : 68:27:19:EF:56:1D
   MTU       : 1500
   Flags     : AUTO_START,IPv4,IPv6
   Ethernet capabilities supported:
           10 Mbits
           100 Mbits
   IPv6 unicast addresses (max 7):
           fe80::6a27:19ff:feef:561d autoconf preferred infinite
           2001:db8::1 manual preferred infinite
           fd0b:3fe0:f9b4:0:6a27:19ff:feef:561d autoconf preferred infinite
           fd0b:3fe0:f9b4::578 DHCP preferred
   IPv6 multicast addresses (max 7):
           ff02::1
           ff02::1:ffef:561d
           ff02::1:ff00:1
           ff02::2
           ff02::1:ff00:578
   IPv6 prefixes (max 6):
           fd0b:3fe0:f9b4::/64 infinite
           fd0b:3fe0:f9b4:4::/62
   IPv6 hop limit           : 64
   IPv6 base reachable time : 30000
   IPv6 reachable time      : 34339
   IPv6 retransmit timer    : 0
   IPv4 unicast addresses (max 1):
           192.168.1.154 DHCP preferred
   IPv4 multicast addresses (max 1):
           <none>
   IPv4 gateway : 192.168.1.1
   IPv4 netmask : 255.255.255.0
   DHCPv4 lease time : 43200
   DHCPv4 renew time : 21600
   DHCPv4 server     : 192.168.1.1
   DHCPv4 requested  : 192.168.1.154
   DHCPv4 state      : bound
   DHCPv4 attempts   : 1

mDNS
----

The nRF Thread Border Router application provides the mDNS module used by the OpenThread stack.
The stack provides also an interface to publish services in runtime.
Read more about the Command Line Interface in the `User interface`_ section.

The following actions are available:

* Publish a custom TXT record by using the ``tbr mdns publish <instance> <service> <proto> <domain> "<TXT record>"`` command.

  .. code-block:: console

     uart:~$ tbr mdns publish nrftbr-app _thread _udp _local "keyone=1 keytwo=2 keythree=3"
     Published record. Index: 1

* List all published records by using the ``tbr mdns records`` command.

  .. code-block:: console

     uart:~$ tbr mdns records
     1. nrftbr-app._thread._udp._local
     2. vendor-switch._thread._udp._local

* Update a published record by using the ``tbr mdns update <record index>`` command.

* Unpublish a record using the ``tbr mdns unpublish <record index>`` command.

Border Agent
------------

The nRF Thread Border Router always advertises the Border Agent service.
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
