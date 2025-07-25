/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#include "parameter_storage_manager/src/parameter_storage_manager_internal_work.h"

#include "parameter_storage_manager/src/parameter_storage_manager_mutex.h"
#include "parameter_storage_manager/src/parameter_storage_manager_resource.h"
#include "parameter_storage_manager/src/parameter_storage_manager_storage_adapter.h"
#include "parameter_storage_manager/src/parameter_storage_manager_storage_adapter_item_type.h"
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

// """Performs the change expression for each loop and determines whether the
// loop should continue.

// Performs the change expression for each loop and determines whether the
// loop should continue. It has functionality similar to the iterator's prev().

// Args:
//     [IN/OUT] work (EsfParameterStorageManagerWorkContext*):
//         Data working context. Cannot pass NULL to this argument.

// Returns:
//     bool: If true, processing can continue. If false, processing cannot
//     continue because the end of the array has been reached.

// Note:
// """
static bool EsfParameterStorageManagerInternalWorkPrev(
    EsfParameterStorageManagerWorkContext* work);

// """Gets information about the update data.

// Gets information about the update data.
// It does not depend on the "index" and retrieves information for all members.

// Args:
//     [IN] handle (EsfParameterStorageManagerHandle): Parameter Storage Manager
//     operation handle. [OUT] info
//     (EsfParameterStorageManagerResourceUpdateInfo*):
//         working member context. Cannot pass NULL to this argument.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInternal: The argument info is an
//     invalid value. kEsfParameterStorageManagerStatusInternal: Internal
//     processing failed. kEsfParameterStorageManagerStatusDataLoss: Unable to
//     access data storage area. kEsfParameterStorageManagerStatusUnavailable:
//     Failed to save data.

// Note:
// """
static EsfParameterStorageManagerStatus
EsfParameterStorageManagerInternalGetWorkUpdateInformation(
    EsfParameterStorageManagerHandle handle,
    EsfParameterStorageManagerResourceUpdateInfo* info);

EsfParameterStorageManagerWorkContext*
EsfParameterStorageManagerInternalAllocateWork(
    EsfParameterStorageManagerMask mask, EsfParameterStorageManagerData data,
    const EsfParameterStorageManagerStructInfo* info, void* private_data) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  bool failure = false;
  EsfParameterStorageManagerWorkContext* ret = NULL;
  do {
    ret = malloc(sizeof(*ret));
    if (ret == NULL) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR("Failed to allocate working memory");
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INTERNAL_PROCESS_ERROR);
      failure = true;
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Allocated work context");

    ret->mask = mask;
    ret->data = data;
    ret->info = info;
    ret->private_data = private_data;
    ret->index = 0;
    ret->member_info = NULL;
    ret->member_data = NULL;

    if (info != NULL) {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE("Allocating member data");
      ret->member_data =
          EsfParameterStorageManagerInternalAllocateMemberData(info->items_num);
      if (ret->member_data == NULL) {
        ESF_PARAMETER_STORAGE_MANAGER_ERROR("Failed to allocate member data");
        failure = true;
        break;
      }
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Allocated member data");
    } else {
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG("No member data allocation needed");
    }
  } while (0);

  if (failure) {
    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Cleaning up due to failure");
    if (ret != NULL) {
      free(ret);
      ret = NULL;
    }
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit");
  return ret;
}

void EsfParameterStorageManagerInternalFreeWork(
    EsfParameterStorageManagerWorkContext* work) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  do {
    if (work == NULL) {
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
          "Work context is NULL, nothing to free");
      break;
    }

    const EsfParameterStorageManagerStructInfo* info = work->info;
    EsfParameterStorageManagerWorkMemberData* member_data = work->member_data;

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Freeing work context");
    free(work);

    // freeing large heap
    if (info != NULL && member_data != NULL) {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE("Freeing large heap");
      for (uint32_t i = 0; i < info->items_num; ++i) {
        EsfParameterStorageManagerStatus ret =
            EsfParameterStorageManagerBufferFree(&member_data[i].buffer);
        if (ret != kEsfParameterStorageManagerStatusOk) {
          ESF_PARAMETER_STORAGE_MANAGER_WARN(
              "Failed to free large heap. i=%" PRIu32 ", ret=%u(%s)", i, ret,
              EsfParameterStorageManagerStrError(ret));
        }
      }
    } else {
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG("No large heap to free");
    }

    if (member_data != NULL) {
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Freeing member data");
      EsfParameterStorageManagerInternalFreeMemberData(member_data);
    } else {
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG("No member data to free");
    }
  } while (0);
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit");
}

