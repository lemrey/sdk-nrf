#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

add_library(tfm_app_rot_partition_tp STATIC)

target_include_directories(tfm_app_rot_partition_tp
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    PRIVATE
        ${CMAKE_BINARY_DIR}/generated
)
target_include_directories(tfm_partitions
    INTERFACE
        ${CMAKE_BINARY_DIR}/generated
)

target_sources(tfm_app_rot_partition_tp
    PRIVATE
        tee_partition.c
        iuicc.c
)

# The generated sources
target_sources(tfm_app_rot_partition_tp
    PRIVATE
        $<$<BOOL:${TFM_PSA_API}>:
            ${CMAKE_BINARY_DIR}/generated/auto_generated/intermedia_tfm_tee_partition.c>
)
target_sources(tfm_partitions
    INTERFACE
        $<$<BOOL:${TFM_PSA_API}>:
            ${CMAKE_BINARY_DIR}/generated/auto_generated/load_info_tfm_tee_partition.c>
)

target_link_libraries(tfm_app_rot_partition_tp
    PRIVATE
        tfm_secure_api
        psa_interface
        secure_fw
        platform_s
        tfm_psa_rot_partition_crypto
)

target_link_libraries(tfm_partitions
    INTERFACE
        tfm_app_rot_partition_tp
)

target_compile_definitions(tfm_partition_defs
    INTERFACE
        TFM_PARTITION_TEE_PARTITION
)
