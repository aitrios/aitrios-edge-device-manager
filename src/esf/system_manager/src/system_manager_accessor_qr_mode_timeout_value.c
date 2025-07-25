/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "system_manager_accessor_qr_mode_timeout_value.h"

#include "parameter_storage_manager.h"
#include "parameter_storage_manager_common.h"
#include "system_manager.h"
#include "utility_log.h"
#include "utility_log_module_id.h"

#ifndef CONFIG_EXTERNAL_SYSTEM_MANAGER_DEFAULT_QR_MODE_TIMEOUT_VALUE
#define CONFIG_EXTERNAL_SYSTEM_MANAGER_DEFAULT_QR_MODE_TIMEOUT_VALUE (0)
#endif

#ifndef ESF_SYSTEM_MANAGER_REMOVE_STATIC
#define STATIC static
#else  // ESF_SYSTEM_MANAGER_REMOVE_STATIC
#define STATIC
#endif  // ESF_SYSTEM_MANAGER_REMOVE_STATIC

// """Checks if the QR Mode Timeout Value mask is enabled.
// This function examines the provided mask to determine whether the
// QR Mode Timeout Value bit is enabled. The function uses the macro
// ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED to perform the bitwise
// check and return the result.
// Args:
//     [IN] mask (EsfParameterStorageManagerMask): The mask that contains
//        various flags, including the QR Mode Timeout Value bit.
// Returns:
//     bool: `true` if the QR Mode Timeout Value mask is enabled, otherwise
//       `false`.
// """
STATIC bool EsfParameterStorageManagerMaskEnabledQrModeTimeoutValue(
    EsfParameterStorageManagerMask mask);

// Information for accessing the QR Mode Timeout Value structure members.
static const EsfParameterStorageManagerMemberInfo kMemberInfo[] = {
    {
        .id = kEsfParameterStorageManagerItemQRModeStateFlag,
        .type = kEsfParameterStorageManagerItemTypeRaw,
        .offset = offsetof(EsfParameterStorageManagerQrModeTimeoutValue,
                           qr_mode_timeout_value),
        .size = ESF_PARAMETER_STORAGE_MANAGER_RAW_SIZEOF(
            EsfParameterStorageManagerQrModeTimeoutValueRaw, data),
        .enabled = EsfParameterStorageManagerMaskEnabledQrModeTimeoutValue,
        .custom = NULL,
    },
};

// Information for accessing the QR Mode Timeout Value structure.
static const EsfParameterStorageManagerStructInfo kStructInfo = {
    .items_num = sizeof(kMemberInfo) / sizeof(kMemberInfo[0]),
    .items = kMemberInfo,
};

EsfSystemManagerResult EsfSystemManagerSaveQrModeTimeoutValueToPsm(
    EsfParameterStorageManagerHandle handle,
    const EsfParameterStorageManagerQrModeTimeoutValueMask* mask,
    const EsfParameterStorageManagerQrModeTimeoutValue* data) {
  if ((mask == NULL) || (data == NULL)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Parameter error. mask=%p data=%p.",
                     "system_manager_accessor_qr_mode_timeout_value.c",
                     __LINE__, mask, data);
    return kEsfSystemManagerResultParamError;
  }

  EsfParameterStorageManagerStatus status = EsfParameterStorageManagerSave(
      handle, (EsfParameterStorageManagerMask)mask,
      (EsfParameterStorageManagerData)data, &kStructInfo, NULL);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM, "%s-%d:Failed to save to Parameter Storage Manager.",
        "system_manager_accessor_qr_mode_timeout_value.c", __LINE__);
    return kEsfSystemManagerResultInternalError;
  }
  return kEsfSystemManagerResultOk;
}

EsfSystemManagerResult EsfSystemManagerLoadQrModeTimeoutValueFromPsm(
    EsfParameterStorageManagerHandle handle,
    const EsfParameterStorageManagerQrModeTimeoutValueMask* mask,
    EsfParameterStorageManagerQrModeTimeoutValue* data) {
  if ((mask == NULL) || (data == NULL)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Parameter error. mask=%p data=%p.",
                     "system_manager_accessor_qr_mode_timeout_value.c",
                     __LINE__, mask, data);
    return kEsfSystemManagerResultParamError;
  }

  EsfParameterStorageManagerStatus status = EsfParameterStorageManagerLoad(
      handle, (EsfParameterStorageManagerMask)mask,
      (EsfParameterStorageManagerData)data, &kStructInfo, NULL);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to load from Parameter Storage Manager.",
                     "system_manager_accessor_qr_mode_timeout_value.c",
                     __LINE__);
    return kEsfSystemManagerResultInternalError;
  }

  if (mask->qr_mode_timeout_value) {
    if (EsfParameterStorageManagerIsDataEmpty(
            (EsfParameterStorageManagerData)data, &kStructInfo, 0)) {
      // Return default value.
      WRITE_DLOG_DEBUG(
          MODULE_ID_SYSTEM,
          "%s-%d:Return default value. qr_mode_timeout_value=%d",
          "system_manager_accessor_qr_mode_timeout_value.c", __LINE__,
          CONFIG_EXTERNAL_SYSTEM_MANAGER_DEFAULT_QR_MODE_TIMEOUT_VALUE);
      data->qr_mode_timeout_value.data =
          (uint8_t)CONFIG_EXTERNAL_SYSTEM_MANAGER_DEFAULT_QR_MODE_TIMEOUT_VALUE;
    }
  }

  return kEsfSystemManagerResultOk;
}

STATIC bool EsfParameterStorageManagerMaskEnabledQrModeTimeoutValue(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfParameterStorageManagerQrModeTimeoutValueMask, qr_mode_timeout_value,
      mask);
}
