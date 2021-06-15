#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
configure_file(
        ${CMAKE_CURRENT_LIST_DIR}/tee_partition/tfm_manifest_list.yaml
        ${CMAKE_CURRENT_BINARY_DIR}/tee_partition/tfm_manifest_list.yaml)

set_property(TARGET zephyr_property_target
        APPEND PROPERTY TFM_CMAKE_OPTIONS
        -DTFM_EXTRA_MANIFEST_LIST_PATH=${CMAKE_CURRENT_BINARY_DIR}/tee_partition/tfm_manifest_list.yaml
        -DTFM_EXTRA_PARTITION_CMAKE_PATH=${CMAKE_CURRENT_LIST_DIR}/tee_partition
        )

target_include_directories(app PRIVATE
        ${ZEPHYR_TRUSTED_FIRMWARE_M_MODULE_DIR}/trusted-firmware-m/interface/include
        )

target_compile_definitions(app
        PRIVATE TFM_PARTITION_TEE_PARTITION)

zephyr_library()

zephyr_library_sources(nrf_modem_lib.c)
zephyr_library_sources(nrf_modem_os.c)
zephyr_library_sources(nrf91_sockets.c)
zephyr_library_sources(shmem_sanity.c)

