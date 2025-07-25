/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#include "json.h"

#include <stdio.h>
#include <stdlib.h>

#include "json_internal.h"
#include "parson/lib/parson.h"

// Failure to obtain number of elements.
#define ESF_JSON_COUNT_GET_ERROR ((int32_t)-1)

static const EsfJsonHandleImpl kEsfJsonHandleDefault = {
    .json_value = NULL,
    .serialized_str = NULL,
};

EsfJsonErrorCode EsfJsonOpen(EsfJsonHandle* handle) {
  ESF_JSON_TRACE("entry");
  // Parameter check.
  if (handle == ESF_JSON_HANDLE_INITIALIZER) {
    ESF_JSON_ERR("handle NULL.");
    return kEsfJsonInvalidArgument;
  }

  EsfJsonHandle tmp_handle = ESF_JSON_HANDLE_INITIALIZER;
  tmp_handle = (EsfJsonHandleImpl*)malloc(sizeof(*tmp_handle));
  if (tmp_handle == NULL) {
    ESF_JSON_ERR("Failed to allocate memory for tmp_handle.");
    return kEsfJsonOutOfMemory;
  }

  // Initialize with default values.
  *tmp_handle = kEsfJsonHandleDefault;

  EsfJsonErrorCode ret = kEsfJsonInternalError;
  // JSON handle member init
  ret = EsfJsonValueContainerInit(&tmp_handle->json_value);
  if (ret != kEsfJsonSuccess) {
    free(tmp_handle);
    ESF_JSON_ERR("EsfJsonValueContainerInit func failed. ret = %u.", ret);
    return kEsfJsonOutOfMemory;
  }

  *handle = tmp_handle;
  ESF_JSON_DEBUG("handle open %p", handle);
  ESF_JSON_TRACE("exit");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonClose(EsfJsonHandle handle) {
  ESF_JSON_TRACE("entry");
  // Parameter check.
  if (handle == ESF_JSON_HANDLE_INITIALIZER) {
    ESF_JSON_ERR("Parameter error. handle = %p", handle);
    return kEsfJsonInvalidArgument;
  }

  EsfJsonErrorCode ret = kEsfJsonInternalError;
  // Free handle processing
  ret = EsfJsonValueContainerFree(handle->json_value);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonValueContainerFree func failed. ret = %u, handle->json_value = "
        "%p",
        ret, handle->json_value);
  }

  if (handle->serialized_str != NULL) {
    json_free_serialized_string(handle->serialized_str);
  }
  ESF_JSON_DEBUG("handle close %p", handle);
  free(handle);
  ESF_JSON_TRACE("exit");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonSerialize(EsfJsonHandle handle, EsfJsonValue value,
                                  const char** str) {
  ESF_JSON_TRACE("entry");
  // Parameter check.
  if (handle == ESF_JSON_HANDLE_INITIALIZER) {
    ESF_JSON_ERR("Parameter error. handle = %p", handle);
    return kEsfJsonHandleError;
  }

  if (str == NULL) {
    ESF_JSON_ERR("Parameter error. str = %p", str);
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

  char* serialized_str = NULL;
  serialized_str = json_serialize_to_string(data);
  if (serialized_str == NULL) {
    ESF_JSON_ERR("json_serialize_to_string func failed.");
    return kEsfJsonOutOfMemory;
  }
  ret = EsfJsonSerializeFree(handle);
  if (ret != kEsfJsonSuccess) {
    json_free_serialized_string(serialized_str);
    ESF_JSON_ERR("EsfJsonSerializeFree func failed. ret = %u", ret);
    return kEsfJsonHandleError;
  }

  handle->serialized_str = serialized_str;

  *str = handle->serialized_str;
  ESF_JSON_DEBUG("JSON Value serialize ID = %" PRId32 ", data = %p", value,
                 data);
  ESF_JSON_TRACE("exit");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonSerializeFree(EsfJsonHandle handle) {
  ESF_JSON_TRACE("entry");
  // Parameter check.
  if (handle == ESF_JSON_HANDLE_INITIALIZER) {
    ESF_JSON_ERR("Parameter error. handle = %p", handle);
    return kEsfJsonHandleError;
  }

  if (handle->serialized_str == NULL) {
    // Do nothing.
  } else {
    json_free_serialized_string(handle->serialized_str);
    handle->serialized_str = NULL;
  }

  ESF_JSON_TRACE("exit");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonDeserialize(EsfJsonHandle handle, const char* str,
                                    EsfJsonValue* value) {
  ESF_JSON_TRACE("entry");
  // Parameter check.
  if (handle == ESF_JSON_HANDLE_INITIALIZER) {
    ESF_JSON_ERR("Parameter error. handle = %p", handle);
    return kEsfJsonHandleError;
  }

  if (str == NULL || value == NULL) {
    ESF_JSON_ERR("Parameter error. str = %p, value = %p", str, value);
    return kEsfJsonInvalidArgument;
  }

  JSON_Value* data = NULL;
  data = json_parse_string(str);
  if (data == NULL) {
    ESF_JSON_ERR("json_parse_string func failed. str = %p.", str);
    return kEsfJsonInvalidArgument;
  }

  EsfJsonValue value_id = ESF_JSON_VALUE_INVALID;
  EsfJsonErrorCode ret = kEsfJsonInternalError;
  ret = EsfJsonValueAdd(handle->json_value, data, &value_id);
  if (ret != kEsfJsonSuccess) {
    json_value_free(data);
    ESF_JSON_ERR(
        "EsfJsonValueAdd func failed. ret = %u, handle->json_value = %p, data "
        "= %p",
        ret, handle->json_value, data);
    return ret;
  }

  // ID storage.
  *value = value_id;
  ESF_JSON_DEBUG("JSON Value deserialize ID = %" PRId32 "", *value);
  ESF_JSON_TRACE("exit");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonObjectInit(EsfJsonHandle handle, EsfJsonValue* value) {
  ESF_JSON_TRACE("entry");
  // Parameter check.
  if (handle == ESF_JSON_HANDLE_INITIALIZER) {
    ESF_JSON_ERR("Parameter error. handle = %p", handle);
    return kEsfJsonHandleError;
  }

  if (value == NULL) {
    ESF_JSON_ERR("Parameter error. value = %p", value);
    return kEsfJsonInvalidArgument;
  }

  JSON_Value* data = NULL;
  data = json_value_init_object();
  if (data == NULL) {
    ESF_JSON_ERR("json_value_init_object func failed.");
    return kEsfJsonOutOfMemory;
  }

  EsfJsonValue value_id = ESF_JSON_VALUE_INVALID;
  EsfJsonErrorCode ret = kEsfJsonInternalError;
  ret = EsfJsonValueAdd(handle->json_value, data, &value_id);
  if (ret != kEsfJsonSuccess) {
    json_value_free(data);
    ESF_JSON_ERR(
        "EsfJsonValueAdd func failed. ret = %u, handle->json_value = %p, data "
        "= %p",
        ret, handle->json_value, data);
    return ret;
  }

  // ID storage.
  *value = value_id;
  ESF_JSON_DEBUG("JSON Object Init Info ID = %" PRId32 "", value_id);
  ESF_JSON_TRACE("exit");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonObjectGet(EsfJsonHandle handle, EsfJsonValue parent,
                                  const char* key, EsfJsonValue* value) {
  ESF_JSON_TRACE("entry");
  // Parameter check.
  if (handle == ESF_JSON_HANDLE_INITIALIZER) {
    ESF_JSON_ERR("Parameter error. handle = %p", handle);
    return kEsfJsonHandleError;
  }

  if (key == NULL || value == NULL) {
    ESF_JSON_ERR("Parameter error. key = %p, value = %p", key, value);
    return kEsfJsonInvalidArgument;
  }

  JSON_Value* data = NULL;
  EsfJsonErrorCode ret = kEsfJsonInternalError;
  ret = EsfJsonValueFind(handle->json_value, parent, &data);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonValueFind func failed. ret = %u, handle->json_value = %p, "
        "parent = %" PRId32 "",
        ret, handle->json_value, parent);
    return ret;
  }

  JSON_Object* object_data = NULL;
  object_data = json_value_get_object(data);
  if (object_data == NULL) {
    ESF_JSON_ERR("json_value_get_object func failed.");
    return kEsfJsonValueTypeError;
  }

  JSON_Value* find_data = NULL;
  find_data = json_object_get_value(object_data, key);
  if (find_data == NULL) {
    ESF_JSON_ERR("json_object_get_value func failed. key = %s.", key);
    return kEsfJsonInvalidArgument;
  }

  EsfJsonValue value_id = ESF_JSON_VALUE_INVALID;
  // Add to handle if JSON Value not managed.
  ret = EsfJsonValueNotManagedAdd(handle->json_value, find_data, &value_id);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonValueNotManagedAdd func failed. ret = %u, handle->json_value = "
        "%p, find_data = %p.",
        ret, handle->json_value, find_data);
    return ret;
  }

  // Storing search results.
  *value = value_id;
  ESF_JSON_DEBUG("JSON Object Get Info parent ID = %" PRId32
                 ", key = %s, get ID = %" PRId32 "",
                 parent, key, value_id);
  ESF_JSON_TRACE("exit");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonObjectSet(EsfJsonHandle handle, EsfJsonValue parent,
                                  const char* key, EsfJsonValue value) {
  ESF_JSON_TRACE("entry");
  // Parameter check.
  if (handle == ESF_JSON_HANDLE_INITIALIZER) {
    ESF_JSON_ERR("Parameter error. handle = %p", handle);
    return kEsfJsonHandleError;
  }

  if (key == NULL) {
    ESF_JSON_ERR("Parameter error. key = %p", key);
    return kEsfJsonInvalidArgument;
  }

  JSON_Value* parent_data = NULL;
  EsfJsonErrorCode ret = kEsfJsonInternalError;
  // Search parent data.
  ret = EsfJsonValueFind(handle->json_value, parent, &parent_data);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonValueFind func failed. ret = %u, handle->json_value = %p, "
        "parent = %" PRId32 "",
        ret, handle->json_value, parent);
    return ret;
  }

  JSON_Value* data = NULL;
  ret = EsfJsonValueNotHaveParentGet(handle->json_value, value, &data);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonValueNotHaveParentGet func failed. ret = %u handle->json_value "
        "= %p, value = %" PRId32 "",
        ret, handle->json_value, value);
    return ret;
  }

  if (!EsfJsonCanJsonAddToJson(handle->json_value, data, parent_data)) {
    ESF_JSON_ERR(
        "EsfJsonCanJsonAddToJson func return false handle->json_value = %p, "
        "data = %p, parent_data = %p.",
        handle->json_value, data, parent_data);
    return kEsfJsonValueDuplicated;
  }

  JSON_Object* object_data = NULL;
  object_data = json_value_get_object(parent_data);
  if (object_data == NULL) {
    ESF_JSON_ERR("json_value_get_object func failed. parent_data = %p.",
                 parent_data);
    return kEsfJsonValueTypeError;
  }

  JSON_Value* tmp_data = NULL;
  tmp_data = json_object_get_value(object_data, key);
  if (tmp_data != NULL) {
    // Recursive deletion JSON value.
    ret = EsfJsonValueRecursiveRemove(handle->json_value, tmp_data);
    if (ret != kEsfJsonSuccess) {
      ESF_JSON_ERR(
          "EsfJsonValueRecursiveRemove func failed. ret = %u, "
          "handle->json_value = %p, tmp_data = %p",
          ret, handle->json_value, tmp_data);
      return kEsfJsonInternalError;
    }
    ret = EsfJsonValueLookupRemove(handle->json_value, tmp_data);
    if (ret != kEsfJsonSuccess) {
      ESF_JSON_ERR(
          "EsfJsonValueLookupRemove func failed. ret = %u, handle->json_value "
          "= %p, tmp_data = %p",
          ret, handle->json_value, tmp_data);
      return kEsfJsonInternalError;
    }
  }

  JSON_Status parson_ret = JSONSuccess;
  parson_ret = json_object_set_value(object_data, key, data);
  if (parson_ret == JSONFailure) {
    ESF_JSON_ERR(
        "json_object_set_value func failed. object_data = %p, key = %s, data = "
        "%p.",
        object_data, key, data);
    return kEsfJsonOutOfMemory;
  }

  ESF_JSON_DEBUG("JSON Object Set Info Object ID = %" PRId32
                 " key = %s Set ID = %" PRId32 "",
                 parent, key, value);
  ESF_JSON_TRACE("exit");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonObjectRemove(EsfJsonHandle handle, EsfJsonValue parent,
                                     const char* key) {
  ESF_JSON_TRACE("entry");
  // Parameter check.
  if (handle == ESF_JSON_HANDLE_INITIALIZER) {
    ESF_JSON_ERR("Parameter error. handle = %p", handle);
    return kEsfJsonHandleError;
  }

  if (key == NULL) {
    ESF_JSON_ERR("Parameter error. key = %p", key);
    return kEsfJsonInvalidArgument;
  }

  JSON_Value* parent_data = NULL;
  EsfJsonErrorCode ret = kEsfJsonInternalError;
  ret = EsfJsonValueFind(handle->json_value, parent, &parent_data);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonValueFind func failed. ret = %u, handle->json_value = %p, "
        "parent = %" PRId32 "",
        ret, handle->json_value, parent);
    return ret;
  }

  JSON_Object* object_data = NULL;
  object_data = json_value_get_object(parent_data);
  if (object_data == NULL) {
    ESF_JSON_ERR("json_value_get_object func failed. parent_data = %p.",
                 parent_data);
    return kEsfJsonValueTypeError;
  }

  JSON_Value* tmp_data = NULL;
  tmp_data = json_object_get_value(object_data, key);
  if (tmp_data == NULL) {
    ESF_JSON_ERR(
        "json_object_get_value func failed. object_data = %p, key = %s.",
        object_data, key);
    return kEsfJsonInvalidArgument;
  }
  // Recursively delete JSON value.
  ret = EsfJsonValueRecursiveRemove(handle->json_value, tmp_data);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonValueRecursiveRemove func failed. ret = %u, handle->json_value "
        "= %p, tmp_data = %p",
        ret, handle->json_value, tmp_data);
    return kEsfJsonInternalError;
  }
  ret = EsfJsonValueLookupRemove(handle->json_value, tmp_data);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonValueLookupRemove func failed. ret = %u, handle->json_value = "
        "%p, tmp_data = %p",
        ret, handle->json_value, tmp_data);
    return kEsfJsonInternalError;
  }

  JSON_Status parson_ret = JSONSuccess;
  parson_ret = json_object_remove(object_data, key);
  if (parson_ret == JSONFailure) {
    ESF_JSON_ERR("json_object_remove func failed. object_data = %p, key = %s.",
                 object_data, key);
    return kEsfJsonInternalError;
  }

  ESF_JSON_DEBUG("JSON Object Remove Info Object ID = %" PRId32 " key = %s",
                 parent, key);
  ESF_JSON_TRACE("exit");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonObjectClear(EsfJsonHandle handle, EsfJsonValue value) {
  ESF_JSON_TRACE("entry");
  // Parameter check.
  if (handle == ESF_JSON_HANDLE_INITIALIZER) {
    ESF_JSON_ERR("Parameter error. handle = %p", handle);
    return kEsfJsonHandleError;
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

  // Recursively delete JSON value.
  ret = EsfJsonValueRecursiveRemove(handle->json_value, data);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonValueRecursiveRemove func failed. ret = %u, handle->json_value "
        "= %p, data = %p",
        ret, handle->json_value, data);
    return kEsfJsonInternalError;
  }

  // Set new JSON object as parent.
  JSON_Value_Type type = JSONError;
  type = json_value_get_type(data);
  if (type == JSONError) {
    ESF_JSON_ERR("JSON type get error.");
    return kEsfJsonInternalError;
  } else if (type == JSONObject) {
    JSON_Object* object_data = NULL;
    object_data = json_value_get_object(data);
    if (object_data == NULL) {
      ESF_JSON_ERR("json_value_get_object func failed. data = %p.", data);

      return kEsfJsonInvalidArgument;
    }
    JSON_Status parson_ret = JSONSuccess;
    parson_ret = json_object_clear(object_data);
    if (parson_ret == JSONFailure) {
      ESF_JSON_ERR("json_object_clear func failed. object_data = %p.",
                   object_data);

      return kEsfJsonInternalError;
    }
  } else {  // array, other
    JSON_Value* new_data = NULL;
    new_data = json_value_init_object();
    if (new_data == NULL) {
      ESF_JSON_ERR("json_value_init_object func failed.");

      return kEsfJsonOutOfMemory;
    }

    ret = EsfJsonValueDataSet(data, new_data);
    if (ret != kEsfJsonSuccess) {
      json_value_free(new_data);
      ESF_JSON_ERR(
          "EsfJsonValueDataSet func failed. ret = %u, data = %p, new_data = %p",
          ret, data, new_data);
      return ret;
    }

    ret = EsfJsonValueReplace(handle->json_value, value, new_data);
    if (ret != kEsfJsonSuccess) {
      json_value_free(new_data);
      ESF_JSON_ERR(
          "EsfJsonValueReplace func failed. ret = %u, handle->json_value = %p, "
          "value = %" PRId32 ", new_data = %p",
          ret, handle->json_value, value, new_data);
      return kEsfJsonInternalError;
    }
  }

  ESF_JSON_DEBUG("JSON Object Clear Info Object ID = %" PRId32 "", value);
  ESF_JSON_TRACE("exit");
  return kEsfJsonSuccess;
}

