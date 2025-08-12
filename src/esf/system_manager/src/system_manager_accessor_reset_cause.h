/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// This header file defines structures and functions related to the HW info
// data management in the parameter storage manager.

#ifndef ESF_SYSTEM_MANAGER_ACCESSOR_RESET_CAUSE_H_
#define ESF_SYSTEM_MANAGER_ACCESSOR_RESET_CAUSE_H_

#include "parameter_storage_manager_common.h"
#include "pl_system_manager.h"
#include "power_manager.h"
#include "system_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ESF_SYSTEM_MANAGER_EVP_RESET_CAUSE_MAX_SIZE (3)
#define ESF_SYSTEM_MANAGER_RESET_CAUSE_MAX_SIZE (3)

// Structure that stores the Reset Cause data.
typedef struct EsfParameterStorageManagerResetCause {
  char evp_reset_cause[ESF_SYSTEM_MANAGER_EVP_RESET_CAUSE_MAX_SIZE];
  char reset_cause[ESF_SYSTEM_MANAGER_RESET_CAUSE_MAX_SIZE];
} EsfParameterStorageManagerResetCause;

// Structure that stores the mask of the Reset Cause data.
typedef struct EsfParameterStorageManagerResetCauseMask {
  uint8_t evp_reset_cause : 1;  // Evp Reset Cause mask.
  uint8_t reset_cause : 1;      // Reset Cause mask.
} EsfParameterStorageManagerResetCauseMask;

typedef enum {
  kEsfSystemManagerResetCauseTypeNone,  // Reset Cause type for None
  kEsfSystemManagerResetCauseTypeEvp,   // Reset Cause type for EVP
  kEsfSystemManagerResetCauseTypePm     // Reset Cause type for Power Manager
} EsfSystemManagerResetCauseType;

// """Saves the ResetCause to the parameter storage manager (PSM).
// This function saves the ResetCause into the specified
// parameter storage manager (PSM) handle. It first checks if the provided
// mask and data pointers are not NULL, then proceeds to save the data
// using the EsfParameterStorageManagerSave function. Finally, it returns
// the result of the save operation.
// Args:
//   [IN] handle (EsfParameterStorageManagerHandle): The handle to the
//     parameter storage manager where the data will be saved.
//   [IN] mask (const EsfParameterStorageManagerResetCauseMask*): A
//   pointer to
//     the mask that specifies which fields are being updated.
//   [IN] data (const EsfParameterStorageManagerResetCause*): A pointer
//   to the
//     data containing the ResetCause to be saved.
// Returns:
//   kEsfSystemManagerResultOk: if the operation is successful.
//   kEsfSystemManagerResultParamError: if either the mask or data pointer is
//     NULL.
//   kEsfSystemManagerResultInternalError: if the internal save operation fails.
// """
EsfSystemManagerResult EsfSystemManagerSaveResetCauseToPsm(
    EsfParameterStorageManagerHandle handle,
    const EsfParameterStorageManagerResetCauseMask *mask,
    const EsfParameterStorageManagerResetCause *data);

// """Loads the ResetCause from the parameter storage manager (PSM).
// This function loads the ResetCause into the specified
// parameter storage manager (PSM) handle. It first checks if the provided
// mask and data pointers are not NULL, then proceeds to load the data
// using the EsfParameterStorageManagerLoad function. Finally, it returns
// the result of the load operation.
// Args:
//   [IN] handle (EsfParameterStorageManagerHandle): The handle to the
//     parameter storage manager from where the data will be loaded.
//   [IN] mask (const EsfParameterStorageManagerResetCauseMask*): A pointer to
//     the mask that specifies which fields are being loaded.
//   [OUT] data (EsfParameterStorageManagerResetCause*): A pointer to the
//     data structure where the ResetCause will be loaded.
// Returns:
//   kEsfSystemManagerResultOk: if the operation is successful.
//   kEsfSystemManagerResultParamError: if either the mask or data pointer is
//     NULL.
//   kEsfSystemManagerResultInternalError: if the internal load operation fails.
//   kEsfSystemManagerResultEmptyData: if the loaded data is empty.
// """
EsfSystemManagerResult EsfSystemManagerLoadResetCauseFromPsm(
    EsfParameterStorageManagerHandle handle,
    const EsfParameterStorageManagerResetCauseMask *mask,
    EsfParameterStorageManagerResetCause *data);

EsfSystemManagerResult EsfSystemManagerSelectResetCause(
    EsfSystemManagerResetCauseType *selected_reset_cause_type);

EsfSystemManagerResult EsfSystemManagerGetResetCauseEventId(
    const char *reset_cause_str, EsfSystemManagerResetCauseType cause_type,
    uint8_t *event_id);

EsfSystemManagerResult EsfSystemManagerConvertStringToEvpResetCause(
    const char *reset_cause_str,
    EsfSystemManagerEvpResetCause *evp_reset_cause);

EsfSystemManagerResult EsfSystemManagerConvertEvpResetCauseToString(
    EsfSystemManagerEvpResetCause evp_reset_cause, char *reset_cause_str,
    size_t reset_cause_str_size);

EsfSystemManagerResult EsfSystemManagerConvertStringToResetCause(
    const char *reset_cause_str, EsfSystemManagerResetCause *reset_cause);

EsfSystemManagerResult EsfSystemManagerConvertResetCauseToString(
    EsfSystemManagerResetCause reset_cause, char *reset_cause_str,
    size_t reset_cause_str_size);

EsfSystemManagerResetCause EsfSystemManagerConvertResetCausePmToSm(
    EsfPwrMgrResetCause reset_cause);

PlSystemManagerResetCause EsfSystemManagerConvertResetCauseSmToPlSm(
    EsfSystemManagerResetCause reset_cause);
#ifdef __cplusplus
}
#endif

#endif  // ESF_SYSTEM_MANAGER_ACCESSOR_RESET_CAUSE_H_
