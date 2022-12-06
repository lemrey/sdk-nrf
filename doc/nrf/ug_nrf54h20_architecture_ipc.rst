.. _ug_nrf54h20_architecture_ipc:

Interprocessor Communication in nRF54H20
########################################

.. contents::
   :local:
   :depth: 2

Interprocessor Communication is required in asymmetric multiprocessing environments to share services' requests, responses, and data between processors.

The following sections describe the communication protocol used for IPC in |NCS| for the nRF54H20 SoC.
They also include an overview of the connection scheme between the cores available in the system.

Technical solution
==================

A single Interprocessor Communication instance used in the nRF54H20 SoC allows full-duplex communication between two cores.
It support only unicast messages.

To communicate with multiple peers, a core requires multiple IPC instances, at least as many as the number of peers.
It is possible to create multiple IPC instances towards a single peer to separate messages with different priority levels.
The number of IPC instances which might be created is limited by the available hardware resources (numbers of channels in the signaling peripherals).

The Interprocessor Communication can be described using the OSI model.

Physical layer
--------------

The physical layer used by IPC in the nRF54H20 SoC consists of shared RAM and signaling peripherals.

Shared RAM must be accessible (read and write) by both cores using this RAM region for communication.
See :ref:`General-purpose shared RAM (RAM0x)<ug_nrf54h20_architecture_memory_gp_shared_ram>` for a description of shared RAM and its placement in the memory map.

Signaling peripherals are the hardware peripherals which can be used by one core to trigger an IRQ in another core.
Each ARM Cortex-M33 core in nRF54H20 has an associated BELLBOARD signaling peripheral.
Each VPR core in nRF54H20 has an associated VEVIF signaling peripheral:

* When a core signals an IRQ to its ARM Cortex-M33 peer, it uses the BELLBOARD associated with the peer.
* When a core signals an IRQ to a VPR peer, it uses the VEVIF associated with the peer.
* When a core receives an IRQ signal, it uses the BELLBOARD or the VEVIF associated with it, regardless which peer originated the signal.

Each VEVIF and BELLBOARD has multiple channels.
The peer originating the signal is recognized with a channel number associated with the peer.
The channel numbers association is presented in the following table.

============================  ==========
Signal originator             Channel id
============================  ==========
SecDom                        0
SecDom: signal to a debugger  1
System Controller             6
Application                   12
Radio                         18
============================  ==========

Some of the channels in BELLBOARDs and VEVIFs are intepreted differently by other receivers.

The following tablex show signals received only by specified cores.

   Secure Domain Core

      ============================  ==========
      Signal originator             Channel id
      ============================  ==========
      System Controller's debugger  7
      Application's debugger        13
      Radio's debugger              19
      ============================  ==========

   Application Core

      This assignement applies only when PPR and FLPR owner is set to the Application.

      =================  ==========
      Signal originator  Channel id
      =================  ==========
      PPR                12
      FLPR               13
      =================  ==========

   Radio Core

      This assignemnt applies only when BBPROC owner is set to the Radio.

      =================  ==========
      Signal originator  Channel id
      =================  ==========
      BBPROC             18
      =================  ==========

Some of the channels in BELLBOARDs and VEVIFs are reserved for functions different from IPC.
See :ref:`Post-mortem debugging capabilities<ug_nrf54h20_architecture_post_mortem_debugging_capabilities>` for information on the assignment of IPC channels for other functions.

MAC, Network, Transport
-----------------------

The layers responsible for maintaining a stable full-duplex stream of data between two cores (MAC, Network, Transport) are implemented in an IPC transport library.
The default IPC transport library for nRF54H20 is *icmsg* (TODO: link to repo).
This lightweight library maintains the connection between a pair of cores for each IPC instance with minimal memory overhead.
Each IPC instance requires an instance of the icmsg library.
Each icmsg library instance requires the following:

   * One memory buffer for A->B messages
   * One memory buffer for B->A messages
   * A pair of signaling channels (one for A->B, one for B->A).

