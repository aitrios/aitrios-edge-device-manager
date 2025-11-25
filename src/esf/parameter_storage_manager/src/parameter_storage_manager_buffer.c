/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#include "parameter_storage_manager/src/parameter_storage_manager_buffer.h"

#include "parameter_storage_manager/src/parameter_storage_manager_internal_work.h"
#include "parameter_storage_manager/src/parameter_storage_manager_resource.h"
#include "parameter_storage_manager/src/parameter_storage_manager_storage_adapter.h"
#include "parameter_storage_manager/src/parameter_storage_manager_utility.h"

#if defined(CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_MEMORY_MANAGER_STUB)
#define EsfMemoryManagerAllocate PSM_EsfMemoryManagerAllocate
#define EsfMemoryManagerFree PSM_EsfMemoryManagerFree
#define EsfMemoryManagerMap PSM_EsfMemoryManagerMap
#define EsfMemoryManagerUnmap PSM_EsfMemoryManagerUnmap
#define EsfMemoryManagerWasmAllocate PSM_EsfMemoryManagerWasmAllocate
#define EsfMemoryManagerWasmFree PSM_EsfMemoryManagerWasmFree
#define EsfMemoryManagerInitialize PSM_EsfMemoryManagerInitialize
#define EsfMemoryManagerFinalize PSM_EsfMemoryManagerFinalize
#define EsfMemoryManagerFopen PSM_EsfMemoryManagerFopen
#define EsfMemoryManagerFclose PSM_EsfMemoryManagerFclose
#define EsfMemoryManagerFseek PSM_EsfMemoryManagerFseek
#define EsfMemoryManagerFwrite PSM_EsfMemoryManagerFwrite
#define EsfMemoryManagerFread PSM_EsfMemoryManagerFread
#define EsfMemoryManagerIsMapSupport PSM_EsfMemoryManagerIsMapSupport
#endif

// """Saves data from a mapped large heap buffer to the a storage area.

// Saves data from a mapped large heap buffer to a data storage area.

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
static EsfParameterStorageManagerStatus
EsfParameterStorageManagerBufferSaveMemoryMap(
    EsfParameterStorageManagerBuffer* buffer,
    EsfParameterStorageManagerItemID id,
    EsfParameterStorageManagerWorkMemberData* member);

// """Save the data to the data storage area using file I/O with large heap
// buffers.

// Save the data to the data storage area using file I/O with large heap
// buffers.

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
static EsfParameterStorageManagerStatus
EsfParameterStorageManagerBufferSaveMemoryFile(
    EsfParameterStorageManagerBuffer* buffer,
    EsfParameterStorageManagerItemID id,
    EsfParameterStorageManagerWorkMemberData* member);

// """Retrieves data from a data storage area into a mapped large heap buffer.

// Retrieves data from a data storage area into a mapped large heap buffer.

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
static EsfParameterStorageManagerStatus
EsfParameterStorageManagerBufferLoadMemoryMap(
    EsfParameterStorageManagerBuffer* buffer,
    EsfParameterStorageManagerItemID id,
    EsfParameterStorageManagerWorkMemberData* member);

// """Retrieves data from the data storage area using file I/O in a large heap
// buffer.

// Retrieves data from the data storage area using file I/O in a large heap
// buffer.

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
static EsfParameterStorageManagerStatus
EsfParameterStorageManagerBufferLoadMemoryFile(
    EsfParameterStorageManagerBuffer* buffer,
    EsfParameterStorageManagerItemID id,
    EsfParameterStorageManagerWorkMemberData* member);

// """Determines whether data in 'data' is included in a mapped data in
// 'buffer'.

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
static bool EsfParameterStorageManagerBufferIsEqualMemoryMap(
    EsfParameterStorageManagerBuffer* buffer, uint32_t offset, uint32_t size,
    const uint8_t* data);

// """Determines whether data in 'data' is included in data in 'buffer'.

// Uses length and offset to compare two pieces of data.
// The data is retrieved from 'buffer' using File I/O.

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
static bool EsfParameterStorageManagerBufferIsEqualMemoryFile(
    EsfParameterStorageManagerBuffer* buffer, uint32_t offset, uint32_t size,
    const uint8_t* data);

