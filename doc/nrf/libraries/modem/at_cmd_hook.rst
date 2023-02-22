.. _at_cmd_hook_readme:

AT command hook library
#######################

.. contents::
   :local:
   :depth: 2

The AT command hook library allows any part of the application to register hooks to the AT API of the Modem library, to get a callback when matching AT commands are sent.

Configuration
*************

You can enable the AT command hook library by setting the :kconfig:option:`CONFIG_AT_CMD_HOOK` Kconfig option.

Usage
*****

To add an AT command hook, you must add the AT command filter and callback with the :c:macro:`AT_CMD_HOOK` macro in the :file:`at_cmd_hook.h` file.
If the filter matches the first symbols of the AT command that is sent to the :c:func:`nrf_modem_at_cmd`, :c:func:`nrf_modem_at_printf` or :c:func:`nrf_modem_at_scanf` functions, the application is notified through the pre and post modem callbacks.

.. note::
   The AT command hooks does not trigger on calls to the :c:func:`nrf_modem_at_cmd_async` function.

.. note::
   When the AT command hook library is enabled, the application must not call the :c:func:`nrf_modem_at_cmd_hooks_set` function because it overrides the handler set by the library.
   Instead, the application must add the AT hook using the :c:macro:`AT_CMD_HOOK` macro.



The :c:type:`nrf_modem_at_cmd_hook_post_callback_t` require the AT command to be stored in the Modem library heap in addition to the shared memory TX region.
To reduce the heap usage, the :kconfig:option:`CONFIG_AT_HOOK_POST_CB_LEN_MAX` Kconfig option is used to decide the maximum AT command string that is given in the post callback.
If the AT command string is longer than :kconfig:option:`CONFIG_AT_HOOK_POST_CB_LEN_MAX`, it will be truncated in the post callback.

Adding an AT command hook
=========================

The application can define an AT command hook to receive a callback using the :c:macro:`AT_CMD_HOOK` macro.
An AT command hook has a name, a filter string, two callback functions, and a state.
Multiple parts of the application can define their own hooks. If there are multiple filters matching the AT command, all matching callbacks will be triggered.

The following code snippet shows how to add a hook that triggers on ``+CFUN`` calls:

.. code-block:: c

	/* AT hook callbacks for +CFUN calls */
	int my_command_pre_callback(const char *cmd);
	{
		printf("Pre callback for %s", at_cmd);
	}

	int my_command_post_callback(const char *cmd, int err);
	{
		printf("Post callback for %s with result %d", at_cmd, err);
	}

	AT_CMD_HOOK(my_command_hook, "AT+CFUN", my_command_pre_callback, my_command_post_callback);

Pausing and resuming
====================

An AT command hook is active by default.
An AT command hook can be paused and resumed with the :c:func:`at_cmd_hook_pause` and :c:func:`at_cmd_hook_resume` functions, respectively.
You can pause a hook at declaration by appending :c:macro:`AT_CMD_HOOK_PAUSED` to the filter definition.

The following code snippet shows how to resume a pre modem hook that is paused by default:

.. code-block:: c

	/* AT hook pre callback for +CFUN calls */
	int my_command_pre_callback(const char *cmd);
	{
		printf("Pre callback for %s", at_cmd);
	}

	int my_command_post_callback(const char *cmd, int err);
	{
		printf("Post callback for %s with result %d", at_cmd, err);
	}

	AT_CMD_HOOK(my_command_pre_hook, "AT+CFUN", my_command_pre_callback, NULL, AT_CMD_HOOK_PAUSED);
	AT_CMD_HOOK(my_command_post_hook, "AT+CFUN", NULL, my_command_post_callback);

	int resume_my_command_pre_filter(void)
	{
		/* resume the filter */
		at_cmd_hook_resume(&my_command_pre_hook);
	}

API documentation
*****************

| Header file: :file:`include/modem/at_cmd_hook.h`
| Source file: :file:`lib/at_cmd_custom/src/at_cmd_hook.c`

.. doxygengroup:: at_cmd_hook
   :project: nrf
   :members:
