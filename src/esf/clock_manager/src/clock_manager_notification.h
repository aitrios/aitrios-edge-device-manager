/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESF_CLOCK_MANAGER_NOTIFICATION_H_
#define ESF_CLOCK_MANAGER_NOTIFICATION_H_

#include "clock_manager.h"
#include "clock_manager_utility.h"
#include "pl_clock_manager.h"

typedef enum EsfClockManagerNotificationType {
  kCondTypeIsNothing,
  kCondTypeIsFinReq,
  kCondTypeIsNtpSyncComplete,
  kNumOfNotifications,
} EsfClockManagerNotificationType;

typedef struct EsfClockManagerNotification {
  EsfClockManagerNotificationType m_notification_type;  // notification
  bool m_additional;
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

// This structure prevents a callback function from race condition.
typedef struct EsfClockManagerCallbackFunc {
  struct {
    pthread_mutex_t m_mutex;
  } m_mutex_base;
  void (*m_cb_func)(bool);
} EsfClockManagerCallbackFunc;

typedef enum EsfClockManagerNtpTimeSyncStatus {
  kEsfClockManagerNtpTimeSyncFailure,
  kEsfClockManagerNtpTimeSyncSuccess,
  kEsfClockManagerHasNotCompletedYet
} EsfClockManagerNtpTimeSyncStatus;

EsfClockManagerReturnValue EsfClockManagerInitNotifier(void);

EsfClockManagerReturnValue EsfClockManagerDeinitNotifier(void);

// """Posts an event regarding with notifications.

// Args:
//    notification_type (const PlClockManagerNotificationType): a type of
//      notifications.
//    additional (const bool): additional information.
//
// Returns:
//    The following values are returned:
//    kPlClockManagerSuccess: success.
//    kPlClockManagerInternalError: internal error broke out.
//
// """
EsfClockManagerReturnValue EsfClockManagerPostNotification(
    const EsfClockManagerNotificationType notification_type,
    const bool additional);

// """Gets the next notification from the notification queue.

// This function retrieves and removes the next notification from the head
// of the notification queue in a thread-safe manner. The caller is responsible
// for freeing the returned notification structure.

// Args:
//    no arguments.

// Returns:
//    Pointer to EsfClockManagerNotification if available, NULL if queue is
//    empty. The returned pointer must be freed by the caller.

// """
EsfClockManagerNotification *EsfClockManagerGetNotification(void);

// """Waits for notifications with timeout.

// This function waits for notifications to arrive using a condition variable
// with the specified timeout. It uses pthread_cond_timedwait internally
// and falls back to sleep if mutex operations fail.

// Args:
//    timeout: Timeout value in milliseconds to wait for notifications.

// Returns:
//    no return value (void).

// """
void EsfClockManagerWaitForNotification(
    const EsfClockManagerMillisecondT timeout);

// """Deletes all pending notifications from the queue.

// This function removes and frees all notifications currently in the
// notification queue in a thread-safe manner. It iterates through the
// entire queue, freeing each notification structure and resetting the
// head and tail pointers.

// Args:
//    no arguments.

// Returns:
//    no return value (void).

// """
void EsfClockManagerDeleteAllNotifications(void);

// """Cancels and destroys the notifier thread of Clock Manager.

// This function cancels the notifier thread using pthread_cancel and
// waits for it to complete with pthread_join. It sets the thread ID
// to invalid after successful cancellation. The function is thread-safe
// and handles mutex locking appropriately.

// Args:
//    no arguments.

// Returns:
//    Results. The following value is returned.
//    kClockManagerSuccess: success
//    kClockManagerInternalError: thread cancellation or mutex operation failed

// """
EsfClockManagerReturnValue EsfClockManagerCancelNotifierThread(void);

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

#endif  // ESF_CLOCK_MANAGER_NOTIFICATION_H_
