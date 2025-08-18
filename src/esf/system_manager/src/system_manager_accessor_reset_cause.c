/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "system_manager_accessor_reset_cause.h"

#include "parameter_storage_manager.h"
#include "parameter_storage_manager_common.h"
#include "pl_system_manager.h"
#include "power_manager.h"
#include "system_manager.h"
#include "utility_log.h"
#include "utility_log_module_id.h"

#ifndef ESF_SYSTEM_MANAGER_REMOVE_STATIC
#define STATIC static
#else  // ESF_SYSTEM_MANAGER_REMOVE_STATIC
#define STATIC
#endif  // ESF_SYSTEM_MANAGER_REMOVE_STATIC

// """Checks if the Reset Cause mask is enabled.
// This function examines the provided mask to determine whether the
// Reset Cause bit is enabled. The function uses the macro
// ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED to perform the bitwise
// check and return the result.
// Args:
//     [IN] mask (EsfParameterStorageManagerMask): The mask that contains
//        various flags, including the Reset Cause bit.
// Returns:
//     bool: `true` if the Reset Cause mask is enabled, otherwise
//       `false`.
// """
STATIC bool EsfParameterStorageManagerMaskEnabledEvpResetCause(
    EsfParameterStorageManagerMask mask);

// """Checks if the Reset Cause mask is enabled.
// This function examines the provided mask to determine whether the
// Reset Cause bit is enabled. The function uses the macro
// ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED to perform the bitwise
// check and return the result.
// Args:
//     [IN] mask (EsfParameterStorageManagerMask): The mask that contains
//        various flags, including the Reset Cause bit.
// Returns:
//     bool: `true` if the Reset Cause mask is enabled, otherwise
//       `false`.
// """
STATIC bool EsfParameterStorageManagerMaskEnabledResetCause(
    EsfParameterStorageManagerMask mask);

// Information for accessing the Reset Cause structure members.
static const EsfParameterStorageManagerMemberInfo kMemberInfo[] = {
    {
        .id = kEsfParameterStorageManagerItemEvpExceptionFactor,
        .type = kEsfParameterStorageManagerItemTypeString,
        .offset = offsetof(EsfParameterStorageManagerResetCause,
                           evp_reset_cause),
        .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
            EsfParameterStorageManagerResetCause, evp_reset_cause),
        .enabled = EsfParameterStorageManagerMaskEnabledEvpResetCause,
        .custom = NULL,
    },
    {
        .id = kEsfParameterStorageManagerItemExceptionFactor,
        .type = kEsfParameterStorageManagerItemTypeString,
        .offset = offsetof(EsfParameterStorageManagerResetCause, reset_cause),
        .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
            EsfParameterStorageManagerResetCause, reset_cause),
        .enabled = EsfParameterStorageManagerMaskEnabledResetCause,
        .custom = NULL,
    },
};

// Information for accessing the Reset Cause structure.
static const EsfParameterStorageManagerStructInfo kStructInfo = {
    .items_num = sizeof(kMemberInfo) / sizeof(kMemberInfo[0]),
    .items = kMemberInfo,
};

EsfSystemManagerResult EsfSystemManagerSaveResetCauseToPsm(
    EsfParameterStorageManagerHandle handle,
    const EsfParameterStorageManagerResetCauseMask* mask,
    const EsfParameterStorageManagerResetCause* data) {
  if ((mask == NULL) || (data == NULL)) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM, "%s-%d:Parameter error. mask=%p data=%p.",
        "system_manager_accessor_reset_cause.c", __LINE__, mask, data);
    return kEsfSystemManagerResultParamError;
  }

  EsfParameterStorageManagerStatus status = EsfParameterStorageManagerSave(
      handle, (EsfParameterStorageManagerMask)mask,
      (EsfParameterStorageManagerData)data, &kStructInfo, NULL);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to save to Parameter Storage Manager.",
                     "system_manager_accessor_reset_cause.c", __LINE__);
    return kEsfSystemManagerResultInternalError;
  }
  return kEsfSystemManagerResultOk;
}

