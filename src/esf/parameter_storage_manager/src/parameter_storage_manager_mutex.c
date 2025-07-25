/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#include "parameter_storage_manager/src/parameter_storage_manager_mutex.h"

#include <pthread.h>

#include "parameter_storage_manager/src/parameter_storage_manager_config.h"
#include "parameter_storage_manager/src/parameter_storage_manager_utility.h"

typedef enum EsfParameterStorageManagerMutex {
  kEsfParameterStorageManagerMutexResource = 0,
  kEsfParameterStorageManagerMutexStorage,
  kEsfParameterStorageManagerMutexNum,
} EsfParameterStorageManagerMutex;

// Internal mutex lock information.
typedef struct EsfParameterStorageManagerResourceMutexInfo {
  // Represents the recursive lock count. 0 means no lock.
  int8_t count;

  // The thread ID that has acquired the lock.
  // Do not reference the 'count' member if it is 0.
  pthread_t own;
} EsfParameterStorageManagerResourceMutexInfo;

// """The pointer type for the predicate function of a condition variable.

// Returns true if the thread IDs are identical.
// Otherwise, returns the value of the global variable bool resource_locked.

// Args:
//     [IN] mtx (EsfParameterStorageManagerMutex):
//          An Enum representing the target Mutex for operation.

// Returns:
//     bool: Returns true if the operation is possible.
//           Returns true if the condition cannot be processed.

// Note:
// """
typedef bool (*EsfParameterStorageManagerResourceExclusivePredicateFunc)(
    EsfParameterStorageManagerMutex);

// """Acquires a recursive lock.

// Acquires a recursive lock. If the lock is acquired, the lock count and thread
// ID are updated.

// Args:
//     [IN] timeout (const struct timespec*): Timeout time (absolute time).
//     [IN] func (EsfParameterStorageManagerResourceExclusivePredicateFunc):
//         Pointer to the predicate function of the condition variable.
//         Cannot pass NULL to this argument.
//     [IN] mtx (EsfParameterStorageManagerMutex):
//          An Enum representing the target Mutex for operation.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusTimedOut: Timeout.
//     kEsfParameterStorageManagerStatusInternal: The parameter is invalid.
static EsfParameterStorageManagerStatus
EsfParameterStorageManagerResourceRecursiveLock(
    EsfParameterStorageManagerWaitType type, const struct timespec* timeout,
    EsfParameterStorageManagerResourceExclusivePredicateFunc func,
    EsfParameterStorageManagerMutex info);

// """Releases a recursive lock.

// Releases a recursive lock. Decrements the lock count.

// Args:
//     [IN] mtx (EsfParameterStorageManagerMutex):
//          An Enum representing the target Mutex for operation.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusTimedOut: Timeout.
//     kEsfParameterStorageManagerStatusInternal: The parameter set by Device
//     Setting is an invalid
//         value.
static EsfParameterStorageManagerStatus
EsfParameterStorageManagerResourceRecursiveUnlock(
    EsfParameterStorageManagerMutex info);

// """Call the lock acquisition function.

// Calls the lock acquisition function. The lock function to be called is
// determined by the argument "type".
// For "kEsfParameterStorageManagerWaitTimeOut", pthread_mutex_timedlock() is
// called. For "kEsfParameterStorageManagerWaitInfinity", pthread_mutex_lock()
// is called.

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
//     kEsfParameterStorageManagerStatusInternal: Parameter Storage Manager was
//     not initialized. kEsfParameterStorageManagerStatusInternal: The parameter
//     set by Parameter Storage Manager is an invalid
//         value.
//     kEsfParameterStorageManagerStatusInternal: Upper limit for recursive lock
//     acquisition. kEsfParameterStorageManagerStatusInternal: Internal
//     processing failed.

// Note:
// """
static EsfParameterStorageManagerStatus
EsfParameterStorageManagerResourceMutexLock(void);

// """Calls the lock release function.

// Calls the lock release function pthread_mutex_unlock().

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
static EsfParameterStorageManagerStatus
EsfParameterStorageManagerResourceMutexUnlock(void);

// """Determines if resource processing is locked.

