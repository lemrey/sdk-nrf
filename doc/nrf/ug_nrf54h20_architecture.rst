.. _ug_nrf54h20_architecture:

Software Architecture for nRF54H20
##################################

The nRF54H20 is a multi-core System-on-Chip (SoC) that uses an asymmetric multiprocessing (AMP) configuration.
Each core is tasked with specific responsibilities and they cooperate to run the whole system use-cases efficiently.

The software architecture documentation briefly describes the responsibilities of the cores and their inter-processor interactions.

Cores
*****

TODO: domains diagram (public and internal versions)

The following are the cores available in the nRF54H20, divided per domain:

* Application Domain:

   Application Core
      The Application Core is the core with the highest processing power in the system.
      Its purpose is to run the main application.

* Global Domain:

   System Controller
      The System Controller (SYSCTRL) is a run-time manager of the SoC configuration.
      Its main responsibility is power and clock management to meet the clock, frequency, latency, and power requirements of the programs running in other cores.

   Fast Lightweight Processor
      The Fast Lightweight Processor (FLPR) is a lightweight processor designed to offload from the application core simple tasks requiring low latencies or high processing power, like the hardware peripheral emulation.

   Peripheral Processor
      The Peripheral Processor (PPR) is a lightweight processor designed to perform simple ultra-low-power operations.
      It manages data exchange with external devices using serial data transmission (SPI, UART, TWI) while the Application Core and the FLPR are in power-saving states.

* Radio Domain:

   Radio Core
      The Radio Core is intended to run the radio protocol stacks (like BLE, IEEE 802.15.4, ESB, Gazell, or other proprietary ones) using the RADIO peripheral with multiprotocol support.
      It is possible to use the remaining processing power of this core also for tasks other than the ones required by the radio protocol stacks.

   Baseband Processor
      The Baseband Processor (BBPROC) is designed to run Radio Frequency (RF) algorithms in software, to support RF features not implemented in RADIO peripheral.

* Secure Domain:

   Secure Domain Core
      The Secure Domain (SECDOM) provides security guarantees for the system with features like the following:

      * Root of Trust
      * System access protection configuration
      * Secure storage
      * Cryptographic operations
      * Device firmware upgrade
      * Crash handling
      * Tamper detection

Memory Layout
*************

The nRF54H20 contains the following:

* Static RAM (SRAM, RAM)
* Magnetic RAM (MRAM) with non volatile memory properties

RAM
===

There are multiple RAM banks in the system.
Each local domain (like Application or Radio) contains its own RAM.
There is also a large part of the RAM in the global domain to be shared between the cores in the system.

TODO: diagram

Local RAM
---------

Local RAM is present in each of local domains

Application domain
^^^^^^^^^^^^^^^^^^

.. image:: images/nrf54h20_memory_map_app.png
   :width: 300 px
   :align: left

The Application Domain contains 32 KB of local RAM.
Accessing this memory from the Application Core has minimal latency, but accessing it from any other core adds significant latency.
Because of this property, the local RAM in the application domain should be used mainly to store data frequently accessed by the Application Core, or to store timing-critical parts of the code executed by Application Core.

Address range
   0x22000000 - 0x22008000

Size
   32 KB

Access control
   Application domain local RAM is accessible by the Application Core.
   Any core (like FLPR or PPR) or peripheral configured to be owned by Application Core (like UARTE or SAADC) can access this memory as well.
   Any core with access to this memory can execute code from it.

   If the TrustZone feature is enabled for Application Core, this memory can be partitioned in one secure and one non-secure region.
   The secure region is accessible only by code executed with the secure attribute, while the non-secure region is accessible by any code.

   .. note::
      Code executed by VPRs (like FLPR or PPR) has its secure attribute matching the given VPR security configuration in the SPU.
      Local RAM cannot include a Non-Secure Callable section.

Radio domain
^^^^^^^^^^^^

