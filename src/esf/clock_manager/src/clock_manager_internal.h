/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESF_CLOCK_MANAGER_INTERNAL_H_
#define ESF_CLOCK_MANAGER_INTERNAL_H_

#include <pthread.h>
#include <stdint.h>

#include "clock_manager.h"
#include "clock_manager_setting.h"
#include "clock_manager_utility.h"

/**
 * Definitions of enumerations
 */

typedef enum EsfClockManagerNotificationType {
  kCondTypeIsNothing,
  kCondTypeIsFinReq,
  kCondTypeIsNtpSyncComplete,
  kNumOfNotifications,
} EsfClockManagerNotificationType;

typedef enum EsfClockManagerNtpTimeSyncStatus {
  kNtpTimeSyncFailure,
  kNtpTimeSyncSuccess,
  kHasNotCompletedYet
} EsfClockManagerNtpTimeSyncStatus;

/**
 * Definitions of structures
 */

// This structure prevents a thread id of the thread from race condition.
typedef struct EsfClockManagerThreadId {
  EsfClockManagerMutexBase m_mutex_base;  // mutex
  pthread_t m_thread_id;                  // thread id
} EsfClockManagerThreadId;

// This structure prevents a condition variable from race condition.
typedef struct EsfClockManagerCondMonitor {
  EsfClockManagerCondBase m_cond_base;
  bool m_req_fin;  // true is thread-fin-request.
} EsfClockManagerCondMonitor;

// This structure prevents a condition variable from race condition.
typedef struct EsfClockManagerCondDaemon {
  EsfClockManagerCondBase m_cond_base;
  bool m_is_daemon_fin;  // true is that NTP client daemon has terminated.
} EsfClockManagerCondDaemon;

typedef struct EsfClockManagerNotification {
  EsfClockManagerNotificationType m_notification_type;  // notification
  int m_additional;
  struct EsfClockManagerNotification *m_next;
} EsfClockManagerNotification;

typedef struct EsfClockManagerNotifications {
  EsfClockManagerNotification *m_head;
  EsfClockManagerNotification *m_tail;
} EsfClockManagerNotifications;

// This structure prevents a condition variable from race condition.
typedef struct EsfClockManagerCondNotifier {
  EsfClockManagerCondBase m_cond_base;
  EsfClockManagerNotifications m_notifications;
} EsfClockManagerCondNotifier;

/**
 * Declarations of package private functions
 */

// """Creates and initializes objects in clock_manager_internal.c

// This function creates objects, then initializes them.

// Args:
//    no arguments

// Returns:
//    The following values are returned:
//    kClockManagerSuccess: success.
//    kClockManagerInternalError: internal error broke out.

// """
EsfClockManagerReturnValue EsfClockManagerInitInternal(void);

// """Deinitializes and frees objects in clock_manager_internal.c

// This function deinitializes objects, then frees them.

// Args:
//    no arguments

// Returns:
//    The following values are returned:
//    kClockManagerSuccess: success.
//    kClockManagerInternalError: internal error broke out.

// """
EsfClockManagerReturnValue EsfClockManagerDeinitInternal(void);

// """Register the given pointer to a callback function.

// The given pointer to a callback function is registered to the global variable
// which has the storage-class specifier static.
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
EsfClockManagerReturnValue EsfClockManagerRegisterNtpSyncCompleteCb(
    void (*on_ntp_sync_complete)(bool));

// """Unregister the pointer to a callback function.

// The pointer to a callback function which already is registered is made
// unregister.  `Unregister'is that the global variable which has the
// storage-class specifier static is substituted NULL for.
// If a pointer which is already registered does not exist, then returns
// kClockManagerSuccess.

// Args:
//    no arguments

// Returns:
//    Results.  The following value is returned.
//    kClockManagerSuccess: success.
//    kClockManagerStateTransitionError: status translation failure.

// """
EsfClockManagerReturnValue EsfClockManagerUnregisterNtpSyncCompleteCb(void);

// """Creates a single thread regarding with notification in Clock Manager.

// It is created that a single thread to call the pointer to the callback
// function which is registered to the global variable which has storage-class
// specifier static.

// Args:
//    no arguments.

// Returns:
//    Results.  The following value is returned.
//    kClockManagerStateTransitionError: no thread has been created.
//    kClockManagerSuccess: success.

// """
EsfClockManagerReturnValue EsfClockManagerCreateNotifierThread(void);

// """Destroys the notifier thread of Clock Manager.

// This function destroys the thread which calls the callback function which
// is registered to the global variable which has storage-class specifier
// static.

// Args:
//    no arguments.

// Returns:
//    Results.  The following value is returned.
//    kClockManagerSuccess: success
//    kClockManagerParamError: invalid parameter
//    kClockManagerStateTransitionError: status translation failure

// """
EsfClockManagerReturnValue EsfClockManagerDestroyNotifier(void);

// """Destroys the thread of Clock Manager forcibly.

// This function forcibly destroys the thread (pthread) of Clock Manager.

// Args:
//    no arguments.

// Returns:
//    Results.  The following value is returned.
//    int 0: stands for success.

// """
int EsfClockManagerCancelMonitorThread(void);

// """Destroys the notifier thread of Clock Manager forcibly.

// This function forcibly destroys the thread which calls the callback function
// which is registered to the global variable which has storage-class specifier
// static.

// Args:
//    no arguments.

// Returns:
//    Results.  The following value is returned.
//    int 0: stands for success.

// """
int EsfClockManagerCancelNotifierThread(void);

/**
 * Declarations of macros
 */

#define ESF_CLOCK_MANAGER_INVALID_THREAD_ID ((pthread_t)(-1))

/**
 * Declarations of package private functions
 */

// """Creates a thread of Clock Manager.

// This function creates a thread (pthread) of Clock Manager.

// Args:
//    surveillance_period (const EsfClockManagerMillisecondT): surveillance
//      period.

// Returns:
//    Results.  The following value is returned.
//    kClockManagerStateTransitionError: no thread has been created.
//    kClockManagerSuccess: success.

// """
EsfClockManagerReturnValue EsfClockManagerCreateMonitorThread(
    const EsfClockManagerMillisecondT surveillance_period);

// """Creates NTP client daemon and starts it.

// This function creates a NTP client daemon, then starts it.

// Args:
//    param (const EsfClockManagerParam *): parameters which pass to ntpclient.

// Returns:
//    Results.  The following value is returned.
//    kClockManagerSuccess: success
//    kClockManagerParamError: invalid parameter
//    kClockManagerStateTransitionError: status translation failure

// """
EsfClockManagerReturnValue EsfClockManagerStartNtpClientDaemon(
    const EsfClockManagerParams *param);

// """Destroys the thread of Clock Manager.

// This function creates the thread (pthread) of Clock Manager.

// Args:
//    no arguments.

// Returns:
//    Results.  The following value is returned.
//    kClockManagerSuccess: success
//    kClockManagerParamError: invalid parameter
//    kClockManagerStateTransitionError: status translation failure

// """
EsfClockManagerReturnValue EsfClockManagerDestroyMonitorThread(void);

// """Waits for terminating the NTP client daemon.

// This function waits for terminating the NTP client daemon.
// The maximum time is ESF_CLOCK_MANAGER_STOP_TIMEOUT(ms).

// Args:
//    no arguments.

// Returns:
//    Results.  The following value is returned.
//    kClockManagerSuccess: success
//    kClockManagerStateTransitionError: status translation failure

// """
EsfClockManagerReturnValue EsfClockManagerWaitForTerminatingDaemon(void);

#endif  // ESF_CLOCK_MANAGER_INTERNAL_H_
