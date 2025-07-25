/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#include "parameter_storage_manager.h"
#include "parameter_storage_manager/src/parameter_storage_manager_config.h"
#include "parameter_storage_manager/src/parameter_storage_manager_internal_work.h"
#include "parameter_storage_manager/src/parameter_storage_manager_mutex.h"
#include "parameter_storage_manager/src/parameter_storage_manager_resource.h"
#include "parameter_storage_manager/src/parameter_storage_manager_storage_adapter.h"
#include "parameter_storage_manager/src/parameter_storage_manager_storage_adapter_item_type.h"
#include "parameter_storage_manager/src/parameter_storage_manager_storage_adapter_settings.h"
#include "parameter_storage_manager/src/parameter_storage_manager_utility.h"

#include ESF_PARAMETER_STORAGE_MANAGER_POWER_MANAGER_FILE

// Structure used for processing to obtain the data size.
typedef struct EsfParameterStorageManagerGetSizeContext {
  // The data ID for which you want to get the data size.
  EsfParameterStorageManagerItemID id;

  // Storage information.
  EsfParameterStorageManagerStorageInfo storage_info;
} EsfParameterStorageManagerGetSizeContext;

typedef struct EsfParameterStorageManagerUpdateBeginContext {
  // work context.
  EsfParameterStorageManagerWorkContext* work;

  // The arguments of EsfParameterStorageManagerUpdateBegin().
  EsfParameterStorageManagerUpdateType type;
} EsfParameterStorageManagerUpdateBeginContext;

// """Initializes the internal resource and the storage area.

// Initializes the internal resource and the storage area.

// Args:
//     nothing.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusResourceExhausted: There are no unused
//     handles. kEsfParameterStorageManagerStatusInternal: Internal error.

// Note:
// """
static EsfParameterStorageManagerStatus EsfParameterStorageManagerInternalInit(
    void);

// """Deinitializes the internal resource and the storage area.

// Deinitializes the internal resource and the storage area.

// Args:
//     nothing.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInternal: Internal error.

// Note:
// """
static EsfParameterStorageManagerStatus
EsfParameterStorageManagerInternalDeinit(void);

// """EsfParameterStorageManagerResourceExclusiveContext is used to activate the
// handle.

// EsfParameterStorageManagerResourceExclusiveContext is used to activate the
// handle.

// Args:
//     [IN] context (EsfParameterStorageManagerResourceExclusiveContext*):
//         Exclusive control context. Cannot pass NULL to this argument.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusResourceExhausted: There are no unused
//     handles. kEsfParameterStorageManagerStatusInternal: Internal error.

// Note:
// """
static EsfParameterStorageManagerStatus
EsfParameterStorageManagerInternalNewHandle(
    EsfParameterStorageManagerResourceExclusiveContext* context);

// """Disable the handle using the
// SFParameterStorageManagerResourceExclusiveContext.

// Disable the handle using the
// SFParameterStorageManagerResourceExclusiveContext.

// Args:
//     [IN] context (EsfParameterStorageManagerResourceExclusiveContext*):
//         Exclusive control context. Cannot pass NULL to this argument.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusFailedPrecondition: The handle is in
//     use. kEsfParameterStorageManagerStatusNotFound: The resource not found.
//     kEsfParameterStorageManagerStatusInternal: Internal error.

// Note:
// """
static EsfParameterStorageManagerStatus
EsfParameterStorageManagerInternalDeleteHandle(
    EsfParameterStorageManagerResourceExclusiveContext* context);

// """Call the Factory Reset function using the
// SFParameterStorageManagerResourceExclusiveContext.

// Call the Factory Reset function using the
// SFParameterStorageManagerResourceExclusiveContext.

// Args:
//     [IN] context (EsfParameterStorageManagerResourceExclusiveContext*):
//         Exclusive control context. Cannot pass NULL to this argument.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusDataLoss: Failed to erase some or all
//     data. kEsfParameterStorageManagerStatusInternal: The argument context is
//     an invalid value.

// Note:
// """
static EsfParameterStorageManagerStatus
EsfParameterStorageManagerInternalInvokeFactoryReset(
    EsfParameterStorageManagerResourceExclusiveContext* context);

// """The target data will be restored to the factory settings.

// The target data will be restored to the factory settings.
// The flags defined in the constant table are referenced to determine whether
// they are applicable.

// Args:
//     nothing.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusDataLoss: Failed to erase some or all
//     data. kEsfParameterStorageManagerStatusInternal: The argument context is
//     an invalid value.

// Note:
// """
static EsfParameterStorageManagerStatus
EsfParameterStorageManagerInternalInvokeFactoryResetRequired(void);

// """Call the registered Factory Reset functions.

// Call the registered Factory Reset functions.
// The lock is acquired when the FactoryReset function is obtained, and released
// when the FactoryReset function is called.

// Args:
//     nothing.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusDataLoss: Failed to erase some or all
//     data. kEsfParameterStorageManagerStatusInternal: The argument context is
//     an invalid value.

// Note:
// """
static EsfParameterStorageManagerStatus
EsfParameterStorageManagerInternalInvokeRegisteredFactoryReset(void);

// """Judges whether the information in the data structure is set correctly.

// Judges whether the information in the data structure is set to a value that
// can be handled by the Parameter Storage Manager internal I/F.

// Args:
//     [IN] info (EsfParameterStorageManagerStructInfo*): Data structure
//     information.

// Returns:
//     bool: Returns true if the parameter is correctly set to the structure.

// Yields:
//     True: Parameters are correctly set to the structure.
//     False: Parameters are not correctly set to the structure.

// Note:
// """
static bool EsfParameterStorageManagerValidateStructInfo(
    const EsfParameterStorageManagerStructInfo* info);

// """Judges whether the member information in the data structure is set
// correctly.

// Judges whether the member information in the data structure is set to a value
// that can be handled by the Parameter Storage Manager internal I/F.

// Args:
//     [IN] info (const EsfParameterStorageManagerMemberInfo*): Data structure
//                  information. Cannot pass NULL to this argument.

// Returns:
//     bool: Returns true if the parameter is correctly set to the structure.

// Yields:
//     True: Parameters are correctly set to the structure.
//     False: Parameters are not correctly set to the structure.

// Note:
// """
static bool EsfParameterStorageManagerValidateStructMember(
    const EsfParameterStorageManagerMemberInfo* info);

// """Judges whether the accessor in the data structure is set correctly.

// Judges whether the accessor in the data structure is set to a value that
// can be handled by the Parameter Storage Manager internal I/F.

// Args:
//     [IN] info (const EsfParameterStorageManagerMemberInfo*): Data structure
//        information. Cannot pass NULL to this argument.

// Returns:
//     bool: Returns true if the parameter is correctly set to the structure.

// Yields:
//     True: Parameters are correctly set to the structure.
//     False: Parameters are not correctly set to the structure.

// Note:
// """
static bool EsfParameterStorageManagerValidateStructAccessor(
    const EsfParameterStorageManagerMemberInfo* info);

// """EsfParameterStorageManagerResourceExclusiveContext is used to add the
// handle reference count.

// EsfParameterStorageManagerResourceExclusiveContext is used to add the handle
// reference count.

// Args:
//     [IN/OUT] context (EsfParameterStorageManagerResourceExclusiveContext*):
//         Exclusive control context. Cannot pass NULL to this argument.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInternal: The argument context is an
//     invalid value.

// Note:
// """
static EsfParameterStorageManagerStatus
EsfParameterStorageManagerInternalReferenceHandle(
    EsfParameterStorageManagerResourceExclusiveContext* context);

// """Subtract the handle's reference count using
// SFParameterStorageManagerResourceExclusiveContext.

// Subtract the handle's reference count using
// SFParameterStorageManagerResourceExclusiveContext.

// Args:
//     [IN/OUT] context (EsfParameterStorageManagerResourceExclusiveContext*):
//         Exclusive control context. Cannot pass NULL to this argument.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInternal: The argument context is an
//     invalid value.

// Note:
// """
static EsfParameterStorageManagerStatus
EsfParameterStorageManagerInternalUnreferenceHandle(
    EsfParameterStorageManagerResourceExclusiveContext* context);

// """Controls the processing sequence of saving data to the data storage area.

// Controls the processing sequence of saving data to the data storage area.

// Args:
//     [IN/OUT] context (EsfParameterStorageManagerResourceExclusiveContext*):
//         Exclusive control context. Cannot pass NULL to this argument.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInternal: The argument info is an
//     invalid value. kEsfParameterStorageManagerStatusResourceExhausted: Failed
//     to allocate internal resources.
//     kEsfParameterStorageManagerStatusPermissionDenied: Could not restore the
//     data to the state
//                                 before saving.
//     kEsfParameterStorageManagerStatusDataLoss: Unable to access data storage
//     area. kEsfParameterStorageManagerStatusUnavailable: Failed to save data.
//     kEsfParameterStorageManagerStatusOutOfRange: The range of the data
//     storage area has been
//                           exceeded.
//     kEsfParameterStorageManagerStatusInternal: Parameter Storage Manager was
//     not initialized. kEsfParameterStorageManagerStatusInternal: Internal
//     processing failed.

// Note:
// """
static EsfParameterStorageManagerStatus
EsfParameterStorageManagerInternalSaveControl(
    EsfParameterStorageManagerResourceExclusiveContext* context);

// """Controls the processing sequence for loading data in the data storage
// area.

// Controls the processing sequence for loading data in the data storage area.

