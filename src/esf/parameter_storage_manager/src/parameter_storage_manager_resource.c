/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#include "parameter_storage_manager/src/parameter_storage_manager_resource.h"

#include "parameter_storage_manager/src/parameter_storage_manager_config.h"
#include "parameter_storage_manager/src/parameter_storage_manager_utility.h"

// The functions contained in this file access resources.
// These functions should be called when exclusive control is in effect.
// To perform exclusive control, use the functions contained in file
// "parameter_storage_manager_mutex.h".

// This is a structure that manages the handle state.
typedef struct EsfParameterStorageManagerResourceHandle {
  // Handle valid flag.
  // Set this flag to true in EsfParameterStorageManagerResourceNewHandle().
  // Set this flag to false in EsfParameterStorageManagerResourceDeleteHandle().
  bool is_valid;

  // The number of the handle in use.
  // When allocated by EsfParameterStorageManagerResourceNewHandle(), it is set
  // to 0. It is incremented by
  // EsfParameterStorageManagerResourceReferenceHandle() and decremented by
  // EsfParameterStorageManagerResourceUnreferenceHandle(). If it is 0, it can
  // be released by EsfParameterStorageManagerResourceDeleteHandle().
  int32_t ref_count;

  // Update information.
  EsfParameterStorageManagerResourceUpdateInfo update;
} EsfParameterStorageManagerResourceHandle;

// This is a structure that manages the resource.
typedef struct EsfParameterStorageManagerResource {
  // Parameter Storage Manager's handle.
  EsfParameterStorageManagerResourceHandle
      handle[CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_HANDLE_MAX];

  // Factory reset functions.
  EsfParameterStorageManagerResourceFactoryReset factory_reset
      [CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_FACTORY_RESET_MAX];

  // The working buffer size when using Large Heap file I/O.
  uint8_t* buffer;
} EsfParameterStorageManagerResource;

// Global variable to pointer to resource.
static EsfParameterStorageManagerResource* resource = NULL;

