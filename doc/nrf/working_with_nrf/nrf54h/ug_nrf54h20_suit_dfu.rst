.. _ug_nrf54h20_suit_dfu:

Device Firmware Update using SUIT
#################################

.. contents::
   :local:
   :depth: 2

The Software Updates for Internet of Things (SUIT) is a metadata format and procedure for performing device firmware updates (DFU).
SUIT was originally created by the `Internet Engineering Task Force <https://datatracker.ietf.org/wg/suit/about/>`__.
Nordic Semiconductor has created an implementation of the SUIT procedure that is the newly introduced DFU procedure for the nRF54H Series of System-on-Chip (SoC) and will be the only DFU procedure compatible with the nRF54H Series.

The SUIT DFU procedure features a SUIT envelope structure.
The SUIT envelope contains a manifest, authentication block, integrated dependencies, and integrated payloads.
You can read more about SUIT-specific concepts in :ref:`ug_suit_dfu_suit_concepts`.

The logic and instructions for how to carry out the DFU to your devices is described within the SUIT manifest (which is found in the SUIT envelope).
Originally, the envelope (including the manifest) is presented in a binary form but with Nordic Semiconductor's implementation, the envelope becomes converted into a .YAML file.

Manifests contain components, which is the main element that SUIT operates on.
Components contain information such as ...


.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   ug_nrf54h20_suit_intro
   ug_nrf54h20_suit_manifest_overview.rst
   ug_nrf54h20_suit_hierarchical_manifests.rst
   ug_nrf54h20_suit_create_manifest.rst
   ug_nrf54h20_suit_components.rst
