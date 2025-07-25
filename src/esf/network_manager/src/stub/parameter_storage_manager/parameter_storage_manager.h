/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESF_PARAMETER_STORAGE_MANAGER_INCLUDE_PARAMETER_STORAGE_MANAGER_PARAMETER_STORAGE_MANAGER_EXT_H_
#define ESF_PARAMETER_STORAGE_MANAGER_INCLUDE_PARAMETER_STORAGE_MANAGER_PARAMETER_STORAGE_MANAGER_EXT_H_
#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "parameter_storage_manager_common.h"

// """Initialize Parameter Storage Manager module.

// Allocate internal resources. Please call this API before other Parameter
// Storage Manager API.

// Args:
//     Nothing.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusResourceExhausted: Failed to allocate
//              internal resources.
//     kEsfParameterStorageManagerStatusInternal: Internal
//              error.

// Note:
// """
EsfParameterStorageManagerStatus EsfParameterStorageManagerInit(void);

// """Terminate Parameter Storage Manager module.

// Freeing internal resources. Do not call other Parameter Storage Manager API
// after this API.

// Args:
//     Nothing.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInternal: Internal error.

// Note:
// """
EsfParameterStorageManagerStatus EsfParameterStorageManagerDeinit(void);

// """Get a handle for controlling Parameter Storage Manager.

// Makes an unused handle available and returns it.
// The obtained handle is able to use to call APIs.

// Args:
//     [OUT] handle (EsfParameterStorageManagerHandle*): Pointer to receive a
//              handle.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInvalidArgument: The argument is
//              invalid.
//     kEsfParameterStorageManagerStatusTimedOut: Could not be completed the
//              operation.
//     kEsfParameterStorageManagerStatusResourceExhausted: There are no unused
//              handles.
//     kEsfParameterStorageManagerStatusInternal: Internal error.

// Note:
// """
EsfParameterStorageManagerStatus EsfParameterStorageManagerOpen(
    EsfParameterStorageManagerHandle* handle);

// """Release a handle for controlling Parameter Storage Manager.

// Makes an used handle unavailable.
// If a handle is in use, it cannot be released.

// Args:
//     [IN] handle (EsfParameterStorageManagerHandle): A handle to be released.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInvalidArgument: The argument is
//              invalid.
//     kEsfParameterStorageManagerStatusFailedPrecondition: The handle is in
//              use.
//     kEsfParameterStorageManagerStatusTimedOut: Could not be completed the
//              operation.
//     kEsfParameterStorageManagerStatusNotFound: The resource not found.
//     kEsfParameterStorageManagerStatusInternal: Internal error.

// Note:
// """
EsfParameterStorageManagerStatus EsfParameterStorageManagerClose(
    EsfParameterStorageManagerHandle handle);

// """Factory reset the storage area.

// Calls the factory reset function of each submodules.
// Those functions erase data from storage area.

// Args:
//     nothing.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusTimedOut: Could not be completed the
//              operation.
//     kEsfParameterStorageManagerStatusDataLoss: Failed to erase some or all
//              data.
//     kEsfParameterStorageManagerStatusInternal: Internal error.

// Note:
// """
EsfParameterStorageManagerStatus EsfParameterStorageManagerInvokeFactoryReset(
    void);

// """Saves data to the data storage area.

// Saves data to the data storage area.

// Args:
//     [IN] handle (EsfParameterStorageManagerHandle): A handle for Parameter
//              Storage Manager.
//     [IN] mask (EsfParameterStorageManagerMask): Mask structure.
//     [IN] data (EsfParameterStorageManagerData): Data structure.
//     [IN] info (const EsfParameterStorageManagerStructInfo*): Access
//              information to the structure.
//     [IN] private_data (void*): User data used for custom operation.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInvalidArgument: The argument is
//              invalid.
//     kEsfParameterStorageManagerStatusInternal: The argument info is
//              an invalid value.
//     kEsfParameterStorageManagerStatusTimedOut: Exclusion control timed out.
//     kEsfParameterStorageManagerStatusNotFound: Valid handle not found.
//     kEsfParameterStorageManagerStatusResourceExhausted: Failed to allocate
//              internal resources.
//     kEsfParameterStorageManagerStatusPermissionDenied: Could not restore the
//              data to the state before saving.
//     kEsfParameterStorageManagerStatusDataLoss: Unable to access data storage
//              area.
//     kEsfParameterStorageManagerStatusUnavailable: Failed to save data.
//     kEsfParameterStorageManagerStatusOutOfRange: The range of the data
//              storage area has been exceeded.
//     kEsfParameterStorageManagerStatusInternal: Parameter Storage Manager was
//              not initialized.
//     kEsfParameterStorageManagerStatusInternal: Internal processing failed.

