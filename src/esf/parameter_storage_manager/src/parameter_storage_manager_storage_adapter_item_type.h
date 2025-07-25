/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef ESF_PARAMETER_STORAGE_MANAGER_SRC_PARAMETER_STORAGE_MANAGER_STORAGE_ADAPTER_ITEM_TYPE_H_  // NOLINT
#define ESF_PARAMETER_STORAGE_MANAGER_SRC_PARAMETER_STORAGE_MANAGER_STORAGE_ADAPTER_ITEM_TYPE_H_  // NOLINT

#include "parameter_storage_manager.h"

// """Refers to the data type and save the data to the data storage area.

// Refers to the data type and save the data to the data storage area.
// Performs data type specific processing and determination.

// Args:
//     [IN] type (EsfParameterStorageManagerItemID): Data type.
//     [IN] item (const void*): Data to be saved.
//     [IN/OUT] private_data (void*): working information.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInternal: The argument info is an
//     invalid value. kEsfParameterStorageManagerStatusInternal: Internal
//     processing failed. kEsfParameterStorageManagerStatusDataLoss: Unable to
//     access data storage area. kEsfParameterStorageManagerStatusUnavailable:
//     Failed to save data.

// Note:
// """
EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterSaveItemType(
    EsfParameterStorageManagerItemType type, const void* item,
    void* private_data);

// """Refers to the data type and load the data from the data storage area.

// Refers to the data type and load the data from the data storage area.
// Performs data type specific processing and determination.

// Args:
//     [IN] type (EsfParameterStorageManagerItemID): Data type.
//     [OUT] item (void*): Data to be saved.
//     [IN/OUT] private_data (void*): working information.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInternal: The argument info is an
//     invalid value. kEsfParameterStorageManagerStatusInternal: Internal
//     processing failed. kEsfParameterStorageManagerStatusDataLoss: Unable to
//     access data storage area. kEsfParameterStorageManagerStatusUnavailable:
//     Failed to save data.

// Note:
// """
EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterLoadItemType(
    EsfParameterStorageManagerItemType type, void* item, void* private_data);

#endif  // ESF_PARAMETER_STORAGE_MANAGER_SRC_PARAMETER_STORAGE_MANAGER_STORAGE_ADAPTER_ITEM_TYPE_H_  // NOLINT
