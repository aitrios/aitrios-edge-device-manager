/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "log_manager_setting.h"

#include "log_manager_internal.h"
#include "parameter_storage_manager.h"
#include "parameter_storage_manager_common.h"

#ifndef CONFIG_EXTERNAL_LOG_MANAGER_DEFAULT_DLOG_LEVEL
#define CONFIG_EXTERNAL_LOG_MANAGER_DEFAULT_DLOG_LEVEL (6)  // info
#endif
#ifndef CONFIG_EXTERNAL_LOG_MANAGER_DEFAULT_ELOG_LEVEL
#define CONFIG_EXTERNAL_LOG_MANAGER_DEFAULT_ELOG_LEVEL (6)  // info
#endif
#ifndef CONFIG_EXTERNAL_LOG_MANAGER_DEFAULT_DLOG_DEST
#define CONFIG_EXTERNAL_LOG_MANAGER_DEFAULT_DLOG_DEST (1)  // uart
#endif
#ifndef CONFIG_EXTERNAL_LOG_MANAGER_DEFAULT_DLOG_FILTER
#define CONFIG_EXTERNAL_LOG_MANAGER_DEFAULT_DLOG_FILTER \
  (ESF_LOG_MANAGER_INTERNAL_NON_MODULE_ID)
#endif
#ifndef CONFIG_EXTERNAL_LOG_MANAGER_DEFAULT_STORAGE_NAME
#define CONFIG_EXTERNAL_LOG_MANAGER_DEFAULT_STORAGE_NAME ""
#endif
#ifndef CONFIG_EXTERNAL_LOG_MANAGER_DEFAULT_STORAGE_PATH
#define CONFIG_EXTERNAL_LOG_MANAGER_DEFAULT_STORAGE_PATH ""
#endif

#ifndef LOG_MANAGER_REMOVE_STATIC
#define STATIC static
#else   // LOG_MANAGER_REMOVE_STATIC
#define STATIC
#endif  // LOG_MANAGER_REMOVE_STATIC

// """Performs mask judgment of DlogLevel
// Args:
//    mask (EsfParameterStorageManagerMask): a pointer to an object of
//      EsfLogManagerParameterForPsmMask.
// Returns:
//    true: If the mask value is valid
//    false: If the mask value is invalid
// """
STATIC bool EsfLogManagerIsDlogLevelMaskEnabled(
    EsfParameterStorageManagerMask mask);
// """Performs mask judgment of ElogLevel
// Args:
//    mask (EsfParameterStorageManagerMask): a pointer to an object of
//      EsfLogManagerParameterForPsmMask.
// Returns:
//    true: If the mask value is valid
//    false: If the mask value is invalid
// """
STATIC bool EsfLogManagerIsElogLevelMaskEnabled(
    EsfParameterStorageManagerMask mask);
// """Performs mask judgment of DlogDest
// Args:
//    mask (EsfParameterStorageManagerMask): a pointer to an object of
//      EsfLogManagerParameterForPsmMask.
// Returns:
//    true: If the mask value is valid
//    false: If the mask value is invalid
// """
STATIC bool EsfLogManagerIsDlogDestMaskEnabled(
    EsfParameterStorageManagerMask mask);
// """Performs mask judgment of DlogFilter
// Args:
//    mask (EsfParameterStorageManagerMask): a pointer to an object of
//      EsfLogManagerParameterForPsmMask.
// Returns:
//    true: If the mask value is valid
//    false: If the mask value is invalid
// """
STATIC bool EsfLogManagerIsDlogFilterMaskEnabled(
    EsfParameterStorageManagerMask mask);
// """Performs mask judgment of DlogUseFlash
// Args:
//    mask (EsfParameterStorageManagerMask): a pointer to an object of
//      EsfLogManagerParameterForPsmMask.
// Returns:
//    true: If the mask value is valid
//    false: If the mask value is invalid
// """
STATIC bool EsfLogManagerIsDlogUseFlashMaskEnabled(
    EsfParameterStorageManagerMask mask);
// """Performs mask judgment of StorageName
// Args:
//    mask (EsfParameterStorageManagerMask): a pointer to an object of
//      EsfLogManagerParameterForPsmMask.
// Returns:
//    true: If the mask value is valid
//    false: If the mask value is invalid
// """
STATIC bool EsfLogManagerIsStorageNameMaskEnabled(
    EsfParameterStorageManagerMask mask);