// Note:
// """
EsfParameterStorageManagerStatus EsfParameterStorageManagerSave(
    EsfParameterStorageManagerHandle handle,
    EsfParameterStorageManagerMask mask, EsfParameterStorageManagerData data,
    const EsfParameterStorageManagerStructInfo* info, void* private_data);

// """Load data from the data storage area.

// Load data from the data storage area.

// Args:
//     [IN] handle (EsfParameterStorageManagerHandle): A handle for Parameter
//              Storage Manager.
//     [IN] mask (EsfParameterStorageManagerMask): Mask structure.
//     [OUT] data (EsfParameterStorageManagerData): Data structure.
//     [IN] info (const EsfParameterStorageManagerStructInfo*): Access
//              information to the structure.
//     [IN] private_data (void*): User data used for custom operation.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInvalidArgument: The argument is
//              invalid.
//     kEsfParameterStorageManagerStatusInternal: The argument info is invalid.
//     kEsfParameterStorageManagerStatusTimedOut: Exclusion control timed out.
//     kEsfParameterStorageManagerStatusNotFound: Valid handle not found.
//     kEsfParameterStorageManagerStatusResourceExhausted: Failed to allocate
//              internal resources.
//     kEsfParameterStorageManagerStatusDataLoss: Unable to access data storage
//              area.
//     kEsfParameterStorageManagerStatusUnavailable: Failed to save data.
//     kEsfParameterStorageManagerStatusOutOfRange: The range of the data
//              storage area has been exceeded.
//     kEsfParameterStorageManagerStatusInternal: Parameter Storage Manager was
//              not initialized.
//     kEsfParameterStorageManagerStatusInternal: Internal processing failed.

// Note:
// """
EsfParameterStorageManagerStatus EsfParameterStorageManagerLoad(
    EsfParameterStorageManagerHandle handle,
    EsfParameterStorageManagerMask mask, EsfParameterStorageManagerData data,
    const EsfParameterStorageManagerStructInfo* info, void* private_data);

// """Delete data in the data storage area.

// Delete data in the data storage area.

// Args:
//     [IN] handle (EsfParameterStorageManagerHandle): A handle for Parameter
//              Storage Manager.
//     [IN] mask (EsfParameterStorageManagerMask): Mask structure.
//     [IN] info (const EsfParameterStorageManagerStructInfo*): Access
//              information to the structure.
//     [IN] private_data (void*): User data used for custom operation.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInvalidArgument: The argument is
//              invalid.
//     kEsfParameterStorageManagerStatusInternal: The argument info is invalid.
//     kEsfParameterStorageManagerStatusTimedOut: Exclusion control timed out.
//     kEsfParameterStorageManagerStatusNotFound: Valid handle not found.
//     kEsfParameterStorageManagerStatusResourceExhausted: Failed to allocate
//              internal resources.
//     kEsfParameterStorageManagerStatusPermissionDenied: Could not restore the
//              data to the state before saving.
//     kEsfParameterStorageManagerStatusDataLoss: Unable to access data storage
//              area.
//     kEsfParameterStorageManagerStatusUnavailable: Failed to save data.
//     kEsfParameterStorageManagerStatusInternal: Parameter Storage Manager was
//              not initialized.
//     kEsfParameterStorageManagerStatusInternal: Internal processing failed.

// Note:
// """
EsfParameterStorageManagerStatus EsfParameterStorageManagerClear(
    EsfParameterStorageManagerHandle handle,
    EsfParameterStorageManagerMask mask,
    const EsfParameterStorageManagerStructInfo* info, void* private_data);

