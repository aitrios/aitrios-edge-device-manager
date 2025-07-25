/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef ESF_PARAMETER_STORAGE_MANAGER_SRC_PARAMETER_STORAGE_MANAGER_INTERNAL_WORK_H_  // NOLINT
#define ESF_PARAMETER_STORAGE_MANAGER_SRC_PARAMETER_STORAGE_MANAGER_INTERNAL_WORK_H_  // NOLINT

#include "parameter_storage_manager.h"
#include "parameter_storage_manager/src/parameter_storage_manager_buffer.h"
#include "parameter_storage_manager/src/parameter_storage_manager_config.h"

// An enumeration that defines the cancellation behavior.
typedef enum EsfParameterStorageManagerCancelMode {
  // Skip the cancellation process.
  kEsfParameterStorageManagerCancelSkip,
  // The cancellation process will be saved.
  kEsfParameterStorageManagerCancelSave,
  // The deletion will be performed by canceling the process.
  kEsfParameterStorageManagerCancelClear,
} EsfParameterStorageManagerCancelMode;

// Structure that stores information about the data storage area.
typedef struct EsfParameterStorageManagerStorageInfo {
  // Stores information about available data storage capabilities.
  EsfParameterStorageManagerItemCapabilities capabilities;

  // The size of the data being written.
  // If the "initial_value" is true, this is the initial data size.
  uint32_t written_size;
} EsfParameterStorageManagerStorageInfo;

// The member's work information structure.
typedef struct EsfParameterStorageManagerWorkMemberData {
  // Use only for non-custom processing.
  // During save and delete processes, if the data does not change before and
  // after the process, set Skip to skip the cancellation process.
  // If the data before saving has been deleted, set Clear to execute data
  // deletion processing when canceling. In all other cases, set Save to execute
  // data save processing when canceling.
  EsfParameterStorageManagerCancelMode cancel;

  // The result of the mask test. If true, it needs to be processed.
  bool enabled;

  // Flag that determine the save behavior. Use only for non-custom processing.
  // If true, a diff operation is performed on existing data.
  // If false, existing data is deleted before saving.
  bool append;

  // Indicates that a temporary data storage area is used.
  // If true, a temporary data storage area is used.
  // If false, a temporary data storage area is not used.
  bool update;

  // Information about the acquired data storage area.
  EsfParameterStorageManagerStorageInfo storage;

  // The management information for the allocated large heap.
  EsfParameterStorageManagerBuffer buffer;

  // This is the information to be updated.
  // In the case of PL Storage, it holds the data ID (PlStorageTmpDataId) of
  // the temporary data storage area.
  // It can be referenced when the update member is true.
  uintptr_t update_data;
} EsfParameterStorageManagerWorkMemberData;

// Structure for storing working information when processing access to the data
// storage area.
typedef struct EsfParameterStorageManagerWorkContext {
  // These are the values passed as arguments from the external API.
  // If the argument does not contain a value, an invalid value will be set.
  EsfParameterStorageManagerMask mask;
  EsfParameterStorageManagerData data;
  const EsfParameterStorageManagerStructInfo* info;
  void* private_data;

  // The index of the data being processed.
  uint32_t index;

  // Member information being processed. Equivalent to &info->items[index].
  const EsfParameterStorageManagerMemberInfo* member_info;

  // Working information for the structure members, accessed as an array.
  EsfParameterStorageManagerWorkMemberData* member_data;
} EsfParameterStorageManagerWorkContext;

// Structure allows uniform access to arrays of different lengths.
// Uses flexible array member.
typedef struct EsfParameterStorageManagerBinaryArray {
  uint32_t size;
  uint8_t data[];
} EsfParameterStorageManagerBinaryArray;

// Structure allows uniform access to arrays of different lengths.
// Uses flexible array member.
typedef struct EsfParameterStorageManagerOffsetBinaryArray {
  uint32_t offset;
  uint32_t size;
  uint8_t data[];
} EsfParameterStorageManagerOffsetBinaryArray;

// Structure allows uniform access to raw type.
typedef struct EsfParameterStorageManagerRaw {
  uint32_t size;
} EsfParameterStorageManagerRaw;

// """Allocates a new working information structure from normal heap memory.

// Allocates a new working information structure from normal heap memory.
// For parameters that you do not use, pass an invalid value as an argument.
// When this structure is no longer needed, release it in
// EsfParameterStorageManagerInternalFreeWork().

// Args:
//     [IN] mask (EsfParameterStorageManagerMask): Mask structure.
//     [IN] data (EsfParameterStorageManagerData): Data structure.
//     [IN] info (const EsfParameterStorageManagerStructInfo*): Access
//     information to the
//                                              structure.
//     [IN] private_data (void*): User data used for custom operation.

// Returns:
//     EsfParameterStorageManagerWorkContext*: The allocated working information
//     structure. If the operation fails, it returns NULL.

