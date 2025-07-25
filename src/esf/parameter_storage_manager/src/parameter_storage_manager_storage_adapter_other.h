/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef ESF_PARAMETER_STORAGE_MANAGER_SRC_PARAMETER_STORAGE_MANAGER_STORAGE_ADAPTER_OTHER_H_  // NOLINT
#define ESF_PARAMETER_STORAGE_MANAGER_SRC_PARAMETER_STORAGE_MANAGER_STORAGE_ADAPTER_OTHER_H_  // NOLINT

#include "parameter_storage_manager.h"
#include "parameter_storage_manager/src/parameter_storage_manager_storage_adapter.h"  // NOLINT
#include "parameter_storage_manager/src/parameter_storage_manager_storage_adapter_settings.h"  // NOLINT

// """Saves data to other storage area.

// Saves data to other storage area.

// Args:
//     [IN] id (EsfParameterStorageManagerItemID): Data type.
//     [IN] offset (uint32_t): The location where data is stored.
//     [IN] size (uint32_t): Data size to be saved.
//     [IN] buf (const void*): Data to be saved.
//     [OUT] outsize (uint32_t*): Data size to be saved.
//     [IN/OUT] member (EsfParameterStorageManagerWorkMemberData*): working
//     information.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInternal: The argument is invalid.
//     kEsfParameterStorageManagerStatusInternal: Internal processing failed.
//     kEsfParameterStorageManagerStatusDataLoss: Unable to access data storage
//     area. kEsfParameterStorageManagerStatusUnavailable: Failed to save data.

// Note:
// """
EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterSaveOther(
    EsfParameterStorageManagerItemID id, uint32_t offset, uint32_t size,
    const void* buf, uint32_t* outsize,
    EsfParameterStorageManagerWorkMemberData* member);

// """Loads data from other storage area.

// Loads data from other storage area.

// Args:
//     [IN] id (EsfParameterStorageManagerItemID): Data type.
//     [IN] member (const EsfParameterStorageManagerWorkMemberData*): working
//     information. [IN] offset (uint32_t): The location where data is stored.
//     [IN] size (uint32_t): Data size to be loaded.
//     [IN] buf (void*): Data to be loaded.
//     [OUT] outsize (uint32_t*): Data size to be loaded.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInternal: The argument info is an
//     invalid value. kEsfParameterStorageManagerStatusInternal: Internal
//     processing failed. kEsfParameterStorageManagerStatusDataLoss: Unable to
//     access data storage area. kEsfParameterStorageManagerStatusUnavailable:
//     Failed to load data.

// Note:
// """
EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterLoadOther(
    EsfParameterStorageManagerItemID id,
    const EsfParameterStorageManagerWorkMemberData* member, uint32_t offset,
    uint32_t size, void* buf, uint32_t* outsize);

// """Erases data from other storage area.

// Erases data from other storage area.

// Args:
//     [IN] id (EsfParameterStorageManagerItemID): Data type.
//     [IN] member (const EsfParameterStorageManagerWorkMemberData*): working
//     information.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInternal: The argument info is an
//     invalid value. kEsfParameterStorageManagerStatusInternal: Internal
//     processing failed. kEsfParameterStorageManagerStatusDataLoss: Unable to
//     access data storage area. kEsfParameterStorageManagerStatusUnavailable:
//     Failed to load data.

// Note:
// """
EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterClearOther(
    EsfParameterStorageManagerItemID id,
    const EsfParameterStorageManagerWorkMemberData* member);

// """Resets the data in other storage area to the factory defaults.

// Resets the data in other storage area to the factory defaults.

// Args:
//     [IN] id (EsfParameterStorageManagerItemID): Data type.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInternal: The argument info is an
//     invalid value. kEsfParameterStorageManagerStatusInternal: Parameter
//     Storage Manager was not initialized.
//     kEsfParameterStorageManagerStatusInternal: Internal processing failed.
//     kEsfParameterStorageManagerStatusDataLoss: Unable to access data storage
//     area. kEsfParameterStorageManagerStatusUnavailable: Failed to save data.

// Note:
// """
EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterFactoryResetOther(
    EsfParameterStorageManagerItemID id);

// """Gets storage information from other storage area.

// Gets storage information from other storage area.

