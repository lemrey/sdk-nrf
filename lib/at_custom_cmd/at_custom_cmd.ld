. = ALIGN(4);
_nrf_modem_at_pre_modem_filter_list_start = .;
KEEP(*(SORT_BY_NAME("._nrf_modem_at_pre_modem_filter.*")));
_nrf_modem_at_pre_modem_filter_list_end = .;

_nrf_modem_at_post_modem_filter_list_start = .;
KEEP(*(SORT_BY_NAME("._nrf_modem_at_post_modem_filter.*")));
_nrf_modem_at_post_modem_filter_list_end = .;