// Returns true if the thread IDs are identical.
// Otherwise, returns the value of the global variable bool resource_locked.

// Args:
//     [IN] mtx (EsfParameterStorageManagerMutex):
//          An Enum representing the target Mutex for operation.

// Returns:
//     bool: Returns true if resource processing is feasible.
//           Returns false if resource processing is not feasible.

// Note:
// """
static bool EsfParameterStorageManagerResourceIsMutexUnlocked(
    EsfParameterStorageManagerMutex mtx);

// """Acquire recursive locks by conditional variables.

// Applies a recursive lock to a condition variable. While acquiring a recursive
// lock, the lock is released while the condition is not met and waits until the
// condition is met. This API should be called while locking by recursive_mutex
// is in effect. This API should be called when locking by lock_count_mutex is
// not enabled. pred is used to determine if locking is enabled.

// Args:
//     [IN] pred (EsfParameterStorageManagerResourceExclusivePredicateFunc):
//         Pointer to the predicate function of the condition variable.
//         Cannot pass NULL to this argument.
//     [IN] type (EsfParameterStorageManagerWaitType): Type of condition
//     variable standby
//                                            operation.
//     [IN] timeout (const struct timespec*): Timeout time (absolute time).
//     [IN] mtx (EsfParameterStorageManagerMutex):
//          An Enum representing the target Mutex for operation.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusTimedOut: Timeout.
//     kEsfParameterStorageManagerStatusInternal: The parameter set by Device
//     Setting is an invalid
//         value.

// Note:
// """
static EsfParameterStorageManagerStatus
EsfParameterStorageManagerResourceRecursiveConditionWait(
    EsfParameterStorageManagerResourceExclusivePredicateFunc pred,
    EsfParameterStorageManagerWaitType type, const struct timespec* timeout,
    EsfParameterStorageManagerMutex mtx);

// """Check whether the context settings are correct.

// Returns true if the context settings are correct.
// Otherwise, returns false.

// Args:
//     [IN] context (EsfParameterStorageManagerResourceExclusiveContext*):
//         Exclusive control context.

// Returns:
//     bool: Returns true if the context settings are correct.
//           Otherwise, returns false.

// Note:
// """
static bool EsfParameterStorageManagerResourceIsValidExclusiveContext(
    const EsfParameterStorageManagerResourceExclusiveContext* context);

// """Acquire recursive locks by conditional variables.

// Performs a wait on a condition variable.

// Args:
//     [IN] type (EsfParameterStorageManagerWaitType): Type of condition
//     variable standby
//                                            operation.
//     [IN] timeout (const struct timespec*): Timeout time (absolute time).

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusTimedOut: Timeout.
//     kEsfParameterStorageManagerStatusInternal: The parameter set by Device
//     Setting is an invalid
//         value.

// Note:
// """
static EsfParameterStorageManagerStatus
EsfParameterStorageManagerResourceConditionWait(
    EsfParameterStorageManagerWaitType type, const struct timespec* timeout);

// """Resource related processing is performed.

// Executes the specified function as a process for the resource only or before
// accessing the storage.

// Args:
//     [IN] context (EsfParameterStorageManagerResourceExclusiveContext*):
//         Exclusive control context. Cannot pass NULL to this argument.
//     [IN] timeout (const struct timespec*): Timeout time (absolute time).

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
//     Other: The return value of the function passed by resource_entry.

// Note:
// """
static EsfParameterStorageManagerStatus
EsfParameterStorageManagerResourceResourceEntryProcess(
    EsfParameterStorageManagerResourceExclusiveContext* context,
    const struct timespec* timeout);

// """Storage related processing is performed.

// Executes the specified function as a process for accessing the storage.

// Args:
//     [IN] context (EsfParameterStorageManagerResourceExclusiveContext*):
//         Exclusive control context. Cannot pass NULL to this argument.
//     [IN] timeout (const struct timespec*): Timeout time (absolute time).

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
//     Other: The return value of the function passed by storage_func.

// Note:
// """
static EsfParameterStorageManagerStatus
EsfParameterStorageManagerResourceStorageProcess(
    EsfParameterStorageManagerResourceExclusiveContext* context,
    const struct timespec* timeout);

