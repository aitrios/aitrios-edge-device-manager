/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef ESF_PARAMETER_STORAGE_MANAGER_SRC_PARAMETER_STORAGE_MANAGER_MUTEX_H_
#define ESF_PARAMETER_STORAGE_MANAGER_SRC_PARAMETER_STORAGE_MANAGER_MUTEX_H_

#include <time.h>

#include "parameter_storage_manager.h"

// This is a forward declaration of a structure defined in this file.
// The structure and the function pointer reference each other's types, so need
// to be resolved by a forward declaration.
typedef struct EsfParameterStorageManagerResourceExclusiveContext
    EsfParameterStorageManagerResourceExclusiveContext;

// Function pointer type to be executed during exclusive control.
typedef EsfParameterStorageManagerStatus (
    *EsfParameterStorageManagerResourceExclusiveFunc)(
    EsfParameterStorageManagerResourceExclusiveContext* context);

// Context structure for performing exclusive control.
struct EsfParameterStorageManagerResourceExclusiveContext {
  // A handle for Parameter Storage Manager.
  // If not needed, set it to ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE.
  EsfParameterStorageManagerHandle handle;

  // This function is executed during exclusive control of internal resources,
  // before access to the data storage area.
  EsfParameterStorageManagerResourceExclusiveFunc resource_entry;

  // This is the function to be executed during exclusive control of the data
  // storage area. Set the function to access the data storage area.
  EsfParameterStorageManagerResourceExclusiveFunc storage_func;

  // This function is executed during exclusive control of internal resources.
  // It is executed after access to the data storage area.
  EsfParameterStorageManagerResourceExclusiveFunc resource_exit;

  // Set the data to be used by functions during exclusive control.
  void* private_data;
};

// Initialization macro for EsfParameterStorageManagerResourceExclusiveContext.
#define ESF_PARAMETER_STORAGE_MANAGER_RESOURCE_EXCLUSIVE_CONTEXT_INITIALIZER \
  {                                                                          \
      ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE, NULL, NULL, NULL, NULL,  \
  }

// Enumeration type defines the waiting method for exclusive control.
typedef enum EsfParameterStorageManagerWaitType {
  // Standby method that times out after a certain period of time.
  kEsfParameterStorageManagerWaitTimeOut,

  // Standby method that does not time out after a certain period of time.
  kEsfParameterStorageManagerWaitInfinity,

  // The maximum value for this enumeration.
  kEsfParameterStorageManagerWaitMax,
} EsfParameterStorageManagerWaitType;

// """Generate timeout time using config values.

// Set the timeout period to the argument timeout. The timeout time is the
// absolute time that is the current time plus
// EXTERNAL_PARAMETER_STORAGE_MANAGER_TIMEOUT_MS.

// Args:
//     [OUT] timeout (struct timespec*): Timeout period.

// Returns:
//     bool: Returns true if timeout time could not be generated.
//           Returns false if a timeout period could be generated.

// Yields:
//     true : timeout time could not be generated.
//     false : a timeout period could be generated.

// Note:
// """
bool EsfParameterStorageManagerCreateTimeOut(struct timespec* timeout);

// """Acquire storage locks.

// Allows recursive locking using the normal attribute pthread_mutex. If a lock
// has already been acquired, the thread is referenced, and if it is the same
// thread that acquired the lock, it is treated as having acquired the lock.
// This API should be called when locking by lock_count_mutex is not enabled.

// Args:
//     [IN] type (EsfParameterStorageManagerWaitType ): Type of condition
//     variable standby
//                                            operation.
//     [IN] timeout (struct timespec*): Timeout time (absolute time).

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusTimedOut: Timeout.
//     kEsfParameterStorageManagerStatusInternal: Parameter Storage Manager was
//     not initialized. kEsfParameterStorageManagerStatusInternal: The parameter
//     set by Parameter Storage Manager is an invalid
//         value.
//     kEsfParameterStorageManagerStatusInternal: Upper limit for recursive lock
//     acquisition. kEsfParameterStorageManagerStatusInternal: Internal
//     processing failed.

// Note:
// """
EsfParameterStorageManagerStatus EsfParameterStorageManagerResourceLock(
    EsfParameterStorageManagerWaitType type, const struct timespec* timeout);

// """Release recursive locks.

// Recursive locking is enabled using the normal attribute pthread_mutex. This
// API must be called while locking by recursive_mutex is enabled. This API
// should be called when locking by lock_count_mutex is not enabled.

// Args:
//     Nothing.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusTimedOut: Timeout.
//     kEsfParameterStorageManagerStatusInternal: Parameter Storage Manager was
//     not initialized. kEsfParameterStorageManagerStatusInternal: Abnormal lock
//     acquisition status. kEsfParameterStorageManagerStatusInternal: Internal
//     processing failed.

// Note:
// """
EsfParameterStorageManagerStatus EsfParameterStorageManagerResourceUnlock(void);

// """Acquire storage locks.

// Allows recursive locking using the normal attribute pthread_mutex. If a lock
// has already been acquired, the thread is referenced, and if it is the same
// thread that acquired the lock, it is treated as having acquired the lock.
// This API should be called when locking by lock_count_mutex is not enabled.

// Args:
//     [IN] type (EsfParameterStorageManagerWaitType ): Type of condition
//     variable standby
//                                            operation.
//     [IN] timeout (struct timespec*): Timeout time (absolute time).

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusTimedOut: Timeout.
//     kEsfParameterStorageManagerStatusInternal: Parameter Storage Manager was
//     not initialized. kEsfParameterStorageManagerStatusInternal: The parameter
//     set by Parameter Storage Manager is an invalid
//         value.
//     kEsfParameterStorageManagerStatusInternal: Upper limit for recursive lock
//     acquisition. kEsfParameterStorageManagerStatusInternal: Internal
//     processing failed.

// Note:
// """
EsfParameterStorageManagerStatus EsfParameterStorageManagerResourceStorageLock(
    EsfParameterStorageManagerWaitType type, const struct timespec* timeout);

// """Release recursive locks.

// Recursive locking is enabled using the normal attribute pthread_mutex. This
// API must be called while locking by recursive_mutex is enabled. This API
// should be called when locking by lock_count_mutex is not enabled.

// Args:
//     Nothing.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusTimedOut: Timeout.
//     kEsfParameterStorageManagerStatusInternal: Parameter Storage Manager was
//     not initialized. kEsfParameterStorageManagerStatusInternal: Abnormal lock
//     acquisition status. kEsfParameterStorageManagerStatusInternal: Internal
//     processing failed.

// Note:
// """
EsfParameterStorageManagerStatus
EsfParameterStorageManagerResourceStorageUnlock(void);

// """EsfParameterStorageManagerResourceExclusiveContext is used for exclusive
// control.

// EsfParameterStorageManagerResourceExclusiveContext is used to manage the
// locking state and callback functions to invoke the processing to be performed
// during locking set in the structure.

// Args:
//     [IN] context (EsfParameterStorageManagerResourceExclusiveContext*):
//         The execution context of the exclusive control.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusNotFound: No handles found.
//     kEsfParameterStorageManagerStatusInternal: Parameter Storage Manager was
//     not initialized. kEsfParameterStorageManagerStatusInternal: The parameter
//     set by Parameter Storage Manager is an invalid
//         value.

// Note:
// """
EsfParameterStorageManagerStatus
EsfParameterStorageManagerResourceExclusiveControl(
    EsfParameterStorageManagerResourceExclusiveContext* context);

#endif  // ESF_PARAMETER_STORAGE_MANAGER_SRC_PARAMETER_STORAGE_MANAGER_MUTEX_H_
