/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESF_LOG_MANAGER_SETTING_H_
#define ESF_LOG_MANAGER_SETTING_H_

#include "log_manager.h"

// Dlog filter structure definition for parameter storage manager
typedef struct EsfParameterStorageManagerDlogFilterRaw {
  uint32_t size;  // size
  uint32_t data;  // data
} EsfParameterStorageManagerDlogFilterRaw;

// Use flash structure definition for parameter storage manager
typedef struct EsfParameterStorageManagerUseFlashRaw {
  uint32_t size;  // size
  uint8_t data;   // data
} EsfParameterStorageManagerUseFlashRaw;

typedef struct EsfLogManagerParamsForPsm {
  char dlog_level[2];                                        // dlog level
  char elog_level[2];                                        // elog level
  char dlog_dest[2];                                         // dlog dest
  EsfParameterStorageManagerDlogFilterRaw dlog_filter;       // dlog filter
  EsfParameterStorageManagerUseFlashRaw use_flash;           // use flash
  char storage_name[ESF_LOG_MANAGER_STORAGE_NAME_MAX_SIZE];  // storage name
  char storage_path[ESF_LOG_MANAGER_STORAGE_SUB_DIR_PATH_MAX_SIZE];  // storage
                                                                     // name
} EsfLogManagerParamsForPsm;

typedef struct EsfLogManagerParameterForPsmMask {
  uint8_t dlog_level : 1;    // The type of input/output settings to
                             // DeviceSetting is DlogLevel
  uint8_t elog_level : 1;    // The type of input/output settings to
                             // DeviceSetting is ElogLevel
  uint8_t dlog_dest : 1;     // The type of input/output settings to
                             // DeviceSetting is Dlog
  uint8_t dlog_filter : 1;   // The type of input/output settings to
                             // DeviceSetting is LogFilter
  uint8_t use_flash : 1;     // The type of input/output settings to
                             // DeviceSetting is flash
  uint8_t storage_name : 1;  // The type of input/output settings to
                             // DeviceSetting is flash
  uint8_t storage_path : 1;  // The type of input/output settings to
                             // DeviceSetting is flash
} EsfLogManagerParameterForPsmMask;

// """Notify log setting information to paramater storage manager and save it to
// flash Args:
//    *mask(EsfLogManagerParameterMask): Mask value for whether to save to
//    *value(EsfLogManagerParameterValue): Log setting information to be saved
//    in flash
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination
//    kEsfLogManagerStatusParamError: invalid parameter
// """
EsfLogManagerStatus EsfLogManagerSaveParamsForPsm(
    EsfLogManagerSettingBlockType const block_type,
    EsfLogManagerParameterMask const *mask,
    EsfLogManagerParameterValue const *value);

// """Gets the log parameter information saved in Flash.
// Args:
//    mask(EsfLogManagerParameterMask): Parameter mask structure to be
//    value(EsfLogManagerParameterValue): Parameter information structure to
//    obtained
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusParamError: invalid parameter
EsfLogManagerStatus EsfLogManagerLoadParamsForPsm(
    EsfLogManagerSettingBlockType const block_type,
    EsfLogManagerParameterMask *mask, EsfLogManagerParameterValue *value);

#endif  // ESF_LOG_MANAGER_SETTING_H_
