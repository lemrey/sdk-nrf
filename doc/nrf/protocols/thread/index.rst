.. _ug_thread:

Thread
######

.. caution::

   The limited customer sampling version of the |NCS| is affected by the OpenThread KeyID Mode 2 Security Vulnerability.
   This vulnerability impacts all Thread devices using OpenThread and allows an attacker in physical proximity to compromise non-router-capable devices and the entire Thread network in the case of router-capable devices.
   The vulnerability allows an attacker in physical proximity to inject arbitrary IPv6 packets into the Thread network via IEEE 802.15.4 frame transmissions.
   Because the Thread Management Framework (TMF) protocol does not have any additional layer of security, the attacker could exploit this vulnerability to update the Thread Network Key and gain full access to the Thread network.
   There is no known exploitation of vulnerability.

   Due to this issue, the Thread certifications for OpenThread libraries in all |NCS| releases up to v2.4.0, which the limited customer sampling is based on, are deprecated.

.. thread_intro_start

Thread is a low-power mesh networking technology, designed specifically for home automation applications.
It is an IPv6-based standard that uses 6LoWPAN technology over the IEEE 802.15.4 protocol.
You can connect a Thread mesh network to the Internet with a Thread Border Router.

The |NCS| provides support for developing Thread applications based on the OpenThread stack.
The OpenThread stack is integrated into Zephyr.

.. thread_intro_end

You can use Thread in parallel with Bluetooth® Low Energy.
See :ref:`ug_multiprotocol_support` for more information.

See :ref:`openthread_samples` for the list of available Thread samples.

You can find more information about Thread on the `Thread Group`_ pages.


.. toctree::
   :maxdepth: 1
   :caption: Subpages:

   overview/index
   configuring
   prebuilt_libs
   tools
   certification
   device_types
   sed_ssed