// """Performs mask judgment of StoragePath
// Args:
//    mask (EsfParameterStorageManagerMask): a pointer to an object of
//      EsfLogManagerParameterForPsmMask.
// Returns:
//    true: If the mask value is valid
//    false: If the mask value is invalid
// """
STATIC bool EsfLogManagerIsStoragePathMaskEnabled(
    EsfParameterStorageManagerMask mask);

// """Converting EsfLogManagerDlogLevel to values ?for the log manager
// Args:
//    value (uint8_t): The value to convert
// Returns:
//    uint8_t: Value converted to EsfLogManagerDlogLevel value
STATIC EsfLogManagerDlogLevel EsfLogManagerParamDlogLevelChangeForLogParam(
    uint8_t value);
// """Converting EsfLogManagerElogLevel to values ?for the log manager
// Args:
//    value (uint8_t): The value to convert
// Returns:
//    uint8_t: Value converted to EsfLogManagerElogLevel value
STATIC EsfLogManagerElogLevel EsfLogManagerParamElogLevelChangeForLogParam(
    uint8_t value);
// """Converting EsfLogManagerDlogDest to values ?for the log manager
// Args:
//    value (uint8_t): The value to convert
// Returns:
//    uint8_t: Value converted to EsfLogManagerDlogDest value
STATIC EsfLogManagerDlogDest EsfLogManagerParamDlogDestChangeForLogParam(
    uint8_t value);

