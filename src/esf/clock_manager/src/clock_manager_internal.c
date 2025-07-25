/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "clock_manager_internal.h"

#include <netutils/ntpclient.h>
#ifdef __NuttX__
#include <nuttx/config.h>
#include <nuttx/sched.h>
#else
#define INVALID_PROCESS_ID ((pid_t)(-1))
#endif  // __NuttX__

#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "clock_manager_setting_internal.h"
#include "clock_manager_utility.h"
#include "utility_log.h"
#include "utility_log_module_id.h"

#ifndef CLOCK_MANAGER_REMOVE_STATIC
#define STATIC static
#else  // CLOCK_MANAGER_REMOVE_STATIC
#define STATIC
#endif  // CLOCK_MANAGER_REMOVE_STATIC

/**
 * Declarations of private functions --- i.e., functions given storage-class
 * specifier static.
 */

// """Stops the NTP client daemon, then destroys it.

// This function stops the NTP client daemon, then destroys it.

// Args:
//    no arguments.

// Returns:
//    Results.  The following value is returned.
//    kClockManagerSuccess: success
//    kClockManagerStateTransitionError: status translation failure

// """
STATIC EsfClockManagerReturnValue EsfClockManagerStopNtpClientDaemon(void);

// """Entry-point function for a pthread of Clock Manager.

// This function is given to the 3rd parameter of POSIX
// pthread_create( , , tart_routine, ).  If the pthread starts, then this
// function is called.

// Args:
//    arg (void *): The 4th parameter of POSIX pthread_create( , , , arg).

// Returns:
//    Always NULL.
//
// """
STATIC void *EsfClockManagerMonitorThreadMain(void *arg);

/**
 * Declarations of private functions --- i.e., functions given storage-class
 * specifier static.
 */

// """Creates and initializes mutex for a thread which belongs to Clock Manager.

// If the given thread_id_with_mutex is NULL, returns kClockManagerParamError.
// This function creates an object of pthread_mutex_t, then initializes it.
// If the given *thread_id_with_mutex is not NULL, does not create an object of
// pthread_mutex_t and returns kClockManagerSuccess.

// Args:
//    thread_id_with_mutex (EsfClockManagerThreadId **const): a pointer to a
//      pointer to an object of EsfClockManagerThreadId.  A pointer to an object
//      created by this function is substituted for *thread_id_with_mutex.

// Returns:
//    The following values are returned:
//    kClockManagerSuccess: success.
//    kClockManagerParamError: invalid parameter.
//    kClockManagerInternalError: internal error broke out.

// """
STATIC EsfClockManagerReturnValue EsfClockManagerInitThreadId(
    EsfClockManagerThreadId **const thread_id_with_mutex);

// """Deinitializes and frees mutex for a thread which belongs to Clock Manager.

// If the given thread_id_with_mutex is NULL, returns kClockManagerParamError.
// This function destroys the object of pthread_mutex_t, then initializes it.
// If the given *thread_id_with_mutex is NULL, returns kClockManagerSuccess.

// Args:
//    thread_id_with_mutex (EsfClockManagerThreadId **const): a pointer to a
//      pointer to an object of EsfClockManagerThreadId.  A pointer to an object
//      created by EsfClockManagerInitThreadId.

// Returns:
//    The following values are returned:
//    kClockManagerSuccess: success.
//    kClockManagerParamError: invalid parameter.
//    kClockManagerInternalError: internal error broke out.

// """
STATIC EsfClockManagerReturnValue EsfClockManagerDeinitThreadId(
    EsfClockManagerThreadId **const thread_id_with_mutex);

// """Creates and initializes an object of structure EsfClockManagerCondMonitor.

// This function creates an object of EsfClockManagerCondMonitor, then
// initializes it.

// Args:
//    no arguments.

// Returns:
//    The following values are returned:
//    kClockManagerSuccess: success.
//    kClockManagerInternalError: internal error broke out.

// """
STATIC EsfClockManagerReturnValue EsfClockManagerInitCondForMonitor(void);

// """Deinitializes and frees an object of structure EsfClockManagerCondMonitor.

// This function deinitializes the object of EsfClockManagerCondMonitor, then
// frees it.

// Args:
//    no arguments.

// Returns:
//    The following values are returned:
//    kClockManagerSuccess: success.
//    kClockManagerInternalError: internal error broke out.

// """
STATIC EsfClockManagerReturnValue EsfClockManagerDeinitCondForMonitor(void);

// """Creates and initializes an object of structure EsfClockManagerCondDaemon.

// This function creates an object of EsfClockManagerCondDaemon, then
// initializes it.

// Args:
//    no arguments.

// Returns:
//    The following values are returned:
//    kClockManagerSuccess: success.
//    kClockManagerInternalError: internal error broke out.

// """
STATIC EsfClockManagerReturnValue EsfClockManagerInitCondForDaemon(void);

// """Deinitializes and frees an object of structure EsfClockManagerCondDaemon.

// This function deinitializes the object of EsfClockManagerCondDaemon, then
// frees it.

// Args:
//    no arguments.

// Returns:
//    The following values are returned:
//    kClockManagerSuccess: success.
//    kClockManagerInternalError: internal error broke out.

// """
STATIC EsfClockManagerReturnValue EsfClockManagerDeinitCondForDaemon(void);

// """Creates and initializes an object of structure
// EsfClockManagerCondNotifier.

// This function creates an object of EsfClockManagerCondNotifier, then
// initializes it.

// Args:
//    no arguments.

// Returns:
//    The following values are returned:
//    kClockManagerSuccess: success.
//    kClockManagerInternalError: internal error broke out.

// """
STATIC EsfClockManagerReturnValue EsfClockManagerInitCondForNotifier(void);

// """Deinitializes and frees an object of structure
// EsfClockManagerCondNotifier.

// This function deinitializes the object of EsfClockManagerCondNotifier, then
// frees it.

// Args:
//    no arguments.

// Returns:
//    The following values are returned:
//    kClockManagerSuccess: success.
//    kClockManagerInternalError: internal error broke out.

// """
STATIC EsfClockManagerReturnValue EsfClockManagerDeinitCondForNotifier(void);

// """Judges whether NTP time synchronization is complete or not.

//

// Args:
//    ptr_ntpc_status (const struct ntpc_status_s *const): a pointer to an
//      object of struct ntpc_status_s.  The value of this object gets from NTP
//      Client Daemon.

// Returns:
//    The following values are returned:
//    kHasNotCompletedYet: has not completed yet.
//    kNtpTimeSyncSuccess: result in success.
//    kNtpTimeSyncFailure: result in failure.
//
// """
STATIC EsfClockManagerNtpTimeSyncStatus EsfClockManagerJudgeNtpTimeSyncComplete(
    const struct ntpc_status_s *const ptr_ntpc_status);

// """Posts an event regarding with NTP time synchronization complete.

//

// Args:
//    ptr_ntpc_status (const struct ntpc_status_s *const): a pointer to an
//      object of struct ntpc_status_s.  The value of this object gets from NTP
//      Client Daemon.

// Returns:
//    The following values are returned:
//    kClockManagerSuccess: success.
//    kClockManagerParamError: invalid parameter.
//
// """
STATIC EsfClockManagerReturnValue EsfClockManagerPostNtpTimeSyncCompleteNotif(
    const struct ntpc_status_s *const ptr_ntpc_status);

