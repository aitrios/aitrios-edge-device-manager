/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "json_handle.h"

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "json_internal.h"
#include "memory_manager.h"
#include "parson/lib/parson.h"

size_t EsfJsonSerializeSizeGet(EsfJsonHandle handle, EsfJsonValue value) {
  ESF_JSON_TRACE("entry");
  if (handle == ESF_JSON_HANDLE_INITIALIZER) {
    ESF_JSON_ERR("handle NULL.");
    ESF_JSON_TRACE("exit.");
    return 0;
  }
  JSON_Value* data = NULL;
  EsfJsonErrorCode ret = kEsfJsonInternalError;
  ret = EsfJsonValueFind(handle->json_value, value, &data);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonValueFind func failed. ret = %u, handle->json_value = %p, "
        "value = %" PRId32 "",
        ret, handle->json_value, value);
    ESF_JSON_TRACE("exit.");
    return 0;
  }
  size_t serialize_size = json_serialization_size(data);
  if (serialize_size == 0) {
    ESF_JSON_ERR("json_serialization_size func failed. data = %p", data);
    ESF_JSON_TRACE("exit.");
    return 0;
  }
  serialize_size = serialize_size - 1;
  ret = EsfJsonConvertedStringSizeGet(handle->json_value, data,
                                      &serialize_size);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonConvertedStringSizeGet func failed. ret = %u, "
        "handle->json_value = %p, ID = %" PRId32 ", serialize_size = %zu",
        ret, handle->json_value, value, serialize_size);
    ESF_JSON_TRACE("exit.");
    return 0;
  }
  ESF_JSON_DEBUG("EsfJsonSerializeSizeGet return size = %zu.", serialize_size);
  ESF_JSON_TRACE("exit.");
  return serialize_size;
}

EsfJsonErrorCode EsfJsonSerializeHandle(EsfJsonHandle handle,
                                        EsfJsonValue value,
                                        EsfMemoryManagerHandle mem_handle,
                                        size_t* serialized_size) {
  ESF_JSON_TRACE("entry");
  EsfJsonErrorCode result = kEsfJsonInternalError;
  // 1. validate parameters
  if (handle == ESF_JSON_HANDLE_INITIALIZER) {
    ESF_JSON_ERR("json handle NULL.");
    ESF_JSON_TRACE("exit.");
    return kEsfJsonHandleError;
  }
  if (serialized_size == NULL) {
    ESF_JSON_ERR("Parameter error. serialized_size = %p", serialized_size);
    ESF_JSON_TRACE("exit.");
    return kEsfJsonInvalidArgument;
  }
  // 2. check handle info
  EsfMemoryManagerHandleInfo mem_handleinfo = {kEsfMemoryManagerTargetLargeHeap,
                                               0};
  EsfMemoryManagerResult mem_ret =
      EsfMemoryManagerGetHandleInfo((uint32_t)mem_handle, &mem_handleinfo);
  if (mem_ret != kEsfMemoryManagerResultSuccess) {
    ESF_JSON_ERR(
        "Failed to get handle info for mem_handle. ret = %u, mem_handle = "
        "%" PRIu32 "",
        mem_ret, mem_handle);
    ESF_JSON_TRACE("exit.");
    return kEsfJsonInternalError;
  } else if (mem_handleinfo.target_area != kEsfMemoryManagerTargetLargeHeap) {
    ESF_JSON_ERR("Target area is not LargeHeap. TargetArea = %u",
                 (uint32_t)mem_handleinfo.target_area);
    ESF_JSON_TRACE("exit.");
    return kEsfJsonHandleError;
  }
  // 3. check MAP support
  EsfMemoryManagerMapSupport support = kEsfMemoryManagerMapIsNotSupport;
  mem_ret = EsfMemoryManagerIsMapSupport(mem_handle, &support);
  if (mem_ret != kEsfMemoryManagerResultSuccess) {
    ESF_JSON_ERR(
        "EsfMemoryManagerIsMapSupport func failed. ret = %u, mem_handle = "
        "%" PRIu32 "",
        mem_ret, mem_handle);
    ESF_JSON_TRACE("exit.");
    return kEsfJsonInternalError;
  }
  // 4. select mem_handle method
  if (support == kEsfMemoryManagerMapIsSupport) {
    // Map.
    ESF_JSON_TRACE(
        "EsfMemoryManagerMap is supported. Call "
        "EsfJsonSerializeHandleMemMap()");
    result = EsfJsonSerializeHandleMemMap(handle, value, mem_handle,
                                          serialized_size);
  } else if (support == kEsfMemoryManagerMapIsNotSupport) {
    // FileIO.
    ESF_JSON_TRACE(
        "EsfMemoryManagerMap is not supported. Call "
        "EsfJsonSerializeHandleFileIO()");
    result = EsfJsonSerializeHandleFileIO(handle, value, mem_handle,
                                          serialized_size);
  } else {
    ESF_JSON_ERR("Unexpected value.");
    ESF_JSON_TRACE("exit.");
    return kEsfJsonInternalError;
  }
  ESF_JSON_TRACE("exit.");
  return result;
}

