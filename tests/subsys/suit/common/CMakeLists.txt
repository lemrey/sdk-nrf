#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(sysinit)

if (CONFIG_MBEDTLS)
# Link SUIT common (incl. crypto) module with mbedTLS library, that provides PSA crypto APIs.
target_link_libraries(suit_common INTERFACE mbedTLS)

# Fix the mbedTLS build scripts:
#   The "mbedtls/library/psa_crypto_cipher.c" file includes "<psa_crypto_cipher.h>",
#   but this header is not a part of mbedTLS public API, thus it is not found and
#   the compiler reports this as an error.
target_include_directories(mbedTLS INTERFACE ${ZEPHYR_MBEDTLS_MODULE_DIR}/library)
endif()

if (CONFIG_SUIT_DEFAULT_MBEDTLS_CONFIG)
# Add include path for the default, customized mbedTLS configuration header.
zephyr_include_directories(${CMAKE_CURRENT_LIST_DIR}/tls_config)
endif()
