/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "clock_manager_notification.h"

#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "clock_manager_setting_internal.h"
#include "clock_manager_utility.h"
#include "pl_clock_manager.h"
#include "utility_log.h"
#include "utility_log_module_id.h"

#ifndef CLOCK_MANAGER_REMOVE_STATIC
#define STATIC static
#else  // CLOCK_MANAGER_REMOVE_STATIC
#define STATIC
#endif  // CLOCK_MANAGER_REMOVE_STATIC

#define ESF_CLOCK_MANAGER_INVALID_THREAD_ID ((pthread_t)(-1))

STATIC const char *const kThread2Name = "ClockMgrNotif";

STATIC EsfClockManagerCondNotifier g_cond_for_notifier_with_mutex = {
    .m_cond_base =
        {
            .m_mutex = PTHREAD_MUTEX_INITIALIZER,
            .m_cond = PTHREAD_COND_INITIALIZER,
        },
    .m_notifications =
        {
            .m_head = NULL,
            .m_tail = NULL,
        },
};

STATIC EsfClockManagerThreadId g_notifier_thread_id_with_mutex = {
    .m_mutex_base =
        {
            .m_mutex = PTHREAD_MUTEX_INITIALIZER,
        },
    .m_thread_id = PL_CLOCK_MANAGER_INVALID_THREAD_ID,
};

STATIC EsfClockManagerCallbackFunc g_ntp_sync_complete_cb_func_with_mutex = {
    .m_mutex_base =
        {
            .m_mutex = PTHREAD_MUTEX_INITIALIZER,
        },
    .m_cb_func = NULL,
};

STATIC EsfClockManagerMillisecondT g_sleep_time = 0;

STATIC bool EsfClockManagerProcessNotificationEvent(
    const EsfClockManagerNotification *const notification);
STATIC void *EsfClockManagerNotifierMain(void *arg);

EsfClockManagerReturnValue EsfClockManagerInitNotifier(void) {
  EsfClockManagerCondBaseReturnValue rv = EsfClockManagerInitCondBase(
      &g_cond_for_notifier_with_mutex.m_cond_base.m_cond);
  if (rv != kCondBaseRvIsSuccess) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- EsfClockManagerInitCondBase failed:%d\n",
                     "clock_manager_notification.c", __LINE__, __func__, rv);
    return kClockManagerInternalError;
  }

  return kClockManagerSuccess;
}

EsfClockManagerReturnValue EsfClockManagerDeinitNotifier(void) {
  EsfClockManagerCondBaseReturnValue rv = EsfClockManagerDeinitCondBase(
      &g_cond_for_notifier_with_mutex.m_cond_base.m_cond);
  if (rv != kCondBaseRvIsSuccess) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- EsfClockManagerDeinitCondBase failed:%d\n",
                     "clock_manager_notification.c", __LINE__, __func__, rv);
    return kClockManagerInternalError;
  }

  return kClockManagerSuccess;
}

EsfClockManagerReturnValue EsfClockManagerRegisterNtpSyncCompleteCb(
    void (*on_ntp_sync_complete)(bool)) {
  if (pthread_mutex_lock(
          &g_ntp_sync_complete_cb_func_with_mutex.m_mutex_base.m_mutex) != 0) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- pthread_mutex_lock failed.\n",
                     "clock_manager_notification.c", __LINE__, __func__);
    return kClockManagerInternalError;
  }

  g_ntp_sync_complete_cb_func_with_mutex.m_cb_func = on_ntp_sync_complete;

  if (pthread_mutex_unlock(
          &g_ntp_sync_complete_cb_func_with_mutex.m_mutex_base.m_mutex) != 0) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- pthread_mutex_unlock failed.\n",
                     "clock_manager_notification.c", __LINE__, __func__);
    return kClockManagerInternalError;
  }

  return kClockManagerSuccess;
}

