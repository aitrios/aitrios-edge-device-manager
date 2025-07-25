/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// This header file defines structures and functions related to theQR Mode
// Timeout Value data management in the parameter storage manager.

#ifndef ESF_SYSTEM_MANAGER_ACCESSOR_QR_MODE_TIMEOUT_VALUE_H_
#define ESF_SYSTEM_MANAGER_ACCESSOR_QR_MODE_TIMEOUT_VALUE_H_

#include "parameter_storage_manager_common.h"
#include "system_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

// Structure that stores the QR Mode Timeout Value raw data.
typedef struct EsfParameterStorageManagerQrModeTimeoutValueRaw {
  uint32_t size;  // Size.
  int32_t data;   // Data.
} EsfParameterStorageManagerQrModeTimeoutValueRaw;

// Structure that stores the QR Mode Timeout Value data.
typedef struct EsfParameterStorageManagerQrModeTimeoutValue {
  EsfParameterStorageManagerQrModeTimeoutValueRaw
      qr_mode_timeout_value;  // QR Mode Timeout Value data.
} EsfParameterStorageManagerQrModeTimeoutValue;

// Structure that stores the mask of the QR Mode Timeout Value data.
typedef struct EsfParameterStorageManagerQrModeTimeoutValueMask {
  uint8_t qr_mode_timeout_value : 1;  // QR Mode Timeout Value mask.
} EsfParameterStorageManagerQrModeTimeoutValueMask;

// """Saves the QR Mode Timeout Value to the parameter storage manager (PSM).
// This function saves the QR Mode Timeout Value into the specified
// parameter storage manager (PSM) handle. It first checks if the provided
// mask and data pointers are not NULL, then proceeds to save the data
// using the EsfParameterStorageManagerSave function. Finally, it returns
// the result of the save operation.
// Args:
//   [IN] handle (EsfParameterStorageManagerHandle): The handle to the
//     parameter storage manager where the data will be saved.
//   [IN] mask (const EsfParameterStorageManagerQrModeTimeoutValueMask*): A
//     pointer to the mask that specifies which fields are being updated.
//   [IN] data (const EsfParameterStorageManagerQrModeTimeoutValue*): A pointer
//     to the data containing the QR Mode Timeout Value to be saved.
// Returns:
//   kEsfSystemManagerResultOk: if the operation is successful.
//   kEsfSystemManagerResultParamError: if either the mask or data pointer is
//     NULL.
//   kEsfSystemManagerResultInternalError: if the internal save operation fails.
// """
EsfSystemManagerResult EsfSystemManagerSaveQrModeTimeoutValueToPsm(
    EsfParameterStorageManagerHandle handle,
    const EsfParameterStorageManagerQrModeTimeoutValueMask* mask,
    const EsfParameterStorageManagerQrModeTimeoutValue* data);

// """Loads the QR Mode Timeout Value from the parameter storage manager (PSM).
// This function loads the QR Mode Timeout Value from the specified
// parameter storage manager (PSM) handle. It first checks if the provided
// mask and data pointers are not NULL, then proceeds to load the data
// using the EsfParameterStorageManagerLoad function. If the value is not
// set in the storage, it returns a default value.
// Args:
//   [IN] handle (EsfParameterStorageManagerHandle): The handle to the parameter
//     storage manager from which the data will be loaded.
//   [IN] mask (const EsfParameterStorageManagerQrModeTimeoutValueMask*): A
//     pointer to the mask that specifies which fields are being loaded.
//   [OUT] data (EsfParameterStorageManagerQrModeTimeoutValue*): A pointer to
//     the data structure where the loaded QR Mode Timeout Value will be stored.
// Returns:
//   kEsfSystemManagerResultOk: if the operation is successful.
//   kEsfSystemManagerResultParamError: if either the mask or data pointer is
//     NULL.
//   kEsfSystemManagerResultInternalError: if the internal load operation
//     fails.
// """
EsfSystemManagerResult EsfSystemManagerLoadQrModeTimeoutValueFromPsm(
    EsfParameterStorageManagerHandle handle,
    const EsfParameterStorageManagerQrModeTimeoutValueMask* mask,
    EsfParameterStorageManagerQrModeTimeoutValue* data);

#ifdef __cplusplus
}
#endif

#endif  // ESF_SYSTEM_MANAGER_ACCESSOR_QR_MODE_TIMEOUT_VALUE_H_