// Args:
//     [IN/OUT] context (EsfParameterStorageManagerResourceExclusiveContext*):
//         Exclusive control context. Cannot pass NULL to this argument.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInternal: The argument info is an
//     invalid value. kEsfParameterStorageManagerStatusResourceExhausted: Failed
//     to allocate internal resources.
//     kEsfParameterStorageManagerStatusDataLoss: Unable to access data storage
//     area. kEsfParameterStorageManagerStatusUnavailable: Failed to save data.
//     kEsfParameterStorageManagerStatusOutOfRange: The range of the data
//     storage area has been
//                           exceeded.
//     kEsfParameterStorageManagerStatusInternal: Parameter Storage Manager was
//     not initialized. kEsfParameterStorageManagerStatusInternal: Internal
//     processing failed.

// Note:
// """
static EsfParameterStorageManagerStatus
EsfParameterStorageManagerInternalLoadControl(
    EsfParameterStorageManagerResourceExclusiveContext* context);

// """Controls the processing sequence for clearing data in the data storage
// area.

// Controls the processing sequence for clearing data in the data storage area.

// Args:
//     [IN/OUT] context (EsfParameterStorageManagerResourceExclusiveContext*):
//         Exclusive control context. Cannot pass NULL to this argument.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInternal: The argument info is an
//     invalid value. kEsfParameterStorageManagerStatusResourceExhausted: Failed
//     to allocate internal resources.
//     kEsfParameterStorageManagerStatusPermissionDenied: Could not restore the
//     data to the state
//                                 before saving.
//     kEsfParameterStorageManagerStatusDataLoss: Unable to access data storage
//     area. kEsfParameterStorageManagerStatusUnavailable: Failed to save data.
//     kEsfParameterStorageManagerStatusInternal: Parameter Storage Manager was
//     not initialized. kEsfParameterStorageManagerStatusInternal: Internal
//     processing failed.

// Note:
// """
static EsfParameterStorageManagerStatus
EsfParameterStorageManagerInternalClearControl(
    EsfParameterStorageManagerResourceExclusiveContext* context);

// """Controls the processing sequence for obtaining the data size of the data
// storage area.

// Controls the processing sequence for obtaining the data size of the data
// storage area.

// Args:
//     [IN/OUT] context (EsfParameterStorageManagerResourceExclusiveContext*):
//         Exclusive control context. Cannot pass NULL to this argument.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInternal: The argument info is an
//     invalid value. kEsfParameterStorageManagerStatusDataLoss: Unable to
//     access data storage area.

// Note:
// """
static EsfParameterStorageManagerStatus
EsfParameterStorageManagerInternalGetSizeControl(
    EsfParameterStorageManagerResourceExclusiveContext* context);

// """Controls the processing sequence that initiates the update of the data
// store.

// Controls the processing sequence that initiates the update of the data store.

// Args:
//     [IN/OUT] context (EsfParameterStorageManagerResourceExclusiveContext*):
//         Exclusive control context. Cannot pass NULL to this argument.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusFailedPrecondition: The update has
//     already started. kEsfParameterStorageManagerStatusOutOfRange: There is a
//     lot of data to update. kEsfParameterStorageManagerStatusDataLoss: Unable
//     to access data storage area.
//     kEsfParameterStorageManagerStatusUnavailable: Failed to allocate
//     temporary data storage data.
//     kEsfParameterStorageManagerStatusPermissionDenied: There is no temporary
//     data storage area. kEsfParameterStorageManagerStatusInternal: Internal
//     error.

// Note:
// """
static EsfParameterStorageManagerStatus
EsfParameterStorageManagerInternalUpdateBeginControl(
    EsfParameterStorageManagerResourceExclusiveContext* context);

// """Controls the processing sequence that initiates a cancellable data update.

// Controls the processing sequence that initiates a cancellable data update.

// Args:
//     [IN/OUT] context (EsfParameterStorageManagerResourceExclusiveContext*):
//         Exclusive control context. Cannot pass NULL to this argument.
//     [IN/OUT] begin_context (EsfParameterStorageManagerUpdateBeginContext*):
//         UpdateBegin control context. Cannot pass NULL to this argument.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusFailedPrecondition: The update has
//     already started. kEsfParameterStorageManagerStatusOutOfRange: There is a
//     lot of data to update. kEsfParameterStorageManagerStatusDataLoss: Unable
//     to access data storage area.
//     kEsfParameterStorageManagerStatusUnavailable: Failed to allocate
//     temporary data storage data.
//     kEsfParameterStorageManagerStatusPermissionDenied: There is no temporary
//     data storage area. kEsfParameterStorageManagerStatusInternal: Internal
//     error.

// Note:
// """
static EsfParameterStorageManagerStatus
EsfParameterStorageManagerInternalUpdateBeginCancellableControl(
    EsfParameterStorageManagerResourceExclusiveContext* context,
    EsfParameterStorageManagerUpdateBeginContext* begin_context);

// """Controls the processing sequence that completes the update of the data
// store.

// Controls the processing sequence that completes the update of the data
// store.

// Args:
//     [IN/OUT] context (EsfParameterStorageManagerResourceExclusiveContext*):
//         Exclusive control context. Cannot pass NULL to this argument.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusFailedPrecondition: The update has
//     already started. kEsfParameterStorageManagerStatusDataLoss: Unable to
//     access data storage area. kEsfParameterStorageManagerStatusUnavailable:
//     Failed to switch temporary data storage area.
//     kEsfParameterStorageManagerStatusInternal: Parameter Storage Manager was
//     not initialized. kEsfParameterStorageManagerStatusInternal: Internal
//     processing failed.

// Note:
// """
static EsfParameterStorageManagerStatus
EsfParameterStorageManagerInternalUpdateCompleteControl(
    EsfParameterStorageManagerResourceExclusiveContext* context);

// """Controls the processing sequence for canceling updates to the data store.

// Controls the processing sequence for canceling updates to the data store.

// Args:
//     [IN/OUT] context (EsfParameterStorageManagerResourceExclusiveContext*):
//         Exclusive control context. Cannot pass NULL to this argument.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusFailedPrecondition: The update has
//     already started. kEsfParameterStorageManagerStatusDataLoss: Unable to
//     access data storage area. kEsfParameterStorageManagerStatusUnavailable:
//     Failed to discard temporary data storage area.
//     kEsfParameterStorageManagerStatusInternal: Parameter Storage Manager was
//     not initialized. kEsfParameterStorageManagerStatusInternal: Internal
//     processing failed.

// Note:
// """
static EsfParameterStorageManagerStatus
EsfParameterStorageManagerInternalUpdateCancelControl(
    EsfParameterStorageManagerResourceExclusiveContext* context);

// """Determines if a particular member of the structure is empty.

// Determines if a particular member of the structure is empty.
// If a particular member of a struct is empty, the return value is true,
// otherwise it returns false.
// Argument checking is already done in the entry function, so no argument
// checking is done within this function.

// Args:
//     [IN] data (uintptr_t): The particular member of the structure.
//     [IN] type (EsfParameterStorageManagerItemType): The type of the target
//              data member.

// Returns:
//     bool: If a particular member of a struct is empty, the return value is
//     true, otherwise it returns false.

// Note:
// """
static bool EsfParameterStorageManagerInternalIsDataEmpty(
    uintptr_t data, EsfParameterStorageManagerItemType type);

