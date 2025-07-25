/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "system_manager_accessor_enrollment.h"

#include "parameter_storage_manager.h"
#include "parameter_storage_manager_common.h"
#include "system_manager.h"
#include "utility_log.h"
#include "utility_log_module_id.h"

#ifndef CONFIG_EXTERNAL_SYSTEM_MANAGER_DEFAULT_DPS_URL
#define CONFIG_EXTERNAL_SYSTEM_MANAGER_DEFAULT_DPS_URL ""
#endif
#ifndef CONFIG_EXTERNAL_SYSTEM_MANAGER_DEFAULT_COMMON_NAME
#define CONFIG_EXTERNAL_SYSTEM_MANAGER_DEFAULT_COMMON_NAME ""
#endif
#ifndef CONFIG_EXTERNAL_SYSTEM_MANAGER_DEFAULT_DPS_SCOPE_ID
#define CONFIG_EXTERNAL_SYSTEM_MANAGER_DEFAULT_DPS_SCOPE_ID ""
#endif
#ifndef CONFIG_EXTERNAL_SYSTEM_MANAGER_DEFAULT_PROJECT_ID
#define CONFIG_EXTERNAL_SYSTEM_MANAGER_DEFAULT_PROJECT_ID ""
#endif
#ifndef CONFIG_EXTERNAL_SYSTEM_MANAGER_DEFAULT_REGISTER_TOKEN
#define CONFIG_EXTERNAL_SYSTEM_MANAGER_DEFAULT_REGISTER_TOKEN ""
#endif

#ifndef ESF_SYSTEM_MANAGER_REMOVE_STATIC
#define STATIC static
#else  // ESF_SYSTEM_MANAGER_REMOVE_STATIC
#define STATIC
#endif  // ESF_SYSTEM_MANAGER_REMOVE_STATIC

// """Checks if the DPS URL mask is enabled.
// This function examines the provided mask to determine whether the
// DPS URL bit is enabled. The function uses the macro
// ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED to perform the bitwise
// check and return the result.
// Args:
//     [IN] mask (EsfParameterStorageManagerMask): The mask that contains
//        various flags, including the DPS URL bit.
// Returns:
//     bool: `true` if the DPS URL mask is enabled, otherwise
//       `false`.
// """
STATIC bool EsfParameterStorageManagerMaskEnabledEnrollmentDpsURL(
    EsfParameterStorageManagerMask mask);

// """Checks if the Common Name mask is enabled.
// This function examines the provided mask to determine whether the
// Common Name bit is enabled. The function uses the macro
// ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED to perform the bitwise
// check and return the result.
// Args:
//     [IN] mask (EsfParameterStorageManagerMask): The mask that contains
//        various flags, including the Common Name bit.
// Returns:
//     bool: `true` if the Common Name mask is enabled, otherwise
//       `false`.
// """
STATIC bool EsfParameterStorageManagerMaskEnabledEnrollmentCommonName(
    EsfParameterStorageManagerMask mask);

// """Checks if the DPS Scope ID mask is enabled.
// This function examines the provided mask to determine whether the
// DPS Scope ID bit is enabled. The function uses the macro
// ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED to perform the bitwise
// check and return the result.
// Args:
//     [IN] mask (EsfParameterStorageManagerMask): The mask that contains
//        various flags, including the DPS Scope ID bit.
// Returns:
//     bool: `true` if the DPS Scope ID mask is enabled, otherwise
//       `false`.
// """
STATIC bool EsfParameterStorageManagerMaskEnabledEnrollmentDpsScopeID(
    EsfParameterStorageManagerMask mask);

// """Checks if the Project ID mask is enabled.
// This function examines the provided mask to determine whether the
// Project ID bit is enabled. The function uses the macro
// ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED to perform the bitwise
// check and return the result.
// Args:
//     [IN] mask (EsfParameterStorageManagerMask): The mask that contains
//        various flags, including the Project ID bit.
// Returns:
//     bool: `true` if the Project ID mask is enabled, otherwise
//       `false`.
// """
STATIC bool EsfParameterStorageManagerMaskEnabledEnrollmentProjectID(
    EsfParameterStorageManagerMask mask);

// """Checks if the Register Token mask is enabled.
// This function examines the provided mask to determine whether the
// Register Token bit is enabled. The function uses the macro
// ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED to perform the bitwise
// check and return the result.
// Args:
//     [IN] mask (EsfParameterStorageManagerMask): The mask that contains
//        various flags, including the Register Token bit.
// Returns:
//     bool: `true` if the Register Token mask is enabled, otherwise
//       `false`.
// """
STATIC bool EsfParameterStorageManagerMaskEnabledEnrollmentRegisterToken(
    EsfParameterStorageManagerMask mask);