// """Posts an event regarding with notifications.

//

// Args:
//    notification_type (const EsfClockManagerNotificationType): a type of
//      notifications.
//    additional (const int): additional information.

// Returns:
//    The following values are returned:
//    kClockManagerSuccess: success.
//    kClockManagerInternalError: internal error broke out.
//
// """
STATIC EsfClockManagerReturnValue EsfClockManagerPostNotification(
    const EsfClockManagerNotificationType notification_type,
    const int additional);

// """Processes an event regarding with notifications.

// The given notification is processed.

// Args:
//    notification (const EsfClockManagerNotification *const): a pointer to an
//      object of notification.

// Returns:
//    True if type is kCondTypeIsFinReq; false otherwise.
//
// """
STATIC bool EsfClockManagerProcessNotificationEvent(
    const EsfClockManagerNotification *const notification);

// """Entry-point function for a pthread regarding with notifying.

// This function is given to the 3rd parameter of POSIX
// pthread_create( , , tart_routine, ).  If the pthread starts, then this
// function is called.

// Args:
//    arg (void *): The 4th parameter of POSIX pthread_create( , , , arg).

// Returns:
//    Always NULL.
//
// """
STATIC void *EsfClockManagerNotifierMain(void *arg);

/**
 * Definitions of global variables
 */

#ifdef __NuttX__
STATIC const char *const kThreadName = "ClockManagerNTPclientMonitor";
#else
STATIC const char *const kThreadName = "ClockMgrNcMonit";
#endif  // #ifdef __NuttX__ -- #else ends.

STATIC EsfClockManagerMillisecondT g_sleep_time;

STATIC EsfClockManagerThreadId *g_monitor_thread_id_with_mutex = NULL;

STATIC EsfClockManagerCondMonitor *g_cond_for_monitor_with_mutex = NULL;

STATIC EsfClockManagerCondDaemon *g_cond_for_daemon_with_mutex = NULL;

STATIC int g_task_id_of_ntp_client_daemon = INVALID_PROCESS_ID;
STATIC struct ntp_sync_params_s g_params_to_ntpc;
STATIC char *g_ntp_server_list = NULL;

#ifdef __NuttX__
STATIC const char *const kThread2Name = "ClockManagerNotifier";
#else
STATIC const char *const kThread2Name = "ClockMgrNotif";
#endif  // #ifdef __NuttX__ -- #else ends.

STATIC EsfClockManagerThreadId *g_notifier_thread_id_with_mutex = NULL;

STATIC EsfClockManagerCondNotifier *g_cond_for_notifier_with_mutex = NULL;

STATIC void (*g_ntp_sync_complete_cb_func)(bool) = NULL;

/**
 * Definitions of package private functions
 */

EsfClockManagerReturnValue EsfClockManagerCreateMonitorThread(
    const EsfClockManagerMillisecondT surveillance_period) {
  if (pthread_mutex_lock(
          g_monitor_thread_id_with_mutex->m_mutex_base.m_mutex)) {
    WRITE_DLOG_CRITICAL(MODULE_ID_SYSTEM,
                        "%s-%d:%s --- pthread_mutex_lock failed.\n",
                        "clock_manager_internal.c", __LINE__, __func__);
    return kClockManagerStateTransitionError;
  }

  if (g_monitor_thread_id_with_mutex->m_thread_id !=
      ESF_CLOCK_MANAGER_INVALID_THREAD_ID) {
    pthread_mutex_unlock(g_monitor_thread_id_with_mutex->m_mutex_base.m_mutex);
#ifdef __NuttX__
    WRITE_DLOG_WARN(MODULE_ID_SYSTEM,
                    "%s-%d:%s --- The thread already exists:%d\n",
                    "clock_manager_internal.c", __LINE__, __func__,
                    g_monitor_thread_id_with_mutex->m_thread_id);
#else
    WRITE_DLOG_WARN(MODULE_ID_SYSTEM,
                    "%s-%d:%s --- The thread already exists:%lu\n",
                    "clock_manager_internal.c", __LINE__, __func__,
                    g_monitor_thread_id_with_mutex->m_thread_id);
#endif  // __NuttX__
    return kClockManagerStateTransitionError;
  }

  const EsfClockManagerMillisecondT sleep_time = g_sleep_time;
  g_sleep_time = surveillance_period;
  g_cond_for_monitor_with_mutex->m_req_fin = false;

  pthread_attr_t thread_attr;
  (void)pthread_attr_init(&thread_attr);
  (void)pthread_attr_setstacksize(
      &thread_attr,
      (size_t)(CONFIG_EXTERNAL_CLOCK_MANAGER_NTP_CLIENT_MONITOR_STACKSIZE));
  errno = 0;
  const int rv = pthread_create(&(g_monitor_thread_id_with_mutex->m_thread_id),
                                &thread_attr, EsfClockManagerMonitorThreadMain,
                                NULL);

  if (rv) {
    g_sleep_time = sleep_time;
    pthread_mutex_unlock(g_monitor_thread_id_with_mutex->m_mutex_base.m_mutex);
    WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (uint16_t)0x8103);
    WRITE_DLOG_CRITICAL(
        MODULE_ID_SYSTEM, "%s-%d:%s --- pthread_create failed:%d,errno:%d\n",
        "clock_manager_internal.c", __LINE__, __func__, rv, errno);
    return kClockManagerStateTransitionError;
  }
  pthread_setname_np(g_monitor_thread_id_with_mutex->m_thread_id, kThreadName);

  pthread_mutex_unlock(g_monitor_thread_id_with_mutex->m_mutex_base.m_mutex);

  return kClockManagerSuccess;
}

