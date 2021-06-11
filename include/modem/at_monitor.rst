.. _at_monitor_readme:

AT monitor
##########

.. contents::
   :local:
   :depth: 2

The AT monitor library is used to define AT monitors, which  consist of a handler function that receives AT notifications, based on a filter, from the :ref:`nrfxlib:nrf_modem`.
The handler is executed from the system workqueue.

Purposes
========

The modem library :ref:`nrfxlib:nrf_modem_at` interface only supports one handler function to send AT notifications to, which is executed in an interrupt service routine (ISR).
It is likely that multiple parts of the application are interested in AT notifications, and that the processing of the notification is done outside of an ISR.

The AT monitor library facilitates the integration of the modem library in NCS applications in two ways:

#. Take care of copying notifications and rescheduling their processing in a thread
#. Dispatch notifications to multiple parts of the application

.. note::
   When the AT monitor library is enabled, the application should not call :c:func:`nrf_modem_at_notif_handler_set` as that would override the handler set by the AT monitor library.
   The application should instead define an AT monitor with a filter set to :c:macro:`ANY` to achieve the same behavior of registering an handler for AT notifications with the modem library.

Initialization
==============

The application can initialize the AT monitor library manually via the :c:func:`at_monitor_init` function or automatically, via the ``SYS_INIT`` macro, by enabling :option:`CONFIG_AT_MONITOR_SYS_INIT`.
Upon initialization, the AT monitor library will register itself as the reicever of AT notifications from the modem library (via :c:func:`nrf_modem_at_notif_handler_set`).

.. note::
   When the AT monitor library is initialized, it will override whatever handler had been previously set with the modem library to receive the AT notifications.

Usage
=====

The application can define an AT monitor to receive notifications via the :c:macro:`AT_MONITOR` macro.
An AT monitor has a name, a filter string, and a callback function. In addition, it may be paused or active (:c:macro:`PAUSED`, :c:macro:`ACTIVE` respecively).
When the AT monitor library receives an AT notification from the modem library, the notification is copied on the AT monitor library heap and is dispatched via the system workqueue to all monitors whose filter matches (even partially) the contents of the notification.
Multiple parts of the application can define their own AT monitor with the same filter as another AT monitor, and thus receive the same notifications, if desired.

The following code snippet shows how to register a handler that receives ``+CEREG`` notifications from the modem library.

.. code-block:: c

	/* AT monitor for +CEREG notifications */
	AT_MONITOR(network_registration, "+CEREG", cereg_mon);

	int cereg_mon(const char *notif)
	{
		printf("Received +CEREG notification: %s", notif);
	}

Pausing and resuming
********************

When defined, an AT monitor is in active state by default. An AT monitor may be paused and resumed with the :c:func:`at_monitor_pause` and :c:func:`at_monitor_resume` functions respectively.
If desired, an AT monitor can be defined to be in paused state at compile time by appending :c:macro:`PAUSED` to the monitor definition.

The following code snippet shows how to define an AT monitor for ``+CEREG`` notifications that is paused at boot, and later resumed.

.. code-block:: c

	/* AT monitor for +CEREG notifications, paused until manually activated */
	AT_MONITOR(network_registration, "+CEREG", cereg_mon, PAUSED);

	void foo(void)
	{
		/* let's resume the monitor */
		at_monitor_resume(network_registration);
	}

Wildcard filter
***************

It is possible to define an AT monitor that will receive all AT notifications, by passing :c:macro:`ANY` as the AT monitor filter string.

The following code snippet shows how to define an AT monitor that will receive all AT notifications.

.. code-block:: c

	/* AT monitor for all notifications */
	AT_MONITOR(catch_all, ANY, at_notif_handler);

	int at_notif_handler(const char *notif)
	{
		printf("Received a notification: %s", notif);
	}

Relation to the AT notification library
=======================================

The AT monitor and :ref:`at_notif_readme` libraries solve similar purposes but there are two important differences:

#. The AT notification library works with the :ref:`at_cmd_readme`, which uses the modem library AT socket. The AT monitor library uses the modem library AT API, which are not socket-based.
#. The AT notification library features runtime registration of notification handlers, whereas the AT monitor library features static registration.

The two libraries are separate; they do not interfere with each other in any way and can be used at the same time.
However, because the two libraries are separate and use separate APIs in the modem library:

#. The AT monitors will only receive notifications to which the application subscribed to using the AT APIs.
#. The :ref:`at_notif_readme` library registrars will only receive notifications to which the application subscribed to using the :ref:`at_cmd_readme`.

API documentation
=================

| Header file: :file:`include/modem/at_monitor.h`
| Source file: :file:`lib/at_monitor/at_monitor.c`

.. doxygengroup:: at_monitor
   :project: nrf
   :members:
