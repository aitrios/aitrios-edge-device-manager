/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "json_internal.h"

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include "json.h"
#include "json_fileio.h"
#include "json_handle.h"
#include "parson/lib/parson.h"

// """Delete JSON object recursively.

// Args:
//    json_value (EsfJsonValueContainer*): JSON value array info.
//    data (JSON_Value*): JSON value to set.
//      information. NULL is not acceptable.

// Returns:
//    kEsfJsonSuccess: Normal termination.
//    kEsfJsonInvalidArgument: Arg parameter error.
//    kEsfJsonInternalError:  Internal error.
static EsfJsonErrorCode EsfJsonObjectRecursiveRemove(
    EsfJsonValueContainer* json_value, JSON_Value* data);

// """Delete JSON Array recursively.

// Args:
//    json_value (EsfJsonValueContainer*): JSON value array info.
//    data (JSON_Value*): JSON value to set.
//      information. NULL is not acceptable.

// Returns:
//    kEsfJsonSuccess: Normal termination.
//    kEsfJsonInvalidArgument: Arg parameter error.
//    kEsfJsonInternalError:  Internal error.
JSON_STATIC EsfJsonErrorCode EsfJsonArrayRecursiveRemove(
    EsfJsonValueContainer* json_value, JSON_Value* data);

// """Set the specified JSON value to JSON object.

// Args:
//    parent_data (JSON_Value*): Parent JSON value.
//      information. NULL is not acceptable.
//    data (JSON_Value*): JSON value.
//      information. NULL is not acceptable.
//    set_data (JSON_Value*): JSON value to set.
//      information. NULL is not acceptable.

// Returns:
//    kEsfJsonSuccess: Normal termination.
//    kEsfJsonInvalidArgument: Arg parameter error.
//    kEsfJsonInternalError:  Internal error.
JSON_STATIC EsfJsonErrorCode
EsfJsonValueObjectSet(const JSON_Value* parent_data, const JSON_Value* data,
                      JSON_Value* set_data);

// """Set the specified JSON value to JSON Array.

// Args:
//    json_value (EsfJsonValueContainer*): JSON value array info.
//    data (JSON_Value*): JSON value to set.
//      information. NULL is not acceptable.

// Returns:
//    kEsfJsonSuccess: Normal termination.
//    kEsfJsonInvalidArgument: Arg parameter error.
//    kEsfJsonInternalError:  Internal error.
JSON_STATIC EsfJsonErrorCode EsfJsonValueArraySet(const JSON_Value* parent_data,
                                                  const JSON_Value* data,
                                                  JSON_Value* set_data);

// """Obtains the number of characters in the replacement string in the
// specified JSON Object when converted to the replacement string.

// Args:
//    json_value (EsfJsonValueContainer*): JSON value array info.
//      information. NULL is not acceptable.
//    object_data (JSON_Object*): JSON Object.
//      information. NULL is not acceptable.
//    str_size (size_t*): string size.
//      information. NULL is not acceptable.

// Returns:
//    kEsfJsonSuccess: Normal termination.
//    kEsfJsonInvalidArgument: Arg parameter error.
//    kEsfJsonInternalError:  Internal error.
//    kEsfJsonIndexExceed: Excess of elements .
JSON_STATIC EsfJsonErrorCode
EsfJsonObjectConvertedStringSizeGet(EsfJsonValueContainer* json_value,
                                    JSON_Object* object_data, size_t* str_size);

// """Obtains the number of characters in the replacement string in the
// specified JSON Array when converted to the replacement string.

// Args:
//    json_value (EsfJsonValueContainer*): JSON value array info.
//      information. NULL is not acceptable.
//    array_data (JSON_Array*): JSON Array.
//      information. NULL is not acceptable.
//    str_size (size_t*): string size.
//      information. NULL is not acceptable.

// Returns:
//    kEsfJsonSuccess: Normal termination.
//    kEsfJsonInvalidArgument: Arg parameter error.
//    kEsfJsonInternalError:  Internal error.
//    kEsfJsonIndexExceed: Excess of elements .
JSON_STATIC EsfJsonErrorCode
EsfJsonArrayConvertedStringSizeGet(EsfJsonValueContainer* json_value,
                                   JSON_Array* array_data, size_t* str_size);

// """Get the number of characters in the substitution string when the
// substitution string is converted to the replaced string.

// Args:
//    json_value (EsfJsonValueContainer*): JSON value array info.
//      information. NULL is not acceptable.
//    replacement_value (EsfJsonValue): Replacement ID.
//    data (JSON_Value*): JSON value to set.
//      information. NULL is not acceptable.

// Returns:
//    kEsfJsonSuccess: Normal termination.
//    kEsfJsonInvalidArgument: Arg parameter error.
//    kEsfJsonInternalError:  Internal error.
JSON_STATIC EsfJsonErrorCode EsfJsonReplacementStringSizeGet(
    EsfJsonValueContainer* json_value, EsfJsonValue replacement_value,
    size_t* str_size);

// """Recursively determines if the specified JSON Object has a String Value
// associated with a Memory Manager handle that supports File I/O.

// Args:
//    json_value (EsfJsonValueContainer*): JSON value array info.
//      information. NULL is not acceptable.
//    data (JSON_Value*): JSON value to set.
//      information. NULL is not acceptable.
//    is_included (bool*): Judgment Result.
//      information. NULL is not acceptable.

// Returns:
//    kEsfJsonSuccess: Normal termination.
//    kEsfJsonInvalidArgument: Arg parameter error.
//    kEsfJsonInternalError:  Internal error.
JSON_STATIC EsfJsonErrorCode EsfJsonObjectRecursiveCheck(
    EsfJsonValueContainer* json_value, JSON_Value* data, bool* is_included);

// """Recursively determines if the specified JSON Array has a String Value
// associated with a Memory Manager handle that supports File I/O.

// Args:
//    json_value (EsfJsonValueContainer*): JSON value array info.
//      information. NULL is not acceptable.
//    data (JSON_Value*): JSON value to set.
//      information. NULL is not acceptable.
//    is_included (bool*): Judgment Result.
//      information. NULL is not acceptable.

// Returns:
//    kEsfJsonSuccess: Normal termination.
//    kEsfJsonInvalidArgument: Arg parameter error.
//    kEsfJsonInternalError:  Internal error.
JSON_STATIC EsfJsonErrorCode EsfJsonArrayRecursiveCheck(
    EsfJsonValueContainer* json_value, JSON_Value* data, bool* is_included);

// """Serializes a JSON value container to a string using file I/O operations.
// This function takes a JSON value container and serializes it into a string,
// utilizing file I/O operations for conversion. It writes the serialized data
// to a memory manager handle and updates the size of written data.

// Args:
//    json_value Pointer to the EsfJsonValueContainer that holds the JSON data
//      to be serialized.
//    mem_handle Handle to the memory manager where the serialized data will be
//      written.
//    mem_handle_open memory manager handle open request (true:open request)
//    serialized_str Pointer to a character pointer where the serialized string
//      will be stored.
//    wsize Pointer to a size_t variable where the size of written data will be
//      updated.
//    serialize_size Pointer to a size_t variable where the total size of
//      serialized data will be accumulated.

// Returns:
// EsfJsonErrorCode indicating the success or failure of the operation.
//    kEsfJsonSuccess: Serialization was successful.
//    kEsfJsonInvalidArgument: One or more input parameters are NULL.
//    kEsfJsonOutOfMemory: Memory allocation failed.
//    kEsfJsonInternalError: An internal error occurred during serialization.
JSON_STATIC EsfJsonErrorCode EsfJsonSerializeConversionStringFileIO(
    EsfJsonValueContainer* json_value, EsfMemoryManagerHandle mem_handle,
    bool mem_handle_open, char** serialized_str, size_t* wsize,
    size_t* serialize_size);

// """Serializes a JSON value into a string with memory mapping.
// This function serializes a given JSON value container into a string while
// managing memory through a specified memory manager handle. It processes
// each memory information entry within the JSON value, converting it to a
// string and mapping the corresponding memory data. The serialized string is
// updated accordingly, and the memory data is copied to the provided memory
//  buffer.

// Args:
//    json_value Pointer to the EsfJsonValueContainer that holds the JSON data
//      to be serialized.
//    mem_handle Handle for the memory manager used for mapping operations.
//    mem_data Pointer to the memory buffer where serialized data will be
//      stored.
//    serialized_str Pointer to the string where the serialized JSON will be
//      stored.
//    wsize Pointer to a size_t variable that will store the written size of the
//      current operation. serialize_size Pointer to a size_t variable that
//      accumulates the total serialized size.

// Returns:
// EsfJsonErrorCode indicating the success or failure of the operation.
//    kEsfJsonSuccess : Serialization was successful.
//    kEsfJsonInvalidArgument : One or more input parameters are invalid.
//    kEsfJsonInternalError : An internal error occurred during serialization.
JSON_STATIC EsfJsonErrorCode EsfJsonSerializeConversionStringMemMap(
    EsfJsonValueContainer* json_value, EsfMemoryManagerHandle mem_handle,
    void** mem_data, char** serialized_str, size_t* wsize,
    size_t* serialize_size);

// """Finds the index of an empty memory handle in the JSON value container.
// This function searches through the `mem_info` array within the provided
// `EsfJsonValueContainer` to find the index where the `id` matches the given
// `EsfJsonValue`. If the container is NULL, it returns a predefined maximum
// value indicating that no valid index was found.

// Args:
//    container A pointer to the `EsfJsonValueContainer` structure which
//      contains the memory information array to be searched.
//    value The `EsfJsonValue` to be matched against the `id` field in the
//      `mem_info` array.

// Returns:
//  Returns the index of the matching `id` in the `mem_info` array.
//  If the container is NULL or if no match is found, it returns
//  `CONFIG_EXTERNAL_CODEC_JSON_MEM_HANDLE_MAX`.
JSON_STATIC size_t EsfJsonFindEmptyMemHandleInfo(
    EsfJsonValueContainer* container, EsfJsonValue value);

