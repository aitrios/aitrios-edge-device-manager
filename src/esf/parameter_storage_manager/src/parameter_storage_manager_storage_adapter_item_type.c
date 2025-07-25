/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#include "parameter_storage_manager/src/parameter_storage_manager_storage_adapter_item_type.h"

#include "parameter_storage_manager/src/parameter_storage_manager_config.h"
#include "parameter_storage_manager/src/parameter_storage_manager_internal_work.h"
#include "parameter_storage_manager/src/parameter_storage_manager_storage_adapter.h"
#include "parameter_storage_manager/src/parameter_storage_manager_storage_adapter_settings.h"
#include "parameter_storage_manager/src/parameter_storage_manager_utility.h"

// """Saves binary data (array type) to the data storage area.

// Saves binary data (array type) to the data storage area.
// Before saving data, save the data before saving to private_data. When an
// error occurs in the saving process of another data member and
// SelfDevaluationSettingItemAccessorCancel is called, use the saved data to
// restore the state before saving.

// Args:
//     [IN] item (void*): Data member specified by offsetof
//                        EsfParameterStorageManagerBinaryArray.
//     [IN/OUT] work (EsfParameterStorageManagerWorkContext*): User data.
//     esfParameterStorageManagerWorkContext.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInternal: Argument is an invalid value.
//     kEsfParameterStorageManagerStatusResourceExhausted: Failed to allocate
//     internal resources. kEsfParameterStorageManagerStatusPermissionDenied:
//     Could not restore the data to the state
//                                 before saving.
//     kEsfParameterStorageManagerStatusDataLoss: Data could not be restored to
//     its pre-saved state. kEsfParameterStorageManagerStatusDataLoss: Unable to
//     access data storage area. kEsfParameterStorageManagerStatusUnavailable:
//     Failed to save data. kEsfParameterStorageManagerStatusOutOfRange: The
//     range of the data storage area has been
//                           exceeded.
//     kEsfParameterStorageManagerStatusInternal: Internal processing failed.

// Note:
// """
static EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterSaveBinaryArray(
    const void* item, EsfParameterStorageManagerWorkContext* work);

// """Loads binary data (array type) from the data storage area.

// Loads binary data (array type) from the data storage area.

// Args:
//     [IN] item (void*): Data member specified by offsetof
//                        EsfParameterStorageManagerBinaryArray.
//     [IN/OUT] work (EsfParameterStorageManagerWorkContext*): User data.
//     esfParameterStorageManagerWorkContext.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInternal: Argument is an invalid value.
//     kEsfParameterStorageManagerStatusDataLoss: Unable to access data storage
//     area. kEsfParameterStorageManagerStatusUnavailable: Failed to save data.
//     kEsfParameterStorageManagerStatusOutOfRange: The range of the data
//     storage area has been
//                           exceeded.
//     kEsfParameterStorageManagerStatusInternal: Internal processing failed.

// Note:
// """
static EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterLoadBinaryArray(
    void* item, EsfParameterStorageManagerWorkContext* work);

// """Saves binary data (pointer type) to the data storage area.

// Saves binary data (pointer type) to the data storage area. Before saving
// data, save the data before saving to private_data. When an error occurs in
// the saving process of another data member and
// SelfDevaluationSettingItemAccessorCancel is called, use the saved data to
// restore the state before saving.

// Args:
//     [IN] item (void*): Data member specified by offsetof
//                        EsfParameterStorageManagerBinary.
//     [IN/OUT] work (EsfParameterStorageManagerWorkContext*): User data.
//     esfParameterStorageManagerWorkContext.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInvalidArgument: The argument data by
//     the user is an
//                                invalid value.
//     kEsfParameterStorageManagerStatusInternal: Argument is an invalid value.
//     kEsfParameterStorageManagerStatusResourceExhausted: Failed to allocate
//     internal resources. kEsfParameterStorageManagerStatusPermissionDenied:
//     Could not restore the data to the state
//                                 before saving.
//     kEsfParameterStorageManagerStatusDataLoss: Data could not be restored to
//     its pre-saved state. kEsfParameterStorageManagerStatusDataLoss: Unable to
//     access data storage area. kEsfParameterStorageManagerStatusUnavailable:
//     Failed to save data. kEsfParameterStorageManagerStatusOutOfRange: The
//     range of the data storage area has been
//                           exceeded.
//     kEsfParameterStorageManagerStatusInternal: Internal processing failed.

// Note:
// """
static EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterSaveBinary(
    const void* item, EsfParameterStorageManagerWorkContext* work);

// """Loads binary data (pointer type) from the data storage area.

// Loads binary data (pointer type) from the data storage area.

