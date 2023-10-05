#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_include_directories(testbinary PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/include
)

if (CONFIG_MOCK_SUIT_PROCESSOR)
  # Include suit-processor directory, so manifest and cose libraries get generated.
  add_subdirectory(${SUIT_PROCESSOR_DIR} suit_processor)
  target_compile_options(test_interface INTERFACE -DCONFIG_SUIT_PROCESSOR)
  target_include_directories(testbinary PRIVATE
    ${SUIT_PROCESSOR_DIR}/include
  )
endif()

if (CONFIG_MOCK_SUIT_MCI)
  target_compile_options(test_interface INTERFACE -DCONFIG_SUIT_MCI)
  target_include_directories(testbinary PRIVATE
    ${SUIT_SUBSYS_DIR}/mci/include
  )
endif()

if (NOT CONFIG_MOCK_SUIT_MCI_UTILS)
  target_sources(testbinary PRIVATE
    ${SUIT_SUBSYS_DIR}/mci/src/utils.c
  )
endif()

if (CONFIG_MOCK_SUIT_STORAGE)
  target_compile_options(test_interface INTERFACE -DCONFIG_SUIT_STORAGE)
  target_include_directories(testbinary PRIVATE
    ${SUIT_SUBSYS_DIR}/storage/include
  )
endif()

if (CONFIG_MOCK_SUIT_UTILS)
  target_compile_options(test_interface INTERFACE -DCONFIG_SUIT_UTILS)
  target_include_directories(testbinary PRIVATE ${SUIT_SUBSYS_DIR}/utils/include)
  target_include_directories(testbinary PRIVATE $ENV{ZEPHYR_BASE}/../modules/lib/zcbor/include)
endif()

if (NOT CONFIG_MOCK_SUIT_PLAT_DECODE_UTIL)
  target_sources(testbinary PRIVATE
    ${SUIT_SUBSYS_DIR}/utils/src/suit_plat_decode_util.c
  )
endif()

if (CONFIG_MOCK_SUIT_LOG)
  target_compile_options(test_interface INTERFACE -DCONFIG_SUIT_LOG_LEVEL)
  target_compile_options(test_interface INTERFACE -DCONFIG_SUIT_LOG_LEVEL_DBG)
  target_compile_options(test_interface INTERFACE -include ztest.h)
  target_sources(testbinary PRIVATE 
    $ENV{ZEPHYR_BASE}/subsys/logging/log_minimal.c
  )
endif()

if (CONFIG_MOCK_SUIT_CRYPTO)
  target_compile_options(test_interface INTERFACE -DCONFIG_SUIT_CRYPTO)
  target_include_directories(testbinary PRIVATE $ENV{ZEPHYR_BASE}/../modules/crypto/mbedtls/include)
endif()

if (CONFIG_MOCK_SUIT_PLAT_CHECK_COMPONENT_ID)
  target_compile_options(test_interface INTERFACE -DCONFIG_SUIT_PLAT_CHECK_COMPONENT_ID)
  target_include_directories(testbinary PRIVATE ${SUIT_SUBSYS_DIR}/platform/include)
endif()

if (CONFIG_MOCK_SUIT_PLATFORM)
  target_compile_options(test_interface INTERFACE -DCONFIG_SUIT_PLATFORM)
  target_include_directories(testbinary PRIVATE ${SUIT_SUBSYS_DIR}/platform/include)
endif()

if (CONFIG_MOCK_SUIT_MEMPTR_STORAGE)
  target_compile_options(test_interface INTERFACE -DCONFIG_SUIT_MEMPTR_STORAGE)
  target_include_directories(testbinary PRIVATE ${SUIT_SUBSYS_DIR}/memptr_storage/include)
endif()
