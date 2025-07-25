/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef ESF_PARAMETER_STORAGE_MANAGER_SRC_PARAMETER_STORAGE_MANAGER_RESOURCE_H_
#define ESF_PARAMETER_STORAGE_MANAGER_SRC_PARAMETER_STORAGE_MANAGER_RESOURCE_H_

#include "parameter_storage_manager.h"

// This is a structure that manages the factory reset.
// The parameters passed as arguments to
// EsfParameterStorageManagerResourceNewFactoryReset() are set.
typedef struct EsfParameterStorageManagerResourceFactoryReset {
  // Factory reset function.
  EsfParameterStorageManagerRegisterFactoryResetType func;

  // User data passed to the factory reset function.
  void* private_data;
} EsfParameterStorageManagerResourceFactoryReset;

// Structure that stores update information.
typedef struct EsfParameterStorageManagerResourceUpdateInfo {
  // The number of valid IDs.
  int32_t count;

  // The data ID to be updated.
  // Specify with EsfParameterStorageManagerUpdateBegin().
  // kEsfParameterStorageManagerItemMax is an invalid value.
  EsfParameterStorageManagerItemID
      id[CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_UPDATE_MAX];

  // This is the information to be updated.
  // In the case of PL Storage, it holds the data ID (PlStorageTmpDataId) of
  // the temporary data storage area.
  // It can be referenced when the 'id' member is not invalid.
  uintptr_t data[CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_UPDATE_MAX];
} EsfParameterStorageManagerResourceUpdateInfo;

// """Initialize the Resource submodule.

// Allocates memory in the Resource submodule. Change the initialized flag to
// true.

// Args:
//     Nothing.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusResourceExhausted: Failed to allocate
//     internal resources.

// Note:
// """
EsfParameterStorageManagerStatus EsfParameterStorageManagerResourceInit(void);

// """Deinit the Resource submodule.

// Releases memory in the Resource submodule. Changes the initialized flag to
// false.

// Args:
//     Nothing.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInternal: Internal processing failed.

// Note:
// """
EsfParameterStorageManagerStatus EsfParameterStorageManagerResourceDeinit(void);

// """Enable handle.

// Activate and retrieve one handle of Parameter Storage Manager managed in the
// Resource submodule.

// Args:
//     [OUT] handle (EsfParameterStorageManagerHandle*): Pointer to the
//     operation handle of Parameter Storage Manager.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusResourceExhausted: Failed to allocate
//     internal resources. kEsfParameterStorageManagerStatusInternal: Device
//     Setting was not initialized. kEsfParameterStorageManagerStatusInternal:
//     Internal processing failed.

// Note:
// """
EsfParameterStorageManagerStatus EsfParameterStorageManagerResourceNewHandle(
    EsfParameterStorageManagerHandle* handle);

// """Disables the handle.

// Disables one handle of Parameter Storage Manager managed in the Resource
// submodule.

// Args:
//     [IN] handle (EsfParameterStorageManagerHandle): Parameter Storage Manager
//     operation handle.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusFailedPrecondition: Handle is in use.
//     kEsfParameterStorageManagerStatusNotFound: No handles found.
//     kEsfParameterStorageManagerStatusInternal: Parameter Storage Manager was
//     not initialized. kEsfParameterStorageManagerStatusInternal: Internal
//     processing failed.

// Note:
// """
EsfParameterStorageManagerStatus EsfParameterStorageManagerResourceDeleteHandle(
    EsfParameterStorageManagerHandle handle);

// """Add the handle's reference count.

// Add the reference count of the handle of Parameter Storage Manager managed in
// the Resource submodule.

// Args:
//     [IN] handle (EsfParameterStorageManagerHandle): Parameter Storage Manager
//     operation handle.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusNotFound: No handles found.
//     kEsfParameterStorageManagerStatusInternal: Parameter Storage Manager was
//     not initialized. kEsfParameterStorageManagerStatusInternal: Internal
//     processing failed.

