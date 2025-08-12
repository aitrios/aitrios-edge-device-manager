/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// This header file defines structures and functions related to the HW info
// data management in the parameter storage manager.

#ifndef ESF_SYSTEM_MANAGER_ACCESSOR_EXCEPTION_INFO_H_
#define ESF_SYSTEM_MANAGER_ACCESSOR_EXCEPTION_INFO_H_

#include "parameter_storage_manager_common.h"
#include "pl_system_control.h"
#include "system_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

// Structure that stores the Exception Info data.
typedef struct EsfParameterStorageManagerExceptionInfo {
  EsfParameterStorageManagerBinary exception_info;
} EsfParameterStorageManagerExceptionInfo;

// Structure that stores the mask of the Exception Info data.
typedef struct EsfParameterStorageManagerExceptionInfoMask {
  uint8_t exception_info : 1;  // Exception Info mask.
} EsfParameterStorageManagerExceptionInfoMask;

// """Saves the ExceptionInfo to the parameter storage manager (PSM).
// This function saves the ExceptionInfo into the specified
// parameter storage manager (PSM) handle. It first checks if the provided
// mask and data pointers are not NULL, then proceeds to save the data
// using the EsfParameterStorageManagerSave function. Finally, it returns
// the result of the save operation.
// Args:
//   [IN] handle (EsfParameterStorageManagerHandle): The handle to the
//     parameter storage manager where the data will be saved.
//   [IN] mask (const EsfParameterStorageManagerExceptionInfoMask*): A
//   pointer to
//     the mask that specifies which fields are being updated.
//   [IN] data (const EsfParameterStorageManagerExceptionInfo*): A pointer
//   to the
//     data containing the ExceptionInfo to be saved.
// Returns:
//   kEsfSystemManagerResultOk: if the operation is successful.
//   kEsfSystemManagerResultParamError: if either the mask or data pointer is
//     NULL.
//   kEsfSystemManagerResultInternalError: if the internal save operation fails.
// """
EsfSystemManagerResult EsfSystemManagerSaveExceptionInfoToPsm(
    EsfParameterStorageManagerHandle handle,
    const EsfParameterStorageManagerExceptionInfoMask *mask,
    const EsfParameterStorageManagerExceptionInfo *data);

// """Loads the ExceptionInfo from the parameter storage manager (PSM).
// This function loads the ExceptionInfo into the specified
// parameter storage manager (PSM) handle. It first checks if the provided
// mask and data pointers are not NULL, then proceeds to load the data
// using the EsfParameterStorageManagerLoad function. Finally, it returns
// the result of the load operation.
// Args:
//   [IN] handle (EsfParameterStorageManagerHandle): The handle to the
//     parameter storage manager from where the data will be loaded.
//   [IN] mask (const EsfParameterStorageManagerExceptionInfoMask*): A pointer
//   to
//     the mask that specifies which fields are being loaded.
//   [OUT] data (EsfParameterStorageManagerExceptionInfo*): A pointer to the
//     data structure where the ExceptionInfo will be loaded.
// Returns:
//   kEsfSystemManagerResultOk: if the operation is successful.
//   kEsfSystemManagerResultParamError: if either the mask or data pointer is
//     NULL.
//   kEsfSystemManagerResultInternalError: if the internal load operation fails.
//   kEsfSystemManagerResultEmptyData: if the loaded data is empty.
// """
EsfSystemManagerResult EsfSystemManagerLoadExceptionInfoFromPsm(
    EsfParameterStorageManagerHandle handle,
    const EsfParameterStorageManagerExceptionInfoMask *mask,
    EsfParameterStorageManagerExceptionInfo *data);

EsfSystemManagerResult EsfSystemManagerClearExceptionInfoOfPsm(
    EsfParameterStorageManagerHandle handle,
    const EsfParameterStorageManagerExceptionInfoMask *mask);

#ifdef __cplusplus
}
#endif

#endif  // ESF_SYSTEM_MANAGER_ACCESSOR_EXCEPTION_INFO_H_
