/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// This header file defines structures and functions related to the EVP
// data management in the parameter storage manager.

#ifndef ESF_SYSTEM_MANAGER_ACCESSOR_EVP_H_
#define ESF_SYSTEM_MANAGER_ACCESSOR_EVP_H_

#include "parameter_storage_manager_common.h"
#include "system_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ESF_SYSTEM_MANAGER_EVP_TLS_MAX_SIZE (2)

// Structure that stores the EVP data.
typedef struct EsfParameterStorageManagerEvp {
  char evp_url[ESF_SYSTEM_MANAGER_EVP_HUB_URL_MAX_SIZE];  // EVP Hub URL data.
  char
      evp_port[ESF_SYSTEM_MANAGER_EVP_HUB_PORT_MAX_SIZE];  // EVP Hub Port data.
  char iot_platform[ESF_SYSTEM_MANAGER_IOT_PLATFORM_MAX_SIZE];  // IoT Platform
                                                                // data.
  char evp_tls[ESF_SYSTEM_MANAGER_EVP_TLS_MAX_SIZE];            // EVP TLS data.
} EsfParameterStorageManagerEvp;

// Structure that stores the mask of the EVP data.
typedef struct EsfParameterStorageManagerEvpMask {
  uint8_t evp_url : 1;       // EVP Hub URL mask.
  uint8_t evp_port : 1;      // EVP Hub Port mask.
  uint8_t iot_platform : 1;  // IoT Platform mask.
  uint8_t evp_tls : 1;       // EVP TLS mask.
} EsfParameterStorageManagerEvpMask;

// """Saves the EVP to the parameter storage manager (PSM).
// This function saves the EVP into the specified
// parameter storage manager (PSM) handle. It first checks if the provided
// mask and data pointers are not NULL, then proceeds to save the data
// using the EsfParameterStorageManagerSave function. Finally, it returns
// the result of the save operation.
// Args:
//   [IN] handle (EsfParameterStorageManagerHandle): The handle to the
//     parameter storage manager where the data will be saved.
//   [IN] mask (const EsfParameterStorageManagerEvpMask*): A pointer to
//     the mask that specifies which fields are being updated.
//   [IN] data (const EsfParameterStorageManagerEvp*): A pointer to the
//     data containing the EVP to be saved.
// Returns:
//   kEsfSystemManagerResultOk: if the operation is successful.
//   kEsfSystemManagerResultParamError: if either the mask or data pointer is
//     NULL.
//   kEsfSystemManagerResultInternalError: if the internal save operation fails.
// """
EsfSystemManagerResult EsfSystemManagerSaveEvpToPsm(
    EsfParameterStorageManagerHandle handle,
    const EsfParameterStorageManagerEvpMask* mask,
    const EsfParameterStorageManagerEvp* data);

// """Loads the EVP from the parameter storage manager (PSM).
// This function loads the EVP from the specified
// parameter storage manager (PSM) handle. It first checks if the provided
// mask and data pointers are not NULL, then proceeds to load the data
// using the EsfParameterStorageManagerLoad function. If the value is not
// set in the storage, it returns a default value.
// Args:
//   [IN] handle (EsfParameterStorageManagerHandle): The handle to the parameter
//     storage manager from which the data will be loaded.
//   [IN] mask (const EsfParameterStorageManagerEvpMask*): A pointer to
//     the mask that specifies which fields are being loaded.
//   [OUT] data (EsfParameterStorageManagerEvp*): A pointer to the
//     data structure where the loaded EVP will be stored.
// Returns:
//   kEsfSystemManagerResultOk: if the operation is successful.
//   kEsfSystemManagerResultParamError: if either the mask or data pointer is
//     NULL.
//   kEsfSystemManagerResultInternalError: if the internal load operation
//     fails.
// """
EsfSystemManagerResult EsfSystemManagerLoadEvpFromPsm(
    EsfParameterStorageManagerHandle handle,
    const EsfParameterStorageManagerEvpMask* mask,
    EsfParameterStorageManagerEvp* data);

#ifdef __cplusplus
}
#endif

#endif  // ESF_SYSTEM_MANAGER_ACCESSOR_EVP_H_