EsfParameterStorageManagerStatus EsfParameterStorageManagerInit(void) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;
  bool resource_locked = false;
  bool storage_locked = false;

  do {
    ret = EsfParameterStorageManagerResourceStorageLock(
        kEsfParameterStorageManagerWaitInfinity, NULL);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Storage lock failed. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
    storage_locked = true;
    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Storage locked");

    ret = EsfParameterStorageManagerResourceLock(
        kEsfParameterStorageManagerWaitInfinity, NULL);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Resource lock failed. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
    resource_locked = true;
    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Resource locked");

    ret = EsfParameterStorageManagerInternalInit();
    if (ret == kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_INFO("Initialization successful");
    } else {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Initialization failed. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INTERNAL_PROCESS_ERROR);
    }
  } while (0);

  if (resource_locked) {
    EsfParameterStorageManagerStatus lock_ret =
        EsfParameterStorageManagerResourceUnlock();
    if (lock_ret != kEsfParameterStorageManagerStatusOk) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Resource unlock failed. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
    } else {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE("Resource unlocked");
    }
  }

  if (storage_locked) {
    EsfParameterStorageManagerStatus lock_ret =
        EsfParameterStorageManagerResourceStorageUnlock();
    if (lock_ret != kEsfParameterStorageManagerStatusOk) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Storage unlock failed. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
    } else {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE("Storage unlocked");
    }
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus EsfParameterStorageManagerDeinit(void) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;
  bool resource_locked = false;
  bool storage_locked = false;

  do {
    ret = EsfParameterStorageManagerResourceStorageLock(
        kEsfParameterStorageManagerWaitInfinity, NULL);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Storage lock failed. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
    storage_locked = true;
    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Storage locked");

    ret = EsfParameterStorageManagerResourceLock(
        kEsfParameterStorageManagerWaitInfinity, NULL);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Resource lock failed. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
    resource_locked = true;
    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Resource locked");

    ret = EsfParameterStorageManagerInternalDeinit();
    if (ret == kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_INFO("Deinitialization successful");
    } else {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Deinitialization failed. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INTERNAL_PROCESS_ERROR);
    }
  } while (0);

  if (resource_locked) {
    EsfParameterStorageManagerStatus lock_ret =
        EsfParameterStorageManagerResourceUnlock();
    if (lock_ret != kEsfParameterStorageManagerStatusOk) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Resource unlock failed. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
    } else {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE("Resource unlocked");
    }
  }

  if (storage_locked) {
    EsfParameterStorageManagerStatus lock_ret =
        EsfParameterStorageManagerResourceStorageUnlock();
    if (lock_ret != kEsfParameterStorageManagerStatusOk) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Storage unlock failed. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
    } else {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE("Storage unlocked");
    }
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus EsfParameterStorageManagerOpen(
    EsfParameterStorageManagerHandle* handle) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;
  do {
    if (handle == NULL) {
      ret = kEsfParameterStorageManagerStatusInvalidArgument;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid handle pointer. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }

    EsfParameterStorageManagerResourceExclusiveContext context =
        ESF_PARAMETER_STORAGE_MANAGER_RESOURCE_EXCLUSIVE_CONTEXT_INITIALIZER;
    context.resource_entry = EsfParameterStorageManagerInternalNewHandle;
    ret = EsfParameterStorageManagerResourceExclusiveControl(&context);

    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "New handle acquisition failed. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }

    *handle = context.handle;
  } while (0);

  if (ret == kEsfParameterStorageManagerStatusOk) {
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Open successful. handle=%" PRId32,
                                        *handle);
  } else {
    ESF_PARAMETER_STORAGE_MANAGER_ERROR(
        "Open failed. ret=%u(%s)", ret,
        EsfParameterStorageManagerStrError(ret));
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus EsfParameterStorageManagerClose(
    EsfParameterStorageManagerHandle handle) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    if (handle == ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE) {
      ret = kEsfParameterStorageManagerStatusInvalidArgument;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid handle. handle=%" PRId32 ", ret=%u(%s)", handle, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Closing handle=%" PRId32, handle);

    EsfParameterStorageManagerResourceExclusiveContext context =
        ESF_PARAMETER_STORAGE_MANAGER_RESOURCE_EXCLUSIVE_CONTEXT_INITIALIZER;
    context.resource_entry = EsfParameterStorageManagerInternalDeleteHandle;
    context.handle = handle;
    ret = EsfParameterStorageManagerResourceExclusiveControl(&context);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Handle close failed. handle=%" PRId32 ", ret=%u(%s)", handle, ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
  } while (0);

  if (ret == kEsfParameterStorageManagerStatusOk) {
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Handle closed. handle=%" PRId32,
                                        handle);
  } else {
    ESF_PARAMETER_STORAGE_MANAGER_ERROR(
        "Handle close failed. handle=%" PRId32 ", ret=%u(%s)", handle, ret,
        EsfParameterStorageManagerStrError(ret));
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus EsfParameterStorageManagerInvokeFactoryReset(
    void) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  ESF_PARAMETER_STORAGE_MANAGER_INFO("Factory Reset started");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    EsfParameterStorageManagerResourceExclusiveContext context =
        ESF_PARAMETER_STORAGE_MANAGER_RESOURCE_EXCLUSIVE_CONTEXT_INITIALIZER;
    context.storage_func = EsfParameterStorageManagerInternalInvokeFactoryReset;

    ret = EsfParameterStorageManagerResourceExclusiveControl(&context);

    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Factory Reset failed. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_INFO("Factory Reset completed");
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus EsfParameterStorageManagerDowngrade(void) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  ESF_PARAMETER_STORAGE_MANAGER_INFO("Downgrade started");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;
  bool storage_locked = false;

  do {
    ret = EsfParameterStorageManagerResourceStorageLock(
        kEsfParameterStorageManagerWaitInfinity, NULL);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Storage lock failed. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INTERNAL_PROCESS_ERROR);
      break;
    }
    storage_locked = true;

    ret = EsfParameterStorageManagerStorageAdapterDowngrade();
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Downgrade failed. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_FACTORY_RESET_ERROR);
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_INFO("Downgrade completed");
  } while (0);

  if (storage_locked) {
    EsfParameterStorageManagerStatus lock_ret =
        EsfParameterStorageManagerResourceStorageUnlock();
    if (lock_ret != kEsfParameterStorageManagerStatusOk) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Storage unlock failed. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INTERNAL_PROCESS_ERROR);
    } else {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE("Storage unlocked");
    }
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %d(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static EsfParameterStorageManagerStatus EsfParameterStorageManagerInternalInit(
    void) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Initializing resources");
    ret = EsfParameterStorageManagerResourceInit();
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Resource initialization failed. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Resources initialized");

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Initializing storage adapter");
    ret = EsfParameterStorageManagerStorageAdapterInit();
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Storage adapter initialization failed. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Storage adapter initialized");
  } while (0);

  if (ret == kEsfParameterStorageManagerStatusOk) {
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Internal initialization completed");
  } else {
    ESF_PARAMETER_STORAGE_MANAGER_ERROR(
        "Internal initialization failed. ret=%u(%s)", ret,
        EsfParameterStorageManagerStrError(ret));
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static EsfParameterStorageManagerStatus
EsfParameterStorageManagerInternalDeinit(void) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Deinitializing storage adapter");
    ret = EsfParameterStorageManagerStorageAdapterDeinit();
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Storage adapter deinitialization failed. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Storage adapter deinitialized");

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Deinitializing resources");
    ret = EsfParameterStorageManagerResourceDeinit();
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Resource deinitialization failed. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Resources deinitialized");
  } while (0);

  if (ret == kEsfParameterStorageManagerStatusOk) {
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Internal deinitialization completed");
  } else {
    ESF_PARAMETER_STORAGE_MANAGER_ERROR(
        "Internal deinitialization failed. ret=%u(%s)", ret,
        EsfParameterStorageManagerStrError(ret));
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static EsfParameterStorageManagerStatus
EsfParameterStorageManagerInternalNewHandle(
    EsfParameterStorageManagerResourceExclusiveContext* context) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret =
      EsfParameterStorageManagerResourceNewHandle(&context->handle);

  if (ret == kEsfParameterStorageManagerStatusOk) {
    ESF_PARAMETER_STORAGE_MANAGER_TRACE("New handle created: %" PRId32,
                                        context->handle);
  } else {
    ESF_PARAMETER_STORAGE_MANAGER_ERROR(
        "New handle creation failed. ret=%u(%s)", ret,
        EsfParameterStorageManagerStrError(ret));
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static EsfParameterStorageManagerStatus
EsfParameterStorageManagerInternalDeleteHandle(
    EsfParameterStorageManagerResourceExclusiveContext* context) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret =
      EsfParameterStorageManagerResourceDeleteHandle(context->handle);

  if (ret == kEsfParameterStorageManagerStatusOk) {
    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Handle deleted: %" PRId32,
                                        context->handle);
  } else {
    ESF_PARAMETER_STORAGE_MANAGER_ERROR(
        "Handle deletion failed. handle=%" PRId32 ", ret=%u(%s)",
        context->handle, ret, EsfParameterStorageManagerStrError(ret));
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static EsfParameterStorageManagerStatus
EsfParameterStorageManagerInternalInvokeFactoryReset(
    EsfParameterStorageManagerResourceExclusiveContext* context) {
  (void)context;
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    if (!EsfParameterStorageManagerResourceIsInitialized()) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Resources not initialized. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INTERNAL_PROCESS_ERROR);
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Starting target data reset");
    ret = EsfParameterStorageManagerInternalInvokeFactoryResetRequired();
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Target data reset failed. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_FACTORY_RESET_ERROR);
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Target data reset completed");

    ret = EsfParameterStorageManagerStorageAdapterClean();
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to delete unmanaged item. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_FACTORY_RESET_ERROR);
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Invoking registered reset functions");
    ret = EsfParameterStorageManagerInternalInvokeRegisteredFactoryReset();
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Registered reset functions failed. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_FACTORY_RESET_ERROR);
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Registered reset functions completed");
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static EsfParameterStorageManagerStatus
EsfParameterStorageManagerInternalInvokeFactoryResetRequired(void) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  ESF_PARAMETER_STORAGE_MANAGER_INFO("Starting targeted data deletion");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  for (int i = 0; i < kEsfParameterStorageManagerItemCustom; ++i) {
    if (EsfParameterStorageManagerStorageAdapterConvertItemIDToFactoryResetRequired(  // NOLINT
            (EsfParameterStorageManagerItemID)i)) {
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Processing item ID %d", i);

      // Call Wdt Keep Alive
      EsfPwrMgrError pwr_ret = EsfPwrMgrWdtKeepAlive();
      if (pwr_ret != kEsfPwrMgrOk) {
        ESF_PARAMETER_STORAGE_MANAGER_WARN("EsfPwrMgrWdtKeepAlive error:%u",
                                           pwr_ret);
        // Continue processing
      }

      ret = EsfParameterStorageManagerStorageAdapterFactoryReset(
          (EsfParameterStorageManagerItemID)i);
      if (ret == kEsfParameterStorageManagerStatusDataLoss ||
          ret == kEsfParameterStorageManagerStatusInternal) {
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Fatal error during reset of ID %d. ret=%u(%s)", i, ret,
            EsfParameterStorageManagerStrError(ret));
        break;
      }
      if (ret != kEsfParameterStorageManagerStatusOk) {
        ESF_PARAMETER_STORAGE_MANAGER_WARN(
            "Non-fatal error during reset of ID %d. ret=%u(%s)", i, ret,
            EsfParameterStorageManagerStrError(ret));
        ret = kEsfParameterStorageManagerStatusOk;
      } else {
        ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Reset successful for ID %d", i);
      }
    }
  }
  if (ret == kEsfParameterStorageManagerStatusOk) {
    ESF_PARAMETER_STORAGE_MANAGER_INFO("Targeted data deletion completed");
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static EsfParameterStorageManagerStatus
EsfParameterStorageManagerInternalInvokeRegisteredFactoryReset(void) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  ESF_PARAMETER_STORAGE_MANAGER_INFO("Starting registered reset functions");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  for (int i = 0;
       i < CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_FACTORY_RESET_MAX &&
       ret == kEsfParameterStorageManagerStatusOk;
       ++i) {
    bool locked = false;
    do {
      ret = EsfParameterStorageManagerResourceLock(
          kEsfParameterStorageManagerWaitInfinity, NULL);
      if (ret != kEsfParameterStorageManagerStatusOk) {
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Resource lock failed for ID %d. ret=%u(%s)", i, ret,
            EsfParameterStorageManagerStrError(ret));
        break;
      }
      locked = true;
      ESF_PARAMETER_STORAGE_MANAGER_TRACE("Resource locked for ID %d", i);

      EsfParameterStorageManagerResourceFactoryReset factory_reset;
      ret =
          EsfParameterStorageManagerResourceGetFactoryReset(i, &factory_reset);
      if (ret != kEsfParameterStorageManagerStatusOk) {
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Reset data not found for ID %d. ret=%u(%s)", i, ret,
            EsfParameterStorageManagerStrError(ret));
        break;
      }

      if (factory_reset.func == NULL) {
        ret = kEsfParameterStorageManagerStatusOk;
        ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Reset ID %d not registered", i);
        break;
      }

      ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Executing reset for ID %d", i);
      ret = factory_reset.func(factory_reset.private_data);
      if (ret == kEsfParameterStorageManagerStatusDataLoss ||
          ret == kEsfParameterStorageManagerStatusInternal) {
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Fatal error in reset for ID %d. ret=%u(%s)", i, ret,
            EsfParameterStorageManagerStrError(ret));
        break;
      }
      if (ret != kEsfParameterStorageManagerStatusOk) {
        ESF_PARAMETER_STORAGE_MANAGER_WARN(
            "Non-fatal error in reset for ID %d. ret=%u(%s)", i, ret,
            EsfParameterStorageManagerStrError(ret));
        ret = kEsfParameterStorageManagerStatusOk;
      } else {
        ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Reset successful for ID %d", i);
      }
    } while (0);

    if (locked) {
      EsfParameterStorageManagerStatus unlock_ret =
          EsfParameterStorageManagerResourceUnlock();
      if (unlock_ret != kEsfParameterStorageManagerStatusOk) {
        ret = unlock_ret;
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Resource unlock failed for ID %d. ret=%u(%s)", i, ret,
            EsfParameterStorageManagerStrError(ret));
      } else {
        ESF_PARAMETER_STORAGE_MANAGER_TRACE("Resource unlocked for ID %d", i);
      }
    }
  }

  if (ret == kEsfParameterStorageManagerStatusOk) {
    ESF_PARAMETER_STORAGE_MANAGER_INFO("All reset functions executed");
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus EsfParameterStorageManagerSave(
    EsfParameterStorageManagerHandle handle,
    EsfParameterStorageManagerMask mask, EsfParameterStorageManagerData data,
    const EsfParameterStorageManagerStructInfo* info, void* private_data) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerWorkContext* work = NULL;
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    if (handle == ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE ||
        mask == ESF_PARAMETER_STORAGE_MANAGER_INVALID_MASK ||
        data == ESF_PARAMETER_STORAGE_MANAGER_INVALID_DATA) {
      ret = kEsfParameterStorageManagerStatusInvalidArgument;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid arguments. handle=%" PRId32 ", mask=%p, data=%p, ret=%u(%s)",
          handle, (void*)mask, (void*)data, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }
    if (!EsfParameterStorageManagerValidateStructInfo(info)) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid struct info. info=%p, ret=%u(%s)", (void*)info, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INTERNAL_PROCESS_ERROR);
      break;
    }

    EsfParameterStorageManagerResourceExclusiveContext context =
        ESF_PARAMETER_STORAGE_MANAGER_RESOURCE_EXCLUSIVE_CONTEXT_INITIALIZER;
    work = EsfParameterStorageManagerInternalAllocateWork(mask, data, info,
                                                          private_data);
    if (work == NULL) {
      ret = kEsfParameterStorageManagerStatusResourceExhausted;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Work memory allocation failed. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }

    int32_t valid_mask = EsfParameterStorageManagerInternalSetupWorkMask(work);
    if (valid_mask == 0) {
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG("No valid mask. Nothing to save.");
      break;
    }

    context.handle = handle;
    context.resource_entry = EsfParameterStorageManagerInternalReferenceHandle;
    context.storage_func = EsfParameterStorageManagerInternalSaveControl;
    context.resource_exit = EsfParameterStorageManagerInternalUnreferenceHandle;
    context.private_data = work;

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Save initiated. handle=%" PRId32
                                        ", valid_mask=%" PRId32,
                                        handle, valid_mask);
    ret = EsfParameterStorageManagerResourceExclusiveControl(&context);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Save failed. handle=%" PRId32 ", ret=%u(%s)", handle, ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Save completed. handle=%" PRId32,
                                        handle);
  } while (0);

  if (work != NULL) {
    EsfParameterStorageManagerInternalFreeWork(work);
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Work memory freed");
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus EsfParameterStorageManagerLoad(
    EsfParameterStorageManagerHandle handle,
    EsfParameterStorageManagerMask mask, EsfParameterStorageManagerData data,
    const EsfParameterStorageManagerStructInfo* info, void* private_data) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerWorkContext* work = NULL;
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    if (handle == ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE ||
        mask == ESF_PARAMETER_STORAGE_MANAGER_INVALID_MASK ||
        data == ESF_PARAMETER_STORAGE_MANAGER_INVALID_DATA) {
      ret = kEsfParameterStorageManagerStatusInvalidArgument;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid arguments. handle=%" PRId32 ", mask=%p, data=%p, ret=%u(%s)",
          handle, (void*)mask, (void*)data, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }
    if (!EsfParameterStorageManagerValidateStructInfo(info)) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid struct info. info=%p, ret=%u(%s)", (void*)info, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INTERNAL_PROCESS_ERROR);
      break;
    }

    EsfParameterStorageManagerResourceExclusiveContext context =
        ESF_PARAMETER_STORAGE_MANAGER_RESOURCE_EXCLUSIVE_CONTEXT_INITIALIZER;
    work = EsfParameterStorageManagerInternalAllocateWork(mask, data, info,
                                                          private_data);
    if (work == NULL) {
      ret = kEsfParameterStorageManagerStatusResourceExhausted;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Work memory allocation failed. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }

    int32_t valid_mask = EsfParameterStorageManagerInternalSetupWorkMask(work);
    if (valid_mask == 0) {
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG("No valid mask. Nothing to load.");
      break;
    }

    context.handle = handle;
    context.resource_entry = EsfParameterStorageManagerInternalReferenceHandle;
    context.storage_func = EsfParameterStorageManagerInternalLoadControl;
    context.resource_exit = EsfParameterStorageManagerInternalUnreferenceHandle;
    context.private_data = work;

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Load initiated. handle=%" PRId32
                                        ", valid_mask=%" PRId32,
                                        handle, valid_mask);
    ret = EsfParameterStorageManagerResourceExclusiveControl(&context);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Load failed. handle=%" PRId32 ", ret=%u(%s)", handle, ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Load completed. handle=%" PRId32,
                                        handle);
  } while (0);

  if (work != NULL) {
    EsfParameterStorageManagerInternalFreeWork(work);
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Work memory freed");
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus EsfParameterStorageManagerClear(
    EsfParameterStorageManagerHandle handle,
    EsfParameterStorageManagerMask mask,
    const EsfParameterStorageManagerStructInfo* info, void* private_data) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerWorkContext* work = NULL;
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    if (handle == ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE ||
        mask == ESF_PARAMETER_STORAGE_MANAGER_INVALID_MASK) {
      ret = kEsfParameterStorageManagerStatusInvalidArgument;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid arguments. handle=%" PRId32 ", mask=%p, ret=%u(%s)", handle,
          (void*)mask, ret, EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }
    if (!EsfParameterStorageManagerValidateStructInfo(info)) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid struct info. info=%p, ret=%u(%s)", (void*)info, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INTERNAL_PROCESS_ERROR);
      break;
    }

    EsfParameterStorageManagerResourceExclusiveContext context =
        ESF_PARAMETER_STORAGE_MANAGER_RESOURCE_EXCLUSIVE_CONTEXT_INITIALIZER;
    work = EsfParameterStorageManagerInternalAllocateWork(
        mask, ESF_PARAMETER_STORAGE_MANAGER_INVALID_DATA, info, private_data);
    if (work == NULL) {
      ret = kEsfParameterStorageManagerStatusResourceExhausted;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Work memory allocation failed. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }

    int32_t valid_mask = EsfParameterStorageManagerInternalSetupWorkMask(work);
    if (valid_mask == 0) {
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG("No valid mask. Nothing to clear.");
      break;
    }

    context.handle = handle;
    context.resource_entry = EsfParameterStorageManagerInternalReferenceHandle;
    context.storage_func = EsfParameterStorageManagerInternalClearControl;
    context.resource_exit = EsfParameterStorageManagerInternalUnreferenceHandle;
    context.private_data = work;

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Clear initiated. handle=%" PRId32
                                        ", valid_mask=%" PRId32,
                                        handle, valid_mask);
    ret = EsfParameterStorageManagerResourceExclusiveControl(&context);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Clear failed. handle=%" PRId32 ", ret=%u(%s)", handle, ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Clear completed. handle=%" PRId32,
                                        handle);
  } while (0);

  if (work != NULL) {
    EsfParameterStorageManagerInternalFreeWork(work);
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Work memory freed");
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus EsfParameterStorageManagerGetSize(
    EsfParameterStorageManagerHandle handle,
    EsfParameterStorageManagerItemID id, uint32_t* loadable_size) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    if (id < 0 || kEsfParameterStorageManagerItemCustom <= id ||
        loadable_size == NULL) {
      ret = kEsfParameterStorageManagerStatusInvalidArgument;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid arguments. handle=%" PRId32
          ", id=%u, loadable_size=%p, ret=%u(%s)",
          handle, id, (void*)loadable_size, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }

    EsfParameterStorageManagerResourceExclusiveContext context =
        ESF_PARAMETER_STORAGE_MANAGER_RESOURCE_EXCLUSIVE_CONTEXT_INITIALIZER;
    EsfParameterStorageManagerGetSizeContext get_size_context = {
        .id = id,
        .storage_info = {
                .capabilities = {.read_only = 0, .enable_offset = 0},
                .written_size = 0u,
            },
    };
    context.handle = handle;
    context.storage_func = EsfParameterStorageManagerInternalGetSizeControl;
    context.private_data = &get_size_context;

    if (context.handle != ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE) {
      context.resource_entry =
          EsfParameterStorageManagerInternalReferenceHandle;
      context.resource_exit =
          EsfParameterStorageManagerInternalUnreferenceHandle;
    }

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Get size initiated. handle=%" PRId32 ", id=%u", handle, id);
    ret = EsfParameterStorageManagerResourceExclusiveControl(&context);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Get size failed. handle=%" PRId32 ", id=%u, ret=%u(%s)", handle, id,
          ret, EsfParameterStorageManagerStrError(ret));
      break;
    }

    *loadable_size = get_size_context.storage_info.written_size;
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Size retrieved. handle=%" PRId32
                                        ", id=%u, size=%" PRIu32,
                                        handle, id, *loadable_size);
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus EsfParameterStorageManagerLock(void) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;
  do {
    struct timespec timeout;
    if (EsfParameterStorageManagerCreateTimeOut(&timeout)) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to create timeout. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Timeout created successfully");

    ret = EsfParameterStorageManagerResourceStorageLock(
        kEsfParameterStorageManagerWaitTimeOut, &timeout);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Lock acquisition failed. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Lock acquired successfully");
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus EsfParameterStorageManagerUnlock(void) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");

  EsfParameterStorageManagerStatus ret =
      EsfParameterStorageManagerResourceStorageUnlock();

  if (ret != kEsfParameterStorageManagerStatusOk) {
    ESF_PARAMETER_STORAGE_MANAGER_ERROR(
        "Unlock operation failed. ret=%u(%s)", ret,
        EsfParameterStorageManagerStrError(ret));
  } else {
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Unlock operation completed successfully");
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus EsfParameterStorageManagerRegisterFactoryReset(
    EsfParameterStorageManagerRegisterFactoryResetType func, void* private_data,
    EsfParameterStorageManagerFactoryResetID* id) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;
  bool locked = false;

  do {
    if (func == NULL || id == NULL) {
      ret = kEsfParameterStorageManagerStatusInvalidArgument;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid arguments: func=%p, id=%p. ret=%u(%s)", (void*)func,
          (void*)id, ret, EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }

    ret = EsfParameterStorageManagerResourceLock(
        kEsfParameterStorageManagerWaitInfinity, NULL);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Resource lock acquisition failed. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
    locked = true;
    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Resource lock acquired successfully");

    ret = EsfParameterStorageManagerResourceNewFactoryReset(func, private_data,
                                                            id);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Factory reset function registration failed. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Factory reset function registered successfully. id=%d", *id);
  } while (0);

  if (locked) {
    EsfParameterStorageManagerStatus unlock_ret =
        EsfParameterStorageManagerResourceUnlock();
    if (unlock_ret != kEsfParameterStorageManagerStatusOk) {
      ret = unlock_ret;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Resource lock release failed. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
    } else {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE(
          "Resource lock released successfully");
    }
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerUnregisterFactoryReset(
    EsfParameterStorageManagerFactoryResetID id) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;
  bool locked = false;

  do {
    if (id == ESF_PARAMETER_STORAGE_MANAGER_INVALID_FACTORY_RESET_ID) {
      ret = kEsfParameterStorageManagerStatusInvalidArgument;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid factory reset ID. id=%" PRId32 ", ret=%u(%s)", id, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }

    ret = EsfParameterStorageManagerResourceLock(
        kEsfParameterStorageManagerWaitInfinity, NULL);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Resource lock acquisition failed. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
    locked = true;
    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Resource lock acquired successfully");

    ret = EsfParameterStorageManagerResourceDeleteFactoryReset(id);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Factory reset function unregistration failed. id=%" PRId32
          ", ret=%u(%s)",
          id, ret, EsfParameterStorageManagerStrError(ret));
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Factory reset function unregistered successfully. id=%d", id);
  } while (0);

  if (locked) {
    EsfParameterStorageManagerStatus unlock_ret =
        EsfParameterStorageManagerResourceUnlock();
    if (unlock_ret != kEsfParameterStorageManagerStatusOk) {
      ret = unlock_ret;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Resource lock release failed. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
    } else {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE(
          "Resource lock released successfully");
    }
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus EsfParameterStorageManagerUpdateBegin(
    EsfParameterStorageManagerHandle handle,
    EsfParameterStorageManagerMask mask,
    const EsfParameterStorageManagerStructInfo* info,
    EsfParameterStorageManagerUpdateType type) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerWorkContext* work = NULL;
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    if (handle == ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE ||
        mask == ESF_PARAMETER_STORAGE_MANAGER_INVALID_MASK || type < 0 ||
        type >= kEsfParameterStorageManagerUpdateTypeMax) {
      ret = kEsfParameterStorageManagerStatusInvalidArgument;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR("Invalid arguments: handle=%" PRId32
                                          ", mask=%" PRIuPTR ", type=%u",
                                          handle, mask, type);
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }
    if (!EsfParameterStorageManagerValidateStructInfo(info)) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid struct info. info=%p, ret=%u(%s)", (void*)info, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INTERNAL_PROCESS_ERROR);
      break;
    }

    work = EsfParameterStorageManagerInternalAllocateWork(
        mask, ESF_PARAMETER_STORAGE_MANAGER_INVALID_DATA, info, NULL);
    if (work == NULL) {
      ret = kEsfParameterStorageManagerStatusResourceExhausted;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR("Failed to allocate working memory");
      break;
    }

    int32_t enabled_mask_num =
        EsfParameterStorageManagerInternalSetupWorkMask(work);
    if (enabled_mask_num == 0) {
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG("No valid mask found");
      break;
    }

    ret = EsfParameterStorageManagerInternalUpdateBeginIsValid(
        work, enabled_mask_num);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid Update begin. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }

    EsfParameterStorageManagerUpdateBeginContext update_begin_context = {
        .work = work,
        .type = type,
    };
    EsfParameterStorageManagerResourceExclusiveContext context = {
        .handle = handle,
        .resource_entry = EsfParameterStorageManagerInternalReferenceHandle,
        .storage_func = EsfParameterStorageManagerInternalUpdateBeginControl,
        .resource_exit = EsfParameterStorageManagerInternalUnreferenceHandle,
        .private_data = &update_begin_context,
    };

    ret = EsfParameterStorageManagerResourceExclusiveControl(&context);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR("Update begin process failed");
      break;
    }
  } while (0);

  if (work != NULL) {
    EsfParameterStorageManagerInternalFreeWork(work);
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus EsfParameterStorageManagerUpdateComplete(
    EsfParameterStorageManagerHandle handle) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    if (handle == ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE) {
      ret = kEsfParameterStorageManagerStatusInvalidArgument;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid handle provided. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }

    EsfParameterStorageManagerCapabilities capabilities;
    ret =
        EsfParameterStorageManagerStorageAdapterGetCapabilities(&capabilities);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to get capabilities. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }

    if (capabilities.cancellable == 0) {
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Cancel API is not available.");
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_TRACE(
        "Preparing for update complete process");

    EsfParameterStorageManagerResourceExclusiveContext context =
        ESF_PARAMETER_STORAGE_MANAGER_RESOURCE_EXCLUSIVE_CONTEXT_INITIALIZER;
    context.handle = handle;
    context.resource_entry = EsfParameterStorageManagerInternalReferenceHandle;
    context.storage_func =
        EsfParameterStorageManagerInternalUpdateCompleteControl;
    context.resource_exit = EsfParameterStorageManagerInternalUnreferenceHandle;
    context.private_data = NULL;

    ret = EsfParameterStorageManagerResourceExclusiveControl(&context);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Update complete process failed. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Update complete process succeeded");
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus EsfParameterStorageManagerUpdateCancel(
    EsfParameterStorageManagerHandle handle) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    if (handle == ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE) {
      ret = kEsfParameterStorageManagerStatusInvalidArgument;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid handle provided. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }

    EsfParameterStorageManagerCapabilities capabilities;
    ret =
        EsfParameterStorageManagerStorageAdapterGetCapabilities(&capabilities);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to get capabilities. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }

    if (capabilities.cancellable == 0) {
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Cancel API is not available.");
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Preparing for update cancel process");

    EsfParameterStorageManagerResourceExclusiveContext context =
        ESF_PARAMETER_STORAGE_MANAGER_RESOURCE_EXCLUSIVE_CONTEXT_INITIALIZER;
    context.handle = handle;
    context.resource_entry = EsfParameterStorageManagerInternalReferenceHandle;
    context.storage_func =
        EsfParameterStorageManagerInternalUpdateCancelControl;
    context.resource_exit = EsfParameterStorageManagerInternalUnreferenceHandle;
    context.private_data = NULL;

    ret = EsfParameterStorageManagerResourceExclusiveControl(&context);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Update cancel process failed. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Update cancel process succeeded");
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

bool EsfParameterStorageManagerIsDataEmpty(
    EsfParameterStorageManagerData data,
    const EsfParameterStorageManagerStructInfo* info, size_t index) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  bool ret = false;
  do {
    if (data == ESF_PARAMETER_STORAGE_MANAGER_INVALID_DATA) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR("Invalid data provided.");
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }
    if (!EsfParameterStorageManagerValidateStructInfo(info)) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR("Invalid struct info provided.");
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }
    if (info->items_num <= index) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Index %zu out of range. items_num: %zu", index, info->items_num);
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_TRACE(
        "Checking if data is empty for index %zu", index);
    ret = EsfParameterStorageManagerInternalIsDataEmpty(
        data + info->items[index].offset, info->items[index].type);

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Data empty check result: %s",
                                        ret ? "true" : "false");
  } while (0);
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %d", ret);
  return ret;
}