EsfParameterStorageManagerWorkMemberData*
EsfParameterStorageManagerInternalAllocateMemberData(size_t size) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerWorkMemberData* member_data = NULL;
  do {
    if (size == 0) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR("The size is too small. size=%zu",
                                          size);
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Allocating member data. size=%zu",
                                        size);
    member_data = malloc(sizeof(*member_data) * size);
    if (member_data == NULL) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to allocate member memory. size=%zu", size);
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INTERNAL_PROCESS_ERROR);
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Initializing member data");
    for (uint32_t i = 0; i < size; ++i) {
      member_data[i].cancel = kEsfParameterStorageManagerCancelSkip;
      member_data[i].enabled = false;
      member_data[i].append = false;
      member_data[i].update = false;
      member_data[i].storage.capabilities.read_only = 0;
      member_data[i].storage.capabilities.enable_offset = 0;
      member_data[i].storage.written_size = 0;
      member_data[i].buffer = (EsfParameterStorageManagerBuffer)
          ESF_PARAMETER_STORAGE_MANAGER_BUFFER_INITIALIZER;
      member_data[i].update_data = (uintptr_t)NULL;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Member data initialized successfully");
  } while (0);

  if (member_data == NULL) {
    ESF_PARAMETER_STORAGE_MANAGER_WARN("Failed to allocate member data");
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit");
  return member_data;
}

void EsfParameterStorageManagerInternalFreeMemberData(
    EsfParameterStorageManagerWorkMemberData* member) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  if (member != NULL) {
    free(member);
  }
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit");
}

void EsfParameterStorageManagerInternalWorkBegin(
    EsfParameterStorageManagerWorkContext* work) {
  work->index = UINT32_MAX;
  work->member_info = NULL;
}

bool EsfParameterStorageManagerInternalWorkNext(
    EsfParameterStorageManagerWorkContext* work) {
  if (work->info->items_num == 0 ||
      (work->index != UINT32_MAX && work->info->items_num - 1 <= work->index)) {
    return false;
  }
  if (work->index == UINT32_MAX) {
    work->index = 0;
  } else {
    ++work->index;
  }
  work->member_info = &work->info->items[work->index];
  return true;
}

static bool EsfParameterStorageManagerInternalWorkPrev(
    EsfParameterStorageManagerWorkContext* work) {
  if (work->index == 0 || work->info->items_num < work->index) {
    return false;
  }
  --work->index;
  work->member_info = &work->info->items[work->index];
  return true;
}

int32_t EsfParameterStorageManagerInternalSetupWorkMask(
    EsfParameterStorageManagerWorkContext* work) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  int32_t ret = 0;
  EsfParameterStorageManagerInternalWorkBegin(work);
  while (EsfParameterStorageManagerInternalWorkNext(work)) {
    work->member_data[work->index].enabled =
        work->member_info->enabled(work->mask);
    if (work->member_data[work->index].enabled) {
      ++ret;
    }
  }
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit ret=%" PRId32, ret);
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerInternalGetWorkStorageInfo(
    EsfParameterStorageManagerHandle handle,
    EsfParameterStorageManagerWorkContext* work) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;
  EsfParameterStorageManagerResourceUpdateInfo info;

  do {
    ret = EsfParameterStorageManagerInternalGetWorkUpdateInformation(handle,
                                                                     &info);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to get update information. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Got update information successfully");

    EsfParameterStorageManagerInternalWorkBegin(work);
    while (EsfParameterStorageManagerInternalWorkNext(work)) {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE("Processing work item index %" PRIu32,
                                          work->index);

      if (!work->member_data[work->index].enabled) {
        ESF_PARAMETER_STORAGE_MANAGER_TRACE("Skipping disabled item");
        continue;
      }
      if (work->member_info->id == kEsfParameterStorageManagerItemCustom) {
        ESF_PARAMETER_STORAGE_MANAGER_TRACE("Skipping custom item");
        continue;
      }

      for (int i = 0; i < info.count; ++i) {
        if (info.id[i] == work->member_info->id) {
          work->member_data[work->index].update = true;
          work->member_data[work->index].update_data = info.data[i];
          ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Update found for item ID %u",
                                              work->member_info->id);
          break;
        }
      }

      ret = EsfParameterStorageManagerStorageAdapterGetStorageInfo(
          work->member_info->id, &work->member_data[work->index],
          &work->member_data[work->index].storage);
      if (ret != kEsfParameterStorageManagerStatusOk) {
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Failed to get storage information. i=%" PRIu32
            ", item_id=%u, ret=%u(%s)",
            work->index, work->member_info->id, ret,
            EsfParameterStorageManagerStrError(ret));
        break;
      }

      ret = EsfParameterStorageManagerStorageAdapterGetItemCapabilities(
          work->member_info->id,
          &work->member_data[work->index].storage.capabilities);
      if (ret != kEsfParameterStorageManagerStatusOk) {
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Failed to get item capabilities. i=%" PRIu32
            ", item_id=%u, ret=%u(%s)",
            work->index, work->member_info->id, ret,
            EsfParameterStorageManagerStrError(ret));
        break;
      }

      ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
          "Got storage info and capabilities for item ID %u",
          work->member_info->id);
    }
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus EsfParameterStorageManagerInternalCancel(
    EsfParameterStorageManagerWorkContext* work) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  while (EsfParameterStorageManagerInternalWorkPrev(work)) {
    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Processing item index %" PRIu32,
                                        work->index);

    if (!work->member_data[work->index].enabled ||
        (work->member_data[work->index].cancel ==
             kEsfParameterStorageManagerCancelSkip &&
         work->member_info->id != kEsfParameterStorageManagerItemCustom)) {
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Skipping item %" PRIu32,
                                          work->index);
      continue;
    }

    if (work->member_info->id == kEsfParameterStorageManagerItemCustom) {
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Cancelling custom item %" PRIu32,
                                          work->index);
      ret = work->member_info->custom->cancel(work->private_data);
      if (ret != kEsfParameterStorageManagerStatusOk) {
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Failed to cancel custom data. i=%" PRIu32 ", ret=%u(%s)",
            work->index, ret, EsfParameterStorageManagerStrError(ret));
        break;
      }
      continue;
    }

    if (work->member_data[work->index].cancel ==
        kEsfParameterStorageManagerCancelClear) {
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Clearing item %" PRIu32,
                                          work->index);
      ret = EsfParameterStorageManagerStorageAdapterClear(
          work->member_info->id, &work->member_data[work->index]);
      if (ret != kEsfParameterStorageManagerStatusOk) {
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Cancellation due to data deletion has failed. i=%" PRIu32
            ", ret=%u(%s)",
            work->index, ret, EsfParameterStorageManagerStrError(ret));
        break;
      }
      continue;
    }

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Saving backup data for item %" PRIu32,
                                        work->index);
    // Overwrite mode, not append mode
    work->member_data[work->index].append = false;
    ret = EsfParameterStorageManagerBufferSave(
        &work->member_data[work->index].buffer, work->member_info->id,
        &work->member_data[work->index]);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to save backup data. i=%" PRIu32 ", item_id=%u, ret=%u(%s)",
          work->index, work->member_info->id, ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
  }

  if (ret == kEsfParameterStorageManagerStatusOk) {
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Cancel operation completed successfully");
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus EsfParameterStorageManagerInternalLoadBackup(
    EsfParameterStorageManagerWorkContext* work) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    size_t size = work->member_data[work->index].storage.written_size;
    ESF_PARAMETER_STORAGE_MANAGER_TRACE(
        "Allocating large heap for backup. Size: %zu", size);

    ret = EsfParameterStorageManagerBufferAllocate(
        size, &work->member_data[work->index].buffer);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to allocate large heap for backup. Size: %zu, ret=%u(%s)",
          size, ret, EsfParameterStorageManagerStrError(ret));
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Large heap allocated successfully");

    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Loading backup data for item ID %u",
                                        work->member_info->id);
    ret = EsfParameterStorageManagerBufferLoad(
        &work->member_data[work->index].buffer, work->member_info->id,
        &work->member_data[work->index]);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to load data for backup. Item ID: %u, ret=%u(%s)",
          work->member_info->id, ret, EsfParameterStorageManagerStrError(ret));
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Backup data loaded successfully");
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerInternalSupportedSave(
    EsfParameterStorageManagerWorkContext* work) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Processing item ID %u, index %" PRIu32,
                                        work->member_info->id, work->index);

    if (work->member_data[work->index].storage.capabilities.read_only) {
      ret = kEsfParameterStorageManagerStatusPermissionDenied;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "This data is read only. i=%" PRIu32 ", item_id=%u, ret=%u(%s)",
          work->index, work->member_info->id, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }

    if (work->member_data[work->index].storage.written_size == 0) {
      work->member_data[work->index].cancel =
          kEsfParameterStorageManagerCancelClear;
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
          "Loading is skipped because it has already been deleted. "
          "i=%" PRIu32 ", item_id=%u",
          work->index, work->member_info->id);
    } else {
      work->member_data[work->index].cancel =
          kEsfParameterStorageManagerCancelSave;

      ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Loading backup for item ID %u",
                                          work->member_info->id);
      ret = EsfParameterStorageManagerInternalLoadBackup(work);
      if (ret != kEsfParameterStorageManagerStatusOk) {
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Failed to load data for backup. i=%" PRIu32
            ", item_id=%u, ret=%u(%s)",
            work->index, work->member_info->id, ret,
            EsfParameterStorageManagerStrError(ret));
        break;
      }
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
          "Backup loaded successfully for item ID %u", work->member_info->id);
    }

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Saving data for item ID %u, type %u",
                                        work->member_info->id,
                                        work->member_info->type);
    ret = EsfParameterStorageManagerStorageAdapterSaveItemType(
        work->member_info->type,
        (const void*)(work->data + work->member_info->offset), work);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to save data. i=%" PRIu32 ", item_id=%u, ret=%u(%s)",
          work->index, work->member_info->id, ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Data saved successfully for item ID %u", work->member_info->id);
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerInternalSupportedClear(
    EsfParameterStorageManagerWorkContext* work) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Processing clear for item ID %u, index %" PRIu32,
        work->member_info->id, work->index);

    if (work->member_data[work->index].storage.capabilities.read_only) {
      ret = kEsfParameterStorageManagerStatusPermissionDenied;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "This data is read only. i=%" PRIu32 ", item_id=%u, ret=%u(%s)",
          work->index, work->member_info->id, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }

    if (work->member_data[work->index].storage.written_size == 0) {
      ret = kEsfParameterStorageManagerStatusOk;
      work->member_data[work->index].cancel =
          kEsfParameterStorageManagerCancelSkip;
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
          "Processing is skipped because it has already been deleted. "
          "i=%" PRIu32 ", item_id=%u",
          work->index, work->member_info->id);
      break;
    }

    work->member_data[work->index].cancel =
        kEsfParameterStorageManagerCancelSave;

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Loading backup for item ID %u",
                                        work->member_info->id);
    ret = EsfParameterStorageManagerInternalLoadBackup(work);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to load data for backup. i=%" PRIu32
          ", item_id=%u, ret=%u(%s)",
          work->index, work->member_info->id, ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Backup loaded successfully for item ID %u", work->member_info->id);

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Clearing data for item ID %u",
                                        work->member_info->id);
    ret = EsfParameterStorageManagerStorageAdapterClear(
        work->member_info->id, &work->member_data[work->index]);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to clear data. i=%" PRIu32 ", item_id=%u, ret=%u(%s)",
          work->index, work->member_info->id, ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Data cleared successfully for item ID %u", work->member_info->id);
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerInternalAlreadyUpdateBegin(
    EsfParameterStorageManagerHandle handle,
    EsfParameterStorageManagerWorkContext* work) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Checking if handle is already being updated");
    ret = EsfParameterStorageManagerResourceHandleIsAlreadyBeingUpdated(handle);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "The update has already started. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }

    // Update begin Loop
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Starting update begin loop");
    EsfParameterStorageManagerInternalWorkBegin(work);
    while (EsfParameterStorageManagerInternalWorkNext(work)) {
      if (!work->member_data[work->index].enabled) {
        ESF_PARAMETER_STORAGE_MANAGER_TRACE(
            "Skipping disabled item at index %" PRIu32, work->index);
        continue;
      }
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
          "Checking if item ID %u is already being updated",
          work->member_info->id);
      ret = EsfParameterStorageManagerResourceUpdateDataIsExistsInHandles(
          work->member_info->id);
      if (ret != kEsfParameterStorageManagerStatusOk) {
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "The update has already started for item ID %u. ret=%u(%s)",
            work->member_info->id, ret,
            EsfParameterStorageManagerStrError(ret));
        break;
      }
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Update begin loop completed");
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerInternalUpdateBeginIsValid(
    EsfParameterStorageManagerWorkContext* work, int32_t enabled_mask_num) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;
  do {
    if (CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_UPDATE_MAX <
        enabled_mask_num) {
      ret = kEsfParameterStorageManagerStatusOutOfRange;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Too many data to update: enabled_mask_num=%" PRId32,
          enabled_mask_num);
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }
    EsfParameterStorageManagerInternalWorkBegin(work);
    while (EsfParameterStorageManagerInternalWorkNext(work) &&
           ret == kEsfParameterStorageManagerStatusOk) {
      if (!work->member_data[work->index].enabled) {
        continue;
      }
      if (kEsfParameterStorageManagerItemCustom <= work->member_info->id) {
        ret = kEsfParameterStorageManagerStatusInvalidArgument;
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "The ItemID is cannot update. i=%" PRIu32 ", id=%u", work->index,
            work->member_info->id);
        ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
            ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
        break;
      }
      for (uint32_t i = 0; i < work->info->items_num; ++i) {
        if (work->index == i || !work->member_data[i].enabled) {
          continue;
        }
        if (work->member_info->id == work->info->items[i].id) {
          ret = kEsfParameterStorageManagerStatusInvalidArgument;
          ESF_PARAMETER_STORAGE_MANAGER_ERROR(
              "The ItemID is duplicated. i=%" PRIu32 ", id=%u", i,
              work->info->items[i].id);
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
              ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
          break;
        }
      }
    }
  } while (0);
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus EsfParameterStorageManagerInternalUpdateCancel(
    EsfParameterStorageManagerHandle handle,
    EsfParameterStorageManagerWorkContext* work) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Starting update cancel process");
    while (EsfParameterStorageManagerInternalWorkPrev(work)) {
      if (!work->member_data[work->index].enabled) {
        ESF_PARAMETER_STORAGE_MANAGER_TRACE(
            "Skipping disabled item at index %" PRIu32, work->index);
        continue;
      }

      ESF_PARAMETER_STORAGE_MANAGER_TRACE("Cancelling update for item ID %u",
                                          work->member_info->id);
      ret = EsfParameterStorageManagerStorageAdapterUpdateCancel(
          work->member_info->id, &work->member_data[work->index]);
      if (ret != kEsfParameterStorageManagerStatusOk) {
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Failed to cancel updating. item_id=%u, ret=%u(%s)",
            work->member_info->id, ret,
            EsfParameterStorageManagerStrError(ret));
        break;
      }
      ESF_PARAMETER_STORAGE_MANAGER_TRACE(
          "Update cancelled successfully for item ID %u",
          work->member_info->id);
    }

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Removing update data from handle");
    EsfParameterStorageManagerStatus remove_ret =
        EsfParameterStorageManagerResourceRemoveUpdateDataFromHandle(handle);
    if (remove_ret != kEsfParameterStorageManagerStatusOk) {
      ret = remove_ret;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to remove update information. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Update data removed successfully from handle");
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static EsfParameterStorageManagerStatus
EsfParameterStorageManagerInternalGetWorkUpdateInformation(
    EsfParameterStorageManagerHandle handle,
    EsfParameterStorageManagerResourceUpdateInfo* info) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    if (handle == ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid handle. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }
    if (info == NULL) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Argument \"info\" is invalid. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Getting update data from handle");
    ret =
        EsfParameterStorageManagerResourceGetUpdateDataFromHandle(handle, info);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to get update information. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Update data retrieved successfully");
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}
