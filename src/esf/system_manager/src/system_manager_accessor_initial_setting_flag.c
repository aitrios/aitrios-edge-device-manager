/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "system_manager_accessor_initial_setting_flag.h"

#include "parameter_storage_manager.h"
#include "parameter_storage_manager_common.h"
#include "system_manager.h"
#include "utility_log.h"
#include "utility_log_module_id.h"

#ifndef CONFIG_EXTERNAL_SYSTEM_MANAGER_DEFAULT_INITIAL_SETTING_FLAG
#define CONFIG_EXTERNAL_SYSTEM_MANAGER_DEFAULT_INITIAL_SETTING_FLAG (0)
#endif

#ifndef ESF_SYSTEM_MANAGER_REMOVE_STATIC
#define STATIC static
#else  // ESF_SYSTEM_MANAGER_REMOVE_STATIC
#define STATIC
#endif  // ESF_SYSTEM_MANAGER_REMOVE_STATIC

// """Checks if the Initial Setting Flag mask is enabled.
// This function examines the provided mask to determine whether the
// Initial Setting Flag bit is enabled. The function uses the macro
// ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED to perform the bitwise
// check and return the result.
// Args:
//     [IN] mask (EsfParameterStorageManagerMask): The mask that contains
//        various flags, including the Initial Setting Flag bit.
// Returns:
//     bool: `true` if the Initial Setting Flag mask is enabled, otherwise
//       `false`.
// """
STATIC bool EsfParameterStorageManagerMaskEnabledInitialSettingFlag(
    EsfParameterStorageManagerMask mask);

// Information for accessing the Initial Setting Flag structure members.
static const EsfParameterStorageManagerMemberInfo kMemberInfo[] = {
    {
        .id = kEsfParameterStorageManagerItemInitialSettingFlag,
        .type = kEsfParameterStorageManagerItemTypeRaw,
        .offset = offsetof(EsfParameterStorageManagerInitialSettingFlag,
                           initial_setting_flag),
        .size = ESF_PARAMETER_STORAGE_MANAGER_RAW_SIZEOF(
            EsfParameterStorageManagerInitialSettingFlagRaw, data),
        .enabled = EsfParameterStorageManagerMaskEnabledInitialSettingFlag,
        .custom = NULL,
    },
};

// Information for accessing the Initial Setting Flag structure.
static const EsfParameterStorageManagerStructInfo kStructInfo = {
    .items_num = sizeof(kMemberInfo) / sizeof(kMemberInfo[0]),
    .items = kMemberInfo,
};

EsfSystemManagerResult EsfSystemManagerSaveInitialSettingFlagToPsm(
    EsfParameterStorageManagerHandle handle,
    const EsfParameterStorageManagerInitialSettingFlagMask* mask,
    const EsfParameterStorageManagerInitialSettingFlag* data) {
  if ((mask == NULL) || (data == NULL)) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM, "%s-%d:Parameter error. mask=%p data=%p.",
        "system_manager_accessor_initial_setting_flag.c", __LINE__, mask, data);
    return kEsfSystemManagerResultParamError;
  }

  EsfParameterStorageManagerStatus status = EsfParameterStorageManagerSave(
      handle, (EsfParameterStorageManagerMask)mask,
      (EsfParameterStorageManagerData)data, &kStructInfo, NULL);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM, "%s-%d:Failed to save to Parameter Storage Manager.",
        "system_manager_accessor_initial_setting_flag.c", __LINE__);
    return kEsfSystemManagerResultInternalError;
  }
  return kEsfSystemManagerResultOk;
}

EsfSystemManagerResult EsfSystemManagerLoadInitialSettingFlagFromPsm(
    EsfParameterStorageManagerHandle handle,
    const EsfParameterStorageManagerInitialSettingFlagMask* mask,
    EsfParameterStorageManagerInitialSettingFlag* data) {
  if ((mask == NULL) || (data == NULL)) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM, "%s-%d:Parameter error. mask=%p data=%p.",
        "system_manager_accessor_initial_setting_flag.c", __LINE__, mask, data);
    return kEsfSystemManagerResultParamError;
  }

  EsfParameterStorageManagerStatus status = EsfParameterStorageManagerLoad(
      handle, (EsfParameterStorageManagerMask)mask,
      (EsfParameterStorageManagerData)data, &kStructInfo, NULL);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to load from Parameter Storage Manager.",
                     "system_manager_accessor_initial_setting_flag.c",
                     __LINE__);
    return kEsfSystemManagerResultInternalError;
  }

  if (mask->initial_setting_flag) {
    if (EsfParameterStorageManagerIsDataEmpty(
            (EsfParameterStorageManagerData)data, &kStructInfo, 0)) {
      // Return default value.
      WRITE_DLOG_DEBUG(
          MODULE_ID_SYSTEM,
          "%s-%d:Return default value. initial_setting_flag=%d",
          "system_manager_accessor_initial_setting_flag.c", __LINE__,
          CONFIG_EXTERNAL_SYSTEM_MANAGER_DEFAULT_INITIAL_SETTING_FLAG);
      data->initial_setting_flag.data =
          (uint8_t)CONFIG_EXTERNAL_SYSTEM_MANAGER_DEFAULT_INITIAL_SETTING_FLAG;
    }
  }

  return kEsfSystemManagerResultOk;
}

STATIC bool EsfParameterStorageManagerMaskEnabledInitialSettingFlag(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfParameterStorageManagerInitialSettingFlagMask, initial_setting_flag,
      mask);
}
