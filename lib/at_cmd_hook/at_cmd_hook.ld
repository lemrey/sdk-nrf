. = ALIGN(4);
_nrf_modem_at_cmd_hook_list_start = .;
KEEP(*(SORT_BY_NAME("._nrf_modem_at_cmd_hook.*")));
_nrf_modem_at_cmd_hook_list_end = .;