EsfParameterStorageManagerStatus EsfParameterStorageManagerResourceInit(void) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    if (resource != NULL) {
      ESF_PARAMETER_STORAGE_MANAGER_WARN(
          "The resource has already been allocated.");
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Allocating resource");
    resource =
        (EsfParameterStorageManagerResource*)calloc(1, sizeof(*resource));
    if (resource == NULL) {
      ret = kEsfParameterStorageManagerStatusResourceExhausted;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to allocate resources. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INTERNAL_PROCESS_ERROR);
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Resource allocated successfully");

    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Initializing handles");
    for (int i = 0; i < CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_HANDLE_MAX;
         ++i) {
      resource->handle[i].is_valid = false;
      resource->handle[i].ref_count = 0;
      resource->handle[i].update.count = 0;
      for (int j = 0; j < CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_UPDATE_MAX;
           ++j) {
        resource->handle[i].update.id[j] = kEsfParameterStorageManagerItemMax;
        resource->handle[i].update.data[j] = (uintptr_t)NULL;
      }
    }
    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Handles initialized");

    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Initializing factory reset functions");
    for (int i = 0;
         i < CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_FACTORY_RESET_MAX; ++i) {
      resource->factory_reset[i].func = NULL;
      resource->factory_reset[i].private_data = NULL;
    }
    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Factory reset functions initialized");
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus EsfParameterStorageManagerResourceDeinit(
    void) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    if (resource == NULL) {
      ESF_PARAMETER_STORAGE_MANAGER_WARN(
          "No resource is allocated. Nothing to deinitialize.");
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Starting resource deinitialization");

    if (resource->buffer != NULL) {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE("Freeing resource buffer");
      free(resource->buffer);
      resource->buffer = NULL;
      ESF_PARAMETER_STORAGE_MANAGER_TRACE("Resource buffer freed successfully");
    } else {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE("No resource buffer to free");
    }

    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Freeing main resource structure");
    free(resource);
    resource = NULL;
    ESF_PARAMETER_STORAGE_MANAGER_TRACE(
        "Main resource structure freed successfully");
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus EsfParameterStorageManagerResourceNewHandle(
    EsfParameterStorageManagerHandle* handle) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    if (handle == NULL) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Argument \"handle\" is NULL. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }
    if (resource == NULL) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "No resources are allocated. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Searching for an available handle");
    ret = kEsfParameterStorageManagerStatusResourceExhausted;
    for (int i = 0; i < CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_HANDLE_MAX;
         ++i) {
      if (resource->handle[i].is_valid) {
        ESF_PARAMETER_STORAGE_MANAGER_TRACE("Handle[%d] is already valid", i);
        continue;
      }
      resource->handle[i].is_valid = true;
      resource->handle[i].ref_count = 0;
      resource->handle[i].update.count = 0;
      for (int j = 0; j < CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_UPDATE_MAX;
           ++j) {
        resource->handle[i].update.id[j] = kEsfParameterStorageManagerItemMax;
        resource->handle[i].update.data[j] = (uintptr_t)NULL;
      }
      *handle = (EsfParameterStorageManagerHandle)i;
      ret = kEsfParameterStorageManagerStatusOk;
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Assigned new handle: %d", i);
      break;
    }

    if (ret == kEsfParameterStorageManagerStatusResourceExhausted) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "There are no handles left to assign. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
    }
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus EsfParameterStorageManagerResourceDeleteHandle(
    EsfParameterStorageManagerHandle handle) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("Attempting to delete handle=%" PRId32,
                                      handle);
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    if (handle == ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Argument \"handle\" is invalid. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }
    if (resource == NULL) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "No resources are allocated. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }
    if (handle < 0 ||
        CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_HANDLE_MAX <= handle) {
      ret = kEsfParameterStorageManagerStatusNotFound;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Handle is out of range. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Handle state: valid=%d, ref_count=%" PRId32,
        resource->handle[handle].is_valid, resource->handle[handle].ref_count);

    if (resource->handle[handle].is_valid == false) {
      ESF_PARAMETER_STORAGE_MANAGER_WARN(
          "The handle has already been released. handle=%" PRId32, handle);
      break;
    }

    if (resource->handle[handle].ref_count != 0) {
      ret = kEsfParameterStorageManagerStatusFailedPrecondition;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "This handle is still referenced. handle=%" PRId32 ", count=%" PRId32
          ", ret=%u(%s)",
          handle, resource->handle[handle].ref_count, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }

    resource->handle[handle].is_valid = false;
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Handle %" PRId32 " successfully deleted", handle);
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerResourceReferenceHandle(
    EsfParameterStorageManagerHandle handle) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("Attempting to reference handle=%" PRId32,
                                      handle);
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    if (handle == ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Argument \"handle\" is invalid. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }
    if (resource == NULL) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "No resources are allocated. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }
    if (handle < 0 ||
        CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_HANDLE_MAX <= handle) {
      ret = kEsfParameterStorageManagerStatusNotFound;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Handle is out of range. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }
    if (resource->handle[handle].is_valid == false) {
      ret = kEsfParameterStorageManagerStatusNotFound;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "The handle has already been released. handle=%" PRId32
          ", ret=%u(%s)",
          handle, ret, EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }

    resource->handle[handle].ref_count += 1;
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Handle %" PRId32 " referenced. New ref_count=%" PRId32, handle,
        resource->handle[handle].ref_count);
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerResourceUnreferenceHandle(
    EsfParameterStorageManagerHandle handle) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  ESF_PARAMETER_STORAGE_MANAGER_TRACE(
      "Attempting to unreference handle=%" PRId32, handle);
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    if (handle == ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Argument \"handle\" is invalid. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }
    if (resource == NULL) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "No resources are allocated. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }
    if (handle < 0 ||
        CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_HANDLE_MAX <= handle) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Handle is out of range. handle=%" PRId32 ", ret=%u(%s)", handle, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }
    if (resource->handle[handle].is_valid == false) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "The handle has already been released. handle=%" PRId32
          ", ret=%u(%s)",
          handle, ret, EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }
    if (resource->handle[handle].ref_count < 1) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "This handle is not referenced. handle=%" PRId32
          ", ref_count=%" PRId32 ", ret=%u(%s)",
          handle, resource->handle[handle].ref_count, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }
    resource->handle[handle].ref_count -= 1;
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Handle %" PRId32 " unreferenced. New ref_count=%" PRId32, handle,
        resource->handle[handle].ref_count);
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerResourceNewFactoryReset(
    EsfParameterStorageManagerRegisterFactoryResetType func, void* private_data,
    EsfParameterStorageManagerFactoryResetID* id) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  ESF_PARAMETER_STORAGE_MANAGER_TRACE(
      "Attempting to register new factory reset. func=%s, private_data=%p, "
      "id=%p",
      func == NULL ? "Invalid" : "Valid", (void*)private_data, (void*)id);
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    if (func == NULL || id == NULL) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Argument \"func\" or \"id\" is NULL. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }
    if (resource == NULL) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "No resources are allocated. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_TRACE(
        "Searching for available factory reset ID");
    ret = kEsfParameterStorageManagerStatusResourceExhausted;
    for (int i = 0;
         i < CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_FACTORY_RESET_MAX; ++i) {
      if (resource->factory_reset[i].func != NULL) {
        ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
            "Factory reset ID %d is already in use.", i);
        continue;
      }
      resource->factory_reset[i].func = func;
      resource->factory_reset[i].private_data = private_data;
      *id = (EsfParameterStorageManagerFactoryResetID)i;
      ret = kEsfParameterStorageManagerStatusOk;
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Assigned new factory reset ID: %d",
                                          i);
      break;
    }
    if (ret == kEsfParameterStorageManagerStatusResourceExhausted) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "There are no factory reset IDs left to assign. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
    }
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerResourceDeleteFactoryReset(
    EsfParameterStorageManagerFactoryResetID id) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  ESF_PARAMETER_STORAGE_MANAGER_TRACE(
      "Attempting to delete factory reset with id=%" PRId32, id);
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    if (id == ESF_PARAMETER_STORAGE_MANAGER_INVALID_FACTORY_RESET_ID) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Argument \"id\" is invalid. id=%" PRId32 ", ret=%u(%s)", id, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }
    if (resource == NULL) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "No resources are allocated. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }
    if (id < 0 ||
        CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_FACTORY_RESET_MAX <= id) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_WARN(
          "Factory reset ID is out of range. id=%" PRId32, id);
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }
    if (resource->factory_reset[id].func == NULL) {
      ESF_PARAMETER_STORAGE_MANAGER_WARN(
          "The factory reset ID has already been released. id=%" PRId32, id);
      break;
    }

    resource->factory_reset[id].func = NULL;
    resource->factory_reset[id].private_data = NULL;
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Factory reset with id=%" PRId32 " successfully deleted", id);
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerResourceGetFactoryReset(
    EsfParameterStorageManagerFactoryResetID id,
    EsfParameterStorageManagerResourceFactoryReset* factory_reset) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  ESF_PARAMETER_STORAGE_MANAGER_TRACE(
      "Attempting to get factory reset with id=%" PRId32, id);
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    if (id < 0 ||
        CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_FACTORY_RESET_MAX <= id ||
        factory_reset == NULL) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Argument is invalid. id=%" PRId32 ", factory_reset=%p, ret=%u(%s)",
          id, (void*)factory_reset, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }
    if (resource == NULL) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "No resources are allocated. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }
    *factory_reset = resource->factory_reset[id];
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Successfully retrieved factory reset for id=%" PRId32, id);
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerResourceSetUpdateDataToHandle(
    EsfParameterStorageManagerHandle handle,
    EsfParameterStorageManagerItemID id, uintptr_t data) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  ESF_PARAMETER_STORAGE_MANAGER_TRACE(
      "Attempting to set update data. handle=%" PRId32 ", id=%u, data=%p",
      handle, id, (void*)data);
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    if (handle == ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Argument is invalid. handle=%" PRId32 ", ret=%u(%s)", handle, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }
    if (resource == NULL) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "No resources are allocated. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }
    if (resource->handle[handle].update.count < 0 ||
        CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_UPDATE_MAX <=
            resource->handle[handle].update.count) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "There is not enough space to hold the update information. "
          "handle=%" PRId32 ", count=%d, ret=%u(%s)",
          handle, resource->handle[handle].update.count, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }
    const int i = resource->handle[handle].update.count;
    resource->handle[handle].update.count += 1;
    resource->handle[handle].update.id[i] = id;
    resource->handle[handle].update.data[i] = data;
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Successfully set update data. handle=%" PRId32 ", id=%u, index=%d",
        handle, id, i);
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerResourceGetUpdateDataFromHandle(
    EsfParameterStorageManagerHandle handle,
    EsfParameterStorageManagerResourceUpdateInfo* info) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  ESF_PARAMETER_STORAGE_MANAGER_TRACE(
      "Attempting to get update data. handle=%" PRId32, handle);
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    if (handle == ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE ||
        info == NULL) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Argument is invalid. handle=%" PRId32 ", info=%p, ret=%u(%s)",
          handle, (void*)info, ret, EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }
    if (resource == NULL) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "No resources are allocated. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }
    memcpy(info, &resource->handle[handle].update, sizeof(*info));
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Successfully retrieved update data. handle=%" PRId32 ", count=%d",
        handle, info->count);
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerResourceRemoveUpdateDataFromHandle(
    EsfParameterStorageManagerHandle handle) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  ESF_PARAMETER_STORAGE_MANAGER_TRACE(
      "Attempting to remove update data. handle=%" PRId32, handle);
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    if (handle == ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Argument is invalid. handle=%" PRId32 ", ret=%u(%s)", handle, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }
    if (resource == NULL) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "No resources are allocated. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_TRACE(
        "Removing update data for handle=%" PRId32, handle);
    resource->handle[handle].update.count = 0;
    for (int i = 0; i < CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_UPDATE_MAX;
         ++i) {
      resource->handle[handle].update.id[i] =
          kEsfParameterStorageManagerItemMax;
      resource->handle[handle].update.data[i] = (uintptr_t)NULL;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Successfully removed update data for handle=%" PRId32, handle);
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerResourceHandleIsAlreadyBeingUpdated(
    EsfParameterStorageManagerHandle handle) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  ESF_PARAMETER_STORAGE_MANAGER_TRACE(
      "Checking if handle is already being updated. handle=%" PRId32, handle);
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    if (handle == ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Argument is invalid. handle=%" PRId32 ", ret=%u(%s)", handle, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }
    if (resource == NULL) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "No resources are allocated. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }
    if (resource->handle[handle].update.count != 0) {
      ret = kEsfParameterStorageManagerStatusFailedPrecondition;
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
          "The update has already started for handle=%" PRId32
          ". count=%" PRId32 ", ret=%u(%s)",
          handle, resource->handle[handle].update.count, ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerResourceUpdateDataIsExistsInHandles(
    EsfParameterStorageManagerItemID id) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  ESF_PARAMETER_STORAGE_MANAGER_TRACE(
      "Checking if update data exists for item ID=%u", id);
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    if (resource == NULL) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "No resources are allocated. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }

    for (int i = 0; i < CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_HANDLE_MAX &&
                    ret == kEsfParameterStorageManagerStatusOk;
         ++i) {
      for (int j = 0; j < resource->handle[i].update.count; ++j) {
        if (resource->handle[i].update.id[j] == id) {
          ret = kEsfParameterStorageManagerStatusFailedPrecondition;
          ESF_PARAMETER_STORAGE_MANAGER_ERROR(
              "The update has already started for item ID=%u in handle=%" PRId32
              ". ret=%u(%s)",
              id, i, ret, EsfParameterStorageManagerStrError(ret));
          break;
        }
      }
    }
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

bool EsfParameterStorageManagerResourceIsInitialized(void) {
  return (resource != NULL);
}

uint8_t* EsfParameterStorageManagerResourceGetBuffer(void) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  uint8_t* ret = NULL;

  do {
    if (resource == NULL) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR("No resources are allocated.");
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }

    if (resource->buffer == NULL) {
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
          "Allocating new buffer of size %d",
          CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_BUFFER_LENGTH);
      resource->buffer = (uint8_t*)malloc(
          CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_BUFFER_LENGTH);
      if (resource->buffer == NULL) {
        ESF_PARAMETER_STORAGE_MANAGER_ERROR("Failed to allocate buffer.");
        ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
            ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INTERNAL_PROCESS_ERROR);
        break;
      }
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Buffer allocated successfully");
    } else {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE("Using existing buffer");
    }
    ret = resource->buffer;
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %p", (void*)ret);
  return ret;
}