EsfClockManagerReturnValue EsfClockManagerStartNtpClientDaemon(
    const EsfClockManagerParams *param) {
  snprintf(g_ntp_server_list, ESF_CLOCK_MANAGER_NTPADDR_MAX_SIZE, "%s",
           &(param->connect.hostname[0]));
  g_params_to_ntpc.sync_interval_base = param->common.sync_interval;
  g_params_to_ntpc.skip_and_limit_params.is_skip_and_limit_on =
      param->skip_and_limit.type != kClockManagerParamTypeOff;
  g_params_to_ntpc.skip_and_limit_params.limit_packet_time =
      (int32_t)param->skip_and_limit.limit_packet_time;
  g_params_to_ntpc.skip_and_limit_params.limit_rtc_correction_value =
      (int32_t)param->skip_and_limit.limit_rtc_correction_value;
  g_params_to_ntpc.skip_and_limit_params.sanity_limit =
      (int32_t)param->skip_and_limit.sanity_limit;
  g_params_to_ntpc.slew_params.is_slew_param_on = param->slew_setting.type !=
                                                  kClockManagerParamTypeOff;
  g_params_to_ntpc.slew_params.stable_rtc_correction_value_base =
      (int32_t)param->slew_setting.stable_rtc_correction_value;
  g_params_to_ntpc.slew_params.stable_sync_number =
      param->slew_setting.stable_sync_number;

  bool is_off = false;
  if (param->skip_and_limit.type == kClockManagerParamTypeOff &&
      param->slew_setting.type == kClockManagerParamTypeOff) {
    is_off = true;
  }
  if (param->skip_and_limit.type == kClockManagerParamTypeDefault) {
    g_params_to_ntpc.skip_and_limit_params.limit_packet_time =
        CLOCK_MANAGER_LIMIT_PACKET_TIME_DEF;
    g_params_to_ntpc.skip_and_limit_params.limit_rtc_correction_value =
        CLOCK_MANAGER_RTC_CORRECT_LIMIT_DEF;
    g_params_to_ntpc.skip_and_limit_params.sanity_limit =
        CLOCK_MANAGER_SANITY_LIMIT_DEF;
  }
  if (param->slew_setting.type == kClockManagerParamTypeDefault) {
    g_params_to_ntpc.slew_params.stable_rtc_correction_value_base =
        CLOCK_MANAGER_STABLE_RTC_DEF;
    g_params_to_ntpc.slew_params.stable_sync_number =
        CLOCK_MANAGER_STABLE_SYNC_CONT_DEF;
  }
  WRITE_DLOG_INFO(
      MODULE_ID_SYSTEM,
      "%s-%d:%s --- ntp_server_list:<%s>"
      "skip_and_limit.type:%d,limit_packet_time:%d,limit_rtc_correction_value:%"
      "d,sanity_limit:%d,slew_setting.type:%d,stable_rtc_correction_value_base:"
      "%d,stable_sync_number:%d\n",
      "clock_manager_internal.c", __LINE__, __func__, g_ntp_server_list,
      param->skip_and_limit.type,
      g_params_to_ntpc.skip_and_limit_params.limit_packet_time,
      g_params_to_ntpc.skip_and_limit_params.limit_rtc_correction_value,
      g_params_to_ntpc.skip_and_limit_params.sanity_limit,
      param->slew_setting.type,
      g_params_to_ntpc.slew_params.stable_rtc_correction_value_base,
      g_params_to_ntpc.slew_params.stable_sync_number);

  g_task_id_of_ntp_client_daemon = INVALID_PROCESS_ID;
  WRITE_DLOG_DEBUG(MODULE_ID_SYSTEM, "%s-%d:%s --- is_off%d\n",
                   "clock_manager_internal.c", __LINE__, __func__, is_off);
  if (is_off) {
    g_task_id_of_ntp_client_daemon = ntpc_start_with_list(g_ntp_server_list);
  } else {
    g_task_id_of_ntp_client_daemon = ntpc_start_with_params(g_ntp_server_list,
                                                            &g_params_to_ntpc);
  }
  if (g_task_id_of_ntp_client_daemon < 0) {
    WRITE_DLOG_CRITICAL(MODULE_ID_SYSTEM,
                        "%s-%d:%s --- failure:NTP_daemon ret:%d\n",
                        "clock_manager_internal.c", __LINE__, __func__,
                        g_task_id_of_ntp_client_daemon);
    g_task_id_of_ntp_client_daemon = INVALID_PROCESS_ID;
    WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (uint16_t)0x8105);
    return kClockManagerStateTransitionError;
  }
  return kClockManagerSuccess;
}

EsfClockManagerReturnValue EsfClockManagerDestroyMonitorThread(void) {
  const int rv =
      pthread_mutex_lock(g_cond_for_monitor_with_mutex->m_cond_base.m_mutex);
  if (rv) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- failure:pthread_mutex_lock:%d\n",
                     "clock_manager_internal.c", __LINE__, __func__, rv);
  } else {
    WRITE_DLOG_INFO(MODULE_ID_SYSTEM, "%s-%d:%s --- pthread_mutex_lock:%d\n",
                    "clock_manager_internal.c", __LINE__, __func__, rv);
  }
  g_cond_for_monitor_with_mutex->m_req_fin = true;
  pthread_cond_signal(g_cond_for_monitor_with_mutex->m_cond_base.m_cond);
  if (!rv) {
    pthread_mutex_unlock(g_cond_for_monitor_with_mutex->m_cond_base.m_mutex);
  }

  return kClockManagerSuccess;
}

EsfClockManagerReturnValue EsfClockManagerWaitForTerminatingDaemon(void) {
  EsfClockManagerReturnValue rv = kClockManagerSuccess;

  struct timespec abs_time = {0};
  EsfClockManagerCalculateAbstimeInMonotonic(
      &abs_time, (EsfClockManagerMillisecondT)ESF_CLOCK_MANAGER_STOP_TIMEOUT);

  const int rv_daemon =
      pthread_mutex_lock(g_cond_for_daemon_with_mutex->m_cond_base.m_mutex);
  if (rv_daemon) {
    WRITE_DLOG_WARN(MODULE_ID_SYSTEM,
                    "%s-%d:%s --- pthread_mutex_lock failed:%d\n",
                    "clock_manager_internal.c", __LINE__, __func__, rv_daemon);
  }
  WRITE_DLOG_INFO(
      MODULE_ID_SYSTEM,
      "%s-%d:%s --- g_task_id_of_ntp_client_daemon:%d, m_is_daemon_fin:%d\n",
      "clock_manager_internal.c", __LINE__, __func__,
      g_task_id_of_ntp_client_daemon,
      g_cond_for_daemon_with_mutex->m_is_daemon_fin);
  if (g_task_id_of_ntp_client_daemon == INVALID_PROCESS_ID &&
      g_cond_for_daemon_with_mutex->m_is_daemon_fin) {
    if (!rv_daemon) {
      pthread_mutex_unlock(g_cond_for_daemon_with_mutex->m_cond_base.m_mutex);
    }
    return kClockManagerDaemonHasAleadyFinished;
  }
  while (!g_cond_for_daemon_with_mutex->m_is_daemon_fin) {
    errno = 0;
    const int r = pthread_cond_timedwait(
        g_cond_for_daemon_with_mutex->m_cond_base.m_cond,
        g_cond_for_daemon_with_mutex->m_cond_base.m_mutex, &abs_time);
    if (r) {
#ifdef __NuttX__
      if ((r == EAGAIN || r == ETIMEDOUT) ||
          (errno == EAGAIN || errno == ETIMEDOUT)) {
        break;
      }
#else
      if ((r == EAGAIN || r == ETIMEDOUT) ||
          (errno == EAGAIN || errno == ETIMEDOUT)) {
        break;
      }
#endif  // #ifdef __NuttX__ -- #else ends.
    }
  }
  if (!g_cond_for_daemon_with_mutex->m_is_daemon_fin) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:%s --- m_is_daemon_fin:%d\n",
                     "clock_manager_internal.c", __LINE__, __func__,
                     g_cond_for_daemon_with_mutex->m_is_daemon_fin);
    rv = kClockManagerStateTransitionError;
    WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (uint16_t)0x810B);
  }
  if (!rv_daemon) {
    pthread_mutex_unlock(g_cond_for_daemon_with_mutex->m_cond_base.m_mutex);
  }

  return rv;
}

/**
 * Definitions of package private functions
 */