// Note:
// """
EsfParameterStorageManagerStatus
EsfParameterStorageManagerResourceReferenceHandle(
    EsfParameterStorageManagerHandle handle);

// """Subtracts the handle's reference count.

// Subtracts the reference count of the handle of Parameter Storage Manager
// managed in the Resource submodule.

// Args:
//     [IN] handle (EsfParameterStorageManagerHandle): Parameter Storage Manager
//     operation handle.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusNotFound: No handles found.
//     kEsfParameterStorageManagerStatusInternal: Parameter Storage Manager was
//     not initialized. kEsfParameterStorageManagerStatusInternal: Internal
//     processing failed.

// Note:
// """
EsfParameterStorageManagerStatus
EsfParameterStorageManagerResourceUnreferenceHandle(
    EsfParameterStorageManagerHandle handle);

// """Add a new Factory Reset function.

// Holds the Factory Reset function to an internal resource. Returns the
// identifier of the registered Factory Reset function.

// Args:
//     [IN] func (EsfParameterStorageManagerRegisterFactoryResetType ): Factory
//     Reset
//         function.
//     [IN] private_data (void*): User data to be given to the Factory Reset
//         function.
//     [OUT] id (EsfParameterStorageManagerFactoryResetID*): Factory Reset
//     identifier.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusResourceExhausted: The maximum number of
//     Factory Reset
//                                  registrations has been reached.
//     kEsfParameterStorageManagerStatusInternal: Parameter Storage Manager was
//     not initialized. kEsfParameterStorageManagerStatusInternal: Internal
//     processing failed.

// Note:
// """
EsfParameterStorageManagerStatus
EsfParameterStorageManagerResourceNewFactoryReset(
    EsfParameterStorageManagerRegisterFactoryResetType func, void* private_data,
    EsfParameterStorageManagerFactoryResetID* id);

// """Delete the Factory Reset function.

// Deletes the Factory Reset function of an internal resource.

// Args:
//     [IN] id (EsfParameterStorageManagerFactoryResetID): Factory Reset
//     identifier.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInternal: Parameter Storage Manager was
//     not initialized. kEsfParameterStorageManagerStatusInternal: Internal
//     processing failed.

// Note:
// """
EsfParameterStorageManagerStatus
EsfParameterStorageManagerResourceDeleteFactoryReset(
    EsfParameterStorageManagerFactoryResetID id);

// """Calls the Factory Reset function.

// Calls the Factory Reset function of the internal resource.

// Args:
//     [IN] id (EsfParameterStorageManagerFactoryResetID): Factory Reset
//     identifier. [OUT] factory_reset
//     (EsfParameterStorageManagerResourceFactoryReset*): Factory
//      Reset data.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInternal: Parameter Storage Manager was
//     not initialized. kEsfParameterStorageManagerStatusInternal: Internal
//     processing failed.

// Note:
// """
EsfParameterStorageManagerStatus
EsfParameterStorageManagerResourceGetFactoryReset(
    EsfParameterStorageManagerFactoryResetID id,
    EsfParameterStorageManagerResourceFactoryReset* factory_reset);

// """Set the information about the temporary data storage area to be updated in
// the handle.

// Set the information about the temporary data storage area to be updated in
// the handle.

// Args:
//     [IN] handle (EsfParameterStorageManagerHandle): Parameter Storage Manager
//     operation handle. [IN] id (EsfParameterStorageManagerItemID): The data ID
//     to be updated. [IN] data (uintptr_t): Information about the data to be
//     updated.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInvalidArgument: The handle is invalid.
//     kEsfParameterStorageManagerStatusInternal: Parameter Storage Manager was
//     not initialized. kEsfParameterStorageManagerStatusInternal: Internal
//     processing failed.

// Note:
// """
EsfParameterStorageManagerStatus
EsfParameterStorageManagerResourceSetUpdateDataToHandle(
    EsfParameterStorageManagerHandle handle,
    EsfParameterStorageManagerItemID id, uintptr_t data);