// """Resource related processing is performed.

// Executes the specified function as a process for after accessing the storage.

// Args:
//     [IN] context (EsfParameterStorageManagerResourceExclusiveContext*):
//         Exclusive control context. Cannot pass NULL to this argument.
//     [IN] timeout (const struct timespec*): Timeout time (absolute time).

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
//     Other: The return value of the function passed by resource_exit.

// Note:
// """
static EsfParameterStorageManagerStatus
EsfParameterStorageManagerResourceResourceExitProcess(
    EsfParameterStorageManagerResourceExclusiveContext* context,
    const struct timespec* timeout);

// Global Variables of Exclusive Control Objects
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static EsfParameterStorageManagerResourceMutexInfo
    mutex_info[kEsfParameterStorageManagerMutexNum];

// Constants for calculating the timeout period.
static const int32_t kMsecPerSec = 1 * 1000;
static const int32_t kNsecPerSec = 1 * 1000 * 1000 * 1000;
static const int32_t kNsecPerMsec = kNsecPerSec / kMsecPerSec;
static const int32_t kConfigSec =
    CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_TIMEOUT_MS / kMsecPerSec;
static const int32_t kConfigMsec =
    CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_TIMEOUT_MS -
    kConfigSec * kMsecPerSec;

bool EsfParameterStorageManagerCreateTimeOut(struct timespec* timeout) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  bool ret = false;

  do {
    if (timeout == NULL) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR("Argument \"timeout\" is NULL.");
      ret = true;
      break;
    }

    int clock_ret = clock_gettime(CLOCK_REALTIME, timeout);
    if (clock_ret != 0) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to get current time. clock_ret=%d", clock_ret);
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INTERNAL_PROCESS_ERROR);
      ret = true;
      break;
    }

    timeout->tv_sec += kConfigSec;
    timeout->tv_nsec += kConfigMsec * kNsecPerMsec;

    if (timeout->tv_nsec >= kNsecPerSec) {
      timeout->tv_sec += 1;
      timeout->tv_nsec -= kNsecPerSec;
      ESF_PARAMETER_STORAGE_MANAGER_TRACE(
          "Carryover completed. New tv_sec=%.lf, tv_nsec=%ld",
          difftime(timeout->tv_sec, 0), timeout->tv_nsec);
    }

    ESF_PARAMETER_STORAGE_MANAGER_TRACE(
        "Timeout set to: tv_sec=%.lf, tv_nsec=%ld",
        difftime(timeout->tv_sec, 0), timeout->tv_nsec);
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %d", ret);
  return ret;
}