int32_t EsfJsonObjectCount(EsfJsonHandle handle, EsfJsonValue parent) {
  ESF_JSON_TRACE("entry");
  // Parameter check.
  if (handle == ESF_JSON_HANDLE_INITIALIZER) {
    ESF_JSON_ERR("Parameter error. handle = %p", handle);
    return ESF_JSON_COUNT_GET_ERROR;
  }

  JSON_Value* data = NULL;
  EsfJsonErrorCode ret = kEsfJsonInternalError;
  ret = EsfJsonValueFind(handle->json_value, parent, &data);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonValueFind func failed. ret = %u, handle->json_value = %p, "
        "parent = %" PRId32 ".",
        ret, handle->json_value, parent);
    return ESF_JSON_COUNT_GET_ERROR;
  }

  JSON_Object* object_data = NULL;
  object_data = json_value_get_object(data);
  if (object_data == NULL) {
    ESF_JSON_ERR("json_value_get_object func failed. data = %p.", data);
    return ESF_JSON_COUNT_GET_ERROR;
  }

  size_t size = 0;
  size = json_object_get_count(object_data);
  if (size > INT32_MAX) {
    ESF_JSON_ERR(
        "json_object_get_count func failed. size = %zu object_data = %p.", size,
        object_data);
    return ESF_JSON_COUNT_GET_ERROR;
  }

  ESF_JSON_DEBUG("JSON Object Count Info Object ID = %" PRId32 ", count = %zu",
                 parent, size);
  ESF_JSON_TRACE("exit");
  return (int32_t)size;
}

EsfJsonErrorCode EsfJsonObjectGetAt(EsfJsonHandle handle, EsfJsonValue parent,
                                    int32_t index, const char** key,
                                    EsfJsonValue* value) {
  ESF_JSON_TRACE("entry");
  // Parameter check.
  if (handle == ESF_JSON_HANDLE_INITIALIZER) {
    ESF_JSON_ERR("Parameter error. handle = %p", handle);
    return kEsfJsonHandleError;
  }

  if (key == NULL || value == NULL || index < 0) {
    ESF_JSON_ERR("Parameter error. key = %p, value = %p, index = %" PRId32 "",
                 key, value, index);
    return kEsfJsonInvalidArgument;
  }

  JSON_Value* data = NULL;
  EsfJsonErrorCode ret = kEsfJsonInternalError;
  ret = EsfJsonValueFind(handle->json_value, parent, &data);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonValueFind func failed. ret = %u, handle->json_value = %p, "
        "parent = %" PRId32 ".",
        ret, handle->json_value, parent);
    return ret;
  }

  JSON_Object* object_data = NULL;
  object_data = json_value_get_object(data);
  if (object_data == NULL) {
    ESF_JSON_ERR("json_value_get_object func failed. data = %p.", data);
    return kEsfJsonValueTypeError;
  }

  const char* key_str = NULL;
  key_str = json_object_get_name(object_data, index);
  if (key_str == NULL) {
    ESF_JSON_ERR(
        "json_object_get_name func failed. object_data = %p, index = %" PRId32
        ".",
        object_data, index);
    return kEsfJsonIndexExceed;
  }

  JSON_Value* find_data = NULL;
  find_data = json_object_get_value_at(object_data, index);
  if (find_data == NULL) {
    ESF_JSON_ERR(
        "json_object_get_value_at func failed. object_data = %p, index = "
        "%" PRId32 ".",
        object_data, index);
    return kEsfJsonIndexExceed;
  }
  EsfJsonValue value_id = ESF_JSON_VALUE_INVALID;
  // Add to handle if JSON Value not managed.
  ret = EsfJsonValueNotManagedAdd(handle->json_value, find_data, &value_id);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonValueNotManagedAdd func failed. ret = %u handle->json_value = "
        "%p, find_data = %p, value_id = %" PRId32 ".",
        ret, handle->json_value, find_data, value_id);
    return ret;
  }
  // Storing search results.
  *key = key_str;
  *value = value_id;

  ESF_JSON_DEBUG("JSON Object Get At Info index = %" PRId32
                 ", get key = %s get ID = %" PRId32 "",
                 index, *key, *value);
  ESF_JSON_TRACE("exit");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonArrayInit(EsfJsonHandle handle, EsfJsonValue* value) {
  ESF_JSON_TRACE("entry");
  // Parameter check.
  if (handle == ESF_JSON_HANDLE_INITIALIZER) {
    ESF_JSON_ERR("Parameter error. handle = %p", handle);
    return kEsfJsonHandleError;
  }

  if (value == NULL) {
    ESF_JSON_ERR("Parameter error. value = %p", value);
    return kEsfJsonInvalidArgument;
  }

  JSON_Value* data = NULL;
  data = json_value_init_array();
  if (data == NULL) {
    ESF_JSON_ERR("json_value_init_array func failed.");
    return kEsfJsonOutOfMemory;
  }

  EsfJsonValue value_id = ESF_JSON_VALUE_INVALID;
  EsfJsonErrorCode ret = kEsfJsonInternalError;
  ret = EsfJsonValueAdd(handle->json_value, data, &value_id);
  if (ret != kEsfJsonSuccess) {
    json_value_free(data);
    ESF_JSON_ERR(
        "EsfJsonValueAdd func failed. ret = %u handle->json_value = %p, data = "
        "%p.",
        ret, handle->json_value, data);
    return ret;
  }

  // ID storage.
  *value = value_id;

  ESF_JSON_DEBUG("JSON Array Init Info ID = %" PRId32 "", value_id);
  ESF_JSON_TRACE("exit");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonArrayGet(EsfJsonHandle handle, EsfJsonValue parent,
                                 int32_t index, EsfJsonValue* value) {
  ESF_JSON_TRACE("entry");
  // Parameter check.
  if (handle == ESF_JSON_HANDLE_INITIALIZER) {
    ESF_JSON_ERR("Parameter error. handle = %p", handle);
    return kEsfJsonHandleError;
  }

  if (value == NULL || index < 0) {
    ESF_JSON_ERR("Parameter error. value = %p, index = %" PRId32 "", value,
                 index);
    return kEsfJsonInvalidArgument;
  }

  JSON_Value* data = NULL;
  EsfJsonErrorCode ret = kEsfJsonInternalError;
  ret = EsfJsonValueFind(handle->json_value, parent, &data);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonValueFind func failed. ret = %u, handle->json_value = %p, "
        "parent = %" PRId32 ".",
        ret, handle->json_value, parent);
    return ret;
  }

  JSON_Array* array_data = NULL;
  array_data = json_value_get_array(data);
  if (array_data == NULL) {
    ESF_JSON_ERR("json_value_get_array func failed. data = %p.", data);
    return kEsfJsonValueTypeError;
  }

  JSON_Value* tmp_data = NULL;
  tmp_data = json_array_get_value(array_data, index);
  if (tmp_data == NULL) {
    ESF_JSON_ERR(
        "json_array_get_value func failed. array_data = %p, index = %" PRId32
        ".",
        array_data, index);
    return kEsfJsonIndexExceed;
  }

  EsfJsonValue value_id = ESF_JSON_VALUE_INVALID;
  // Add to handle if JSON Value not managed.
  ret = EsfJsonValueNotManagedAdd(handle->json_value, tmp_data, &value_id);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonValueNotManagedAdd func failed. ret = %u, handle->json_value = "
        "%p, tmp_data = %p.",
        ret, handle->json_value, tmp_data);
    return ret;
  }

  *value = value_id;

  ESF_JSON_DEBUG("JSON Array Get Info parent ID = %" PRId32
                 " index = %d get ID = %" PRId32 "",
                 parent, index, value_id);
  ESF_JSON_TRACE("exit");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonArrayAppend(EsfJsonHandle handle, EsfJsonValue parent,
                                    EsfJsonValue value) {
  ESF_JSON_TRACE("entry");
  // Parameter check.
  if (handle == ESF_JSON_HANDLE_INITIALIZER) {
    ESF_JSON_ERR("Parameter error. handle = %p", handle);
    return kEsfJsonHandleError;
  }

  JSON_Value* parent_data = NULL;
  EsfJsonErrorCode ret = kEsfJsonInternalError;
  // parent JSON Value find
  ret = EsfJsonValueFind(handle->json_value, parent, &parent_data);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonValueFind func failed. ret = %u handle->json_value = %p, "
        "parent = %" PRId32 ".",
        ret, handle->json_value, parent);
    return ret;
  }

  JSON_Value* data = NULL;
  ret = EsfJsonValueNotHaveParentGet(handle->json_value, value, &data);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonValueNotHaveParentGet func failed. ret = %u, "
        "handle->json_value = %p, value = %" PRId32 ".",
        ret, handle->json_value, value);
    return ret;
  }

  if (!EsfJsonCanJsonAddToJson(handle->json_value, data, parent_data)) {
    ESF_JSON_ERR(
        "EsfJsonCanJsonAddToJson func return false handle->json_value = %p, "
        "data = %p, parent_data = %p.",
        handle->json_value, data, parent_data);
    return kEsfJsonValueDuplicated;
  }

  JSON_Array* array_data = NULL;
  array_data = json_value_get_array(parent_data);
  if (array_data == NULL) {
    ESF_JSON_ERR("json_value_get_array func failed. parent_data = %p.",
                 parent_data);
    return kEsfJsonValueTypeError;
  }

  JSON_Status parson_ret = JSONSuccess;
  parson_ret = json_array_append_value(array_data, data);
  if (parson_ret == JSONFailure) {
    ESF_JSON_ERR(
        "json_array_append_value func failed. array_data = %p, data = %p.",
        array_data, data);
    return kEsfJsonOutOfMemory;
  }

  ESF_JSON_DEBUG("JSON Array Append Info parent ID = %" PRId32
                 " append ID = %" PRId32 "",
                 parent, value);
  ESF_JSON_TRACE("exit");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonArrayReplace(EsfJsonHandle handle, EsfJsonValue parent,
                                     int32_t index, EsfJsonValue value) {
  ESF_JSON_TRACE("entry");
  // Parameter check.
  if (handle == ESF_JSON_HANDLE_INITIALIZER) {
    ESF_JSON_ERR("Parameter error. handle = %p", handle);
    return kEsfJsonHandleError;
  }

  if (index < 0) {
    ESF_JSON_ERR("Parameter error. index = %" PRId32 "", index);
    return kEsfJsonInvalidArgument;
  }

  JSON_Value* parent_data = NULL;
  EsfJsonErrorCode ret = kEsfJsonInternalError;

  // parent JSON Value find
  ret = EsfJsonValueFind(handle->json_value, parent, &parent_data);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonValueFind func failed. ret = %u, handle->json_value = %p, "
        "parent = %" PRId32 ".",
        ret, handle->json_value, parent);
    return ret;
  }

  JSON_Value* data = NULL;
  ret = EsfJsonValueNotHaveParentGet(handle->json_value, value, &data);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonValueNotHaveParentGet func failed. ret = %u, "
        "handle->json_value = %p, value = %" PRId32 ".",
        ret, handle->json_value, value);
    return ret;
  }

  if (!EsfJsonCanJsonAddToJson(handle->json_value, data, parent_data)) {
    ESF_JSON_ERR(
        "EsfJsonCanJsonAddToJson func return false handle->json_value = %p, "
        "data = %p, parent_data = %p.",
        handle->json_value, data, parent_data);
    return kEsfJsonValueDuplicated;
  }

  JSON_Array* array_data = NULL;
  array_data = json_value_get_array(parent_data);
  if (array_data == NULL) {
    ESF_JSON_ERR("json_value_get_array func failed. parent_data = %p.",
                 parent_data);
    return kEsfJsonValueTypeError;
  }

  JSON_Value* tmp_data = NULL;
  tmp_data = json_array_get_value(array_data, index);
  if (tmp_data == NULL) {
    ESF_JSON_ERR(
        "json_array_get_value func failed. array_data = %p, index = %" PRId32
        ".",
        array_data, index);
    return kEsfJsonIndexExceed;
  } else {
    // Recursive deletion JSON value.
    ret = EsfJsonValueRecursiveRemove(handle->json_value, tmp_data);
    if (ret != kEsfJsonSuccess) {
      ESF_JSON_ERR(
          "EsfJsonValueRecursiveRemove func failed. ret = %u, "
          "handle->json_value = %p, tmp_data = %p.",
          ret, handle->json_value, tmp_data);
      return kEsfJsonInternalError;
    }

    ret = EsfJsonValueLookupRemove(handle->json_value, tmp_data);
    if (ret != kEsfJsonSuccess) {
      ESF_JSON_ERR(
          "EsfJsonValueLookupRemove func failed. ret = %u, handle->json_value "
          "= %p, tmp_data = %p.",
          ret, handle->json_value, tmp_data);
      return kEsfJsonInternalError;
    }
  }

  JSON_Status parson_ret = JSONSuccess;
  parson_ret = json_array_replace_value(array_data, index, data);
  if (parson_ret == JSONFailure) {
    ESF_JSON_ERR(
        "json_array_replace_value func failed. array_data = %p, index = "
        "%" PRId32 ", data = %p.",
        array_data, index, data);
    return kEsfJsonInternalError;
  }

  ESF_JSON_DEBUG("JSON Array Replace Info parent ID = %" PRId32
                 " index = %d replace ID = %" PRId32 "",
                 parent, index, value);
  ESF_JSON_TRACE("exit");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonArrayRemove(EsfJsonHandle handle, EsfJsonValue parent,
                                    int32_t index) {
  ESF_JSON_TRACE("entry");
  // Parameter check.
  if (handle == ESF_JSON_HANDLE_INITIALIZER) {
    ESF_JSON_ERR("Parameter error. handle = %p", handle);
    return kEsfJsonHandleError;
  }

  if (index < 0) {
    ESF_JSON_ERR("Parameter error. index = %" PRId32 "", index);
    return kEsfJsonInvalidArgument;
  }

  JSON_Value* parent_data = NULL;
  EsfJsonErrorCode ret = kEsfJsonInternalError;
  ret = EsfJsonValueFind(handle->json_value, parent, &parent_data);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonValueFind func failed. ret = %u handle->json_value = %p, "
        "parent = %" PRId32 ".",
        ret, handle->json_value, parent);
    return ret;
  }

  JSON_Array* array_data = NULL;
  array_data = json_value_get_array(parent_data);
  if (array_data == NULL) {
    ESF_JSON_ERR("json_value_get_array func failed. parent_data = %p.",
                 parent_data);
    return kEsfJsonValueTypeError;
  }

  JSON_Value* tmp_data = NULL;
  tmp_data = json_array_get_value(array_data, index);
  if (tmp_data == NULL) {
    ESF_JSON_ERR(
        "json_array_get_value func failed. array_data = %p, index = %" PRId32
        ".",
        array_data, index);
    return kEsfJsonIndexExceed;
  } else {
    // Recursive deletion JSON value.
    ret = EsfJsonValueRecursiveRemove(handle->json_value, tmp_data);
    if (ret != kEsfJsonSuccess) {
      ESF_JSON_ERR(
          "EsfJsonValueRecursiveRemove func failed. ret = %u, "
          "handle->json_value = %p, tmp_data = %p.",
          ret, handle->json_value, tmp_data);
      return kEsfJsonInternalError;
    }
    ret = EsfJsonValueLookupRemove(handle->json_value, tmp_data);
    if (ret != kEsfJsonSuccess) {
      ESF_JSON_ERR(
          "EsfJsonValueLookupRemove func failed. ret = %u, handle->json_value "
          "= %p, tmp_data = %p.",
          ret, handle->json_value, tmp_data);
      return kEsfJsonInternalError;
    }
  }

  JSON_Status parson_ret = JSONSuccess;
  parson_ret = json_array_remove(array_data, index);
  if (parson_ret == JSONFailure) {
    ESF_JSON_ERR(
        "json_array_remove func failed. array_data = %p, index = %" PRId32 ".",
        array_data, index);
    return kEsfJsonInternalError;
  }

  ESF_JSON_DEBUG("JSON Array Remove Info parent ID = %" PRId32 " index = %d",
                 parent, index);
  ESF_JSON_TRACE("exit");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonArrayClear(EsfJsonHandle handle, EsfJsonValue value) {
  ESF_JSON_TRACE("entry");
  // Parameter check.
  if (handle == ESF_JSON_HANDLE_INITIALIZER) {
    ESF_JSON_ERR("Parameter error. handle = %p", handle);
    return kEsfJsonHandleError;
  }

  JSON_Value* data = NULL;
  EsfJsonErrorCode ret = kEsfJsonInternalError;
  ret = EsfJsonValueFind(handle->json_value, value, &data);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonValueFind func failed. ret = %u, handle->json_value = %p, "
        "value = %" PRId32 ".",
        ret, handle->json_value, value);
    return ret;
  }

  // Recursive deletion JSON value.
  ret = EsfJsonValueRecursiveRemove(handle->json_value, data);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonValueRecursiveRemove func failed. ret = %u, handle->json_value "
        "= %p, data = %p.",
        ret, handle->json_value, data);
    return kEsfJsonInternalError;
  }

  JSON_Value_Type type = JSONError;
  // Set new JSON array as parent.
  type = json_value_get_type(data);
  if (type == JSONError) {
    ESF_JSON_ERR("JSON type error data = %p.", data);
    return kEsfJsonInternalError;
  } else if (type == JSONArray) {
    JSON_Array* array_data = NULL;
    array_data = json_value_get_array(data);
    if (array_data == NULL) {
      ESF_JSON_ERR("json_value_get_array func failed. data = %p.", data);

      return kEsfJsonInvalidArgument;
    }
    JSON_Status parson_ret = JSONSuccess;
    parson_ret = json_array_clear(array_data);
    if (parson_ret == JSONFailure) {
      ESF_JSON_ERR("json_array_clear func failed. array_data = %p.",
                   array_data);

      return kEsfJsonInternalError;
    }
  } else {  // object, other
    JSON_Value* new_data = NULL;
    new_data = json_value_init_array();
    if (new_data == NULL) {
      ESF_JSON_ERR("json_value_init_array func failed.");

      return kEsfJsonOutOfMemory;
    }
    ret = EsfJsonValueDataSet(data, new_data);
    if (ret != kEsfJsonSuccess) {
      json_value_free(new_data);
      ESF_JSON_ERR(
          "EsfJsonValueDataSet func failed. ret = %u, data = %p, new_data = "
          "%p.",
          ret, data, new_data);
      return ret;
    }
    ret = EsfJsonValueReplace(handle->json_value, value, new_data);
    if (ret != kEsfJsonSuccess) {
      json_value_free(new_data);
      ESF_JSON_ERR(
          "EsfJsonValueReplace func failed. ret = %u, handle->json_value = %p, "
          "value = %" PRId32 ", new_data = %p.",
          ret, handle->json_value, value, new_data);
      return kEsfJsonInternalError;
    }
  }

  ESF_JSON_DEBUG("JSON Array Clear Info ID = %" PRId32 "", value);
  ESF_JSON_TRACE("exit");
  return kEsfJsonSuccess;
}