// Note:
// """
EsfParameterStorageManagerWorkContext*
EsfParameterStorageManagerInternalAllocateWork(
    EsfParameterStorageManagerMask mask, EsfParameterStorageManagerData data,
    const EsfParameterStorageManagerStructInfo* info, void* private_data);

// """Frees the working information structure.

// Frees the working information structure.
// Any mapped heap memory is also unmapped and released.

// Args:
//     [IN/OUT] work (EsfParameterStorageManagerWorkContext*):
//         Data working context. Cannot pass NULL to this argument.

// Returns:
//     nothing.

// Note:
// """
void EsfParameterStorageManagerInternalFreeWork(
    EsfParameterStorageManagerWorkContext* work);

// """Allocates a new member information structure from normal heap memory.

// Allocates a new member information structure from normal heap memory.
// When this structure is no longer needed, release it in
// EsfParameterStorageManagerInternalFreeMemberData().

// Args:
//     [IN] size (size_t): Number of members.

// Returns:
//     EsfParameterStorageManagerWorkMemberData*: The allocated member
//     information structure. If the operation fails, it returns NULL.

// Note:
// """
EsfParameterStorageManagerWorkMemberData*
EsfParameterStorageManagerInternalAllocateMemberData(size_t size);

// """Frees the member information structure.

// Frees the member information structure.
// Any mapped heap memory is also unmapped and released.

// Args:
//     [IN/OUT] member (EsfParameterStorageManagerWorkMemberData*):
//         Data member context. Cannot pass NULL to this argument.

// Returns:
//     nothing.

// Note:
// """
void EsfParameterStorageManagerInternalFreeMemberData(
    EsfParameterStorageManagerWorkMemberData* member);

// """Performs pre-processing for the loop.

// This function is similar to the begin() function of an iterator.
// Can simplifies loop processing by calling
// EsfParameterStorageManagerInternalWorkNext().

// Args:
//     [IN/OUT] work (EsfParameterStorageManagerWorkContext*):
//         Data working context. Cannot pass NULL to this argument.

// Returns:
//     nothing.

// Note:
// """
void EsfParameterStorageManagerInternalWorkBegin(
    EsfParameterStorageManagerWorkContext* work);

// """Performs the change expression for each loop and determines whether the
// loop should continue.

// Performs the change expression for each loop and determines whether the
// loop should continue. It has functionality similar to the iterator's next().

// Args:
//     [IN/OUT] work (EsfParameterStorageManagerWorkContext*):
//         Data working context. Cannot pass NULL to this argument.

// Returns:
//     bool: If true, processing can continue. If false, processing cannot
//     continue because the end of the array has been reached.

// Note:
// """
bool EsfParameterStorageManagerInternalWorkNext(
    EsfParameterStorageManagerWorkContext* work);

// """Sets the mask enable flag.

// Use the mask validity check function to set the mask validity flag.
// Please make sure that the mask validity check function is not NULL before
// calling this function.

// Args:
//     [IN/OUT] work (EsfParameterStorageManagerWorkContext*):
//         Data working context. Cannot pass NULL to this argument.

// Returns:
//     int32_t: The number of valid masks.

// Note:
// """
int32_t EsfParameterStorageManagerInternalSetupWorkMask(
    EsfParameterStorageManagerWorkContext* work);

// """Gets information about the data storage area.

// Gets information about the data storage area.
// It does not depend on the "index" and retrieves information for all members.

// Args:
//     [IN] handle (EsfParameterStorageManagerHandle): Parameter Storage Manager
//     operation handle. [IN/OUT] work (EsfParameterStorageManagerWorkContext*):
//         Data working context. Cannot pass NULL to this argument.

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
EsfParameterStorageManagerInternalGetWorkStorageInfo(
    EsfParameterStorageManagerHandle handle,
    EsfParameterStorageManagerWorkContext* work);

// """Restores data from the data storage area.

// Restores data from the data storage area.
// Data will be restored from the member indicated by "index" to the beginning
// of the array.

// Args:
//     [IN/OUT] work (EsfParameterStorageManagerWorkContext*):
//         Data working context. Cannot pass NULL to this argument.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInternal: The argument info is an
//     invalid value. kEsfParameterStorageManagerStatusInternal: Internal
//     processing failed. kEsfParameterStorageManagerStatusResourceExhausted:
//     Out of resources. kEsfParameterStorageManagerStatusDataLoss: Unable to
//     access data storage area. kEsfParameterStorageManagerStatusUnavailable:
//     Failed to save data.

// Note:
// """
EsfParameterStorageManagerStatus EsfParameterStorageManagerInternalCancel(
    EsfParameterStorageManagerWorkContext* work);

// """Loads data from the data storage area and backs it up.

// Loads data from the data storage area and backs it up.
// Backs up the data of the member pointed to by "index".
// Large heap is used for the backup, and the heap is stored in "buffer".