EsfParameterStorageManagerStatus EsfParameterStorageManagerResourceLock(
    EsfParameterStorageManagerWaitType type, const struct timespec* timeout) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret =
      EsfParameterStorageManagerResourceRecursiveLock(
          type, timeout, EsfParameterStorageManagerResourceIsMutexUnlocked,
          kEsfParameterStorageManagerMutexResource);
  if (ret == kEsfParameterStorageManagerStatusOk) {
    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Resource lock acquired successfully");
  } else {
    ESF_PARAMETER_STORAGE_MANAGER_ERROR(
        "Failed to get resource lock. ret=%u(%s)", ret,
        EsfParameterStorageManagerStrError(ret));
    if (ret == kEsfParameterStorageManagerStatusTimedOut) {
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_WARN(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INTERNAL_PROCESS_WARN);
    } else {
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INTERNAL_PROCESS_ERROR);
    }
  }
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus EsfParameterStorageManagerResourceUnlock(
    void) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret =
      EsfParameterStorageManagerResourceRecursiveUnlock(
          kEsfParameterStorageManagerMutexResource);
  if (ret == kEsfParameterStorageManagerStatusOk) {
    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Resource lock released successfully");
  } else {
    ESF_PARAMETER_STORAGE_MANAGER_ERROR(
        "Failed to release resource lock. ret=%u(%s)", ret,
        EsfParameterStorageManagerStrError(ret));
    ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
        ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INTERNAL_PROCESS_ERROR);
  }
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus EsfParameterStorageManagerResourceStorageLock(
    EsfParameterStorageManagerWaitType type, const struct timespec* timeout) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret =
      EsfParameterStorageManagerResourceRecursiveLock(
          type, timeout, EsfParameterStorageManagerResourceIsMutexUnlocked,
          kEsfParameterStorageManagerMutexStorage);
  if (ret == kEsfParameterStorageManagerStatusOk) {
    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Storage lock acquired successfully");
  } else {
    ESF_PARAMETER_STORAGE_MANAGER_ERROR(
        "Failed to get storage lock. ret=%u(%s)", ret,
        EsfParameterStorageManagerStrError(ret));
    if (ret == kEsfParameterStorageManagerStatusTimedOut) {
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_WARN(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INTERNAL_PROCESS_WARN);
    } else {
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INTERNAL_PROCESS_ERROR);
    }
  }
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerResourceStorageUnlock(void) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret =
      EsfParameterStorageManagerResourceRecursiveUnlock(
          kEsfParameterStorageManagerMutexStorage);
  if (ret == kEsfParameterStorageManagerStatusOk) {
    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Storage lock released successfully");
  } else {
    ESF_PARAMETER_STORAGE_MANAGER_ERROR(
        "Failed to release storage lock. ret=%u(%s)", ret,
        EsfParameterStorageManagerStrError(ret));
    ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
        ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INTERNAL_PROCESS_ERROR);
  }
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerResourceExclusiveControl(
    EsfParameterStorageManagerResourceExclusiveContext* context) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    if (!EsfParameterStorageManagerResourceIsValidExclusiveContext(context)) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Argument \"context\" is invalid. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Generating timeout time");
    struct timespec timeout;
    memset(&timeout, 0, sizeof(timeout));
    if (EsfParameterStorageManagerCreateTimeOut(&timeout)) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to generate timeout time. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Executing resource entry process");
    ret = EsfParameterStorageManagerResourceResourceEntryProcess(context,
                                                                 &timeout);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to execute resource process. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Executing storage process");
    ret = EsfParameterStorageManagerResourceStorageProcess(context, &timeout);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to execute storage process. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      // fallthrough to do resource_exit process
    }

    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Executing resource exit process");
    EsfParameterStorageManagerStatus exit_ret =
        EsfParameterStorageManagerResourceResourceExitProcess(context,
                                                              &timeout);
    if (exit_ret != kEsfParameterStorageManagerStatusOk) {
      ret = exit_ret;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to execute exiting resource processing. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static bool EsfParameterStorageManagerResourceIsMutexUnlocked(
    EsfParameterStorageManagerMutex mtx) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  bool ret = true;

  do {
    if (mutex_info[mtx].count == 0) {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE("Mutex is unlocked. mtx=%u", mtx);
      break;
    }
    if (pthread_equal(mutex_info[mtx].own, pthread_self())) {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE(
          "The current thread owns the mutex lock. mtx=%u", mtx);
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "The mutex is locked by another thread. mtx=%u, count=%" PRId8
        ", own=" ESF_PARAMETER_STORAGE_MANAGER_PTHREAD_ID_FORMAT,
        mtx, mutex_info[mtx].count,
        ESF_PARAMETER_STORAGE_MANAGER_PTHREAD_ID_VALUE(mutex_info[mtx].own));
    ret = false;
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %d", ret);
  return ret;
}

static EsfParameterStorageManagerStatus
EsfParameterStorageManagerResourceRecursiveConditionWait(
    EsfParameterStorageManagerResourceExclusivePredicateFunc pred,
    EsfParameterStorageManagerWaitType type, const struct timespec* timeout,
    EsfParameterStorageManagerMutex mtx) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("pred=%s, type=%u, timeout=%p, mtx=%u",
                                      pred == NULL ? "Invalid" : "Valid", type,
                                      (void*)timeout, mtx);
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  int iteration = 0;
  while (!pred(mtx)) {
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Waiting for condition, iteration %d",
                                        iteration);
    ret = EsfParameterStorageManagerResourceConditionWait(type, timeout);
    if (ret == kEsfParameterStorageManagerStatusTimedOut) {
      ESF_PARAMETER_STORAGE_MANAGER_WARN(
          "The timeout expired because the conditions were not met. "
          "ret=%u(%s), iteration=%d",
          ret, EsfParameterStorageManagerStrError(ret), iteration);
      break;
    }
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "An error occurred while waiting for a condition. "
          "ret=%u(%s), iteration=%d",
          ret, EsfParameterStorageManagerStrError(ret), iteration);
      break;
    }
    ++iteration;
  }

  if (ret == kEsfParameterStorageManagerStatusOk) {
    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Condition met after %d iterations",
                                        iteration);
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static EsfParameterStorageManagerStatus
EsfParameterStorageManagerResourceRecursiveLock(
    EsfParameterStorageManagerWaitType type, const struct timespec* timeout,
    EsfParameterStorageManagerResourceExclusivePredicateFunc func,
    EsfParameterStorageManagerMutex mtx) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  bool locked = false;
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;
  do {
    if (func == NULL || mtx < 0 || kEsfParameterStorageManagerMutexNum <= mtx) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Argument is invalid. func=%p, mtx=%u, ret=%u(%s)", func, mtx, ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_TRACE(
        "Attempting to acquire initial mutex lock");
    ret = EsfParameterStorageManagerResourceMutexLock();
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to get initial lock. mtx=%u, ret=%u(%s)", mtx, ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
    locked = true;
    ESF_PARAMETER_STORAGE_MANAGER_TRACE(
        "Initial mutex lock acquired successfully");

    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Waiting for recursive condition");
    ret = EsfParameterStorageManagerResourceRecursiveConditionWait(
        func, type, timeout, mtx);

    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to get recursive lock. mtx=%u, ret=%u(%s)", mtx, ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }

    if (mutex_info[mtx].count < 0 || mutex_info[mtx].count == INT8_MAX) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "The lock count limit has been reached. mtx=%u, count=%" PRId8
          ", ret=%u(%s)",
          mtx, mutex_info[mtx].count, ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }

    mutex_info[mtx].count += 1;
    mutex_info[mtx].own = pthread_self();

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Successfully acquired mutex lock. mtx=%u, count=%" PRId8
        ", own=" ESF_PARAMETER_STORAGE_MANAGER_PTHREAD_ID_FORMAT,
        mtx, mutex_info[mtx].count,
        ESF_PARAMETER_STORAGE_MANAGER_PTHREAD_ID_VALUE(mutex_info[mtx].own));
  } while (0);

  if (locked) {
    ESF_PARAMETER_STORAGE_MANAGER_TRACE(
        "Attempting to release initial mutex lock");
    EsfParameterStorageManagerStatus lock_ret =
        EsfParameterStorageManagerResourceMutexUnlock();
    if (lock_ret != kEsfParameterStorageManagerStatusOk) {
      ret = lock_ret;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to release initial mutex lock. mtx=%u, ret=%u(%s)", mtx, ret,
          EsfParameterStorageManagerStrError(ret));
    } else {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE(
          "Initial mutex lock released successfully");
    }
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static EsfParameterStorageManagerStatus
EsfParameterStorageManagerResourceRecursiveUnlock(
    EsfParameterStorageManagerMutex mtx) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  bool locked = false;
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;
  do {
    if (mtx < 0 || kEsfParameterStorageManagerMutexNum <= mtx) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Argument \"mtx\" is invalid. mtx=%u, ret=%u(%s)", mtx, ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_TRACE(
        "Attempting to acquire mutex lock for unlocking");
    ret = EsfParameterStorageManagerResourceMutexLock();
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to get lock for unlocking. mtx=%u, ret=%u(%s)", mtx, ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
    locked = true;
    ESF_PARAMETER_STORAGE_MANAGER_TRACE(
        "Mutex lock for unlocking acquired successfully");

    if (mutex_info[mtx].count <= 0) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "The lock count is already zero or negative. mtx=%u, count=%" PRId8
          ", ret=%u(%s)",
          mtx, mutex_info[mtx].count, ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
    if (!pthread_equal(mutex_info[mtx].own, pthread_self())) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Attempt to unlock mutex owned by another thread. mtx=%u, "
          "count=%" PRId8
          ", own=" ESF_PARAMETER_STORAGE_MANAGER_PTHREAD_ID_FORMAT
          " ret=%u(%s)",
          mtx, mutex_info[mtx].count,
          ESF_PARAMETER_STORAGE_MANAGER_PTHREAD_ID_VALUE(mutex_info[mtx].own),
          ret, EsfParameterStorageManagerStrError(ret));
      break;
    }

    mutex_info[mtx].count -= 1;
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Successfully decreased mutex lock count. mtx=%u, count=%" PRId8
        ", own=" ESF_PARAMETER_STORAGE_MANAGER_PTHREAD_ID_FORMAT,
        mtx, mutex_info[mtx].count,
        ESF_PARAMETER_STORAGE_MANAGER_PTHREAD_ID_VALUE(mutex_info[mtx].own));
    if (mutex_info[mtx].count == 0) {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE(
          "Mutex fully unlocked. Broadcasting to waiting threads");
      int cond_ret = pthread_cond_broadcast(&cond);
      if (cond_ret != 0) {
        ESF_PARAMETER_STORAGE_MANAGER_WARN(
            "Failed to unblock waiting threads. mtx=%u, cond_ret=%d", mtx,
            cond_ret);
      } else {
        ESF_PARAMETER_STORAGE_MANAGER_TRACE(
            "Successfully broadcasted to waiting threads");
      }
    }
  } while (0);

  if (locked) {
    ESF_PARAMETER_STORAGE_MANAGER_TRACE(
        "Attempting to release mutex lock used for unlocking");
    EsfParameterStorageManagerStatus lock_ret =
        EsfParameterStorageManagerResourceMutexUnlock();
    if (lock_ret != kEsfParameterStorageManagerStatusOk) {
      ret = lock_ret;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to release mutex lock used for unlocking. mtx=%u, ret=%u(%s)",
          mtx, ret, EsfParameterStorageManagerStrError(ret));
    } else {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE(
          "Successfully released mutex lock used for unlocking");
    }
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static bool EsfParameterStorageManagerResourceIsValidExclusiveContext(
    const EsfParameterStorageManagerResourceExclusiveContext* context) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  bool ret = true;

  do {
    if (context == NULL) {
      ret = false;
      break;
    }
    // all functions
    if (context->resource_entry != NULL && context->resource_exit != NULL &&
        context->storage_func != NULL) {
      break;
    }
    // resource entry only
    if (context->resource_entry != NULL && context->resource_exit == NULL &&
        context->storage_func == NULL) {
      break;
    }
    // storage only
    if (context->resource_entry == NULL && context->resource_exit == NULL &&
        context->storage_func != NULL) {
      break;
    }
    ret = false;
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %d", ret);
  return ret;
}

