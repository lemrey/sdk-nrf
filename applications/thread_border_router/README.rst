.. _nrf_tbr_app:

nRF Thread Border Router
########################

.. contents::
   :local:
   :depth: 2

The nRF Thread Border Router (nRF TBR) application demonstrates the Thread Border Router functionality for Nordic Semiconductor's SoCs.

Application overview
********************

The Thread Border Router is a type of network device that provides connectivity from the Thread network to adjacent networks on other physical layers - Wi-Fi or Ethernet.
The Thread Border Router also provides services for devices within the IEEE 802.15.4 network, including routing services for off-network operations.

.. figure:: /images/nrf_tbr_app_overview.svg
    :alt: Thread Border Router connections

    Thread Border Router connections

The nRF TBR application is Nordic Semiconductor's implementation of the Thread Border Router.

It supports IPv4 and IPv6 connectivity between Thread and non-Thread devices over the external network link.

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

Bidirectional IPv6 connectivity
  IPv6 communication between Thread and non-Thread (external) networks.
  It currently supports Ethernet as the external network link.

mDNS
  A multicast-based discovery of services.

DHCPv6 and prefix delegation
  A network protocol for configuring IPv6 hosts with IP addresses, IP prefixes, default route, local segment MTU, and other configuration data required to operate in the IPv6 network.

Border Agent
  A function that relays MeshCoP messages between the Thread Network and an external Thread Commissioner.

IPv4 support (NAT64/DNS64)
  Description

Multicast forwarding
  Multicast Forwarding offers reaching devices on the Thread and Wi-Fi or Ethernet network in the same multicast group from both directions.

ND Proxy
  A handling of Neighbor Solicitation messages to provide information about IPv6 addresses of Thread Nodes with active Domain Unicast Addresses.

Requirements
************

The application supports the following preview development kit (PDK):

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf54h20dk_nrf54h20_cpuapp@soc1

In addition to the PDK, the application needs the following hardware modules:

* The `PHYTEC link board ETH`_ extension board or Wi-Fi `nRF7002 EB`_ (Wi-Fi connection) as an external interface for Internet connection.
* An nRF54-Style-DK IO adapter board (PCA64161) as a SHIM (Shove Hardware in the Middle), connecting the `PHYTEC link board ETH`_ extension board or Wi-Fi `nRF7002 EB`_ to the nRF54H20 PDK.

.. figure:: /images/nrf_tbr_app_hardware.svg
    :alt: Hardware connection

    Hardware setup connection

To perform testing, the following additional devices are required:

* One of the following networking devices depending on the connection:

  * A router with IPv6 or IPv4 (if the NAT feature is enabled) connectivity for Ethernet or Wi-Fi connection.
  * A network switch for Ethernet connection only.

   .. note::
      nRF TBR uses ICMPv6 Router Advertisement messages to share routing information and its prefixes with other devices.
      If non-Thread network nodes are connected through a router, make sure that IPv6 Router Advertisement Guard feature on the router is disabled.
      Otherwise, communication between the networks may be disrupted.

* A Linux host machine.
* Optionally, you can also use the :ref:`Thread CLI device <ot_cli_sample>` running on, for example, the nRF52840 DK.


User interface
**************

nRF TBR application supports commands listed in the `OpenThread CLI Reference`_.
The OpenThread CLI is integrated into the system shell accessible over the serial connection.
To indicate a Thread command, the ``ot`` keyword needs to precede the command.

The application also provides an additional set of commands related to nRF TBR features.
To indicate a nRF TBR command, the ``tbr`` keyword needs to precede the command.
Run the ``tbr`` command without any parameters to list all available commands.

Configuration
*************

|config|

.. _nrf_tbr_app_configuration:

nRF TBR configuration files
===========================

The nRF TBR application provides predefined configuration files for different build types, and they are located in the :file:`applications/thread_border_router` directory.

The following configuration files are provided:

* :file:`prj.conf` - This file contains the standard configuration for the nRF TBR application and represents a ``debug`` build type.

* :file:`prj_release.conf` - This configuration file represents a ``release`` build type of the application.
  You can use this version to enable only the necessary application functionalities to optimize its performance and size.
  This configuration file can be included by adding ``-DCONF_FILE=prj_release.conf`` to your build command. See :ref:`cmake_options`.

* :file:`overlay-nat64.conf` - The overlay file that adds NAT64 and all necessary features required to access resources in IPv4 networks from Thread network.
  You can include it by adding ``-DOVERLAY_CONFIG=overlay-nat64.conf`` to your build command.