// Args:
//     [IN] item (void*): Data member specified by offsetof
//                        EsfParameterStorageManagerBinary.
//     [IN/OUT] work (EsfParameterStorageManagerWorkContext*): User data.
//     esfParameterStorageManagerWorkContext.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInternal: Argument is an invalid value.
//     kEsfParameterStorageManagerStatusDataLoss: Unable to access data storage
//     area. kEsfParameterStorageManagerStatusUnavailable: Failed to save data.
//     kEsfParameterStorageManagerStatusOutOfRange: The range of the data
//     storage area has been
//                           exceeded.
//     kEsfParameterStorageManagerStatusInternal: Internal processing failed.

// Note:
// """
static EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterLoadBinary(
    void* item, EsfParameterStorageManagerWorkContext* work);

// """Saves binary data (array type) with specified offset to the data storage
// area.

// Before saving data, save the data before saving to private_data. When an
// error occurs in the saving process of another data member and
// SelfDevaluationSettingItemAccessorCancel is called, use the saved data to
// restore the state before saving.

// Args:
//     [IN] item (void*): Data member specified by offsetof
//                        EsfParameterStorageManagerOffsetBinaryArray.
//     [IN/OUT] work (EsfParameterStorageManagerWorkContext*): User data.
//     esfParameterStorageManagerWorkContext.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInternal: Argument is an invalid value.
//     kEsfParameterStorageManagerStatusResourceExhausted: Failed to allocate
//     internal resources. kEsfParameterStorageManagerStatusPermissionDenied:
//     Could not restore the data to the state
//                                 before saving.
//     kEsfParameterStorageManagerStatusDataLoss: Data could not be restored to
//     its pre-saved state. kEsfParameterStorageManagerStatusDataLoss: Unable to
//     access data storage area. kEsfParameterStorageManagerStatusUnavailable:
//     Failed to save data. kEsfParameterStorageManagerStatusOutOfRange: The
//     range of the data storage area has been
//                           exceeded.
//     kEsfParameterStorageManagerStatusInternal: Internal processing failed.

// Note:
// """
static EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterSaveOffsetBinaryArray(
    const void* item, EsfParameterStorageManagerWorkContext* work);

// """Loads offset-specified binary data (array type) from the data storage
// area.

// Loads offset-specified binary data (array type) from the data storage area.

// Args:
//     [IN] item (void*): Data member specified by offsetof
//                        EsfParameterStorageManagerOffsetBinaryArray.
//     [IN/OUT] work (EsfParameterStorageManagerWorkContext*): User data.
//     esfParameterStorageManagerWorkContext.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInternal: Argument is an invalid value.
//     kEsfParameterStorageManagerStatusDataLoss: Unable to access data storage
//     area. kEsfParameterStorageManagerStatusUnavailable: Failed to save data.
//     kEsfParameterStorageManagerStatusOutOfRange: The range of the data
//     storage area has been
//                           exceeded.
//     kEsfParameterStorageManagerStatusInternal: Internal processing failed.

// Note:
// """
static EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterLoadOffsetBinaryArray(
    void* item, EsfParameterStorageManagerWorkContext* work);

// """Saves offset-specified binary data (pointer type) to the data storage
// area.

// Saves offset-specified binary data (pointer type) to the data storage area.
// Before saving data, save the data before saving to private_data. When an
// error occurs in the saving process of another data member and
// SelfDevaluationSettingItemAccessorCancel is called, use the saved data to
// restore the state before saving.

// Args:
//     [IN] item (void*): Data member specified by offsetof
//                        EsfParameterStorageManagerOffsetBinary.
//     [IN/OUT] work (EsfParameterStorageManagerWorkContext*): User data.
//     esfParameterStorageManagerWorkContext.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInvalidArgument: The argument item->data
//     by the user is an
//                                invalid value.
//     kEsfParameterStorageManagerStatusInternal: Argument is an invalid value.
//     kEsfParameterStorageManagerStatusResourceExhausted: Failed to allocate
//     internal resources. kEsfParameterStorageManagerStatusPermissionDenied:
//     Could not restore the data to the state
//                                 before saving.
//     kEsfParameterStorageManagerStatusDataLoss: Data could not be restored to
//     its pre-saved state. kEsfParameterStorageManagerStatusDataLoss: Unable to
//     access data storage area. kEsfParameterStorageManagerStatusUnavailable:
//     Failed to save data. kEsfParameterStorageManagerStatusOutOfRange: The
//     range of the data storage area has been
//                           exceeded.
//     kEsfParameterStorageManagerStatusInternal: Internal processing failed.

// Note:
// """
static EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterSaveOffsetBinary(
    const void* item, EsfParameterStorageManagerWorkContext* work);

// """Loads offset-specified binary data (pointer type) from the data storage
// area.

// Loads offset-specified binary data (pointer type) from the data storage area.

// Args:
//     [IN] item (void*): Data member specified by offsetof
//                        EsfParameterStorageManagerOffsetBinary.
//     [IN/OUT] work (EsfParameterStorageManagerWorkContext*): User data.
//     esfParameterStorageManagerWorkContext.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInvalidArgument: The argument item->data
//     by the user is an
//                                invalid value.
//     kEsfParameterStorageManagerStatusInternal: Argument is an invalid value.
//     kEsfParameterStorageManagerStatusDataLoss: Unable to access data storage
//     area. kEsfParameterStorageManagerStatusUnavailable: Failed to save data.
//     kEsfParameterStorageManagerStatusOutOfRange: The range of the data
//     storage area has been
//                           exceeded.
//     kEsfParameterStorageManagerStatusInternal: Internal processing failed.

// Note:
// """
static EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterLoadOffsetBinary(
    void* item, EsfParameterStorageManagerWorkContext* work);

// """Saves a string to the data storage area.

// Saves a string to the data storage area. Before saving data, save the data
// before saving to private_data. When an error occurs in the saving process of
// another data member and SelfDevaluationSettingItemAccessorCancel is called,
// use the saved data to restore the state before saving.

// Args:
//     [IN] item (void*): Data member specified by offsetof "const char[]".
//     [IN/OUT] work (EsfParameterStorageManagerWorkContext*): User data.
//     esfParameterStorageManagerWorkContext.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInternal: Argument is an invalid value.
//     kEsfParameterStorageManagerStatusResourceExhausted: Failed to allocate
//     internal resources. kEsfParameterStorageManagerStatusPermissionDenied:
//     Could not restore the data to the state
//                                 before saving.
//     kEsfParameterStorageManagerStatusDataLoss: Data could not be restored to
//     its pre-saved state. kEsfParameterStorageManagerStatusDataLoss: Unable to
//     access data storage area. kEsfParameterStorageManagerStatusUnavailable:
//     Failed to save data. kEsfParameterStorageManagerStatusOutOfRange: The
//     range of the data storage area has been
//                           exceeded.
//     kEsfParameterStorageManagerStatusInternal: Internal processing failed.

// Note:
// """
static EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterSaveString(
    const void* item, EsfParameterStorageManagerWorkContext* work);

// """Loads a string from the data storage area.

// Loads a string from the data storage area.

// Args:
//     [IN] item (void*): Data member specified by offsetof "char[]".
//     [IN/OUT] work (EsfParameterStorageManagerWorkContext*): User data.
//     esfParameterStorageManagerWorkContext.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInternal: Argument is an invalid value.
//     kEsfParameterStorageManagerStatusDataLoss: Unable to access data storage
//     area. kEsfParameterStorageManagerStatusUnavailable: Failed to save data.
//     kEsfParameterStorageManagerStatusOutOfRange: The range of the data
//     storage area has been
//                           exceeded.
//     kEsfParameterStorageManagerStatusInternal: Internal processing failed.

// Note:
// """
static EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterLoadString(
    void* item, EsfParameterStorageManagerWorkContext* work);

// """Saves a string to the data storage area.

// Saves a string to the data storage area. Before saving data, save the data
// before saving to private_data. When an error occurs in the saving process of
// another data member and SelfDevaluationSettingItemAccessorCancel is called,
// use the saved data to restore the state before saving.

// Args:
//     [IN] item (void*): Data member specified by offsetof "const uint8_t[]".
//     [IN/OUT] work (EsfParameterStorageManagerWorkContext*): User data.
//     esfParameterStorageManagerWorkContext.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInternal: Argument is an invalid value.
//     kEsfParameterStorageManagerStatusResourceExhausted: Failed to allocate
//     internal resources. kEsfParameterStorageManagerStatusPermissionDenied:
//     Could not restore the data to the state
//                                 before saving.
//     kEsfParameterStorageManagerStatusDataLoss: Data could not be restored to
//     its pre-saved state. kEsfParameterStorageManagerStatusDataLoss: Unable to
//     access data storage area. kEsfParameterStorageManagerStatusUnavailable:
//     Failed to save data. kEsfParameterStorageManagerStatusOutOfRange: The
//     range of the data storage area has been
//                           exceeded.
//     kEsfParameterStorageManagerStatusInternal: Internal processing failed.

// Note:
// """
static EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterSaveRaw(
    const void* item, EsfParameterStorageManagerWorkContext* work);

// """Loads raw data from the data storage area.

// Loads raw data from the data storage area.

// Args:
//     [IN] item (void*): Data member specified by offsetof "uint8_t[]".
//     [IN/OUT] work (EsfParameterStorageManagerWorkContext*): User data.
//     esfParameterStorageManagerWorkContext.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInternal: Argument is an invalid value.
//     kEsfParameterStorageManagerStatusDataLoss: Unable to access data storage
//     area. kEsfParameterStorageManagerStatusUnavailable: Failed to save data.
//     kEsfParameterStorageManagerStatusOutOfRange: The range of the data
//     storage area has been
//                           exceeded.
//     kEsfParameterStorageManagerStatusInternal: Internal processing failed.

// Note:
// """
static EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterLoadRaw(
    void* item, EsfParameterStorageManagerWorkContext* work);

typedef EsfParameterStorageManagerStatus (
    *EsfParameterStorageManagerItemTypeSave)(
    const void*, EsfParameterStorageManagerWorkContext*);

typedef EsfParameterStorageManagerStatus (
    *EsfParameterStorageManagerItemTypeLoad)(
    void*, EsfParameterStorageManagerWorkContext*);

// data type processing table
static const EsfParameterStorageManagerItemTypeSave
    kDataTypeSaveFuncTable[kEsfParameterStorageManagerItemTypeMax] = {
        EsfParameterStorageManagerStorageAdapterSaveBinaryArray,
        EsfParameterStorageManagerStorageAdapterSaveBinary,
        EsfParameterStorageManagerStorageAdapterSaveOffsetBinaryArray,
        EsfParameterStorageManagerStorageAdapterSaveOffsetBinary,
        EsfParameterStorageManagerStorageAdapterSaveString,
        EsfParameterStorageManagerStorageAdapterSaveRaw,
};
static const EsfParameterStorageManagerItemTypeLoad
    kDataTypeLoadFuncTable[kEsfParameterStorageManagerItemTypeMax] = {
        EsfParameterStorageManagerStorageAdapterLoadBinaryArray,
        EsfParameterStorageManagerStorageAdapterLoadBinary,
        EsfParameterStorageManagerStorageAdapterLoadOffsetBinaryArray,
        EsfParameterStorageManagerStorageAdapterLoadOffsetBinary,
        EsfParameterStorageManagerStorageAdapterLoadString,
        EsfParameterStorageManagerStorageAdapterLoadRaw,
};

EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterSaveItemType(
    EsfParameterStorageManagerItemType type, const void* item,
    void* private_data) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    if (type < 0 || kEsfParameterStorageManagerItemTypeMax <= type) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid Item ID. type=%u, ret=%u(%s)", type, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }
    EsfParameterStorageManagerWorkContext* work =
        (EsfParameterStorageManagerWorkContext*)private_data;
    if (item == NULL || work == NULL || work->member_info == NULL) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "NULL argument. type=%u, item=%p, work=%p, member_info=%p, "
          "ret=%u(%s)",
          type, item, work, (work ? work->member_info : NULL), ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Saving item. type=%u", type);
    ret = kDataTypeSaveFuncTable[type](item, work);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Save failed. type=%u, ret=%u(%s)", type, ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Item saved successfully. type=%u",
                                        type);
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterLoadItemType(
    EsfParameterStorageManagerItemType type, void* item, void* private_data) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    if (type < 0 || kEsfParameterStorageManagerItemTypeMax <= type) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid Item ID. type=%u, ret=%u(%s)", type, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }
    EsfParameterStorageManagerWorkContext* work =
        (EsfParameterStorageManagerWorkContext*)private_data;
    if (item == NULL || work == NULL || work->member_info == NULL) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "NULL argument. type=%u, item=%p, work=%p, member_info=%p, "
          "ret=%u(%s)",
          type, item, work, (work ? work->member_info : NULL), ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Loading item. type=%u", type);
    ret = kDataTypeLoadFuncTable[type](item, work);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Load failed. type=%u, ret=%u(%s)", type, ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Item loaded successfully. type=%u",
                                        type);
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterSaveBinaryArray(
    const void* item, EsfParameterStorageManagerWorkContext* work) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    const EsfParameterStorageManagerBinaryArray* obj =
        (const EsfParameterStorageManagerBinaryArray*)item;
    if (work->member_info->size < obj->size) {
      ret = kEsfParameterStorageManagerStatusOutOfRange;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Data size exceeds buffer size. obj_size=%" PRIu32
          ", buffer_size=%zu, ret=%u(%s)",
          obj->size, work->member_info->size, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }
    if (obj->size == 0) {
      if (work->member_data[work->index].storage.written_size == 0) {
        ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Data already empty. id=%u",
                                            work->member_info->id);
        work->member_data[work->index].cancel =
            kEsfParameterStorageManagerCancelSkip;
        ret = kEsfParameterStorageManagerStatusOk;
        break;
      }
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Clearing data. id=%u",
                                          work->member_info->id);
      ret = EsfParameterStorageManagerStorageAdapterClear(
          work->member_info->id, &work->member_data[work->index]);
    } else {
      if (work->member_data[work->index].buffer.size == obj->size &&
          EsfParameterStorageManagerBufferIsEqual(
              &work->member_data[work->index].buffer, 0, obj->size,
              obj->data)) {
        ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Data already written. id=%u",
                                            work->member_info->id);
        work->member_data[work->index].cancel =
            kEsfParameterStorageManagerCancelSkip;
        ret = kEsfParameterStorageManagerStatusOk;
        break;
      }
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Saving data. id=%u, size=%" PRIu32,
                                          work->member_info->id, obj->size);
      work->member_data[work->index].append = false;
      uint32_t save_size = 0;
      ret = EsfParameterStorageManagerStorageAdapterSave(
          work->member_info->id, 0, obj->size, obj->data, &save_size,
          &work->member_data[work->index]);
      if (ret == kEsfParameterStorageManagerStatusOk &&
          obj->size != save_size) {
        ret = kEsfParameterStorageManagerStatusDataLoss;
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Unexpected data size. id=%u, obj->size=%" PRIu32
            ", save_size=%" PRIu32 ", ret=%u(%s)",
            work->member_info->id, obj->size, save_size, ret,
            EsfParameterStorageManagerStrError(ret));
      }
    }
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Save failed. id=%u, ret=%u(%s)", work->member_info->id, ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterLoadBinaryArray(
    void* item, EsfParameterStorageManagerWorkContext* work) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    EsfParameterStorageManagerBinaryArray* obj =
        (EsfParameterStorageManagerBinaryArray*)item;
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Loading data. id=%u, max_size=%zu",
                                        work->member_info->id,
                                        work->member_info->size);
    ret = EsfParameterStorageManagerStorageAdapterLoad(
        work->member_info->id, &work->member_data[work->index], 0,
        work->member_info->size, obj->data, &obj->size);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Load failed. id=%u, ret=%u(%s)", work->member_info->id, ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Data loaded successfully. id=%u, size=%" PRIu32, work->member_info->id,
        obj->size);
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterSaveBinary(
    const void* item, EsfParameterStorageManagerWorkContext* work) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    const EsfParameterStorageManagerBinary* obj =
        (const EsfParameterStorageManagerBinary*)item;
    if (obj->data == NULL || obj->size == 0) {
      if (work->member_data[work->index].storage.written_size == 0) {
        ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Data already empty. id=%u",
                                            work->member_info->id);
        work->member_data[work->index].cancel =
            kEsfParameterStorageManagerCancelSkip;
        ret = kEsfParameterStorageManagerStatusOk;
        break;
      }
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Clearing data. id=%u",
                                          work->member_info->id);
      ret = EsfParameterStorageManagerStorageAdapterClear(
          work->member_info->id, &work->member_data[work->index]);
    } else {
      if (work->member_data[work->index].buffer.size == obj->size &&
          EsfParameterStorageManagerBufferIsEqual(
              &work->member_data[work->index].buffer, 0, obj->size,
              obj->data)) {
        ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Data already written. id=%u",
                                            work->member_info->id);
        work->member_data[work->index].cancel =
            kEsfParameterStorageManagerCancelSkip;
        ret = kEsfParameterStorageManagerStatusOk;
        break;
      }
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Saving data. id=%u, size=%" PRIu32,
                                          work->member_info->id, obj->size);
      work->member_data[work->index].append = false;
      uint32_t save_size = 0;
      ret = EsfParameterStorageManagerStorageAdapterSave(
          work->member_info->id, 0, obj->size, obj->data, &save_size,
          &work->member_data[work->index]);
      if (ret == kEsfParameterStorageManagerStatusOk &&
          obj->size != save_size) {
        ret = kEsfParameterStorageManagerStatusDataLoss;
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Unexpected data size. id=%u, obj->size=%" PRIu32
            ", save_size=%" PRIu32 ", ret=%u(%s)",
            work->member_info->id, obj->size, save_size, ret,
            EsfParameterStorageManagerStrError(ret));
      }
    }
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Save failed. id=%u, ret=%u(%s)", work->member_info->id, ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Data saved successfully. id=%u",
                                        work->member_info->id);
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterLoadBinary(
    void* item, EsfParameterStorageManagerWorkContext* work) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    EsfParameterStorageManagerBinary* obj =
        (EsfParameterStorageManagerBinary*)item;
    if (obj->data == NULL) {
      ret = kEsfParameterStorageManagerStatusInvalidArgument;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Binary data pointer is NULL. id=%u, ret=%u(%s)",
          work->member_info->id, ret, EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Loading data. id=%u, max_size=%" PRIu32, work->member_info->id,
        obj->size);
    ret = EsfParameterStorageManagerStorageAdapterLoad(
        work->member_info->id, &work->member_data[work->index], 0, obj->size,
        obj->data, &obj->size);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Load failed. id=%u, ret=%u(%s)", work->member_info->id, ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Data loaded successfully. id=%u, size=%" PRIu32, work->member_info->id,
        obj->size);
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterSaveOffsetBinaryArray(
    const void* item, EsfParameterStorageManagerWorkContext* work) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    const EsfParameterStorageManagerOffsetBinaryArray* obj =
        (const EsfParameterStorageManagerOffsetBinaryArray*)item;
    if (obj->size == 0) {
      ret = kEsfParameterStorageManagerStatusInvalidArgument;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid size. id=%u, size=%" PRIu32 ", ret=%u(%s)",
          work->member_info->id, obj->size, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }
    if (work->member_info->size < obj->size) {
      ret = kEsfParameterStorageManagerStatusOutOfRange;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Data size exceeds buffer size. id=%u, data_size=%" PRIu32
          ", buffer_size=%zu, ret=%u(%s)",
          work->member_info->id, obj->size, work->member_info->size, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }
    if (obj->offset != 0 &&
        work->member_data[work->index].storage.capabilities.enable_offset ==
            0) {
      ret = kEsfParameterStorageManagerStatusInvalidArgument;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Offset not supported. id=%u, offset=%u, ret=%u(%s)",
          work->member_info->id, obj->offset, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }
    if (EsfParameterStorageManagerBufferIsEqual(
            &work->member_data[work->index].buffer, obj->offset, obj->size,
            obj->data)) {
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Data already written. id=%u",
                                          work->member_info->id);
      work->member_data[work->index].cancel =
          kEsfParameterStorageManagerCancelSkip;
      ret = kEsfParameterStorageManagerStatusOk;
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Saving data. id=%u, offset=%" PRIu32 ", size=%" PRIu32,
        work->member_info->id, obj->offset, obj->size);
    work->member_data[work->index].append = true;
    uint32_t save_size = 0;
    ret = EsfParameterStorageManagerStorageAdapterSave(
        work->member_info->id, obj->offset, obj->size, obj->data, &save_size,
        &work->member_data[work->index]);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Save failed. id=%u, ret=%u(%s)", work->member_info->id, ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
    if (obj->size != save_size) {
      ret = kEsfParameterStorageManagerStatusDataLoss;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Unexpected data size. id=%u, obj->size=%" PRIu32
          ", save_size=%" PRIu32 ", ret=%u(%s)",
          work->member_info->id, obj->size, save_size, ret,
          EsfParameterStorageManagerStrError(ret));
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Data saved successfully. id=%u, save_size=%" PRIu32,
        work->member_info->id, save_size);
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterLoadOffsetBinaryArray(
    void* item, EsfParameterStorageManagerWorkContext* work) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    EsfParameterStorageManagerOffsetBinaryArray* obj =
        (EsfParameterStorageManagerOffsetBinaryArray*)item;
    if (obj->offset != 0 &&
        work->member_data[work->index].storage.capabilities.enable_offset ==
            0) {
      ret = kEsfParameterStorageManagerStatusInvalidArgument;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Offset not supported. id=%u, offset=%u, ret=%u(%s)",
          work->member_info->id, obj->offset, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Loading data. id=%u, offset=%" PRIu32 ", max_size=%zu",
        work->member_info->id, obj->offset, work->member_info->size);
    ret = EsfParameterStorageManagerStorageAdapterLoad(
        work->member_info->id, &work->member_data[work->index], obj->offset,
        work->member_info->size, obj->data, &obj->size);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Load failed. id=%u, ret=%u(%s)", work->member_info->id, ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Data loaded successfully. id=%u, size=%" PRIu32, work->member_info->id,
        obj->size);
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterSaveOffsetBinary(
    const void* item, EsfParameterStorageManagerWorkContext* work) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    const EsfParameterStorageManagerOffsetBinary* obj =
        (const EsfParameterStorageManagerOffsetBinary*)item;
    if (obj->size == 0) {
      ret = kEsfParameterStorageManagerStatusInvalidArgument;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid size. id=%u, size=%" PRIu32 ", ret=%u(%s)",
          work->member_info->id, obj->size, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }
    if (obj->data == NULL) {
      ret = kEsfParameterStorageManagerStatusInvalidArgument;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "NULL data pointer. id=%u, ret=%u(%s)", work->member_info->id, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }
    if (obj->offset != 0 &&
        work->member_data[work->index].storage.capabilities.enable_offset ==
            0) {
      ret = kEsfParameterStorageManagerStatusInvalidArgument;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Offset not supported. id=%u, offset=%u, ret=%u(%s)",
          work->member_info->id, obj->offset, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }
    if (EsfParameterStorageManagerBufferIsEqual(
            &work->member_data[work->index].buffer, obj->offset, obj->size,
            obj->data)) {
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Data already written. id=%u",
                                          work->member_info->id);
      work->member_data[work->index].cancel =
          kEsfParameterStorageManagerCancelSkip;
      ret = kEsfParameterStorageManagerStatusOk;
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Saving data. id=%u, offset=%" PRIu32 ", size=%" PRIu32,
        work->member_info->id, obj->offset, obj->size);
    work->member_data[work->index].append = true;
    uint32_t save_size = 0;
    ret = EsfParameterStorageManagerStorageAdapterSave(
        work->member_info->id, obj->offset, obj->size, obj->data, &save_size,
        &work->member_data[work->index]);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Save failed. id=%u, ret=%u(%s)", work->member_info->id, ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
    if (obj->size != save_size) {
      ret = kEsfParameterStorageManagerStatusDataLoss;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Unexpected data size. id=%u, obj->size=%" PRIu32
          ", save_size=%" PRIu32 ", ret=%u(%s)",
          work->member_info->id, obj->size, save_size, ret,
          EsfParameterStorageManagerStrError(ret));
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Data saved successfully. id=%u, save_size=%" PRIu32,
        work->member_info->id, save_size);
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterLoadOffsetBinary(
    void* item, EsfParameterStorageManagerWorkContext* work) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    EsfParameterStorageManagerOffsetBinary* obj =
        (EsfParameterStorageManagerOffsetBinary*)item;
    if (obj->data == NULL) {
      ret = kEsfParameterStorageManagerStatusInvalidArgument;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "NULL data pointer. id=%u, ret=%u(%s)", work->member_info->id, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }
    if (obj->offset != 0 &&
        work->member_data[work->index].storage.capabilities.enable_offset ==
            0) {
      ret = kEsfParameterStorageManagerStatusInvalidArgument;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Offset not supported. id=%u, offset=%u, ret=%u(%s)",
          work->member_info->id, obj->offset, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Loading data. id=%u, offset=%" PRIu32 ", max_size=%" PRIu32,
        work->member_info->id, obj->offset, obj->size);
    ret = EsfParameterStorageManagerStorageAdapterLoad(
        work->member_info->id, &work->member_data[work->index], obj->offset,
        obj->size, obj->data, &obj->size);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Load failed. id=%u, ret=%u(%s)", work->member_info->id, ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Data loaded successfully. id=%u, size=%" PRIu32, work->member_info->id,
        obj->size);
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterSaveString(
    const void* item, EsfParameterStorageManagerWorkContext* work) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    const char* obj = (const char*)item;
    size_t len = strnlen(obj, work->member_info->size);
    if (len == work->member_info->size) {
      ret = kEsfParameterStorageManagerStatusOutOfRange;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "String exceeds buffer size. id=%u, string_len=%zu, buffer_size=%zu, "
          "ret=%u(%s)",
          work->member_info->id, len, work->member_info->size, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }
    if (len == 0) {
      if (work->member_data[work->index].storage.written_size == 0) {
        ESF_PARAMETER_STORAGE_MANAGER_DEBUG("String already empty. id=%u",
                                            work->member_info->id);
        work->member_data[work->index].cancel =
            kEsfParameterStorageManagerCancelSkip;
        ret = kEsfParameterStorageManagerStatusOk;
        break;
      }
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Clearing string. id=%u",
                                          work->member_info->id);
      ret = EsfParameterStorageManagerStorageAdapterClear(
          work->member_info->id, &work->member_data[work->index]);
    } else {
      if (work->member_data[work->index].buffer.size == len &&
          EsfParameterStorageManagerBufferIsEqual(
              &work->member_data[work->index].buffer, 0, len,
              (const uint8_t*)obj)) {
        ESF_PARAMETER_STORAGE_MANAGER_DEBUG("String already written. id=%u",
                                            work->member_info->id);
        work->member_data[work->index].cancel =
            kEsfParameterStorageManagerCancelSkip;
        ret = kEsfParameterStorageManagerStatusOk;
        break;
      }
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Saving string. id=%u, length=%zu",
                                          work->member_info->id, len);
      work->member_data[work->index].append = false;
      uint32_t save_size = 0;
      ret = EsfParameterStorageManagerStorageAdapterSave(
          work->member_info->id, 0, len, (const uint8_t*)obj, &save_size,
          &work->member_data[work->index]);
      if (ret == kEsfParameterStorageManagerStatusOk && len != save_size) {
        ret = kEsfParameterStorageManagerStatusDataLoss;
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Unexpected data size. id=%u, len=%zu, save_size=%" PRIu32
            ", ret=%u(%s)",
            work->member_info->id, len, save_size, ret,
            EsfParameterStorageManagerStrError(ret));
      }
    }
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Save failed. id=%u, ret=%u(%s)", work->member_info->id, ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("String saved successfully. id=%u",
                                        work->member_info->id);
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterLoadString(
    void* item, EsfParameterStorageManagerWorkContext* work) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    char* obj = (char*)item;
    if (work->member_info->size == 0) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid argument. member_size=%zu, id=%u, ret=%u(%s)",
          work->member_info->size, work->member_info->id, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Loading string. id=%u, max_size=%zu",
                                        work->member_info->id,
                                        work->member_info->size);
    uint32_t load_size = 0;
    ret = EsfParameterStorageManagerStorageAdapterLoad(
        work->member_info->id, &work->member_data[work->index], 0,
        work->member_info->size, (uint8_t*)obj, &load_size);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Load failed. id=%u, ret=%u(%s)", work->member_info->id, ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
    if (load_size < work->member_info->size) {
      obj[load_size] = '\0';
    } else {
      obj[work->member_info->size - 1] = '\0';
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "String loaded successfully. id=%u, length=%u", work->member_info->id,
        load_size);
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterSaveRaw(
    const void* item, EsfParameterStorageManagerWorkContext* work) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    EsfParameterStorageManagerRaw* obj = (EsfParameterStorageManagerRaw*)item;
    const uint8_t* obj_data =
        (const uint8_t*)((uintptr_t)obj +
                         sizeof(EsfParameterStorageManagerRaw));
    if (work->member_info->size <= sizeof(EsfParameterStorageManagerRaw)) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid argument. member_size=%zu, id=%u, ret=%u(%s)",
          work->member_info->size, work->member_info->id, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }
    const size_t write_size =
        work->member_info->size - sizeof(EsfParameterStorageManagerRaw);
    if (work->member_data[work->index].buffer.size == write_size &&
        EsfParameterStorageManagerBufferIsEqual(
            &work->member_data[work->index].buffer, 0, write_size, obj_data)) {
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Data already written. id=%u",
                                          work->member_info->id);
      work->member_data[work->index].cancel =
          kEsfParameterStorageManagerCancelSkip;
      ret = kEsfParameterStorageManagerStatusOk;
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Saving raw data. id=%u, size=%zu",
                                        work->member_info->id, write_size);
    work->member_data[work->index].append = false;
    uint32_t save_size = 0;
    ret = EsfParameterStorageManagerStorageAdapterSave(
        work->member_info->id, 0, write_size, obj_data, &save_size,
        &work->member_data[work->index]);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Save failed. id=%u, ret=%u(%s)", work->member_info->id, ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
    if (write_size != save_size) {
      ret = kEsfParameterStorageManagerStatusDataLoss;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Unexpected data size. id=%u, write_size=%zu, save_size=%" PRIu32
          ", ret=%u(%s)",
          work->member_info->id, write_size, save_size, ret,
          EsfParameterStorageManagerStrError(ret));
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Raw data saved successfully. id=%u, save_size=%" PRIu32,
        work->member_info->id, save_size);
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterLoadRaw(
    void* item, EsfParameterStorageManagerWorkContext* work) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    EsfParameterStorageManagerRaw* obj = (EsfParameterStorageManagerRaw*)item;
    uint8_t* data =
        (uint8_t*)((uintptr_t)item + sizeof(EsfParameterStorageManagerRaw));
    if (work->member_info->size <= sizeof(EsfParameterStorageManagerRaw)) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid argument. member_size=%zu, id=%u, ret=%u(%s)",
          work->member_info->size, work->member_info->id, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }
    size_t max_size =
        work->member_info->size - sizeof(EsfParameterStorageManagerRaw);
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Loading raw data. id=%u, max_size=%zu",
                                        work->member_info->id, max_size);
    ret = EsfParameterStorageManagerStorageAdapterLoad(
        work->member_info->id, &work->member_data[work->index], 0, max_size,
        data, &obj->size);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Load failed. id=%u, ret=%u(%s)", work->member_info->id, ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Raw data loaded successfully. id=%u, size=%" PRIu32,
        work->member_info->id, obj->size);
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}
