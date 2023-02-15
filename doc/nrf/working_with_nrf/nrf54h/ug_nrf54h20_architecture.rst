.. _ug_nrf54h20_architecture:

Software Architecture for nRF54H20
##################################

.. contents::
   :local:
   :depth: 2

The nRF54H20 is a multicore System-on-Chip (SoC) that uses an asymmetric multiprocessing (AMP) configuration.
Each core is tasked with specific responsibilities and optimized for different workloads.

The software architecture documentation briefly describes the responsibilities of the cores and their interprocessor interactions.

The following pages briefly describes the responsibilities of the cores, their interprocessor interactions, the memory mapping, and the boot sequence in nRF54H20.

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   ug_nrf54h20_architecture_cpu
   ug_nrf54h20_architecture_memory
   ug_nrf54h20_architecture_ipc
   ug_nrf54h20_architecture_boot