EsfClockManagerReturnValue EsfClockManagerUnregisterNtpSyncCompleteCb(void) {
  if (pthread_mutex_lock(
          &g_ntp_sync_complete_cb_func_with_mutex.m_mutex_base.m_mutex) != 0) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- pthread_mutex_lock failed.\n",
                     "clock_manager_notification.c", __LINE__, __func__);
    return kClockManagerInternalError;
  }

  g_ntp_sync_complete_cb_func_with_mutex.m_cb_func = NULL;

  if (pthread_mutex_unlock(
          &g_ntp_sync_complete_cb_func_with_mutex.m_mutex_base.m_mutex) != 0) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- pthread_mutex_unlock failed.\n",
                     "clock_manager_notification.c", __LINE__, __func__);
    return kClockManagerInternalError;
  }

  return kClockManagerSuccess;
}

EsfClockManagerReturnValue EsfClockManagerDestroyNotifier(void) {
  if (pthread_mutex_lock(
          &g_notifier_thread_id_with_mutex.m_mutex_base.m_mutex) != 0) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- pthread_mutex_lock failed.\n",
                     "clock_manager_notification.c", __LINE__, __func__);
    return kClockManagerInternalError;
  }

  pthread_t thread_id = g_notifier_thread_id_with_mutex.m_thread_id;

  if (pthread_mutex_unlock(
          &g_notifier_thread_id_with_mutex.m_mutex_base.m_mutex) != 0) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- pthread_mutex_unlock failed.\n",
                     "clock_manager_notification.c", __LINE__, __func__);
    return kClockManagerInternalError;
  }

  if (thread_id == PL_CLOCK_MANAGER_INVALID_THREAD_ID) {
    WRITE_DLOG_DEBUG(MODULE_ID_SYSTEM, "%s-%d:%s\n",
                     "clock_manager_notification.c", __LINE__, __func__);
    return kClockManagerSuccess;
  }

  EsfClockManagerReturnValue result =
      EsfClockManagerPostNotification(kCondTypeIsFinReq, false);

  if (result != kClockManagerSuccess) {
    if (result == kClockManagerNotifierHasAlreadyFinished) {
      return kClockManagerSuccess;
    } else {
      WRITE_DLOG_ERROR(
          MODULE_ID_SYSTEM,
          "%s-%d:%s --- EsfClockManagerPostNotification failed:%d\n",
          "clock_manager_notification.c", __LINE__, __func__, result);
      return kClockManagerInternalError;
    }
  }

  WRITE_DLOG_INFO(MODULE_ID_SYSTEM,
                  "%s-%d:%s Wait for notifier thread to finish\n",
                  "clock_manager_notification.c", __LINE__, __func__);

  int rv = pthread_join(thread_id, NULL);
  if (rv != 0) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:%s --- pthread_join failed:%d\n",
                     "clock_manager_notification.c", __LINE__, __func__, rv);
    return kClockManagerInternalError;
  }

  WRITE_DLOG_INFO(MODULE_ID_SYSTEM, "%s-%d:%s Notifier thread has finished\n",
                  "clock_manager_notification.c", __LINE__, __func__);

  rv =
      pthread_mutex_lock(&g_notifier_thread_id_with_mutex.m_mutex_base.m_mutex);
  if (rv != 0) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- pthread_mutex_lock failed:%d\n",
                     "clock_manager_notification.c", __LINE__, __func__, rv);
  }

  g_notifier_thread_id_with_mutex.m_thread_id =
      PL_CLOCK_MANAGER_INVALID_THREAD_ID;

  if (rv == 0) {
    rv = pthread_mutex_unlock(
        &g_notifier_thread_id_with_mutex.m_mutex_base.m_mutex);
    if (rv != 0) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- pthread_mutex_unlock failed:%d\n",
                       "clock_manager_notification.c", __LINE__, __func__, rv);
    }
  }

  EsfClockManagerDeleteAllNotifications();

  return kClockManagerSuccess;
}