// """Obtain information about the temporary data storage area to be updated
// from the handle.

// Obtain information about the temporary data storage area to be updated from
// the handle.

// Args:
//     [IN] handle (EsfParameterStorageManagerHandle): Parameter Storage Manager
//     operation handle. [OUT] info
//     (EsfParameterStorageManagerResourceUpdateInfo*): Structure that stores
//                  update information.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInvalidArgument: The handle is invalid.
//     kEsfParameterStorageManagerStatusInternal: Parameter Storage Manager was
//     not initialized. kEsfParameterStorageManagerStatusInternal: Internal
//     processing failed.

// Note:
// """
EsfParameterStorageManagerStatus
EsfParameterStorageManagerResourceGetUpdateDataFromHandle(
    EsfParameterStorageManagerHandle handle,
    EsfParameterStorageManagerResourceUpdateInfo* info);

// """Deletes the information about the temporary data storage area to be
// updated from the handle.

// Deletes the information about the temporary data storage area to be updated
// from the handle.

// Args:
//     [IN] handle (EsfParameterStorageManagerHandle): Parameter Storage Manager
//     operation handle.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInvalidArgument: The handle is invalid.
//     kEsfParameterStorageManagerStatusInternal: Parameter Storage Manager was
//     not initialized. kEsfParameterStorageManagerStatusInternal: Internal
//     processing failed.

// Note:
// """
EsfParameterStorageManagerStatus
EsfParameterStorageManagerResourceRemoveUpdateDataFromHandle(
    EsfParameterStorageManagerHandle handle);

// """Determines whether the target handle has already started updating.

// Determines whether the target handle has already started updating.

// Args:
//     [IN] handle (EsfParameterStorageManagerHandle): Parameter Storage Manager
//     operation handle.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInvalidArgument: Invalid argument.
//     kEsfParameterStorageManagerStatusFailedPrecondition: The update has
//     already started. kEsfParameterStorageManagerStatusInternal: Device
//     Setting was not initialized. kEsfParameterStorageManagerStatusInternal:
//     Internal processing failed.

// Note:
// """
EsfParameterStorageManagerStatus
EsfParameterStorageManagerResourceHandleIsAlreadyBeingUpdated(
    EsfParameterStorageManagerHandle handle);

// """Determines whether the target data ID is managed by a handle.

// Determines whether the target data ID is managed by a handle.

// Args:
//     [IN] id (EsfParameterStorageManagerItemID): The data ID to be updated.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusFailedPrecondition: The update has
//     already started. kEsfParameterStorageManagerStatusInternal: Device
//     Setting was not initialized. kEsfParameterStorageManagerStatusInternal:
//     Internal processing failed.

// Note:
// """
EsfParameterStorageManagerStatus
EsfParameterStorageManagerResourceUpdateDataIsExistsInHandles(
    EsfParameterStorageManagerItemID id);

// """Determines if the Parameter Storage Manager is initialized.

// Determines if the Parameter Storage Manager is initialized.
// If the return value is true, it is initialized.
// If the return value is false, it is not initialized.

// Args:
//     Nothing.

// Returns:
//     bool: If the return value is true, it is initialized.
//           If the return value is false, it is not initialized.

// Note:
// """
bool EsfParameterStorageManagerResourceIsInitialized(void);

// """Returns a working buffer for the Large Heap.

// Returns a working buffer for the Large Heap.
// If the return value is NULL, an error occurred.
// The length of that buffer is
// CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_BUFFER_LENGTH.

// Args:
//     Nothing.

// Returns:
//     uint8_t*: a working buffer.

// Note:
// """
uint8_t* EsfParameterStorageManagerResourceGetBuffer(void);

#endif  // ESF_PARAMETER_STORAGE_MANAGER_SRC_PARAMETER_STORAGE_MANAGER_RESOURCE_H_  // NOLINT
