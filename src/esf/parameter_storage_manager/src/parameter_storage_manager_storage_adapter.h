/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef ESF_PARAMETER_STORAGE_MANAGER_SRC_PARAMETER_STORAGE_MANAGER_STORAGE_ADAPTER_H_  // NOLINT
#define ESF_PARAMETER_STORAGE_MANAGER_SRC_PARAMETER_STORAGE_MANAGER_STORAGE_ADAPTER_H_  // NOLINT

#include "parameter_storage_manager.h"
#include "parameter_storage_manager/src/parameter_storage_manager_internal_work.h"  // NOLINT

// """Saves data to the data storage area.

// Saves data to the data storage area.

// Args:
//     [IN] id (EsfParameterStorageManagerItemID): Data type.
//     [IN] offset (uint32_t): The location where data is stored.
//     [IN] size (uint32_t): Data size to be saved.
//     [IN] data (const uint8_t*): Data to be saved.
//     [OUT] save_size (uint32_t*): Data size to be saved.
//     [IN/OUT] member (EsfParameterStorageManagerWorkMemberData*): working
//     information.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInvalidArgument: The argument is
//     invalid. kEsfParameterStorageManagerStatusInternal: The argument info is
//     an invalid value. kEsfParameterStorageManagerStatusInternal: Internal
//     processing failed. kEsfParameterStorageManagerStatusOutOfRange: Invalid
//     range was specified. kEsfParameterStorageManagerStatusDataLoss: Unable to
//     access data storage area. kEsfParameterStorageManagerStatusUnavailable:
//     Failed to save data.

// Note:
// """
EsfParameterStorageManagerStatus EsfParameterStorageManagerStorageAdapterSave(
    EsfParameterStorageManagerItemID id, uint32_t offset, uint32_t size,
    const uint8_t* data, uint32_t* save_size,
    EsfParameterStorageManagerWorkMemberData* member);

// """Load data from the data storage area.

// Load data from the data storage area.

// Args:
//     [IN] id (EsfParameterStorageManagerItemID): Data type.
//     [IN] member (const EsfParameterStorageManagerWorkMemberData*): working
//     information. [IN] offset (uint32_t): The location where data is stored.
//     [IN] size (uint32_t): Data size to be saved.
//     [OUT] data (uint8_t*): Data to be saved.
//     [OUT] load_size (uint32_t*): Data size to be saved.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInvalidArgument: The argument is
//     invalid. kEsfParameterStorageManagerStatusOutOfRange: Invalid range was
//     specified. kEsfParameterStorageManagerStatusInternal: The argument info
//     is an invalid value. kEsfParameterStorageManagerStatusInternal: Internal
//     processing failed. kEsfParameterStorageManagerStatusDataLoss: Unable to
//     access data storage area. kEsfParameterStorageManagerStatusUnavailable:
//     Failed to save data.

// Note:
// """
EsfParameterStorageManagerStatus EsfParameterStorageManagerStorageAdapterLoad(
    EsfParameterStorageManagerItemID id,
    const EsfParameterStorageManagerWorkMemberData* member, uint32_t offset,
    uint32_t size, uint8_t* data, uint32_t* load_size);

// """Clears the data in the data storage area.

// Clears the data in the data storage area.

// Args:
//     [IN] id (EsfParameterStorageManagerItemID): Data type.
//     [IN] member (EsfParameterStorageManagerWorkMemberData*): working
//     information.

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
EsfParameterStorageManagerStatus EsfParameterStorageManagerStorageAdapterClear(
    EsfParameterStorageManagerItemID id,
    EsfParameterStorageManagerWorkMemberData* member);

// """Resets the data in the data storage area to the factory defaults.

// Resets the data in the data storage area to the factory defaults.

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
EsfParameterStorageManagerStorageAdapterFactoryReset(
    EsfParameterStorageManagerItemID id);

// """Resets the data in the data storage area to the factory defaults.

// Resets the data in the data storage area to the factory defaults.

