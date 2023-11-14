.. _ug_nrf54h20_suit_hierarchical_manifests:

Hierarchical manifests
######################

The Software Updates for the Internet of Things (SUIT) procedure provides the option to employ a hierarchical manifest system.
This hierarchical manifest system is a specific type of manifest topology, and functions as a delivery chain, allowing for versatile customization of the Device Firmware Update (DFU) process across various specific use cases.
The nRF54H20 System on Chip (SoC) contains multiple cores which may require multiple manifests, hence the need for a hierarchal manifest topology.

The hierarchical manifest system, which entails multiple manifests, allows for properties to be assigned to specific manifests. Some example use cases where hierarchical manifests are beneficial are:

* When different components (that are declared within the manifest) need to have different access rights to certain memory locations on the device.

* When your final application only needs one manifest that requires downgrade prevention instructions, while several other Domains (hence, manifests) do not need these instructions.

To perform a DFU procedure on the nRF54H20 SoC, you must use either the manifest templates provided by Nordic Semiconductor or the manifest generator to create your own.
Additionally, there may be some use cases where you will have to modify the manifests or the topology for your specific use case.

Default manifest topology
*************************

The design of the hierarchy of the manifests is considered the manifest topology.
The hierarchical manifest topology can be described as having a root manifest, with several dependency manifests within it.
A dependency manifest contains a specific updatable image or payload for a specific core/Domain of the device.
Therefore, there can be multiple dependency manifests required. The root manifest bundles all the dependency manifests into a single executable image for the DFU.

.. figure:: images/nrf54h20_suit_default_manifest_topology.png
   :alt: Default hierarchical manifest topology for the nRF54H20 SoC

   Default hierarchical manifest topology for the nRF54H20 SoC

By default, using SUIT’s hierarchical manifest involves using your own generated manifests (OEM controlled) in combination with manifests provided by Nordic Semiconductor - particularly for the Secure Domain (SecDom) and System Controller.
An example of how this would be implemented includes:

* OEM controlled manifests:
   * A root manifest that coordinates update and invocation processes on its dependency manifests.
     It does not belong to any Domain or controls any local Domain resources.

   * A dependency manifest for each of the Domains, such as the Application and Radio Domains.

* Nordic Semiconductor controlled manifests - for the SecDom, including the System Controller

Alternatively, you can use the manifest generator to create your own manifests. Read the “How to use the manifest generator” user guide for instructions.
Additionally, if you would like to have multiple dependency manifests for specific Domain you will need to modify the manifest topology.

If you need to modify the default manifest topology, see the “How to customize the SUIT DFU” user guide for instructions and example use cases.