EsfSystemManagerResult EsfSystemManagerLoadResetCauseFromPsm(
    EsfParameterStorageManagerHandle handle,
    const EsfParameterStorageManagerResetCauseMask* mask,
    EsfParameterStorageManagerResetCause* data) {
  if ((mask == NULL) || (data == NULL)) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM, "%s-%d:Parameter error. mask=%p data=%p.",
        "system_manager_accessor_reset_cause.c", __LINE__, mask, data);
    return kEsfSystemManagerResultParamError;
  }

  EsfParameterStorageManagerStatus status = EsfParameterStorageManagerLoad(
      handle, (EsfParameterStorageManagerMask)mask,
      (EsfParameterStorageManagerData)data, &kStructInfo, NULL);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to load from Parameter Storage Manager.",
                     "system_manager_accessor_reset_cause.c", __LINE__);
    return kEsfSystemManagerResultInternalError;
  }

  return kEsfSystemManagerResultOk;
}

STATIC bool EsfParameterStorageManagerMaskEnabledEvpResetCause(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfParameterStorageManagerResetCauseMask, evp_reset_cause, mask);
}

STATIC bool EsfParameterStorageManagerMaskEnabledResetCause(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfParameterStorageManagerResetCauseMask, reset_cause, mask);
}

EsfSystemManagerResult EsfSystemManagerSelectResetCause(
    EsfSystemManagerResetCauseType* selected_reset_cause_type) {
  if (selected_reset_cause_type == NULL) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Parameter error. selected_reset_cause_type is NULL.",
        "system_manager_accessor_reset_cause.c", __LINE__);
    return kEsfSystemManagerResultParamError;
  }

  EsfSystemManagerResetCauseType reset_cause_type =
      kEsfSystemManagerResetCauseTypeNone;

  /* get EvpResetCause */
  EsfSystemManagerEvpResetCause evp_reset_cause =
      kEsfSystemManagerEvpResetCauseClear;

  EsfSystemManagerResult result =
      EsfSystemManagerGetEvpResetCause(&evp_reset_cause);
  if (result != kEsfSystemManagerResultOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to get EvpResetCause. result=%d",
                     "system_manager_accessor_reset_cause.c", __LINE__, result);
    return result;
  }

  if (evp_reset_cause != kEsfSystemManagerEvpResetCauseClear) {
    reset_cause_type = kEsfSystemManagerResetCauseTypeEvp;
    goto exit;
  }

  /* get ResetCause */
  EsfSystemManagerResetCause reset_cause = kEsfSystemManagerResetCauseUnknown;

  result = EsfSystemManagerGetResetCause(&reset_cause);
  if (result != kEsfSystemManagerResultOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to get ResetCause. result=%d",
                     "system_manager_accessor_reset_cause.c", __LINE__, result);
    return result;
  }

  if ((reset_cause != kEsfSystemManagerResetCauseDefault) &&
      (reset_cause != kEsfSystemManagerResetCauseClear)) {
    reset_cause_type = kEsfSystemManagerResetCauseTypePm;
    goto exit;
  }

exit:

  *selected_reset_cause_type = reset_cause_type;

  return kEsfSystemManagerResultOk;
}

EsfSystemManagerResult EsfSystemManagerConvertStringToEvpResetCause(
    const char* reset_cause_str,
    EsfSystemManagerEvpResetCause* evp_reset_cause) {
  if ((reset_cause_str == NULL) || (evp_reset_cause == NULL)) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Parameter error. reset_cause_str=%p evp_reset_cause=%p.",
        "system_manager_accessor_reset_cause.c", __LINE__, reset_cause_str,
        evp_reset_cause);
    return kEsfSystemManagerResultParamError;
  }

  if (strcmp(reset_cause_str, "1") == 0) {
    *evp_reset_cause = kEsfSystemManagerEvpResetCauseMemoryAllocFailure;
  } else if (strcmp(reset_cause_str, "2") == 0) {
    *evp_reset_cause = kEsfSystemManagerEvpResetCauseFreezeDetection;
  } else {
    *evp_reset_cause = kEsfSystemManagerEvpResetCauseClear;
  }

  return kEsfSystemManagerResultOk;
}