// Args:
//     [IN] id (EsfParameterStorageManagerItemID): Data type.
//     [IN] member (const EsfParameterStorageManagerWorkMemberData*): working
//     information. [OUT] storage (EsfParameterStorageManagerStorageInfo*):
//     Information on data storage
//         area.

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
EsfParameterStorageManagerStorageAdapterGetStorageInfo(
    EsfParameterStorageManagerItemID id,
    const EsfParameterStorageManagerWorkMemberData* member,
    EsfParameterStorageManagerStorageInfo* storage);

// """Requests the data storage area to start updating.

// Requests the data storage area to start updating.

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
//     invalid value. kEsfParameterStorageManagerStatusInternal: Parameter
//     Storage Manager was not initialized.
//     kEsfParameterStorageManagerStatusInternal: Internal processing failed.
//     kEsfParameterStorageManagerStatusDataLoss: Unable to access data storage
//     area. kEsfParameterStorageManagerStatusUnavailable: Failed to allocate
//     data.

// Note:
// """
EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterUpdateBegin(
    EsfParameterStorageManagerItemID id,
    EsfParameterStorageManagerUpdateType type,
    EsfParameterStorageManagerWorkMemberData* member);

// """Requests the data storage area to complete updating.

// Requests the data storage area to complete updating.

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
//     invalid value. kEsfParameterStorageManagerStatusInternal: Parameter
//     Storage Manager was not initialized.
//     kEsfParameterStorageManagerStatusInternal: Internal processing failed.
//     kEsfParameterStorageManagerStatusDataLoss: Unable to access data storage
//     area. kEsfParameterStorageManagerStatusUnavailable: Failed to switch
//     data.

// Note:
// """
EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterUpdateComplete(
    EsfParameterStorageManagerItemID id,
    EsfParameterStorageManagerWorkMemberData* member);

// """Requests the data storage area to cancel updating.

// Requests the data storage area to cancel updating.

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
//     invalid value. kEsfParameterStorageManagerStatusInternal: Parameter
//     Storage Manager was not initialized.
//     kEsfParameterStorageManagerStatusInternal: Internal processing failed.
//     kEsfParameterStorageManagerStatusDataLoss: Unable to access data storage
//     area. kEsfParameterStorageManagerStatusUnavailable: Failed to discard
//     data.

// Note:
// """
EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterUpdateCancel(
    EsfParameterStorageManagerItemID id,
    EsfParameterStorageManagerWorkMemberData* member);

// """Initializes the data storage area.

// Initializes the data storage area.

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
EsfParameterStorageManagerStatus EsfParameterStorageManagerStorageAdapterInit(
    void);

// """Deinitializes the data storage area.

// Deinitializes the data storage area.

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
EsfParameterStorageManagerStatus EsfParameterStorageManagerStorageAdapterDeinit(
    void);

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
EsfParameterStorageManagerStorageAdapterGetCapabilities(
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
EsfParameterStorageManagerStorageAdapterGetItemCapabilities(
    EsfParameterStorageManagerItemID id,
    EsfParameterStorageManagerItemCapabilities* capabilities);

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
EsfParameterStorageManagerStatus EsfParameterStorageManagerStorageAdapterClean(
    void);

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
EsfParameterStorageManagerStorageAdapterDowngrade(void);

EsfParameterStorageManagerStatus
  EsfParameterStorageManagerStorageAdapterCleanPLStorage(void);

EsfParameterStorageManagerStatus
  EsfParameterStorageManagerStorageAdapterCleanOther(void);

EsfParameterStorageManagerStatus
  EsfParameterStorageManagerStorageAdapterDowngradeOther(void);

EsfParameterStorageManagerStatus
  EsfParameterStorageManagerStorageAdapterInitPLStorage(void);

EsfParameterStorageManagerStatus
  EsfParameterStorageManagerStorageAdapterInitOther(void);

EsfParameterStorageManagerStatus
  EsfParameterStorageManagerStorageAdapterDeinitOther(void);

EsfParameterStorageManagerStatus
  EsfParameterStorageManagerStorageAdapterDeinitPLStorage(void);

#endif  // ESF_PARAMETER_STORAGE_MANAGER_SRC_PARAMETER_STORAGE_MANAGER_STORAGE_ADAPTER_H_  // NOLINT
