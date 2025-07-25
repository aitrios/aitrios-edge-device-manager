/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// This header file defines structures and functions related to the Initial
// Setting Flag data management in the parameter storage manager.

#ifndef ESF_SYSTEM_MANAGER_ACCESSOR_INITIAL_SETTING_FLAG_H_
#define ESF_SYSTEM_MANAGER_ACCESSOR_INITIAL_SETTING_FLAG_H_

#include "parameter_storage_manager_common.h"
#include "system_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

// Structure that stores the Initial Setting Flag raw data.
typedef struct EsfParameterStorageManagerInitialSettingFlagRaw {
  uint32_t size;  // Size.
  uint8_t data;   // Data.
} EsfParameterStorageManagerInitialSettingFlagRaw;

// Structure that stores the Initial Setting Flag data.
typedef struct EsfParameterStorageManagerInitialSettingFlag {
  EsfParameterStorageManagerInitialSettingFlagRaw
      initial_setting_flag;  // Initial Setting Flag data.
} EsfParameterStorageManagerInitialSettingFlag;

// Structure that stores the mask of the Initial Setting Flag data.
typedef struct EsfParameterStorageManagerInitialSettingFlagMask {
  uint8_t initial_setting_flag : 1;  // Initial Setting Flag mask.
} EsfParameterStorageManagerInitialSettingFlagMask;

// """Saves the Initial Setting Flag to the parameter storage manager (PSM).
// This function saves the Initial Setting Flag into the specified
// parameter storage manager (PSM) handle. It first checks if the provided
// mask and data pointers are not NULL, then proceeds to save the data
// using the EsfParameterStorageManagerSave function. Finally, it returns
// the result of the save operation.
// Args:
//   [IN] handle (EsfParameterStorageManagerHandle): The handle to the
//     parameter storage manager where the data will be saved.
//   [IN] mask (const EsfParameterStorageManagerInitialSettingFlagMask*): A
//   pointer to
//     the mask that specifies which fields are being updated.
//   [IN] data (const EsfParameterStorageManagerInitialSettingFlag*): A pointer
//   to the
//     data containing the Initial Setting Flag to be saved.
// Returns:
//   kEsfSystemManagerResultOk: if the operation is successful.
//   kEsfSystemManagerResultParamError: if either the mask or data pointer is
//     NULL.
//   kEsfSystemManagerResultInternalError: if the internal save operation fails.
// """
EsfSystemManagerResult EsfSystemManagerSaveInitialSettingFlagToPsm(
    EsfParameterStorageManagerHandle handle,
    const EsfParameterStorageManagerInitialSettingFlagMask* mask,
    const EsfParameterStorageManagerInitialSettingFlag* data);

// """Loads the Initial Setting Flag from the parameter storage manager (PSM).
// This function loads the Initial Setting Flag from the specified
// parameter storage manager (PSM) handle. It first checks if the provided
// mask and data pointers are not NULL, then proceeds to load the data
// using the EsfParameterStorageManagerLoad function. If the value is not
// set in the storage, it returns a default value.
// Args:
//   [IN] handle (EsfParameterStorageManagerHandle): The handle to the parameter
//     storage manager from which the data will be loaded.
//   [IN] mask (const EsfParameterStorageManagerInitialSettingFlagMask*): A
//   pointer to
//     the mask that specifies which fields are being loaded.
//   [OUT] data (EsfParameterStorageManagerInitialSettingFlag*): A pointer to
//     the data structure where the loaded Initial Setting Flag will be stored.
// Returns:
//   kEsfSystemManagerResultOk: if the operation is successful.
//   kEsfSystemManagerResultParamError: if either the mask or data pointer is
//     NULL.
//   kEsfSystemManagerResultInternalError: if the internal load operation
//     fails.
// """
EsfSystemManagerResult EsfSystemManagerLoadInitialSettingFlagFromPsm(
    EsfParameterStorageManagerHandle handle,
    const EsfParameterStorageManagerInitialSettingFlagMask* mask,
    EsfParameterStorageManagerInitialSettingFlag* data);

#ifdef __cplusplus
}
#endif

#endif  // ESF_SYSTEM_MANAGER_ACCESSOR_INITIAL_SETTING_FLAG_H_
