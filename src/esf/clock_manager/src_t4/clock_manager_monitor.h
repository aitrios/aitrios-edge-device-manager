/*
 * SPDX-FileCopy    bool m_req_fin;  // true is thread-fin-request.
} EsfClockManagerCondMonitor;

// """Initializes the monitor subsystem.

// This function initializes the monitor thread ID and finish request flag
// to their default values. It sets the thread ID to invalid and the
// finish request flag to false.

// Args:
//    no arguments.

// Returns:
//    Results. The following value is returned.
//    kClockManagerSuccess: success

// """
EsfClockManagerReturnValue EsfClockManagerInitMonitor(void);tText: 2024-2025
Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESF_CLOCK_MANAGER_MONITOR_H_
#define ESF_CLOCK_MANAGER_MONITOR_H_

/**
 * Definitions of macros
 */

#define CLOCK_MANAGER_NTP_ERROR_TIME \
  CONFIG_EXTERNAL_CLOCK_MANAGER_NTP_ERROR_TIME

#include <pthread.h>
#include <stdint.h>

#include "clock_manager.h"
#include "clock_manager_setting.h"
#include "clock_manager_utility.h"

// This structure prevents a condition variable from race condition.
typedef struct EsfClockManagerCondMonitor {
  PlClockManagerCondBase m_cond_base;
  bool m_req_fin;  // true is thread-fin-request.
} EsfClockManagerCondMonitor;

// """Initializes the monitor subsystem.

// This function initializes the monitor thread ID and finish request flag
// to their default values. It sets the thread ID to invalid and the
// finish request flag to false.

// Args:
//    no arguments.

// Returns:
//    Results. The following value is returned.
//    kClockManagerSuccess: success

// """
EsfClockManagerReturnValue EsfClockManagerInitMonitor(void);

// """Deinitializes the monitor subsystem.

// This function resets the monitor thread ID and finish request flag
// to their default values. It sets the thread ID to invalid and the
// finish request flag to false, similar to initialization.

// Args:
//    no arguments.

// Returns:
//    Results. The following value is returned.
//    kClockManagerSuccess: success

// """
EsfClockManagerReturnValue EsfClockManagerDeinitMonitor(void);

// """Creates and starts the monitor thread.

// This function creates a new monitor thread that periodically checks
// NTP synchronization status and posts notifications when sync completes.
// The thread runs EsfClockManagerMonitorThreadMain and uses the specified
// surveillance period for polling intervals.

// Args:
//    surveillance_period: Time interval in milliseconds between NTP status
//    checks.

// Returns:
//    Results. The following value is returned.
//    kClockManagerSuccess: success
//    kClockManagerStateTransitionError: thread creation failed or thread
//    already exists

// """
EsfClockManagerReturnValue EsfClockManagerCreateMonitorThread(
    EsfClockManagerMillisecondT surveillance_period);

// """Requests termination of the monitor thread.

// This function signals the monitor thread to terminate by setting the
// finish request flag to true and signaling the condition variable.
// The thread will check this flag in its main loop and exit gracefully.
// This is a non-blocking operation that only requests termination.

// Args:
//    no arguments.

// Returns:
//    Results. The following value is returned.
//    kClockManagerSuccess: success

// """
EsfClockManagerReturnValue EsfClockManagerDestroyMonitorThread(void);

// """Cancels and forcibly terminates the monitor thread.

// This function forcibly cancels the monitor thread using pthread_cancel
// and waits for it to complete with pthread_join. Unlike DestroyMonitorThread
// which requests graceful termination, this function immediately cancels
// the thread execution. The thread ID is set to invalid after cancellation.

// Args:
//    no arguments.

// Returns:
//    Results. The following value is returned.
//    kClockManagerSuccess: success
//    kClockManagerInternalError: thread cancellation or join failed

// """
EsfClockManagerReturnValue EsfClockManagerCancelMonitorThread(void);

#endif  // ESF_CLOCK_MANAGER_MONITOR_H_
