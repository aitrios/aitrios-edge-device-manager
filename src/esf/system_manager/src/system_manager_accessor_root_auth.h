/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// This header file defines structures and functions related to the Root Auth
// data management in the parameter storage manager.

#ifndef ESF_SYSTEM_MANAGER_ACCESSOR_ROOT_AUTH_H_
#define ESF_SYSTEM_MANAGER_ACCESSOR_ROOT_AUTH_H_

#include "parameter_storage_manager_common.h"
#include "system_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

// Structure that stores the Root Auth data.
typedef struct EsfParameterStorageManagerRootAuth {
  EsfParameterStorageManagerOffsetBinary root_ca;               // Root CA data.
  char root_ca_hash[ESF_SYSTEM_MANAGER_ROOT_CA_HASH_MAX_SIZE];  // Root CA Hash
                                                                // data.
} EsfParameterStorageManagerRootAuth;

// Structure that stores the mask of the Root Auth data.
typedef struct EsfParameterStorageManagerRootAuthMask {
  uint8_t root_ca : 1;       // Root CA mask.
  uint8_t root_ca_hash : 1;  // Root CA Hash mask.
} EsfParameterStorageManagerRootAuthMask;

// """Loads the Root Auth from the parameter storage manager (PSM).
// This function loads the Root Auth from the specified
// parameter storage manager (PSM) handle. It first checks if the provided
// mask and data pointers are not NULL, then proceeds to load the data
// using the EsfParameterStorageManagerLoad function. If the value is not
// set in the storage, it returns a default value.
// Args:
//   [IN] handle (EsfParameterStorageManagerHandle): The handle to the parameter
//     storage manager from which the data will be loaded.
//   [IN] mask (const EsfParameterStorageManagerRootAuthMask*): A pointer to
//     the mask that specifies which fields are being loaded.
//   [OUT] data (EsfParameterStorageManagerRootAuth*): A pointer to the
//     data structure where the loaded Root Auth will be stored.
// Returns:
//   kEsfSystemManagerResultOk: if the operation is successful.
//   kEsfSystemManagerResultParamError: if either the mask or data pointer is
//     NULL.
//   kEsfSystemManagerResultInternalError: if the internal load operation
//     fails.
//   kEsfSystemManagerResultEmptyData: Root CA data is empty.
// """
EsfSystemManagerResult EsfSystemManagerLoadRootAuthFromPsm(
    EsfParameterStorageManagerHandle handle,
    const EsfParameterStorageManagerRootAuthMask* mask,
    EsfParameterStorageManagerRootAuth* data);

// """Saves the Root Auth to the parameter storage manager (PSM).
// This function saves the Root Auth into the specified
// parameter storage manager (PSM) handle. It first checks if the provided
// mask and data pointers are not NULL, then proceeds to save the data
// using the EsfParameterStorageManagerSave function. Finally, it returns
// the result of the save operation.
// Args:
//   [IN] handle (EsfParameterStorageManagerHandle): The handle to the
//     parameter storage manager where the data will be saved.
//   [IN] mask (const EsfParameterStorageManagerRootAuthMask*): A pointer to
//     the mask that specifies which fields are being updated.
//   [IN] data (const EsfParameterStorageManagerRootAuth*): A pointer to the
//     data containing the Root Auth to be saved.
// Returns:
//   kEsfSystemManagerResultOk: if the operation is successful.
//   kEsfSystemManagerResultParamError: if either the mask or data pointer is
//     NULL.
//   kEsfSystemManagerResultInternalError: if the internal save operation fails.
// """
EsfSystemManagerResult EsfSystemManagerSaveRootAuthToPsm(
    EsfParameterStorageManagerHandle handle,
    const EsfParameterStorageManagerRootAuthMask* mask,
    const EsfParameterStorageManagerRootAuth* data);

#ifdef __cplusplus
}
#endif

#endif  // ESF_SYSTEM_MANAGER_ACCESSOR_ROOT_AUTH_H_