// Information for accessing the Enrollment structure members.
static const EsfParameterStorageManagerMemberInfo kMemberInfo[] = {
    {
        .id = kEsfParameterStorageManagerItemDpsURL,
        .type = kEsfParameterStorageManagerItemTypeString,
        .offset = offsetof(EsfParameterStorageManagerEnrollment, dps_url),
        .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
            EsfParameterStorageManagerEnrollment, dps_url),
        .enabled = EsfParameterStorageManagerMaskEnabledEnrollmentDpsURL,
        .custom = NULL,
    },
    {
        .id = kEsfParameterStorageManagerItemCommonName,
        .type = kEsfParameterStorageManagerItemTypeString,
        .offset = offsetof(EsfParameterStorageManagerEnrollment, common_name),
        .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
            EsfParameterStorageManagerEnrollment, common_name),
        .enabled = EsfParameterStorageManagerMaskEnabledEnrollmentCommonName,
        .custom = NULL,
    },
    {
        .id = kEsfParameterStorageManagerItemDpsScopeID,
        .type = kEsfParameterStorageManagerItemTypeString,
        .offset = offsetof(EsfParameterStorageManagerEnrollment, dps_scope_id),
        .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
            EsfParameterStorageManagerEnrollment, dps_scope_id),
        .enabled = EsfParameterStorageManagerMaskEnabledEnrollmentDpsScopeID,
        .custom = NULL,
    },
    {
        .id = kEsfParameterStorageManagerItemProjectID,
        .type = kEsfParameterStorageManagerItemTypeString,
        .offset = offsetof(EsfParameterStorageManagerEnrollment, project_id),
        .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
            EsfParameterStorageManagerEnrollment, project_id),
        .enabled = EsfParameterStorageManagerMaskEnabledEnrollmentProjectID,
        .custom = NULL,
    },
    {
        .id = kEsfParameterStorageManagerItemRegisterToken,
        .type = kEsfParameterStorageManagerItemTypeString,
        .offset = offsetof(EsfParameterStorageManagerEnrollment,
                           register_token),
        .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
            EsfParameterStorageManagerEnrollment, register_token),
        .enabled = EsfParameterStorageManagerMaskEnabledEnrollmentRegisterToken,
        .custom = NULL,
    },
};

// Information for accessing the Enrollment structure.
static const EsfParameterStorageManagerStructInfo kStructInfo = {
    .items_num = sizeof(kMemberInfo) / sizeof(kMemberInfo[0]),
    .items = kMemberInfo,
};

EsfSystemManagerResult EsfSystemManagerSaveEnrollmentToPsm(
    EsfParameterStorageManagerHandle handle,
    const EsfParameterStorageManagerEnrollmentMask* mask,
    const EsfParameterStorageManagerEnrollment* data) {
  if ((mask == NULL) || (data == NULL)) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM, "%s-%d:Parameter error. mask=%p data=%p.",
        "system_manager_accessor_enrollment.c", __LINE__, mask, data);
    return kEsfSystemManagerResultParamError;
  }

  EsfParameterStorageManagerStatus status = EsfParameterStorageManagerSave(
      handle, (EsfParameterStorageManagerMask)mask,
      (EsfParameterStorageManagerData)data, &kStructInfo, NULL);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to save to Parameter Storage Manager.",
                     "system_manager_accessor_enrollment.c", __LINE__);
    return kEsfSystemManagerResultInternalError;
  }
  return kEsfSystemManagerResultOk;
}