// Args:
//     [IN] id (EsfParameterStorageManagerItemID): Data type.
//     [IN] member (const EsfParameterStorageManagerWorkMemberData*): working
//     information. [OUT] storage (EsfParameterStorageManagerStorageInfo*):
//     Storage information.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInternal: The argument info is an
//     invalid value. kEsfParameterStorageManagerStatusInternal: Internal
//     processing failed. kEsfParameterStorageManagerStatusDataLoss: Unable to
//     access data storage area. kEsfParameterStorageManagerStatusUnavailable:
//     Failed to load data.

// Note:
// """
EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterGetStorageInfoOther(
    EsfParameterStorageManagerItemID id,
    const EsfParameterStorageManagerWorkMemberData* member,
    EsfParameterStorageManagerStorageInfo* storage);

// """Requests the other data storage area to start updating.

// Requests the other data storage area to start updating.

// Args:
//     [IN] id (EsfParameterStorageManagerItemID): Data type.
//     [IN] type (EsfParameterStorageManagerUpdateType): Update type.
//     [IN/OUT] member (EsfParameterStorageManagerWorkMemberData*): Working
//     information
//                  for members of the target data structure.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInternal: The argument info is an
//     invalid value. kEsfParameterStorageManagerStatusInternal: Internal
//     processing failed. kEsfParameterStorageManagerStatusDataLoss: Unable to
//     access data storage area. kEsfParameterStorageManagerStatusUnavailable:
//     Failed to allocate data.

// Note:
// """
EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterUpdateBeginOther(
    EsfParameterStorageManagerItemID id,
    EsfParameterStorageManagerUpdateType type,
    EsfParameterStorageManagerWorkMemberData* member);

// """Requests the other data storage area to complete updating.

// Requests the other data storage area to complete updating.

// Args:
//     [IN] id (EsfParameterStorageManagerItemID): Data type.
//     [IN/OUT] member (EsfParameterStorageManagerWorkMemberData*): Working
//     information
//                  for members of the target data structure.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInternal: The argument info is an
//     invalid value. kEsfParameterStorageManagerStatusInternal: Internal
//     processing failed. kEsfParameterStorageManagerStatusDataLoss: Unable to
//     access data storage area. kEsfParameterStorageManagerStatusUnavailable:
//     Failed to switch data.

// Note:
// """
EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterUpdateCompleteOther(
    EsfParameterStorageManagerItemID id,
    EsfParameterStorageManagerWorkMemberData* member);

// """Requests the other data storage area to cancel updating.

// Requests the other data storage area to cancel updating.

// Args:
//     [IN] id (EsfParameterStorageManagerItemID): Data type.
//     [IN/OUT] member (EsfParameterStorageManagerWorkMemberData*): Working
//     information
//                  for members of the target data structure.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInternal: The argument info is an
//     invalid value. kEsfParameterStorageManagerStatusInternal: Internal
//     processing failed. kEsfParameterStorageManagerStatusDataLoss: Unable to
//     access data storage area. kEsfParameterStorageManagerStatusUnavailable:
//     Failed to discard data.

// Note:
// """
EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterUpdateCancelOther(
    EsfParameterStorageManagerItemID id,
    EsfParameterStorageManagerWorkMemberData* member);

// """Initializes other storage area.

// Initializes other storage area.

// Args:
//     nothing.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInternal: Internal processing failed.

// Note:
// """
EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterInitOther(void);

// """Deinitializes other storage area.

// Deinitializes other storage area.

// Args:
//     nothing.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInternal: Internal processing failed.

// Note:
// """
EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterDeinitOther(void);

// """Delete unmanaged items.

// Delete unmanaged items.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInternal: Internal processing failed.
//     kEsfParameterStorageManagerStatusDataLoss: Unable to access data storage
//     area.
//     kEsfParameterStorageManagerStatusUnavailable: Failed to delete
//     data.

// Note:
// """
EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterCleanOther(void);

// """Downgrades the device from v2 to v1.

// Downgrades the device from v2 to v1.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInternal: Internal processing failed.

// Note:
// """
EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterDowngradeOther(void);

#endif  // ESF_PARAMETER_STORAGE_MANAGER_SRC_PARAMETER_STORAGE_MANAGER_STORAGE_ADAPTER_OTHER_H_  // NOLINT