// Args:
//     [IN/OUT] work (EsfParameterStorageManagerWorkContext*):
//         Data working context. Cannot pass NULL to this argument.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInternal: The argument info is an
//     invalid value. kEsfParameterStorageManagerStatusInternal: Internal
//     processing failed. kEsfParameterStorageManagerStatusResourceExhausted:
//     Out of resources. kEsfParameterStorageManagerStatusDataLoss: Unable to
//     access data storage area. kEsfParameterStorageManagerStatusUnavailable:
//     Failed to load data.

// Note:
// """
EsfParameterStorageManagerStatus EsfParameterStorageManagerInternalLoadBackup(
    EsfParameterStorageManagerWorkContext* work);

// """Supported save operations.

// Controls the processing sequence for saving data to the data store
// (non-custom data only).

// Args:
//     [IN/OUT] work (EsfParameterStorageManagerWorkContext*):
//         Data working context. Cannot pass NULL to this argument.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInternal: The argument info is an
//     invalid value. kEsfParameterStorageManagerStatusPermissionDenied: Could
//     not restore the data to the state
//                                 before saving.
//     kEsfParameterStorageManagerStatusDataLoss: Unable to access data storage
//     area. kEsfParameterStorageManagerStatusUnavailable: Failed to save data.
//     kEsfParameterStorageManagerStatusOutOfRange: The range of the data
//     storage area has been
//                           exceeded.
//     kEsfParameterStorageManagerStatusInternal: Internal processing failed.

// Note:
// """
EsfParameterStorageManagerStatus
EsfParameterStorageManagerInternalSupportedSave(
    EsfParameterStorageManagerWorkContext* work);

// """Supported clear operations.

// Controls the processing sequence for clearing data to the data store
// (non-custom data only).

// Args:
//     [IN/OUT] work (EsfParameterStorageManagerWorkContext*):
//         Data working context. Cannot pass NULL to this argument.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInternal: The argument info is an
//     invalid value. kEsfParameterStorageManagerStatusPermissionDenied: Could
//     not restore the data to the state
//                                 before saving.
//     kEsfParameterStorageManagerStatusDataLoss: Unable to access data storage
//     area. kEsfParameterStorageManagerStatusUnavailable: Failed to save data.
//     kEsfParameterStorageManagerStatusInternal: Internal processing failed.

// Note:
// """
EsfParameterStorageManagerStatus
EsfParameterStorageManagerInternalSupportedClear(
    EsfParameterStorageManagerWorkContext* work);

// """Supported clear operations.

// Controls the processing sequence for clearing data to the data store
// (non-custom data only).

// Args:
//     [IN] handle (EsfParameterStorageManagerHandle): Parameter Storage Manager
//     operation handle. [IN/OUT] work (EsfParameterStorageManagerWorkContext*):
//         Data working context. Cannot pass NULL to this argument.

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
EsfParameterStorageManagerInternalAlreadyUpdateBegin(
    EsfParameterStorageManagerHandle handle,
    EsfParameterStorageManagerWorkContext* work);

// """Determine if the specified update target is valid.

// Determine if the specified update target is valid.

// Args:
//     [IN] work (EsfParameterStorageManagerWorkContext*):
//         Data working context. Cannot pass NULL to this argument.
//     [IN] enabled_mask_num (int32_t):
//         The number of valid Item IDs.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInvalidArgument: The item ID is
//        duplicated.
//     kEsfParameterStorageManagerStatusOutOfRange: Too many item IDs.

// Note:
// """
EsfParameterStorageManagerStatus
EsfParameterStorageManagerInternalUpdateBeginIsValid(
    EsfParameterStorageManagerWorkContext* work, int32_t enabled_mask_num);

// """Cancels the start of data refresh.

// Cancels the start of data refresh.
// The temporary data storage area will be discarded.

// Args:
//     [IN] handle (EsfParameterStorageManagerHandle): Parameter Storage Manager
//     operation handle. [IN/OUT] work (EsfParameterStorageManagerWorkContext*):
//         Data working context. Cannot pass NULL to this argument.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInternal: The argument info is an
//     invalid value. kEsfParameterStorageManagerStatusInternal: Internal
//     processing failed. kEsfParameterStorageManagerStatusResourceExhausted:
//     Out of resources. kEsfParameterStorageManagerStatusDataLoss: Unable to
//     access data storage area. kEsfParameterStorageManagerStatusUnavailable:
//     Failed to discard temporary data storage area.

// Note:
// """
EsfParameterStorageManagerStatus EsfParameterStorageManagerInternalUpdateCancel(
    EsfParameterStorageManagerHandle handle,
    EsfParameterStorageManagerWorkContext* work);

#endif  // ESF_PARAMETER_STORAGE_MANAGER_SRC_PARAMETER_STORAGE_MANAGER_INTERNAL_WORK_H_  // NOLINT
