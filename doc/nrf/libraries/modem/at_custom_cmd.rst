.. _at_custom_cmd_readme:

AT custom command
#################

.. contents::
   :local:
   :depth: 2

AT custom command is a library which allows the application to implement custom AT commands in the application or override existing AT commands with custom implementations.

To add a custom AT command or overwrite an existing command, the AT command's filter and callback must be added using the :c:macro:`AT_FILTER` macro in :file:`at_custom_cmd.h`.
This adds the AT filter to a list that will be iterated before sending an AT command to the modem by calling :c:func:`nrf_modem_at_cmd`.
If the filter matches the start of the command, or the command in its entirety, the AT command is passed to the filter callback instead of the modem. When returning from the callback, the content of the provided buffer is treated as the modem response.

.. note::
When the AT custom command library is enabled, the application must not call :c:func:`nrf_modem_at_filter_set` as this will override the handler set by the library.
   Instead, the application must add the AT filters using the :c:macro:`AT_FILTER` macro.

Usage
=====

Adding a filter
***************

The application can define an AT filter to receive a callback using the :c:macro:`AT_FILTER` macro.
An AT filter has a name, a filter string, and a callback function.
Multiple parts of the application can define their own AT filters. However, there can be only one callback per filter match.
If multiple filters matches an AT command, the first match found will be used. Care must therefore be taken to keep the filters as precise as possible to avoid conflicts with other AT commands.

The following code snippet shows how to add a filter that triggers on ``+MYCOMMAND`` calls to :c:func:`nrf_modem_at_cmd`.

.. note::
   The filter will only trigger on calls to :c:func:`nrf_modem_at_cmd`. Calls to :c:func:`nrf_modem_at_printf`, :c:func:`nrf_modem_at_cmd_async` and :c:func:`nrf_modem_at_scanf` will be forwarded directly to the modem.

.. code-block:: c

   /* AT filter callback for +MYCOMMAND calls */
   AT_FILTER(my_command_filter, "AT+MYCOMMAND", my_command_callback);

	int my_command_callback(char *buf, size_t len, char *at_cmd);
	{
		printf("Received +MYCOMMAND call: %s", at_cmd);
		return at_custom_cmd_response_buffer_fill(buf, len "OK\r\n");
	}

AT command responses for filtered commands
******************************************

When returning from the callback, the content of the provided :c:var:`buf` buffer is treated as the modem response by the Modem library. Hence, the response format must match that of the modem:
* Successful responses end with "OK\r\n".
* If the response should be threated as an error, use "ERROR\r\n", "+CME ERROR: <errorcode>" or "+CMS ERROR: <errorcode>" dependent on the error.

To simplify filling the response buffer, the provided function :c:func:`at_custom_cmd_response_buffer_fill` can be used. This allows format arguments and ensures that the response do not overflow the response buffer.

The following code snippet shows how responses can be added to the ``+MYCOMMAND`` AT command.

.. code-block:: c

	/* AT filter callback for +MYCOMMAND calls */
	AT_FILTER(my_command_filter, "AT+MYCOMMAND", my_command_callback);

	int my_command_callback(char *buf, size_t len, char *at_cmd);
	{
		/* test */
		if(strncmp("AT+MYCOMMAND=?", at_cmd, strlen("AT+MYCOMMAND=?")) == 0) {
			return at_custom_cmd_response_buffer_fill(buf, len, "+MYCOMMAND: (%d, %d)\r\nOK\r\n", 0, 1);
		}
		/* set */
		if(strncmp("AT+MYCOMMAND=", at_cmd, strlen("AT+MYCOMMAND=")) == 0) {
			return at_custom_cmd_response_buffer_fill(buf, len, "OK\r\n");
		}
		/* read */
		if(strncmp("AT+MYCOMMAND?", at_cmd, strlen("AT+MYCOMMAND?")) == 0) {
			return at_custom_cmd_response_buffer_fill(buf, len, "+CME ERROR: %d\r\n", 1);
		}
	}


Pausing and resuming
********************

When defined, an AT filter is active by default.
An AT filter can be paused and resumed with the :c:func:`at_filter_pause` and :c:func:`at_filter_resume` functions, respectively.
If desired, an AT filter can be paused at declaration by appending :c:macro:`PAUSED` to the filter definition.

The following code snippet shows how to resume a filter that is paused by default:

.. code-block:: c

	/* AT filter callback for +MYCOMMAND calls */
	AT_FILTER(my_command_filter, "AT+MYCOMMAND", my_command_callback, PAUSED);

	int resume_my_command_filter(void)
	{
		/* resume the filter */
		at_filter_resume(my_command_filter);
	}

	int my_command_callback(char *buf, size_t len, char *at_cmd);
	{
		return at_custom_cmd_response_buffer_fill(buf, len "OK\r\n");
	}


API documentation
=================

| Header file: :file:`include/modem/at_custom_cmd.h`
| Source file: :file:`lib/at_custom_cmd/src/at_custom_cmd.c`

.. doxygengroup:: at_custom_cmd
   :project: nrf
   :members:
