/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 #include "system_manager_accessor_exception_info.h"

#include "parameter_storage_manager.h"
#include "parameter_storage_manager_common.h"
#include "system_manager.h"
#include "utility_log.h"
#include "utility_log_module_id.h"

#ifndef ESF_SYSTEM_MANAGER_REMOVE_STATIC
#define STATIC static
#else  // ESF_SYSTEM_MANAGER_REMOVE_STATIC
#define STATIC
#endif  // ESF_SYSTEM_MANAGER_REMOVE_STATIC

// """Checks if the Exception Info mask is enabled.
// This function examines the provided mask to determine whether the
// Exception Info bit is enabled. The function uses the macro
// ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED to perform the bitwise
// check and return the result.
// Args:
//     [IN] mask (EsfParameterStorageManagerMask): The mask that contains
//        various flags, including the Exception Info bit.
// Returns:
//     bool: `true` if the Exception Info mask is enabled, otherwise
//       `false`.
// """
STATIC bool EsfParameterStorageManagerMaskEnabledExceptionInfo(
    EsfParameterStorageManagerMask mask);

// Information for accessing the Exception Info structure members.
static const EsfParameterStorageManagerMemberInfo kMemberInfo[] = {
    {
        .id = kEsfParameterStorageManagerItemExceptionInfo,
        .type = kEsfParameterStorageManagerItemTypeBinaryPointer,
        .offset = offsetof(EsfParameterStorageManagerExceptionInfo,
                           exception_info),
        .size = 0,
        .enabled = EsfParameterStorageManagerMaskEnabledExceptionInfo,
        .custom = NULL,
    },
};

// Information for accessing the Exception Info structure.
static const EsfParameterStorageManagerStructInfo kStructInfo = {
    .items_num = sizeof(kMemberInfo) / sizeof(kMemberInfo[0]),
    .items = kMemberInfo,
};

EsfSystemManagerResult EsfSystemManagerSaveExceptionInfoToPsm(
    EsfParameterStorageManagerHandle handle,
    const EsfParameterStorageManagerExceptionInfoMask* mask,
    const EsfParameterStorageManagerExceptionInfo* data) {
  if ((mask == NULL) || (data == NULL)) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM, "%s-%d:Parameter error. mask=%p data=%p.",
        "system_manager_accessor_exception_info.c", __LINE__, mask, data);
    return kEsfSystemManagerResultParamError;
  }

  EsfParameterStorageManagerStatus status = EsfParameterStorageManagerSave(
      handle, (EsfParameterStorageManagerMask)mask,
      (EsfParameterStorageManagerData)data, &kStructInfo, NULL);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to save to Parameter Storage Manager.",
                     "system_manager_accessor_exception_info.c", __LINE__);
    return kEsfSystemManagerResultInternalError;
  }
  return kEsfSystemManagerResultOk;
}

EsfSystemManagerResult EsfSystemManagerLoadExceptionInfoFromPsm(
    EsfParameterStorageManagerHandle handle,
    const EsfParameterStorageManagerExceptionInfoMask* mask,
    EsfParameterStorageManagerExceptionInfo* data) {
  if ((mask == NULL) || (data == NULL)) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM, "%s-%d:Parameter error. mask=%p data=%p.",
        "system_manager_accessor_exception_info.c", __LINE__, mask, data);
    return kEsfSystemManagerResultParamError;
  }

  EsfParameterStorageManagerStatus status = EsfParameterStorageManagerLoad(
      handle, (EsfParameterStorageManagerMask)mask,
      (EsfParameterStorageManagerData)data, &kStructInfo, NULL);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to load from Parameter Storage Manager.",
                     "system_manager_accessor_exception_info.c", __LINE__);
    return kEsfSystemManagerResultInternalError;
  }

  return kEsfSystemManagerResultOk;
}

EsfSystemManagerResult EsfSystemManagerClearExceptionInfoOfPsm(
    EsfParameterStorageManagerHandle handle,
    const EsfParameterStorageManagerExceptionInfoMask* mask) {
  if (mask == NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Parameter error. mask=%p.",
                     "system_manager_accessor_exception_info.c", __LINE__,
                     mask);
    return kEsfSystemManagerResultParamError;
  }

  EsfParameterStorageManagerStatus status = EsfParameterStorageManagerClear(
      handle, (EsfParameterStorageManagerMask)mask, &kStructInfo, NULL);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to clear Parameter Storage Manager.",
                     "system_manager_accessor_exception_info.c", __LINE__);
    return kEsfSystemManagerResultInternalError;
  }
  return kEsfSystemManagerResultOk;
}

STATIC bool EsfParameterStorageManagerMaskEnabledExceptionInfo(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfParameterStorageManagerExceptionInfoMask, exception_info, mask);
}