static const EsfParameterStorageManagerMemberInfo
    kLogManagerParamMembersInfo[] = {
        {
            .id = kEsfParameterStorageManagerItemDebugLogLevel,
            .type = kEsfParameterStorageManagerItemTypeString,
            .offset = offsetof(EsfLogManagerParamsForPsm, dlog_level),
            .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
                EsfLogManagerParamsForPsm, dlog_level),
            .enabled = EsfLogManagerIsDlogLevelMaskEnabled,
            .custom = NULL,
        },
        {
            .id = kEsfParameterStorageManagerItemEventLogLevel,
            .type = kEsfParameterStorageManagerItemTypeString,
            .offset = offsetof(EsfLogManagerParamsForPsm, elog_level),
            .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
                EsfLogManagerParamsForPsm, elog_level),
            .enabled = EsfLogManagerIsElogLevelMaskEnabled,
            .custom = NULL,
        },
        {
            .id = kEsfParameterStorageManagerItemDebugLogDestination,
            .type = kEsfParameterStorageManagerItemTypeString,
            .offset = offsetof(EsfLogManagerParamsForPsm, dlog_dest),
            .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
                EsfLogManagerParamsForPsm, dlog_dest),
            .enabled = EsfLogManagerIsDlogDestMaskEnabled,
            .custom = NULL,
        },
        {
            .id = kEsfParameterStorageManagerItemLogFilter,
            .type = kEsfParameterStorageManagerItemTypeRaw,
            .offset = offsetof(EsfLogManagerParamsForPsm, dlog_filter),
            .size = ESF_PARAMETER_STORAGE_MANAGER_RAW_SIZEOF(
                EsfParameterStorageManagerDlogFilterRaw, data),
            .enabled = EsfLogManagerIsDlogFilterMaskEnabled,
            .custom = NULL,
        },
        {
            .id = kEsfParameterStorageManagerItemLogUseFlash,
            .type = kEsfParameterStorageManagerItemTypeRaw,
            .offset = offsetof(EsfLogManagerParamsForPsm, use_flash),
            .size = ESF_PARAMETER_STORAGE_MANAGER_RAW_SIZEOF(
                EsfParameterStorageManagerUseFlashRaw, data),
            .enabled = EsfLogManagerIsDlogUseFlashMaskEnabled,
            .custom = NULL,
        },
        {
            .id = kEsfParameterStorageManagerItemStorageName,
            .type = kEsfParameterStorageManagerItemTypeString,
            .offset = offsetof(EsfLogManagerParamsForPsm, storage_name),
            .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
                EsfLogManagerParamsForPsm, storage_name),
            .enabled = EsfLogManagerIsStorageNameMaskEnabled,
            .custom = NULL,
        },
        {
            .id = kEsfParameterStorageManagerItemStorageSubDirectoryPath,
            .type = kEsfParameterStorageManagerItemTypeString,
            .offset = offsetof(EsfLogManagerParamsForPsm, storage_path),
            .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
                EsfLogManagerParamsForPsm, storage_path),
            .enabled = EsfLogManagerIsStoragePathMaskEnabled,
            .custom = NULL,
        },
};
static const EsfParameterStorageManagerMemberInfo
    kLogManagerParamMembersInfo2[] = {
        {
            .id = kEsfParameterStorageManagerItemDebugLogLevel2,
            .type = kEsfParameterStorageManagerItemTypeString,
            .offset = offsetof(EsfLogManagerParamsForPsm, dlog_level),
            .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
                EsfLogManagerParamsForPsm, dlog_level),
            .enabled = EsfLogManagerIsDlogLevelMaskEnabled,
            .custom = NULL,
        },
        {
            .id = kEsfParameterStorageManagerItemEventLogLevel2,
            .type = kEsfParameterStorageManagerItemTypeString,
            .offset = offsetof(EsfLogManagerParamsForPsm, elog_level),
            .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
                EsfLogManagerParamsForPsm, elog_level),
            .enabled = EsfLogManagerIsElogLevelMaskEnabled,
            .custom = NULL,
        },
        {
            .id = kEsfParameterStorageManagerItemDebugLogDestination2,
            .type = kEsfParameterStorageManagerItemTypeString,
            .offset = offsetof(EsfLogManagerParamsForPsm, dlog_dest),
            .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
                EsfLogManagerParamsForPsm, dlog_dest),
            .enabled = EsfLogManagerIsDlogDestMaskEnabled,
            .custom = NULL,
        },
        {
            .id = kEsfParameterStorageManagerItemLogFilter2,
            .type = kEsfParameterStorageManagerItemTypeRaw,
            .offset = offsetof(EsfLogManagerParamsForPsm, dlog_filter),
            .size = ESF_PARAMETER_STORAGE_MANAGER_RAW_SIZEOF(
                EsfParameterStorageManagerDlogFilterRaw, data),
            .enabled = EsfLogManagerIsDlogFilterMaskEnabled,
            .custom = NULL,
        },
        {
            .id = kEsfParameterStorageManagerItemLogUseFlash2,
            .type = kEsfParameterStorageManagerItemTypeRaw,
            .offset = offsetof(EsfLogManagerParamsForPsm, use_flash),
            .size = ESF_PARAMETER_STORAGE_MANAGER_RAW_SIZEOF(
                EsfParameterStorageManagerUseFlashRaw, data),
            .enabled = EsfLogManagerIsDlogUseFlashMaskEnabled,
            .custom = NULL,
        },
        {
            .id = kEsfParameterStorageManagerItemStorageName2,
            .type = kEsfParameterStorageManagerItemTypeString,
            .offset = offsetof(EsfLogManagerParamsForPsm, storage_name),
            .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
                EsfLogManagerParamsForPsm, storage_name),
            .enabled = EsfLogManagerIsStorageNameMaskEnabled,
            .custom = NULL,
        },
        {
            .id = kEsfParameterStorageManagerItemStorageSubDirectoryPath2,
            .type = kEsfParameterStorageManagerItemTypeString,
            .offset = offsetof(EsfLogManagerParamsForPsm, storage_path),
            .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
                EsfLogManagerParamsForPsm, storage_path),
            .enabled = EsfLogManagerIsStoragePathMaskEnabled,
            .custom = NULL,
        },
};
static const EsfParameterStorageManagerMemberInfo
    kLogManagerParamMembersInfo3[] = {
        {
            .id = kEsfParameterStorageManagerItemDebugLogLevel3,
            .type = kEsfParameterStorageManagerItemTypeString,
            .offset = offsetof(EsfLogManagerParamsForPsm, dlog_level),
            .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
                EsfLogManagerParamsForPsm, dlog_level),
            .enabled = EsfLogManagerIsDlogLevelMaskEnabled,
            .custom = NULL,
        },
        {
            .id = kEsfParameterStorageManagerItemEventLogLevel3,
            .type = kEsfParameterStorageManagerItemTypeString,
            .offset = offsetof(EsfLogManagerParamsForPsm, elog_level),
            .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
                EsfLogManagerParamsForPsm, elog_level),
            .enabled = EsfLogManagerIsElogLevelMaskEnabled,
            .custom = NULL,
        },
        {
            .id = kEsfParameterStorageManagerItemDebugLogDestination3,
            .type = kEsfParameterStorageManagerItemTypeString,
            .offset = offsetof(EsfLogManagerParamsForPsm, dlog_dest),
            .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
                EsfLogManagerParamsForPsm, dlog_dest),
            .enabled = EsfLogManagerIsDlogDestMaskEnabled,
            .custom = NULL,
        },
        {
            .id = kEsfParameterStorageManagerItemLogFilter3,
            .type = kEsfParameterStorageManagerItemTypeRaw,
            .offset = offsetof(EsfLogManagerParamsForPsm, dlog_filter),
            .size = ESF_PARAMETER_STORAGE_MANAGER_RAW_SIZEOF(
                EsfParameterStorageManagerDlogFilterRaw, data),
            .enabled = EsfLogManagerIsDlogFilterMaskEnabled,
            .custom = NULL,
        },
        {
            .id = kEsfParameterStorageManagerItemLogUseFlash3,
            .type = kEsfParameterStorageManagerItemTypeRaw,
            .offset = offsetof(EsfLogManagerParamsForPsm, use_flash),
            .size = ESF_PARAMETER_STORAGE_MANAGER_RAW_SIZEOF(
                EsfParameterStorageManagerUseFlashRaw, data),
            .enabled = EsfLogManagerIsDlogUseFlashMaskEnabled,
            .custom = NULL,
        },
        {
            .id = kEsfParameterStorageManagerItemStorageName3,
            .type = kEsfParameterStorageManagerItemTypeString,
            .offset = offsetof(EsfLogManagerParamsForPsm, storage_name),
            .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
                EsfLogManagerParamsForPsm, storage_name),
            .enabled = EsfLogManagerIsStorageNameMaskEnabled,
            .custom = NULL,
        },
        {
            .id = kEsfParameterStorageManagerItemStorageSubDirectoryPath3,
            .type = kEsfParameterStorageManagerItemTypeString,
            .offset = offsetof(EsfLogManagerParamsForPsm, storage_path),
            .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
                EsfLogManagerParamsForPsm, storage_path),
            .enabled = EsfLogManagerIsStoragePathMaskEnabled,
            .custom = NULL,
        },
};
static const EsfParameterStorageManagerMemberInfo
    kLogManagerParamMembersInfo4[] = {
        {
            .id = kEsfParameterStorageManagerItemDebugLogLevel4,
            .type = kEsfParameterStorageManagerItemTypeString,
            .offset = offsetof(EsfLogManagerParamsForPsm, dlog_level),
            .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
                EsfLogManagerParamsForPsm, dlog_level),
            .enabled = EsfLogManagerIsDlogLevelMaskEnabled,
            .custom = NULL,
        },
        {
            .id = kEsfParameterStorageManagerItemEventLogLevel4,
            .type = kEsfParameterStorageManagerItemTypeString,
            .offset = offsetof(EsfLogManagerParamsForPsm, elog_level),
            .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
                EsfLogManagerParamsForPsm, elog_level),
            .enabled = EsfLogManagerIsElogLevelMaskEnabled,
            .custom = NULL,
        },
        {
            .id = kEsfParameterStorageManagerItemDebugLogDestination4,
            .type = kEsfParameterStorageManagerItemTypeString,
            .offset = offsetof(EsfLogManagerParamsForPsm, dlog_dest),
            .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
                EsfLogManagerParamsForPsm, dlog_dest),
            .enabled = EsfLogManagerIsDlogDestMaskEnabled,
            .custom = NULL,
        },
        {
            .id = kEsfParameterStorageManagerItemLogFilter4,
            .type = kEsfParameterStorageManagerItemTypeRaw,
            .offset = offsetof(EsfLogManagerParamsForPsm, dlog_filter),
            .size = ESF_PARAMETER_STORAGE_MANAGER_RAW_SIZEOF(
                EsfParameterStorageManagerDlogFilterRaw, data),
            .enabled = EsfLogManagerIsDlogFilterMaskEnabled,
            .custom = NULL,
        },
        {
            .id = kEsfParameterStorageManagerItemLogUseFlash4,
            .type = kEsfParameterStorageManagerItemTypeRaw,
            .offset = offsetof(EsfLogManagerParamsForPsm, use_flash),
            .size = ESF_PARAMETER_STORAGE_MANAGER_RAW_SIZEOF(
                EsfParameterStorageManagerUseFlashRaw, data),
            .enabled = EsfLogManagerIsDlogUseFlashMaskEnabled,
            .custom = NULL,
        },
        {
            .id = kEsfParameterStorageManagerItemStorageName4,
            .type = kEsfParameterStorageManagerItemTypeString,
            .offset = offsetof(EsfLogManagerParamsForPsm, storage_name),
            .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
                EsfLogManagerParamsForPsm, storage_name),
            .enabled = EsfLogManagerIsStorageNameMaskEnabled,
            .custom = NULL,
        },
        {
            .id = kEsfParameterStorageManagerItemStorageSubDirectoryPath4,
            .type = kEsfParameterStorageManagerItemTypeString,
            .offset = offsetof(EsfLogManagerParamsForPsm, storage_path),
            .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
                EsfLogManagerParamsForPsm, storage_path),
            .enabled = EsfLogManagerIsStoragePathMaskEnabled,
            .custom = NULL,
        },
};

