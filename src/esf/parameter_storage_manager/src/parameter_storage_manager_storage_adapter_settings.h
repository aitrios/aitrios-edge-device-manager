/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef ESF_PARAMETER_STORAGE_MANAGER_SRC_PARAMETER_STORAGE_MANAGER_STORAGE_ADAPTER_SETTINGS_H_  // NOLINT
#define ESF_PARAMETER_STORAGE_MANAGER_SRC_PARAMETER_STORAGE_MANAGER_STORAGE_ADAPTER_SETTINGS_H_  // NOLINT

#include "parameter_storage_manager.h"
#include "parameter_storage_manager/src/parameter_storage_manager_config.h"

// The following files must be included after
// parameter_storage_manager_config.h:
#include ESF_PARAMETER_STORAGE_MANAGER_PL_STORAGE_FILE

typedef enum EsfParameterStorageManagerStorageID {
  kEsfParameterStorageManagerStoragePl,
  kEsfParameterStorageManagerStorageOther,
  kEsfParameterStorageManagerStorageMax,
} EsfParameterStorageManagerStorageID;

typedef enum EsfParameterStorageManagerStorageOtherDataID {
  kEsfParameterStorageManagerStorageOtherDataMax,
} EsfParameterStorageManagerStorageOtherDataID;

// """Convert EsfParameterStorageManagerItemID to type ID of data storage area.

// Convert EsfParameterStorageManagerItemID to type ID of data storage area.

// Args:
//     [IN] id (EsfParameterStorageManagerItemID): An enumeration type that
//     defines the
//                                       data supported by Parameter Storage
//                                       Manager.

// Returns:
//     EsfParameterStorageManagerStorageID: Returns the value of one of the
//     EsfParameterStorageManagerStorageIDs corresponding to the argument
//     EsfParameterStorageManagerItemID. If there is no corresponding data,
//     kEsfParameterStorageManagerStorageMax is returned.
//

// Note:
// """
EsfParameterStorageManagerStorageID
EsfParameterStorageManagerStorageAdapterConvertItemIDToStorageID(
    EsfParameterStorageManagerItemID id);

// """Convert EsfParameterStorageManagerItemID to PL Storage data ID.

// Convert EsfParameterStorageManagerItemID to PL Storage data ID.

// Args:
//     [IN] id (EsfParameterStorageManagerItemID): An enumeration type that
//     defines the
//                                       data supported by Parameter Storage
//                                       Manager.

// Returns:
//     EsfParameterStorageManagerStorageID: Returns one of the PlStorageDataId
//     values corresponding to the argument EsfParameterStorageManagerItemID. If
//     there is no corresponding data, PlStorageDataMax is returned.
//

// Note:
// """
PlStorageDataId
EsfParameterStorageManagerStorageAdapterConvertItemIDToPLStorageID(
    EsfParameterStorageManagerItemID id);

// """Convert EsfParameterStorageManagerItemID to data ID of other data storage
// area.

// Convert EsfParameterStorageManagerItemID to data ID of other data storage
// area.

// Args:
//     [IN] id (EsfParameterStorageManagerItemID): An enumeration type that
//     defines the
//                                       data supported by Parameter Storage
//                                       Manager.

// Returns:
//     EsfParameterStorageManagerStorageOtherDataID: Returns the value of one of
//     the EsfParameterStorageManagerStorageOtherDataIDs corresponding to the
//     argument EsfParameterStorageManagerItemID. If there is no corresponding
//     data, returns kEsfParameterStorageManagerStorageOtherDataMax.
//

// Note:
// """
EsfParameterStorageManagerStorageOtherDataID
EsfParameterStorageManagerStorageAdapterConvertItemIDToOtherDataID(
    EsfParameterStorageManagerItemID id);

// """Convert EsfParameterStorageManagerItemID to Factory Reset required flag.

// Convert EsfParameterStorageManagerItemID to Factory Reset required flag.

// Args:
//     [IN] id (EsfParameterStorageManagerItemID): An enumeration type that
//     defines the
//                                       data supported by Parameter Storage
//                                       Manager.

// Returns:
//     bool: If true, a factory reset is required.
//           If false, a factory reset is not required.

// Note:
// """
bool EsfParameterStorageManagerStorageAdapterConvertItemIDToFactoryResetRequired(  // NOLINT
    EsfParameterStorageManagerItemID id);

#endif  // ESF_PARAMETER_STORAGE_MANAGER_SRC_PARAMETER_STORAGE_MANAGER_STORAGE_ADAPTER_SETTINGS_H_  // NOLINT
