/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// This header file defines structures and functions related to the Enrollment
// data management in the parameter storage manager.

#ifndef ESF_SYSTEM_MANAGER_ACCESSOR_ENROLLMENT_H_
#define ESF_SYSTEM_MANAGER_ACCESSOR_ENROLLMENT_H_

#include "parameter_storage_manager_common.h"
#include "system_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

// Structure that stores the Enrollment data.
typedef struct EsfParameterStorageManagerEnrollment {
  char dps_url[ESF_SYSTEM_MANAGER_DPS_URL_MAX_SIZE];            // DPS URL data.
  char common_name[ESF_SYSTEM_MANAGER_COMMON_NAME_MAX_SIZE];    // Common
                                                                // Name
                                                                // data.
  char dps_scope_id[ESF_SYSTEM_MANAGER_DPS_SCOPE_ID_MAX_SIZE];  // DPS Scope ID
                                                                // data.
  char project_id[ESF_SYSTEM_MANAGER_PROJECT_ID_MAX_SIZE];  // Project ID data.
  char register_token
      [ESF_SYSTEM_MANAGER_REGISTER_TOKEN_MAX_SIZE];  // Register Token data.
} EsfParameterStorageManagerEnrollment;

// Structure that stores the mask of the Enrollment data.
typedef struct EsfParameterStorageManagerEnrollmentMask {
  uint8_t dps_url : 1;         // DPS URL mask.
  uint8_t common_name : 1;     // Common Name mask.
  uint8_t dps_scope_id : 1;    // DPS Scope ID mask.
  uint8_t project_id : 1;      // Project ID mask.
  uint8_t register_token : 1;  // Register Token mask.
} EsfParameterStorageManagerEnrollmentMask;

// """Saves the Enrollment to the parameter storage manager (PSM).
// This function saves the Enrollment into the specified
// parameter storage manager (PSM) handle. It first checks if the provided
// mask and data pointers are not NULL, then proceeds to save the data
// using the EsfParameterStorageManagerSave function. Finally, it returns
// the result of the save operation.
// Args:
//   [IN] handle (EsfParameterStorageManagerHandle): The handle to the
//     parameter storage manager where the data will be saved.
//   [IN] mask (const EsfParameterStorageManagerEnrollmentMask*): A pointer to
//     the mask that specifies which fields are being updated.
//   [IN] data (const EsfParameterStorageManagerEnrollment*): A pointer to the
//     data containing the Enrollment to be saved.
// Returns:
//   kEsfSystemManagerResultOk: if the operation is successful.
//   kEsfSystemManagerResultParamError: if either the mask or data pointer is
//     NULL.
//   kEsfSystemManagerResultInternalError: if the internal save operation fails.
// """
EsfSystemManagerResult EsfSystemManagerSaveEnrollmentToPsm(
    EsfParameterStorageManagerHandle handle,
    const EsfParameterStorageManagerEnrollmentMask* mask,
    const EsfParameterStorageManagerEnrollment* data);

// """Loads the Enrollment from the parameter storage manager (PSM).
// This function loads the Enrollment from the specified
// parameter storage manager (PSM) handle. It first checks if the provided
// mask and data pointers are not NULL, then proceeds to load the data
// using the EsfParameterStorageManagerLoad function. If the value is not
// set in the storage, it returns a default value.
// Args:
//   [IN] handle (EsfParameterStorageManagerHandle): The handle to the parameter
//     storage manager from which the data will be loaded.
//   [IN] mask (const EsfParameterStorageManagerEnrollmentMask*): A pointer to
//     the mask that specifies which fields are being loaded.
//   [OUT] data (EsfParameterStorageManagerEnrollment*): A pointer to the
//     data structure where the loaded Enrollment will be stored.
// Returns:
//   kEsfSystemManagerResultOk: if the operation is successful.
//   kEsfSystemManagerResultParamError: if either the mask or data pointer is
//     NULL.
//   kEsfSystemManagerResultInternalError: if the internal load operation
//     fails.
// """
EsfSystemManagerResult EsfSystemManagerLoadEnrollmentFromPsm(
    EsfParameterStorageManagerHandle handle,
    const EsfParameterStorageManagerEnrollmentMask* mask,
    EsfParameterStorageManagerEnrollment* data);

#ifdef __cplusplus
}
#endif

#endif  // ESF_SYSTEM_MANAGER_ACCESSOR_ENROLLMENT_H_