STATIC bool EsfClockManagerProcessNotificationEvent(
    const EsfClockManagerNotification *const notification) {
  void (*cb_func)(bool) = NULL;

  {
    // Get callback function with mutex protection
    const int rv = pthread_mutex_lock(
        &g_ntp_sync_complete_cb_func_with_mutex.m_mutex_base.m_mutex);
    if (rv == 0) {
      cb_func = g_ntp_sync_complete_cb_func_with_mutex.m_cb_func;
      if (pthread_mutex_unlock(
              &g_ntp_sync_complete_cb_func_with_mutex.m_mutex_base.m_mutex) !=
          0) {
        WRITE_DLOG_WARN(MODULE_ID_SYSTEM,
                        "%s-%d:%s --- pthread_mutex_unlock failed.\n",
                        "clock_manager_notification.c", __LINE__, __func__);
      }
    } else {
      WRITE_DLOG_WARN(MODULE_ID_SYSTEM,
                      "%s-%d:%s --- pthread_mutex_lock failed:%d.\n",
                      "clock_manager_notification.c", __LINE__, __func__, rv);
    }
  }

  bool is_fin = false;
  switch (notification->m_notification_type) {
    case kCondTypeIsNothing:
      break;
    case kCondTypeIsFinReq:
      is_fin = true;
      break;
    case kCondTypeIsNtpSyncComplete:
      if (cb_func != NULL) {
        const bool is_sync_complete = notification->m_additional;
        if (!is_sync_complete) {
          WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (uint16_t)0x8108);
          WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                           "%s-%d:%s --- NTP time synchronization "
                           "completed with failure.\n",
                           "clock_manager_notification.c", __LINE__, __func__);
        } else {
          WRITE_ELOG_INFO(MODULE_ID_SYSTEM, (uint16_t)0x8109);
          WRITE_DLOG_INFO(MODULE_ID_SYSTEM,
                          "%s-%d:%s --- NTP time synchronization completed "
                          "successfully.\n",
                          "clock_manager_notification.c", __LINE__, __func__);
        }
        if (is_sync_complete) {
          EsfClockManagerSaveParamsInternal();
        }
        cb_func(is_sync_complete);
        EsfClockManagerMarkCompletedSync();
      }
      break;
    default:
      break;
  }

  return is_fin;
}

STATIC void *EsfClockManagerNotifierMain(void *arg) {
  NOT_USED(arg);

  bool is_exit = false;
  WRITE_DLOG_INFO(MODULE_ID_SYSTEM, "%s-%d:%s --- has just started.\n",
                  "clock_manager_notification.c", __LINE__, __func__);

  while (true) {
    EsfClockManagerNotification *notif = EsfClockManagerGetNotification();
    if (notif != NULL) {
      is_exit = EsfClockManagerProcessNotificationEvent(notif);

      free((void *)notif);
      notif = NULL;

      if (is_exit) {
        break;
      }
    } else {
      EsfClockManagerWaitForNotification(g_sleep_time);
    }
  }

  WRITE_DLOG_INFO(MODULE_ID_SYSTEM, "%s-%d:%s --- is about to end.\n",
                  "clock_manager_notification.c", __LINE__, __func__);
  return NULL;
}