EsfClockManagerReturnValue EsfClockManagerInitInternal(void) {
  g_ntp_server_list = (char *)malloc(ESF_CLOCK_MANAGER_NTPADDR_MAX_SIZE);
  if (g_ntp_server_list == NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:%s --- malloc failed.\n",
                     "clock_manager_internal.c", __LINE__, __func__);
    return kClockManagerInternalError;
  }
  g_ntp_server_list[0] = '\0';
  const EsfClockManagerReturnValue rv =
      EsfClockManagerInitThreadId(&g_monitor_thread_id_with_mutex);
  if (rv != kClockManagerSuccess) {
    free(g_ntp_server_list);
    g_ntp_server_list = NULL;
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:%s --- Initialization on monitor thread id failed.\n",
        "clock_manager_internal.c", __LINE__, __func__);
    return kClockManagerInternalError;
  }
  const EsfClockManagerReturnValue rv2 = EsfClockManagerInitCondForMonitor();
  if (rv2 != kClockManagerSuccess) {
    EsfClockManagerDeinitThreadId(&g_monitor_thread_id_with_mutex);
    free(g_ntp_server_list);
    g_ntp_server_list = NULL;
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:%s --- Initialization on cond for monitor failed.\n",
        "clock_manager_internal.c", __LINE__, __func__);
    return kClockManagerInternalError;
  }
  const EsfClockManagerReturnValue rv3 = EsfClockManagerInitCondForDaemon();
  if (rv3 != kClockManagerSuccess) {
    EsfClockManagerDeinitCondForMonitor();
    EsfClockManagerDeinitThreadId(&g_monitor_thread_id_with_mutex);
    free(g_ntp_server_list);
    g_ntp_server_list = NULL;
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- Initialization on cond for daemon failed.\n",
                     "clock_manager_internal.c", __LINE__, __func__);
    return kClockManagerInternalError;
  }
  const EsfClockManagerReturnValue rv4 =
      EsfClockManagerInitThreadId(&g_notifier_thread_id_with_mutex);
  if (rv4 != kClockManagerSuccess) {
    free(g_ntp_server_list);
    g_ntp_server_list = NULL;
    EsfClockManagerDeinitThreadId(&g_monitor_thread_id_with_mutex);
    EsfClockManagerDeinitCondForMonitor();
    EsfClockManagerDeinitCondForDaemon();
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:%s --- Initialization on notifier thread id failed.\n",
        "clock_manager_internal.c", __LINE__, __func__);
    return kClockManagerInternalError;
  }
  const EsfClockManagerReturnValue rv5 = EsfClockManagerInitCondForNotifier();
  if (rv5 != kClockManagerSuccess) {
    free(g_ntp_server_list);
    g_ntp_server_list = NULL;
    EsfClockManagerDeinitThreadId(&g_monitor_thread_id_with_mutex);
    EsfClockManagerDeinitCondForMonitor();
    EsfClockManagerDeinitCondForDaemon();
    EsfClockManagerDeinitThreadId(&g_notifier_thread_id_with_mutex);
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:%s --- Initialization on cond for notifier failed.\n",
        "clock_manager_internal.c", __LINE__, __func__);
    return kClockManagerInternalError;
  }

  return kClockManagerSuccess;
}

EsfClockManagerReturnValue EsfClockManagerDeinitInternal(void) {
  free(g_ntp_server_list);
  g_ntp_server_list = NULL;
  EsfClockManagerDeinitThreadId(&g_monitor_thread_id_with_mutex);
  EsfClockManagerDeinitCondForMonitor();
  EsfClockManagerDeinitCondForDaemon();
  EsfClockManagerDeinitThreadId(&g_notifier_thread_id_with_mutex);
  EsfClockManagerDeinitCondForNotifier();

  return kClockManagerSuccess;
}

EsfClockManagerReturnValue EsfClockManagerRegisterNtpSyncCompleteCb(
    void (*on_ntp_sync_complete)(bool)) {
  g_ntp_sync_complete_cb_func = on_ntp_sync_complete;
  return kClockManagerSuccess;
}

EsfClockManagerReturnValue EsfClockManagerUnregisterNtpSyncCompleteCb(void) {
  g_ntp_sync_complete_cb_func = NULL;
  return kClockManagerSuccess;
}

EsfClockManagerReturnValue EsfClockManagerCreateNotifierThread(void) {
  if (pthread_mutex_lock(
          g_notifier_thread_id_with_mutex->m_mutex_base.m_mutex)) {
    WRITE_DLOG_CRITICAL(MODULE_ID_SYSTEM,
                        "%s-%d:%s --- pthread_mutex_lock failed.\n",
                        "clock_manager_internal.c", __LINE__, __func__);
    return kClockManagerStateTransitionError;
  }

  if (g_notifier_thread_id_with_mutex->m_thread_id !=
      ESF_CLOCK_MANAGER_INVALID_THREAD_ID) {
    pthread_mutex_unlock(g_notifier_thread_id_with_mutex->m_mutex_base.m_mutex);
#ifdef __NuttX__
    WRITE_DLOG_WARN(MODULE_ID_SYSTEM,
                    "%s-%d:%s --- The thread already exists:%d\n",
                    "clock_manager_internal.c", __LINE__, __func__,
                    g_notifier_thread_id_with_mutex->m_thread_id);
#else
    WRITE_DLOG_WARN(MODULE_ID_SYSTEM,
                    "%s-%d:%s --- The thread already exists:%lu\n",
                    "clock_manager_internal.c", __LINE__, __func__,
                    g_notifier_thread_id_with_mutex->m_thread_id);
#endif  // __NuttX__
    return kClockManagerStateTransitionError;
  }

  pthread_attr_t thread_attr;
  (void)pthread_attr_init(&thread_attr);
  (void)pthread_attr_setstacksize(
      &thread_attr, (size_t)CONFIG_EXTERNAL_CLOCK_MANAGER_NOTIFIER_STACKSIZE);
  const int rv = pthread_create(&(g_notifier_thread_id_with_mutex->m_thread_id),
                                &thread_attr, EsfClockManagerNotifierMain,
                                NULL);

  if (rv) {
    pthread_mutex_unlock(g_notifier_thread_id_with_mutex->m_mutex_base.m_mutex);
    WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (uint16_t)0x8104);
    WRITE_DLOG_CRITICAL(
        MODULE_ID_SYSTEM, "%s-%d:%s --- pthread_create failed:%d,errno:%d\n",
        "clock_manager_internal.c", __LINE__, __func__, rv, errno);
    return kClockManagerStateTransitionError;
  }
  pthread_setname_np(g_notifier_thread_id_with_mutex->m_thread_id,
                     kThread2Name);

  pthread_mutex_unlock(g_notifier_thread_id_with_mutex->m_mutex_base.m_mutex);

  return kClockManagerSuccess;
}

EsfClockManagerReturnValue EsfClockManagerDestroyNotifier(void) {
  return EsfClockManagerPostNotification(kCondTypeIsFinReq, 0);
}

int EsfClockManagerCancelMonitorThread(void) {
  int rv = 0;
  if (g_monitor_thread_id_with_mutex != NULL) {
    rv = -1;
    if (!pthread_mutex_lock(
            g_monitor_thread_id_with_mutex->m_mutex_base.m_mutex)) {
      if (g_monitor_thread_id_with_mutex->m_thread_id !=
          ESF_CLOCK_MANAGER_INVALID_THREAD_ID) {
        rv = pthread_cancel(g_monitor_thread_id_with_mutex->m_thread_id);
        WRITE_DLOG_INFO(MODULE_ID_SYSTEM, "%s-%d:%s --- pthread_cancel:%d\n",
                        "clock_manager_internal.c", __LINE__, __func__, rv);
        if (!rv) {
          rv = pthread_join(g_monitor_thread_id_with_mutex->m_thread_id, NULL);
          g_monitor_thread_id_with_mutex->m_thread_id =
              ESF_CLOCK_MANAGER_INVALID_THREAD_ID;
        }
      }
    }
    pthread_mutex_unlock(g_monitor_thread_id_with_mutex->m_mutex_base.m_mutex);
  }
  return rv;
}

