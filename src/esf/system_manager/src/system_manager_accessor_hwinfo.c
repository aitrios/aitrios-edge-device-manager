/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "system_manager_accessor_hwinfo.h"

#include "parameter_storage_manager.h"
#include "parameter_storage_manager_common.h"
#include "pl.h"
#include "pl_system_manager.h"
#include "system_manager.h"
#include "utility_log.h"
#include "utility_log_module_id.h"

#ifndef ESF_SYSTEM_MANAGER_REMOVE_STATIC
#define STATIC static
#else  // ESF_SYSTEM_MANAGER_REMOVE_STATIC
#define STATIC
#endif  // ESF_SYSTEM_MANAGER_REMOVE_STATIC

// """Checks if the HW Info mask is enabled.
// This function examines the provided mask to determine whether the
// HW Info bit is enabled. The function uses the macro
// ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED to perform the bitwise
// check and return the result.
// Args:
//     [IN] mask (EsfParameterStorageManagerMask): The mask that contains
//        various flags, including the HW Info bit.
// Returns:
//     bool: `true` if the HW Info mask is enabled, otherwise
//       `false`.
// """
STATIC bool EsfParameterStorageManagerMaskEnabledHwInfo(
    EsfParameterStorageManagerMask mask);

// Information for accessing the HW Info structure members.
static const EsfParameterStorageManagerMemberInfo kMemberInfo[] = {
    {
        .id = kEsfParameterStorageManagerItemHWInfoText,
        .type = kEsfParameterStorageManagerItemTypeBinaryPointer,
        .offset = offsetof(EsfParameterStorageManagerHwInfo, hw_info),
        .size = 0,
        .enabled = EsfParameterStorageManagerMaskEnabledHwInfo,
        .custom = NULL,
    },
};

// Information for accessing the HW Info structure.
static const EsfParameterStorageManagerStructInfo kStructInfo = {
    .items_num = sizeof(kMemberInfo) / sizeof(kMemberInfo[0]),
    .items = kMemberInfo,
};

EsfSystemManagerResult EsfSystemManagerLoadHwInfoFromPsm(
    EsfParameterStorageManagerHandle handle,
    const EsfParameterStorageManagerHwInfoMask *mask,
    EsfParameterStorageManagerHwInfo *data) {
  if ((mask == NULL) || (data == NULL)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Parameter error. mask=%p data=%p.",
                     "system_manager_accessor_hwinfo.c", __LINE__, mask, data);
    return kEsfSystemManagerResultParamError;
  }

  EsfParameterStorageManagerStatus status = EsfParameterStorageManagerLoad(
      handle, (EsfParameterStorageManagerMask)mask,
      (EsfParameterStorageManagerData)data, &kStructInfo, NULL);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to load from Parameter Storage Manager.",
                     "system_manager_accessor_hwinfo.c", __LINE__);
    return kEsfSystemManagerResultInternalError;
  }

  if (mask->hw_info) {
    if (EsfParameterStorageManagerIsDataEmpty(
            (EsfParameterStorageManagerData)data, &kStructInfo, 0)) {
      // WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (uint16_t)0x8d13);
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:HW Info is empty.",
                       "system_manager_accessor_hwinfo.c", __LINE__);
      return kEsfSystemManagerResultEmptyData;
    }
  }

  return kEsfSystemManagerResultOk;
}

STATIC bool EsfParameterStorageManagerMaskEnabledHwInfo(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfParameterStorageManagerHwInfoMask, hw_info, mask);
}

EsfSystemManagerResult EsfSystemManagerParseHwInfo(
    EsfSystemManagerHwInfo *data,
    EsfParameterStorageManagerHwInfo *data_struct) {
  if ((data == NULL) || (data_struct == NULL)) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM, "%s-%d:Parameter error. data=%p data_struct=%p.",
        "system_manager_accessor_hwinfo.c", __LINE__, data, data_struct);
    return kEsfSystemManagerResultParamError;
  }

  PlSystemManagerHwInfo *pl_data_struct = calloc(1,
                                                 sizeof(PlSystemManagerHwInfo));
  if (pl_data_struct == NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Failed to calloc.",
                     "system_manager_accessor_hwinfo.c", __LINE__);
    return kEsfSystemManagerResultInternalError;
  }

  PlErrCode err = PlSystemManagerParseHwInfo(
      (char *)(data_struct->hw_info.data), pl_data_struct);
  if (err != kPlErrCodeOk) {
    if (err == kPlErrNotFound) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:HW Info file not found. err=%d",
                       "system_manager_accessor_hwinfo.c", __LINE__, err);
      // If the HW Info is not found, return an empty data result.
      free(pl_data_struct);
      return kEsfSystemManagerResultEmptyData;
    } else {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:Failed to parse HW info. err=%d",
                       "system_manager_accessor_hwinfo.c", __LINE__, err);
      free(pl_data_struct);
      return kEsfSystemManagerResultInternalError;
    }
  }

  strncpy(data->model_name, pl_data_struct->model_name,
          sizeof(data->model_name) - 1);
  data->model_name[ESF_SYSTEM_MANAGER_HWINFO_MODEL_NAME_MAX_SIZE - 1] = '\0';
  strncpy(data->manufacturer_name, pl_data_struct->manufacturer_name,
          sizeof(data->manufacturer_name) - 1);
  data->manufacturer_name[ESF_SYSTEM_MANAGER_HWINFO_MANUFACTURER_NAME_MAX_SIZE -
                          1] = '\0';
  strncpy(data->product_serial_number, pl_data_struct->product_serial_number,
          sizeof(data->product_serial_number) - 1);
  data->product_serial_number
      [ESF_SYSTEM_MANAGER_HWINFO_PRODUCT_SERIAL_NUMBER_MAX_SIZE - 1] = '\0';
  strncpy(data->serial_number, pl_data_struct->serial_number,
          sizeof(data->serial_number) - 1);
  data->serial_number[ESF_SYSTEM_MANAGER_HWINFO_SERIAL_NUMBER_MAX_SIZE - 1] =
      '\0';
  strncpy(data->aiisp_chip_id, pl_data_struct->aiisp_chip_id,
          sizeof(data->aiisp_chip_id) - 1);
  data->aiisp_chip_id[ESF_SYSTEM_MANAGER_HWINFO_AIISP_CHIP_ID_MAX_SIZE - 1] =
      '\0';
  strncpy(data->sensor_id, pl_data_struct->sensor_id,
          sizeof(data->sensor_id) - 1);
  data->sensor_id[ESF_SYSTEM_MANAGER_HWINFO_SENSOR_ID_MAX_SIZE - 1] = '\0';
  strncpy(data->app_processor_type, pl_data_struct->app_processor_type,
          sizeof(data->app_processor_type) - 1);
  data->app_processor_type
      [ESF_SYSTEM_MANAGER_HWINFO_APP_PROCESSOR_TYPE_MAX_SIZE - 1] = '\0';
  strncpy(data->sensor_model_name, pl_data_struct->sensor_model_name,
          sizeof(data->sensor_model_name) - 1);
  data->sensor_model_name[ESF_SYSTEM_MANAGER_HWINFO_SENSOR_MODEL_NAME_MAX_SIZE -
                          1] = '\0';

  free(pl_data_struct);

  return kEsfSystemManagerResultOk;
}