static const EsfParameterStorageManagerStructInfo kLogManagerStructInfo = {
    .items_num = sizeof(kLogManagerParamMembersInfo) /
                 sizeof(kLogManagerParamMembersInfo[0]),
    .items = kLogManagerParamMembersInfo,
};
static const EsfParameterStorageManagerStructInfo kLogManagerStructInfo2 = {
    .items_num = sizeof(kLogManagerParamMembersInfo2) /
                 sizeof(kLogManagerParamMembersInfo2[0]),
    .items = kLogManagerParamMembersInfo2,
};
static const EsfParameterStorageManagerStructInfo kLogManagerStructInfo3 = {
    .items_num = sizeof(kLogManagerParamMembersInfo3) /
                 sizeof(kLogManagerParamMembersInfo3[0]),
    .items = kLogManagerParamMembersInfo3,
};
static const EsfParameterStorageManagerStructInfo kLogManagerStructInfo4 = {
    .items_num = sizeof(kLogManagerParamMembersInfo4) /
                 sizeof(kLogManagerParamMembersInfo4[0]),
    .items = kLogManagerParamMembersInfo4,
};

STATIC bool EsfLogManagerIsDlogLevelMaskEnabled(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfLogManagerParameterForPsmMask, dlog_level, mask);
}
STATIC bool EsfLogManagerIsElogLevelMaskEnabled(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfLogManagerParameterForPsmMask, elog_level, mask);
}
STATIC bool EsfLogManagerIsDlogDestMaskEnabled(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfLogManagerParameterForPsmMask, dlog_dest, mask);
}
STATIC bool EsfLogManagerIsDlogFilterMaskEnabled(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfLogManagerParameterForPsmMask, dlog_filter, mask);
}
STATIC bool EsfLogManagerIsDlogUseFlashMaskEnabled(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfLogManagerParameterForPsmMask, use_flash, mask);
}
STATIC bool EsfLogManagerIsStorageNameMaskEnabled(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfLogManagerParameterForPsmMask, storage_name, mask);
}
STATIC bool EsfLogManagerIsStoragePathMaskEnabled(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfLogManagerParameterForPsmMask, storage_path, mask);
}
STATIC EsfLogManagerDlogLevel EsfLogManagerParamDlogLevelChangeForLogParam(
    uint8_t value) {
  EsfLogManagerDlogLevel level = kEsfLogManagerDlogLevelCritical;
  switch (value) {
    case 2:
      level = kEsfLogManagerDlogLevelCritical;
      break;
    case 3:
      level = kEsfLogManagerDlogLevelError;
      break;
    case 4:
      level = kEsfLogManagerDlogLevelWarn;
      break;
    case 6:
      level = kEsfLogManagerDlogLevelInfo;
      break;
    case 7:
      level = kEsfLogManagerDlogLevelDebug;
      break;
    case 8:
      level = kEsfLogManagerDlogLevelTrace;
      break;
    default:
      level = kEsfLogManagerDlogLevelInfo;
      break;
  }
  return level;
}
STATIC EsfLogManagerElogLevel EsfLogManagerParamElogLevelChangeForLogParam(
    uint8_t value) {
  EsfLogManagerElogLevel level = kEsfLogManagerElogLevelCritical;
  switch (value) {
    case 2:
      level = kEsfLogManagerElogLevelCritical;
      break;
    case 3:
      level = kEsfLogManagerElogLevelError;
      break;
    case 4:
      level = kEsfLogManagerElogLevelWarn;
      break;
    case 6:
      level = kEsfLogManagerElogLevelInfo;
      break;
    case 7:
      level = kEsfLogManagerElogLevelDebug;
      break;
    case 8:
      level = kEsfLogManagerElogLevelTrace;
      break;
    default:
      level = kEsfLogManagerElogLevelInfo;
      break;
  }
  return level;
}
STATIC EsfLogManagerDlogDest EsfLogManagerParamDlogDestChangeForLogParam(
    uint8_t value) {
  EsfLogManagerDlogDest dest = kEsfLogManagerDlogDestUart;
  switch (value) {
    case 1:
      dest = kEsfLogManagerDlogDestUart;
      break;
    case 2:
      dest = kEsfLogManagerDlogDestStore;
      break;
    case 3:
      dest = kEsfLogManagerDlogDestBoth;
      break;
    default:
      dest = kEsfLogManagerDlogDestUart;
      break;
  }
  return dest;
}