See :ref:`app_build_system` for more information on the |NCS| configuration system.

.. note::
    :ref:`nrf_tbr_app_select_configuration` is optional.
    The :file:`prj.conf` build type is used by default if no build type is explicitly selected.

Building and running
********************

.. |application path| replace:: :file:`applications/thread_border_router`
.. |build command| replace:: west build -p -b nrf54h20dk_nrf54h20_cpuapp\@soc1

.. include:: /includes/application_build_and_run_54h.txt

.. _nrf_tbr_app_select_configuration:

Selecting a configuration type
==============================

Before you start testing the application, you can select one of the :ref:`nrf_tbr_app_configuration`, depending on your building method.

Selecting a build type in |VSC|
-------------------------------

.. include:: /getting_started/modifying.rst
   :start-after: build_types_selection_vsc_start
   :end-before: build_types_selection_vsc_end

Selecting a build type from command line
----------------------------------------

.. include:: /getting_started/modifying.rst
   :start-after: build_types_selection_cmd_start
   :end-before: For example, you can replace the

For example, you can replace the *selected_build_type* variable to build the ``release`` firmware for ``nrf54h20dk_nrf54h20_cpuapp`` by running the following command in the project directory:

.. code-block:: console

   west build -b nrf54h20dk_nrf54h20_cpuapp@soc1 -- -DCONF_FILE=prj_release.conf

Also, you can enable specific feature for ``nrf54h20dk_nrf54h20_cpuapp`` by running the following command in the project directory:

.. code-block:: console

   west build -b nrf54h20dk_nrf54h20_cpuapp@soc1 -- -DOVERLAY_CONFIG=overlay-nat64.conf

Thread CLI device building
==========================

For testing purposes, you can optionally use the :ref:`ot_cli_sample` sample together with nRF TBR application.
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

1. Enable acceptance of RA messages.
   Set the ``accept_ra`` parameter to ``2`` by running the following command on Linux host terminal:

   .. code-block:: console

      sudo sysctl -w net.ipv6.conf.all.accept_ra=2

#. Allow all kinds of prefix lengths in RA messages to be accepted.
   Set the ``accept_ra_rt_info_max_plen`` parameter to ``128`` by running the following command on Linux host terminal:

   .. code-block:: console

      sudo sysctl -w net.ipv6.conf.all.accept_ra_rt_info_max_plen=128

DHCPv6 and prefix delegation
----------------------------

During the nRF TBR startup, the application automatically communicates with the DHCPv6 server to receive an address for a non-Thread interface.
Moreover, it requests a delegated prefix for subnetting.

Use the ``net iface`` command to display and verify information about the network interface.
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

1. Connect to the PDK that runs the nRF TBR application with a terminal emulator that supports VT100/ANSI escape characters (for example, PuTTY).
   See :ref:`putty` for the required settings.

   .. note::
        |app_thread_hwfc_enabled|

#. .. include:: /includes/thread_configure_network.txt
#. .. include:: /includes/thread_enable_network.txt
#. Wait until the device becomes a leader of the network.
   To check the current state, use the ``ot state`` command.
   For example:

   .. code-block:: console

      uart:~$ ot state
      leader
      Done

#. Ping the Linux host machine by using the ``ot ping <linux ip address>`` command.
   Following is a sample command output:

   .. code-block:: console

      uart:~$ ot ping fdde:ad00:beef:cafe:e6d3:c0a7:5dbd:6420
      16 bytes from fdde:ad00:beef:cafe:e6d3:c0a7:5dbd:6420: icmp_seq=1 hlim=64 time=10ms
      1 packets transmitted, 1 packets received. Packet loss = 0.0%. Round-trip min/avg/max = 10/10.0/10 ms.
      Done

#. Get the OMR IP address for nRF TBR with the ``ot ipaddr`` command.
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

      In the above example, the OMR prefix **fd17:4a96:9b59:1::/64** is used to create the nRF TBR OMR address that can be reached from the Ethernet network.
      Notice that the IPv6 address **fd17:4a96:9b59:1:ac19:43e0:b905:c0af** starts with the OMR prefix.

#. Ping the nRF TBR's OMR address from the Linux host machine by using the ``ping <nRF TBR ip address>`` command.
   Following is a sample Linux command output:

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