EsfParameterStorageManagerStatus EsfParameterStorageManagerGetCapabilities(
    EsfParameterStorageManagerCapabilities* capabilities) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    if (capabilities == NULL) {
      ret = kEsfParameterStorageManagerStatusInvalidArgument;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Null pointer provided for capabilities. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_TRACE(
        "Retrieving storage adapter capabilities");
    ret = EsfParameterStorageManagerStorageAdapterGetCapabilities(capabilities);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to get storage adapter capabilities. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Successfully retrieved capabilities");
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus EsfParameterStorageManagerGetItemCapabilities(
    EsfParameterStorageManagerItemID id,
    EsfParameterStorageManagerItemCapabilities* capabilities) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    if (id < 0 || kEsfParameterStorageManagerItemCustom <= id) {
      ret = kEsfParameterStorageManagerStatusInvalidArgument;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid item ID provided. id=%u, ret=%u(%s)", id, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }

    if (capabilities == NULL) {
      ret = kEsfParameterStorageManagerStatusInvalidArgument;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Null pointer provided for capabilities. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_TRACE(
        "Retrieving item capabilities for ID %u", id);
    ret = EsfParameterStorageManagerStorageAdapterGetItemCapabilities(
        id, capabilities);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to get item capabilities for ID %u. ret=%u(%s)", id, ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Successfully retrieved item capabilities for ID %u", id);
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static bool EsfParameterStorageManagerValidateStructInfo(
    const EsfParameterStorageManagerStructInfo* info) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  bool ret = true;

  do {
    if (info == NULL) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Null pointer provided for struct info.");
      ret = false;
      break;
    }

    if (info->items_num == 0 || info->items_num >= UINT32_MAX) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR("Invalid items_num: %zu",
                                          info->items_num);
      ret = false;
      break;
    }

    if (info->items == NULL) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR("Null pointer provided for items.");
      ret = false;
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Validating %zu struct members",
                                        info->items_num);

    for (size_t i = 0; i < info->items_num; ++i) {
      ret = EsfParameterStorageManagerValidateStructMember(&info->items[i]);
      if (ret == false) {
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Invalid member information at index %zu", i);
        break;
      }
    }

    if (ret) {
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
          "All struct members validated successfully");
    }
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %d", ret);
  return ret;
}

