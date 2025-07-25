/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESF_CLOCK_MANAGER_CLOCK_MANAGER_H_
#define ESF_CLOCK_MANAGER_CLOCK_MANAGER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/**
 * Definitions of macros
 */

// This macro represents maximum time when Clock Manager waits for ntpclient
// daemon to terminate.  The unit is milliseconds.
#define ESF_CLOCK_MANAGER_STOP_TIMEOUT (2000)

/**
 * Definitions of enumerations
 */

// This enumeration represents enumeration-constants which Clock Manager's APIs
// return.
typedef enum {
  kClockManagerSuccess,               // Success
  kClockManagerParamError,            // Invalid parameter error
  kClockManagerInternalError,         // Internal error
  kClockManagerStateTransitionError,  // State translation error
  kClockManagerNotifierHasAlreadyFinished,
  kClockManagerMonitorHasAlreadyFinished,
  kClockManagerDaemonHasAleadyFinished
} EsfClockManagerReturnValue;

/**
 * Declarations of public functions
 */

// """Initializes Clock Manager.

// This function is not thread-safe.  This function acts in the caller context.
// This function creates objects of pthread_mutex_t, pthread_cond_t or
// structures which have parameters, and so on in volatile memory by it that
// this function calls malloc, then initializes them.
// EsfParameterStorageManagerOpen is called in this function.

// Args:
//    no arguments.

// Returns:
//    Results.  The following value is returned.
//    kClockManagerSuccess: success.
//    kClockManagerInternalError: internal error.
//    kClockManagerStateTransitionError: status translation failure.

// """
EsfClockManagerReturnValue EsfClockManagerInit(void);

// """Deinitializes Clock Manager.

// This function is not thread-safe.  This function acts in the caller context.
// This function frees objects allocated in volatile memory by
// EsfClockManagerInit.
// EsfParameterStorageManagerClose is called in this function.

// Args:
//    no arguments.

// Returns:
//    Results.  The following value is returned.
//    kClockManagerSuccess: success.
//    kClockManagerInternalError: internal error.
//    kClockManagerStateTransitionError: status translation failure.

// """
EsfClockManagerReturnValue EsfClockManagerDeinit(void);

// """Starts one single thead of Clock Manager and ntpclient daemon.

// Two singleton threads of Clock Manager are created, ntpclient daemon is
// started according to the prior given parameters or parameters in
// non-volatile.  No multiple threads of Clock Manager or ntpclient are created.

// Args:
//    no arguments.

// Returns:
//    Results.  The following value is returned.
//    kClockManagerSuccess: success.
//    kClockManagerInternalError: internal error.
//    kClockManagerStateTransitionError: status translation failure.

// """
EsfClockManagerReturnValue EsfClockManagerStart(void);

// """Stops the single thead of Clock Manager and ntpclient daemon.

// This function stops the daemon of ntpclient and the thread of Clock Manager.
// This function waits for the daemon to terminate, its maximum time is
// ESF_CLOCK_MANAGER_STOP_TIMEOUT milliseconds.

// Args:
//    no arguments.

// Returns:
//    Results.  The following value is returned.
//    kClockManagerSuccess: success.
//    kClockManagerInternalError: internal error.
//    kClockManagerStateTransitionError: status translation failure.

// """
EsfClockManagerReturnValue EsfClockManagerStop(void);

// """Register the given pointer to a callback function.

// The given pointer to a callback function is registered.
// The callback function is called when NTP synchronization has completed.

// Args:
//    on_ntp_sync_complete (void (*)(bool)): a pointer to a callback function
//      which is called when NTP synchronization has completed.

// Returns:
//    Results.  The following value is returned.
//    kClockManagerSuccess: success.
//    kClockManagerParamError: invalid parameter.
//    kClockManagerStateTransitionError: status translation failure.

// """
EsfClockManagerReturnValue EsfClockManagerRegisterCbOnNtpSyncComplete(
    void (*on_ntp_sync_complete)(bool));

// """Unregister the pointer to a callback function.

// The pointer to a callback function which already is registered is made
// unregister.  If a pointer which is already registered does not exist, then
// returns kClockManagerSuccess.

// Args:
//    no arguments

// Returns:
//    Results.  The following value is returned.
//    kClockManagerSuccess: success.
//    kClockManagerStateTransitionError: status translation failure.

// """
EsfClockManagerReturnValue EsfClockManagerUnregisterCbOnNtpSyncComplete(void);

#ifdef __cplusplus
}
#endif

#endif  // ESF_CLOCK_MANAGER_CLOCK_MANAGER_H_
