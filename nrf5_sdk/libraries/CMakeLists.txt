zephyr_library()
zephyr_library_sources(app_error.c)
zephyr_library_sources(app_error_weak.c)
zephyr_library_sources(app_util_platform.c)

zephyr_library_sources(app_scheduler.c)

zephyr_library_sources(nrf_strerror.c)
zephyr_library_sources(app_timer.c)
zephyr_library_sources_ifdef(CONFIG_APP_BUTTON app_button.c)


zephyr_library_sources(boards.c)
zephyr_library_sources(bsp/bsp.c)
zephyr_library_sources(bsp/bsp_btn_ble.c)

zephyr_library_sources(sensorsim.c)

zephyr_library_sources(ble_advertising.c)
zephyr_library_sources(ble_link_ctx_manager.c)
zephyr_library_sources(ble_conn_state.c)
zephyr_library_sources(ble_conn_params.c)

zephyr_library_sources(nrf_ble_gatt.c)
zephyr_library_sources(nrf_ble_qwr.c)

zephyr_library_sources(nrf_pwr_mgmt.c)
zephyr_library_sources(irq_connect.c)
zephyr_library_sources(nrf_atfifo.c)
zephyr_library_sources(nrf_atomic.c)
zephyr_library_sources(nrf_atflags.c)
zephyr_linker_sources(SECTIONS nrf_pwr_mgmt.ld)


add_subdirectory(fstorage)
add_subdirectory(fds)
add_subdirectory(ble_services)
add_subdirectory(peer_manager)