static EsfParameterStorageManagerStatus
EsfParameterStorageManagerResourceMutexLock(void) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    int lock_ret = 0;
    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Attempting infinite wait mutex lock");
    lock_ret = pthread_mutex_lock(&mutex);
    if (lock_ret != 0) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Unexpected error occurred during mutex lock. lock_ret=%d, "
          "ret=%u(%s)",
          lock_ret, ret, EsfParameterStorageManagerStrError(ret));
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Mutex lock acquired successfully");
    ret = kEsfParameterStorageManagerStatusOk;
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static EsfParameterStorageManagerStatus
EsfParameterStorageManagerResourceMutexUnlock(void) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  int unlock_ret = pthread_mutex_unlock(&mutex);
  if (unlock_ret == 0) {
    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Mutex unlocked successfully");
  } else {
    ret = kEsfParameterStorageManagerStatusInternal;
    if (unlock_ret == EPERM) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to unlock mutex: current thread does not own the mutex. "
          "unlock_ret=%d, ret=%u(%s)",
          unlock_ret, ret, EsfParameterStorageManagerStrError(ret));
    } else {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to unlock mutex. unlock_ret=%d, ret=%u(%s)", unlock_ret, ret,
          EsfParameterStorageManagerStrError(ret));
    }
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static EsfParameterStorageManagerStatus
EsfParameterStorageManagerResourceConditionWait(
    EsfParameterStorageManagerWaitType type, const struct timespec* timeout) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    if (type == kEsfParameterStorageManagerWaitTimeOut && timeout == NULL) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Argument \"timeout\" is NULL for timeout wait. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }

    int lock_ret = 0;
    switch (type) {
      case kEsfParameterStorageManagerWaitTimeOut:
        ESF_PARAMETER_STORAGE_MANAGER_TRACE("Attempting timed condition wait");
        lock_ret = pthread_cond_timedwait(&cond, &mutex, timeout);
        break;
      case kEsfParameterStorageManagerWaitInfinity:
        ESF_PARAMETER_STORAGE_MANAGER_TRACE(
            "Attempting infinite condition wait");
        lock_ret = pthread_cond_wait(&cond, &mutex);
        break;
      case kEsfParameterStorageManagerWaitMax:
      default:
        ret = kEsfParameterStorageManagerStatusInternal;
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Invalid wait type specified. type=%u, ret=%u(%s)", type, ret,
            EsfParameterStorageManagerStrError(ret));
        break;
    }
    if (ret != kEsfParameterStorageManagerStatusOk) {
      break;
    }

    if (lock_ret == ETIMEDOUT) {
      ret = kEsfParameterStorageManagerStatusTimedOut;
      ESF_PARAMETER_STORAGE_MANAGER_WARN("Condition wait timed out.");
      break;
    }
    if (lock_ret != 0) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Condition wait failed. lock_ret=%d, ret=%u(%s)", lock_ret, ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_TRACE(
        "Condition wait completed successfully");
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static EsfParameterStorageManagerStatus
EsfParameterStorageManagerResourceResourceEntryProcess(
    EsfParameterStorageManagerResourceExclusiveContext* context,
    const struct timespec* timeout) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  if (context->resource_entry != NULL) {
    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Starting resource entry process");
    bool locked = false;
    do {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE(
          "Attempting to acquire resource lock");
      ret = EsfParameterStorageManagerResourceLock(
          kEsfParameterStorageManagerWaitTimeOut, timeout);
      if (ret != kEsfParameterStorageManagerStatusOk) {
        if (ret == kEsfParameterStorageManagerStatusTimedOut) {
          ESF_PARAMETER_STORAGE_MANAGER_WARN(
              "Timed out while trying to acquire resource lock. ret=%u(%s)",
              ret, EsfParameterStorageManagerStrError(ret));
        } else {
          ESF_PARAMETER_STORAGE_MANAGER_ERROR(
              "Failed to get resource lock. ret=%u(%s)", ret,
              EsfParameterStorageManagerStrError(ret));
        }
        break;
      }
      locked = true;
      ESF_PARAMETER_STORAGE_MANAGER_TRACE(
          "Resource lock acquired successfully");

      ESF_PARAMETER_STORAGE_MANAGER_TRACE("Executing resource entry function");
      ret = context->resource_entry(context);
      if (ret != kEsfParameterStorageManagerStatusOk) {
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Failed to execute resource processing. ret=%u(%s)", ret,
            EsfParameterStorageManagerStrError(ret));
        break;
      }
      ESF_PARAMETER_STORAGE_MANAGER_TRACE(
          "Resource entry function executed successfully");
    } while (0);

    if (locked) {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE(
          "Attempting to release resource lock");
      EsfParameterStorageManagerStatus lock_ret =
          EsfParameterStorageManagerResourceUnlock();
      if (lock_ret != kEsfParameterStorageManagerStatusOk) {
        ret = lock_ret;
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Failed to release resource lock. ret=%u(%s)", ret,
            EsfParameterStorageManagerStrError(ret));
      } else {
        ESF_PARAMETER_STORAGE_MANAGER_TRACE(
            "Resource lock released successfully");
      }
    }
  } else {
    ESF_PARAMETER_STORAGE_MANAGER_TRACE("No resource entry function specified");
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static EsfParameterStorageManagerStatus
EsfParameterStorageManagerResourceStorageProcess(
    EsfParameterStorageManagerResourceExclusiveContext* context,
    const struct timespec* timeout) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  if (context->storage_func != NULL) {
    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Starting storage process");
    bool locked = false;
    do {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE("Attempting to acquire storage lock");
      ret = EsfParameterStorageManagerResourceStorageLock(
          kEsfParameterStorageManagerWaitTimeOut, timeout);
      if (ret != kEsfParameterStorageManagerStatusOk) {
        if (ret == kEsfParameterStorageManagerStatusTimedOut) {
          ESF_PARAMETER_STORAGE_MANAGER_WARN(
              "Timed out while trying to acquire storage lock. ret=%u(%s)", ret,
              EsfParameterStorageManagerStrError(ret));
        } else {
          ESF_PARAMETER_STORAGE_MANAGER_ERROR(
              "Failed to get storage lock. ret=%u(%s)", ret,
              EsfParameterStorageManagerStrError(ret));
        }
        break;
      }
      locked = true;
      ESF_PARAMETER_STORAGE_MANAGER_TRACE("Storage lock acquired successfully");

      ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Executing storage function");
      ret = context->storage_func(context);
      if (ret != kEsfParameterStorageManagerStatusOk) {
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Failed to execute storage processing. ret=%u(%s)", ret,
            EsfParameterStorageManagerStrError(ret));
        break;
      }
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
          "Storage function executed successfully");
    } while (0);

    if (locked) {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE("Attempting to release storage lock");
      EsfParameterStorageManagerStatus lock_ret =
          EsfParameterStorageManagerResourceStorageUnlock();
      if (lock_ret != kEsfParameterStorageManagerStatusOk) {
        ret = lock_ret;
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Failed to release storage lock. ret=%u(%s)", ret,
            EsfParameterStorageManagerStrError(ret));
      } else {
        ESF_PARAMETER_STORAGE_MANAGER_TRACE(
            "Storage lock released successfully");
      }
    }
  } else {
    ESF_PARAMETER_STORAGE_MANAGER_TRACE("No storage function specified");
  }

  if (ret != kEsfParameterStorageManagerStatusOk) {
    ESF_PARAMETER_STORAGE_MANAGER_WARN("Storage process failed. Status: %u",
                                       ret);
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static EsfParameterStorageManagerStatus
EsfParameterStorageManagerResourceResourceExitProcess(
    EsfParameterStorageManagerResourceExclusiveContext* context,
    const struct timespec* timeout) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  (void)timeout;
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  if (context->resource_exit != NULL) {
    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Starting resource exit process");
    bool locked = false;
    do {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE(
          "Attempting to acquire resource lock for exit process");
      ret = EsfParameterStorageManagerResourceLock(
          kEsfParameterStorageManagerWaitInfinity, NULL);
      if (ret != kEsfParameterStorageManagerStatusOk) {
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Failed to get resource lock for exit process. ret=%u(%s)", ret,
            EsfParameterStorageManagerStrError(ret));
        break;
      }
      locked = true;
      ESF_PARAMETER_STORAGE_MANAGER_TRACE(
          "Resource lock for exit process acquired successfully");

      ESF_PARAMETER_STORAGE_MANAGER_TRACE("Executing resource exit function");
      ret = context->resource_exit(context);
      if (ret != kEsfParameterStorageManagerStatusOk) {
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Failed to execute resource exit processing. ret=%u(%s)", ret,
            EsfParameterStorageManagerStrError(ret));
        break;
      }
      ESF_PARAMETER_STORAGE_MANAGER_TRACE(
          "Resource exit function executed successfully");
    } while (0);

    if (locked) {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE(
          "Attempting to release resource lock after exit process");
      EsfParameterStorageManagerStatus lock_ret =
          EsfParameterStorageManagerResourceUnlock();
      if (lock_ret != kEsfParameterStorageManagerStatusOk) {
        ret = lock_ret;
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Failed to release resource lock after exit process. ret=%u(%s)",
            ret, EsfParameterStorageManagerStrError(ret));
      } else {
        ESF_PARAMETER_STORAGE_MANAGER_TRACE(
            "Resource lock released successfully after exit process");
      }
    }
  } else {
    ESF_PARAMETER_STORAGE_MANAGER_TRACE("No resource exit function specified");
  }

  if (ret != kEsfParameterStorageManagerStatusOk) {
    ESF_PARAMETER_STORAGE_MANAGER_WARN(
        "Resource exit process failed. Status: %u", ret);
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}
