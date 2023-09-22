/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUIT_MEMPTR_STORAGE_H__
#define SUIT_MEMPTR_STORAGE_H__

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#define SUCCESS		   0
#define INVALID_ARGUMENT   -1
#define INVALID_RECORD	   -2
#define NO_FREE_RECORDS	   -3
#define UNALLOCATED_RECORD -4

typedef void *memptr_storage_handle;

/**
 * @brief Get the memptr storage object
 *
 * @param handle Memptr storage handle
 * @return int 0 in case of success, otherwise error code
 */
int get_memptr_storage(memptr_storage_handle *handle);

/**
 * @brief Release storage record
 *
 * @param handle Memptr storage handle
 * @return int 0 in case of success, otherwise error code
 */
int release_memptr_storage(memptr_storage_handle handle);

/**
 * @brief
 *
 * @param handle Memptr storage handle
 * @param payload_ptr Payload data pointer to be stored
 * @param payload_size Payload data size
 * @return int 0 in case of success, otherwise error code
 */
int store_memptr_ptr(memptr_storage_handle handle, uint8_t *payload_ptr, size_t payload_size);

/**
 * @brief Get the memptr ptr object
 *
 * @param handle Memptr storage handle
 * @param payload_ptr Pointer to payload pointer
 * @param payload_size Pointer to payload size
 * @return int 0 in case of success, otherwise error code
 */
int get_memptr_ptr(memptr_storage_handle handle, uint8_t **payload_ptr, size_t *payload_size);

#endif /* SUIT_MEMPTR_STORAGE_H__ */
