/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "system_manager_accessor_evp.h"

#include "parameter_storage_manager.h"
#include "parameter_storage_manager_common.h"
#include "system_manager.h"
#include "utility_log.h"
#include "utility_log_module_id.h"

#ifndef CONFIG_EXTERNAL_SYSTEM_MANAGER_DEFAULT_EVP_HUB_URL
#define CONFIG_EXTERNAL_SYSTEM_MANAGER_DEFAULT_EVP_HUB_URL ""
#endif
#ifndef CONFIG_EXTERNAL_SYSTEM_MANAGER_DEFAULT_EVP_HUB_PORT
#define CONFIG_EXTERNAL_SYSTEM_MANAGER_DEFAULT_EVP_HUB_PORT ""
#endif
#ifndef CONFIG_EXTERNAL_SYSTEM_MANAGER_DEFAULT_IOT_PLATFORM
#define CONFIG_EXTERNAL_SYSTEM_MANAGER_DEFAULT_IOT_PLATFORM ""
#endif
#ifndef CONFIG_EXTERNAL_SYSTEM_MANAGER_DEFAULT_EVP_TLS
#define CONFIG_EXTERNAL_SYSTEM_MANAGER_DEFAULT_EVP_TLS "0"
#endif

#ifndef ESF_SYSTEM_MANAGER_REMOVE_STATIC
#define STATIC static
#else  // ESF_SYSTEM_MANAGER_REMOVE_STATIC
#define STATIC
#endif  // ESF_SYSTEM_MANAGER_REMOVE_STATIC

// """Checks if the EVP Hub URL mask is enabled.
// This function examines the provided mask to determine whether the
// EVP Hub URL bit is enabled. The function uses the macro
// ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED to perform the bitwise
// check and return the result.
// Args:
//     [IN] mask (EsfParameterStorageManagerMask): The mask that contains
//        various flags, including the EVP Hub URL bit.
// Returns:
//     bool: `true` if the EVP Hub URL mask is enabled, otherwise
//       `false`.
// """
STATIC bool EsfParameterStorageManagerMaskEnabledEvpUrl(
    EsfParameterStorageManagerMask mask);

// """Checks if the EVP Hub Port mask is enabled.
// This function examines the provided mask to determine whether the
// EVP Hub Port bit is enabled. The function uses the macro
// ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED to perform the bitwise
// check and return the result.
// Args:
//     [IN] mask (EsfParameterStorageManagerMask): The mask that contains
//        various flags, including the EVP Hub Port bit.
// Returns:
//     bool: `true` if the EVP Hub Port mask is enabled, otherwise
//       `false`.
// """
STATIC bool EsfParameterStorageManagerMaskEnabledEvpPort(
    EsfParameterStorageManagerMask mask);

// """Checks if the IoT Platform mask is enabled.
// This function examines the provided mask to determine whether the
// IoT Platform bit is enabled. The function uses the macro
// ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED to perform the bitwise
// check and return the result.
// Args:
//     [IN] mask (EsfParameterStorageManagerMask): The mask that contains
//        various flags, including the IoT Platform bit.
// Returns:
//     bool: `true` if the IoT Platform mask is enabled, otherwise
//       `false`.
// """
STATIC bool EsfParameterStorageManagerMaskEnabledIotPlatform(
    EsfParameterStorageManagerMask mask);

// """Checks if the EVP TLS mask is enabled.
// This function examines the provided mask to determine whether the
// EVP TLS bit is enabled. The function uses the macro
// ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED to perform the bitwise
// check and return the result.
// Args:
//     [IN] mask (EsfParameterStorageManagerMask): The mask that contains
//        various flags, including the EVP TLS bit.
// Returns:
//     bool: `true` if the EVP TLS mask is enabled, otherwise
//       `false`.
// """
STATIC bool EsfParameterStorageManagerMaskEnabledEvpTls(
    EsfParameterStorageManagerMask mask);

// Information for accessing the EVP structure members.
static const EsfParameterStorageManagerMemberInfo kMemberInfo[] = {
    {
        .id = kEsfParameterStorageManagerItemEvpHubURL,
        .type = kEsfParameterStorageManagerItemTypeString,
        .offset = offsetof(EsfParameterStorageManagerEvp, evp_url),
        .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
            EsfParameterStorageManagerEvp, evp_url),
        .enabled = EsfParameterStorageManagerMaskEnabledEvpUrl,
        .custom = NULL,
    },
    {
        .id = kEsfParameterStorageManagerItemEvpHubPort,
        .type = kEsfParameterStorageManagerItemTypeString,
        .offset = offsetof(EsfParameterStorageManagerEvp, evp_port),
        .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
            EsfParameterStorageManagerEvp, evp_port),
        .enabled = EsfParameterStorageManagerMaskEnabledEvpPort,
        .custom = NULL,
    },
    {
        .id = kEsfParameterStorageManagerItemEvpIotPlatform,
        .type = kEsfParameterStorageManagerItemTypeString,
        .offset = offsetof(EsfParameterStorageManagerEvp, iot_platform),
        .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
            EsfParameterStorageManagerEvp, iot_platform),
        .enabled = EsfParameterStorageManagerMaskEnabledIotPlatform,
        .custom = NULL,
    },
    {
        .id = kEsfParameterStorageManagerItemEvpTls,
        .type = kEsfParameterStorageManagerItemTypeString,
        .offset = offsetof(EsfParameterStorageManagerEvp, evp_tls),
        .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
            EsfParameterStorageManagerEvp, evp_tls),
        .enabled = EsfParameterStorageManagerMaskEnabledEvpTls,
        .custom = NULL,
    },
};