// """Resets the data in the data storage area to the factory defaults.

// Resets the data in the data storage area to the factory defaults.

// Args:
//     [IN] mask (EsfParameterStorageManagerMask): Mask structure.
//     [IN] info (const EsfParameterStorageManagerStructInfo*): Access
//              information to the structure.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInvalidArgument: The argument is
//              invalid.
//     kEsfParameterStorageManagerStatusTimedOut: Exclusion control timed out.
//     kEsfParameterStorageManagerStatusResourceExhausted: Failed to allocate
//              internal resources.
//     kEsfParameterStorageManagerStatusDataLoss: Unable to access data storage
//              area.
//     kEsfParameterStorageManagerStatusUnavailable: Failed to access data
//              storage area.
//     kEsfParameterStorageManagerStatusInternal: Parameter Storage Manager was
//              not initialized.
//     kEsfParameterStorageManagerStatusInternal: Internal processing failed.

// Note:
// """
EsfParameterStorageManagerStatus EsfParameterStorageManagerGetSize(
    EsfParameterStorageManagerHandle handle,
    EsfParameterStorageManagerItemID id, uint32_t* loadable_size);

// """Starts exclusive control of access to the data storage area.

// Starts exclusive control of access to the data storage area.

// Args:
//     Noting.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusTimedOut: Exclusion control timed out.
//     kEsfParameterStorageManagerStatusInternal: Internal processing failed.

// Note:
// """
EsfParameterStorageManagerStatus EsfParameterStorageManagerLock(void);

// """Terminates exclusive control of access to the data storage area.

// Terminates exclusive control of access to the data storage area.

// Args:
//     Noting.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInternal: Internal processing failed.

// Note:
// """
EsfParameterStorageManagerStatus EsfParameterStorageManagerUnlock(void);

// """Register the Factory Reset function.

// Register the Factory Reset function.

// Args:
//     [IN] func (EsfParameterStorageManagerRegisterFactoryResetType):
//              Factory Reset function to be registered.
//     [IN] private_data (void*): User data used in the Factory Reset function
//              to be registered.
//     [OUT] id (EsfParameterStorageManagerFactoryResetID*): The registration
//              identifier of Factory Reset.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInvalidArgument: The argument is
//              invalid.
//     kEsfParameterStorageManagerStatusResourceExhausted: Failed to allocate
//              internal resources.
//     kEsfParameterStorageManagerStatusInternal: Internal processing failed.

// Note:
// """
EsfParameterStorageManagerStatus EsfParameterStorageManagerRegisterFactoryReset(
    EsfParameterStorageManagerRegisterFactoryResetType func, void* private_data,
    EsfParameterStorageManagerFactoryResetID* id);

// """Unregister the Factory Reset function.

// Unregister the Factory Reset function.

// Args:
//     [IN] id (EsfParameterStorageManagerFactoryResetID): The registration
//              identifier of Factory Reset.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInvalidArgument: The argument is
//              invalid.
//     kEsfParameterStorageManagerStatusNotFound: Valid handle not
//              found.
//     kEsfParameterStorageManagerStatusInternal: Parameter Storage
//              Manager was not initialized.
//     kEsfParameterStorageManagerStatusInternal: Internal processing failed.

// Note:
// """
EsfParameterStorageManagerStatus
EsfParameterStorageManagerUnregisterFactoryReset(
    EsfParameterStorageManagerFactoryResetID id);

// """Update of the data storage area will begin.

// Update of the data storage area will begin.

// Args:
//     [IN] handle (EsfParameterStorageManagerHandle): A handle for Parameter
//              Storage Manager.
//     [IN] mask (EsfParameterStorageManagerMask): Mask structure.
//     [IN] info (const EsfParameterStorageManagerStructInfo*): Access
//              information to the structure.
//     [IN] type (EsfParameterStorageManagerUpdateType): State of the temporary
//              data storage area after executing this API.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInvalidArgument: The argument is
//              invalid.
//     kEsfParameterStorageManagerStatusFailedPrecondition: The update has
//              already started.
//     kEsfParameterStorageManagerStatusTimedOut: Exclusion control timed out.
//     kEsfParameterStorageManagerStatusNotFound: Valid handle not found.
//     kEsfParameterStorageManagerStatusResourceExhausted: Failed to allocate
//              internal resources.
//     kEsfParameterStorageManagerStatusDataLoss: Unable to access data storage
//              area.
//     kEsfParameterStorageManagerStatusUnavailable: Failed to discard temporary
//              data storage area.
//     kEsfParameterStorageManagerStatusInternal: Parameter Storage Manager was
//              not initialized.
//     kEsfParameterStorageManagerStatusInternal: Internal processing failed.