int EsfClockManagerCancelNotifierThread(void) {
  int rv = 0;
  if (g_notifier_thread_id_with_mutex != NULL) {
    rv = -1;
    if (!pthread_mutex_lock(
            g_notifier_thread_id_with_mutex->m_mutex_base.m_mutex)) {
      if (g_notifier_thread_id_with_mutex->m_thread_id !=
          ESF_CLOCK_MANAGER_INVALID_THREAD_ID) {
        rv = pthread_cancel(g_notifier_thread_id_with_mutex->m_thread_id);
        WRITE_DLOG_INFO(MODULE_ID_SYSTEM, "%s-%d:%s --- pthread_cancel:%d\n",
                        "clock_manager_internal.c", __LINE__, __func__, rv);
        if (!rv) {
          rv = pthread_join(g_notifier_thread_id_with_mutex->m_thread_id, NULL);
          g_notifier_thread_id_with_mutex->m_thread_id =
              ESF_CLOCK_MANAGER_INVALID_THREAD_ID;
        }
      }
      pthread_mutex_unlock(
          g_notifier_thread_id_with_mutex->m_mutex_base.m_mutex);
    }
  }
  return rv;
}

/**
 * Definitions of private functions --- i.e., functions given storage-class
 * specifier static.
 */

STATIC EsfClockManagerReturnValue EsfClockManagerStopNtpClientDaemon(void) {
  g_cond_for_daemon_with_mutex->m_is_daemon_fin = false;
  const int rv = ntpc_stop();
  WRITE_DLOG_INFO(MODULE_ID_SYSTEM, "%s-%d:%s --- ntpc_stop:%d\n",
                  "clock_manager_internal.c", __LINE__, __func__, rv);
  if (!rv) {
    g_task_id_of_ntp_client_daemon = INVALID_PROCESS_ID;
    return kClockManagerSuccess;
  }
  // This statement is not executed.
  return kClockManagerStateTransitionError;
}

STATIC void *EsfClockManagerMonitorThreadMain(void *arg) {
  NOT_USED(arg);
  struct timespec abs_time = {0};
  struct ntpc_status_s *status = NULL;
  WRITE_DLOG_INFO(MODULE_ID_SYSTEM, "%s-%d:%s --- has just started.\n",
                  "clock_manager_internal.c", __LINE__, __func__);

  status = (struct ntpc_status_s *)malloc(sizeof(struct ntpc_status_s));
  if (status == NULL) {
    WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (uint16_t)0x8110);
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- malloc for struct ntpc_status_s failed.\n",
                     "clock_manager_internal.c", __LINE__, __func__);
  }

  /*unsigned int my_counter = 0;
  const size_t debug_buf_size = (size_t)512;
  char *debug = (char *)malloc(debug_buf_size);*/
  bool has_complete_notif_been_posted = false;
  // printf("[IN]   EsfClockManagerMonitorThreadMain\n");
  while (status != NULL) {
    memset(status, 0, sizeof(*status));
    ntpc_status(status);
    /*++my_counter;
    snprintf(
        debug, debug_buf_size,
        "[INFO] EsfClockManagerMonitorThreadMain <%u> --- nsamples=%u, pid=%d, "
        "ntpc_daemon_state=%d, sanity_over_cnt=%u, num_of_errors=%d\n",
        my_counter, status->nsamples, status->pid, status->ntpc_daemon_state,
        status->sanity_over_cnt, status->num_of_errors);
    printf("%s", debug);*/
    // printf("[INFO] EsfClockManagerMonitorThreadMain --- <%u>\n", my_counter);

    if (!has_complete_notif_been_posted) {
      has_complete_notif_been_posted =
          EsfClockManagerPostNtpTimeSyncCompleteNotif(status) ==
          kClockManagerSuccess;
    }

    if (g_task_id_of_ntp_client_daemon != INVALID_PROCESS_ID) {
      const int rv = pthread_mutex_lock(
          g_cond_for_monitor_with_mutex->m_cond_base.m_mutex);
      if (status->pid == INVALID_PROCESS_ID &&
          (!g_cond_for_monitor_with_mutex->m_req_fin)) {
        WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (uint16_t)0x8106);
        WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                         "%s-%d:%s --- NTP Client Daemon has already ended.\n",
                         "clock_manager_internal.c", __LINE__, __func__);
        const bool is_either_on =
            g_params_to_ntpc.skip_and_limit_params.is_skip_and_limit_on ||
            g_params_to_ntpc.slew_params.is_slew_param_on;
        pid_t daemon_pid = INVALID_PROCESS_ID;
        if (is_either_on) {
          daemon_pid = ntpc_start_with_params(g_ntp_server_list,
                                              &g_params_to_ntpc);
        } else {
          daemon_pid = ntpc_start_with_list(g_ntp_server_list);
        }
        if (daemon_pid < (pid_t)0) {
          WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (uint16_t)0x8107);
          WRITE_DLOG_ERROR(
              MODULE_ID_SYSTEM,
              "%s-%d:%s --- Restarting a daemon whose name is "
              "\"NTP Client Daemon\" failed. daemon_pid:%d, is_either_on:%d\n",
              "clock_manager_internal.c", __LINE__, __func__, daemon_pid,
              is_either_on);
        }
      }
      if (!rv) {
        pthread_mutex_unlock(
            g_cond_for_monitor_with_mutex->m_cond_base.m_mutex);
      }
    }

    EsfClockManagerCalculateAbstimeInMonotonic(&abs_time, g_sleep_time);

    const int rv_monitor =
        pthread_mutex_lock(g_cond_for_monitor_with_mutex->m_cond_base.m_mutex);
    if (rv_monitor) {
      WRITE_DLOG_ERROR(
          MODULE_ID_SYSTEM, "%s-%d:%s --- pthread_mutex_lock failed:%d\n",
          "clock_manager_internal.c", __LINE__, __func__, rv_monitor);
    }
    if (g_cond_for_monitor_with_mutex->m_req_fin) {
      g_cond_for_monitor_with_mutex->m_req_fin = false;
      if (!rv_monitor) {
        pthread_mutex_unlock(
            g_cond_for_monitor_with_mutex->m_cond_base.m_mutex);
      }
      break;
    }
    if (!rv_monitor) {
      pthread_cond_timedwait(g_cond_for_monitor_with_mutex->m_cond_base.m_cond,
                             g_cond_for_monitor_with_mutex->m_cond_base.m_mutex,
                             &abs_time);
    } else {
      sleep((unsigned int)(g_sleep_time / 1000U));
    }
    if (g_cond_for_monitor_with_mutex->m_req_fin) {
      g_cond_for_monitor_with_mutex->m_req_fin = false;
      if (!rv_monitor) {
        pthread_mutex_unlock(
            g_cond_for_monitor_with_mutex->m_cond_base.m_mutex);
      }
      break;
    }
    if (!rv_monitor) {
      pthread_mutex_unlock(g_cond_for_monitor_with_mutex->m_cond_base.m_mutex);
    }
  }
  /*free(debug);
  debug = NULL;*/

  const int sleep_time_in_ms = 100;
  const int max_number_for_fail_safe = ESF_CLOCK_MANAGER_STOP_TIMEOUT /
                                       sleep_time_in_ms;
  int fail_safe_counter = 0;
  EsfClockManagerStopNtpClientDaemon();
  if (status != NULL) {
    do {
      usleep((useconds_t)(1000 * sleep_time_in_ms));
      memset(status, 0, sizeof(*status));
      ntpc_status(status);
      if (!has_complete_notif_been_posted) {
        has_complete_notif_been_posted =
            EsfClockManagerPostNtpTimeSyncCompleteNotif(status) ==
            kClockManagerSuccess;
      }
      fail_safe_counter++;
      // printf("[INFO] EsfClockManagerMonitorThreadMain --- fail_safe_counter =
      // %d\n", fail_safe_counter);
    } while (status->pid != (pid_t)-1 &&
             fail_safe_counter < max_number_for_fail_safe);
  }

  WRITE_DLOG_INFO(MODULE_ID_SYSTEM, "%s-%d:%s --- fail_safe_counter=%d\n",
                  "clock_manager_internal.c", __LINE__, __func__,
                  fail_safe_counter);
  if (status != NULL && status->pid > (pid_t)-1) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- status->pid=%d, fail_safe_counter=%d\n",
                     "clock_manager_internal.c", __LINE__, __func__,
                     status->pid, fail_safe_counter);
  }
  const int rv_daemon =
      pthread_mutex_lock(g_cond_for_daemon_with_mutex->m_cond_base.m_mutex);
  if (rv_daemon) {
    WRITE_DLOG_WARN(MODULE_ID_SYSTEM,
                    "%s-%d:%s --- pthread_mutex_lock failed:%d\n",
                    "clock_manager_internal.c", __LINE__, __func__, rv_daemon);
  }
  // printf("[INFO] EsfClockManagerMonitorThreadMain --- status.pid = %d\n",
  // status.pid);
  if (status != NULL && status->pid == (pid_t)-1) {
    g_cond_for_daemon_with_mutex->m_is_daemon_fin = true;
  }
  pthread_cond_signal(g_cond_for_daemon_with_mutex->m_cond_base.m_cond);
  if (!rv_daemon) {
    pthread_mutex_unlock(g_cond_for_daemon_with_mutex->m_cond_base.m_mutex);
  }

  const int rv_monitor2 =
      pthread_mutex_lock(g_monitor_thread_id_with_mutex->m_mutex_base.m_mutex);
  if (rv_monitor2) {
    WRITE_DLOG_WARN(
        MODULE_ID_SYSTEM, "%s-%d:%s --- pthread_mutex_lock failed:%d\n",
        "clock_manager_internal.c", __LINE__, __func__, rv_monitor2);
  }
  g_monitor_thread_id_with_mutex->m_thread_id =
      ESF_CLOCK_MANAGER_INVALID_THREAD_ID;
  if (!rv_monitor2) {
    pthread_mutex_unlock(g_monitor_thread_id_with_mutex->m_mutex_base.m_mutex);
  }

  free(status);
  status = NULL;

  // printf("[OUT]  EsfClockManagerMonitorThreadMain\n");
  WRITE_DLOG_INFO(MODULE_ID_SYSTEM, "%s-%d:%s --- is about to end.\n",
                  "clock_manager_internal.c", __LINE__, __func__);
  return NULL;
}