EsfJsonErrorCode EsfJsonSerializeIncludesHandle(EsfJsonHandle handle,
                                                EsfJsonValue value,
                                                bool* is_included) {
  ESF_JSON_TRACE("entry");
  if (handle == ESF_JSON_HANDLE_INITIALIZER) {
    ESF_JSON_ERR("handle NULL.");
    ESF_JSON_TRACE("exit.");
    return kEsfJsonHandleError;
  }
  if (is_included == NULL) {
    ESF_JSON_ERR("Parameter error. is_included = %p", is_included);
    ESF_JSON_TRACE("exit.");
    return kEsfJsonInvalidArgument;
  }
  JSON_Value* data = NULL;
  EsfJsonErrorCode ret = kEsfJsonInternalError;
  ret = EsfJsonValueFind(handle->json_value, value, &data);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonValueFind func failed. ret = %u, handle->json_value = %p, "
        "value = %" PRId32 "",
        ret, handle->json_value, value);
    ESF_JSON_TRACE("exit.");
    return ret;
  }
  ret = EsfJsonCheckStringReplace(handle->json_value, data, is_included);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonCheckStringReplace func failed. ret = %u, handle->json_value "
        "= %p, data = %p",
        ret, handle->json_value, data);
    ESF_JSON_TRACE("exit.");
    return ret;
  }
  ESF_JSON_TRACE("exit.");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonStringInitHandle(EsfJsonHandle handle,
                                         EsfMemoryManagerHandle mem_handle,
                                         size_t mem_size, EsfJsonValue* value) {
  ESF_JSON_TRACE("entry");
  EsfJsonErrorCode result = kEsfJsonInternalError;
  // 1. validate parameters
  if (handle == ESF_JSON_HANDLE_INITIALIZER) {
    ESF_JSON_ERR("json handle NULL.");
    ESF_JSON_TRACE("exit.");
    return kEsfJsonHandleError;
  }
  if (value == NULL) {
    ESF_JSON_ERR("Parameter error. value = %p", value);
    ESF_JSON_TRACE("exit.");
    return kEsfJsonInvalidArgument;
  }
  if (mem_size == 0) {
    ESF_JSON_ERR("Parameter error. mem_size = %zu", mem_size);
    ESF_JSON_TRACE("exit.");
    return kEsfJsonInvalidArgument;
  }
  // 2. check handle info
  EsfMemoryManagerHandleInfo mem_handleinfo = {kEsfMemoryManagerTargetLargeHeap,
                                               0};
  EsfMemoryManagerResult mem_ret =
      EsfMemoryManagerGetHandleInfo((uint32_t)mem_handle, &mem_handleinfo);
  if (mem_ret != kEsfMemoryManagerResultSuccess) {
    ESF_JSON_ERR(
        "Failed to get handle info for mem_handle. ret = %u, mem_handle = "
        "%" PRIu32 "",
        mem_ret, mem_handle);
    ESF_JSON_TRACE("exit.");
    return kEsfJsonInternalError;
  } else if (mem_handleinfo.target_area != kEsfMemoryManagerTargetLargeHeap) {
    ESF_JSON_ERR("Target area is not LargeHeap. TargetArea = %u",
                 (uint32_t)mem_handleinfo.target_area);
    ESF_JSON_TRACE("exit.");
    return kEsfJsonHandleError;
  }
  // 3. check MAP support
  EsfMemoryManagerMapSupport support = kEsfMemoryManagerMapIsNotSupport;
  mem_ret = EsfMemoryManagerIsMapSupport(mem_handle, &support);
  if (mem_ret != kEsfMemoryManagerResultSuccess) {
    ESF_JSON_ERR(
        "EsfMemoryManagerIsMapSupport func failed. ret = %u, mem_handle = "
        "%" PRIu32 "",
        mem_ret, mem_handle);
    ESF_JSON_TRACE("exit.");
    return kEsfJsonInternalError;
  }
  // 4. select mem_handle method
  if ((support == kEsfMemoryManagerMapIsSupport) ||
      (support == kEsfMemoryManagerMapIsNotSupport)) {
    // Call the Map/FileIO common API
    ESF_JSON_TRACE(
        "Call EsfJsonStringInitHandleProcess() whether the map function is "
        "supported or not");
    result = EsfJsonStringInitHandleProcess(handle, mem_handle, mem_size,
                                            value);
  } else {
    ESF_JSON_ERR("Unexpected value.");
    ESF_JSON_TRACE("exit.");
    return kEsfJsonInternalError;
  }
  ESF_JSON_TRACE("exit.");
  return result;
}