static bool EsfParameterStorageManagerValidateStructMember(
    const EsfParameterStorageManagerMemberInfo* info) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  bool ret = true;

  do {
    if (info == NULL) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Null pointer provided for member info.");
      ret = false;
      break;
    }

    if (info->id < 0 || kEsfParameterStorageManagerItemMax <= info->id) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR("Invalid item id: %u", info->id);
      ret = false;
      break;
    }

    if (info->type < 0 ||
        kEsfParameterStorageManagerItemTypeMax <= info->type) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR("Invalid type id: %u", info->type);
      ret = false;
      break;
    }

    if (info->enabled == NULL) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Null pointer provided for 'enabled' member.");
      ret = false;
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_TRACE(
        "Validating struct accessor for item id: %u", info->id);
    ret = EsfParameterStorageManagerValidateStructAccessor(info);
    if (ret == false) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to validate struct accessor for item id: %u", info->id);
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_TRACE(
        "Successfully validated struct member. id: %u, type: %u", info->id,
        info->type);
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %d", ret);
  return ret;
}

static bool EsfParameterStorageManagerValidateStructAccessor(
    const EsfParameterStorageManagerMemberInfo* info) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  bool ret = true;

  do {
    if (info == NULL) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Null pointer provided for member info.");
      ret = false;
      break;
    }

    if (info->id != kEsfParameterStorageManagerItemCustom) {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE(
          "Not a custom item, skipping custom accessor validation.");
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Validating custom accessor functions");

    if (info->custom == NULL) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Null pointer provided for custom accessor.");
      ret = false;
      break;
    }

    if (info->custom->save == NULL) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Null pointer provided for custom save function.");
      ret = false;
      break;
    }

    if (info->custom->load == NULL) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Null pointer provided for custom load function.");
      ret = false;
      break;
    }

    if (info->custom->clear == NULL) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Null pointer provided for custom clear function.");
      ret = false;
      break;
    }

    if (info->custom->cancel == NULL) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Null pointer provided for custom cancel function.");
      ret = false;
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_TRACE(
        "Custom accessor functions validated successfully.");
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %d", ret);
  return ret;
}

