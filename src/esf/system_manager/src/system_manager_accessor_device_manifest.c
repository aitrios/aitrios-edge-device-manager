/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "system_manager_accessor_device_manifest.h"

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

// """Checks if the Device Manifest mask is enabled.
// This function examines the provided mask to determine whether the
// Device Manifest bit is enabled. The function uses the macro
// ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED to perform the bitwise
// check and return the result.
// Args:
//     [IN] mask (EsfParameterStorageManagerMask): The mask that contains
//        various flags, including the Device Manifest bit.
// Returns:
//     bool: `true` if the Device Manifest mask is enabled, otherwise
//       `false`.
// """
STATIC bool EsfParameterStorageManagerMaskEnabledDeviceManifest(
    EsfParameterStorageManagerMask mask);

// Information for accessing the Device Manifest structure members.
static const EsfParameterStorageManagerMemberInfo kMemberInfo[] = {
    {
        .id = kEsfParameterStorageManagerItemDeviceManifest,
        .type = kEsfParameterStorageManagerItemTypeOffsetBinaryPointer,
        .offset = offsetof(EsfParameterStorageManagerDeviceManifest,
                           device_manifest),
        .size = 0,
        .enabled = EsfParameterStorageManagerMaskEnabledDeviceManifest,
        .custom = NULL,
    },
};

// Information for accessing the Device Manifest structure.
static const EsfParameterStorageManagerStructInfo kStructInfo = {
    .items_num = sizeof(kMemberInfo) / sizeof(kMemberInfo[0]),
    .items = kMemberInfo,
};

EsfSystemManagerResult EsfSystemManagerLoadDeviceManifestFromPsm(
    EsfParameterStorageManagerHandle handle,
    const EsfParameterStorageManagerDeviceManifestMask* mask,
    EsfParameterStorageManagerDeviceManifest* data) {
  if ((mask == NULL) || (data == NULL)) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM, "%s-%d:Parameter error. mask=%p data=%p.",
        "system_manager_accessor_device_manifest.c", __LINE__, mask, data);
    return kEsfSystemManagerResultParamError;
  }

  EsfParameterStorageManagerStatus status = EsfParameterStorageManagerLoad(
      handle, (EsfParameterStorageManagerMask)mask,
      (EsfParameterStorageManagerData)data, &kStructInfo, NULL);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to load from Parameter Storage Manager.",
                     "system_manager_accessor_device_manifest.c", __LINE__);
    return kEsfSystemManagerResultInternalError;
  }

  if (mask->device_manifest) {
    if (EsfParameterStorageManagerIsDataEmpty(
            (EsfParameterStorageManagerData)data, &kStructInfo, 0)) {
      WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (uint16_t)0x8d10);
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Device Manifest is empty.",
                       "system_manager_accessor_device_manifest.c", __LINE__);
      return kEsfSystemManagerResultEmptyData;
    }
  }
  return kEsfSystemManagerResultOk;
}

STATIC bool EsfParameterStorageManagerMaskEnabledDeviceManifest(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfParameterStorageManagerDeviceManifestMask, device_manifest, mask);
}