EsfClockManagerReturnValue EsfClockManagerPostNotification(
    const EsfClockManagerNotificationType notification_type,
    const bool additional) {
  const int rv =
      pthread_mutex_lock(&g_notifier_thread_id_with_mutex.m_mutex_base.m_mutex);

  if (g_notifier_thread_id_with_mutex.m_thread_id ==
      PL_CLOCK_MANAGER_INVALID_THREAD_ID) {
    WRITE_DLOG_WARN(MODULE_ID_SYSTEM,
                    "%s-%d:%s --- Notifier thread id is invalid.\n",
                    "clock_manager_notification.c", __LINE__, __func__);
    if (!rv) {
      pthread_mutex_unlock(
          &g_notifier_thread_id_with_mutex.m_mutex_base.m_mutex);
    }
    return kClockManagerNotifierHasAlreadyFinished;
  }

  if (!rv) {
    pthread_mutex_unlock(&g_notifier_thread_id_with_mutex.m_mutex_base.m_mutex);
  }

  EsfClockManagerNotification *notif =
      (EsfClockManagerNotification *)malloc(sizeof(*notif));
  if (notif == NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:%s --- malloc failed.\n",
                     "clock_manager_notification.c", __LINE__, __func__);
    return kClockManagerInternalError;
  }

  notif->m_notification_type = notification_type;
  notif->m_additional = additional;
  notif->m_next = NULL;

  const int rv2 =
      pthread_mutex_lock(&g_cond_for_notifier_with_mutex.m_cond_base.m_mutex);
  const int rv_notif =
      pthread_mutex_lock(&g_notifier_thread_id_with_mutex.m_mutex_base.m_mutex);
  if (g_notifier_thread_id_with_mutex.m_thread_id !=
      PL_CLOCK_MANAGER_INVALID_THREAD_ID) {
    if (g_cond_for_notifier_with_mutex.m_notifications.m_tail == NULL) {
      g_cond_for_notifier_with_mutex.m_notifications.m_tail = notif;
      g_cond_for_notifier_with_mutex.m_notifications.m_head = notif;
    } else {
      g_cond_for_notifier_with_mutex.m_notifications.m_tail->m_next = notif;
      g_cond_for_notifier_with_mutex.m_notifications.m_tail = notif;
    }

    pthread_cond_signal(&g_cond_for_notifier_with_mutex.m_cond_base.m_cond);

    if (!rv_notif) {
      pthread_mutex_unlock(
          &g_notifier_thread_id_with_mutex.m_mutex_base.m_mutex);
    }
    if (!rv2) {
      pthread_mutex_unlock(&g_cond_for_notifier_with_mutex.m_cond_base.m_mutex);
    }

  } else {
    if (!rv_notif) {
      pthread_mutex_unlock(
          &g_notifier_thread_id_with_mutex.m_mutex_base.m_mutex);
    }
    if (!rv2) {
      pthread_mutex_unlock(&g_cond_for_notifier_with_mutex.m_cond_base.m_mutex);
    }
    free(notif);
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:%s --- malloc is OK, but Notifier thread id is invalid.\n",
        "clock_manager_notification.c", __LINE__, __func__);
    return kClockManagerNotifierHasAlreadyFinished;
  }

  return kClockManagerSuccess;
}

EsfClockManagerNotification *EsfClockManagerGetNotification(void) {
  EsfClockManagerNotification *notif = NULL;

  int rv =
      pthread_mutex_lock(&g_cond_for_notifier_with_mutex.m_cond_base.m_mutex);
  if (rv) {
    WRITE_DLOG_WARN(MODULE_ID_SYSTEM,
                    "%s-%d:%s --- pthread_mutex_lock failed:%d.\n",
                    "clock_manager_notification.c", __LINE__, __func__, rv);
  }

  if (g_cond_for_notifier_with_mutex.m_notifications.m_head != NULL) {
    notif = g_cond_for_notifier_with_mutex.m_notifications.m_head;

    g_cond_for_notifier_with_mutex.m_notifications.m_head = notif->m_next;
    if (notif->m_next == NULL) {
      g_cond_for_notifier_with_mutex.m_notifications.m_tail = NULL;
    }
  }

  if (!rv) {
    pthread_mutex_unlock(&g_cond_for_notifier_with_mutex.m_cond_base.m_mutex);
  }

  return notif;
}

void EsfClockManagerDeleteAllNotifications(void) {
  int rv =
      pthread_mutex_lock(&g_cond_for_notifier_with_mutex.m_cond_base.m_mutex);
  if (rv) {
    WRITE_DLOG_WARN(MODULE_ID_SYSTEM,
                    "%s-%d:%s --- pthread_mutex_lock failed:%d.\n",
                    "clock_manager_notification.c", __LINE__, __func__, rv);
  }

  while (g_cond_for_notifier_with_mutex.m_notifications.m_head != NULL) {
    EsfClockManagerNotification *box =
        g_cond_for_notifier_with_mutex.m_notifications.m_head;
    g_cond_for_notifier_with_mutex.m_notifications.m_head = box->m_next;
    free((void *)box);
    box = NULL;
  }
  g_cond_for_notifier_with_mutex.m_notifications.m_tail = NULL;

  if (!rv) {
    pthread_mutex_unlock(&g_cond_for_notifier_with_mutex.m_cond_base.m_mutex);
  }
}

