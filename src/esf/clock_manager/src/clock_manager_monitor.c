/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "clock_manager_monitor.h"

#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "clock_manager.h"
#include "clock_manager_notification.h"
#include "clock_manager_setting_internal.h"
#include "clock_manager_utility.h"
#include "utility_log.h"
#include "utility_log_module_id.h"

#ifndef CLOCK_MANAGER_REMOVE_STATIC
#define STATIC static
#else  // CLOCK_MANAGER_REMOVE_STATIC
#define STATIC
#endif  // CLOCK_MANAGER_REMOVE_STATIC

#ifndef INVALID_PROCESS_ID
#define INVALID_PROCESS_ID ((pid_t)(-1))
#endif

STATIC const char *const kThreadName = "ClockMgrNcMonit";

STATIC EsfClockManagerMillisecondT g_monitor_sleep_time = 0;

STATIC EsfClockManagerThreadId g_monitor_thread_id_with_mutex = {
    .m_mutex_base =
        {
            .m_mutex = PTHREAD_MUTEX_INITIALIZER,
        },
    .m_thread_id = PL_CLOCK_MANAGER_INVALID_THREAD_ID,
};

STATIC EsfClockManagerCondMonitor g_cond_for_monitor_with_mutex = {
    .m_cond_base =
        {
            .m_mutex = PTHREAD_MUTEX_INITIALIZER,
            .m_cond = PTHREAD_COND_INITIALIZER,
        },
    .m_req_fin = false,
};

STATIC void *EsfClockManagerMonitorThreadMain(void *arg);

EsfClockManagerReturnValue EsfClockManagerInitMonitor(void) {
  if (pthread_mutex_lock(&g_cond_for_monitor_with_mutex.m_cond_base.m_mutex)) {
    WRITE_DLOG_CRITICAL(MODULE_ID_SYSTEM,
                        "%s-%d:%s --- pthread_mutex_lock failed.\n",
                        "clock_manager_monitor.c", __LINE__, __func__);
    return kClockManagerInternalError;
  }

  g_cond_for_monitor_with_mutex.m_req_fin = false;

  if (pthread_mutex_unlock(
          &g_cond_for_monitor_with_mutex.m_cond_base.m_mutex)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- pthread_mutex_unlock failed.\n",
                     "clock_manager_monitor.c", __LINE__, __func__);
    return kClockManagerInternalError;
  }

  EsfClockManagerCondBaseReturnValue rv = EsfClockManagerInitCondBase(
      &g_cond_for_monitor_with_mutex.m_cond_base.m_cond);
  if (rv != kCondBaseRvIsSuccess) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- EsfClockManagerInitCondBase failed:%d\n",
                     "clock_manager_monitor.c", __LINE__, __func__, rv);
    return kClockManagerInternalError;
  }

  return kClockManagerSuccess;
}

EsfClockManagerReturnValue EsfClockManagerDeinitMonitor(void) {
  EsfClockManagerCondBaseReturnValue rv = EsfClockManagerDeinitCondBase(
      &g_cond_for_monitor_with_mutex.m_cond_base.m_cond);
  if (rv != kCondBaseRvIsSuccess) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- EsfClockManagerDeinitCondBase failed:%d\n",
                     "clock_manager_monitor.c", __LINE__, __func__, rv);
    return kClockManagerInternalError;
  }

  return kClockManagerSuccess;
}

