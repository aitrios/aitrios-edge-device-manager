/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESF_FIRMWARE_MANAGER_SRC_COMMON_FIRMWARE_MANAGER_COMMON_H
#define ESF_FIRMWARE_MANAGER_SRC_COMMON_FIRMWARE_MANAGER_COMMON_H

#include <stdio.h>

#include "firmware_manager.h"
#include "memory_manager.h"

// Verify that handle != NULL && handle == s_active_context
#define ESF_FW_MGR_VERIFY_HANDLE(handle) \
  ((s_active_context != NULL) && ((void *)handle == (void *)s_active_context))

#define SAFE_STRNCPY(dest, src, size) snprintf((dest), (size), "%s", (src))

EsfFwMgrResult EsfFwMgrMapOrOpenLargeHeapMemory(
    EsfMemoryManagerHandle mem_handle, int32_t size, bool map_suppoted,
    void **mapped_address);
EsfFwMgrResult EsfFwMgrUnmapOrCloseLargeHeapMemory(
    EsfMemoryManagerHandle mem_handle, bool map_supported);

void EsfFwMgrHashToHexString(int32_t hash_size, const uint8_t *hash,
                             char *hex_string);

#endif  // ESF_FIRMWARE_MANAGER_SRC_COMMON_FIRMWARE_MANAGER_COMMON_H