EsfLogManagerStatus EsfLogManagerSaveParamsForPsm(
    EsfLogManagerSettingBlockType const block_type,
    EsfLogManagerParameterMask const *mask,
    EsfLogManagerParameterValue const *value) {
  if (block_type >= ESF_LOG_MANAGER_BLOCK_TYPE_MAX_NUM) {
    ESF_LOG_MANAGER_ERROR("Invalid parameter. block_type=%d\n", block_type);
    return kEsfLogManagerStatusParamError;
  }

  EsfLogManagerParameterForPsmMask *psm_mask =
      (EsfLogManagerParameterForPsmMask *)malloc(sizeof(*psm_mask));
  if (psm_mask == NULL) {
    ESF_LOG_MANAGER_ERROR("Failed to malloc.\n");
    return kEsfLogManagerStatusFailed;
  }
  EsfLogManagerParamsForPsm *psm_data =
      (EsfLogManagerParamsForPsm *)malloc(sizeof(*psm_data));
  if (psm_data == NULL) {
    ESF_LOG_MANAGER_ERROR("Failed to malloc.\n");
    free(psm_mask);
    return kEsfLogManagerStatusFailed;
  }

  memset(psm_mask, 0, sizeof(EsfLogManagerParameterForPsmMask));
  memset(psm_data, 0, sizeof(EsfLogManagerParamsForPsm));

  if (mask->dlog_level) {
    const char dlog_level[kEsfLogManagerDlogLevelNum] = {'2', '3', '4',
                                                         '6', '7', '8'};
    psm_mask->dlog_level = mask->dlog_level;
    psm_data->dlog_level[0] = dlog_level[value->dlog_level];
    psm_data->dlog_level[1] = '\0';
  }
  if (mask->elog_level) {
    const char elog_level[kEsfLogManagerElogLevelNum] = {'2', '3', '4',
                                                         '6', '7', '8'};
    psm_mask->elog_level = mask->elog_level;
    psm_data->elog_level[0] = elog_level[value->elog_level];
    psm_data->elog_level[1] = '\0';
  }
  if (mask->dlog_dest) {
    const char dlog_dest[kEsfLogManagerDlogDestNum] = {'1', '2', '3'};
    psm_mask->dlog_dest = mask->dlog_dest;
    psm_data->dlog_dest[0] = dlog_dest[value->dlog_dest];
    psm_data->dlog_dest[1] = '\0';
  }
  if (mask->dlog_filter) {
    psm_mask->dlog_filter = mask->dlog_filter;
    psm_data->dlog_filter.data = value->dlog_filter;
  }
  psm_data->use_flash.data = 0;
  psm_mask->use_flash = 0;
  if (mask->storage_name) {
    psm_mask->storage_name = mask->storage_name;
    size_t len = strlen(value->storage_name);
    strncpy(psm_data->storage_name, value->storage_name, len);
    psm_data->storage_name[len] = '\0';
  }
  if (mask->storage_path) {
    psm_mask->storage_path = mask->storage_path;
    size_t len = strlen(value->storage_path);
    strncpy(psm_data->storage_path, value->storage_path, len);
    psm_data->storage_path[len] = '\0';
  }

  EsfLogManagerStatus log_manager_status = kEsfLogManagerStatusOk;

  {
    EsfParameterStorageManagerHandle psm_handle =
        ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE;

    EsfParameterStorageManagerStatus status =
        EsfParameterStorageManagerOpen(&psm_handle);
    if (status != kEsfParameterStorageManagerStatusOk) {
      ESF_LOG_MANAGER_ERROR("Failed to open PSM. status=%d\n", status);
      log_manager_status = kEsfLogManagerStatusFailed;
      goto exit;
    }

    const EsfParameterStorageManagerStructInfo
        *struct_info[ESF_LOG_MANAGER_BLOCK_TYPE_MAX_NUM];
    struct_info[0] = &kLogManagerStructInfo;
    struct_info[1] = &kLogManagerStructInfo2;
    struct_info[2] = &kLogManagerStructInfo3;
    struct_info[3] = &kLogManagerStructInfo4;
    status = EsfParameterStorageManagerSave(
        psm_handle, (EsfParameterStorageManagerMask)psm_mask,
        (EsfParameterStorageManagerData)psm_data, struct_info[block_type],
        NULL);
    if (status != kEsfParameterStorageManagerStatusOk) {
      ESF_LOG_MANAGER_ERROR("Failed to save PSM. status=%d\n", status);
      (void)EsfParameterStorageManagerClose(psm_handle);
      log_manager_status = kEsfLogManagerStatusFailed;
      goto exit;
    }

    status = EsfParameterStorageManagerClose(psm_handle);
    if (status != kEsfParameterStorageManagerStatusOk) {
      ESF_LOG_MANAGER_ERROR("Failed to close PSM. status=%d\n", status);
      log_manager_status = kEsfLogManagerStatusFailed;
      goto exit;
    }
  }

  ESF_LOG_MANAGER_DEBUG(
      "Saved value. dlog_level=%s elog_level=%s dlog_dest=%s "
      "dlog_filter=%d storage_name=%s storage_path=%s\n",
      psm_data->dlog_level, psm_data->elog_level, psm_data->dlog_dest,
      psm_data->dlog_filter.data, psm_data->storage_name,
      psm_data->storage_path);

exit:
  free(psm_data);
  free(psm_mask);

  return log_manager_status;
}
EsfLogManagerStatus EsfLogManagerLoadParamsForPsm(
    EsfLogManagerSettingBlockType const block_type,
    EsfLogManagerParameterMask *mask, EsfLogManagerParameterValue *value) {
  if (block_type >= ESF_LOG_MANAGER_BLOCK_TYPE_MAX_NUM) {
    ESF_LOG_MANAGER_ERROR("Invalid parameter. block_type=%d\n", block_type);
    return kEsfLogManagerStatusParamError;
  }

  EsfLogManagerParamsForPsm psm_data = {0};

  bool use_default = false;
// Disable the process of setting parameters to the PSM.
#ifndef CONFIG_EXTERNAL_LOG_MANAGER_PSM_DISABLE
  EsfLogManagerParameterForPsmMask psm_mask = {0};
  psm_mask.dlog_level = mask->dlog_level;
  psm_mask.elog_level = mask->elog_level;
  psm_mask.dlog_dest = mask->dlog_dest;
  psm_mask.dlog_filter = mask->dlog_filter;
  psm_mask.storage_name = mask->storage_name;
  psm_mask.storage_path = mask->storage_path;

  {
    EsfParameterStorageManagerHandle psm_handle =
        ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE;

    EsfParameterStorageManagerStatus status =
        EsfParameterStorageManagerOpen(&psm_handle);
    if (status != kEsfParameterStorageManagerStatusOk) {
      ESF_LOG_MANAGER_ERROR("Failed to open PSM. status=%d\n", status);
      use_default = true;
    } else {
      const EsfParameterStorageManagerStructInfo
          *struct_info[ESF_LOG_MANAGER_BLOCK_TYPE_MAX_NUM];
      struct_info[0] = &kLogManagerStructInfo;
      struct_info[1] = &kLogManagerStructInfo2;
      struct_info[2] = &kLogManagerStructInfo3;
      struct_info[3] = &kLogManagerStructInfo4;
      status = EsfParameterStorageManagerLoad(
          psm_handle, (EsfParameterStorageManagerMask)&psm_mask,
          (EsfParameterStorageManagerData)&psm_data, struct_info[block_type],
          NULL);
      if (status != kEsfParameterStorageManagerStatusOk) {
        ESF_LOG_MANAGER_ERROR("Failed to load PSM. status=%d\n", status);
        use_default = true;
      }
      status = EsfParameterStorageManagerClose(psm_handle);
      if (status != kEsfParameterStorageManagerStatusOk) {
        ESF_LOG_MANAGER_ERROR("Failed to close PSM. status=%d\n", status);
        // Do nothing.
      }
    }
  }
#else // CONFIG_EXTERNAL_LOG_MANAGER_PSM_DISABLE
  use_default = true;
#endif // CONFIG_EXTERNAL_LOG_MANAGER_PSM_DISABLE

  if (use_default) {
    // If load failed, set default parameter.
    value->dlog_level = EsfLogManagerParamDlogLevelChangeForLogParam(
        CONFIG_EXTERNAL_LOG_MANAGER_DEFAULT_DLOG_LEVEL);
    value->elog_level = EsfLogManagerParamElogLevelChangeForLogParam(
        CONFIG_EXTERNAL_LOG_MANAGER_DEFAULT_ELOG_LEVEL);
    value->dlog_dest = EsfLogManagerParamDlogDestChangeForLogParam(
        CONFIG_EXTERNAL_LOG_MANAGER_DEFAULT_DLOG_DEST);
    value->dlog_filter = CONFIG_EXTERNAL_LOG_MANAGER_DEFAULT_DLOG_FILTER;
    strncpy(value->storage_name,
            CONFIG_EXTERNAL_LOG_MANAGER_DEFAULT_STORAGE_NAME,
            ESF_LOG_MANAGER_STORAGE_NAME_MAX_SIZE);
    strncpy(value->storage_path,
            CONFIG_EXTERNAL_LOG_MANAGER_DEFAULT_STORAGE_PATH,
            ESF_LOG_MANAGER_STORAGE_SUB_DIR_PATH_MAX_SIZE);
    ESF_LOG_MANAGER_DEBUG(
        "Use default value. dlog_level=%d elog_level=%d dlog_dest=%d "
        "dlog_filter=%d storage_name=%s storage_path=%s\n",
        value->dlog_level, value->elog_level, value->dlog_dest,
        value->dlog_filter, value->storage_name, value->storage_path);
  } else {
    // Due to the current absence of a use case where mask is 0, no check for
    // mask will be performed.
    if (EsfParameterStorageManagerIsDataEmpty(
            (EsfParameterStorageManagerData)&psm_data, &kLogManagerStructInfo,
            0)) {
      // Return default value.
      value->dlog_level = EsfLogManagerParamDlogLevelChangeForLogParam(
          CONFIG_EXTERNAL_LOG_MANAGER_DEFAULT_DLOG_LEVEL);
    } else {
      uint8_t level = atoi(psm_data.dlog_level);
      value->dlog_level = EsfLogManagerParamDlogLevelChangeForLogParam(level);
    }

    if (EsfParameterStorageManagerIsDataEmpty(
            (EsfParameterStorageManagerData)&psm_data, &kLogManagerStructInfo,
            1)) {
      // Return default value.
      value->elog_level = EsfLogManagerParamElogLevelChangeForLogParam(
          CONFIG_EXTERNAL_LOG_MANAGER_DEFAULT_ELOG_LEVEL);
    } else {
      uint8_t level = atoi(psm_data.elog_level);
      value->elog_level = EsfLogManagerParamElogLevelChangeForLogParam(level);
    }

    if (EsfParameterStorageManagerIsDataEmpty(
            (EsfParameterStorageManagerData)&psm_data, &kLogManagerStructInfo,
            2)) {
      // Return default value.
      value->dlog_dest = EsfLogManagerParamDlogDestChangeForLogParam(
          CONFIG_EXTERNAL_LOG_MANAGER_DEFAULT_DLOG_DEST);
    } else {
      uint8_t dest = atoi(psm_data.dlog_dest);
      value->dlog_dest = EsfLogManagerParamDlogDestChangeForLogParam(dest);
    }

    if (EsfParameterStorageManagerIsDataEmpty(
            (EsfParameterStorageManagerData)&psm_data, &kLogManagerStructInfo,
            3)) {
      // Return default value.
      value->dlog_filter = CONFIG_EXTERNAL_LOG_MANAGER_DEFAULT_DLOG_FILTER;
    } else {
      value->dlog_filter = psm_data.dlog_filter.data;
    }

    if (EsfParameterStorageManagerIsDataEmpty(
            (EsfParameterStorageManagerData)&psm_data, &kLogManagerStructInfo,
            5)) {
      // Return default value.
      strncpy(value->storage_name,
              CONFIG_EXTERNAL_LOG_MANAGER_DEFAULT_STORAGE_NAME,
              sizeof(value->storage_name));
    } else {
      strncpy(value->storage_name, psm_data.storage_name,
              sizeof(value->storage_name));
    }

    if (EsfParameterStorageManagerIsDataEmpty(
            (EsfParameterStorageManagerData)&psm_data, &kLogManagerStructInfo,
            6)) {
      // Return default value.
      strncpy(value->storage_path,
              CONFIG_EXTERNAL_LOG_MANAGER_DEFAULT_STORAGE_PATH,
              sizeof(value->storage_path));
    } else {
      strncpy(value->storage_path, psm_data.storage_path,
              sizeof(value->storage_path));
    }
  }

  ESF_LOG_MANAGER_DEBUG(
      "Loaded value. dlog_level=%d elog_level=%d dlog_dest=%d "
      "dlog_filter=%d storage_name=%s storage_path=%s\n",
      value->dlog_level, value->elog_level, value->dlog_dest,
      value->dlog_filter, value->storage_name, value->storage_path);

  return kEsfLogManagerStatusOk;
}