EsfClockManagerReturnValue EsfClockManagerCreateMonitorThread(
    EsfClockManagerMillisecondT surveillance_period) {
  if (pthread_mutex_lock(
          &g_monitor_thread_id_with_mutex.m_mutex_base.m_mutex)) {
    WRITE_DLOG_CRITICAL(MODULE_ID_SYSTEM,
                        "%s-%d:%s --- pthread_mutex_lock failed.\n",
                        "clock_manager_monitor.c", __LINE__, __func__);
    return kClockManagerStateTransitionError;
  }

  if (g_monitor_thread_id_with_mutex.m_thread_id !=
      PL_CLOCK_MANAGER_INVALID_THREAD_ID) {
    WRITE_DLOG_WARN(MODULE_ID_SYSTEM,
                    "%s-%d:%s --- The thread already exists:%lu\n",
                    "clock_manager_monitor.c", __LINE__, __func__,
                    g_monitor_thread_id_with_mutex.m_thread_id);
    (void)pthread_mutex_unlock(
        &g_monitor_thread_id_with_mutex.m_mutex_base.m_mutex);
    return kClockManagerStateTransitionError;
  }

  const EsfClockManagerMillisecondT sleep_time_tmp = g_monitor_sleep_time;
  g_monitor_sleep_time = surveillance_period;
  g_cond_for_monitor_with_mutex.m_req_fin = false;

  const int rv = pthread_create(&(g_monitor_thread_id_with_mutex.m_thread_id),
                                NULL, EsfClockManagerMonitorThreadMain, NULL);

  if (rv) {
    g_monitor_sleep_time = sleep_time_tmp;
    (void)pthread_mutex_unlock(
        &g_monitor_thread_id_with_mutex.m_mutex_base.m_mutex);
    WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (uint16_t)0x8103);
    WRITE_DLOG_CRITICAL(
        MODULE_ID_SYSTEM, "%s-%d:%s --- pthread_create failed:%d,errno:%d\n",
        "clock_manager_monitor.c", __LINE__, __func__, rv, errno);
    return kClockManagerStateTransitionError;
  }
  pthread_setname_np(g_monitor_thread_id_with_mutex.m_thread_id, kThreadName);

  if (pthread_mutex_unlock(
          &g_monitor_thread_id_with_mutex.m_mutex_base.m_mutex)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- pthread_mutex_unlock failed.\n",
                     "clock_manager_monitor.c", __LINE__, __func__);
    return kClockManagerStateTransitionError;
  }

  return kClockManagerSuccess;
}

EsfClockManagerReturnValue EsfClockManagerDestroyMonitorThread(void) {
  int rv =
      pthread_mutex_lock(&g_cond_for_monitor_with_mutex.m_cond_base.m_mutex);
  if (rv != 0) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- failure:pthread_mutex_lock:%d\n",
                     "clock_manager_monitor.c", __LINE__, __func__, rv);
    return kClockManagerInternalError;
  }

  g_cond_for_monitor_with_mutex.m_req_fin = true;

  rv = pthread_cond_signal(&g_cond_for_monitor_with_mutex.m_cond_base.m_cond);
  if (rv != 0) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- failure:pthread_cond_signal:%d\n",
                     "clock_manager_monitor.c", __LINE__, __func__, rv);
    (void)pthread_mutex_unlock(
        &g_cond_for_monitor_with_mutex.m_cond_base.m_mutex);
    return kClockManagerInternalError;
  }

  rv = pthread_mutex_unlock(&g_cond_for_monitor_with_mutex.m_cond_base.m_mutex);
  if (rv != 0) {
    WRITE_DLOG_CRITICAL(MODULE_ID_SYSTEM,
                        "%s-%d:%s --- pthread_mutex_unlock failed:%d.\n",
                        "clock_manager_monitor.c", __LINE__, __func__, rv);
    return kClockManagerInternalError;
  }

  WRITE_DLOG_INFO(MODULE_ID_SYSTEM,
                  "%s-%d:%s Wait for monitor thread to finish\n",
                  "clock_manager_monitor.c", __LINE__, __func__);

  rv = pthread_join(g_monitor_thread_id_with_mutex.m_thread_id, NULL);
  if (rv != 0) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:%s --- pthread_join failed:%d\n",
                     "clock_manager_monitor.c", __LINE__, __func__, rv);
    return kClockManagerInternalError;
  }

  WRITE_DLOG_INFO(MODULE_ID_SYSTEM, "%s-%d:%s Monitor thread has finished\n",
                  "clock_manager_monitor.c", __LINE__, __func__);

  rv = pthread_mutex_lock(&g_monitor_thread_id_with_mutex.m_mutex_base.m_mutex);
  if (rv != 0) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- pthread_mutex_lock failed:%d\n",
                     "clock_manager_monitor.c", __LINE__, __func__, rv);
  }

  g_monitor_thread_id_with_mutex.m_thread_id =
      PL_CLOCK_MANAGER_INVALID_THREAD_ID;

  if (rv == 0) {
    rv = pthread_mutex_unlock(
        &g_monitor_thread_id_with_mutex.m_mutex_base.m_mutex);
    if (rv != 0) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- pthread_mutex_unlock failed:%d\n",
                       "clock_manager_monitor.c", __LINE__, __func__, rv);
    }
  }

  rv = pthread_mutex_lock(&g_cond_for_monitor_with_mutex.m_cond_base.m_mutex);
  if (rv != 0) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- pthread_mutex_lock failed:%d\n",
                     "clock_manager_monitor.c", __LINE__, __func__, rv);
  }

  g_cond_for_monitor_with_mutex.m_req_fin = false;

  if (rv == 0) {
    rv = pthread_mutex_unlock(
        &g_cond_for_monitor_with_mutex.m_cond_base.m_mutex);
    if (rv != 0) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- pthread_mutex_unlock failed:%d\n",
                       "clock_manager_monitor.c", __LINE__, __func__, rv);
    }
  }

  return kClockManagerSuccess;
}