static EsfParameterStorageManagerStatus
EsfParameterStorageManagerInternalReferenceHandle(
    EsfParameterStorageManagerResourceExclusiveContext* context) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");

  if (context == NULL) {
    ESF_PARAMETER_STORAGE_MANAGER_ERROR("Null pointer provided for context.");
    return kEsfParameterStorageManagerStatusInternal;
  }

  EsfParameterStorageManagerStatus ret =
      EsfParameterStorageManagerResourceReferenceHandle(context->handle);
  if (ret != kEsfParameterStorageManagerStatusOk) {
    ESF_PARAMETER_STORAGE_MANAGER_ERROR(
        "Failed to get a handle reference. ret=%u(%s)", ret,
        EsfParameterStorageManagerStrError(ret));
  } else {
    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Successfully referenced handle");
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static EsfParameterStorageManagerStatus
EsfParameterStorageManagerInternalUnreferenceHandle(
    EsfParameterStorageManagerResourceExclusiveContext* context) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");

  if (context == NULL) {
    ESF_PARAMETER_STORAGE_MANAGER_ERROR("Null pointer provided for context.");
    return kEsfParameterStorageManagerStatusInternal;
  }

  EsfParameterStorageManagerStatus ret =
      EsfParameterStorageManagerResourceUnreferenceHandle(context->handle);
  if (ret != kEsfParameterStorageManagerStatusOk) {
    ESF_PARAMETER_STORAGE_MANAGER_ERROR(
        "Failed to release a reference to a handle. ret=%u(%s)", ret,
        EsfParameterStorageManagerStrError(ret));
  } else {
    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Successfully unreferenced handle");
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static EsfParameterStorageManagerStatus
EsfParameterStorageManagerInternalSaveControl(
    EsfParameterStorageManagerResourceExclusiveContext* context) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    if (context == NULL || context->private_data == NULL) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Null pointer provided for context or private_data.");
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }

    EsfParameterStorageManagerWorkContext* work =
        (EsfParameterStorageManagerWorkContext*)context->private_data;

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Getting work storage info");
    ret = EsfParameterStorageManagerInternalGetWorkStorageInfo(context->handle,
                                                               work);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to get storage info. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }

    // Save Loop
    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Starting save loop");
    EsfParameterStorageManagerInternalWorkBegin(work);
    while (EsfParameterStorageManagerInternalWorkNext(work)) {
      if (!work->member_data[work->index].enabled) {
        ESF_PARAMETER_STORAGE_MANAGER_TRACE(
            "Skipping disabled member at index %" PRIu32, work->index);
        continue;
      }
      if (work->member_info->id == kEsfParameterStorageManagerItemCustom) {
        ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
            "Saving custom data at index %" PRIu32, work->index);
        ret = work->member_info->custom->save(
            (const void*)(work->data + work->member_info->offset),
            work->private_data);
        if (ret != kEsfParameterStorageManagerStatusOk) {
          ESF_PARAMETER_STORAGE_MANAGER_ERROR(
              "Failed to save custom data. i=%" PRIu32 ", ret=%u(%s)",
              work->index, ret, EsfParameterStorageManagerStrError(ret));
          break;
        }
        continue;
      }
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
          "Saving supported data at index %" PRIu32, work->index);
      ret = EsfParameterStorageManagerInternalSupportedSave(work);
      if (ret != kEsfParameterStorageManagerStatusOk) {
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Failed to save supported data. i=%" PRIu32 ", ret=%u(%s)",
            work->index, ret, EsfParameterStorageManagerStrError(ret));
        break;
      }
    }

    if (ret == kEsfParameterStorageManagerStatusOk ||
        ret == kEsfParameterStorageManagerStatusDataLoss) {
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
          "Save process completed with status: %u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }

    // Reverts the changed data.
    ESF_PARAMETER_STORAGE_MANAGER_WARN(
        "Save failed, attempting to revert changes");
    EsfParameterStorageManagerStatus revert_ret =
        EsfParameterStorageManagerInternalCancel(work);
    if (revert_ret != kEsfParameterStorageManagerStatusOk) {
      ret = kEsfParameterStorageManagerStatusDataLoss;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to cancel data update. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_FLASH_CONTROL_FAILURE);
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Successfully reverted changes");
    ESF_PARAMETER_STORAGE_MANAGER_EVENT_WARN(
        ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_ROLLBACK_FAILURE);
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static EsfParameterStorageManagerStatus
EsfParameterStorageManagerInternalLoadControl(
    EsfParameterStorageManagerResourceExclusiveContext* context) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    if (context == NULL || context->private_data == NULL) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Null pointer provided for context or private_data.");
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }

    EsfParameterStorageManagerWorkContext* work =
        (EsfParameterStorageManagerWorkContext*)context->private_data;

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Getting work storage info");
    ret = EsfParameterStorageManagerInternalGetWorkStorageInfo(context->handle,
                                                               work);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to get storage info. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }

    // Load Loop
    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Starting load loop");
    EsfParameterStorageManagerInternalWorkBegin(work);
    while (EsfParameterStorageManagerInternalWorkNext(work)) {
      if (!work->member_data[work->index].enabled) {
        ESF_PARAMETER_STORAGE_MANAGER_TRACE(
            "Skipping disabled member at index %" PRIu32, work->index);
        continue;
      }
      if (work->member_info->id == kEsfParameterStorageManagerItemCustom) {
        ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
            "Loading custom data at index %" PRIu32, work->index);
        ret = work->member_info->custom->load(
            (void*)(work->data + work->member_info->offset),
            work->private_data);
        if (ret != kEsfParameterStorageManagerStatusOk) {
          ESF_PARAMETER_STORAGE_MANAGER_ERROR(
              "Failed to load custom data. i=%" PRIu32 ", ret=%u(%s)",
              work->index, ret, EsfParameterStorageManagerStrError(ret));
          break;
        }
        continue;
      }
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
          "Loading supported data at index %" PRIu32 ", item_id=%u",
          work->index, work->member_info->id);
      ret = EsfParameterStorageManagerStorageAdapterLoadItemType(
          work->member_info->type,
          (void*)(work->data + work->member_info->offset), work);
      if (ret != kEsfParameterStorageManagerStatusOk) {
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Failed to load data. i=%" PRIu32 ", item_id=%u, ret=%u(%s)",
            work->index, work->member_info->id, ret,
            EsfParameterStorageManagerStrError(ret));
        break;
      }
    }

    if (ret == kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
          "Load process completed successfully");
    }
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static EsfParameterStorageManagerStatus
EsfParameterStorageManagerInternalClearControl(
    EsfParameterStorageManagerResourceExclusiveContext* context) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    if (context == NULL || context->private_data == NULL) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Null pointer provided for context or private_data.");
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }

    EsfParameterStorageManagerWorkContext* work =
        (EsfParameterStorageManagerWorkContext*)context->private_data;

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Getting work storage info");
    ret = EsfParameterStorageManagerInternalGetWorkStorageInfo(context->handle,
                                                               work);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to get storage info. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }

    // Clear Loop
    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Starting clear loop");
    EsfParameterStorageManagerInternalWorkBegin(work);
    while (EsfParameterStorageManagerInternalWorkNext(work)) {
      if (!work->member_data[work->index].enabled) {
        ESF_PARAMETER_STORAGE_MANAGER_TRACE(
            "Skipping disabled member at index %" PRIu32, work->index);
        continue;
      }
      if (work->member_info->id == kEsfParameterStorageManagerItemCustom) {
        ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
            "Clearing custom data at index %" PRIu32, work->index);
        ret = work->member_info->custom->clear(work->private_data);
        if (ret != kEsfParameterStorageManagerStatusOk) {
          ESF_PARAMETER_STORAGE_MANAGER_ERROR(
              "Failed to clear custom data. i=%" PRIu32 ", ret=%u(%s)",
              work->index, ret, EsfParameterStorageManagerStrError(ret));
          break;
        }
        continue;
      }

      ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
          "Clearing supported data at index %" PRIu32, work->index);
      ret = EsfParameterStorageManagerInternalSupportedClear(work);
      if (ret != kEsfParameterStorageManagerStatusOk) {
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Failed to clear supported data. i=%" PRIu32 ", ret=%u(%s)",
            work->index, ret, EsfParameterStorageManagerStrError(ret));
        break;
      }
    }

    if (ret == kEsfParameterStorageManagerStatusOk ||
        ret == kEsfParameterStorageManagerStatusDataLoss) {
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
          "Clear process completed with status: %u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }

    // Reverts the changed data.
    ESF_PARAMETER_STORAGE_MANAGER_WARN(
        "Clear failed, attempting to revert changes");
    EsfParameterStorageManagerStatus revert_ret =
        EsfParameterStorageManagerInternalCancel(work);
    if (revert_ret != kEsfParameterStorageManagerStatusOk) {
      ret = kEsfParameterStorageManagerStatusDataLoss;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to cancel data update. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_FLASH_CONTROL_FAILURE);
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Successfully reverted changes");
    ESF_PARAMETER_STORAGE_MANAGER_EVENT_WARN(
        ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_ROLLBACK_FAILURE);
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static EsfParameterStorageManagerStatus
EsfParameterStorageManagerInternalGetSizeControl(
    EsfParameterStorageManagerResourceExclusiveContext* context) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;
  EsfParameterStorageManagerWorkMemberData* member_data = NULL;

  do {
    if (context == NULL || context->private_data == NULL) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Null pointer provided for context or private_data.");
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }

    EsfParameterStorageManagerGetSizeContext* work =
        (EsfParameterStorageManagerGetSizeContext*)context->private_data;

    // If the handle is not invalid, generate member information.
    if (context->handle != ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE) {
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
          "Generating member information for valid handle");
      EsfParameterStorageManagerResourceUpdateInfo info;
      ret = EsfParameterStorageManagerResourceGetUpdateDataFromHandle(
          context->handle, &info);
      if (ret != kEsfParameterStorageManagerStatusOk) {
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Failed to get update information. ret=%u(%s)", ret,
            EsfParameterStorageManagerStrError(ret));
        break;
      }

      // setup member information
      if (0 < info.count) {
        ESF_PARAMETER_STORAGE_MANAGER_TRACE(
            "Searching for matching ID in %d items", info.count);
        for (int i = 0; i < info.count; ++i) {
          if (info.id[i] == work->id) {
            ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Found matching ID: %u",
                                                work->id);
            member_data =
                EsfParameterStorageManagerInternalAllocateMemberData(1);
            if (member_data == NULL) {
              ret = kEsfParameterStorageManagerStatusResourceExhausted;
              ESF_PARAMETER_STORAGE_MANAGER_ERROR(
                  "Failed to allocate work memory. ret=%u(%s)", ret,
                  EsfParameterStorageManagerStrError(ret));
              break;
            }
            member_data[0].update = true;
            member_data[0].update_data = info.data[i];
            break;
          }
        }
      } else {
        ESF_PARAMETER_STORAGE_MANAGER_DEBUG("No items to search");
      }
      if (ret != kEsfParameterStorageManagerStatusOk) {
        break;
      }
    } else {
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
          "Invalid handle, skipping member information generation");
    }

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Getting storage info for ID: %u",
                                        work->id);
    ret = EsfParameterStorageManagerStorageAdapterGetStorageInfo(
        work->id, member_data, &work->storage_info);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to get storage information. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Successfully retrieved storage info");
  } while (0);

  if (member_data != NULL) {
    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Freeing member data");
    EsfParameterStorageManagerInternalFreeMemberData(member_data);
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static EsfParameterStorageManagerStatus
EsfParameterStorageManagerInternalUpdateBeginControl(
    EsfParameterStorageManagerResourceExclusiveContext* context) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    if (context == NULL || context->private_data == NULL) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Null pointer provided for context or private_data.");
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }

    EsfParameterStorageManagerUpdateBeginContext* begin_context =
        (EsfParameterStorageManagerUpdateBeginContext*)context->private_data;
    EsfParameterStorageManagerWorkContext* work = begin_context->work;

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Getting capabilities");
    EsfParameterStorageManagerCapabilities capabilities;
    ret =
        EsfParameterStorageManagerStorageAdapterGetCapabilities(&capabilities);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to get capabilities. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }

    if (capabilities.cancellable == 0) {
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Cancel API is not available.");
      if (begin_context->type != kEsfParameterStorageManagerUpdateEmpty) {
        ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
            "Update type is not empty, skipping clear operation");
        break;
      }

      ESF_PARAMETER_STORAGE_MANAGER_TRACE(
          "Starting clear operation for non-cancellable update");
      EsfParameterStorageManagerInternalWorkBegin(work);
      while (EsfParameterStorageManagerInternalWorkNext(work)) {
        if (!work->member_data[work->index].enabled) {
          ESF_PARAMETER_STORAGE_MANAGER_TRACE(
              "Skipping disabled member at index %" PRIu32, work->index);
          continue;
        }
        ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Clearing item at index %" PRIu32
                                            ", item_id=%u",
                                            work->index, work->member_info->id);
        ret = EsfParameterStorageManagerStorageAdapterClear(
            work->member_info->id, &work->member_data[work->index]);
        if (ret != kEsfParameterStorageManagerStatusOk) {
          ESF_PARAMETER_STORAGE_MANAGER_ERROR(
              "Failed to clear. i=%" PRIu32 ", item_id=%u, ret=%u(%s)",
              work->index, work->member_info->id, ret,
              EsfParameterStorageManagerStrError(ret));
          break;
        }
      }
      if (ret == kEsfParameterStorageManagerStatusOk) {
        ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
            "Clear operation completed successfully");
      }
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Initiating cancellable update begin control");
    ret = EsfParameterStorageManagerInternalUpdateBeginCancellableControl(
        context, begin_context);
    if (ret == kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
          "Cancellable update begin control completed successfully");
    } else {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Cancellable update begin control failed. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
    }
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static EsfParameterStorageManagerStatus
EsfParameterStorageManagerInternalUpdateBeginCancellableControl(
    EsfParameterStorageManagerResourceExclusiveContext* context,
    EsfParameterStorageManagerUpdateBeginContext* begin_context) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    if (context == NULL || begin_context == NULL ||
        begin_context->work == NULL) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Null pointer provided for context, begin_context, or work.");
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }

    EsfParameterStorageManagerWorkContext* work = begin_context->work;

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Checking if update has already begun");
    ret = EsfParameterStorageManagerInternalAlreadyUpdateBegin(context->handle,
                                                               work);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Unable to start updating data. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Getting work storage info");
    ret = EsfParameterStorageManagerInternalGetWorkStorageInfo(context->handle,
                                                               work);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to get storage info. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }

    // Update begin Loop
    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Starting update begin loop");
    EsfParameterStorageManagerInternalWorkBegin(work);
    while (EsfParameterStorageManagerInternalWorkNext(work)) {
      if (!work->member_data[work->index].enabled) {
        ESF_PARAMETER_STORAGE_MANAGER_TRACE(
            "Skipping disabled member at index %" PRIu32, work->index);
        continue;
      }
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Updating item at index %" PRIu32
                                          ", item_id=%u",
                                          work->index, work->member_info->id);
      ret = EsfParameterStorageManagerStorageAdapterUpdateBegin(
          work->member_info->id, begin_context->type,
          &work->member_data[work->index]);
      if (ret != kEsfParameterStorageManagerStatusOk) {
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Failed to start data update. i=%" PRIu32
            ", item_id=%u, ret=%u(%s)",
            work->index, work->member_info->id, ret,
            EsfParameterStorageManagerStrError(ret));
        break;
      }
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
          "Setting update data to handle for item_id=%u",
          work->member_info->id);
      ret = EsfParameterStorageManagerResourceSetUpdateDataToHandle(
          context->handle, work->member_info->id,
          work->member_data[work->index].update_data);
      if (ret != kEsfParameterStorageManagerStatusOk) {
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Failed to store update information. i=%" PRIu32
            ", item_id=%u, ret=%u(%s)",
            work->index, work->member_info->id, ret,
            EsfParameterStorageManagerStrError(ret));
        break;
      }
    }

    if (ret == kEsfParameterStorageManagerStatusOk ||
        ret == kEsfParameterStorageManagerStatusDataLoss) {
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
          "Update begin process completed with status: %u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }

    // Reverts the updating data.
    ESF_PARAMETER_STORAGE_MANAGER_WARN(
        "Update failed, attempting to revert changes");
    EsfParameterStorageManagerStatus revert_ret =
        EsfParameterStorageManagerInternalUpdateCancel(context->handle, work);
    if (revert_ret != kEsfParameterStorageManagerStatusOk) {
      ret = kEsfParameterStorageManagerStatusDataLoss;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to cancel data update. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_FLASH_CONTROL_FAILURE);
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Successfully reverted changes");
    ESF_PARAMETER_STORAGE_MANAGER_EVENT_WARN(
        ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_ROLLBACK_FAILURE);
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static EsfParameterStorageManagerStatus
EsfParameterStorageManagerInternalUpdateCompleteControl(
    EsfParameterStorageManagerResourceExclusiveContext* context) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;
  EsfParameterStorageManagerWorkMemberData* member_data = NULL;

  do {
    if (context == NULL) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR("Null pointer provided for context.");
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }

    EsfParameterStorageManagerResourceUpdateInfo info;

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Checking if handle is already being updated");
    ret = EsfParameterStorageManagerResourceHandleIsAlreadyBeingUpdated(
        context->handle);
    if (ret != kEsfParameterStorageManagerStatusFailedPrecondition) {
      if (ret == kEsfParameterStorageManagerStatusOk) {
        ESF_PARAMETER_STORAGE_MANAGER_ERROR("Update has not started.");
        ret = kEsfParameterStorageManagerStatusFailedPrecondition;
        ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
            ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      }
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Unable to complete updating data. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Getting update data from handle");
    ret = EsfParameterStorageManagerResourceGetUpdateDataFromHandle(
        context->handle, &info);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to get update information. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Allocating member data for %d items",
                                        info.count);
    member_data =
        EsfParameterStorageManagerInternalAllocateMemberData(info.count);
    if (member_data == NULL) {
      ret = kEsfParameterStorageManagerStatusResourceExhausted;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to allocate work memory. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_TRACE(
        "Starting update complete process for %d items", info.count);
    for (int i = 0; i < info.count; ++i) {
      member_data[i].update = true;
      member_data[i].update_data = info.data[i];
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Completing update for item id=%u",
                                          info.id[i]);
      ret = EsfParameterStorageManagerStorageAdapterUpdateComplete(
          info.id[i], &member_data[i]);
      if (ret != kEsfParameterStorageManagerStatusOk) {
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Failed to complete update. id=%u, ret=%u(%s)", info.id[i], ret,
            EsfParameterStorageManagerStrError(ret));
        break;
      }
    }

    if (ret == kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
          "Update complete process finished successfully");
    }
  } while (0);

  if (context != NULL) {
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Removing update data from handle");
    EsfParameterStorageManagerResourceRemoveUpdateDataFromHandle(
        context->handle);
  }

  if (member_data != NULL) {
    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Freeing member data");
    EsfParameterStorageManagerInternalFreeMemberData(member_data);
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static EsfParameterStorageManagerStatus
EsfParameterStorageManagerInternalUpdateCancelControl(
    EsfParameterStorageManagerResourceExclusiveContext* context) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;
  EsfParameterStorageManagerWorkMemberData* member_data = NULL;

  do {
    if (context == NULL) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR("Null pointer provided for context.");
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }

    EsfParameterStorageManagerResourceUpdateInfo info;

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Checking if handle is already being updated");
    ret = EsfParameterStorageManagerResourceHandleIsAlreadyBeingUpdated(
        context->handle);
    if (ret != kEsfParameterStorageManagerStatusFailedPrecondition) {
      if (ret == kEsfParameterStorageManagerStatusOk) {
        ESF_PARAMETER_STORAGE_MANAGER_ERROR("Update has not started.");
        ret = kEsfParameterStorageManagerStatusFailedPrecondition;
        ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
            ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      }
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Unable to cancel updating data. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Getting update data from handle");
    ret = EsfParameterStorageManagerResourceGetUpdateDataFromHandle(
        context->handle, &info);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to get update information. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Allocating member data for %d items",
                                        info.count);
    member_data =
        EsfParameterStorageManagerInternalAllocateMemberData(info.count);
    if (member_data == NULL) {
      ret = kEsfParameterStorageManagerStatusResourceExhausted;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to allocate work memory. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_TRACE(
        "Starting update cancel process for %d items", info.count);
    for (int i = 0; i < info.count; ++i) {
      member_data[i].update = true;
      member_data[i].update_data = info.data[i];
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Cancelling update for item id=%u",
                                          info.id[i]);
      ret = EsfParameterStorageManagerStorageAdapterUpdateCancel(
          info.id[i], &member_data[i]);
      if (ret != kEsfParameterStorageManagerStatusOk) {
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Failed to cancel update. id=%u, ret=%u(%s)", info.id[i], ret,
            EsfParameterStorageManagerStrError(ret));
        break;
      }
    }

    if (ret == kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
          "Update cancel process finished successfully");
    }
  } while (0);

  if (context != NULL) {
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Removing update data from handle");
    EsfParameterStorageManagerResourceRemoveUpdateDataFromHandle(
        context->handle);
  }

  if (member_data != NULL) {
    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Freeing member data");
    EsfParameterStorageManagerInternalFreeMemberData(member_data);
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static bool EsfParameterStorageManagerInternalIsDataEmpty(
    uintptr_t data, EsfParameterStorageManagerItemType type) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  bool ret = false;

  ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Checking if data is empty for type: %u",
                                      type);

  switch (type) {
    case kEsfParameterStorageManagerItemTypeBinaryArray: {
      const EsfParameterStorageManagerBinaryArray* obj =
          (const EsfParameterStorageManagerBinaryArray*)data;
      ret = ESF_PARAMETER_STORAGE_MANAGER_BINARY_ARRAY_IS_EMPTY(obj);
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG("BinaryArray is %s",
                                          ret ? "empty" : "not empty");
      break;
    }
    case kEsfParameterStorageManagerItemTypeBinaryPointer: {
      const EsfParameterStorageManagerBinary* obj =
          (const EsfParameterStorageManagerBinary*)data;
      ret = ESF_PARAMETER_STORAGE_MANAGER_BINARY_POINTER_IS_EMPTY(obj);
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG("BinaryPointer is %s",
                                          ret ? "empty" : "not empty");
      break;
    }
    case kEsfParameterStorageManagerItemTypeOffsetBinaryArray: {
      const EsfParameterStorageManagerOffsetBinaryArray* obj =
          (const EsfParameterStorageManagerOffsetBinaryArray*)data;
      ret = ESF_PARAMETER_STORAGE_MANAGER_OFFSET_BINARY_ARRAY_IS_EMPTY(obj);
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG("OffsetBinaryArray is %s",
                                          ret ? "empty" : "not empty");
      break;
    }
    case kEsfParameterStorageManagerItemTypeOffsetBinaryPointer: {
      const EsfParameterStorageManagerOffsetBinary* obj =
          (const EsfParameterStorageManagerOffsetBinary*)data;
      ret = ESF_PARAMETER_STORAGE_MANAGER_OFFSET_BINARY_POINTER_IS_EMPTY(obj);
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG("OffsetBinaryPointer is %s",
                                          ret ? "empty" : "not empty");
      break;
    }
    case kEsfParameterStorageManagerItemTypeString: {
      const char* obj = (const char*)data;
      ret = ESF_PARAMETER_STORAGE_MANAGER_STRING_IS_EMPTY(obj);
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG("String is %s",
                                          ret ? "empty" : "not empty");
      break;
    }
    case kEsfParameterStorageManagerItemTypeRaw: {
      const EsfParameterStorageManagerRaw* obj =
          (const EsfParameterStorageManagerRaw*)data;
      ret = ESF_PARAMETER_STORAGE_MANAGER_RAW_IS_EMPTY(obj);
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Raw is %s",
                                          ret ? "empty" : "not empty");
      break;
    }
    case kEsfParameterStorageManagerItemTypeMax:
    default:
      ESF_PARAMETER_STORAGE_MANAGER_ERROR("Invalid item type: %u", type);
      break;
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %d", ret);
  return ret;
}
