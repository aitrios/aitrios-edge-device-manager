/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "json_fileio.h"

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "json_internal.h"
#include "memory_manager.h"
#include "parson/lib/parson.h"

// Mutex object for FileIO access state.
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// In the case of EsfJsonStringInitFileIO, overwrite MemInfo to the element of
// ESF_JSON_VALUE_INVALID, and in the case of EsfJsonStringSetFileIO, overwrite
// MemInfo to the element of the same JSON Value.
// If neither of the above applies and there is no space for the element, return
// CONFIG_EXTERNAL_CODEC_JSON_MEM_HANDLE_MAX.
JSON_STATIC size_t EsfJsonFindEmptyMemInfo(EsfJsonValueContainer* container,
                                           EsfJsonValue value) {
  size_t i = 0;
  if (container == NULL) {
    return CONFIG_EXTERNAL_CODEC_JSON_MEM_HANDLE_MAX;
  }
  for (i = 0; i < CONFIG_EXTERNAL_CODEC_JSON_MEM_HANDLE_MAX; ++i) {
    if (container->mem_info[i].id == value) {
      break;
    }
  }
  return i;
}

EsfJsonErrorCode EsfJsonSerializeFileIO(EsfJsonHandle handle,
                                        EsfJsonValue value,
                                        EsfMemoryManagerHandle mem_handle,
                                        size_t* mem_size) {
  ESF_JSON_TRACE("entry");
  if (handle == ESF_JSON_HANDLE_INITIALIZER) {
    ESF_JSON_ERR("handle NULL.");
    return kEsfJsonHandleError;
  }
  if (mem_size == NULL) {
    ESF_JSON_ERR("Parameter error. mem_size = %p", mem_size);
    return kEsfJsonInvalidArgument;
  }
  EsfMemoryManagerMapSupport support = kEsfMemoryManagerMapIsNotSupport;
  EsfMemoryManagerResult mem_ret = EsfMemoryManagerIsMapSupport(mem_handle,
                                                                &support);
  if (mem_ret != kEsfMemoryManagerResultSuccess) {
    ESF_JSON_ERR(
        "EsfMemoryManagerIsMapSupport func failed. ret = %u, mem_handle = "
        "%" PRIu32 "",
        mem_ret, mem_handle);
    return kEsfJsonInternalError;
  }
  if (support == kEsfMemoryManagerMapIsSupport) {
    ESF_JSON_ERR("This API is unsupported");
    return kEsfJsonInternalError;
  } else if (support == kEsfMemoryManagerMapIsNotSupport) {
    // Do nothing.
  } else {
    ESF_JSON_ERR("Unexpected value.");
    return kEsfJsonInternalError;
  }
  EsfJsonErrorCode ret = kEsfJsonInternalError;
  ret = EsfJsonMutexLock(&mutex);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR("EsfJsonMutexLock func failed. ret = %u", ret);
    return ret;
  }
  EsfJsonErrorCode result = kEsfJsonSuccess;
  char* serialized_str = NULL;
  do {
    JSON_Value* data = NULL;
    result = EsfJsonValueFind(handle->json_value, value, &data);
    if (result != kEsfJsonSuccess) {
      ESF_JSON_ERR(
          "EsfJsonValueFind func failed. ret = %u, handle->json_value = %p, "
          "value = %" PRId32 "",
          result, handle->json_value, value);
      break;
    }
    serialized_str = json_serialize_to_string(data);
    if (serialized_str == NULL) {
      ESF_JSON_ERR("json_serialize_to_string func failed.");
      result = kEsfJsonOutOfMemory;
      break;
    }

    bool is_included = false;
    result = EsfJsonSerializeIncludesFileIO(handle, value, &is_included);
    if (result != kEsfJsonSuccess) {
      json_free_serialized_string(serialized_str);
      ESF_JSON_ERR(
          "EsfJsonSerializeIncludesFileIO func failed. ret = %u, handle = %p, "
          "value = %" PRId32 "",
          result, handle, value);
      break;
    }
    result = EsfJsonSerializeUsingMemoryManagerFileIO(
        handle->json_value, mem_handle, false, serialized_str, is_included,
        mem_size);
    if (result != kEsfJsonSuccess) {
      ESF_JSON_ERR(
          "EsfJsonSerializeUsingMemoryManager func failed. ret = %u, "
          "handle->json_value = %p, mem_handle = %" PRIu32
          "mem_handle_open = %d, serialized_str = %p, is_included = %d",
          result, handle->json_value, mem_handle, false, serialized_str,
          is_included);
      json_free_serialized_string(serialized_str);
      break;
    }
  } while (0);

  ret = EsfJsonMutexUnlock(&mutex);
  if (ret != kEsfJsonSuccess) {
    json_free_serialized_string(serialized_str);
    ESF_JSON_ERR("EsfJsonMutexUnlock func failed. ret = %u", ret);
    return ret;
  }
  if (result != kEsfJsonSuccess) {
    return result;
  }
  json_free_serialized_string(serialized_str);
  ESF_JSON_DEBUG("JSON Value serialize FileIO ID = %" PRId32
                 ", mem_handle = %" PRIu32 ", mem_size = %zu.",
                 value, mem_handle, *mem_size);
  ESF_JSON_TRACE("exit.");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonSerializeIncludesFileIO(EsfJsonHandle handle,
                                                EsfJsonValue value,
                                                bool* is_included) {
  ESF_JSON_TRACE("entry");
  if (handle == ESF_JSON_HANDLE_INITIALIZER) {
    ESF_JSON_ERR("handle NULL.");
    return kEsfJsonHandleError;
  }
  if (is_included == NULL) {
    ESF_JSON_ERR("Parameter error. is_included = %p", is_included);
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
    return ret;
  }
  ret = EsfJsonCheckStringReplace(handle->json_value, data, is_included);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonCheckStringReplace func failed. ret = %u, handle->json_value "
        "= %p, data = %p",
        ret, handle->json_value, data);
    return ret;
  }
  ESF_JSON_TRACE("exit.");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonStringInitFileIO(EsfJsonHandle handle,
                                         EsfMemoryManagerHandle mem_handle,
                                         size_t mem_size, EsfJsonValue* value) {
  ESF_JSON_TRACE("entry");
  if (handle == ESF_JSON_HANDLE_INITIALIZER) {
    ESF_JSON_ERR("handle NULL.");
    return kEsfJsonHandleError;
  }
  if (value == NULL) {
    ESF_JSON_ERR("Parameter error. value = %p", value);
    return kEsfJsonInvalidArgument;
  }
  size_t empty_mem_index = EsfJsonFindEmptyMemInfo(handle->json_value,
                                                   ESF_JSON_VALUE_INVALID);
  if (empty_mem_index == CONFIG_EXTERNAL_CODEC_JSON_MEM_HANDLE_MAX) {
    ESF_JSON_ERR(
        "There is no free space in the File I/O retention area of the handle.");
    return kEsfJsonValueLimit;
  }
  EsfMemoryManagerMapSupport support = kEsfMemoryManagerMapIsNotSupport;
  EsfMemoryManagerResult result = EsfMemoryManagerIsMapSupport(mem_handle,
                                                               &support);
  if (result != kEsfMemoryManagerResultSuccess) {
    ESF_JSON_ERR(
        "EsfMemoryManagerIsMapSupport func failed. ret = %u, mem_handle = "
        "%" PRIu32 "",
        result, mem_handle);
    return kEsfJsonInternalError;
  }
  if (support == kEsfMemoryManagerMapIsSupport) {
    ESF_JSON_ERR("This API is unsupported");
    return kEsfJsonInternalError;
  } else if (support == kEsfMemoryManagerMapIsNotSupport) {
    // Do nothing.
  } else {
    ESF_JSON_ERR("Unexpected value.");
    return kEsfJsonInternalError;
  }
  off_t current_offset = 0;
  result = EsfMemoryManagerFseek(mem_handle, 0, SEEK_CUR, &current_offset);
  if (result != kEsfMemoryManagerResultSuccess) {
    ESF_JSON_ERR(
        "EsfMemoryManagerFseek func failed. ret = %u, handle = %" PRIu32
        ", res_offset = %jd",
        result, mem_handle, (intmax_t)current_offset);
    return kEsfJsonInvalidArgument;
  }
  char tmp_string[REPLACEMENT_STRING_MAX_SIZE] = REPLACEMENT_STRING "tmp";
  JSON_Value* tmp_data = NULL;
  tmp_data = json_value_init_string(tmp_string);
  if (tmp_data == NULL) {
    ESF_JSON_ERR("json_value_init_string func failed.");
    return kEsfJsonOutOfMemory;
  }
  EsfJsonValue value_id = ESF_JSON_VALUE_INVALID;
  EsfJsonErrorCode ret = kEsfJsonInternalError;
  bool remove_flag = false;
  do {
    ret = EsfJsonValueAdd(handle->json_value, tmp_data, &value_id);
    if (ret != kEsfJsonSuccess) {
      ESF_JSON_ERR(
          "EsfJsonValueAdd func failed. ret = %u, handle->json_value = %p, "
          "tmp_data = %p",
          ret, handle->json_value, tmp_data);
      break;
    }
    char replace_string[REPLACEMENT_STRING_MAX_SIZE] = REPLACEMENT_STRING;
    snprintf(replace_string, sizeof(replace_string),
             REPLACEMENT_STRING "%" PRId32 "", value_id);
    JSON_Value* data = NULL;
    data = json_value_init_string(replace_string);
    if (data == NULL) {
      ESF_JSON_ERR("json_value_init_string func failed.");
      remove_flag = true;
      ret = kEsfJsonOutOfMemory;
      break;
    }
    ret = EsfJsonValueReplace(handle->json_value, value_id, data);
    if (ret != kEsfJsonSuccess) {
      ESF_JSON_ERR(
          "EsfJsonValueReplace func failed. ret = %u, json_value = %p ID = "
          "%" PRId32 ", data = %p",
          ret, handle->json_value, value_id, data);
      remove_flag = true;
      json_value_free(data);
      break;
    }
  } while (0);
  if (remove_flag == true) {
    EsfJsonErrorCode ret_val = EsfJsonValueLookupRemove(handle->json_value,
                                                        tmp_data);
    if (ret_val != kEsfJsonSuccess) {
      ESF_JSON_ERR(
          "EsfJsonValueLookupRemove func failed. ret = %u, json_value = %p "
          "tmp_data = %p",
          ret_val, handle->json_value, tmp_data);
      ret = ret_val;
    }
  }
  json_value_free(tmp_data);

  if (ret == kEsfJsonSuccess) {
    handle->json_value->mem_info[empty_mem_index].id = value_id;
    handle->json_value->mem_info[empty_mem_index].data = mem_handle;
    handle->json_value->mem_info[empty_mem_index].size = mem_size;
    handle->json_value->mem_info[empty_mem_index].offset = current_offset;
    *value = value_id;
    ESF_JSON_DEBUG("JSON String Init FileIO Info ID = %" PRId32 "", value_id);
  }

  ESF_JSON_TRACE("exit.");
  return ret;
}