EsfSystemManagerResult EsfSystemManagerConvertStringToResetCause(
    const char* reset_cause_str, EsfSystemManagerResetCause* reset_cause) {
  if ((reset_cause_str == NULL) || (reset_cause == NULL)) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Parameter error. reset_cause_str=%p reset_cause=%p.",
        "system_manager_accessor_reset_cause.c", __LINE__, reset_cause_str,
        reset_cause);
    return kEsfSystemManagerResultParamError;
  }

  if (strcmp(reset_cause_str, "0") == 0) {
    *reset_cause = kEsfSystemManagerResetCauseSysChipPowerOnReset;
  } else if (strcmp(reset_cause_str, "1") == 0) {
    *reset_cause = kEsfSystemManagerResetCauseSysBrownOut;
  } else if (strcmp(reset_cause_str, "2") == 0) {
    *reset_cause = kEsfSystemManagerResetCauseCoreSoft;
  } else if (strcmp(reset_cause_str, "3") == 0) {
    *reset_cause = kEsfSystemManagerResetCauseCoreDeepSleep;
  } else if (strcmp(reset_cause_str, "4") == 0) {
    *reset_cause = kEsfSystemManagerResetCauseWDT;
  } else if (strcmp(reset_cause_str, "5") == 0) {
    *reset_cause = kEsfSystemManagerResetCauseSoftResetNormal;
  } else if (strcmp(reset_cause_str, "6") == 0) {
    *reset_cause = kEsfSystemManagerResetCauseSoftResetError;
  } else if (strcmp(reset_cause_str, "7") == 0) {
    *reset_cause = kEsfSystemManagerResetCauseDefault;
  } else if (strcmp(reset_cause_str, "") == 0) {
    *reset_cause = kEsfSystemManagerResetCauseClear;
  } else {
    *reset_cause = kEsfSystemManagerResetCauseUnknown;
  }

  return kEsfSystemManagerResultOk;
}

EsfSystemManagerResult EsfSystemManagerConvertResetCauseToString(
    EsfSystemManagerResetCause reset_cause, char* reset_cause_str,
    size_t reset_cause_str_size) {
  if (reset_cause_str == NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Parameter error. reset_cause_str is NULL.",
                     "system_manager_accessor_reset_cause.c", __LINE__);
    return kEsfSystemManagerResultParamError;
  }

  if (reset_cause == kEsfSystemManagerResetCauseSysChipPowerOnReset) {
    strncpy(reset_cause_str, "0", reset_cause_str_size);
  } else if (reset_cause == kEsfSystemManagerResetCauseSysBrownOut) {
    strncpy(reset_cause_str, "1", reset_cause_str_size);
  } else if (reset_cause == kEsfSystemManagerResetCauseCoreSoft) {
    strncpy(reset_cause_str, "2", reset_cause_str_size);
  } else if (reset_cause == kEsfSystemManagerResetCauseCoreDeepSleep) {
    strncpy(reset_cause_str, "3", reset_cause_str_size);
  } else if (reset_cause == kEsfSystemManagerResetCauseWDT) {
    strncpy(reset_cause_str, "4", reset_cause_str_size);
  } else if (reset_cause == kEsfSystemManagerResetCauseSoftResetNormal) {
    strncpy(reset_cause_str, "5", reset_cause_str_size);
  } else if (reset_cause == kEsfSystemManagerResetCauseSoftResetError) {
    strncpy(reset_cause_str, "6", reset_cause_str_size);
  } else if (reset_cause == kEsfSystemManagerResetCauseDefault) {
    strncpy(reset_cause_str, "7", reset_cause_str_size);
  } else if (reset_cause == kEsfSystemManagerResetCauseClear) {
    strncpy(reset_cause_str, "", reset_cause_str_size);
  } else {
    return kEsfSystemManagerResultParamError;
  }

  reset_cause_str[reset_cause_str_size - 1] = '\0';

  return kEsfSystemManagerResultOk;
}

EsfSystemManagerResult EsfSystemManagerConvertEvpResetCauseToString(
    EsfSystemManagerEvpResetCause evp_reset_cause, char* reset_cause_str,
    size_t reset_cause_str_size) {
  if (reset_cause_str == NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Parameter error. reset_cause_str is NULL.",
                     "system_manager_accessor_reset_cause.c", __LINE__);
    return kEsfSystemManagerResultParamError;
  }

  if (evp_reset_cause == kEsfSystemManagerEvpResetCauseMemoryAllocFailure) {
    strncpy(reset_cause_str, "1", reset_cause_str_size);
  } else if (evp_reset_cause == kEsfSystemManagerEvpResetCauseFreezeDetection) {
    strncpy(reset_cause_str, "2", reset_cause_str_size);
  } else {
    return kEsfSystemManagerResultParamError;
  }

  reset_cause_str[reset_cause_str_size - 1] = '\0';

  return kEsfSystemManagerResultOk;
}