int32_t EsfJsonArrayCount(EsfJsonHandle handle, EsfJsonValue parent) {
  ESF_JSON_TRACE("entry");
  // Parameter check.
  if (handle == ESF_JSON_HANDLE_INITIALIZER) {
    ESF_JSON_ERR("Parameter error. handle = %p", handle);
    return ESF_JSON_COUNT_GET_ERROR;
  }

  JSON_Value* parent_data = NULL;
  EsfJsonErrorCode ret = kEsfJsonInternalError;
  ret = EsfJsonValueFind(handle->json_value, parent, &parent_data);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonValueFind func failed. ret = %u, handle->json_value = %p, "
        "parent = %" PRId32 ".",
        ret, handle->json_value, parent);
    return ESF_JSON_COUNT_GET_ERROR;
  }

  JSON_Array* array_data = NULL;
  array_data = json_value_get_array(parent_data);
  if (array_data == NULL) {
    ESF_JSON_ERR("json_value_get_array func failed. parent_data = %p.",
                 parent_data);
    return ESF_JSON_COUNT_GET_ERROR;
  }

  size_t array_count = 0;
  array_count = json_array_get_count(array_data);
  if (array_count > INT32_MAX) {
    ESF_JSON_ERR(
        "json_array_get_count func failed. array_data = %p, array_count = %zu",
        array_data, array_count);
    return ESF_JSON_COUNT_GET_ERROR;
  }

  ESF_JSON_DEBUG("JSON Array Count Info parent ID = %" PRId32 " count = %zu",
                 parent, array_count);
  ESF_JSON_TRACE("exit");
  return (int32_t)array_count;
}