EsfJsonErrorCode EsfJsonStringSetFileIO(EsfJsonHandle handle,
                                        EsfJsonValue value,
                                        EsfMemoryManagerHandle mem_handle,
                                        size_t mem_size) {
  ESF_JSON_TRACE("entry");
  if (handle == ESF_JSON_HANDLE_INITIALIZER) {
    ESF_JSON_ERR("handle NULL.");
    return kEsfJsonHandleError;
  }

  EsfMemoryManagerMapSupport support = kEsfMemoryManagerMapIsNotSupport;
  EsfMemoryManagerResult result = EsfMemoryManagerIsMapSupport(mem_handle,
                                                               &support);
  if (result != kEsfMemoryManagerResultSuccess) {
    ESF_JSON_ERR(
        "EsfMemoryManagerIsMapSupport func failed. ret = %u, mem_handle = "
        "%" PRIu32 "",
        result, mem_handle);
    return kEsfJsonInternalError;
  }
  if (support == kEsfMemoryManagerMapIsSupport) {
    ESF_JSON_ERR("This API is unsupported");
    return kEsfJsonInternalError;
  } else if (support == kEsfMemoryManagerMapIsNotSupport) {
    // Do nothing.
  } else {
    ESF_JSON_ERR("Unexpected value.");
    return kEsfJsonInternalError;
  }

  off_t current_offset = 0;
  result = EsfMemoryManagerFseek(mem_handle, 0, SEEK_CUR, &current_offset);
  if (result != kEsfMemoryManagerResultSuccess) {
    ESF_JSON_ERR(
        "EsfMemoryManagerFseek func failed. ret = %u, handle = %" PRIu32
        ", res_offset = %jd",
        result, mem_handle, (intmax_t)current_offset);
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
    return ret;
  }

  size_t empty_mem_index = EsfJsonFindEmptyMemInfo(handle->json_value, value);
  if (empty_mem_index == CONFIG_EXTERNAL_CODEC_JSON_MEM_HANDLE_MAX) {
    ESF_JSON_ERR(
        "There is no free space in the File I/O retention area of the handle.");
    return kEsfJsonValueLimit;
  }

  ret = EsfJsonStringSetHandleInternal(handle->json_value, value, data);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonStringSetHandleInternal func failed. ret = %u, "
        "handle->json_value = %p, value = %" PRId32 ", data = %p.",
        ret, handle->json_value, value, data);
    return ret;
  }
  handle->json_value->mem_info[empty_mem_index].id = value;
  handle->json_value->mem_info[empty_mem_index].data = mem_handle;
  handle->json_value->mem_info[empty_mem_index].size = mem_size;
  handle->json_value->mem_info[empty_mem_index].offset = current_offset;

  ESF_JSON_DEBUG("JSON String Set FileIO Info ID = %" PRId32 "", value);
  ESF_JSON_TRACE("exit.");
  return kEsfJsonSuccess;
}