EsfParameterStorageManagerStatus EsfParameterStorageManagerBufferAllocate(
    uint32_t size, EsfParameterStorageManagerBuffer* buffer) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    if (size == 0 || INT32_MAX <= size || buffer == NULL) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid arguments. size=%" PRIu32 ", buffer=%p, ret=%u(%s)", size,
          (void*)buffer, ret, EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }

    EsfMemoryManagerResult mem_ret = EsfMemoryManagerAllocate(
        kEsfMemoryManagerTargetLargeHeap, NULL, size, &buffer->handle);
    if (mem_ret != kEsfMemoryManagerResultSuccess) {
      ret = kEsfParameterStorageManagerStatusResourceExhausted;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Large heap allocation failed. size=%" PRIu32
          ", mem_ret=%u, ret=%u(%s)",
          size, mem_ret, ret, EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_MEMORY_CONTROL_FAILURE);
      break;
    }

    buffer->size = size;
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Buffer allocated. buffer=%p, handle=%" PRIuPTR ", size=%" PRIu32,
        (void*)buffer, (uintptr_t)buffer->handle, buffer->size);
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus EsfParameterStorageManagerBufferFree(
    EsfParameterStorageManagerBuffer* buffer) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    if (buffer == NULL) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid buffer pointer. buffer=%p, ret=%u(%s)", (void*)buffer, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }
    if (buffer->size == 0) {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE(
          "Buffer size is 0, skipping release. buffer=%p", (void*)buffer);
      break;
    }

    EsfMemoryManagerResult mem_ret = EsfMemoryManagerFree(buffer->handle, NULL);
    if (mem_ret != kEsfMemoryManagerResultSuccess) {
      ret = kEsfParameterStorageManagerStatusResourceExhausted;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Large heap free failed. buffer=%p, handle=%" PRIuPTR
          ", size=%" PRIu32 ", mem_ret=%u, ret=%u(%s)",
          (void*)buffer, (uintptr_t)buffer->handle, buffer->size, mem_ret, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_MEMORY_CONTROL_FAILURE);
    } else {
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
          "Buffer freed. buffer=%p, handle=%" PRIuPTR ", size=%" PRIu32,
          (void*)buffer, (uintptr_t)buffer->handle, buffer->size);
    }
    buffer->size = 0;
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus EsfParameterStorageManagerBufferSave(
    EsfParameterStorageManagerBuffer* buffer,
    EsfParameterStorageManagerItemID id,
    EsfParameterStorageManagerWorkMemberData* member) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;
  EsfMemoryManagerMapSupport map_enabled = kEsfMemoryManagerMapIsSupport;

  do {
    if (buffer == NULL || member == NULL) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid arguments. buffer=%p, member=%p, ret=%u(%s)", (void*)buffer,
          (void*)member, ret, EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }

    EsfMemoryManagerResult mem_ret =
        EsfMemoryManagerIsMapSupport(buffer->handle, &map_enabled);
    if (mem_ret != kEsfMemoryManagerResultSuccess) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Map support check failed. buffer=%p, handle=%" PRIuPTR
          ", size=%" PRIu32 ", mem_ret=%u, ret=%u(%s)",
          (void*)buffer, (uintptr_t)buffer->handle, buffer->size, mem_ret, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_MEMORY_CONTROL_FAILURE);
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Map support: %u, buffer=%p, handle=%" PRIuPTR ", size=%" PRIu32,
        map_enabled, (void*)buffer, (uintptr_t)buffer->handle, buffer->size);

    switch (map_enabled) {
      case kEsfMemoryManagerMapIsSupport:
        ret = EsfParameterStorageManagerBufferSaveMemoryMap(buffer, id, member);
        break;
      case kEsfMemoryManagerMapIsNotSupport:
        ret =
            EsfParameterStorageManagerBufferSaveMemoryFile(buffer, id, member);
        break;
      default:
        ret = kEsfParameterStorageManagerStatusInternal;
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Unknown map support. MapIsSupport=%u, buffer=%p, "
            "handle=%" PRIuPTR ", size=%" PRIu32 ", ret=%u(%s)",
            map_enabled, (void*)buffer, (uintptr_t)buffer->handle, buffer->size,
            ret, EsfParameterStorageManagerStrError(ret));
        ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
            ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
        break;
    }

    if (ret == kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
          "Buffer saved. id=%u, buffer=%p, handle=%" PRIuPTR ", size=%" PRIu32,
          id, (void*)buffer, (uintptr_t)buffer->handle, buffer->size);
    } else {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Buffer save failed. id=%u, buffer=%p, handle=%" PRIuPTR
          ", size=%" PRIu32 ", ret=%u(%s)",
          id, (void*)buffer, (uintptr_t)buffer->handle, buffer->size, ret,
          EsfParameterStorageManagerStrError(ret));
    }
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus EsfParameterStorageManagerBufferLoad(
    EsfParameterStorageManagerBuffer* buffer,
    EsfParameterStorageManagerItemID id,
    EsfParameterStorageManagerWorkMemberData* member) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;
  EsfMemoryManagerMapSupport map_enabled = kEsfMemoryManagerMapIsSupport;

  do {
    if (buffer == NULL || member == NULL) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid arguments. buffer=%p, member=%p, ret=%u(%s)", (void*)buffer,
          (void*)member, ret, EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }

    EsfMemoryManagerResult mem_ret =
        EsfMemoryManagerIsMapSupport(buffer->handle, &map_enabled);
    if (mem_ret != kEsfMemoryManagerResultSuccess) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Map support check failed. buffer=%p, handle=%" PRIuPTR
          ", size=%" PRIu32 ", mem_ret=%u, ret=%u(%s)",
          (void*)buffer, (uintptr_t)buffer->handle, buffer->size, mem_ret, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_MEMORY_CONTROL_FAILURE);
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Map support: %u, buffer=%p, handle=%" PRIuPTR ", size=%" PRIu32,
        map_enabled, (void*)buffer, (uintptr_t)buffer->handle, buffer->size);

    switch (map_enabled) {
      case kEsfMemoryManagerMapIsSupport:
        ret = EsfParameterStorageManagerBufferLoadMemoryMap(buffer, id, member);
        break;
      case kEsfMemoryManagerMapIsNotSupport:
        ret =
            EsfParameterStorageManagerBufferLoadMemoryFile(buffer, id, member);
        break;
      default:
        ret = kEsfParameterStorageManagerStatusInternal;
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Unknown map support. MapIsSupport=%u, buffer=%p, "
            "handle=%" PRIuPTR ", size=%" PRIu32 ", ret=%u(%s)",
            map_enabled, (void*)buffer, (uintptr_t)buffer->handle, buffer->size,
            ret, EsfParameterStorageManagerStrError(ret));
        ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
            ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
        break;
    }

    if (ret == kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
          "Buffer loaded. id=%u, buffer=%p, handle=%" PRIuPTR ", size=%" PRIu32,
          id, (void*)buffer, (uintptr_t)buffer->handle, buffer->size);
    } else {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Buffer load failed. id=%u, buffer=%p, handle=%" PRIuPTR
          ", size=%" PRIu32 ", ret=%u(%s)",
          id, (void*)buffer, (uintptr_t)buffer->handle, buffer->size, ret,
          EsfParameterStorageManagerStrError(ret));
    }
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

