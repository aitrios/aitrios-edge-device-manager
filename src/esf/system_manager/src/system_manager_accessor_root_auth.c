/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "system_manager_accessor_root_auth.h"

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

// """Checks if the Root CA mask is enabled.
// This function examines the provided mask to determine whether the
// Root CA bit is enabled. The function uses the macro
// ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED to perform the bitwise
// check and return the result.
// Args:
//     [IN] mask (EsfParameterStorageManagerMask): The mask that contains
//        various flags, including the Root CA bit.
// Returns:
//     bool: `true` if the Root CA mask is enabled, otherwise
//       `false`.
// """
STATIC bool EsfParameterStorageManagerMaskEnabledRootCa(
    EsfParameterStorageManagerMask mask);

// """Checks if the Root CA Hash mask is enabled.
// This function examines the provided mask to determine whether the
// Root CA Hash bit is enabled. The function uses the macro
// ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED to perform the bitwise
// check and return the result.
// Args:
//     [IN] mask (EsfParameterStorageManagerMask): The mask that contains
//        various flags, including the Root CA Hash bit.
// Returns:
//     bool: `true` if the Root CA Hash mask is enabled, otherwise
//       `false`.
// """
STATIC bool EsfParameterStorageManagerMaskEnabledRootCaHash(
    EsfParameterStorageManagerMask mask);

// Information for accessing the Root Auth structure members.
static const EsfParameterStorageManagerMemberInfo kMemberInfo[] = {
    {
        .id = kEsfParameterStorageManagerItemPkiRootCerts,
        .type = kEsfParameterStorageManagerItemTypeOffsetBinaryPointer,
        .offset = offsetof(EsfParameterStorageManagerRootAuth, root_ca),
        .size = 0,
        .enabled = EsfParameterStorageManagerMaskEnabledRootCa,
        .custom = NULL,
    },
    {
        .id = kEsfParameterStorageManagerItemPkiRootCertsHash,
        .type = kEsfParameterStorageManagerItemTypeString,
        .offset = offsetof(EsfParameterStorageManagerRootAuth, root_ca_hash),
        .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
            EsfParameterStorageManagerRootAuth, root_ca_hash),
        .enabled = EsfParameterStorageManagerMaskEnabledRootCaHash,
        .custom = NULL,
    },
};

// Information for accessing the Root Auth structure.
static const EsfParameterStorageManagerStructInfo kStructInfo = {
    .items_num = sizeof(kMemberInfo) / sizeof(kMemberInfo[0]),
    .items = kMemberInfo,
};

EsfSystemManagerResult EsfSystemManagerLoadRootAuthFromPsm(
    EsfParameterStorageManagerHandle handle,
    const EsfParameterStorageManagerRootAuthMask* mask,
    EsfParameterStorageManagerRootAuth* data) {
  if ((mask == NULL) || (data == NULL)) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM, "%s-%d:Parameter error. mask=%p data=%p.",
        "system_manager_accessor_root_auth.c", __LINE__, mask, data);
    return kEsfSystemManagerResultParamError;
  }

  EsfParameterStorageManagerStatus status = EsfParameterStorageManagerLoad(
      handle, (EsfParameterStorageManagerMask)mask,
      (EsfParameterStorageManagerData)data, &kStructInfo, NULL);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to load from Parameter Storage Manager.",
                     "system_manager_accessor_root_auth.c", __LINE__);
    return kEsfSystemManagerResultInternalError;
  }

  if (mask->root_ca) {
    if (EsfParameterStorageManagerIsDataEmpty(
            (EsfParameterStorageManagerData)data, &kStructInfo, 0)) {
      WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (uint16_t)0x8d11);
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Root CA is empty.",
                       "system_manager_accessor_root_auth.c", __LINE__);
      return kEsfSystemManagerResultEmptyData;
    }
  }

  if (mask->root_ca_hash) {
    if (EsfParameterStorageManagerIsDataEmpty(
            (EsfParameterStorageManagerData)data, &kStructInfo, 1)) {
      WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (uint16_t)0x8d12);
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Root CA Hash is empty.",
                       "system_manager_accessor_root_auth.c", __LINE__);
      return kEsfSystemManagerResultEmptyData;
    }
  }

  return kEsfSystemManagerResultOk;
}

EsfSystemManagerResult EsfSystemManagerSaveRootAuthToPsm(
    EsfParameterStorageManagerHandle handle,
    const EsfParameterStorageManagerRootAuthMask* mask,
    const EsfParameterStorageManagerRootAuth* data) {
  if ((mask == NULL) || (data == NULL)) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM, "%s-%d:Parameter error. mask=%p data=%p.",
        "system_manager_accessor_root_auth.c", __LINE__, mask, data);
    return kEsfSystemManagerResultParamError;
  }

  EsfParameterStorageManagerStatus status = EsfParameterStorageManagerSave(
      handle, (EsfParameterStorageManagerMask)mask,
      (EsfParameterStorageManagerData)data, &kStructInfo, NULL);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to save to Parameter Storage Manager.",
                     "system_manager_accessor_root_auth.c", __LINE__);
    return kEsfSystemManagerResultInternalError;
  }
  return kEsfSystemManagerResultOk;
}

STATIC bool EsfParameterStorageManagerMaskEnabledRootCa(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfParameterStorageManagerRootAuthMask, root_ca, mask);
}

STATIC bool EsfParameterStorageManagerMaskEnabledRootCaHash(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfParameterStorageManagerRootAuthMask, root_ca_hash, mask);
}