EsfSystemManagerResetCause EsfSystemManagerConvertResetCausePmToSm(
    EsfPwrMgrResetCause reset_cause) {
  switch (reset_cause) {
    case kEsfPwrMgrResetCauseSysChipPowerOnReset:
      return kEsfSystemManagerResetCauseSysChipPowerOnReset;
    case kEsfPwrMgrResetCauseSysBrownOut:
      return kEsfSystemManagerResetCauseSysBrownOut;
    case kEsfPwrMgrResetCauseCoreSoft:
      return kEsfSystemManagerResetCauseCoreSoft;
    case kEsfPwrMgrResetCauseCoreDeepSleep:
      return kEsfSystemManagerResetCauseCoreDeepSleep;
    case kEsfPwrMgrResetCauseWDT:
      return kEsfSystemManagerResetCauseWDT;
    default:
      return kEsfSystemManagerResetCauseUnknown;
  }
}

PlSystemManagerResetCause EsfSystemManagerConvertResetCauseSmToPlSm(
    EsfSystemManagerResetCause reset_cause) {
  switch (reset_cause) {
    case kEsfSystemManagerResetCauseSysChipPowerOnReset:
      return kPlSystemManagerResetCauseSysChipPowerOnReset;
    case kEsfSystemManagerResetCauseSysBrownOut:
      return kPlSystemManagerResetCauseSysBrownOut;
    case kEsfSystemManagerResetCauseCoreSoft:
      return kPlSystemManagerResetCauseCoreSoft;
    case kEsfSystemManagerResetCauseCoreDeepSleep:
      return kPlSystemManagerResetCauseCoreDeepSleep;
    case kEsfSystemManagerResetCauseWDT:
      return kPlSystemManagerResetCauseWDT;
    case kEsfSystemManagerResetCauseDefault:
      return kPlSystemManagerResetCauseDefault;
    case kEsfSystemManagerResetCauseClear:
      return kPlSystemManagerResetCauseClear;
    default:
      return kPlSystemManagerResetCauseUnknown;
  }
}

EsfSystemManagerResult EsfSystemManagerGetResetCauseEventId(
    const char* reset_cause_str, EsfSystemManagerResetCauseType cause_type,
    uint8_t* event_id) {
  if (reset_cause_str == NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Parameter error. reset_cause_str is NULL.",
                     "system_manager_accessor_reset_cause.c", __LINE__);
    return kEsfSystemManagerResultParamError;
  }

  if (cause_type == kEsfSystemManagerResetCauseTypeEvp) {
    if (strcmp(reset_cause_str, "1") == 0) {
      *event_id = 0x40;
    } else if (strcmp(reset_cause_str, "2") == 0) {
      *event_id = 0x41;
    } else {
      return kEsfSystemManagerResultParamError;
    }
  } else {
    if (strcmp(reset_cause_str, "0") == 0) {
      *event_id = 0x01;  // System chip power on reset
    } else if (strcmp(reset_cause_str, "1") == 0) {
      *event_id = 0x02;  // System brown out reset
    } else if (strcmp(reset_cause_str, "2") == 0) {
      *event_id = 0x04;  // Core soft reset
    } else if (strcmp(reset_cause_str, "3") == 0) {
      *event_id = 0x05;  // Core deep sleep reset
    } else if (strcmp(reset_cause_str, "4") == 0) {
      *event_id = 0x06;  // WDT reset
    } else if (strcmp(reset_cause_str, "5") == 0) {
      *event_id = 0x03;  // Soft reset normal
    } else if (strcmp(reset_cause_str, "6") == 0) {
      *event_id = 0x04;  // Soft reset error
    } else {
      return kEsfSystemManagerResultParamError;
    }
  }

  return kEsfSystemManagerResultOk;
}
