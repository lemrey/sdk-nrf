.. _ug_nrf54h20_gs:

Getting started with nRF54H20 PDK
#################################

.. contents::
   :local:
   :depth: 2

This section will get you started with your nRF54H20 PDK.
You will configure, build and program a basic sample to the development kit.

If you have already set up your nRF54H20 DK and want to learn more, see :ref:`ug_nrf54h20_architecture` for an in-depth description of the architecture of the nRF54H20.
*

.. _ug_nrf54h20_gs_requirements:

Requirements
************

This userguide requires the following:

* nRF54H20 PDK
* The installation of the software required in a regular |NCS| installation.
  See the :ref:`introductory documentation <getting_started>` for more information.

.. _ug_nrf54h20_gs_requirements_software:

Installing the additional HCS software requirements
***************************************************

Follow these steps:

1. Clone the *sdk-nrf-next* repository, as follows:

   a. Create a folder named :file:`ncs-next`.
   #. On the command line, go to the :file:`ncs-next` folder and initialize west with the revision of the nRF Connect SDK from the initial limited sampling, as follows::

         west init -m https://github.com/nrfconnect/sdk-nrf-next --mr nRFConnectSDK_limsampling_revision
   #. Run ``west update`` and then ``west zephyr-export``

#. Install the following Python dependencies from the :file:`ncs` folder:

   .. tabs::

      .. group-tab:: Windows

         Enter the following command in a command-line window in the :file:`ncs` folder:

         .. parsed-literal::
            :class: highlight

            pip3 install -r zephyr/scripts/requirements.txt
            pip3 install -r nrf/scripts/requirements.txt
            pip3 install -r bootloader/mcuboot/scripts/requirements.txt

      .. group-tab:: Linux

         Enter the following command in a terminal window in the :file:`ncs` folder:

         .. parsed-literal::
            :class: highlight

            pip3 install --user -r zephyr/scripts/requirements.txt
            pip3 install --user -r nrf/scripts/requirements.txt
            pip3 install --user -r bootloader/mcuboot/scripts/requirements.txt

      .. group-tab:: macOS

         Enter the following command in a terminal window in the :file:`ncs` folder:

         .. parsed-literal::
            :class: highlight

            pip3 install -r zephyr/scripts/requirements.txt
            pip3 install -r nrf/scripts/requirements.txt
            pip3 install -r bootloader/mcuboot/scripts/requirements.txt

#. Install nRF regtool::

      python3 -m pip install --user modules/lib/nrf-regtool

.. _ug_nrf54h20_gs_sample:

Getting started with the "Multicore: Hello World" sample
********************************************************

The :ref:`multicore_hello_world` sample builds a multicore sample running on both the application core (``cpuapp``) and the ultra-low-power core (PPR, ``cpuppr``).
It uses the following build targets:

* ``nrf54h20dk_nrf54h20_app@soc1``
* ``nrf54h20dk_nrf54h20_ppr@soc1``

To build and flash the :ref:`multicore_hello_world` sample on the nRF54H20-PDK, do the following:

1. Navigate to the :file:`samples/multicore/hello_world` folder containing the sample.
#. Build the sample running the following command::

      west build -d build/nrf54/hello_world -b nrf54dk_nrf5420soc1_cpuapp@soc1 nrf/samples/multicore/hello_world/

#. Flash the sample using the stardard |NCS| flash command::

      west flash

The sample will be automatically built and programmed on both the application and the ultra-low-power core of the nRF54H20.

For using UART, you also have to apply the following overlay::

   west build -d build/nrf54/hello_world -b nrf54dk_nrf5420soc1_cpuapp@soc1 nrf/samples/multicore/hello_world/-- \
   -DDTC_OVERLAY_FILE="serial_cpuapp.overlay;" \
   -DCONFIG_SERIAL=y \
   -DCONFIG_CONSOLE=y;