The icmsg library is designed to efficiently send short messages to a peer.
Its buffer allocation is not optimized for bulk transfer of long messages and it does not provide a zero-copy model.
To transfer bigger amount of data (like network packets) between two cores, icmsg can be used as control plane.
However, you must use as data plane memory allocation mechanisms optimized for bigger data and supporting flexible memory allocation and a zero-copy model (like Network Buffers (TODO: link to Zephyr's Network Buffers)).

.. note::
   The memory allocation mechanism for the data plane is a single-core memory allocation working on shared memory buffers.
   It does not have to involve another IPC transport.
   The ownership of the shared buffers is passed between the cores using the control plane, but only one of the cores is responsible for managing (allocating, resizing, freeing) the buffers.

Session
-------

The IPC session establishment in the nRF54H20 SoC is performed by the *ipc_service* library (TODO: link to Zephyr repo or docs).
This library provides an unified API for IPC transports and it integrates specific transports as its backends.
icmsg is integrated as one of the available ipc_service backends.
The library is responsible for creating IPC instances based on the system configuration described in the devicetree.

It is recommended to use the ipc_service API to perform inter-processor communication in the nRF54H20 SoC.

Presentation and Application layers
-----------------------------------

The presentation and application layers are connection specific.
Each connection has other requirements regarding the type and the nature of the exchanged messages.

Selected solutions are listed in the table below.

======================  =====================
Connection              Communication library
======================  =====================
System Controller       nrfs
Secure Domain           RPC (nrf_rpc)
Radio Bluetooth         HCI or nrf_rpc
Radio 802.15.4          Spinel
PPR with event_manager  event_proxy
======================  =====================

IPC connections scheme
======================

TODO: Describe overall IPC connections and create a diagram

Radio core
----------

The Radio Core is intended to expose radio communication services to the Application Core through IPC.
These services include:

   * Bluetooth (HCI or host API)
   * IEEE 802.15.4 radio driver API

These services are hidden behind Zephyr APIs available in the Application Core, like the Bluetooth host API or the IEEE 802.15.4 driver API.
All services can be enabled simultaneously using separated endpoints in a shared IPC instance.
You can implement other services running in the Radio Core and expose them to the Application Core using more endpoints from the shared IPC instance.

If TrustZone is enabled in the Application Core, Radio Core services are accessible from the Application's Non-Secure Processing Environment.
If TrustZone is disabled in the Application Core, Radio Core services are accessible from the only available processing environment, the Secure Processing Environment.
If TrustZone is enabled in the Radio Core, it exposes its services from the Non-Secure Processing Environment.
If TrustZone is disabled in the Radio Core, it exposes its services from the Secure Processing Environment.

TrustZone configuration for the Application and Radio core can differ.
It is valid to access services exposed from SPE in NSPE, or the other way around.

Secure Domain Core
------------------

The Secure Domain Core exposes security-related services to the other local cores in the system (Application, Radio).
The list of services includes:

   * Key management
   * Secure storage
   * Cryptography

     * Cryptographically Secure Pseudo Random Number Generator

   * Device Firmware Upgrade
   * Debugger access management
   * Reset notifications
   * Encrypted core dumps

If TrustZone is enabled in a local core, Secure Domain Core services are accessible from the local core's Secure Processing Environment.
If TrustZone is disabled in a local core, Secure Domain Core services are accessible from the only available processing environment, the Secure Processing Environment.
It is not possible to access Secure Domain Core services from Non-Secure Processing Environment directly.
NSPE must use local secure services to request SPE to use Secure Domain Core's services.

The Secure Domain Core handles the requests in its Non-Secure Processing Environment.

System Controller
-----------------

The System Controller exposes the following services to local cores (Application, Radio, Secure Domain) through IPC:

   * Clocks accuracy notification and calibration requests
   * MRAM latency management (related to power management)
   * CPU frequency management (using Dynamic Voltage and Frequency Scaling)
   * Die temperature service
   * L2CACHE performance
   * Power output service

If TrustZone is enabled in a local core, services are accessible from local core's Non-Secure Processing Environment.
If TrustZone is disabled in a local core, services are accessible from the only available processing environment, the Secure Processing Environment.

FLPR
----

The FLPR exposes IPC communication for its owner to manage its operations.
The communication details depends on the FLPR role in the system.

PPR
---

The PPR exposes IPC communication for its owner to manage its operations.
The communication details depends on the PPR role in the system.

In the SensorHub application, PPR uses an Event Manager Proxy through IPC to extend the Event Manager framework running in the Application Core.

Reset Handling
**************

Reset sources
=============

Global
------

Local
-----

Reset Handling Procedure
========================

Global
------

Local
-----

.. _ug_nrf54h20_architecture_post_mortem_debugging_capabilities:

Post-mortem debugging capabilities
==================================

=================  ==========
Signal originator  Channel id
=================  ==========
Application Crash  14
Radio Crash        20
=================  ==========