/**
 * Definitions of private functions --- i.e., functions given storage-class
 * specifier static.
 */

STATIC EsfClockManagerReturnValue EsfClockManagerInitThreadId(
    EsfClockManagerThreadId **const thread_id_with_mutex) {
  if (thread_id_with_mutex == NULL) {
    return kClockManagerParamError;
  }

  if (*thread_id_with_mutex != NULL) {
    return kClockManagerSuccess;
  }

  *thread_id_with_mutex =
      (EsfClockManagerThreadId *)malloc(sizeof(EsfClockManagerThreadId));
  if (*thread_id_with_mutex == NULL) {
    WRITE_DLOG_CRITICAL(MODULE_ID_SYSTEM, "%s-%d:%s --- malloc failed.\n",
                        "clock_manager_internal.c", __LINE__, __func__);
    return kClockManagerInternalError;
  }

  (*thread_id_with_mutex)->m_mutex_base.m_mutex = NULL;

  const EsfClockManagerMutexBaseReturnValue rv =
      EsfClockManagerInitMutexBase(&((*thread_id_with_mutex)->m_mutex_base));
  if (rv != kMutexBaseRvIsSuccess) {
    free(*thread_id_with_mutex);
    *thread_id_with_mutex = NULL;
    return kClockManagerInternalError;
  }

  (*thread_id_with_mutex)->m_thread_id = ESF_CLOCK_MANAGER_INVALID_THREAD_ID;

  return kClockManagerSuccess;
}

STATIC EsfClockManagerReturnValue EsfClockManagerDeinitThreadId(
    EsfClockManagerThreadId **const thread_id_with_mutex) {
  if (thread_id_with_mutex == NULL) {
    return kClockManagerParamError;
  }

  if (*thread_id_with_mutex != NULL) {
    const EsfClockManagerMutexBaseReturnValue rv =
        EsfClockManagerDeinitMutexBase(
            &((*thread_id_with_mutex)->m_mutex_base));
    NOT_USED(rv);
  }

  free(*thread_id_with_mutex);
  *thread_id_with_mutex = NULL;

  return kClockManagerSuccess;
}

STATIC EsfClockManagerReturnValue EsfClockManagerInitCondForMonitor(void) {
  if (g_cond_for_monitor_with_mutex != NULL) {
    return kClockManagerSuccess;
  }

  g_cond_for_monitor_with_mutex =
      (EsfClockManagerCondMonitor *)malloc(sizeof(EsfClockManagerCondMonitor));
  if (g_cond_for_monitor_with_mutex == NULL) {
    WRITE_DLOG_CRITICAL(MODULE_ID_SYSTEM, "%s-%d:%s --- malloc failed.\n",
                        "clock_manager_internal.c", __LINE__, __func__);
    return kClockManagerInternalError;
  }

  g_cond_for_monitor_with_mutex->m_cond_base.m_mutex = NULL;
  g_cond_for_monitor_with_mutex->m_cond_base.m_cond = NULL;

  const EsfClockManagerCondBaseReturnValue rv = EsfClockManagerInitCondBase(
      &(g_cond_for_monitor_with_mutex->m_cond_base));
  if (rv != kCondBaseRvIsSuccess) {
    free(g_cond_for_monitor_with_mutex);
    g_cond_for_monitor_with_mutex = NULL;
    return kClockManagerInternalError;
  }

  g_cond_for_monitor_with_mutex->m_req_fin = true;

  return kClockManagerSuccess;
}

STATIC EsfClockManagerReturnValue EsfClockManagerDeinitCondForMonitor(void) {
  if (g_cond_for_monitor_with_mutex != NULL) {
    EsfClockManagerDeinitCondBase(
        &(g_cond_for_monitor_with_mutex->m_cond_base));
  }

  free(g_cond_for_monitor_with_mutex);
  g_cond_for_monitor_with_mutex = NULL;

  return kClockManagerSuccess;
}