void EsfClockManagerWaitForNotification(
    const EsfClockManagerMillisecondT timeout) {
  struct timespec abs_time = {0};
  bool calc_time = EsfClockManagerCalculateAbstimeInMonotonic(&abs_time,
                                                              g_sleep_time);

  int rv =
      pthread_mutex_lock(&g_cond_for_notifier_with_mutex.m_cond_base.m_mutex);

  if ((rv == 0) && calc_time) {
    pthread_cond_timedwait(&g_cond_for_notifier_with_mutex.m_cond_base.m_cond,
                           &g_cond_for_notifier_with_mutex.m_cond_base.m_mutex,
                           &abs_time);

  } else {
    WRITE_DLOG_WARN(MODULE_ID_SYSTEM,
                    "%s-%d:%s --- pthread_mutex_lock failed.\n",
                    "clock_manager_notification.c", __LINE__, __func__);
    sleep((unsigned int)(g_sleep_time / 1000U));
  }

  if (rv == 0) {
    if (pthread_mutex_unlock(
            &g_cond_for_notifier_with_mutex.m_cond_base.m_mutex)) {
      WRITE_DLOG_WARN(MODULE_ID_SYSTEM,
                      "%s-%d:%s --- pthread_mutex_unlock failed.\n",
                      "clock_manager_notification.c", __LINE__, __func__);
    }
  }
}

EsfClockManagerReturnValue EsfClockManagerCreateNotifierThread(
    EsfClockManagerMillisecondT surveillance_period) {
  g_sleep_time = surveillance_period;

  if (pthread_mutex_lock(
          &g_notifier_thread_id_with_mutex.m_mutex_base.m_mutex)) {
    WRITE_DLOG_CRITICAL(MODULE_ID_SYSTEM,
                        "%s-%d:%s --- pthread_mutex_lock failed.\n",
                        "clock_manager_notification.c", __LINE__, __func__);
    return kClockManagerStateTransitionError;
  }

  if (g_notifier_thread_id_with_mutex.m_thread_id !=
      PL_CLOCK_MANAGER_INVALID_THREAD_ID) {
    pthread_mutex_unlock(&g_notifier_thread_id_with_mutex.m_mutex_base.m_mutex);
    WRITE_DLOG_WARN(MODULE_ID_SYSTEM,
                    "%s-%d:%s --- The thread already exists:%lu\n",
                    "clock_manager_notification.c", __LINE__, __func__,
                    g_notifier_thread_id_with_mutex.m_thread_id);
    return kClockManagerStateTransitionError;
  }

  pthread_attr_t thread_attr;
  (void)pthread_attr_init(&thread_attr);
  const int rv = pthread_create(&(g_notifier_thread_id_with_mutex.m_thread_id),
                                &thread_attr, EsfClockManagerNotifierMain,
                                NULL);

  if (rv) {
    pthread_mutex_unlock(&g_notifier_thread_id_with_mutex.m_mutex_base.m_mutex);
    WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (uint16_t)0x8104);
    WRITE_DLOG_CRITICAL(
        MODULE_ID_SYSTEM, "%s-%d:%s --- pthread_create failed:%d,errno:%d\n",
        "clock_manager_notification.c", __LINE__, __func__, rv, errno);
    return kClockManagerStateTransitionError;
  }
  pthread_setname_np(g_notifier_thread_id_with_mutex.m_thread_id, kThread2Name);

  pthread_mutex_unlock(&g_notifier_thread_id_with_mutex.m_mutex_base.m_mutex);

  return kClockManagerSuccess;
}

EsfClockManagerReturnValue EsfClockManagerCancelNotifierThread(void) {
  if (g_notifier_thread_id_with_mutex.m_thread_id !=
      PL_CLOCK_MANAGER_INVALID_THREAD_ID) {
    int rv = pthread_cancel(g_notifier_thread_id_with_mutex.m_thread_id);
    WRITE_DLOG_INFO(MODULE_ID_SYSTEM, "%s-%d:%s --- pthread_cancel:%d\n",
                    "clock_manager_notification.c", __LINE__, __func__, rv);
    if (!rv) {
      rv = pthread_join(g_notifier_thread_id_with_mutex.m_thread_id, NULL);
      g_notifier_thread_id_with_mutex.m_thread_id =
          PL_CLOCK_MANAGER_INVALID_THREAD_ID;
    }
  }

  return kClockManagerSuccess;
}