bool EsfParameterStorageManagerBufferIsEqual(
    EsfParameterStorageManagerBuffer* buffer, uint32_t offset, uint32_t size,
    const uint8_t* data) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  bool ret = true;
  EsfMemoryManagerMapSupport map_enabled = kEsfMemoryManagerMapIsSupport;

  do {
    if (buffer == NULL || data == NULL) {
      ret = false;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid arguments. buffer=%p, data=%p, ret=%d", (void*)buffer,
          (void*)data, ret);
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }

    if (buffer->size < size) {
      ret = false;
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
          "Buffer size too small. buffer=%p, handle=%" PRIuPTR
          ", buffer_size=%" PRIu32 ", comparison_size=%" PRIu32,
          (void*)buffer, (uintptr_t)buffer->handle, buffer->size, size);
      break;
    }
    if (buffer->size - size < offset) {
      ret = false;
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
          "Offset+size exceeds buffer. buffer=%p, handle=%" PRIuPTR
          ", buffer_size=%" PRIu32 ", offset=%" PRIu32 ", size=%" PRIu32,
          (void*)buffer, (uintptr_t)buffer->handle, buffer->size, offset, size);
      break;
    }

    EsfMemoryManagerResult mem_ret =
        EsfMemoryManagerIsMapSupport(buffer->handle, &map_enabled);
    if (mem_ret != kEsfMemoryManagerResultSuccess) {
      ret = false;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Map support check failed. buffer=%p, handle=%" PRIuPTR
          ", size=%" PRIu32 ", mem_ret=%u",
          (void*)buffer, (uintptr_t)buffer->handle, buffer->size, mem_ret);
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_MEMORY_CONTROL_FAILURE);
      break;
    }

    switch (map_enabled) {
      case kEsfMemoryManagerMapIsSupport:
        ret = EsfParameterStorageManagerBufferIsEqualMemoryMap(buffer, offset,
                                                               size, data);
        break;
      case kEsfMemoryManagerMapIsNotSupport:
        ret = EsfParameterStorageManagerBufferIsEqualMemoryFile(buffer, offset,
                                                                size, data);
        break;
      default:
        ret = false;
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Unknown map support. MapIsSupport=%u, buffer=%p, "
            "handle=%" PRIuPTR ", size=%" PRIu32,
            map_enabled, (void*)buffer, (uintptr_t)buffer->handle,
            buffer->size);
        ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
            ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
        break;
    }

    if (ret) {
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
          "Buffer content equal. buffer=%p, handle=%" PRIuPTR ", size=%" PRIu32
          ", offset=%" PRIu32,
          (void*)buffer, (uintptr_t)buffer->handle, buffer->size, offset);
    } else {
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
          "Buffer content not equal. buffer=%p, handle=%" PRIuPTR
          ", size=%" PRIu32 ", offset=%" PRIu32,
          (void*)buffer, (uintptr_t)buffer->handle, buffer->size, offset);
    }
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %d", ret);
  return ret;
}