// Note:
// """
EsfParameterStorageManagerStatus EsfParameterStorageManagerUpdateBegin(
    EsfParameterStorageManagerHandle handle,
    EsfParameterStorageManagerMask mask,
    const EsfParameterStorageManagerStructInfo* info,
    EsfParameterStorageManagerUpdateType type);

// """The data storage area update is complete.

// The data storage area update is complete.

// Args:
//     [IN] handle (EsfParameterStorageManagerHandle): A handle for Parameter
//              Storage Manager.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInvalidArgument: The argument is
//              invalid.
//     kEsfParameterStorageManagerStatusFailedPrecondition: The update has
//              not started.
//     kEsfParameterStorageManagerStatusTimedOut: Exclusion control timed out.
//     kEsfParameterStorageManagerStatusNotFound: Valid handle not found.
//     kEsfParameterStorageManagerStatusResourceExhausted: Failed to allocate
//              internal resources.
//     kEsfParameterStorageManagerStatusDataLoss: Unable to access data storage
//              area.
//     kEsfParameterStorageManagerStatusUnavailable: Failed to discard temporary
//              data storage area.
//     kEsfParameterStorageManagerStatusInternal: Parameter Storage Manager was
//              not initialized.
//     kEsfParameterStorageManagerStatusInternal: Internal processing failed.

// Note:
// """
EsfParameterStorageManagerStatus EsfParameterStorageManagerUpdateComplete(
    EsfParameterStorageManagerHandle handle);

// """The data storage area update is complete.

// The data storage area update is complete.

// Args:
//     [IN] handle (EsfParameterStorageManagerHandle): A handle for Parameter
//              Storage Manager.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInvalidArgument: The argument is
//              invalid.
//     kEsfParameterStorageManagerStatusFailedPrecondition: The update has
//              not started.
//     kEsfParameterStorageManagerStatusTimedOut: Exclusion control timed out.
//     kEsfParameterStorageManagerStatusNotFound: Valid handle not found.
//     kEsfParameterStorageManagerStatusResourceExhausted: Failed to allocate
//              internal resources.
//     kEsfParameterStorageManagerStatusDataLoss: Unable to access data storage
//              area.
//     kEsfParameterStorageManagerStatusUnavailable: Failed to discard temporary
//              data storage area.
//     kEsfParameterStorageManagerStatusInternal: Parameter Storage Manager was
//              not initialized.
//     kEsfParameterStorageManagerStatusInternal: Internal processing failed.

// Note:
// """
EsfParameterStorageManagerStatus EsfParameterStorageManagerUpdateCancel(
    EsfParameterStorageManagerHandle handle);

// """Determines if a particular member of the structure is empty.

// Determines if a particular member of the structure is empty.
// If a particular member of a struct is empty, the return value is true,
// otherwise it returns false.

// Args:
//     [IN] data (EsfParameterStorageManagerData): Data structure.
//     [IN] info (const EsfParameterStorageManagerStructInfo* info): The access
//              information for the structure.
//     [IN] index (size_t): The index of the target data member.
//              Please specify a value smaller than info->items_num.

// Returns:
//     bool: If a particular member of a struct is empty, the return value is
//     true, otherwise it returns false.

// Note:
// """
bool EsfParameterStorageManagerIsDataEmpty(
    EsfParameterStorageManagerData data,
    const EsfParameterStorageManagerStructInfo* info, size_t index);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
}
#endif
#endif  // ESF_PARAMETER_STORAGE_MANAGER_INCLUDE_PARAMETER_STORAGE_MANAGER_PARAMETER_STORAGE_MANAGER_EXT_H_
