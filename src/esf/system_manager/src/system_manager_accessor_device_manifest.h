/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// This header file defines structures and functions related to the Device
// Manifest data management in the parameter storage manager.

#ifndef ESF_SYSTEM_MANAGER_ACCESSOR_DEVICE_MANIFEST_H_
#define ESF_SYSTEM_MANAGER_ACCESSOR_DEVICE_MANIFEST_H_

#include "parameter_storage_manager_common.h"
#include "system_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

// Structure that stores the Device Manifest data.
typedef struct EsfParameterStorageManagerDeviceManifest {
  EsfParameterStorageManagerOffsetBinary
      device_manifest;  // Device Manifest data.
} EsfParameterStorageManagerDeviceManifest;

// Structure that stores the mask of the  Device Manifest data.
typedef struct EsfParameterStorageManagerDeviceManifestMask {
  uint8_t device_manifest : 1;  // Device Manifest mask.
} EsfParameterStorageManagerDeviceManifestMask;

// """Loads the Device Manifest from the parameter storage manager (PSM).
// This function loads the Device Manifest from the specified
// parameter storage manager (PSM) handle. It first checks if the provided
// mask and data pointers are not NULL, then proceeds to load the data
// using the EsfParameterStorageManagerLoad function. If the value is not
// set in the storage, it returns a default value.
// Args:
//   [IN] handle (EsfParameterStorageManagerHandle): The handle to the parameter
//     storage manager from which the data will be loaded.
//   [IN] mask (const EsfParameterStorageManagerDeviceManifestMask*): A pointer
//     to the mask that specifies which fields are being loaded.
//   [OUT] data (EsfParameterStorageManagerDeviceManifest*): A pointer to the
//     data structure where the loaded Device Manifest will be stored.
// Returns:
//   kEsfSystemManagerResultOk: if the operation is successful.
//   kEsfSystemManagerResultParamError: if either the mask or data pointer is
//     NULL.
//   kEsfSystemManagerResultInternalError: if the internal load operation
//     fails.
//   kEsfSystemManagerResultEmptyData: Device Manifest data is empty.
// """
EsfSystemManagerResult EsfSystemManagerLoadDeviceManifestFromPsm(
    EsfParameterStorageManagerHandle handle,
    const EsfParameterStorageManagerDeviceManifestMask* mask,
    EsfParameterStorageManagerDeviceManifest* data);

#ifdef __cplusplus
}
#endif

#endif  // ESF_SYSTEM_MANAGER_ACCESSOR_DEVICE_MANIFEST_H_