EsfJsonErrorCode EsfJsonStringInit(EsfJsonHandle handle, const char* str,
                                   EsfJsonValue* value) {
  ESF_JSON_TRACE("entry");
  // Parameter check.
  if (handle == ESF_JSON_HANDLE_INITIALIZER) {
    ESF_JSON_ERR("Parameter error. handle = %p", handle);
    return kEsfJsonHandleError;
  }

  if (str == NULL || value == NULL) {
    ESF_JSON_ERR("Parameter error. str = %p, value = %p", str, value);
    return kEsfJsonInvalidArgument;
  }

  JSON_Value* data = NULL;
  data = json_value_init_string(str);
  if (data == NULL) {
    ESF_JSON_ERR("json_value_init_string func failed.");
    return kEsfJsonOutOfMemory;
  }

  EsfJsonValue value_id = ESF_JSON_VALUE_INVALID;
  EsfJsonErrorCode ret = kEsfJsonInternalError;
  ret = EsfJsonValueAdd(handle->json_value, data, &value_id);
  if (ret != kEsfJsonSuccess) {
    json_value_free(data);
    ESF_JSON_ERR(
        "EsfJsonValueAdd func failed. ret = %u, handle->json_value = %p, data "
        "= %p.",
        ret, handle->json_value, data);
    return ret;
  }

  // ID storage.
  *value = value_id;

  ESF_JSON_DEBUG("JSON String Init Info ID = %" PRId32 "", value_id);
  ESF_JSON_TRACE("exit");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonStringGet(EsfJsonHandle handle, EsfJsonValue value,
                                  const char** str) {
  ESF_JSON_TRACE("entry");
  // Parameter check.
  if (handle == ESF_JSON_HANDLE_INITIALIZER) {
    ESF_JSON_ERR("Parameter error. handle = %p", handle);
    return kEsfJsonHandleError;
  }

  if (str == NULL) {
    ESF_JSON_ERR("Parameter error. str = %p", str);
    return kEsfJsonInvalidArgument;
  }

  JSON_Value* data = NULL;
  EsfJsonErrorCode ret = kEsfJsonInternalError;
  ret = EsfJsonValueFind(handle->json_value, value, &data);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonValueFind func failed. ret = %u, handle->json_value = %p, "
        "value = %" PRId32 ".",
        ret, handle->json_value, value);
    return ret;
  }

  const char* string_value = NULL;
  string_value = json_value_get_string(data);
  if (string_value == NULL) {
    ESF_JSON_ERR("json_value_get_string func failed. data = %p.", data);
    return kEsfJsonValueTypeError;
  }
  *str = string_value;

  ESF_JSON_DEBUG("JSON String Get Info ID = %" PRId32 "", value);
  ESF_JSON_TRACE("exit");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonStringSet(EsfJsonHandle handle, EsfJsonValue value,
                                  const char* str) {
  ESF_JSON_TRACE("entry");
  // Parameter check.
  if (handle == ESF_JSON_HANDLE_INITIALIZER) {
    ESF_JSON_ERR("Parameter error. handle = %p", handle);
    return kEsfJsonHandleError;
  }

  if (str == NULL) {
    ESF_JSON_ERR("Parameter error. str = %p", str);
    return kEsfJsonInvalidArgument;
  }

  JSON_Value* data = NULL;
  EsfJsonErrorCode ret = kEsfJsonInternalError;
  ret = EsfJsonValueFind(handle->json_value, value, &data);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonValueFind func failed. ret = %u, handle->json_value = %p, "
        "value = %" PRId32 ".",
        ret, handle->json_value, value);
    return ret;
  }

  // Recursive deletion JSON value.
  ret = EsfJsonValueRecursiveRemove(handle->json_value, data);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonValueRecursiveRemove func failed. ret = %u, "
        "handle->json_value = %p, data = %p.",
        ret, handle->json_value, data);
    return kEsfJsonInternalError;
  }

  JSON_Value* new_data = NULL;
  // Set new JSON String as parent.
  new_data = json_value_init_string(str);
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
    return ret;
  }

  ret = EsfJsonValueReplace(handle->json_value, value, new_data);
  if (ret != kEsfJsonSuccess) {
    json_value_free(new_data);
    ESF_JSON_ERR(
        "EsfJsonValueReplace func failed. ret = %u, handle->json_value = %p, "
        "value = %" PRId32 ", new_data = %p.",
        ret, handle->json_value, value, new_data);
    return kEsfJsonInternalError;
  }

  ESF_JSON_DEBUG("JSON String Set Info ID = %" PRId32 "", value);
  ESF_JSON_TRACE("exit");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonIntegerInit(EsfJsonHandle handle, int32_t num,
                                    EsfJsonValue* value) {
  ESF_JSON_TRACE("entry");
  // Parameter check.
  if (handle == ESF_JSON_HANDLE_INITIALIZER) {
    ESF_JSON_ERR("Parameter error. handle = %p", handle);
    return kEsfJsonHandleError;
  }

  if (value == NULL) {
    ESF_JSON_ERR("Parameter error. value = %p", value);
    return kEsfJsonInvalidArgument;
  }

  double set_num = 0;
  set_num = (double)num;

  JSON_Value* data = NULL;
  data = json_value_init_number(set_num);
  if (data == NULL) {
    ESF_JSON_ERR("json_value_init_number func failed. set_num = %f.", set_num);
    return kEsfJsonOutOfMemory;
  }

  EsfJsonValue value_id = ESF_JSON_VALUE_INVALID;
  EsfJsonErrorCode ret = kEsfJsonInternalError;
  ret = EsfJsonValueAdd(handle->json_value, data, &value_id);
  if (ret != kEsfJsonSuccess) {
    json_value_free(data);
    ESF_JSON_ERR(
        "EsfJsonValueAdd func failed. ret = %u, handle->json_value = %p, data "
        "= %p.",
        ret, handle->json_value, data);
    return ret;
  }

  // ID storage.
  *value = value_id;

  ESF_JSON_DEBUG("JSON Integer Init Info num = %" PRId32 " ID = %" PRId32 "",
                 num, value_id);
  ESF_JSON_TRACE("exit");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonRealInit(EsfJsonHandle handle, double num,
                                 EsfJsonValue* value) {
  ESF_JSON_TRACE("entry");
  // Parameter check.
  if (handle == ESF_JSON_HANDLE_INITIALIZER) {
    ESF_JSON_ERR("Parameter error. handle = %p", handle);
    return kEsfJsonHandleError;
  }

  if (value == NULL) {
    ESF_JSON_ERR("Parameter error. value = %p", value);
    return kEsfJsonInvalidArgument;
  }

  JSON_Value* data = NULL;
  data = json_value_init_number(num);
  if (data == NULL) {
    ESF_JSON_ERR("json_value_init_number func failed. num = %f.", num);
    return kEsfJsonOutOfMemory;
  }

  EsfJsonValue value_id = ESF_JSON_VALUE_INVALID;
  EsfJsonErrorCode ret = kEsfJsonInternalError;
  ret = EsfJsonValueAdd(handle->json_value, data, &value_id);
  if (ret != kEsfJsonSuccess) {
    json_value_free(data);
    ESF_JSON_ERR(
        "EsfJsonValueAdd func failed. ret = %u, handle->json_value = %p, data "
        "= %p.",
        ret, handle->json_value, data);
    return ret;
  }

  // ID storage.
  *value = value_id;

  ESF_JSON_DEBUG("JSON Real Init Info num = %f ID = %" PRId32 "", num,
                 value_id);
  ESF_JSON_TRACE("exit");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonIntegerGet(EsfJsonHandle handle, EsfJsonValue value,
                                   int32_t* num) {
  ESF_JSON_TRACE("entry");
  // Parameter check.
  if (handle == ESF_JSON_HANDLE_INITIALIZER) {
    ESF_JSON_ERR("Parameter error. handle = %p", handle);
    return kEsfJsonHandleError;
  }

  if (num == NULL) {
    ESF_JSON_ERR("Parameter error. num = %p", num);
    return kEsfJsonInvalidArgument;
  }

  JSON_Value* data = NULL;
  EsfJsonErrorCode ret = kEsfJsonInternalError;
  ret = EsfJsonValueFind(handle->json_value, value, &data);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonValueFind func failed. ret = %u, handle->json_value = %p, "
        "value = %" PRId32 ".",
        ret, handle->json_value, value);
    return ret;
  }

  JSON_Value_Type type = JSONError;
  type = json_value_get_type(data);
  if (type != JSONNumber) {
    ESF_JSON_ERR("JSON type error type = %d, data = %p", type, data);
    return kEsfJsonValueTypeError;
  }

  *num = (int32_t)json_value_get_number(data);

  ESF_JSON_DEBUG("JSON Integer Get Info ID = %" PRId32 " num = %" PRId32 "",
                 value, *num);
  ESF_JSON_TRACE("exit");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonRealGet(EsfJsonHandle handle, EsfJsonValue value,
                                double* num) {
  ESF_JSON_TRACE("entry");
  // Parameter check.
  if (handle == ESF_JSON_HANDLE_INITIALIZER) {
    ESF_JSON_ERR("Parameter error. handle = %p", handle);
    return kEsfJsonHandleError;
  }

  if (num == NULL) {
    ESF_JSON_ERR("Parameter error. num = %p", num);
    return kEsfJsonInvalidArgument;
  }

  JSON_Value* data = NULL;
  EsfJsonErrorCode ret = kEsfJsonInternalError;
  ret = EsfJsonValueFind(handle->json_value, value, &data);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonValueFind func failed. ret = %u, handle->json_value = %p, "
        "value = %" PRId32 ".",
        ret, handle->json_value, value);
    return ret;
  }

  JSON_Value_Type type = JSONError;
  type = json_value_get_type(data);
  if (type != JSONNumber) {
    ESF_JSON_ERR("JSON type error type = %d, data = %p", type, data);
    return kEsfJsonValueTypeError;
  }

  *num = json_value_get_number(data);

  ESF_JSON_DEBUG("JSON Real Get Info ID = %" PRId32 " num = %f", value, *num);
  ESF_JSON_TRACE("exit");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonIntegerSet(EsfJsonHandle handle, EsfJsonValue value,
                                   int32_t num) {
  ESF_JSON_TRACE("entry");
  // Parameter check.
  if (handle == ESF_JSON_HANDLE_INITIALIZER) {
    ESF_JSON_ERR("Parameter error. handle = %p", handle);
    return kEsfJsonHandleError;
  }

  JSON_Value* data = NULL;
  EsfJsonErrorCode ret = kEsfJsonInternalError;
  ret = EsfJsonValueFind(handle->json_value, value, &data);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonValueFind func failed. ret = %u, handle->json_value = %p, "
        "value = %" PRId32 ".",
        ret, handle->json_value, value);
    return ret;
  }

  // Recursive deletion JSON value.
  ret = EsfJsonValueRecursiveRemove(handle->json_value, data);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonValueRecursiveRemove func failed. ret = %u, "
        "handle->json_value = %p, data = %p.",
        ret, handle->json_value, data);
    return kEsfJsonInternalError;
  }

  double set_num = 0;
  // Set new JSON number as parent.
  set_num = (double)num;

  JSON_Value* new_data = NULL;
  new_data = json_value_init_number(set_num);
  if (new_data == NULL) {
    ESF_JSON_ERR("json_value_init_number func failed.");
    return kEsfJsonOutOfMemory;
  }

  ret = EsfJsonValueDataSet(data, new_data);
  if (ret != kEsfJsonSuccess) {
    json_value_free(new_data);
    ESF_JSON_ERR(
        "EsfJsonValueDataSet func failed. ret = %u, data = %p, new_data = %p.",
        ret, data, new_data);
    return ret;
  }

  ret = EsfJsonValueReplace(handle->json_value, value, new_data);
  if (ret != kEsfJsonSuccess) {
    json_value_free(new_data);
    ESF_JSON_ERR(
        "EsfJsonValueReplace func failed. ret = %u, handle->json_value = %p, "
        "value = %" PRId32 ", new_data = %p.",
        ret, handle->json_value, value, new_data);
    return kEsfJsonInternalError;
  }

  ESF_JSON_DEBUG("JSON Integer Set Info ID = %" PRId32 " num = %" PRId32 "",
                 value, num)
  ESF_JSON_TRACE("exit");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonRealSet(EsfJsonHandle handle, EsfJsonValue value,
                                double num) {
  ESF_JSON_TRACE("entry");
  // Parameter check.
  if (handle == ESF_JSON_HANDLE_INITIALIZER) {
    ESF_JSON_ERR("Parameter error. handle = %p", handle);
    return kEsfJsonHandleError;
  }

  JSON_Value* data = NULL;
  EsfJsonErrorCode ret = kEsfJsonInternalError;
  ret = EsfJsonValueFind(handle->json_value, value, &data);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonValueFind func failed. ret = %u, handle->json_value = %p, "
        "value = %" PRId32 ".",
        ret, handle->json_value, value);
    return ret;
  }

  // Recursive deletion JSON value.
  ret = EsfJsonValueRecursiveRemove(handle->json_value, data);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonValueRecursiveRemove func failed. ret = %u, "
        "handle->json_value = %p, data = %p.",
        ret, handle->json_value, data);
    return kEsfJsonInternalError;
  }

  JSON_Value* new_data = NULL;
  // Set new JSON number as parent.
  new_data = json_value_init_number(num);
  if (new_data == NULL) {
    ESF_JSON_ERR("json_value_init_number func failed. num = %f.", num);
    return kEsfJsonOutOfMemory;
  }

  ret = EsfJsonValueDataSet(data, new_data);
  if (ret != kEsfJsonSuccess) {
    json_value_free(new_data);
    ESF_JSON_ERR(
        "EsfJsonValueDataSet func failed. ret = %u, data = %p, new_data = %p.",
        ret, data, new_data);
    return ret;
  }

  ret = EsfJsonValueReplace(handle->json_value, value, new_data);
  if (ret != kEsfJsonSuccess) {
    json_value_free(new_data);
    ESF_JSON_ERR(
        "EsfJsonValueReplace func failed. ret = %u, handle->json_value = %p, "
        "value = %" PRId32 ", new_data = %p.",
        ret, handle->json_value, value, new_data);
    return kEsfJsonInternalError;
  }

  ESF_JSON_DEBUG("JSON Real Set Info ID = %" PRId32 " num = %f", value, num)
  ESF_JSON_TRACE("exit");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonBooleanInit(EsfJsonHandle handle, bool boolean,
                                    EsfJsonValue* value) {
  ESF_JSON_TRACE("entry");
  // Parameter check.
  if (handle == ESF_JSON_HANDLE_INITIALIZER) {
    ESF_JSON_ERR("Parameter error. handle = %p", handle);
    return kEsfJsonHandleError;
  }

  if (value == NULL) {
    ESF_JSON_ERR("Parameter error. value = %p", value);
    return kEsfJsonInvalidArgument;
  }

  int set_boolean = 0;
  set_boolean = (int)boolean;

  JSON_Value* data = NULL;
  data = json_value_init_boolean(set_boolean);
  if (data == NULL) {
    ESF_JSON_ERR("json_value_init_boolean func failed. set_boolean = %d.",
                 set_boolean);
    return kEsfJsonOutOfMemory;
  }

  EsfJsonValue value_id = ESF_JSON_VALUE_INVALID;
  EsfJsonErrorCode ret = kEsfJsonInternalError;
  ret = EsfJsonValueAdd(handle->json_value, data, &value_id);
  if (ret != kEsfJsonSuccess) {
    json_value_free(data);
    ESF_JSON_ERR(
        "EsfJsonValueAdd func failed. ret = %u, handle->json_value = %p, data "
        "=  %p.",
        ret, handle->json_value, data);
    return ret;
  }

  // ID storage.
  *value = value_id;

  ESF_JSON_DEBUG("JSON Boolean Init Info boolean = %d ID = %" PRId32 "",
                 boolean, value_id);
  ESF_JSON_TRACE("exit");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonBooleanGet(EsfJsonHandle handle, EsfJsonValue value,
                                   bool* boolean) {
  ESF_JSON_TRACE("entry");
  // Parameter check.
  if (handle == ESF_JSON_HANDLE_INITIALIZER) {
    ESF_JSON_ERR("Parameter error. handle = %p", handle);
    return kEsfJsonHandleError;
  }

  if (boolean == NULL) {
    ESF_JSON_ERR("Parameter error. boolean = %p", boolean);
    return kEsfJsonInvalidArgument;
  }

  JSON_Value* data = NULL;
  EsfJsonErrorCode ret = kEsfJsonInternalError;
  ret = EsfJsonValueFind(handle->json_value, value, &data);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonValueFind func failed. ret = %u, handle->json_value = %p, "
        "value = %" PRId32 ".",
        ret, handle->json_value, value);
    return ret;
  }

  JSON_Value_Type type = JSONError;
  type = json_value_get_type(data);
  if (type != JSONBoolean) {
    ESF_JSON_ERR("JSON type error type = %d, data = %p.", type, data);
    return kEsfJsonValueTypeError;
  }

  *boolean = (bool)json_value_get_boolean(data);

  ESF_JSON_DEBUG("JSON Boolean Get Info ID = %" PRId32 " boolean = %d", value,
                 *boolean);
  ESF_JSON_TRACE("exit");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonBooleanSet(EsfJsonHandle handle, EsfJsonValue value,
                                   bool boolean) {
  ESF_JSON_TRACE("entry");
  // Parameter check.
  if (handle == ESF_JSON_HANDLE_INITIALIZER) {
    ESF_JSON_ERR("Parameter error. handle = %p", handle);
    return kEsfJsonHandleError;
  }

  JSON_Value* data = NULL;
  EsfJsonErrorCode ret = kEsfJsonInternalError;
  ret = EsfJsonValueFind(handle->json_value, value, &data);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonValueFind func failed. ret = %u, handle->json_value = %p, "
        "value = %" PRId32 ".",
        ret, handle->json_value, value);
    return ret;
  }

  // Recursive deletion JSON value.
  ret = EsfJsonValueRecursiveRemove(handle->json_value, data);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonValueRecursiveRemove func failed. ret = %u, "
        "handle->json_value = %p, data = %p.",
        ret, handle->json_value, data);
    return kEsfJsonInternalError;
  }

  int set_boolean = 0;
  // Set new JSON boolean as parent.
  set_boolean = (int)boolean;

  JSON_Value* new_data = NULL;
  new_data = json_value_init_boolean(set_boolean);
  if (new_data == NULL) {
    ESF_JSON_ERR("json_value_init_boolean func failed. set_boolean = %d.",
                 set_boolean);
    return kEsfJsonOutOfMemory;
  }

  ret = EsfJsonValueDataSet(data, new_data);
  if (ret != kEsfJsonSuccess) {
    json_value_free(new_data);
    ESF_JSON_ERR(
        "EsfJsonValueDataSet func failed. ret = %u, data = %p, new_data = %p.",
        ret, data, new_data);
    return ret;
  }

  ret = EsfJsonValueReplace(handle->json_value, value, new_data);
  if (ret != kEsfJsonSuccess) {
    json_value_free(new_data);
    ESF_JSON_ERR(
        "EsfJsonValueReplace func failed. ret = %u, handle->json_value = %p, "
        "value = %" PRId32 ", new_data = %p.",
        ret, handle->json_value, value, new_data);
    return kEsfJsonInternalError;
  }

  ESF_JSON_DEBUG("JSON Boolean Set Info ID = %" PRId32 " boolean = %d", value,
                 boolean);
  ESF_JSON_TRACE("exit");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonNullInit(EsfJsonHandle handle, EsfJsonValue* value) {
  ESF_JSON_TRACE("entry");
  // Parameter check.
  if (handle == ESF_JSON_HANDLE_INITIALIZER) {
    ESF_JSON_ERR("Parameter error. handle = %p", handle);
    return kEsfJsonHandleError;
  }

  if (value == NULL) {
    ESF_JSON_ERR("Parameter error. value = %p", value);
    return kEsfJsonInvalidArgument;
  }

  JSON_Value* data = NULL;
  data = json_value_init_null();
  if (data == NULL) {
    ESF_JSON_ERR("json_value_init_null func failed.");
    return kEsfJsonOutOfMemory;
  }

  EsfJsonValue value_id = ESF_JSON_VALUE_INVALID;
  EsfJsonErrorCode ret = kEsfJsonInternalError;
  ret = EsfJsonValueAdd(handle->json_value, data, &value_id);
  if (ret != kEsfJsonSuccess) {
    json_value_free(data);
    ESF_JSON_ERR(
        "EsfJsonValueAdd func failed. ret = %u, handle->json_value = %p, data "
        "= %p.",
        ret, handle->json_value, data);
    return ret;
  }

  // ID storage.
  *value = value_id;

  ESF_JSON_DEBUG("JSON Null Init Info ID = %" PRId32 "", value_id);
  ESF_JSON_TRACE("exit");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonNullSet(EsfJsonHandle handle, EsfJsonValue value) {
  ESF_JSON_TRACE("entry");
  // Parameter check.
  if (handle == ESF_JSON_HANDLE_INITIALIZER) {
    ESF_JSON_ERR("Parameter error. handle = %p", handle);
    return kEsfJsonHandleError;
  }

  JSON_Value* data = NULL;
  EsfJsonErrorCode ret = kEsfJsonInternalError;
  ret = EsfJsonValueFind(handle->json_value, value, &data);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonValueFind func failed. ret = %u, handle->json_value = %p, "
        "value = %" PRId32 ".",
        ret, handle->json_value, value);
    return ret;
  }

  // Recursive deletion JSON value.
  ret = EsfJsonValueRecursiveRemove(handle->json_value, data);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonValueRecursiveRemove func failed. ret = %u, "
        "handle->json_value = %p, data = %p.",
        ret, handle->json_value, data);
    return kEsfJsonInternalError;
  }

  JSON_Value* new_data = NULL;
  // Set new JSON null as parent.
  new_data = json_value_init_null();
  if (new_data == NULL) {
    ESF_JSON_ERR("json_value_init_null func failed.");
    return kEsfJsonOutOfMemory;
  }

  ret = EsfJsonValueDataSet(data, new_data);
  if (ret != kEsfJsonSuccess) {
    json_value_free(new_data);
    ESF_JSON_ERR(
        "EsfJsonValueDataSet func failed. ret = %u, data = %p, new_data = %p.",
        ret, data, new_data);
    return ret;
  }

  ret = EsfJsonValueReplace(handle->json_value, value, new_data);
  if (ret != kEsfJsonSuccess) {
    json_value_free(new_data);
    ESF_JSON_ERR(
        "EsfJsonValueReplace func failed. ret = %u, handle->json_value = %p, "
        "value = %" PRId32 ", new_data = %p.",
        ret, handle->json_value, value, new_data);
    return kEsfJsonInternalError;
  }

  ESF_JSON_DEBUG("JSON Null Set Info ID = %" PRId32 "", value);
  ESF_JSON_TRACE("exit");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonValueCopy(EsfJsonHandle handle, EsfJsonValue source,
                                  EsfJsonValue* destination) {
  ESF_JSON_TRACE("entry");
  // Parameter check.
  if (handle == ESF_JSON_HANDLE_INITIALIZER) {
    ESF_JSON_ERR("Parameter error. handle = %p", handle);
    return kEsfJsonHandleError;
  }

  if (destination == NULL) {
    ESF_JSON_ERR("Parameter error. destination = %p", destination);
    return kEsfJsonInvalidArgument;
  }

  JSON_Value* data = NULL;
  EsfJsonErrorCode ret = kEsfJsonInternalError;
  ret = EsfJsonValueFind(handle->json_value, source, &data);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonValueFind func failed. ret = %u, handle->json_value = %p, "
        "source = %" PRId32 ".",
        ret, handle->json_value, source);
    return ret;
  }

  JSON_Value* copy_data = NULL;
  copy_data = json_value_deep_copy(data);
  if (copy_data == NULL) {
    ESF_JSON_ERR("json_value_deep_copy func failed. data = %p.", data);
    return kEsfJsonInternalError;
  }

  EsfJsonValue value_id = ESF_JSON_VALUE_INVALID;
  ret = EsfJsonValueAdd(handle->json_value, copy_data, &value_id);
  if (ret != kEsfJsonSuccess) {
    json_value_free(copy_data);
    ESF_JSON_ERR(
        "EsfJsonValueAdd func failed. ret = %u, handle->json_value = %p, "
        "copy_data = %p.",
        ret, handle->json_value, copy_data);
    return ret;
  }

  *destination = value_id;

  ESF_JSON_DEBUG("JSON Value Copy Info source ID = %" PRId32
                 " destination ID = %" PRId32 "",
                 source, value_id);
  ESF_JSON_TRACE("exit");
  return kEsfJsonSuccess;
}

