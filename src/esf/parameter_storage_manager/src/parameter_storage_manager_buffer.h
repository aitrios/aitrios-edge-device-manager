/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef ESF_PARAMETER_STORAGE_MANAGER_SRC_PARAMETER_STORAGE_MANAGER_BUFFER_H_
#define ESF_PARAMETER_STORAGE_MANAGER_SRC_PARAMETER_STORAGE_MANAGER_BUFFER_H_

#include "parameter_storage_manager.h"
#include "parameter_storage_manager/src/parameter_storage_manager_config.h"
#include ESF_PARAMETER_STORAGE_MANAGER_UTILITY_MEMORY_FILE

#define ESF_PARAMETER_STORAGE_MANAGER_BUFFER_INITIALIZER {0, 0}

// Structure that manages large heap handle information.
typedef struct EsfParameterStorageManagerBuffer {
  // The handle to the heap. If NULL, the handle has not been obtained yet.
  EsfMemoryManagerHandle handle;

  // The size of the allocated large heap.
  uint32_t size;
} EsfParameterStorageManagerBuffer;

// Forward declaration to avoid cross-referencing header files.
typedef struct EsfParameterStorageManagerWorkMemberData
    EsfParameterStorageManagerWorkMemberData;

// """Allocates a buffer in the Large Heap.

// Allocates a buffer in the Large Heap.

// Args:
//     [IN] size (uint32_t):
//          The size of the Large Heap to be allocated.
//     [OUT] buffer (EsfParameterStorageManagerBuffer*):
//          A buffer structure that holds a large heap.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk:
//          Success.
//     kEsfParameterStorageManagerStatusInvalidArgument:
//          The argument is invalid.
//     kEsfParameterStorageManagerStatusResourceExhausted:
//          Failed to allocate Large Heap.
//     kEsfParameterStorageManagerStatusInternal:
//          Internal processing failed.

// Note:
// """
EsfParameterStorageManagerStatus EsfParameterStorageManagerBufferAllocate(
    uint32_t size, EsfParameterStorageManagerBuffer* buffer);

// """Frees a buffer from the large heap.

// Frees a buffer from the large heap.

// Args:
//     [IN] buffer (EsfParameterStorageManagerBuffer*):
//          A buffer structure that holds a large heap.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk:
//          Success.
//     kEsfParameterStorageManagerStatusInvalidArgument:
//          The argument is invalid.
//     kEsfParameterStorageManagerStatusInternal:
//          Internal processing failed.

// Note:
// """
EsfParameterStorageManagerStatus EsfParameterStorageManagerBufferFree(
    EsfParameterStorageManagerBuffer* buffer);

// """Saves data from the large heap buffer to the data storage area.

// Saves data from the large heap buffer to the data storage area.

// Args:
//     [IN] buffer (EsfParameterStorageManagerBuffer*):
//          A buffer structure that holds a large heap.
//     [IN] id (EsfParameterStorageManagerItemID):
//          The data ID.
//     [IN/OUT] member (EsfParameterStorageManagerWorkMemberData*):
//          Data member context. Cannot pass NULL to this argument.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk:
//          Success.
//     kEsfParameterStorageManagerStatusInvalidArgument:
//          The argument is invalid.
//     kEsfParameterStorageManagerStatusPermissionDenied:
//          Could not restore the data to the state before saving.
//     kEsfParameterStorageManagerStatusDataLoss:
//          Unable to access data storage area.
//     kEsfParameterStorageManagerStatusUnavailable:
//          Failed to save data.
//     kEsfParameterStorageManagerStatusInternal:
//          Internal processing failed.

// Note:
// """
EsfParameterStorageManagerStatus EsfParameterStorageManagerBufferSave(
    EsfParameterStorageManagerBuffer* buffer,
    EsfParameterStorageManagerItemID id,
    EsfParameterStorageManagerWorkMemberData* member);

// """Gets data from the data storage area into a large heap buffer.

// Gets data from the data storage area into a large heap buffer.

// Args:
//     [IN] buffer (EsfParameterStorageManagerBuffer*):
//          A buffer structure that holds a large heap.
//     [IN] id (EsfParameterStorageManagerItemID):
//          The data ID.
//     [IN/OUT] member (EsfParameterStorageManagerWorkMemberData*):
//          Data member context. Cannot pass NULL to this argument.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk:
//          Success.
//     kEsfParameterStorageManagerStatusInvalidArgument:
//          The argument is invalid.
//     kEsfParameterStorageManagerStatusPermissionDenied:
//          Could not restore the data to the state before saving.
//     kEsfParameterStorageManagerStatusDataLoss:
//          Unable to access data storage area.
//     kEsfParameterStorageManagerStatusUnavailable:
//          Failed to load data.
//     kEsfParameterStorageManagerStatusInternal:
//          Internal processing failed.

// Note:
// """
EsfParameterStorageManagerStatus EsfParameterStorageManagerBufferLoad(
    EsfParameterStorageManagerBuffer* buffer,
    EsfParameterStorageManagerItemID id,
    EsfParameterStorageManagerWorkMemberData* member);

// """Determines whether data in 'data' is included in data in 'buffer'.

// Uses length and offset to compare two pieces of data.

// Args:
//     [IN] buffer (EsfParameterStorageManagerBuffer*):
//          buffer of Large Heap.
//     [IN] offset (uint32_t):
//          The offset into the Large Heap where comparison begin.
//     [IN] size (uint32_t):
//          data length.
//     [IN] data (const uint8_t *):
//          data.

// Returns:
//     bool: Returns true if 'data' and 'buffer' match. otherwise returns false.

// Note:
// """
bool EsfParameterStorageManagerBufferIsEqual(
    EsfParameterStorageManagerBuffer* buffer, uint32_t offset, uint32_t size,
    const uint8_t* data);

#endif  // ESF_PARAMETER_STORAGE_MANAGER_SRC_PARAMETER_STORAGE_MANAGER_BUFFER_H_