// EsfJsonValueContainer default value.
static const EsfJsonValueContainer kEsfJsonValueContainerDefault = {
    .data = NULL,
    .capacity = 16,
    .size = 0,
    .last_id = ESF_JSON_VALUE_MAX,
    .mem_info = {{0}},
};

static const EsfJsonMemoryInfo kEsfJsonMemoryDefault = {
    .id = ESF_JSON_VALUE_INVALID,
    .data = 0,
    .size = 0,
    .offset = 0,
};

// Mutex object for MemoryManager access state.
JSON_STATIC pthread_mutex_t mem_mutex = PTHREAD_MUTEX_INITIALIZER;

EsfJsonErrorCode EsfJsonValueFind(EsfJsonValueContainer* json_value,
                                  EsfJsonValue value, JSON_Value** data) {
  ESF_JSON_TRACE("entry");
  // Parameter check.
  if (json_value == NULL || data == NULL) {
    ESF_JSON_ERR("Parameter error. json_value = %p, data = %p", json_value,
                 data);
    return kEsfJsonInvalidArgument;
  }

  int32_t i = 0;
  // JSON value ID search
  for (i = 0; i < json_value->size; ++i) {
    if (json_value->data[i].id == value) {
      *data = json_value->data[i].data;
      ESF_JSON_TRACE("JSON Value found is %p.", json_value->data[i].data);
      ESF_JSON_TRACE("exit");
      return kEsfJsonSuccess;
    }
  }

  // Not found.
  ESF_JSON_ERR("JSON value not found.");
  return kEsfJsonValueNotFound;
}

EsfJsonErrorCode EsfJsonValueLookup(EsfJsonValueContainer* json_value,
                                    const JSON_Value* data,
                                    EsfJsonValue* value) {
  ESF_JSON_TRACE("entry");
  // Parameter check.
  if (json_value == NULL || data == NULL || value == NULL) {
    ESF_JSON_ERR("Parameter error. json_value = %p, data = %p, value = %p",
                 json_value, data, value);
    return kEsfJsonInvalidArgument;
  }

  int32_t i = 0;
  // JSON value search.
  for (i = 0; i < json_value->size; ++i) {
    if (json_value->data[i].data == data) {
      *value = json_value->data[i].id;
      ESF_JSON_TRACE("JSON Value ID found is %d.", json_value->data[i].id);
      ESF_JSON_TRACE("exit");
      return kEsfJsonSuccess;
    }
  }

  // Not found.
  ESF_JSON_ERR("JSON value ID not found.");
  return kEsfJsonValueNotFound;
}