static EsfParameterStorageManagerStatus
EsfParameterStorageManagerBufferSaveMemoryMap(
    EsfParameterStorageManagerBuffer* buffer,
    EsfParameterStorageManagerItemID id,
    EsfParameterStorageManagerWorkMemberData* member) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    void* addr = NULL;
    uint32_t save_size = 0;
    EsfMemoryManagerResult mem_ret =
        EsfMemoryManagerMap(buffer->handle, NULL, buffer->size, &addr);
    if (mem_ret != kEsfMemoryManagerResultSuccess) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "LHeap map failed. buffer=%p, handle=%" PRIuPTR ", size=%" PRIu32
          ", mem_ret=%u, ret=%u(%s)",
          (void*)buffer, (uintptr_t)buffer->handle, buffer->size, mem_ret, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_MEMORY_CONTROL_FAILURE);
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_TRACE(
        "LHeap mapped. buffer=%p, handle=%" PRIuPTR ", size=%" PRIu32
        ", addr=%p",
        (void*)buffer, (uintptr_t)buffer->handle, buffer->size, addr);

    ret = EsfParameterStorageManagerStorageAdapterSave(
        id, 0, buffer->size, addr, &save_size, member);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "LHeap data save failed. id=%u, size=%" PRIu32 ", ret=%u(%s)", id,
          buffer->size, ret, EsfParameterStorageManagerStrError(ret));
    } else if (buffer->size != save_size) {
      ret = kEsfParameterStorageManagerStatusDataLoss;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Save size mismatch. buffer->size=%" PRIu32 ", save_size=%" PRIu32
          ", ret=%u(%s)",
          buffer->size, save_size, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
    } else {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE(
          "LHeap data saved. id=%u, save_size=%" PRIu32, id, save_size);
    }

    mem_ret = EsfMemoryManagerUnmap(buffer->handle, &addr);
    if (mem_ret != kEsfMemoryManagerResultSuccess) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "LHeap unmap failed. mem_ret=%u, ret=%u(%s)", mem_ret, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_MEMORY_CONTROL_FAILURE);
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_TRACE(
        "LHeap unmapped. buffer=%p, handle=%" PRIuPTR, (void*)buffer,
        (uintptr_t)buffer->handle);
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static EsfParameterStorageManagerStatus
EsfParameterStorageManagerBufferSaveMemoryFile(
    EsfParameterStorageManagerBuffer* buffer,
    EsfParameterStorageManagerItemID id,
    EsfParameterStorageManagerWorkMemberData* member) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;
  bool is_opened = false;

  do {
    if (CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_BUFFER_LENGTH <
            buffer->size &&
        member->storage.capabilities.enable_offset == false) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Offset writing not possible. id=%u, ret=%u(%s)", id, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INTERNAL_PROCESS_ERROR);
      break;
    }

    uint8_t* addr = EsfParameterStorageManagerResourceGetBuffer();
    if (addr == NULL) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Temp heap allocation failed. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }

    EsfMemoryManagerResult mem_ret = EsfMemoryManagerFopen(buffer->handle);
    if (mem_ret != kEsfMemoryManagerResultSuccess) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "LHeap open failed. mem_ret=%u, ret=%u(%s)", mem_ret, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_MEMORY_CONTROL_FAILURE);
      break;
    }
    is_opened = true;

    off_t roffset;
    mem_ret = EsfMemoryManagerFseek(buffer->handle, 0, SEEK_SET, &roffset);
    if (mem_ret != kEsfMemoryManagerResultSuccess) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "LHeap seek failed. mem_ret=%u, ret=%u(%s)", mem_ret, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_MEMORY_CONTROL_FAILURE);
      break;
    }

    uint32_t offset = 0;
    uint32_t save_size = 0;
    while (offset < buffer->size) {
      const uint32_t buf_size =
          buffer->size - offset <
                  CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_BUFFER_LENGTH
              ? buffer->size - offset
              : CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_BUFFER_LENGTH;
      size_t read_size = 0;
      mem_ret =
          EsfMemoryManagerFread(buffer->handle, addr, buf_size, &read_size);
      if (mem_ret != kEsfMemoryManagerResultSuccess) {
        ret = kEsfParameterStorageManagerStatusInternal;
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "LHeap read failed. mem_ret=%u, ret=%u(%s)", mem_ret, ret,
            EsfParameterStorageManagerStrError(ret));
        ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
            ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_MEMORY_CONTROL_FAILURE);
        break;
      }

      if (read_size != buf_size && offset + read_size < buffer->size) {
        ret = kEsfParameterStorageManagerStatusInternal;
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Unexpected read size. read_size=%zu, offset=%" PRIu32
            ", buffer_size=%" PRIu32 " ret=%u(%s)",
            read_size, offset, buffer->size, ret,
            EsfParameterStorageManagerStrError(ret));
        ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
            ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INTERNAL_PROCESS_ERROR);
        break;
      }

      ret = EsfParameterStorageManagerStorageAdapterSave(
          id, offset, read_size, addr, &save_size, member);
      if (ret != kEsfParameterStorageManagerStatusOk) {
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Storage save failed. ret=%u(%s)", ret,
            EsfParameterStorageManagerStrError(ret));
        break;
      }
      if (read_size != save_size) {
        ret = kEsfParameterStorageManagerStatusInternal;
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Read/save size mismatch. read_size=%zu, save_size=%" PRIu32
            ", ret=%u(%s)",
            read_size, save_size, ret, EsfParameterStorageManagerStrError(ret));
        ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
            ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INTERNAL_PROCESS_ERROR);
        break;
      }
      offset += read_size;
      ESF_PARAMETER_STORAGE_MANAGER_TRACE(
          "Chunk saved. offset=%" PRIu32 ", size=%zu", offset, read_size);
    }

    if (ret == kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE(
          "Buffer saved. id=%u, total_size=%" PRIu32, id, buffer->size);
    }
  } while (0);

  if (is_opened) {
    EsfMemoryManagerResult mem_ret = EsfMemoryManagerFclose(buffer->handle);
    if (mem_ret != kEsfMemoryManagerResultSuccess) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "LHeap close failed. mem_ret=%u, ret=%u(%s)", mem_ret, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_MEMORY_CONTROL_FAILURE);
    } else {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE("LHeap closed.");
    }
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static EsfParameterStorageManagerStatus
EsfParameterStorageManagerBufferLoadMemoryMap(
    EsfParameterStorageManagerBuffer* buffer,
    EsfParameterStorageManagerItemID id,
    EsfParameterStorageManagerWorkMemberData* member) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    void* addr = NULL;
    uint32_t load_size = 0;
    EsfMemoryManagerResult mem_ret =
        EsfMemoryManagerMap(buffer->handle, NULL, buffer->size, &addr);
    if (mem_ret != kEsfMemoryManagerResultSuccess) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "LHeap map failed. buffer=%p, handle=%" PRIuPTR ", size=%" PRIu32
          ", mem_ret=%u, ret=%u(%s)",
          (void*)buffer, (uintptr_t)buffer->handle, buffer->size, mem_ret, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_MEMORY_CONTROL_FAILURE);
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_TRACE(
        "LHeap mapped. buffer=%p, handle=%" PRIuPTR ", size=%" PRIu32
        ", addr=%p",
        (void*)buffer, (uintptr_t)buffer->handle, buffer->size, addr);

    ret = EsfParameterStorageManagerStorageAdapterLoad(
        id, member, 0, buffer->size, (uint8_t*)addr, &load_size);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "LHeap data load failed. id=%u, size=%" PRIu32 ", ret=%u(%s)", id,
          buffer->size, ret, EsfParameterStorageManagerStrError(ret));
    } else if (buffer->size != load_size) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Load size mismatch. buffer->size=%" PRIu32 ", load_size=%" PRIu32
          ", ret=%u(%s)",
          buffer->size, load_size, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
    } else {
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
          "LHeap data loaded. id=%u, load_size=%" PRIu32, id, load_size);
    }

    mem_ret = EsfMemoryManagerUnmap(buffer->handle, &addr);
    if (mem_ret != kEsfMemoryManagerResultSuccess) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "LHeap unmap failed. buffer=%p, handle=%" PRIuPTR
          ", mem_ret=%u, ret=%u(%s)",
          (void*)buffer, (uintptr_t)buffer->handle, mem_ret, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_MEMORY_CONTROL_FAILURE);
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_TRACE(
        "LHeap unmapped. buffer=%p, handle=%" PRIuPTR, (void*)buffer,
        (uintptr_t)buffer->handle);
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static EsfParameterStorageManagerStatus
EsfParameterStorageManagerBufferLoadMemoryFile(
    EsfParameterStorageManagerBuffer* buffer,
    EsfParameterStorageManagerItemID id,
    EsfParameterStorageManagerWorkMemberData* member) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;
  bool is_opened = false;

  do {
    if (CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_BUFFER_LENGTH <
            buffer->size &&
        member->storage.capabilities.enable_offset == false) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Offset writing not possible. id=%u, ret=%u(%s)", id, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INTERNAL_PROCESS_ERROR);
      break;
    }

    uint8_t* addr = EsfParameterStorageManagerResourceGetBuffer();
    if (addr == NULL) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Temp heap allocation failed. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }

    EsfMemoryManagerResult mem_ret = EsfMemoryManagerFopen(buffer->handle);
    if (mem_ret != kEsfMemoryManagerResultSuccess) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "LHeap open failed. mem_ret=%u, ret=%u(%s)", mem_ret, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_MEMORY_CONTROL_FAILURE);
      break;
    }
    is_opened = true;

    off_t roffset;
    mem_ret = EsfMemoryManagerFseek(buffer->handle, 0, SEEK_SET, &roffset);
    if (mem_ret != kEsfMemoryManagerResultSuccess) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "LHeap seek failed. mem_ret=%u, ret=%u(%s)", mem_ret, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_MEMORY_CONTROL_FAILURE);
      break;
    }

    uint32_t offset = 0;
    uint32_t read_size = 0;
    while (offset < buffer->size) {
      ret = EsfParameterStorageManagerStorageAdapterLoad(
          id, member, offset,
          CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_BUFFER_LENGTH, addr,
          &read_size);
      if (ret != kEsfParameterStorageManagerStatusOk) {
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Storage load failed. id=%u, offset=%" PRIu32 ", ret=%u(%s)", id,
            offset, ret, EsfParameterStorageManagerStrError(ret));
        break;
      }
      if (read_size !=
              CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_BUFFER_LENGTH &&
          offset + read_size < buffer->size) {
        ret = kEsfParameterStorageManagerStatusInternal;
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Unexpected read size. read_size=%" PRIu32 ", offset=%" PRIu32
            ", buffer_size=%" PRIu32 " ret=%u(%s)",
            read_size, offset, buffer->size, ret,
            EsfParameterStorageManagerStrError(ret));
        ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
            ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INTERNAL_PROCESS_ERROR);
        break;
      }

      size_t save_size = 0;
      mem_ret =
          EsfMemoryManagerFwrite(buffer->handle, addr, read_size, &save_size);
      if (mem_ret != kEsfMemoryManagerResultSuccess) {
        ret = kEsfParameterStorageManagerStatusInternal;
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "LHeap write failed. mem_ret=%u, ret=%u(%s)", mem_ret, ret,
            EsfParameterStorageManagerStrError(ret));
        ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
            ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_MEMORY_CONTROL_FAILURE);
        break;
      }
      if (read_size != save_size) {
        ret = kEsfParameterStorageManagerStatusInternal;
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Read/save size mismatch. read_size=%" PRIu32
            ", save_size=%zu, ret=%u(%s)",
            read_size, save_size, ret, EsfParameterStorageManagerStrError(ret));
        ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
            ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INTERNAL_PROCESS_ERROR);
        break;
      }
      offset += read_size;
      ESF_PARAMETER_STORAGE_MANAGER_TRACE(
          "Chunk loaded and saved. offset=%" PRIu32 ", size=%" PRIu32, offset,
          read_size);
    }

    if (ret == kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE(
          "Buffer loaded. id=%u, total_size=%" PRIu32, id, buffer->size);
    }
  } while (0);

  if (is_opened) {
    EsfMemoryManagerResult mem_ret = EsfMemoryManagerFclose(buffer->handle);
    if (mem_ret != kEsfMemoryManagerResultSuccess) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "LHeap close failed. mem_ret=%u, ret=%u(%s)", mem_ret, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_MEMORY_CONTROL_FAILURE);
    } else {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE("LHeap closed.");
    }
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static bool EsfParameterStorageManagerBufferIsEqualMemoryMap(
    EsfParameterStorageManagerBuffer* buffer, uint32_t offset, uint32_t size,
    const uint8_t* data) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  bool ret = true;

  do {
    void* addr = NULL;
    EsfMemoryManagerResult mem_ret =
        EsfMemoryManagerMap(buffer->handle, NULL, buffer->size, &addr);
    if (mem_ret != kEsfMemoryManagerResultSuccess) {
      ret = false;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "LHeap map failed. buffer=%p, handle=%" PRIuPTR ", size=%" PRIu32
          ", mem_ret=%u",
          (void*)buffer, (uintptr_t)buffer->handle, buffer->size, mem_ret);
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_MEMORY_CONTROL_FAILURE);
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_TRACE(
        "LHeap mapped. buffer=%p, handle=%" PRIuPTR ", size=%" PRIu32
        ", addr=%p",
        (void*)buffer, (uintptr_t)buffer->handle, buffer->size, addr);

    ret = EsfParameterStorageManagerStorageAdapterIsWrittenData(
        size, offset, data, buffer->size, 0, addr);
    if (ret == false) {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE(
          "Data mismatch. buffer=%p, handle=%" PRIuPTR ", offset=%" PRIu32
          ", size=%" PRIu32,
          (void*)buffer, (uintptr_t)buffer->handle, offset, size);
    } else {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE(
          "Data match. buffer=%p, handle=%" PRIuPTR ", offset=%" PRIu32
          ", size=%" PRIu32,
          (void*)buffer, (uintptr_t)buffer->handle, offset, size);
    }

    mem_ret = EsfMemoryManagerUnmap(buffer->handle, &addr);
    if (mem_ret != kEsfMemoryManagerResultSuccess) {
      ret = false;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "LHeap unmap failed. buffer=%p, handle=%" PRIuPTR ", mem_ret=%u",
          (void*)buffer, (uintptr_t)buffer->handle, mem_ret);
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_MEMORY_CONTROL_FAILURE);
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_TRACE(
        "LHeap unmapped. buffer=%p, handle=%" PRIuPTR, (void*)buffer,
        (uintptr_t)buffer->handle);
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %d", ret);
  return ret;
}

