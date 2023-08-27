.. _nrf_tbr_app:

nRF Thread Border Router
########################

.. contents::
   :local:
   :depth: 2

The Thread Border Router is a type of a network device that provides connectivity from the Thread network
to adjacent networks on other physical layers (such as Wi-Fi or Ethernet). Border Router provides services
for devices within the IEEE 802.15.4 network, including routing services for off-network operations.

nRF Thread Border Router (nRF TBR) is a Nordic Semiconductor's implementation of Thread Border Router.

.. figure:: /images/nrf_tbr_app_overview.svg
    :alt: Thread BR connections

    Thread BR connections

Application overview
********************

Architecture
============

nRF TBR application runs on the single SoC and utilizes external internet interface (`Hardware`_).
The application is built using |NCS| that includes Zephyr RTOS and OpenThread stack.

nRF TBR architecture is presented on the following diagram:

.. figure:: /images/nrf_tbr_app_arch.svg
    :alt: Thread BR architecture

    nRF Thread BR architecture

Features
========

The current version of nRF TBR supports the following features:

* Bidirectional IPv6 connectivity
    IPv6 communication between Thread and non-Thread (external) networks.
    It currently supports Ethernet as the backbone link.

* mDNS
    Multicast based discovery of services.

* DHCPv6 and prefix delegation
    Network protocol for configuring IPv6 hosts with IP addresses, IP prefixes, default route, local segment MTU,
    and other configuration data required to operate in an IPv6 network.

* Border Agent
    Relaying MeshCoP messages between a Thread Network and an external Thread Commissioner.

Requirements
************

Hardware
========

The application supports the following development kit:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf54h20dk_nrf54h20_cpuapp@soc1

Besides the PDK the following hardware modules are also needed for nRF TBR application:

..
  Need some more information about "Daughterboard"

* `PHYTEC link board ETH`_
* Daughterboard

.. figure:: /images/nrf_tbr_app_hardware.svg
    :alt: Hardware connection

    Hardware connection

Testing prerequisites
=====================

Current version of the application supports only IPv6 connectivity between Thread and non-Thread devices over the backbone link.

.. note::
  The application can support IPv4 networking within non-Thread network for other purposes, e.g. mDNS and name resolution.

To perform testing, the following devices are required:

..
  TODO: Add link to Thread CLI sample documentation for "Optional:" point: https://res.developer.nordicsemi.com/ncs_dev/doc/PR-1397/nrf/samples/openthread/cli/README.html

* A router with IPv6 connectivity through Ethernet or a network switch.
* The nRF54H20 PDK (`Hardware`_ ) with nRF TBR application.
* Linux host machine.
* Optional: Thread CLI device (e.g.running on nRF52840 DK).

..
  We need to somehow stress this more as it may break testing environment

.. note::
  nRF TBR uses ICMPv6 Router Advertisement messages to share routing information and its prefixes with
  other devices. If non-Thread network nodes are connected through a router make sure that IPv6 Router Advertisement
  Guard feature on the router is disabled. Otherwise, it can disrupt communication between both networks.

User interface
**************

nRF TBR application supports commands listed in `OpenThread CLI Reference`_.
OpenThread CLI is integrated into the system shell accessible over serial connection.
To indicate a Thread command, the ``ot`` keyword needs to precede the command.

Moreover, the application provides an additional set of commands related to nRF TBR
features. To indicate a nRF TBR command, the ``tbr`` keyword need to precede the command.
In order to list available commands run the ``tbr`` command without any parameters.

Configuration
*************

|config|

Linux machine configuration
===========================

To let Thread devices communicate with non-Thread networks, nRF TBR provides routing configuration and prefixes by sending Routing Advertisement (RA) messages.
Linux host machine has to be configured to accept incoming RA messages.

#. Enable acceptance of RA messages - set accept_ra parameter to ``2`` by running the following command on Linux host terminal:

   .. code-block:: console

     sudo sysctl -w n.ipv6.conf.all.accept_ra=2

#. Allow all kinds of prefix length in RAs messages to be accepted - set the accept_ra_rt_info_max_plen parameter to ``128`` by
   running the following command on Linux host machine's command line:

   .. code-block:: console

     sudo sysctl -w n.ipv6.conf.all.accept_ra_rt_info_max_plen=128

Building and running nRF TBR application
========================================

.. |application path| replace:: :file:`applications/thread_border_router`

.. include:: /includes/application_build_and_run.txt

For testing purposes Thread CLI sample can be used alongside nRF TBR application.

Building Thread CLI device (optional)
*************************************

Instuction how to build Thread CLI device can be found here: :ref:`.. _ot_cli_sample_building_and_running:`

Testing
=======

This section presents ways to test Border Router functionalites

The following diagram presents a test setup.

..
  NOTE: we need to change "Thread CLI Device (end device)" to "Thread End Device (optional)"

.. figure:: /images/nrf_tbr_app_setup.svg
    :alt: Thread BR test setup

    Thread BR test setup

.. include:: /includes/thread_enable_network.txt

..
  Note on PuTTY - In Thread docs there are links to a guide how to use it, maybe we could add this too?

To verify the communication between nRF Thread Border Router and non-Thread network, complete the following steps:

.. note::
  In order to communicate with non-Thread networks nRF TBR generates off-mesh route (OMR) prefix that is propagated as a part of Thread Network Data. All device in the network use it generates an IPv6 addres that is later used for communication with devices from external network. To check OMR prefix use the ``ot br omrprefix`` command.

#. Ensure that nRF54H20 development kit is programmed with nRF Thread Border Router application.
#. Turn on the development kit
#. Connect to the development kit with a terminal emulator, e.g. PuTTY.
#. .. include:: /includes/thread_configure_network.txt
#. .. include:: /includes/thread_enable_network.txt
#. Wait until the device becomes a leader of the network. In order to check the current state use ``ot state`` command.
#. Ping Linux Host machine using the ``ot ping <ip address>`` command. Example command output is presented below:

   .. code-block:: console

      uart:~$ ot ping fdde:ad00:beef:cafe:5569:2ae8:30b6:b25b
      fdde:ad00:beef:cafe:5569:2ae8:30b6:b25b: icmp_seq=3 hlim=64 time=5ms
      1 packets transmitted, 1 packets received. Packet loss = 0.0%. Round-trip min/avg/max = 5/5.0/5 ms.

#. Ping nRF TBR's OMR address from Linux Host using ``ping <ip address>``. Example command output is presented below:

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


Testing connectivity with the End Device
----------------------------------------

To verify the communication between Thread End Device and non-Thread device, complete the following steps:

..
  Here we need a link to Thread CLI page under 'Ensure that development kit'

#. Perform steps described in `Testing connectivity frome the Border Router`_ to establish the connection
   with an external network.
#. Ensure that End Device's development kit is programmed with Thread CLI sample.
#. Turn on the End Device's development kit.
#. Connect to the development kit with a terminal emulator, e.g. PuTTY.
#. .. include:: /includes/thread_configure_network.txt
#. .. include:: /includes/thread_enable_network.txt
#. Wait until the device becomes a child or router. In order to check the current state use ``ot state`` command.
#. Ping Linux Host machine using the ``ot ping <ip address>`` command. Example command output is presented below:

   .. code-block:: console
      uart:~$ ot ping fd0b:3fe0:f9b4:0:3477:7647:4943:842a
      16 bytes from fd0b:3fe0:f9b4:0:3477:7647:4943:842a: icmp_seq=1 hlim=64 time=18ms
      1 packets transmitted, 1 packets received. Packet loss = 0.0%. Round-trip min/avg/max = 18/18.0/18 ms.

#. Ping End Device's OMR address from Linux Host using ``ping <ip address>``. Example command output is presented below (more information about OMR addres: `Testing`_):

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


DHCPv6 and Prefix Delegation
----------------------------

During the nRF Thread Border Router startup it automatically communicates with DHCPv6 server in order
to receive an address for non-Thread interface. Moreover, it requests a delegated prefix for subnetting.

Information about network interface can be displayed and verified using the following ``net iface`` command.
Assigned address by DHCP server should be listed in 'IPv6 Unicast addresses' section as in the example command output below:

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

nRF Thread Border Router provides mDNS module used by OpenThread stack that also provides interface to publish services in runtime (more information about Command Line Interface: `User interface`_).

List of available actions:

- to publish a custom TXT record, use the following command:
``tbr mdns publish <instance> <service> <proto> <domain> "<TXT record>"``

.. code-block:: console

   uart:~$ tbr mdns publish nrftbr-app _thread _udp _local "keyone=1 keytwo=2 keythree=3"
   Published record. Index: 1

- list published records, use the following command:
 ``tbr mdns records``

.. code-block:: console

   uart:~$ tbr mdns records"
   1. nrftbr-app._thread._udp._local
   2. vendor-switch._thread._udp._local

-  update published record, use the following command:
``tbr mdns update <record index>``

- unpublish the record, use the following command:
``tbr mdns unpublish <record index>``


Border Agent
------------

nRF Thread Border Router always advertises Border Agent service. It provides the following information:

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

Border Agent service can be discovered by any software supporting mDNS protocol. For example, to discover the service, use open-source tool [Avahi](https://www.avahi.org/) and ``avahi-browse -rt _meshcop._udp`` command.
.. code-block:: console

   user@host:~$ avahi-browse -rt _meshcop._udp
   + enx00133bb15706 IPv4 nordicsemi-tbr                                _meshcop._udp        local
   = enx00133bb15706 IPv4 nordicsemi-tbr                                _meshcop._udp        local
      hostname = [nordicsemi-tbr.local]
      address = [192.168.1.154]
      port = [52861]
      txt = ["omr=@\253\023J\150\155Y\000\001" "at=\000\000\000\000\000\000\000\000" "sb=\000\000\000I" "xa=~P\2085m0\195\180" "tv=1.3.0" "xp=\222\173\000\190\239\000\202\254" "nn=OpenThread" "mn=tbr" "vn=nordicsemi"]

Dependencies
============

This application integrates Thread Protocol
* :ref:`_ug_thread:`

The application depends on the following Zephyr libraries:

* :ref:`zephyr:logging_api`
* :ref:`zephyr:kernel_api`