.. image:: images/nrf54h20_memory_map_radio.png
   :width: 300 px
   :align: left

The Radio domain contains 96 KB of local RAM.
Any access to this memory has minimal latency if originated either from Radio Core or from peripherals with EasyDMA located in radio domain.
Any access from any other core has a significant latency.
Because of this property, local RAM in the radio domain should be used mainly to store data frequently accessed by the Radio Core or the radio protocol frames to be accessed by CCM or RADIO peripherals, or to store timing critical parts of the code executed by the Radio Core.

Address range
   0x23000000 - 0x23018000

Size
   96 KB

Access control
   The Radio domain local RAM is accessible by the Radio Core.
   Any core (like FLPR or PPR) or peripheral configured to be owned by the Radio Core (like UARTE or SAADC) can access this memory as well.
   Any core with access to this memory can execute code from it.

   If the TrustZone feature is enabled for the Radio Core, this memory can be partitioned in one secure and one non-secure region.
   The secure region is accessible only by code executed with the secure attribute, while the non-secure region is accessible by any code.

   .. note::
      Code executed by VPRs (like FLPR or PPR) has its secure attribute matching the given VPR security configuration in the SPU
      Local RAM cannot include a Non-Secure Callable section.

BBPROC memory
"""""""""""""

TODO: diagram

The Lower 32 KB of local RAM in the Radio Domain (0x23010000 - 0x23018000) is tightly coupled with BBPROC.
Any access to this memory has minimal latency if originated from BBPROC.
Any access originated from the Radio Core or from peripherals with EasyDMA located in radio domain have a little greater latency while accessing BBPROC memory.
Access from other domains is possible, but with significant latency.

BBPROC memory is the only memory from which BBPROC can fetch its instructions.
Because of this property, this memory block is mostly intended to store BBPROC code and data.
When BBPROC is unused in a system, this memory can be used as additional local RAM in the Radio Domain.

Address range
   0x23010000 - 0x23018000

Size
   32 KB

Access control
   The access to the BBPROC memory is configured as the access to the local RAM in the Radio Domain.

Secure domain
^^^^^^^^^^^^^

.. image:: images/nrf54h20_memory_map_secure.png
   :width: 300 px
   :align: left

The Secure Domain contains 64 KB of local RAM.
Any access to this memory from the Secure Domain core has minimal latency, but accesses from any other core have significant latency.
Because of this property, the local RAM in the Secure Domain should be used mainly to store data frequently accessed by its local core, or to store timing critical parts of the code executed by it.

Address range
   0x21000000 - 0x22010000

Size
   64 KB

Access control
   The Secure-Domain local RAM is accessible by its local core.
   Any core (like FLPR or PPR) or peripheral configured to be owned by the Secure Domain (like UARTE or SAADC) can access this memory as well.
   Any core with access to this memory can execute code from it.

   If the TrustZone feature is enabled for the Secure Domain, this memory can be partitioned in one secure and one non-secure region (both regions owned by Secure Domain).
   The secure region is accessible only by code executed with the Secure attribute, while the Non-secure region is accessible by any code.
   The Local RAM cannot include a Non-Secure Callable section.

Global RAM
----------

The Global Domain RAM (or Global RAM, GRAM) is distributed in multiple instances across the system.
Each of the instances has other properties and other purposes.

TODO: access control

.. _ug_nrf54h20_architecture_gp_shared_ram:

General-purpose shared RAM (RAM0x)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. image:: images/nrf54h20_memory_map_ram0x.png
   :width: 300 px
   :align: left

The biggest part of the RAM memory in the system is located in the Global Domain as general-purpose shared RAM.
Access to this memory is relatively fast from all the local domains (like the Application or the Radio ones).
Access to this memory from DMA used by USB has minimal latency.