EsfSystemManagerResult EsfSystemManagerLoadEnrollmentFromPsm(
    EsfParameterStorageManagerHandle handle,
    const EsfParameterStorageManagerEnrollmentMask* mask,
    EsfParameterStorageManagerEnrollment* data) {
  if ((mask == NULL) || (data == NULL)) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM, "%s-%d:Parameter error. mask=%p data=%p.",
        "system_manager_accessor_enrollment.c", __LINE__, mask, data);
    return kEsfSystemManagerResultParamError;
  }

  EsfParameterStorageManagerStatus status = EsfParameterStorageManagerLoad(
      handle, (EsfParameterStorageManagerMask)mask,
      (EsfParameterStorageManagerData)data, &kStructInfo, NULL);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to load from Parameter Storage Manager.",
                     "system_manager_accessor_enrollment.c", __LINE__);
    return kEsfSystemManagerResultInternalError;
  }

  if (mask->dps_url) {
    if (EsfParameterStorageManagerIsDataEmpty(
            (EsfParameterStorageManagerData)data, &kStructInfo, 0)) {
      // Return default value.
      WRITE_DLOG_DEBUG(MODULE_ID_SYSTEM,
                       "%s-%d:Return default value. dps_url=%s",
                       "system_manager_accessor_enrollment.c", __LINE__,
                       CONFIG_EXTERNAL_SYSTEM_MANAGER_DEFAULT_DPS_URL);
      strncpy(data->dps_url, CONFIG_EXTERNAL_SYSTEM_MANAGER_DEFAULT_DPS_URL,
              sizeof(data->dps_url));
    }
  }
  if (mask->common_name) {
    if (EsfParameterStorageManagerIsDataEmpty(
            (EsfParameterStorageManagerData)data, &kStructInfo, 1)) {
      // Return default value.
      WRITE_DLOG_DEBUG(MODULE_ID_SYSTEM,
                       "%s-%d:Return default value. common_name=%s",
                       "system_manager_accessor_enrollment.c", __LINE__,
                       CONFIG_EXTERNAL_SYSTEM_MANAGER_DEFAULT_COMMON_NAME);
      strncpy(data->common_name,
              CONFIG_EXTERNAL_SYSTEM_MANAGER_DEFAULT_COMMON_NAME,
              sizeof(data->common_name));
    }
  }
  if (mask->dps_scope_id) {
    if (EsfParameterStorageManagerIsDataEmpty(
            (EsfParameterStorageManagerData)data, &kStructInfo, 2)) {
      // Return default value.
      WRITE_DLOG_DEBUG(MODULE_ID_SYSTEM,
                       "%s-%d:Return default value. dps_scope_id=%s",
                       "system_manager_accessor_enrollment.c", __LINE__,
                       CONFIG_EXTERNAL_SYSTEM_MANAGER_DEFAULT_DPS_SCOPE_ID);
      strncpy(data->dps_scope_id,
              CONFIG_EXTERNAL_SYSTEM_MANAGER_DEFAULT_DPS_SCOPE_ID,
              sizeof(data->dps_scope_id));
    }
  }
  if (mask->project_id) {
    if (EsfParameterStorageManagerIsDataEmpty(
            (EsfParameterStorageManagerData)data, &kStructInfo, 3)) {
      // Return default value.
      WRITE_DLOG_DEBUG(MODULE_ID_SYSTEM,
                       "%s-%d:Return default value. project_id=%s",
                       "system_manager_accessor_enrollment.c", __LINE__,
                       CONFIG_EXTERNAL_SYSTEM_MANAGER_DEFAULT_PROJECT_ID);
      strncpy(data->project_id,
              CONFIG_EXTERNAL_SYSTEM_MANAGER_DEFAULT_PROJECT_ID,
              sizeof(data->project_id));
    }
  }
  if (mask->register_token) {
    if (EsfParameterStorageManagerIsDataEmpty(
            (EsfParameterStorageManagerData)data, &kStructInfo, 4)) {
      // Return default value.
      WRITE_DLOG_DEBUG(MODULE_ID_SYSTEM,
                       "%s-%d:Return default value. register_token=%s",
                       "system_manager_accessor_enrollment.c", __LINE__,
                       CONFIG_EXTERNAL_SYSTEM_MANAGER_DEFAULT_REGISTER_TOKEN);
      strncpy(data->register_token,
              CONFIG_EXTERNAL_SYSTEM_MANAGER_DEFAULT_REGISTER_TOKEN,
              sizeof(data->register_token));
    }
  }
  return kEsfSystemManagerResultOk;
}

STATIC bool EsfParameterStorageManagerMaskEnabledEnrollmentDpsURL(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfParameterStorageManagerEnrollmentMask, dps_url, mask);
}

STATIC bool EsfParameterStorageManagerMaskEnabledEnrollmentCommonName(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfParameterStorageManagerEnrollmentMask, common_name, mask);
}

STATIC bool EsfParameterStorageManagerMaskEnabledEnrollmentDpsScopeID(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfParameterStorageManagerEnrollmentMask, dps_scope_id, mask);
}

STATIC bool EsfParameterStorageManagerMaskEnabledEnrollmentProjectID(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfParameterStorageManagerEnrollmentMask, project_id, mask);
}

STATIC bool EsfParameterStorageManagerMaskEnabledEnrollmentRegisterToken(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfParameterStorageManagerEnrollmentMask, register_token, mask);
}