After performing all steps from `Testing communication with Thread Border Router`_ you can optionally verify the communication between a Thread Router or End Device and a non-Thread device.

1. Make sure that the development kit (DK) that will work as Thread node is programmed with the CLI sample (`Thread CLI device building`_).
#. Turn on the DK.
#. |connect_terminal_ANSI|

   .. note::
        |thread_hwfc_enabled|

#. .. include:: /includes/thread_configure_network.txt
#. .. include:: /includes/thread_enable_network.txt
#. Wait until the device joins to Thread network created by nRF TBR in `Testing communication with Thread Border Router`_.
   To verify whether the device joined to Thread network, use the ``ot state`` command.
   Device should become either a ``child`` or ``router``:

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

#. Ping the Thread node OMR address from the Linux host machine by using the ``ping <node ip address>`` command.
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

Multicast Forwarding
--------------------

After performing all steps from `Testing communication with Thread Border Router`_ you can optionally verify bidirectional communication using multicast messages between a Thread Router or End Device and a non-Thread device.

#. Make sure that the development kit (DK) that will work as Thread node is programmed with the CLI sample (`Thread CLI device building`_).

   .. note::
      In order to perform Multicast Listener Registration the CLI sample must be built with enabled CONFIG_OPENTHREAD_MLR option.

#. Turn on the DK.
#. |connect_terminal_ANSI|

   .. note::
        |thread_hwfc_enabled|

#. .. include:: /includes/thread_configure_network.txt
#. .. include:: /includes/thread_enable_network.txt
#. Wait until the device joins to Thread network created by nRF TBR in `Testing communication with Thread Border Router`_.
   To verify whether the device joined to Thread network, use the ``ot state`` command.
   Device should become either a ``child`` or ``router``:

   .. code-block:: console

      uart:~$ ot state
      router
      Done

#. Add new IPv6 multicast address with "Admin-Local" scope or higher. In the following example End Device subscribes to the ``ff05::abcd`` address:

   .. code-block:: console

      uart:~$ ot ipmaddr add ff05::abcd
      Done

#. Open an UDP port and subscribe to the multicast address on the Linux host.
   For example, use the socat open-source tool and run the ``socat -u UDP6-RECV:<port>,ipv6-join-group='[<address>]':<interface>: -`` command where the ``<port>`` has to be changed to a given UDP port number, the ``<address>`` has to be changed to a given IPv6 multicast address, and the ``<interface>`` has to be changed to a network interface name or its index. Then, it will wait for incoming UDP datagrams and print their payload to the standard output:

   .. code-block:: console

      user@host:~$ socat -u UDP6-RECVFROM:55555,ipv6-join-group='[ff05::abcd]':2: -

#. Open an UDP socket with the ``ot udp open`` command:

   .. code-block:: console

      uart:~$ ot udp open
      Done

#. Send an UDP datagram with a given payload using the ``ot udp send <address> <port> <text payload>`` command where the ``<address>`` and the ``<port>`` have to match values provided to the socat command:

   .. code-block:: console

      uart:~$ ot udp send ff05::abcd 55555 foobar
      Done

#. Verfy that the given payload is printed to the standard output of the Linux Host.

#. Bind an UDP port with the ``ot bind :: <port>`` command where ``<port>`` should be changed to a chosen value.

   .. code-block:: console

      uart:~$ ot udp bind :: 55555
      Done

#. The socat tool can also be used to send UDP datagrams from the Linux Host to the End Device.
   For example, use the ``echo "<payload>" | socat -u - UDP6-SENDTO:[<address>]:<port>,so-bindtodevice=<interface>`` command where ``<payload>`` specifies the text message, ``<address>`` and the ``<port>`` have to match values provided to the End Device, and the ``<interface>`` has to be changed to a given Linux Host's network interface's name:

   .. code-block:: console

      user@host:~$ echo " foobar" | socat -u  - UDP6-SENDTO:[ff05::abcd]:55555,so-bindtodevice=enx00133bb15706

#. Verify that the given payload is printed to the End Device's serial:

   .. code-block:: console

      8 bytes from fdde:ad00:beef:cafe:5569:2ae8:30b6:b25b foobar

Related samples
===============

You can use the :ref:`ot_cli_sample` sample for testing purposes.

Dependencies
************

This application uses the :ref:`ug_thread` protocol.

The application uses the following Zephyr libraries:

* :ref:`zephyr:logging_api`
* :ref:`zephyr:kernel_api`