EsfJsonErrorCode EsfJsonValueTypeGet(EsfJsonHandle handle, EsfJsonValue value,
                                     EsfJsonValueType* type) {
  ESF_JSON_TRACE("entry");
  // Parameter check.
  if (handle == ESF_JSON_HANDLE_INITIALIZER) {
    ESF_JSON_ERR("Parameter error. handle = %p", handle);
    return kEsfJsonHandleError;
  }

  if (type == NULL) {
    ESF_JSON_ERR("Parameter error. type = %p", type);
    return kEsfJsonInvalidArgument;
  }

  JSON_Value* data = NULL;
  EsfJsonErrorCode ret = kEsfJsonInternalError;
  ret = EsfJsonValueFind(handle->json_value, value, &data);
  if (ret != kEsfJsonSuccess) {
    ESF_JSON_ERR(
        "EsfJsonValueFind func failed. ret = %u, handle->json_value = %p, "
        "value = %" PRId32 ".",
        ret, handle->json_value, value);
    return ret;
  }

  JSON_Value_Type parson_type = JSONError;
  parson_type = json_value_get_type(data);
  if (parson_type == JSONError) {
    ESF_JSON_ERR("JSON type error data = %p.", data);
    return kEsfJsonInternalError;
  }
  *type = EsfJsonValueTypeConvert(parson_type);
  ESF_JSON_DEBUG("JSON Value Type Info type = %u", *type);
  ESF_JSON_TRACE("exit");
  return kEsfJsonSuccess;
}
