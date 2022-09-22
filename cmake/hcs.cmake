set(IC_NEXT_RELATIVE_DIR "../../modules/ic-next")
get_filename_component(IC_NEXT_DIR ${CMAKE_CURRENT_LIST_DIR}/${IC_NEXT_RELATIVE_DIR} ABSOLUTE)

list(APPEND
	CUSTOM_DTS_OVERLAY_DIR_LIST
	${IC_NEXT_DIR}/dts/arm/nordic_nrf_next/overlays
	${IC_NEXT_DIR}/dts/common/nordic_nrf_next/overlays
	${IC_NEXT_DIR}/dts/riscv/nordic_nrf_next/overlays
	)
