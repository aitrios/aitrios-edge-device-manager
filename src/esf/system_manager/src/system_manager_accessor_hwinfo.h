/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// This header file defines structures and functions related to the HW info
// data management in the parameter storage manager.

#ifndef ESF_SYSTEM_MANAGER_ACCESSOR_HWINFO_H_
#define ESF_SYSTEM_MANAGER_ACCESSOR_HWINFO_H_

#include "parameter_storage_manager_common.h"
#include "system_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct EsfParameterStorageManagerHwInfo {
  EsfParameterStorageManagerBinary hw_info;
} EsfParameterStorageManagerHwInfo;

// Structure that stores the mask of the HW Info data.
typedef struct EsfParameterStorageManagerHwInfoMask {
  uint8_t hw_info : 1;  // HW Info mask.
} EsfParameterStorageManagerHwInfoMask;

// """Loads the HW Info from the parameter storage manager (PSM).
// This function loads the HW Info into the specified
// parameter storage manager (PSM) handle. It first checks if the provided
// mask and data pointers are not NULL, then proceeds to load the data
// using the EsfParameterStorageManagerLoad function. Finally, it returns
// the result of the load operation.
// Args:
//   [IN] handle (EsfParameterStorageManagerHandle): The handle to the
//     parameter storage manager from where the data will be loaded.
//   [IN] mask (const EsfParameterStorageManagerHwInfoMask*): A pointer to
//     the mask that specifies which fields are being loaded.
//   [OUT] data (EsfParameterStorageManagerHwInfo*): A pointer to the
//     data structure where the HW Info will be loaded.
// Returns:
//   kEsfSystemManagerResultOk: if the operation is successful.
//   kEsfSystemManagerResultParamError: if either the mask or data pointer is
//     NULL.
//   kEsfSystemManagerResultInternalError: if the internal load operation fails.
//   kEsfSystemManagerResultEmptyData: if the loaded data is empty.
// """
EsfSystemManagerResult EsfSystemManagerLoadHwInfoFromPsm(
    EsfParameterStorageManagerHandle handle,
    const EsfParameterStorageManagerHwInfoMask *mask,
    EsfParameterStorageManagerHwInfo *data);

EsfSystemManagerResult EsfSystemManagerParseHwInfo(
    EsfSystemManagerHwInfo *data,
    EsfParameterStorageManagerHwInfo *data_struct);

#ifdef __cplusplus
}
#endif

#endif  // ESF_SYSTEM_MANAGER_ACCESSOR_HWINFO_H_