EsfClockManagerReturnValue EsfClockManagerCancelMonitorThread(void) {
  int rv =
      pthread_mutex_lock(&g_monitor_thread_id_with_mutex.m_mutex_base.m_mutex);
  if (rv != 0) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- pthread_mutex_lock failed:%d\n",
                     "clock_manager_monitor.c", __LINE__, __func__, rv);
    return kClockManagerInternalError;
  }

  if (g_monitor_thread_id_with_mutex.m_thread_id !=
      PL_CLOCK_MANAGER_INVALID_THREAD_ID) {
    rv = pthread_cancel(g_monitor_thread_id_with_mutex.m_thread_id);
    WRITE_DLOG_INFO(MODULE_ID_SYSTEM, "%s-%d:%s --- pthread_cancel:%d\n",
                    "clock_manager_monitor.c", __LINE__, __func__, rv);
    if (rv != 0) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- pthread_cancel failed:%d\n",
                       "clock_manager_monitor.c", __LINE__, __func__, rv);
      (void)pthread_mutex_unlock(
          &g_monitor_thread_id_with_mutex.m_mutex_base.m_mutex);
      return kClockManagerInternalError;
    }

    rv = pthread_join(g_monitor_thread_id_with_mutex.m_thread_id, NULL);
    if (rv != 0) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- pthread_join failed:%d\n",
                       "clock_manager_monitor.c", __LINE__, __func__, rv);
      (void)pthread_mutex_unlock(
          &g_monitor_thread_id_with_mutex.m_mutex_base.m_mutex);
      return kClockManagerInternalError;
    }

    g_monitor_thread_id_with_mutex.m_thread_id =
        PL_CLOCK_MANAGER_INVALID_THREAD_ID;
  }

  rv = pthread_mutex_unlock(
      &g_monitor_thread_id_with_mutex.m_mutex_base.m_mutex);
  if (rv != 0) {
    WRITE_DLOG_CRITICAL(MODULE_ID_SYSTEM,
                        "%s-%d:%s --- pthread_mutex_unlock failed:%d.\n",
                        "clock_manager_monitor.c", __LINE__, __func__, rv);
    return kClockManagerInternalError;
  }

  return kClockManagerSuccess;
}