static bool EsfParameterStorageManagerBufferIsEqualMemoryFile(
    EsfParameterStorageManagerBuffer* buffer, uint32_t offset, uint32_t size,
    const uint8_t* data) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  bool ret = true;
  bool is_opened = false;

  do {
    uint8_t* addr = EsfParameterStorageManagerResourceGetBuffer();
    if (addr == NULL) {
      ret = false;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR("Temp heap allocation failed.");
      break;
    }

    EsfMemoryManagerResult mem_ret = EsfMemoryManagerFopen(buffer->handle);
    if (mem_ret != kEsfMemoryManagerResultSuccess) {
      ret = false;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "LHeap open failed. buffer=%p, handle=%" PRIuPTR ", mem_ret=%u",
          (void*)buffer, (uintptr_t)buffer->handle, mem_ret);
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_MEMORY_CONTROL_FAILURE);
      break;
    }
    is_opened = true;

    off_t roffset;
    mem_ret = EsfMemoryManagerFseek(buffer->handle, offset, SEEK_SET, &roffset);
    if (mem_ret != kEsfMemoryManagerResultSuccess) {
      ret = false;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "LHeap seek failed. buffer=%p, handle=%" PRIuPTR ", offset=%" PRIu32
          ", mem_ret=%u",
          (void*)buffer, (uintptr_t)buffer->handle, offset, mem_ret);
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_MEMORY_CONTROL_FAILURE);
      break;
    }

    uint32_t compared_size = 0;
    while (compared_size < size) {
      uint32_t read_unit_size =
          size - compared_size <
                  CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_BUFFER_LENGTH
              ? size - compared_size
              : CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_BUFFER_LENGTH;
      size_t read_size = 0;
      mem_ret = EsfMemoryManagerFread(buffer->handle, addr, read_unit_size,
                                      &read_size);
      if (mem_ret != kEsfMemoryManagerResultSuccess) {
        ret = false;
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "LHeap read failed. buffer=%p, handle=%" PRIuPTR
            ", read_unit_size=%" PRIu32 ", mem_ret=%u",
            (void*)buffer, (uintptr_t)buffer->handle, read_unit_size, mem_ret);
        ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
            ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_MEMORY_CONTROL_FAILURE);
        break;
      }

      if (read_size != read_unit_size) {
        ret = false;
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Unexpected read size. buffer=%p, handle=%" PRIuPTR
            ", read_size=%zu, unit_size=%" PRIu32,
            (void*)buffer, (uintptr_t)buffer->handle, read_size,
            read_unit_size);
        ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
            ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INTERNAL_PROCESS_ERROR);
        break;
      }

      ret = EsfParameterStorageManagerStorageAdapterIsWrittenData(
          read_unit_size, 0, data + compared_size, read_unit_size, 0, addr);
      if (ret == false) {
        ESF_PARAMETER_STORAGE_MANAGER_TRACE(
            "Data mismatch. compared_size=%" PRIu32 ", read_unit_size=%" PRIu32,
            compared_size, read_unit_size);
        break;
      }
      compared_size += read_unit_size;
      ESF_PARAMETER_STORAGE_MANAGER_TRACE(
          "Chunk compared. compared_size=%" PRIu32 ", read_unit_size=%" PRIu32,
          compared_size, read_unit_size);
    }

    if (ret && compared_size == size) {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE(
          "Buffer fully compared. size=%" PRIu32, size);
    }
  } while (0);

  if (is_opened) {
    EsfMemoryManagerResult mem_ret = EsfMemoryManagerFclose(buffer->handle);
    if (mem_ret != kEsfMemoryManagerResultSuccess) {
      ret = false;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "LHeap close failed. buffer=%p, handle=%" PRIuPTR ", mem_ret=%u",
          (void*)buffer, (uintptr_t)buffer->handle, mem_ret);
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_MEMORY_CONTROL_FAILURE);
    } else {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE("LHeap closed.");
    }
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %d", ret);
  return ret;
}