// Information for accessing the EVP structure.
static const EsfParameterStorageManagerStructInfo kStructInfo = {
    .items_num = sizeof(kMemberInfo) / sizeof(kMemberInfo[0]),
    .items = kMemberInfo,
};

EsfSystemManagerResult EsfSystemManagerSaveEvpToPsm(
    EsfParameterStorageManagerHandle handle,
    const EsfParameterStorageManagerEvpMask* mask,
    const EsfParameterStorageManagerEvp* data) {
  if ((mask == NULL) || (data == NULL)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Parameter error. mask=%p data=%p.",
                     "system_manager_accessor_evp.c", __LINE__, mask, data);
    return kEsfSystemManagerResultParamError;
  }

  EsfParameterStorageManagerStatus status = EsfParameterStorageManagerSave(
      handle, (EsfParameterStorageManagerMask)mask,
      (EsfParameterStorageManagerData)data, &kStructInfo, NULL);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to save to Parameter Storage Manager.",
                     "system_manager_accessor_evp.c", __LINE__);
    return kEsfSystemManagerResultInternalError;
  }
  return kEsfSystemManagerResultOk;
}

EsfSystemManagerResult EsfSystemManagerLoadEvpFromPsm(
    EsfParameterStorageManagerHandle handle,
    const EsfParameterStorageManagerEvpMask* mask,
    EsfParameterStorageManagerEvp* data) {
  if ((mask == NULL) || (data == NULL)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Parameter error. mask=%p data=%p.",
                     "system_manager_accessor_evp.c", __LINE__, mask, data);
    return kEsfSystemManagerResultParamError;
  }

  EsfParameterStorageManagerStatus status = EsfParameterStorageManagerLoad(
      handle, (EsfParameterStorageManagerMask)mask,
      (EsfParameterStorageManagerData)data, &kStructInfo, NULL);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to load from Parameter Storage Manager.",
                     "system_manager_accessor_evp.c", __LINE__);
    return kEsfSystemManagerResultInternalError;
  }

  if (mask->evp_url) {
    if (EsfParameterStorageManagerIsDataEmpty(
            (EsfParameterStorageManagerData)data, &kStructInfo, 0)) {
      // Return default value.
      WRITE_DLOG_DEBUG(MODULE_ID_SYSTEM,
                       "%s-%d:Return default value. evp_url=%s",
                       "system_manager_accessor_evp.c", __LINE__,
                       CONFIG_EXTERNAL_SYSTEM_MANAGER_DEFAULT_EVP_HUB_URL);
      strncpy(data->evp_url, CONFIG_EXTERNAL_SYSTEM_MANAGER_DEFAULT_EVP_HUB_URL,
              sizeof(data->evp_url));
    }
  }
  if (mask->evp_port) {
    if (EsfParameterStorageManagerIsDataEmpty(
            (EsfParameterStorageManagerData)data, &kStructInfo, 1)) {
      // Return default value.
      WRITE_DLOG_DEBUG(MODULE_ID_SYSTEM,
                       "%s-%d:Return default value. evp_port=%s",
                       "system_manager_accessor_evp.c", __LINE__,
                       CONFIG_EXTERNAL_SYSTEM_MANAGER_DEFAULT_EVP_HUB_PORT);
      strncpy(data->evp_port,
              CONFIG_EXTERNAL_SYSTEM_MANAGER_DEFAULT_EVP_HUB_PORT,
              sizeof(data->evp_port));
    }
  }
  if (mask->iot_platform) {
    if (EsfParameterStorageManagerIsDataEmpty(
            (EsfParameterStorageManagerData)data, &kStructInfo, 2)) {
      // Return default value.
      WRITE_DLOG_DEBUG(MODULE_ID_SYSTEM,
                       "%s-%d:Return default value. iot_platform=%s",
                       "system_manager_accessor_evp.c", __LINE__,
                       CONFIG_EXTERNAL_SYSTEM_MANAGER_DEFAULT_IOT_PLATFORM);
      strncpy(data->iot_platform,
              CONFIG_EXTERNAL_SYSTEM_MANAGER_DEFAULT_IOT_PLATFORM,
              sizeof(data->iot_platform));
    }
  }
  if (mask->evp_tls) {
    if (EsfParameterStorageManagerIsDataEmpty(
            (EsfParameterStorageManagerData)data, &kStructInfo, 3)) {
      // Return default value.
      WRITE_DLOG_DEBUG(MODULE_ID_SYSTEM,
                       "%s-%d:Return default value. evp_tls=%s",
                       "system_manager_accessor_evp.c", __LINE__,
                       CONFIG_EXTERNAL_SYSTEM_MANAGER_DEFAULT_EVP_TLS);
      strncpy(data->evp_tls, CONFIG_EXTERNAL_SYSTEM_MANAGER_DEFAULT_EVP_TLS,
              sizeof(data->evp_tls));
    }
  }

  return kEsfSystemManagerResultOk;
}

STATIC bool EsfParameterStorageManagerMaskEnabledEvpUrl(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfParameterStorageManagerEvpMask, evp_url, mask);
}

STATIC bool EsfParameterStorageManagerMaskEnabledEvpPort(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfParameterStorageManagerEvpMask, evp_port, mask);
}

STATIC bool EsfParameterStorageManagerMaskEnabledIotPlatform(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfParameterStorageManagerEvpMask, iot_platform, mask);
}

STATIC bool EsfParameterStorageManagerMaskEnabledEvpTls(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfParameterStorageManagerEvpMask, evp_tls, mask);
}