STATIC void *EsfClockManagerMonitorThreadMain(void *arg) {
  (void)(arg);
  struct timespec abs_time = {0};
  WRITE_DLOG_INFO(MODULE_ID_SYSTEM, "%s-%d:%s --- has just started.\n",
                  "clock_manager_monitor.c", __LINE__, __func__);

  bool has_complete_notif_been_posted = false;
  PlClockManagerNtpStatus ntp_status = NULL;
  bool exit_flag = false;
  unsigned int num_of_errors = 0;
  unsigned int ntp_error_time = CLOCK_MANAGER_NTP_ERROR_TIME;

  while (true) {
    if (PlClockManagerIsNtpClientDaemonActive() == false) {
      WRITE_DLOG_WARN(MODULE_ID_SYSTEM,
                      "%s-%d:%s --- NTP client daemon is dead. Restarting...\n",
                      "clock_manager_monitor.c", __LINE__, __func__);
      WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (uint16_t)0x8106);

      PlClockManagerReturnValue pl_ret = PlClockManagerRestartNtpClientDaemon();
      if (pl_ret != kPlClockManagerSuccess) {
        WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                         "%s-%d:%s --- Restart NTP client daemon failed:%d\n",
                         "clock_manager_monitor.c", __LINE__, __func__, pl_ret);
        WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (uint16_t)0x8107);
      } else {
        WRITE_DLOG_INFO(MODULE_ID_SYSTEM,
                        "%s-%d:%s --- Restart NTP client daemon succeeded.\n",
                        "clock_manager_monitor.c", __LINE__, __func__);
      }
    }

    ntp_status = PlClockManagerGetNtpStatus();

    const PlClockManagerNtpTimeSyncStatus sync_status =
        PlClockManagerJudgeNtpTimeSyncComplete(ntp_status);

    if (sync_status == kPlClockManagerNtpTimeSyncSuccess) {
      num_of_errors = 0;
      if (!has_complete_notif_been_posted) {
        EsfClockManagerReturnValue ret =
            EsfClockManagerPostNotification(kCondTypeIsNtpSyncComplete, true);
        if (ret != kClockManagerSuccess) {
          WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                           "%s-%d:%s --- EsfClockManagerPostNotification "
                           "failed:%d\n",
                           "clock_manager_monitor.c", __LINE__, __func__, ret);
        } else {
          has_complete_notif_been_posted = true;
        }
      }
    } else {
      num_of_errors++;
    }

    free(ntp_status);
    ntp_status = NULL;

    {  // Check if the total error time exceeds the NTP error time
      unsigned int total_error_time =
          num_of_errors * (unsigned int)(g_monitor_sleep_time / 1000U);
      if (total_error_time >= ntp_error_time) {
        WRITE_DLOG_WARN(
            MODULE_ID_SYSTEM, "%s-%d:%s --- NTP sync failed for %d seconds. \n",
            "clock_manager_monitor.c", __LINE__, __func__, total_error_time);
        WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (uint16_t)0x810D);
        num_of_errors = 0;
      }
    }

    bool calc_time = EsfClockManagerCalculateAbstimeInMonotonic(
        &abs_time, g_monitor_sleep_time);

    {
      // Wait for the condition variable to be signaled or timeout
      int rv = pthread_mutex_lock(
          &g_cond_for_monitor_with_mutex.m_cond_base.m_mutex);
      if (rv != 0) {
        WRITE_DLOG_WARN(MODULE_ID_SYSTEM,
                        "%s-%d:%s --- pthread_mutex_lock failed:%d\n",
                        "clock_manager_monitor.c", __LINE__, __func__, rv);
      }

      if ((rv == 0) && calc_time) {
        // If the mutex was successfully locked
        // Wait for the condition variable to be signaled or timeout
        (void)pthread_cond_timedwait(
            &g_cond_for_monitor_with_mutex.m_cond_base.m_cond,
            &g_cond_for_monitor_with_mutex.m_cond_base.m_mutex, &abs_time);
      } else {
        sleep((unsigned int)(g_monitor_sleep_time / 1000U));
      }

      if (g_cond_for_monitor_with_mutex.m_req_fin) {
        exit_flag = true;
      }

      if (rv == 0) {
        // If the mutex was successfully locked
        // Unlock the mutex before exiting
        rv = pthread_mutex_unlock(
            &g_cond_for_monitor_with_mutex.m_cond_base.m_mutex);
        if (rv != 0) {
          WRITE_DLOG_WARN(MODULE_ID_SYSTEM,
                          "%s-%d:%s --- pthread_mutex_unlock failed:%d\n",
                          "clock_manager_monitor.c", __LINE__, __func__, rv);
        }
      }

      if (exit_flag) {
        break;
      }
    }
  }

  WRITE_DLOG_INFO(MODULE_ID_SYSTEM, "%s-%d:%s --- is about to end.\n",
                  "clock_manager_monitor.c", __LINE__, __func__);
  return NULL;
}