EsfJsonErrorCode EsfJsonStringSetHandle(EsfJsonHandle handle,
                                        EsfJsonValue value,
                                        EsfMemoryManagerHandle mem_handle,
                                        size_t mem_size) {
  ESF_JSON_TRACE("entry");
  EsfJsonErrorCode result = kEsfJsonInternalError;
  // 1. validate parameters
  if (handle == ESF_JSON_HANDLE_INITIALIZER) {
    ESF_JSON_ERR("json handle NULL.");
    ESF_JSON_TRACE("exit.");
    return kEsfJsonHandleError;
  }
  if (mem_size == 0) {
    ESF_JSON_ERR("Parameter error. mem_size = %zu", mem_size);
    ESF_JSON_TRACE("exit.");
    return kEsfJsonInvalidArgument;
  }
  // 2. check handle info
  EsfMemoryManagerHandleInfo mem_handleinfo = {kEsfMemoryManagerTargetLargeHeap,
                                               0};
  EsfMemoryManagerResult mem_ret =
      EsfMemoryManagerGetHandleInfo((uint32_t)mem_handle, &mem_handleinfo);
  if (mem_ret != kEsfMemoryManagerResultSuccess) {
    ESF_JSON_ERR(
        "Failed to get handle info for mem_handle. ret = %u, mem_handle = "
        "%" PRIu32 "",
        mem_ret, mem_handle);
    ESF_JSON_TRACE("exit.");
    return kEsfJsonInternalError;
  } else if (mem_handleinfo.target_area != kEsfMemoryManagerTargetLargeHeap) {
    ESF_JSON_ERR("Target area is not LargeHeap. TargetArea = %u",
                 (uint32_t)mem_handleinfo.target_area);
    ESF_JSON_TRACE("exit.");
    return kEsfJsonHandleError;
  }
  // 3. check MAP support
  EsfMemoryManagerMapSupport support = kEsfMemoryManagerMapIsNotSupport;
  mem_ret = EsfMemoryManagerIsMapSupport(mem_handle, &support);
  if (mem_ret != kEsfMemoryManagerResultSuccess) {
    ESF_JSON_ERR(
        "EsfMemoryManagerIsMapSupport func failed. ret = %u, mem_handle = "
        "%" PRIu32 "",
        mem_ret, mem_handle);
    ESF_JSON_TRACE("exit.");
    return kEsfJsonInternalError;
  }
  // 4. select mem_handle method
  if ((support == kEsfMemoryManagerMapIsSupport) ||
      (support == kEsfMemoryManagerMapIsNotSupport)) {
    // Call the Map/FileIO common API
    ESF_JSON_TRACE(
        "Call EsfJsonStringSetHandleProcess() whether the map function is "
        "supported or not");
    result = EsfJsonStringSetHandleProcess(handle, value, mem_handle, mem_size);
  } else {
    ESF_JSON_ERR("Unexpected value.");
    ESF_JSON_TRACE("exit.");
    return kEsfJsonInternalError;
  }
  ESF_JSON_TRACE("exit.");
  return result;
}