EsfJsonErrorCode EsfJsonValueLookupRemove(EsfJsonValueContainer* json_value,
                                          const JSON_Value* data) {
  ESF_JSON_TRACE("entry");
  // Parameter check.
  if (json_value == NULL || data == NULL) {
    ESF_JSON_ERR("Parameter error. json_value = %p, data = %p", json_value,
                 data);
    return kEsfJsonInvalidArgument;
  }

  int32_t i = 0;
  // JSON value search and remove.
  for (i = 0; i < json_value->size; ++i) {
    if (json_value->data[i].data == data) {
      ESF_JSON_TRACE("Deleted JSON Value ID = %d, data = %p",
                     json_value->data[i].id, json_value->data[i].data);
      for (int j = 0; j < CONFIG_EXTERNAL_CODEC_JSON_MEM_HANDLE_MAX; ++j) {
        if (json_value->mem_info[j].id == json_value->data[i].id) {
          json_value->mem_info[j].id = ESF_JSON_VALUE_INVALID;
          break;
        }
      }
      json_value->data[i].id = ESF_JSON_VALUE_INVALID;
      json_value->data[i].data = NULL;
      ESF_JSON_TRACE("exit");
      return kEsfJsonSuccess;
    }
  }

  // Not found.
  ESF_JSON_TRACE("exit");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonValueAdd(EsfJsonValueContainer* json_value,
                                 JSON_Value* data, EsfJsonValue* value) {
  ESF_JSON_TRACE("entry");
  // Parameter check.
  if (json_value == NULL || data == NULL || value == NULL) {
    ESF_JSON_ERR("Parameter error. json_value = %p, data = %p, value = %p",
                 json_value, data, value);
    return kEsfJsonInvalidArgument;
  }

  // Size check.
  if (json_value->size + 1 > ESF_JSON_VALUE_MAX) {
    ESF_JSON_ERR("ID has exceeded the maximum value.");
    return kEsfJsonValueLimit;
  }

  EsfJsonValueData* realloc_data = NULL;
  // JSON value array check.
  if (json_value->size + 1 > json_value->capacity) {
    if (json_value->capacity >= ESF_JSON_VALUE_MAX) {
      ESF_JSON_ERR("ID has exceeded the maximum value.");

      return kEsfJsonValueLimit;
    }
    int32_t tmp_capacity = json_value->capacity * 2;
    if (tmp_capacity > ESF_JSON_VALUE_MAX) {
      tmp_capacity = ESF_JSON_VALUE_MAX;
    }
    realloc_data = (EsfJsonValueData*)realloc(
        json_value->data, (sizeof(*realloc_data) * tmp_capacity));
    if (realloc_data == NULL) {
      ESF_JSON_ERR("Failed to allocate memory for realloc_data.");
      return kEsfJsonOutOfMemory;
    } else {
      json_value->capacity = tmp_capacity;
      json_value->data = realloc_data;
    }
  }

  // JSON data add.
  ++(json_value->size);
  json_value->last_id = json_value->size;
  int32_t offset = (json_value->size) - 1;
  json_value->data[offset].data = data;
  json_value->data[offset].id = json_value->last_id;
  *value = json_value->data[offset].id;
  ESF_JSON_TRACE("Add JSON Value ID = %d", json_value->data[offset].id);

  ESF_JSON_TRACE("exit");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonValueReplace(EsfJsonValueContainer* json_value,
                                     EsfJsonValue value, JSON_Value* data) {
  ESF_JSON_TRACE("entry");
  // Parameter check.
  if (json_value == NULL || data == NULL) {
    ESF_JSON_ERR("Parameter error. json_value = %p, data = %p", json_value,
                 data);
    return kEsfJsonInvalidArgument;
  }

  int32_t i = 0;
  // JSON value ID search and Replace.
  for (i = 0; i < json_value->size; ++i) {
    if (json_value->data[i].id == value) {
      for (int j = 0; j < CONFIG_EXTERNAL_CODEC_JSON_MEM_HANDLE_MAX; ++j) {
        if (json_value->mem_info[j].id == json_value->data[i].id) {
          json_value->mem_info[j].id = ESF_JSON_VALUE_INVALID;
          break;
        }
      }
      json_value->data[i].data = data;
      ESF_JSON_TRACE("exit");
      return kEsfJsonSuccess;
    }
  }

  // Not found.
  ESF_JSON_ERR("JSON value ID not found.");
  return kEsfJsonValueNotFound;
}

EsfJsonErrorCode EsfJsonValueContainerInit(EsfJsonValueContainer** json_value) {
  ESF_JSON_TRACE("entry");
  // Parameter check.
  if (json_value == NULL) {
    ESF_JSON_ERR("Parameter error. json_value = %p", json_value);
    return kEsfJsonInvalidArgument;
  }

  EsfJsonValueContainer* tmp_esf_json_value_container = NULL;
  tmp_esf_json_value_container =
      (EsfJsonValueContainer*)malloc(sizeof(*tmp_esf_json_value_container));
  if (tmp_esf_json_value_container == NULL) {
    ESF_JSON_ERR("Failed to allocate memory for tmp_esf_json_value_container.");
    return kEsfJsonOutOfMemory;
  }

  // Initialize
  *tmp_esf_json_value_container = kEsfJsonValueContainerDefault;
  for (int i = 0; i < CONFIG_EXTERNAL_CODEC_JSON_MEM_HANDLE_MAX; ++i) {
    tmp_esf_json_value_container->mem_info[i].id = ESF_JSON_VALUE_INVALID;
    tmp_esf_json_value_container->mem_info[i].data = 0;
    tmp_esf_json_value_container->mem_info[i].offset = 0;
    tmp_esf_json_value_container->mem_info[i].size = 0;
  }

  EsfJsonValueData* tmp_esf_json_value_data = NULL;
  tmp_esf_json_value_data =
      (EsfJsonValueData*)malloc((sizeof(*tmp_esf_json_value_data) *
                                 (kEsfJsonValueContainerDefault.capacity)));
  if (tmp_esf_json_value_data == NULL) {
    free(tmp_esf_json_value_container);
    ESF_JSON_ERR("Failed to allocate memory for tmp_esf_json_value_data.");
    return kEsfJsonOutOfMemory;
  }

  // Initialize
  int32_t j = 0;
  for (j = 0; j < tmp_esf_json_value_container->capacity; ++j) {
    tmp_esf_json_value_data[j].id = ESF_JSON_VALUE_INVALID;
    tmp_esf_json_value_data[j].data = NULL;
  }

  tmp_esf_json_value_container->data = tmp_esf_json_value_data;
  *json_value = tmp_esf_json_value_container;

  ESF_JSON_TRACE("exit");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonValueContainerFree(EsfJsonValueContainer* json_value) {
  ESF_JSON_TRACE("entry");
  // Parameter check.
  if (json_value == NULL) {
    ESF_JSON_ERR("Parameter error. json_value = %p", json_value);
    return kEsfJsonInvalidArgument;
  }

  int32_t i = 0;
  // Delete valid data.
  for (i = 0; i < json_value->size; ++i) {
    if (json_value->data[i].data == NULL &&
        json_value->data[i].id == ESF_JSON_VALUE_INVALID) {
      // Go to the next loop without doing anything.
      continue;
    } else {
      JSON_Value* parent_data = NULL;
      parent_data = json_value_get_parent(json_value->data[i].data);
      if (parent_data != NULL) {
        continue;
      } else {
        EsfJsonErrorCode ret = kEsfJsonInternalError;
        // Recursive deletion JSON value.
        ret = EsfJsonValueRecursiveRemove(json_value, json_value->data[i].data);
        if (ret != kEsfJsonSuccess) {
          ESF_JSON_ERR(
              "EsfJsonValueRecursiveRemove func failed. ret = %u, json_value "
              "= "
              "%p, json_value->data[i].data = %p.",
              ret, json_value, json_value->data[i].data);
          return kEsfJsonInternalError;
        }

        // Remove data saving
        JSON_Value* remove_data = json_value->data[i].data;
        ret = EsfJsonValueLookupRemove(json_value, json_value->data[i].data);
        if (ret != kEsfJsonSuccess) {
          ESF_JSON_ERR(
              "EsfJsonValueLookupRemove func failed. ret = %u, json_value = "
              "%p, json_value->data[i].data = %p.",
              ret, json_value, json_value->data[i].data);
          return kEsfJsonInternalError;
        }
        json_value_free(remove_data);
      }
    }
  }
  free(json_value->data);
  free(json_value);
  ESF_JSON_TRACE("exit");
  return kEsfJsonSuccess;
}

EsfJsonValueType EsfJsonValueTypeConvert(JSON_Value_Type json_type) {
  ESF_JSON_TRACE("entry");
  switch (json_type) {
    case JSONNull:
      ESF_JSON_TRACE("exit");
      return kEsfJsonValueTypeNull;

    case JSONString:
      ESF_JSON_TRACE("exit");
      return kEsfJsonValueTypeString;

    case JSONNumber:
      ESF_JSON_TRACE("exit");
      return kEsfJsonValueTypeNumber;

    case JSONObject:
      ESF_JSON_TRACE("exit");
      return kEsfJsonValueTypeObject;

    case JSONArray:
      ESF_JSON_TRACE("exit");
      return kEsfJsonValueTypeArray;

    case JSONBoolean:
      ESF_JSON_TRACE("exit");
      return kEsfJsonValueTypeBoolean;

    default:
      // This block is not executed.
      ESF_JSON_TRACE("exit");
      return kEsfJsonValueTypeObject;
  }
}

EsfJsonErrorCode EsfJsonValueRecursiveRemove(EsfJsonValueContainer* json_value,
                                             JSON_Value* data) {
  ESF_JSON_TRACE("entry");
  // Parameter check.
  if (json_value == NULL || data == NULL) {
    ESF_JSON_ERR("Parameter error. json_value = %p, data = %p", json_value,
                 data);
    return kEsfJsonInvalidArgument;
  }

  JSON_Value_Type type = JSONError;
  // Deleting child data.
  type = json_value_get_type(data);
  if (type == JSONError) {
    ESF_JSON_ERR("json_value_get_type func failed. data = %p.", data);
    return kEsfJsonInternalError;
  } else if (type == JSONObject) {
    return EsfJsonObjectRecursiveRemove(json_value, data);
  } else if (type == JSONArray) {
    return EsfJsonArrayRecursiveRemove(json_value, data);
  } else {
    ESF_JSON_TRACE("exit");
    return kEsfJsonSuccess;
  }
}

JSON_STATIC EsfJsonErrorCode EsfJsonObjectRecursiveRemove(
    EsfJsonValueContainer* json_value, JSON_Value* data) {
  ESF_JSON_TRACE("entry");
  // Parameter check.
  if (json_value == NULL || data == NULL) {
    ESF_JSON_ERR("Parameter error. json_value = %p, data = %p", json_value,
                 data);
    return kEsfJsonInvalidArgument;
  }

  JSON_Object* object_data = NULL;
  object_data = json_value_get_object(data);
  if (object_data == NULL) {
    ESF_JSON_ERR("json_value_get_object func failed. data = %p.", data);
    return kEsfJsonInvalidArgument;
  }

  size_t size = 0;
  size = json_object_get_count(object_data);

  size_t i = 0;
  for (i = 0; i < size; ++i) {
    JSON_Value* tmp_data = NULL;
    tmp_data = json_object_get_value_at(object_data, i);
    if (tmp_data == NULL) {
      ESF_JSON_ERR("json_object_get_value_at func failed. data = %p.", data);
      return kEsfJsonInternalError;
    }

    EsfJsonErrorCode ret = EsfJsonValueRecursiveRemove(json_value, tmp_data);
    if (ret != kEsfJsonSuccess) {
      ESF_JSON_ERR(
          "EsfJsonValueRecursiveRemove func failed. ret = %u, json_value = "
          "%p, tmp_data = %p.",
          ret, json_value, tmp_data);
      return kEsfJsonInternalError;
    }
    ret = EsfJsonValueLookupRemove(json_value, tmp_data);
    if (ret != kEsfJsonSuccess) {
      ESF_JSON_ERR(
          "EsfJsonValueLookupRemove func failed. ret = %u, json_value = %p, "
          "tmp_data = %p.",
          ret, json_value, tmp_data);
      return kEsfJsonInternalError;
    }
  }
  ESF_JSON_TRACE("exit");
  return kEsfJsonSuccess;
}

JSON_STATIC EsfJsonErrorCode EsfJsonArrayRecursiveRemove(
    EsfJsonValueContainer* json_value, JSON_Value* data) {
  ESF_JSON_TRACE("entry");
  // Parameter check.
  if (json_value == NULL || data == NULL) {
    ESF_JSON_ERR("Parameter error. json_value = %p, data = %p", json_value,
                 data);
    return kEsfJsonInvalidArgument;
  }

  JSON_Array* array_data = NULL;
  array_data = json_value_get_array(data);
  if (array_data == NULL) {
    ESF_JSON_ERR("json_value_get_array func failed. data = %p.", data);
    return kEsfJsonInvalidArgument;
  }

  size_t size = 0;
  size = json_array_get_count(array_data);

  size_t i = 0;
  for (i = 0; i < size; ++i) {
    JSON_Value* tmp_data = NULL;
    tmp_data = json_array_get_value(array_data, i);
    if (tmp_data == NULL) {
      ESF_JSON_ERR(
          "json_array_get_value func failed. array_data = %p, i = %zu.",
          array_data, i);
      return kEsfJsonInternalError;
    }

    EsfJsonErrorCode ret = EsfJsonValueRecursiveRemove(json_value, tmp_data);
    if (ret != kEsfJsonSuccess) {
      ESF_JSON_ERR(
          "EsfJsonValueRecursiveRemove func failed. ret = %u json_value = "
          "%p, tmp_data = %p.",
          ret, json_value, tmp_data);
      return kEsfJsonInternalError;
    }
    ret = EsfJsonValueLookupRemove(json_value, tmp_data);
    if (ret != kEsfJsonSuccess) {
      ESF_JSON_ERR(
          "EsfJsonValueLookupRemove func failed. ret = %u, json_value = %p, "
          "tmp_data = %p.",
          ret, json_value, tmp_data);
      return kEsfJsonInternalError;
    }
  }
  ESF_JSON_TRACE("exit");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonValueDataSet(JSON_Value* data, JSON_Value* set_data) {
  ESF_JSON_TRACE("entry");
  // Parameter check.
  if (data == NULL || set_data == NULL) {
    ESF_JSON_ERR("Parameter error. data = %p, set_data = %p", data, set_data);
    return kEsfJsonInvalidArgument;
  }
  JSON_Value* parent_data = NULL;
  JSON_Value_Type type = JSONError;
  parent_data = json_value_get_parent(data);
  if (parent_data == NULL) {
    json_value_free(data);
    ESF_JSON_TRACE("exit");
    return kEsfJsonSuccess;
  } else {
    type = json_value_get_type(parent_data);
    if (type == JSONError) {
      ESF_JSON_ERR("json_value_get_type func failed. parent_data = %p.",
                   parent_data);
      return kEsfJsonInternalError;
    } else if (type == JSONObject) {
      return EsfJsonValueObjectSet(parent_data, data, set_data);
    } else if (type == JSONArray) {
      return EsfJsonValueArraySet(parent_data, data, set_data);
    } else {
      ESF_JSON_ERR("JSON Value type is incorrect.");
      return kEsfJsonInternalError;
    }
  }
}

JSON_STATIC EsfJsonErrorCode
EsfJsonValueObjectSet(const JSON_Value* parent_data, const JSON_Value* data,
                      JSON_Value* set_data) {
  ESF_JSON_TRACE("entry");
  if (parent_data == NULL || data == NULL || set_data == NULL) {
    ESF_JSON_ERR("Parameter error. parent_data = %p, data = %p, set_data = %p",
                 parent_data, data, set_data);
    return kEsfJsonInvalidArgument;
  }

  JSON_Object* object_data = NULL;
  object_data = json_value_get_object(parent_data);
  if (object_data == NULL) {
    ESF_JSON_ERR("json_value_get_object func failed. parent_data = %p.",
                 parent_data);
    return kEsfJsonInvalidArgument;
  }

  size_t size = 0;
  size_t i = 0;
  size = json_object_get_count(object_data);

  for (i = 0; i < size; ++i) {
    JSON_Value* tmp_data = NULL;
    tmp_data = json_object_get_value_at(object_data, i);
    if (tmp_data == NULL) {
      ESF_JSON_ERR(
          "json_object_get_value_at func failed. object_data = %p, i = %zu",
          object_data, i);
      return kEsfJsonInternalError;
    }
    if (data == tmp_data) {
      break;
    }
  }

  const char* key = NULL;
  key = json_object_get_name(object_data, i);
  if (key == NULL) {
    ESF_JSON_ERR("json_object_get_name func failed. object_data = %p, i = %zu",
                 object_data, i);
    return kEsfJsonInternalError;
  }

  JSON_Status parson_ret = JSONSuccess;
  parson_ret = json_object_set_value(object_data, key, set_data);
  if (parson_ret == JSONFailure) {
    ESF_JSON_ERR(
        "json_object_set_value func failed. object_data = %p, key = %s, "
        "set_data = %p",
        object_data, key, set_data);
    return kEsfJsonInternalError;
  }

  ESF_JSON_TRACE("exit");
  return kEsfJsonSuccess;
}

JSON_STATIC EsfJsonErrorCode EsfJsonValueArraySet(const JSON_Value* parent_data,
                                                  const JSON_Value* data,
                                                  JSON_Value* set_data) {
  ESF_JSON_TRACE("entry");
  if (parent_data == NULL || data == NULL || set_data == NULL) {
    ESF_JSON_ERR("Parameter error. parent_data = %p, data = %p, set_data = %p",
                 parent_data, data, set_data);
    return kEsfJsonInvalidArgument;
  }

  JSON_Array* array_data = NULL;
  array_data = json_value_get_array(parent_data);
  if (array_data == NULL) {
    ESF_JSON_ERR("json_value_get_array func failed. parent_data = %p.",
                 parent_data);
    return kEsfJsonInvalidArgument;
  }

  size_t size = 0;
  size_t i = 0;
  size = json_array_get_count(array_data);

  for (i = 0; i < size; ++i) {
    JSON_Value* tmp_data = NULL;
    tmp_data = json_array_get_value(array_data, i);
    if (tmp_data == NULL) {
      ESF_JSON_ERR("json_array_get_value func failed. array_data = %p, i = %zu",
                   array_data, i);
      return kEsfJsonInternalError;
    }
    if (data == tmp_data) {
      break;
    }
  }

  JSON_Status parson_ret = JSONSuccess;
  parson_ret = json_array_replace_value(array_data, i, set_data);
  if (parson_ret == JSONFailure) {
    ESF_JSON_ERR(
        "json_array_replace_value func failed. array_data = %p, i = %zu, "
        "set_data = %p",
        array_data, i, set_data);
    return kEsfJsonInternalError;
  }
  ESF_JSON_TRACE("exit");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonValueNotHaveParentGet(EsfJsonValueContainer* json_value,
                                              EsfJsonValue value,
                                              JSON_Value** data) {
  ESF_JSON_TRACE("entry");
  if (json_value == NULL || data == NULL) {
    ESF_JSON_ERR("Parameter error. json_value = %p, data = %p", json_value,
                 data);
    return kEsfJsonInvalidArgument;
  }
  EsfJsonErrorCode ret = kEsfJsonInternalError;
  JSON_Value* find_data = NULL;
  // target JSON Value find
  ret = EsfJsonValueFind(json_value, value, &find_data);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonValueFind func failed. ret = %u, json_value = %p, value = "
        "%" PRId32 ".",
        ret, json_value, value);
    return ret;
  }

  JSON_Value* tmp_data = NULL;
  tmp_data = json_value_get_parent(find_data);
  if (tmp_data != NULL) {
    ESF_JSON_ERR("json_value_get_parent func failed. find_data = %p.",
                 find_data);
    return kEsfJsonParentAlreadyExists;
  }

  *data = find_data;
  ESF_JSON_TRACE("exit");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonValueNotManagedAdd(EsfJsonValueContainer* json_value,
                                           JSON_Value* data,
                                           EsfJsonValue* value) {
  ESF_JSON_TRACE("entry");
  if (json_value == NULL || data == NULL || value == NULL) {
    ESF_JSON_ERR("Parameter error. json_value = %p, data = %p, value = %p",
                 json_value, data, value);
    return kEsfJsonInvalidArgument;
  }
  EsfJsonErrorCode ret = kEsfJsonInternalError;
  EsfJsonValue value_id = (EsfJsonValue)ESF_JSON_VALUE_INVALID;
  ret = EsfJsonValueLookup(json_value, data, &value_id);
  // If not found, add new.
  if (ret == kEsfJsonValueNotFound) {
    ret = EsfJsonValueAdd(json_value, data, &value_id);
    if (ret != kEsfJsonSuccess) {
      ESF_JSON_ERR(
          "EsfJsonValueAdd func failed. ret = %u, json_value = %p, data = %p.",
          ret, json_value, data);
      return ret;
    }
  } else if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonValueLookup func failed. ret = %u, json_value = %p, data = "
        "%p.",
        ret, json_value, data);
    return ret;
  }
  // Storing search results.
  *value = value_id;

  ESF_JSON_TRACE("exit");
  return kEsfJsonSuccess;
}

bool EsfJsonCanJsonAddToJson(EsfJsonValueContainer* json_value,
                             JSON_Value* data, JSON_Value* parent_data) {
  ESF_JSON_TRACE("entry");
  // param check
  if (json_value == NULL || data == NULL || parent_data == NULL) {
    ESF_JSON_ERR(
        "Parameter error. json_value = %p, data = %p, parent_data = %p",
        json_value, data, parent_data);
    return false;
  }
  if (data == parent_data) {
    ESF_JSON_ERR("Identical data not allowed.");
    return false;
  }
  // Get parent json of parent json.
  JSON_Value* parent_parent_data = json_value_get_parent(parent_data);
  if (parent_parent_data == NULL) {
    // No parent json.
    ESF_JSON_TRACE("exit");
    return true;
  }
  // Check for can add the parent of the parent.
  return EsfJsonCanJsonAddToJson(json_value, data, parent_parent_data);
}

EsfJsonErrorCode EsfJsonConvertedStringSizeGet(
    EsfJsonValueContainer* json_value, JSON_Value* data, size_t* str_size) {
  ESF_JSON_TRACE("entry.");
  if (json_value == NULL || str_size == NULL) {
    ESF_JSON_ERR("Parameter error. json_value = %p, data = %p, str_size = %p",
                 json_value, data, str_size);
    return kEsfJsonInvalidArgument;
  }
  JSON_Value_Type type = JSONError;
  type = json_value_get_type(data);
  if (type == JSONError) {
    ESF_JSON_ERR("json_value_get_type func failed. data = %p.", data);
    return kEsfJsonInternalError;
  } else if (type == JSONObject) {
    JSON_Object* object_data = NULL;
    object_data = json_value_get_object(data);
    if (object_data == NULL) {
      ESF_JSON_ERR("json_value_get_object func failed. data = %p", data);
      return kEsfJsonInvalidArgument;
    }
    EsfJsonErrorCode ret =
        EsfJsonObjectConvertedStringSizeGet(json_value, object_data, str_size);
    if (ret != kEsfJsonSuccess) {
      ESF_JSON_ERR(
          "EsfJsonObjectConvertedStringSizeGet func failed. ret = %u, "
          "json_value = %p, object_data = %p, str_size = %p",
          ret, json_value, object_data, str_size);
      return kEsfJsonInternalError;
    }

  } else if (type == JSONArray) {
    JSON_Array* array_data = NULL;
    array_data = json_value_get_array(data);
    if (array_data == NULL) {
      ESF_JSON_ERR("json_value_get_array func failed. data = %p", data);
      return kEsfJsonInvalidArgument;
    }
    EsfJsonErrorCode ret =
        EsfJsonArrayConvertedStringSizeGet(json_value, array_data, str_size);
    if (ret != kEsfJsonSuccess) {
      ESF_JSON_ERR(
          "EsfJsonArrayConvertedStringSizeGet func failed. ret = %u, "
          "json_value = %p, object_data = %p, str_size = %p",
          ret, json_value, array_data, str_size);
      return kEsfJsonInternalError;
    }
  } else if (type == JSONString) {
    EsfJsonValue find_id = (EsfJsonValue)ESF_JSON_VALUE_INVALID;
    EsfJsonErrorCode ret = EsfJsonValueLookup(json_value, data, &find_id);
    if (ret != kEsfJsonSuccess) {
      ESF_JSON_ERR(
          "EsfJsonValueLookup func failed. ret = %u, json_value = %p, data = "
          "%p",
          ret, json_value, data);
      return kEsfJsonInternalError;
    }
    ret = EsfJsonReplacementStringSizeGet(json_value, find_id, str_size);
    if (ret != kEsfJsonSuccess) {
      ESF_JSON_ERR(
          "EsfJsonReplacementStringSizeGet func failed. ret = %u, json_value "
          "= "
          "%p, find_id = %" PRId32 "",
          ret, json_value, find_id);
      return kEsfJsonInternalError;
    }
  } else {
    // Do nothing.
  }
  ESF_JSON_TRACE("exit.");
  return kEsfJsonSuccess;
}

JSON_STATIC EsfJsonErrorCode EsfJsonObjectConvertedStringSizeGet(
    EsfJsonValueContainer* json_value, JSON_Object* object_data,
    size_t* str_size) {
  ESF_JSON_TRACE("entry.");
  if (json_value == NULL || object_data == NULL || str_size == NULL) {
    ESF_JSON_ERR(
        "Parameter error. json_value = %p, object_data = %p, str_size = %p",
        json_value, object_data, str_size);
    return kEsfJsonInvalidArgument;
  }
  size_t size = 0;
  size = json_object_get_count(object_data);
  for (size_t i = 0; i < size; ++i) {
    JSON_Value* find_data = NULL;
    find_data = json_object_get_value_at(object_data, i);
    if (find_data == NULL) {
      ESF_JSON_ERR(
          "json_object_get_value_at func failed. object_data = %p, index = "
          "%zu",
          object_data, i);
      return kEsfJsonIndexExceed;
    }
    EsfJsonErrorCode ret = EsfJsonConvertedStringSizeGet(json_value, find_data,
                                                         str_size);
    if (ret != kEsfJsonSuccess) {
      ESF_JSON_ERR(
          "EsfJsonConvertedStringSizeGet func failed. ret = %u, json_value = "
          "%p, find_data = %p, str_size = %p",
          ret, json_value, find_data, str_size);
      return kEsfJsonInternalError;
    }
  }
  ESF_JSON_TRACE("exit.");
  return kEsfJsonSuccess;
}

JSON_STATIC EsfJsonErrorCode
EsfJsonArrayConvertedStringSizeGet(EsfJsonValueContainer* json_value,
                                   JSON_Array* array_data, size_t* str_size) {
  ESF_JSON_TRACE("entry.");
  if (json_value == NULL || array_data == NULL || str_size == NULL) {
    ESF_JSON_ERR(
        "Parameter error. json_value = %p, array_data = %p, str_size = %p",
        json_value, array_data, str_size);
    return kEsfJsonInvalidArgument;
  }
  size_t size = 0;
  size = json_array_get_count(array_data);
  for (size_t i = 0; i < size; ++i) {
    JSON_Value* find_data = NULL;
    find_data = json_array_get_value(array_data, i);
    if (find_data == NULL) {
      ESF_JSON_ERR(
          "json_object_get_value_at func failed. object_data = %p, index = "
          "%zu",
          array_data, i);
      return kEsfJsonIndexExceed;
    }
    EsfJsonErrorCode ret = EsfJsonConvertedStringSizeGet(json_value, find_data,
                                                         str_size);
    if (ret != kEsfJsonSuccess) {
      ESF_JSON_ERR(
          "EsfJsonConvertedStringSizeGet func failed. ret = %u, json_value = "
          "%p, find_data = %p, str_size = %p",
          ret, json_value, find_data, str_size);
      return kEsfJsonInternalError;
    }
  }
  ESF_JSON_TRACE("exit.");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonCheckStringReplace(EsfJsonValueContainer* json_value,
                                           JSON_Value* data,
                                           bool* is_included) {
  ESF_JSON_TRACE("entry.");
  if (json_value == NULL || data == NULL || is_included == NULL) {
    ESF_JSON_ERR(
        "Parameter error. json_value = %p, data = %p, is_included = %p",
        json_value, data, is_included);
    return kEsfJsonInvalidArgument;
  }
  JSON_Value_Type type = JSONError;
  type = json_value_get_type(data);
  if (type == JSONError) {
    ESF_JSON_ERR("json_value_get_type func failed. data = %p.", data);
    return kEsfJsonInternalError;
  } else if (type == JSONObject) {
    EsfJsonErrorCode ret = EsfJsonObjectRecursiveCheck(json_value, data,
                                                       is_included);
    if (ret != kEsfJsonSuccess) {
      ESF_JSON_ERR(
          "EsfJsonObjectRecursiveCheck func failed. ret = %u, json_value = "
          "%p, "
          "data = %p, is_included = %p",
          ret, json_value, data, is_included);
      return kEsfJsonInternalError;
    }
  } else if (type == JSONArray) {
    EsfJsonErrorCode ret = EsfJsonArrayRecursiveCheck(json_value, data,
                                                      is_included);
    if (ret != kEsfJsonSuccess) {
      ESF_JSON_ERR(
          "EsfJsonArrayRecursiveCheck func failed. ret = %u, json_value = %p, "
          "data = %p, is_included = %p",
          ret, json_value, data, is_included);
      return kEsfJsonInternalError;
    }
  } else if (type == JSONString) {
    EsfJsonValue find_id = (EsfJsonValue)ESF_JSON_VALUE_INVALID;
    EsfJsonErrorCode ret = EsfJsonValueLookup(json_value, data, &find_id);
    if (ret != kEsfJsonSuccess) {
      ESF_JSON_ERR(
          "EsfJsonValueLookup func failed. ret = %u, json_value = %p, data = "
          "%p",
          ret, json_value, data);
      return kEsfJsonInternalError;
    }
    for (int i = 0; i < CONFIG_EXTERNAL_CODEC_JSON_MEM_HANDLE_MAX; ++i) {
      if (json_value->mem_info[i].id == find_id) {
        *is_included = true;
        ESF_JSON_TRACE("exit.");
        return kEsfJsonSuccess;
      } else {
        continue;
      }
    }
  } else {
    // Do nothing.
  }
  ESF_JSON_TRACE("exit.");
  return kEsfJsonSuccess;
}

JSON_STATIC EsfJsonErrorCode EsfJsonReplacementStringSizeGet(
    EsfJsonValueContainer* json_value, EsfJsonValue replacement_value,
    size_t* str_size) {
  ESF_JSON_TRACE("entry.");
  if (json_value == NULL || str_size == NULL) {
    ESF_JSON_ERR("Parameter error. json_value = %p, str_size = %p", json_value,
                 str_size);
    return kEsfJsonInvalidArgument;
  }
  for (int i = 0; i < CONFIG_EXTERNAL_CODEC_JSON_MEM_HANDLE_MAX; ++i) {
    if (json_value->mem_info[i].id == replacement_value) {
      char convert_str[REPLACEMENT_STRING_MAX_SIZE];
      int len = snprintf(convert_str, sizeof(convert_str),
                         REPLACEMENT_STRING "%" PRId32 "",
                         json_value->mem_info[i].id);
      if (len < 0) {
        ESF_JSON_ERR("Failed to get string size.");
        return kEsfJsonInternalError;
      }
      if (len > (int)sizeof(convert_str)) {
        ESF_JSON_ERR("Buffer size exceeded.");
        return kEsfJsonInternalError;
      }
      if (json_value->mem_info[i].size >= (size_t)len) {
        *str_size = *str_size + (json_value->mem_info[i].size - (size_t)len);
      } else {
        *str_size = *str_size + ((size_t)len - json_value->mem_info[i].size);
      }

      return kEsfJsonSuccess;
    } else {
      continue;
    }
  }
  ESF_JSON_TRACE("exit.");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonSerializeUsingMemoryManagerFileIO(
    EsfJsonValueContainer* json_value, EsfMemoryManagerHandle mem_handle,
    bool mem_handle_open, char* serialized_str, bool is_included,
    size_t* serialized_size) {
  ESF_JSON_TRACE("entry.");
  if (json_value == NULL || serialized_str == NULL || serialized_size == NULL) {
    ESF_JSON_ERR(
        "Parameter error. json_value = %p, serialized_str = %p, "
        "serialized_size = %p",
        json_value, serialized_str, serialized_size);
    return kEsfJsonInvalidArgument;
  }
  size_t wsize = 0;
  size_t serialize_size = 0;
  if (is_included == true) {
    EsfJsonErrorCode ret = EsfJsonSerializeConversionStringFileIO(
        json_value, mem_handle, mem_handle_open, &serialized_str, &wsize,
        &serialize_size);
    if (ret != kEsfJsonSuccess) {
      ESF_JSON_ERR(
          "EsfJsonSerializeConversionStringFileIO func failed. ret = %u, "
          "json_value "
          "= %p, mem_handle = %" PRIu32 ", serialized_str = %p",
          ret, json_value, mem_handle, serialized_str);
      return kEsfJsonInternalError;
    }

  } else {
    // Do nothing.
  }
  size_t serialized_len = strlen(serialized_str) + 1;
  EsfMemoryManagerResult result = EsfMemoryManagerFwrite(
      mem_handle, serialized_str, serialized_len, &wsize);
  if (result != kEsfMemoryManagerResultSuccess) {
    ESF_JSON_ERR(
        "EsfMemoryManagerFwrite func failed. ret = %u, mem_handle = %" PRIu32
        ", buf = %p, size = %zu, wsize = %zu",
        result, mem_handle, serialized_str, serialized_len, wsize);
    return kEsfJsonInternalError;
  }
  serialize_size += wsize;
  if (serialize_size != 0) {
    serialize_size -= 1;
  }
  *serialized_size = serialize_size;
  ESF_JSON_TRACE("exit.");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonSerializeUsingMemoryManagerMemMap(
    EsfJsonValueContainer* json_value, EsfMemoryManagerHandle mem_handle,
    void* mem_data, char* serialized_str, bool is_included,
    size_t* serialized_size) {
  ESF_JSON_TRACE("entry.");
  if (json_value == NULL || serialized_str == NULL || serialized_size == NULL ||
      mem_data == NULL) {
    ESF_JSON_ERR(
        "Parameter error. json_value = %p, serialized_str = %p, "
        "serialized_size = %p mem_data = %p",
        json_value, serialized_str, serialized_size, mem_data);
    return kEsfJsonInvalidArgument;
  }
  size_t wsize = 0;
  size_t serialize_size = 0;
  void* mem_data_tmp = mem_data;
  if (is_included == true) {
    EsfJsonErrorCode ret = EsfJsonSerializeConversionStringMemMap(
        json_value, mem_handle, &mem_data_tmp, &serialized_str, &wsize,
        &serialize_size);
    if (ret != kEsfJsonSuccess) {
      ESF_JSON_ERR(
          "EsfJsonSerializeConversionStringMemMap func failed. ret = %u, "
          "json_value "
          "= %p, mem_handle = %" PRIu32
          ", mem_data_tmp = %p, serialized_str = %p",
          ret, json_value, mem_handle, mem_data_tmp, serialized_str);
      return kEsfJsonInternalError;
    }

  } else {
    // Do nothing.
  }
  size_t serialized_len = strlen(serialized_str) + 1;
  memcpy(mem_data_tmp, serialized_str, serialized_len);
  serialize_size += serialized_len;
  if (serialize_size != 0) {
    serialize_size -= 1;
  }
  *serialized_size = serialize_size;
  ESF_JSON_TRACE("exit.");
  return kEsfJsonSuccess;
}

JSON_STATIC EsfJsonErrorCode EsfJsonObjectRecursiveCheck(
    EsfJsonValueContainer* json_value, JSON_Value* data, bool* is_included) {
  ESF_JSON_TRACE("entry.");
  if (json_value == NULL || data == NULL || is_included == NULL) {
    ESF_JSON_ERR(
        "Parameter error. json_value = %p, data = %p, is_included = %p",
        json_value, data, is_included);
    return kEsfJsonInvalidArgument;
  }
  JSON_Object* object_data = NULL;
  object_data = json_value_get_object(data);
  if (object_data == NULL) {
    ESF_JSON_ERR("json_value_get_object func failed. data = %p", data);
    return kEsfJsonInvalidArgument;
  }
  size_t size = 0;
  size = json_object_get_count(object_data);
  for (size_t i = 0; i < size; ++i) {
    JSON_Value* find_data = NULL;
    find_data = json_object_get_value_at(object_data, i);
    if (find_data == NULL) {
      ESF_JSON_ERR(
          "json_object_get_value_at func failed. object_data = %p, index = "
          "%zu",
          object_data, i);
      return kEsfJsonIndexExceed;
    }
    EsfJsonErrorCode ret = EsfJsonCheckStringReplace(json_value, find_data,
                                                     is_included);
    if (ret != kEsfJsonSuccess) {
      ESF_JSON_ERR(
          "EsfJsonCheckStringReplace func failed. ret = %u, json_value = %p, "
          "find_data = %p",
          ret, json_value, find_data);
      return kEsfJsonInternalError;
    }
  }
  ESF_JSON_TRACE("exit.");
  return kEsfJsonSuccess;
}

JSON_STATIC EsfJsonErrorCode EsfJsonArrayRecursiveCheck(
    EsfJsonValueContainer* json_value, JSON_Value* data, bool* is_included) {
  ESF_JSON_TRACE("entry.");
  if (json_value == NULL || data == NULL || is_included == NULL) {
    ESF_JSON_ERR(
        "Parameter error. json_value = %p, data = %p, is_included = %p",
        json_value, data, is_included);
    return kEsfJsonInvalidArgument;
  }
  JSON_Array* array_data = NULL;
  array_data = json_value_get_array(data);
  if (array_data == NULL) {
    ESF_JSON_ERR("json_value_get_array func failed. data = %p", data);
    return kEsfJsonInvalidArgument;
  }
  size_t size = 0;
  size = json_array_get_count(array_data);
  for (size_t i = 0; i < size; ++i) {
    JSON_Value* find_data = NULL;
    find_data = json_array_get_value(array_data, i);
    if (find_data == NULL) {
      ESF_JSON_ERR(
          "json_array_get_value func failed. array_data = %p, index = "
          "%zu",
          array_data, i);
      return kEsfJsonIndexExceed;
    }
    EsfJsonErrorCode ret = EsfJsonCheckStringReplace(json_value, find_data,
                                                     is_included);
    if (ret != kEsfJsonSuccess) {
      ESF_JSON_ERR(
          "EsfJsonCheckStringReplace func failed. ret = %u, json_value = %p, "
          "find_data = %p",
          ret, json_value, find_data);
      return kEsfJsonInternalError;
    }
  }
  ESF_JSON_TRACE("exit.");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonMutexLock(pthread_mutex_t* mutex) {
  ESF_JSON_TRACE("entry.");
  if (mutex == NULL) {
    ESF_JSON_ERR("Parameter error. mutex = %p", mutex);
    return kEsfJsonInvalidArgument;
  }

  int ret = pthread_mutex_lock(mutex);
  if (ret != 0) {
    ESF_JSON_ERR("pthread_mutex_lock func failed. ret = %d errorno = %d.", ret,
                 errno);
    return kEsfJsonInternalError;
  }

  ESF_JSON_TRACE("exit.");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonMutexUnlock(pthread_mutex_t* mutex) {
  ESF_JSON_TRACE("entry.");
  if (mutex == NULL) {
    ESF_JSON_ERR("Parameter error. mutex = %p", mutex);
    return kEsfJsonInvalidArgument;
  }
  int ret = pthread_mutex_unlock(mutex);
  if (ret != 0) {
    ESF_JSON_ERR("pthread_mutex_unlock func failed. ret = %d errorno = %d.",
                 ret, errno);
    return kEsfJsonInternalError;
  }

  ESF_JSON_TRACE("exit.");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonStringSetHandleInternal(
    EsfJsonValueContainer* json_value, EsfJsonValue value, JSON_Value* data) {
  ESF_JSON_TRACE("entry.");
  if (json_value == NULL || data == NULL) {
    ESF_JSON_ERR("Parameter error. json_value = %p, data = %p.", json_value,
                 data);
    return kEsfJsonInvalidArgument;
  }
  // Recursive deletion JSON value.
  EsfJsonErrorCode ret = EsfJsonValueRecursiveRemove(json_value, data);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonValueRecursiveRemove func failed. ret = %u, "
        "json_value = %p, data = %p.",
        ret, json_value, data);
    return kEsfJsonInternalError;
  }
  for (int i = 0; i < CONFIG_EXTERNAL_CODEC_JSON_MEM_HANDLE_MAX; ++i) {
    JSON_Value* tmp_data = NULL;
    ret = EsfJsonValueFind(json_value, json_value->mem_info[i].id, &tmp_data);
    if (ret == kEsfJsonValueNotFound) {
      json_value->mem_info[i] = kEsfJsonMemoryDefault;
    }
  }

  char replace_string[REPLACEMENT_STRING_MAX_SIZE] = REPLACEMENT_STRING;
  snprintf(replace_string, sizeof(replace_string),
           REPLACEMENT_STRING "%" PRId32 "", value);
  JSON_Value* new_data = NULL;
  new_data = json_value_init_string(replace_string);
  if (new_data == NULL) {
    ESF_JSON_ERR("json_value_init_string func failed.");
    return kEsfJsonOutOfMemory;
  }

  ret = EsfJsonValueDataSet(data, new_data);
  if (ret != kEsfJsonSuccess) {
    json_value_free(new_data);
    ESF_JSON_ERR(
        "EsfJsonValueDataSet func failed. ret = %u, data = %p, new_data = %p.",
        ret, data, new_data);
    return kEsfJsonInternalError;
  }

  ret = EsfJsonValueReplace(json_value, value, new_data);
  if (ret != kEsfJsonSuccess) {
    json_value_free(new_data);
    ESF_JSON_ERR(
        "EsfJsonValueReplace func failed. ret = %u, json_value = %p, ID = "
        "%" PRId32 ", new_data = %p",
        ret, json_value, value, new_data);
    return kEsfJsonInternalError;
  }
  ESF_JSON_TRACE("exit.");
  return kEsfJsonSuccess;
}

JSON_STATIC EsfJsonErrorCode EsfJsonSerializeConversionStringFileIO(
    EsfJsonValueContainer* json_value, EsfMemoryManagerHandle mem_handle,
    bool mem_handle_open, char** serialized_str, size_t* wsize,
    size_t* serialize_size) {
  ESF_JSON_TRACE("entry.");
  if (json_value == NULL || serialized_str == NULL || wsize == NULL ||
      serialize_size == NULL) {
    ESF_JSON_ERR(
        "Parameter error. json_value = %p, serialized_str = %p, wsize = %p, "
        "serialize_size = %p.",
        json_value, serialized_str, wsize, serialize_size);
    return kEsfJsonInvalidArgument;
  }
  char* buf = (char*)malloc(CONFIG_EXTERNAL_CODEC_JSON_BUFFER_SIZE);
  if (buf == NULL) {
    ESF_JSON_ERR("Failed to allocate memory for buf.");
    return kEsfJsonOutOfMemory;
  }
  EsfJsonErrorCode ret = kEsfJsonSuccess;
  EsfMemoryManagerResult mem_ret = kEsfMemoryManagerResultSuccess;
  for (int i = 0; i < CONFIG_EXTERNAL_CODEC_JSON_MEM_HANDLE_MAX; ++i) {
    if (ret == kEsfJsonSuccess &&
        json_value->mem_info[i].id != ESF_JSON_VALUE_INVALID) {
      char convert_str[REPLACEMENT_STRING_MAX_SIZE];
      int str_len = snprintf(convert_str, sizeof(convert_str),
                             REPLACEMENT_STRING "%" PRId32 "",
                             json_value->mem_info[i].id);
      if (str_len < 0) {
        ESF_JSON_ERR("Failed to get string size.");
        ret = kEsfJsonInternalError;
        break;
      }
      if (str_len > (int)sizeof(convert_str)) {
        ESF_JSON_ERR("Buffer size exceeded.");
        ret = kEsfJsonInternalError;
        break;
      }
      char* search_p;
      search_p = strstr(*serialized_str, convert_str);
      if (search_p == NULL) {
        continue;
      } else {
        size_t tmp_size = (size_t)(search_p - *serialized_str);
        EsfMemoryManagerResult result = EsfMemoryManagerFwrite(
            mem_handle, *serialized_str, tmp_size, wsize);
        if (result != kEsfMemoryManagerResultSuccess) {
          ESF_JSON_ERR(
              "EsfMemoryManagerFwrite func failed. ret = %u, mem_handle = "
              "%" PRIu32 ", buf = %p, size = %zu, wsize = %zu",
              result, mem_handle, *serialized_str, tmp_size, *wsize);
          ret = kEsfJsonInternalError;
          break;
        }
        *serialize_size += *wsize;
        // open mem_handle:json_value->mem_info[i].data
        if (mem_handle_open) {
          mem_ret = EsfMemoryManagerFopen(json_value->mem_info[i].data);
          if (mem_ret != kEsfMemoryManagerResultSuccess) {
            ESF_JSON_ERR(
                "EsfMemoryManagerFopen for json_value->mem_info[%d].data = "
                "%" PRIu32 " failed. ret = %u",
                i, json_value->mem_info[i].data, mem_ret);
            ret = kEsfJsonInternalError;
            break;
          }
        }
        off_t offset = 0;
        result = EsfMemoryManagerFseek(json_value->mem_info[i].data,
                                       json_value->mem_info[i].offset, SEEK_SET,
                                       &offset);
        if (result != kEsfMemoryManagerResultSuccess) {
          ESF_JSON_ERR(
              "EsfMemoryManagerFseek func failed. ret = %u, json_value = "
              "%" PRIu32 ", offset = %jd, res_offset = %jd",
              ret, json_value->mem_info[i].data,
              (intmax_t)json_value->mem_info[i].offset, (intmax_t)offset);
          // Close mem_handle:json_value->mem_info[i].data
          if (mem_handle_open) {
            mem_ret = EsfMemoryManagerFclose(json_value->mem_info[i].data);
            if (mem_ret != kEsfMemoryManagerResultSuccess) {
              ESF_JSON_ERR(
                  "EsfMemoryManagerFclose for json_value->mem_info[%d].data = "
                  "%" PRIu32 " failed. ret = %u",
                  i, json_value->mem_info[i].data, mem_ret);
            }
          }
          ret = kEsfJsonInternalError;
          break;
        }
        size_t written_size = 0;
        while (json_value->mem_info[i].size != written_size) {
          size_t size = json_value->mem_info[i].size - written_size;
          if (size >= CONFIG_EXTERNAL_CODEC_JSON_BUFFER_SIZE) {
            size = CONFIG_EXTERNAL_CODEC_JSON_BUFFER_SIZE;
          } else {
            // Do nothing.
          }
          size_t read_size = 0;
          result = EsfMemoryManagerFread(json_value->mem_info[i].data, buf,
                                         size, &read_size);
          if (result != kEsfMemoryManagerResultSuccess) {
            ESF_JSON_ERR(
                "EsfMemoryManagerFread func failed. ret = %u, mem_handle = "
                "%" PRIu32 ", buf = %p, size = %zu, rsize = %zu",
                result, json_value->mem_info[i].data, buf,
                json_value->mem_info[i].size, read_size);
            ret = kEsfJsonInternalError;
            break;
          }
          result = EsfMemoryManagerFwrite(mem_handle, buf, read_size, wsize);
          if (result != kEsfMemoryManagerResultSuccess) {
            ESF_JSON_ERR(
                "EsfMemoryManagerFwrite func failed. ret = %u, mem_handle = "
                "%" PRIu32 ", buf = %p, size = %zu, wsize = %zu",
                result, mem_handle, buf, read_size, *wsize);
            ret = kEsfJsonInternalError;
            break;
          }
          if (read_size != *wsize) {
            ESF_JSON_ERR("Export size does not match read size.");
            ret = kEsfJsonInternalError;
            break;
          }
          written_size += *wsize;
          *serialize_size += *wsize;
        }
        *serialized_str = search_p + str_len;
        // Close mem_handle:json_value->mem_info[i].data
        if (mem_handle_open) {
          mem_ret = EsfMemoryManagerFclose(json_value->mem_info[i].data);
          if (mem_ret != kEsfMemoryManagerResultSuccess) {
            ESF_JSON_ERR(
                "EsfMemoryManagerFclose for json_value->mem_info[%d].data = "
                "%" PRIu32 " failed. ret = %u",
                i, json_value->mem_info[i].data, mem_ret);
            ret = kEsfJsonInternalError;
            break;
          }
        }
      }
    } else {
      break;
    }
  }
  free(buf);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_TRACE("exit.");
    return ret;
  }
  ESF_JSON_TRACE("exit.");
  return kEsfJsonSuccess;
}

JSON_STATIC EsfJsonErrorCode EsfJsonSerializeConversionStringMemMap(
    EsfJsonValueContainer* json_value, EsfMemoryManagerHandle mem_handle,
    void** mem_data, char** serialized_str, size_t* wsize,
    size_t* serialize_size) {
  ESF_JSON_TRACE("entry.");
  if (json_value == NULL || serialized_str == NULL || wsize == NULL ||
      serialize_size == NULL || mem_data == NULL || *mem_data == NULL) {
    ESF_JSON_ERR(
        "Parameter error. json_value = %p, serialized_str = %p, wsize = %p, "
        "serialize_size = %p. mem_data = %p",
        json_value, serialized_str, wsize, serialize_size, mem_data);
    return kEsfJsonInvalidArgument;
  }
  EsfJsonErrorCode ret = kEsfJsonSuccess;
  for (int i = 0; i < CONFIG_EXTERNAL_CODEC_JSON_MEM_HANDLE_MAX; ++i) {
    if (ret == kEsfJsonSuccess &&
        json_value->mem_info[i].id != ESF_JSON_VALUE_INVALID) {
      char convert_str[REPLACEMENT_STRING_MAX_SIZE];
      int str_len = snprintf(convert_str, sizeof(convert_str),
                             REPLACEMENT_STRING "%" PRId32 "",
                             json_value->mem_info[i].id);
      if (str_len < 0) {
        ESF_JSON_ERR("Failed to get string size.");
        ret = kEsfJsonInternalError;
        break;
      }
      if (str_len > (int)sizeof(convert_str)) {
        ESF_JSON_ERR("Buffer size exceeded.");
        ret = kEsfJsonInternalError;
        break;
      }
      char* search_p;
      search_p = strstr(*serialized_str, convert_str);
      if (search_p == NULL) {
        continue;
      } else {
        size_t tmp_size = (size_t)(search_p - *serialized_str);
        memcpy(*mem_data, *serialized_str, tmp_size);
        *wsize = tmp_size;
        *serialize_size += *wsize;
        *mem_data = (void*)((uintptr_t)(*mem_data) + (uintptr_t)tmp_size);
        // map mem_handle:json_value->mem_info[i].data
        void* mem_data_read = NULL;
        void* mem_data_read_tmp = NULL;
        EsfMemoryManagerResult mem_ret =
            EsfMemoryManagerMap(json_value->mem_info[i].data, NULL,
                                json_value->mem_info[i].size, &mem_data_read);
        if (mem_ret != kEsfMemoryManagerResultSuccess) {
          ESF_JSON_ERR(
              "EsfMemoryManagerMap for json_value->mem_info[%d].data = "
              "%" PRIu32 " failed. ret = %u",
              i, json_value->mem_info[i].data, mem_ret);
          ret = kEsfJsonInternalError;
          break;
        }
        mem_data_read_tmp = mem_data_read;
        // Setting the handle data position for reading
        mem_data_read = (void*)((uintptr_t)mem_data_read +
                                (uintptr_t)json_value->mem_info[i].offset);
        size_t written_size = 0;
        while (json_value->mem_info[i].size != written_size) {
          size_t size = json_value->mem_info[i].size - written_size;
          if (size >= CONFIG_EXTERNAL_CODEC_JSON_BUFFER_SIZE) {
            size = CONFIG_EXTERNAL_CODEC_JSON_BUFFER_SIZE;
          } else {
            // Do nothing.
          }
          *wsize = size;
          memcpy(*mem_data, mem_data_read, size);
          *mem_data = (void*)((uintptr_t)(*mem_data) + (uintptr_t)size);
          mem_data_read = (void*)((uintptr_t)mem_data_read + (uintptr_t)size);
          written_size += *wsize;
          *serialize_size += *wsize;
        }
        *serialized_str = search_p + str_len;
        // unmap mem_handle:json_value->mem_info[i].data
        mem_ret = EsfMemoryManagerUnmap(json_value->mem_info[i].data,
                                        &mem_data_read_tmp);
        mem_data_read = NULL;
        mem_data_read_tmp = NULL;
        if (mem_ret != kEsfMemoryManagerResultSuccess) {
          ESF_JSON_ERR(
              "EsfMemoryManagerUnmap for json_value->mem_info[%d].data = "
              "%" PRIu32 " failed. ret = %u",
              i, json_value->mem_info[i].data, mem_ret);
          ret = kEsfJsonInternalError;
          break;
        }
      }
    } else {
      break;
    }
  }
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_TRACE("exit.");
    return ret;
  }
  ESF_JSON_TRACE("exit.");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonSerializeHandleMemMap(EsfJsonHandle handle,
                                              EsfJsonValue value,
                                              EsfMemoryManagerHandle mem_handle,
                                              size_t* serialized_size) {
  ESF_JSON_TRACE("entry");
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
  if (mem_handle == (EsfMemoryManagerHandle)0) {
    ESF_JSON_ERR("mem handle NULL");
    ESF_JSON_TRACE("exit.");
    return kEsfJsonHandleError;
  }
  EsfJsonErrorCode ret = kEsfJsonInternalError;
  ret = EsfJsonMutexLock(&mem_mutex);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR("EsfJsonMutexLock func failed. ret = %u", ret);
    ESF_JSON_TRACE("exit.");
    return ret;
  }
  // 2. map mem_handle
  void* mem_data = NULL;
  EsfMemoryManagerResult mem_ret =
      EsfMemoryManagerMap(mem_handle, NULL, (int32_t)MAP_ALL_AREA, &mem_data);
  if (mem_ret != kEsfMemoryManagerResultSuccess) {
    ESF_JSON_ERR("EsfMemoryManagerMap for mem handle failed. ret = %u",
                 mem_ret);
    ret = EsfJsonMutexUnlock(&mem_mutex);
    if (ret != kEsfJsonSuccess) {
      ESF_JSON_ERR("EsfJsonMutexUnlock func failed. ret = %u", ret);
    }
    ESF_JSON_TRACE("exit.");
    return kEsfJsonInternalError;
  }
  // 3. Json Serialize
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
    result = EsfJsonSerializeIncludesHandle(handle, value, &is_included);
    if (result != kEsfJsonSuccess) {
      json_free_serialized_string(serialized_str);
      ESF_JSON_ERR(
          "EsfJsonSerializeIncludesHandle func failed. ret = %u, handle = %p, "
          "value = %" PRId32 "",
          result, handle, value);
      break;
    }
    result = EsfJsonSerializeUsingMemoryManagerMemMap(
        handle->json_value, mem_handle, mem_data, serialized_str, is_included,
        serialized_size);
    if (result != kEsfJsonSuccess) {
      ESF_JSON_ERR(
          "EsfJsonSerializeUsingMemoryManagerMemMap func failed. ret = %u, "
          "handle->json_value = %p, mem_handle = %" PRIu32
          ", mem_data = %p, serialized_str = %p, is_included = %d",
          result, handle->json_value, mem_handle, mem_data, serialized_str,
          is_included);
      json_free_serialized_string(serialized_str);
      break;
    }
  } while (0);
  // 4. unmap mem_handle
  mem_ret = EsfMemoryManagerUnmap(mem_handle, &mem_data);
  if (mem_ret != kEsfMemoryManagerResultSuccess) {
    ESF_JSON_ERR("EsfMemoryManagerUnmap for mem handle failed. ret = %u",
                 mem_ret);
    if (result == kEsfJsonSuccess) {
      result = kEsfJsonInternalError;
    }
  }
  ret = EsfJsonMutexUnlock(&mem_mutex);
  if (ret != kEsfJsonSuccess) {
    json_free_serialized_string(serialized_str);
    ESF_JSON_ERR("EsfJsonMutexUnlock func failed. ret = %u", ret);
    ESF_JSON_TRACE("exit.");
    return ret;
  }
  if (result != kEsfJsonSuccess) {
    ESF_JSON_TRACE("exit.");
    return result;
  }
  json_free_serialized_string(serialized_str);
  ESF_JSON_DEBUG("JSON Value serialize mem handle ID = %" PRId32
                 ", mem_handle = %" PRIu32 ", serialized_size = %zu.",
                 value, mem_handle, *serialized_size);

  ESF_JSON_TRACE("exit.");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonSerializeHandleFileIO(EsfJsonHandle handle,
                                              EsfJsonValue value,
                                              EsfMemoryManagerHandle mem_handle,
                                              size_t* serialized_size) {
  ESF_JSON_TRACE("entry");
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
  if (mem_handle == (EsfMemoryManagerHandle)0) {
    ESF_JSON_ERR("mem handle NULL");
    ESF_JSON_TRACE("exit.");
    return kEsfJsonHandleError;
  }
  EsfJsonErrorCode ret = kEsfJsonInternalError;
  ret = EsfJsonMutexLock(&mem_mutex);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR("EsfJsonMutexLock func failed. ret = %u", ret);
    ESF_JSON_TRACE("exit.");
    return ret;
  }
  // 2. open mem_handle
  EsfMemoryManagerResult mem_ret = EsfMemoryManagerFopen(mem_handle);
  if (mem_ret != kEsfMemoryManagerResultSuccess) {
    ESF_JSON_ERR("EsfMemoryManagerFopen for mem handle failed. ret = %u",
                 mem_ret);
    ret = EsfJsonMutexUnlock(&mem_mutex);
    if (ret != kEsfJsonSuccess) {
      ESF_JSON_ERR("EsfJsonMutexUnlock func failed. ret = %u", ret);
    }
    ESF_JSON_TRACE("exit.");
    return kEsfJsonInternalError;
  }
  // 3. Json Serialize
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
    result = EsfJsonSerializeIncludesHandle(handle, value, &is_included);
    if (result != kEsfJsonSuccess) {
      json_free_serialized_string(serialized_str);
      ESF_JSON_ERR(
          "EsfJsonSerializeIncludesHandle func failed. ret = %u, handle = %p, "
          "value = %" PRId32 "",
          result, handle, value);
      break;
    }
    result = EsfJsonSerializeUsingMemoryManagerFileIO(
        handle->json_value, mem_handle, true, serialized_str, is_included,
        serialized_size);
    if (result != kEsfJsonSuccess) {
      ESF_JSON_ERR(
          "EsfJsonSerializeUsingMemoryManagerFileIO func failed. ret = %u, "
          "handle->json_value = %p, mem_handle = %" PRIu32
          "mem_handle_open = %d, serialized_str = %p, is_included = %d",
          result, handle->json_value, mem_handle, true, serialized_str,
          is_included);
      json_free_serialized_string(serialized_str);
      break;
    }
  } while (0);
  // 4. close mem_handle
  mem_ret = EsfMemoryManagerFclose(mem_handle);
  if (mem_ret != kEsfMemoryManagerResultSuccess) {
    ESF_JSON_ERR("EsfMemoryManagerFclose for mem handle failed. ret = %u",
                 mem_ret);
    if (result == kEsfJsonSuccess) {
      result = kEsfJsonInternalError;
    }
  }
  ret = EsfJsonMutexUnlock(&mem_mutex);
  if (ret != kEsfJsonSuccess) {
    json_free_serialized_string(serialized_str);
    ESF_JSON_ERR("EsfJsonMutexUnlock func failed. ret = %u", ret);
    ESF_JSON_TRACE("exit.");
    return ret;
  }
  if (result != kEsfJsonSuccess) {
    ESF_JSON_TRACE("exit.");
    return result;
  }
  json_free_serialized_string(serialized_str);
  ESF_JSON_DEBUG("JSON Value serialize mem handle ID = %" PRId32
                 ", mem_handle = %" PRIu32 ", serialized_size = %zu.",
                 value, mem_handle, *serialized_size);

  ESF_JSON_TRACE("exit.");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonStringInitHandleProcess(
    EsfJsonHandle handle, EsfMemoryManagerHandle mem_handle, size_t mem_size,
    EsfJsonValue* value) {
  ESF_JSON_TRACE("entry");
  // 1. validate parameters
  if (handle == ESF_JSON_HANDLE_INITIALIZER) {
    ESF_JSON_ERR("handle NULL.");
    ESF_JSON_TRACE("exit.");
    return kEsfJsonHandleError;
  }
  if (value == NULL) {
    ESF_JSON_ERR("Parameter error. value = %p", value);
    ESF_JSON_TRACE("exit.");
    return kEsfJsonInvalidArgument;
  }
  // 2. Json StringInit
  size_t empty_mem_index =
      EsfJsonFindEmptyMemHandleInfo(handle->json_value, ESF_JSON_VALUE_INVALID);
  if (empty_mem_index == CONFIG_EXTERNAL_CODEC_JSON_MEM_HANDLE_MAX) {
    ESF_JSON_ERR(
        "There is no free space in the mem handle retention area of the "
        "handle.");
    ESF_JSON_TRACE("exit.");
    return kEsfJsonValueLimit;
  }
  char tmp_string[REPLACEMENT_STRING_MAX_SIZE] = REPLACEMENT_STRING "tmp";
  JSON_Value* tmp_data = NULL;
  tmp_data = json_value_init_string(tmp_string);
  if (tmp_data == NULL) {
    ESF_JSON_ERR("json_value_init_string func failed.");
    ESF_JSON_TRACE("exit.");
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

  off_t current_offset = 0;
  if (ret == kEsfJsonSuccess) {
    handle->json_value->mem_info[empty_mem_index].id = value_id;
    handle->json_value->mem_info[empty_mem_index].data = mem_handle;
    handle->json_value->mem_info[empty_mem_index].size = mem_size;
    handle->json_value->mem_info[empty_mem_index].offset = current_offset;
    *value = value_id;
    ESF_JSON_DEBUG("JSON String Init mem handle Info ID = %" PRId32 "",
                   value_id);
  }

  ESF_JSON_TRACE("exit.");
  return ret;
}

EsfJsonErrorCode EsfJsonStringSetHandleProcess(
    EsfJsonHandle handle, EsfJsonValue value, EsfMemoryManagerHandle mem_handle,
    size_t mem_size) {
  ESF_JSON_TRACE("entry");
  // 1. validate parameters
  if (handle == ESF_JSON_HANDLE_INITIALIZER) {
    ESF_JSON_ERR("handle NULL.");
    ESF_JSON_TRACE("exit.");
    return kEsfJsonHandleError;
  }
  // 2. Json StringSet
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

  size_t empty_mem_index = EsfJsonFindEmptyMemHandleInfo(handle->json_value,
                                                         value);
  if (empty_mem_index == CONFIG_EXTERNAL_CODEC_JSON_MEM_HANDLE_MAX) {
    ESF_JSON_ERR(
        "There is no free space in the mem handle retention area of the "
        "handle.");
    ESF_JSON_TRACE("exit.");
    return kEsfJsonValueLimit;
  }

  ret = EsfJsonStringSetHandleInternal(handle->json_value, value, data);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonStringSetHandleInternal func failed. ret = %u, "
        "handle->json_value = %p, value = %" PRId32 ", data = %p.",
        ret, handle->json_value, value, data);
    ESF_JSON_TRACE("exit.");
    return ret;
  }
  off_t current_offset = 0;
  handle->json_value->mem_info[empty_mem_index].id = value;
  handle->json_value->mem_info[empty_mem_index].data = mem_handle;
  handle->json_value->mem_info[empty_mem_index].size = mem_size;
  handle->json_value->mem_info[empty_mem_index].offset = current_offset;

  ESF_JSON_DEBUG("JSON String Set mem handle Info ID = %" PRId32 "", value);
  ESF_JSON_TRACE("exit.");
  return kEsfJsonSuccess;
}

// In the case of EsfJsonStringInitHandle, overwrite MemInfo to the element of
// ESF_JSON_VALUE_INVALID, and in the case of EsfJsonStringSetHandle, overwrite
// MemInfo to the element of the same JSON Value.
// If neither of the above applies and there is no space for the element,
// return CONFIG_EXTERNAL_CODEC_JSON_MEM_HANDLE_MAX.
JSON_STATIC size_t EsfJsonFindEmptyMemHandleInfo(
    EsfJsonValueContainer* container, EsfJsonValue value) {
  ESF_JSON_TRACE("entry");
  size_t i = 0;
  if (container == NULL) {
    ESF_JSON_TRACE("exit.");
    return CONFIG_EXTERNAL_CODEC_JSON_MEM_HANDLE_MAX;
  }
  for (i = 0; i < CONFIG_EXTERNAL_CODEC_JSON_MEM_HANDLE_MAX; ++i) {
    if (container->mem_info[i].id == value) {
      break;
    }
  }
  ESF_JSON_TRACE("exit.");
  return i;
}