STATIC EsfClockManagerReturnValue EsfClockManagerInitCondForDaemon(void) {
  if (g_cond_for_daemon_with_mutex != NULL) {
    return kClockManagerSuccess;
  }

  g_cond_for_daemon_with_mutex =
      (EsfClockManagerCondDaemon *)malloc(sizeof(EsfClockManagerCondDaemon));
  if (g_cond_for_daemon_with_mutex == NULL) {
    WRITE_DLOG_CRITICAL(MODULE_ID_SYSTEM, "%s-%d:%s --- malloc failed.\n",
                        "clock_manager_internal.c", __LINE__, __func__);
    return kClockManagerInternalError;
  }

  g_cond_for_daemon_with_mutex->m_cond_base.m_mutex = NULL;
  g_cond_for_daemon_with_mutex->m_cond_base.m_cond = NULL;

  const EsfClockManagerCondBaseReturnValue rv =
      EsfClockManagerInitCondBase(&(g_cond_for_daemon_with_mutex->m_cond_base));
  if (rv != kCondBaseRvIsSuccess) {
    free(g_cond_for_daemon_with_mutex);
    g_cond_for_daemon_with_mutex = NULL;
    return kClockManagerInternalError;
  }

  g_cond_for_daemon_with_mutex->m_is_daemon_fin = false;

  return kClockManagerSuccess;
}

STATIC EsfClockManagerReturnValue EsfClockManagerDeinitCondForDaemon(void) {
  if (g_cond_for_daemon_with_mutex != NULL) {
    EsfClockManagerDeinitCondBase(&(g_cond_for_daemon_with_mutex->m_cond_base));
  }

  free(g_cond_for_daemon_with_mutex);
  g_cond_for_daemon_with_mutex = NULL;

  return kClockManagerSuccess;
}

STATIC EsfClockManagerReturnValue EsfClockManagerInitCondForNotifier(void) {
  if (g_cond_for_notifier_with_mutex != NULL) {
    return kClockManagerSuccess;
  }

  g_cond_for_notifier_with_mutex = (EsfClockManagerCondNotifier *)malloc(
      sizeof(EsfClockManagerCondNotifier));
  if (g_cond_for_notifier_with_mutex == NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:%s --- malloc failed.\n",
                     "clock_manager_internal.c", __LINE__, __func__);
    return kClockManagerInternalError;
  }

  g_cond_for_notifier_with_mutex->m_cond_base.m_mutex = NULL;
  g_cond_for_notifier_with_mutex->m_cond_base.m_cond = NULL;

  const EsfClockManagerCondBaseReturnValue rv = EsfClockManagerInitCondBase(
      &(g_cond_for_notifier_with_mutex->m_cond_base));
  if (rv != kCondBaseRvIsSuccess) {
    free(g_cond_for_notifier_with_mutex);
    g_cond_for_notifier_with_mutex = NULL;
    return kClockManagerInternalError;
  }

  g_cond_for_notifier_with_mutex->m_notifications.m_head = NULL;
  g_cond_for_notifier_with_mutex->m_notifications.m_tail = NULL;

  return kClockManagerSuccess;
}

STATIC EsfClockManagerReturnValue EsfClockManagerDeinitCondForNotifier(void) {
  if (g_cond_for_notifier_with_mutex != NULL) {
    while (g_cond_for_notifier_with_mutex->m_notifications.m_head != NULL) {
      const EsfClockManagerNotification *box =
          g_cond_for_notifier_with_mutex->m_notifications.m_head;
      g_cond_for_notifier_with_mutex->m_notifications.m_head = box->m_next;
      free((void *)box);
      box = NULL;
    }
    g_cond_for_notifier_with_mutex->m_notifications.m_tail = NULL;
    EsfClockManagerDeinitCondBase(
        &(g_cond_for_notifier_with_mutex->m_cond_base));
  }

  free(g_cond_for_notifier_with_mutex);
  g_cond_for_notifier_with_mutex = NULL;

  return kClockManagerSuccess;
}

STATIC EsfClockManagerNtpTimeSyncStatus EsfClockManagerJudgeNtpTimeSyncComplete(
    const struct ntpc_status_s *ptr_ntpc_status) {
  if (ptr_ntpc_status == NULL) {
    return kHasNotCompletedYet;
  }
  /*printf(
      "ptr_ntpc_status->pid=%d, ptr_ntpc_status->ntpc_daemon_state=%d, "
      "ptr_ntpc_status->nsamples=%u, ptr_ntpc_status->num_of_errors=%d\n",
      ptr_ntpc_status->pid, ptr_ntpc_status->ntpc_daemon_state,
      ptr_ntpc_status->nsamples, ptr_ntpc_status->num_of_errors);*/
  WRITE_DLOG_INFO(
      MODULE_ID_SYSTEM,
      "%s-%d:%s --- ptr_ntpc_status->pid=%d, "
      "ptr_ntpc_status->ntpc_daemon_state=%d, ptr_ntpc_status->nsamples=%u, "
      "ptr_ntpc_status->num_of_errors=%d\n",
      "clock_manager_internal.c", __LINE__, __func__, ptr_ntpc_status->pid,
      ptr_ntpc_status->ntpc_daemon_state, ptr_ntpc_status->nsamples,
      ptr_ntpc_status->num_of_errors);
  if ((ptr_ntpc_status->pid < (pid_t)0) ||
      (ptr_ntpc_status->ntpc_daemon_state != NTP_RUNNING)) {
    return kHasNotCompletedYet;
  }
  if (ptr_ntpc_status->nsamples == 0) {
    if (ptr_ntpc_status->num_of_errors >=
        CONFIG_NETUTILS_NTPCLIENT_NUM_SAMPLES) {
      return kNtpTimeSyncFailure;
    } else {
      return kHasNotCompletedYet;
    }
  } else {
    return kNtpTimeSyncSuccess;
  }
}

STATIC EsfClockManagerReturnValue EsfClockManagerPostNtpTimeSyncCompleteNotif(
    const struct ntpc_status_s *const ptr_ntpc_status) {
  if (ptr_ntpc_status == NULL) {
    return kClockManagerParamError;
  }
  const EsfClockManagerNtpTimeSyncStatus ntp_status =
      EsfClockManagerJudgeNtpTimeSyncComplete(ptr_ntpc_status);
  switch (ntp_status) {
    case kNtpTimeSyncFailure:
      return kClockManagerInternalError;
      break;
    case kNtpTimeSyncSuccess:
      EsfClockManagerPostNotification(kCondTypeIsNtpSyncComplete, (int)true);
      return kClockManagerSuccess;
      break;
    case kHasNotCompletedYet:  // fallthrough
    default:
      break;
  }

  return kClockManagerParamError;
}