This memory is intended to store the majority of the data used by local cores (and does not fit in local domains' RAM) including shared memory used for Inter-Processor Communication (IPC) between local cores.
Buffers for USB data must be stored in this memory part, in the region owned by the core owning USB (usually the Application Core in typical applications).

Address range
   0x2F000000 - 0x2F0E0000

Size
   896 KB

Access control
   The general-purpose shared RAM is split into multiple partitions.
   Each of the local cores has two partitions assigned: one configured as Secure, the other one as Non-Secure.
   The partitions are configured in the given core's UICR.

   If TrustZone is enabled for a core, the Secure partition is used to store the data of the Secure Processing Environment, while the Non-Secure partition stores the data of the Non-Secure Processing Environment and the shared memory used by the Inter-Processor Communication towards other local domains.
   If TrustZone is disabled for a core, the Secure partition assigned to this core is used to store program data, while the Non-Secure partition contains the shared memory used by IPC.

   Secure partitions are grouped at the beginning of the general-purpose shared RAM block, while Non-Secure partitions are grouped at the end.
   Non-Secure partitions are overlapping to define shared-memory IPC regions.

   A partition assigned to a core is accessible from this core, other cores owned by this core, or DMAs used by peripherals used by this core.
   Access from other cores or peripherals is prevented.
   A partition configured as Secure is accessible only from the Secure Processing Environment in the core owning the given partition.
   A partition configured as Non-Secure is accessible from both the Secure and Non-Secure Processing Environments running in the core owning the given partition.


   .. note::
      If TrustZone is disabled for a given core, the only available Processing Environment is Secure.

TODO: secure domain usage of RAM0x, full partitioning, table with MPC OVERRIDEs

SYSCTRL memory (RAM20)
^^^^^^^^^^^^^^^^^^^^^^

.. image:: images/nrf54h20_memory_map_ram20.png
   :width: 300 px
   :align: left

The SYSCTRL memory is a part of the global RAM tightly coupled with the System Controller.
Access to this memory block from the System Controller has minimal latency and can be performed without powering up any other parts of the system.
Access to this memory from the local domains has higher latency than access to the general-purpose shared RAM.

This memory is statically partitioned.
The layout is not to be adjusted for specific products.

This memory is intended to store the code executed in the System Controller, the System Controller's data, and the shared memory used for Inter-Processor Communication between the System Controller and other cores.
Because of the static allocation property, this memory stores also the shared memory used for communication between debugger probes connected to cores in the system and the Secure Domain Core.

Address range
   0x2F880000 - 0x2F890000

Size
   64 KB

Access control
   The SYSCTRL memory is split into multiple partitions.
   The System Controller has access to all of them (System Controller's code and data, and shared memory regions).
   The shared memory regions are also accessible by the cores using particular region for communication with the System Controller and the debugger.
   The shared memory regions are configured in the given core's UICR.
   Cores do not have access to other parts of the SYSCTRL memory.

   If TrustZone is enabled for a core, the shared memory region is accessible from the Non-Secure Processing Environment.
   If TrustZone is disabled for a core, the shared memory region is accessible from the Secure Processing Environment.

Fast global RAM (RAM21)
^^^^^^^^^^^^^^^^^^^^^^^

.. image:: images/nrf54h20_memory_map_ram21.png
   :width: 300 px
   :align: left

The Fast global RAM is a part of the global RAM tightly coupled with the Fast Lightweight Processor.
Access to this memory block from the FLPR and fast peripherals' DMA (I3C, CAN, PWM120, UARTE120, SPIS120, SPIM120, SPIM121) has minimal latency and can be performed without powering up any other parts of the system.
Access to this memory from the local domains has higher latency than access to the general-purpose shared RAM.

This memory is intended to store the code executed in the FLPR, the FLPR's data, the shared memory used for Inter-Processor Communication between the FLPR and the core managing the FLPR, and DMA buffers for the fast peripherals.

Address range
   0x2F890000 - 0x2F898000

Size
   32 KB

Access control
   The FLPR and its owner have access to all partitions assigned to FLPR and its Inter-Processor Communication.
   Each of the memory partition assigned for DMA of the fast peripherals is accessible from the core owning given set of the peripherals.
   The FLPR and the fast peripherals are by default owned by the Application Core.
   This ownership and matching memory access rights can be customized in UICRs.

   The security attribute of memory partitions must follow FLPR and DMA engines security settings.

Slow global RAM (RAM3x)
^^^^^^^^^^^^^^^^^^^^^^^

.. image:: images/nrf54h20_memory_map_ram3x.png
   :width: 300 px
   :align: left

The Slow global RAM is a part of the global RAM tightly coupled with the Peripheral Processor.
Access to this memory block from the PPR and slow periperals' DMA has minimal latency and can be performed without powering up any other parts of the system.
Access to this memory from the local domains has higher latency than access to the general-purpose shared RAM.

This memory is intended to store the code executed in the PPR, the PPR's data, the shared memory used for Inter-Processor Communication between the PPR and the core managing the PPR, and DMA buffers for the slow peripherals.

Address range
   0x2FC00000 - 0x2FC08000

Size
   32 KB

Access control
  The PPR and its owner have access to all the partitions assigned to the PPR and its Inter-Processor Communication.
  Each of the memory partition assigned for DMA of the slow peripherals is accessible from the core owning the given set of peripherals.
  The PPR and the slow peripherals are by default owned by the Application Core.
  The ownership and matching memory access rights can be customized in UICRs.

  The security attribute of memory partitions must follow PPR and CMA engines security settings.

MRAM (non volatile memory)
==========================

The MRAM is divided in the following parts:

* MRAM_10
* MRAM_11

MRAM_10
-------

.. image:: images/nrf54h20_memory_map_mram10.png
   :width: 300 px
   :align: left

The MRAM_10 is a part of the non-volatile memory intended to keep firmware images to execute.
Access to this memory has minimal latency to avoid CPU stalls on instruction fetches.
This part of the memory is not writable while the main application is running (it is writable only during the Firmware Upgrade procedure) to avoid any latency caused by write operations.
Apart from executable code images, this part of the memory stores the Secure Information Configuration Registers (SICR) used by the programs running in the Secure Domain Core.
If code and data for the Application Core do not fit in MRAM_10, it can be partially or fully placed in MRAM_11.

Address range
   0x0E000000 - 0x0E100000

Size
   1024 KB

Access control
   The Application Core and the Radio Core have read and execute access to memory regions assigned to them.
   If Trustzone is disabled for any of these cores, then the assigned memory region is a single block containing secure code and data.
   If Trustzone is enabled for any of these cores, then the assigned memory region is split in three blocks:

   * Secure code and data
   * Non-secure code and data
   * Non-secure callable (NSC)

   The code executed in the Secure Processing Environment of a core has access to all three blocks assigned to the core.
   The code executed in the Non-Secure Processing Environment has access only to the Non-secure code and data block, and can call function veneers located in the NSC block.

   The System Controller's code and data region is accessible only by the Secure Domain Core.
   The content of this region is copied by the Secure Domain Core to RAM_20 before the System Controller is started.
   The System Controller accesses its code and data from the copy in RAM_20.

   Secure Domain has access to all parts of the MRAM_10.
   Other cores can access only the parts assigned to them, according to the security rules described above.

MRAM_11
-------

.. image:: images/nrf54h20_memory_map_mram11.png
   :width: 300 px
   :align: left

The MRAM_11 is a part of the non-volatile memory intended to keep non-volatile writable data.
Writing to MRAM_11 can increase access latency for other cores reading from MRAM_11.
When a core is reading or executing code from MRAM_11, the impact of the additional latency must be taken in consideration.
Each of the local cores (Application, Radio, Secure Domain) have an allocated partition in MRAM_11 to store their non-volatile data.
Each of the cores has full control on the data layout and management in the assigned MRAM partition.
There is also a Device Firmware Upgrade partition used to store firmware images used during the upgrade procedure.
If code and data for the Application Core do not fit in MRAM_10, it can be partially or fully placed in MRAM_11.

Address range
   0x0E100000 - 0x0E200000

Size
   1024 KB

Access control
   The Application Core and the Radio Core have read and write access to their assigned non-volatile data regions.
   The non-volatile data region assigned to the core having trustzone disabled is marked as Secure, while the non-volatile data region assigned to the core having trustzone enabled is marked as Non-Secure.

   If code or data for the Application Core is placed in MRAM_11, the Application Core has *read and execute* access to this partition.
   This access can be configured as follows:

   * Default configuration - all the application code and data is placed in MRAM_10.
     It is configured with a single MPC configuration entry contained entirely in MRAM_10.
   * All the app code and data is placed in MRAM_11.
     It is configured with a single MPC configuration entry contained entirely in MRAM_11.
   * The app code and data is partially in MRAM_10, partially in MRAM_11.
     It is configured with a single MPC configuration entry covering partially MRAM_10 and partially MRAM_11.
     Because of the continuous memory address range it is possible to use a single memory region to describe such data block.

   The Secure Domain has access to all the parts of MRAM_11.
   The Application Core has read and write access to the DFU partition.
   The security configuration of this partition follows the Trustzone configuration of the Application Core (Secure if Trustzone is disabled, or Non-Secure if Trustzone is enabled).

MPC configuration
=================

The Secure Domain configures MPCs before it boots other domains.
MPC configuration provides rights for other cores to access memory regions assigned to them.
If a core tries to access a memory address not assigned to it, the transaction fails.

The Secure Domain can access all the memory regions in the system and does not require explicit access rights in MPC.

The Secure Domain configures OVERRIDEs in the MPC assigned to ``AXI_0`` to provide the access rights needed:

===========  =====  ===========  ==========  ======================================================
OVERRIDE Id  Owner  Permissions  SECUREMASK  Regions
===========  =====  ===========  ==========  ======================================================
1            App    RW           1           App's NV storage; DFU storage bank
2            Radio  RW           1           Radio's NV storage
3            All    R            any         UICRs, FICR
4            App    RW           1           Radio <-> App IPC; App's Non-Secure data
5            App    RWS          1 *         SecDom <-> App IPC; App's Secure data
6            App    RX           1           App's Non-Secure code
7            App    RXS          0           App's S-NSC; App's Secure code
8            Radio  RW           1           Radio <-> App IPC; Radio's Non-Secure data
9            Radio  RWS          1 *         SecDom <-> Radio IPC; Radio's Secure data
10           Radio  RX           1           Radio's Non-Secure code
11           Radio  RXS          0           Radio's S-NSC; Radio's Secure code
===========  =====  ===========  ==========  ======================================================

.. note::
   SECUREMASK can be set to ``0`` for any secure memory partition.
   However, beause of the hardware configuration, OVERRIDEs marked with * have SECUREMASK set to ``1``.
   Such configuration implies that addresses of this entry should have 28th bit cleared.
   It does not expose any security thread because permissions filtering denies access from non-secure masters.

The Secure Domain configures OVERRIDEs in the MPC assigned to ``AXI_1`` to provide the access rights needed:

===========  =======  ===========  ==========  ================================================================================================================
OVERRIDE Id  Owner    Permissions  SECUREMASK  Regions
===========  =======  ===========  ==========  ================================================================================================================
5            SysCtrl  RW           1           Radio's non-volatile storage; App's non-volatile storage; DFU storage bank; Secure Domain's non-volatile storage
===========  =======  ===========  ==========  ================================================================================================================

   .. note::
      During the installation step of the Device Firmware Update procedure, write access is enabled for more MRAM partitions.
      During this step the only active core is the Secure Domain Core.

The Secure Domain configures OVERRIDEs in the MPC assigned to ``AXI_2`` to provide the access rights needed:

===========  =====  ===========  ==========  ==============================================================================
OVERRIDE Id  Owner  Permissions  SECUREMASK  Regions
===========  =====  ===========  ==========  ==============================================================================
1            App    RW           1           Application mngMbox; SysCtrl <-> App IPC
2            App    RWX          1           FLPR code; FLPR data; FLPR <-> App IPC; DMA buffers for App's fast peripherals
3            Radio  RW           1           Radio mngMbox; SysCtrl <-> Radio IPC
4            Radio  RW           1           DMA buffers for Radio's fast peripherals (if any)
===========  =====  ===========  ==========  ==============================================================================

The Secure Domain configures OVERRIDEs in the MPC assigned to ``AXI_3`` to provide the access rights needed:

===========  =====  ===========  ==========  ===========================================================================
OVERRIDE Id  Owner  Permissions  SECUREMASK  Regions
===========  =====  ===========  ==========  ===========================================================================
1            App    RWX          1           PPR code; PPR data; PPR <-> App IPC; DMA buffers for App's fast peripherals
3            Radio  RW           1           DMA buffers for Radio's fast peripherals (if any)
===========  =====  ===========  ==========  ===========================================================================

.. TODO: Diagrams showing memory view from App's SPE, App's NSPE, maybe for other cores as well?

SAU configuration
=================

Each one of the Cortex-M33 CPUs in the system with the TrustZone feature enabled (specifically, the Application, Radio, and Secure Domain Cores) associates a Security Attribution Unit (SAU) peripheral.
The Secure Domain configures the SAUs for itself during its initialization before it switches to the Non-Secure Processing Environment (NSPE).
The Secure Domain configures the SAUs for other cores before it boots them (TODO: verify if that's correct or actually each SPE configures its SAU).

SAU configuration provides the rights for the Non-Secure Processing Environment to access resources allocated for it.
If the NSPE tries to access a memory address not allocated to it, the transaction fails.

=============  ==================  ===================================  ====================================================================================================================================================================================
SAU region Id  Security attribute  Region                               Comments
=============  ==================  ===================================  ====================================================================================================================================================================================
0              NS                  0x0000_0000 - 0x1000_0000            The entire non-secure address space of the MRAM. Proper partitioning is done in MPC.
1              NS                  Core's S-NSC in MRAM                 S-NSC veneers in the secure address space of MRAM.
2              NS                  0x2000_0000 - 0x3000_0000            The entire non-secure address space of the RAM. Proper partitioning is done in MPC.
3              NS                  Core's S-NSC in RAM - 0x1_0000_0000  S-NSC veneers in the secure address space of the global MRAM, all peripherals, external memory, and CPU registers. If no S-NSC is in the RAM, the start address must be 0x4000_0000.
=============  ==================  ===================================  ====================================================================================================================================================================================

Inter-Processor Communication
*****************************

Inter-Processor Communication is required in Asymmetric Multiprocessing environments to share services' requests, responses, and data between processors.

The following sections describe the communication protocol used for IPC in |NCS| for the nRF54H20 SoC.
They also include an overview of the connection scheme between the cores available in the system.

Technical solution
==================

A single Inter-Processor Communication instance used in the nRF54H20 SoC allows full-duplex communication between two cores.
It support only unicast messages.

To communicate with multiple peers, a core requires multiple IPC instances, at least as many as the number of peers.
It is possible to create multiple IPC instances towards a single peer to separate messages with different priority levels.
The number of IPC instances which might be created is limited by the available hardware resources (numbers of channels in the signaling peripherals).

The Inter-Processor Communication can be described using the OSI model.

Physical layer
--------------

The physical layer used by IPC in the nRF54H20 SoC consists of shared RAM and signaling peripherals.

Shared RAM must be accessible (read and write) by both cores using this RAM region for communication.
See :ref:`General-purpose shared RAM (RAM0x)<ug_nrf54h20_architecture_gp_shared_ram>` for a description of shared RAM and its placement in the memory map.

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
