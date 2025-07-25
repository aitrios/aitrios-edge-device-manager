/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef ESF_PARAMETER_STORAGE_MANAGER_SRC_PARAMETER_STORAGE_MANAGER_STORAGE_ADAPTER_PL_H_  // NOLINT
#define ESF_PARAMETER_STORAGE_MANAGER_SRC_PARAMETER_STORAGE_MANAGER_STORAGE_ADAPTER_PL_H_  // NOLINT

#include "parameter_storage_manager.h"

#include "parameter_storage_manager/src/parameter_storage_manager_config.h"
#include "parameter_storage_manager/src/parameter_storage_manager_storage_adapter.h"
#include "parameter_storage_manager/src/parameter_storage_manager_storage_adapter_settings.h"  // NOLINT

// """Converts PlErrCode to EsfParameterStorageManagerStatus and returns it.

// Returns the error code of EsfParameterStorageManagerStatus that corresponds
// to the error code of PlErrCode.

// Args:
//     [IN] status (PlErrCode): error code.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusDataLoss: Failed to save data.
//     kEsfParameterStorageManagerStatusUnavailable: Unable to access data
//     storage area. kEsfParameterStorageManagerStatusInternal: Internal
//     processing failed.

// Note:
// """
EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterConvertPlResult(PlErrCode status);

// """Saves data to PL Storage.

// Saves data to PL Storage.

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
//     kEsfParameterStorageManagerStatusInternal: The argument info is an
//     invalid value. kEsfParameterStorageManagerStatusInternal: Internal
//     processing failed. kEsfParameterStorageManagerStatusDataLoss: Unable to
//     access data storage area. kEsfParameterStorageManagerStatusUnavailable:
//     Failed to save data.

// Note:
// """
EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterSavePLStorage(
    EsfParameterStorageManagerItemID id, uint32_t offset, uint32_t size,
    const void* buf, uint32_t* outsize,
    EsfParameterStorageManagerWorkMemberData* member);

// """Loads data from PL Storage.

// Loads data from PL Storage.

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
EsfParameterStorageManagerStorageAdapterLoadPLStorage(
    EsfParameterStorageManagerItemID id,
    const EsfParameterStorageManagerWorkMemberData* member, uint32_t offset,
    uint32_t size, void* buf, uint32_t* outsize);

// """Erases data from PL Storage.

// Erases data from PL Storage.

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
//     Failed to erase data.

// Note:
// """
EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterClearPLStorage(
    EsfParameterStorageManagerItemID id,
    const EsfParameterStorageManagerWorkMemberData* member);

// """Factory reset data from PL Storage.

// Factory reset data from PL Storage.

// Args:
//     [IN] id (EsfParameterStorageManagerItemID): Data type.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInternal: The argument info is an
//     invalid value. kEsfParameterStorageManagerStatusInternal: Internal
//     processing failed. kEsfParameterStorageManagerStatusDataLoss: Unable to
//     access data storage area. kEsfParameterStorageManagerStatusUnavailable:
//     Failed to factory reset data.

// Note:
// """
EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterFactoryResetPLStorage(
    EsfParameterStorageManagerItemID id);

// """Gets storage information from PL Storage.

// Gets storage information from PL Storage.

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
EsfParameterStorageManagerStorageAdapterGetStorageInfoPLStorage(
    EsfParameterStorageManagerItemID id,
    const EsfParameterStorageManagerWorkMemberData* member,
    EsfParameterStorageManagerStorageInfo* storage);

// """Requests the pl storage data storage area to start updating.

// Requests the pl storage data storage area to start updating.

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
EsfParameterStorageManagerStorageAdapterUpdateBeginPLStorage(
    EsfParameterStorageManagerItemID id,
    EsfParameterStorageManagerUpdateType type,
    EsfParameterStorageManagerWorkMemberData* member);

// """Requests the pl storage data storage area to complete updating.

// Requests the pl storage data storage area to complete updating.

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
EsfParameterStorageManagerStorageAdapterUpdateCompletePLStorage(
    EsfParameterStorageManagerItemID id,
    EsfParameterStorageManagerWorkMemberData* member);

// """Requests the pl storage data storage area to cancel updating.

// Requests the pl storage data storage area to cancel updating.

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
EsfParameterStorageManagerStorageAdapterUpdateCancelPLStorage(
    EsfParameterStorageManagerItemID id,
    EsfParameterStorageManagerWorkMemberData* member);

// """Initializes the PL Storage.

// Initializes the PL Storage.

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
EsfParameterStorageManagerStorageAdapterInitPLStorage(void);

// """Deinitializes the PL Storage.

// Deinitializes the PL Storage.

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
EsfParameterStorageManagerStorageAdapterDeinitPLStorage(void);

// """Gets information about available data storage capabilities.

// Gets information about available data storage capabilities.
// If a function is unavailable for all data storage areas, the structure member
// corresponding to that function will be set to 0. If a function is available
// for even one data storage area, the structure member corresponding to that
// function will be set to 1.

// Args:
//     [OUT] capabilities (EsfParameterStorageManagerCapabilities*):
//          Stores functional information about the data managed by
//          Parameter Storage Manager.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk:
//          Success.
//     kEsfParameterStorageManagerStatusInvalidArgument:
//          The argument is invalid.
//     kEsfParameterStorageManagerStatusInternal:
//          Internal processing failed.

// Note:
// """
EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterGetCapabilitiesPLStorage(
    EsfParameterStorageManagerCapabilities* capabilities);

// """Gets function information about the data managed by the data storage area.

// Gets function information about the data managed by the data storage area.
// Available functions vary depending on the device and data ID.

// Args:
//     [IN] id (EsfParameterStorageManagerItemID):
//          The data ID for which you want to get function information.
//     [OUT] capabilities (EsfParameterStorageManagerItemCapabilities*):
//          Stores information about available data storage capabilities.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk:
//          Success.
//     kEsfParameterStorageManagerStatusInvalidArgument:
//          The argument is invalid.
//     kEsfParameterStorageManagerStatusInternal:
//          Internal processing failed.

// Note:
// """
EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterGetItemCapabilitiesPLStorage(
    EsfParameterStorageManagerItemID id,
    EsfParameterStorageManagerItemCapabilities* capabilities);

// """Delete unmanaged items.

// Delete unmanaged items by PlStorageClean().

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
EsfParameterStorageManagerStorageAdapterCleanPLStorage(void);

// """Downgrades the device from v2 to v1.

// Downgrades the device from v2 to v1 by PlStorageDowngrade().

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInternal: Internal processing failed.

// Note:
// """
EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterDowngradePLStorage(void);

#endif  // ESF_PARAMETER_STORAGE_MANAGER_SRC_PARAMETER_STORAGE_MANAGER_STORAGE_ADAPTER_PL_H_  // NOLINT