STATIC EsfClockManagerReturnValue EsfClockManagerPostNotification(
    const EsfClockManagerNotificationType notification_type,
    const int additional) {
  const int rv =
      pthread_mutex_lock(g_notifier_thread_id_with_mutex->m_mutex_base.m_mutex);
  if (g_notifier_thread_id_with_mutex->m_thread_id ==
      ESF_CLOCK_MANAGER_INVALID_THREAD_ID) {
    WRITE_DLOG_WARN(MODULE_ID_SYSTEM,
                    "%s-%d:%s --- Notifier thread id is invalid.\n",
                    "clock_manager_internal.c", __LINE__, __func__);
    if (!rv) {
      pthread_mutex_unlock(
          g_notifier_thread_id_with_mutex->m_mutex_base.m_mutex);
    }
    return kClockManagerNotifierHasAlreadyFinished;
  }
  if (!rv) {
    pthread_mutex_unlock(g_notifier_thread_id_with_mutex->m_mutex_base.m_mutex);
  }
  EsfClockManagerNotification *notif =
      (EsfClockManagerNotification *)malloc(sizeof(*notif));
  if (notif == NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:%s --- malloc failed.\n",
                     "clock_manager_internal.c", __LINE__, __func__);
    return kClockManagerInternalError;
  }

  notif->m_notification_type = notification_type;
  notif->m_additional = additional;
  notif->m_next = NULL;

  const int rv2 =
      pthread_mutex_lock(g_cond_for_notifier_with_mutex->m_cond_base.m_mutex);
  const int rv_notif =
      pthread_mutex_lock(g_notifier_thread_id_with_mutex->m_mutex_base.m_mutex);
  if (g_notifier_thread_id_with_mutex->m_thread_id !=
      ESF_CLOCK_MANAGER_INVALID_THREAD_ID) {
    if (g_cond_for_notifier_with_mutex->m_notifications.m_tail == NULL) {
      g_cond_for_notifier_with_mutex->m_notifications.m_tail = notif;
      g_cond_for_notifier_with_mutex->m_notifications.m_head = notif;
    } else {
      g_cond_for_notifier_with_mutex->m_notifications.m_tail->m_next = notif;
      g_cond_for_notifier_with_mutex->m_notifications.m_tail = notif;
    }
    pthread_cond_signal(g_cond_for_notifier_with_mutex->m_cond_base.m_cond);

    if (!rv_notif) {
      pthread_mutex_unlock(
          g_notifier_thread_id_with_mutex->m_mutex_base.m_mutex);
    }
    if (!rv2) {
      pthread_mutex_unlock(g_cond_for_notifier_with_mutex->m_cond_base.m_mutex);
    }
  } else {
    if (!rv_notif) {
      pthread_mutex_unlock(
          g_notifier_thread_id_with_mutex->m_mutex_base.m_mutex);
    }
    if (!rv2) {
      pthread_mutex_unlock(g_cond_for_notifier_with_mutex->m_cond_base.m_mutex);
    }
    free(notif);
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:%s --- malloc is OK, but Notifier thread id is invalid.\n",
        "clock_manager_internal.c", __LINE__, __func__);
    return kClockManagerNotifierHasAlreadyFinished;
  }

  return kClockManagerSuccess;
}

STATIC bool EsfClockManagerProcessNotificationEvent(
    const EsfClockManagerNotification *const notification) {
  void (*const cb_func)(bool) = g_ntp_sync_complete_cb_func;
  bool is_fin = false;
  switch (notification->m_notification_type) {
    case kCondTypeIsNothing:
      break;
    case kCondTypeIsFinReq:
      is_fin = true;
      break;
    case kCondTypeIsNtpSyncComplete:
      if (cb_func != NULL) {
        const bool is_sync_complete = notification->m_additional != (int)false;
        if (!is_sync_complete) {
          WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (uint16_t)0x8108);
          WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                           "%s-%d:%s --- NTP time synchronization "
                           "completed with failure.\n",
                           "clock_manager_internal.c", __LINE__, __func__);
        } else {
          WRITE_ELOG_INFO(MODULE_ID_SYSTEM, (uint16_t)0x8109);
          WRITE_DLOG_INFO(MODULE_ID_SYSTEM,
                          "%s-%d:%s --- NTP time synchronization completed "
                          "successfully.\n",
                          "clock_manager_internal.c", __LINE__, __func__);
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
  struct timespec abs_time = {0};
  WRITE_DLOG_INFO(MODULE_ID_SYSTEM, "%s-%d:%s --- has just started.\n",
                  "clock_manager_internal.c", __LINE__, __func__);

  while (true) {
    int rv =
        pthread_mutex_lock(g_cond_for_notifier_with_mutex->m_cond_base.m_mutex);
    if (rv) {
      WRITE_DLOG_WARN(MODULE_ID_SYSTEM,
                      "%s-%d:%s --- pthread_mutex_lock failed:%d.\n",
                      "clock_manager_internal.c", __LINE__, __func__, rv);
    }
    while (g_cond_for_notifier_with_mutex->m_notifications.m_head != NULL) {
      const EsfClockManagerNotification *notif =
          g_cond_for_notifier_with_mutex->m_notifications.m_head;

      g_cond_for_notifier_with_mutex->m_notifications.m_head = notif->m_next;
      if (notif->m_next == NULL) {
        g_cond_for_notifier_with_mutex->m_notifications.m_tail = NULL;
      }
      if (!rv) {
        pthread_mutex_unlock(
            g_cond_for_notifier_with_mutex->m_cond_base.m_mutex);
      }
      is_exit = EsfClockManagerProcessNotificationEvent(notif);
      free((void *)notif);
      notif = NULL;

      if (is_exit) {
        break;
      }

      rv = pthread_mutex_lock(
          g_cond_for_notifier_with_mutex->m_cond_base.m_mutex);
    }
    if (is_exit) {
      break;
    }
    if (!rv) {
      EsfClockManagerCalculateAbstimeInMonotonic(&abs_time, g_sleep_time);
      pthread_cond_timedwait(
          g_cond_for_notifier_with_mutex->m_cond_base.m_cond,
          g_cond_for_notifier_with_mutex->m_cond_base.m_mutex, &abs_time);
      pthread_mutex_unlock(g_cond_for_notifier_with_mutex->m_cond_base.m_mutex);
    } else {
      sleep((unsigned int)(g_sleep_time / 1000U));
    }
  }

  const int rv_notif =
      pthread_mutex_lock(g_notifier_thread_id_with_mutex->m_mutex_base.m_mutex);
  if (rv_notif) {
    WRITE_DLOG_WARN(MODULE_ID_SYSTEM,
                    "%s-%d:%s --- pthread_mutex_lock failed:%d.\n",
                    "clock_manager_internal.c", __LINE__, __func__, rv_notif);
  }
  g_notifier_thread_id_with_mutex->m_thread_id =
      ESF_CLOCK_MANAGER_INVALID_THREAD_ID;
  if (!rv_notif) {
    pthread_mutex_unlock(g_notifier_thread_id_with_mutex->m_mutex_base.m_mutex);
  }

  if (g_cond_for_notifier_with_mutex != NULL) {
    const int rv =
        pthread_mutex_lock(g_cond_for_notifier_with_mutex->m_cond_base.m_mutex);
    if (rv) {
      WRITE_DLOG_WARN(MODULE_ID_SYSTEM,
                      "%s-%d:%s --- pthread_mutex_lock failed:%d.\n",
                      "clock_manager_internal.c", __LINE__, __func__, rv);
    }
    while (g_cond_for_notifier_with_mutex->m_notifications.m_head != NULL) {
      const EsfClockManagerNotification *box =
          g_cond_for_notifier_with_mutex->m_notifications.m_head;
      g_cond_for_notifier_with_mutex->m_notifications.m_head = box->m_next;
      free((void *)box);
      box = NULL;
    }
    if (!rv) {
      pthread_mutex_unlock(g_cond_for_notifier_with_mutex->m_cond_base.m_mutex);
    }
  }

  WRITE_DLOG_INFO(MODULE_ID_SYSTEM, "%s-%d:%s --- is about to end.\n",
                  "clock_manager_internal.c", __LINE__, __func__);
  return NULL;
}
