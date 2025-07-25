/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef ESF_PARAMETER_STORAGE_MANAGER_SRC_PARAMETER_STORAGE_MANAGER_UTILITY_H_
#define ESF_PARAMETER_STORAGE_MANAGER_SRC_PARAMETER_STORAGE_MANAGER_UTILITY_H_

#include "parameter_storage_manager.h"

// """Gets a string describing the error code.

// Gets a string describing the error code. It is thread safe.

// Args:
//     [IN] status (EsfParameterStorageManagerStatus): error code.

// Returns:
//     const char*: A string describing the error code. Never return NULL.

// Note:
// """
const char* EsfParameterStorageManagerStrError(
    EsfParameterStorageManagerStatus status);

// """Determines whether data in 'buf' is included in data in 'cache'.

// Uses length and offset to compare two pieces of data.

// Args:
//     [IN] buf_len (uint32_t): data length.
//     [IN] buf_offset (uint32_t): data offset.
//     [IN] buf (const uint8_t *): data. returns false if it is NULL.
//     [IN] cache_len (uint32_t): cache data length
//     [IN] cache_offset (uint32_t): cache data offset
//     [IN] cache (const uint8_t *): cache data. returns false if it is NULL.

// Returns:
//     bool: Returns true if 'buf' and 'cache' match. otherwise returns false.

// Note:
// """
bool EsfParameterStorageManagerStorageAdapterIsWrittenData(
    uint32_t buf_len, uint32_t buf_offset, const uint8_t* buf,
    uint32_t cache_len, uint32_t cache_offset, const uint8_t* cache);

#endif  // ESF_PARAMETER_STORAGE_MANAGER_SRC_PARAMETER_STORAGE_MANAGER_UTILITY_H_  // NOLINT
