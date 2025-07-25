/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "clock_manager.h"

#include <pthread.h>

#include "clock_manager_internal.h"
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

// """Initializes mutex for APIs which multithread call.

// This function creates an object of pthread_mutex_t, then initializes it.
// If static global variable: g_mutex_for_threads is not NULL, this function
// does not create an object of pthread_mutex_t.

// Args:
//    no arguments.

// Returns:
//    The following values are returned:
//    kClockManagerSuccess: success.
//    kClockManagerInternalError: internal error broke out.

// """
STATIC EsfClockManagerReturnValue EsfClockManagerInitMutex(void);

// """Deinitializes mutex for APIs which multithread call.

// This function destroys the object of pthread_mutex_t, then free it.

// Args:
//    no arguments.

// Returns:
//    The following values are returned:
//    kClockManagerSuccess: success.
//    kClockManagerInternalError: internal error broke out.

// """
STATIC EsfClockManagerReturnValue EsfClockManagerDeinitMutex(void);

// """Marks entering threads-manipulations.

// This function marks entering threads-manipulations.
// Threads-manipulations stands for creating or destroying threads.

// Args:
//    no arguments.

// Returns:
//    errno

// """
STATIC int EsfClockManagerEnterThreadsManipulation(void);

// """Unmarks by leaving threads-manipulations.

// This function marks leaving threads-manipulations.
// Threads-manipulations stands for creating or destroying threads.

// Args:
//    no arguments.

// Returns:
//    errno

// """
STATIC int EsfClockManagerLeaveThreadsManipulation(void);

// """Starts threads and NTP Client Daemon.

// When this function is called, the caller must get the mutex.
// Starts threads which belong to Clock Manager and NTP Client Daemon.

// Args:
//    no arguments.

// Returns:
//    errno

// """
STATIC EsfClockManagerReturnValue EsfClockManagerStartInternal(void);

// """Stops threads and NTP Client Daemon.

// When this function is called, the caller must get the mutex.
// Stops threads and NTP Client Daemon.

// Args:
//    no arguments.

// Returns:
//    errno

// """
#ifndef CONFIG_EXTERNAL_TARGET_RPI
STATIC EsfClockManagerReturnValue EsfClockManagerStopInternal(void);
#endif // CONFIG_EXTERNAL_TARGET_RPI

/**
 * Definitions of global variables
 */

STATIC pthread_mutex_t *g_mutex_for_threads = NULL;
#ifdef CONFIG_EXTERNAL_TARGET_RPI
STATIC void (*g_ntp_sync_complete_cb_func)(bool) = NULL;
#endif  // CONFIG_EXTERNAL_TARGET_RPI
/**
 * Definitions of public functions
 */

EsfClockManagerReturnValue EsfClockManagerInit(void) {
  const EsfClockManagerReturnValue rv = EsfClockManagerInitMutex();
  if (rv != kClockManagerSuccess) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- EsfClockManagerInitMutex failed:%d\n",
                     "clock_manager.c", __LINE__, __func__, rv);
    return kClockManagerInternalError;
  }

  const EsfClockManagerReturnValue rv2 = EsfClockManagerInitSetting();
  if (rv2 != kClockManagerSuccess) {
    EsfClockManagerDeinitMutex();
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- EsfClockManagerInitSetting failed:%d\n",
                     "clock_manager.c", __LINE__, __func__, rv2);
    return kClockManagerInternalError;
  }

#ifndef CONFIG_EXTERNAL_TARGET_RPI
  const EsfClockManagerReturnValue rv3 = EsfClockManagerInitInternal();
  if (rv3 != kClockManagerSuccess) {
    EsfClockManagerDeinitSetting();
    EsfClockManagerDeinitMutex();
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- EsfClockManagerInitInternal failed:%d\n",
                     "clock_manager.c", __LINE__, __func__, rv3);
    return kClockManagerInternalError;
  }

  const EsfClockManagerReturnValue rv4 =
      EsfClockManagerRegisterFactoryResetCb();
  if (rv4 != kClockManagerSuccess) {
    EsfClockManagerDeinitInternal();
    EsfClockManagerDeinitSetting();
    EsfClockManagerDeinitMutex();
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:%s --- EsfClockManagerRegisterFactoryResetCb failed:%d\n",
        "clock_manager.c", __LINE__, __func__, rv4);
    return kClockManagerInternalError;
  }
#endif  // CONFIG_EXTERNAL_TARGET_RPI
  return kClockManagerSuccess;
}

EsfClockManagerReturnValue EsfClockManagerDeinit(void) {
#ifndef CONFIG_EXTERNAL_TARGET_RPI
  const EsfClockManagerReturnValue rv_stop = EsfClockManagerStop();
  WRITE_DLOG_INFO(MODULE_ID_SYSTEM, "%s-%d:%s --- EsfClockManagerStop:%d\n",
                  "clock_manager.c", __LINE__, __func__, rv_stop);
  if (rv_stop != kClockManagerSuccess) {
    const int rv_cancel_monitor = EsfClockManagerCancelMonitorThread();
    NOT_USED(rv_cancel_monitor);
    const int rv_cancel_notifier = EsfClockManagerCancelNotifierThread();
    NOT_USED(rv_cancel_notifier);
  }

  EsfClockManagerUnregisterFactoryResetCb();
  EsfClockManagerDeinitSetting();
  EsfClockManagerDeinitInternal();
  EsfClockManagerDeinitMutex();
#else
  EsfClockManagerDeinitSetting();
  EsfClockManagerDeinitMutex();
#endif  // CONFIG_EXTERNAL_TARGET_RPI
  return kClockManagerSuccess;
}

EsfClockManagerReturnValue EsfClockManagerStart(void) {
  if (g_mutex_for_threads == NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- EsfClockManagerInit must be called "
                     "before this function is called.\n",
                     "clock_manager.c", __LINE__, __func__);
    return kClockManagerStateTransitionError;
  }
  if (EsfClockManagerEnterThreadsManipulation()) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- Could not enter the critical region.\n",
                     "clock_manager.c", __LINE__, __func__);
    return kClockManagerInternalError;
  }

  EsfClockManagerMarkStartingSync();
  const EsfClockManagerReturnValue rv = EsfClockManagerStartInternal();
  if (rv != kClockManagerSuccess) {
    EsfClockManagerMarkCompletedSync();
  }

  EsfClockManagerLeaveThreadsManipulation();
  return rv;
}

EsfClockManagerReturnValue EsfClockManagerStop(void) {
#ifndef CONFIG_EXTERNAL_TARGET_RPI
  if (g_mutex_for_threads == NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- EsfClockManagerInit must be called "
                     "before this function is called.\n",
                     "clock_manager.c", __LINE__, __func__);
    return kClockManagerStateTransitionError;
  }
  EsfClockManagerEnterThreadsManipulation();
  const EsfClockManagerReturnValue rv = EsfClockManagerStopInternal();
  EsfClockManagerLeaveThreadsManipulation();
  return rv;
#else
  return kClockManagerSuccess;
#endif // CONFIG_EXTERNAL_TARGET_RPI
}

EsfClockManagerReturnValue EsfClockManagerRegisterCbOnNtpSyncComplete(
    void (*on_ntp_sync_complete)(bool)) {
  if (on_ntp_sync_complete == NULL) {
    WRITE_DLOG_WARN(MODULE_ID_SYSTEM,
                    "%s-%d:%s --- The formal parameter is NULL.\n",
                    "clock_manager.c", __LINE__, __func__);
    return kClockManagerParamError;
  }
  if (g_mutex_for_threads == NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- EsfClockManagerInit must be called "
                     "before this function is called.\n",
                     "clock_manager.c", __LINE__, __func__);
    return kClockManagerStateTransitionError;
  }
  if (EsfClockManagerEnterThreadsManipulation()) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- Could not enter the critical region.\n",
                     "clock_manager.c", __LINE__, __func__);
    return kClockManagerInternalError;
  }
#ifndef CONFIG_EXTERNAL_TARGET_RPI
  const EsfClockManagerReturnValue ret_register_cb =
      EsfClockManagerRegisterNtpSyncCompleteCb(on_ntp_sync_complete);
  EsfClockManagerLeaveThreadsManipulation();
  return ret_register_cb;
#else
  g_ntp_sync_complete_cb_func = on_ntp_sync_complete;
  EsfClockManagerLeaveThreadsManipulation();
  return kClockManagerSuccess;
#endif // CONFIG_EXTERNAL_TARGET_RPI
}

EsfClockManagerReturnValue EsfClockManagerUnregisterCbOnNtpSyncComplete(void) {
  if (g_mutex_for_threads == NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- EsfClockManagerInit must be called "
                     "before this function is called.\n",
                     "clock_manager.c", __LINE__, __func__);
    return kClockManagerStateTransitionError;
  }
  if (EsfClockManagerEnterThreadsManipulation()) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- Could not enter the critical region.\n",
                     "clock_manager.c", __LINE__, __func__);
    return kClockManagerInternalError;
  }
#ifndef CONFIG_EXTERNAL_TARGET_RPI
  const EsfClockManagerReturnValue ret_unregister_cb =
      EsfClockManagerUnregisterNtpSyncCompleteCb();
  EsfClockManagerLeaveThreadsManipulation();
  return ret_unregister_cb;
#else
  g_ntp_sync_complete_cb_func = NULL;
  EsfClockManagerLeaveThreadsManipulation();
  return kClockManagerSuccess;
#endif // CONFIG_EXTERNAL_TARGET_RPI
}

/**
 * Definitions of package private functions
 */

STATIC EsfClockManagerReturnValue EsfClockManagerInitMutex(void) {
  if (g_mutex_for_threads != NULL) {
    return kClockManagerSuccess;
  }

  g_mutex_for_threads = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
  if (g_mutex_for_threads == NULL) {
    WRITE_DLOG_CRITICAL(
        MODULE_ID_SYSTEM,
        "%s-%d:%s --- malloc(sizeof(pthread_mutex_t)) failed.\n",
        "clock_manager.c", __LINE__, __func__);
    return kClockManagerInternalError;
  }

  const int rv = pthread_mutex_init(g_mutex_for_threads, NULL);
  if (rv) {
    free(g_mutex_for_threads);
    g_mutex_for_threads = NULL;
    WRITE_DLOG_CRITICAL(MODULE_ID_SYSTEM,
                        "%s-%d:%s --- pthread_mutex_init failed:%d\n",
                        "clock_manager.c", __LINE__, __func__, rv);
    return kClockManagerInternalError;
  }

  return kClockManagerSuccess;
}

STATIC EsfClockManagerReturnValue EsfClockManagerDeinitMutex(void) {
  if (g_mutex_for_threads != NULL) {
    const int rv = pthread_mutex_destroy(g_mutex_for_threads);
    NOT_USED(rv);
  }

  free(g_mutex_for_threads);
  g_mutex_for_threads = NULL;

  return kClockManagerSuccess;
}

STATIC int EsfClockManagerEnterThreadsManipulation(void) {
  return pthread_mutex_lock(g_mutex_for_threads);
}

STATIC int EsfClockManagerLeaveThreadsManipulation(void) {
  return pthread_mutex_unlock(g_mutex_for_threads);
}

STATIC EsfClockManagerReturnValue EsfClockManagerStartInternal(void) {
#ifndef CONFIG_EXTERNAL_TARGET_RPI
  EsfClockManagerParams *params_obj =
      (EsfClockManagerParams *)malloc(sizeof(*params_obj));
  if (params_obj == NULL) {
    WRITE_DLOG_CRITICAL(MODULE_ID_SYSTEM,
                        "%s-%d:%s --- malloc(sizeof(*params_obj)) failed.\n",
                        "clock_manager.c", __LINE__, __func__);
    return kClockManagerInternalError;
  }

  EsfClockManagerGetParamsInternal(params_obj, CALLER_IS_START_INTERNAL);

  EsfClockManagerReturnValue rv = EsfClockManagerCreateMonitorThread(
      1000 * params_obj->common.polling_time);
  if (rv != kClockManagerSuccess) {
    free(params_obj);
    return rv;
  }

  rv = EsfClockManagerCreateNotifierThread();
  if (rv != kClockManagerSuccess) {
    EsfClockManagerStopInternal();
    free(params_obj);
    return rv;
  }

  rv = EsfClockManagerStartNtpClientDaemon(params_obj);
  free(params_obj);
  if (rv != kClockManagerSuccess) {
    EsfClockManagerStopInternal();
    return rv;
  }
#else
  if (g_ntp_sync_complete_cb_func)
  g_ntp_sync_complete_cb_func(true);
#endif // CONFIG_EXTERNAL_TARGET_RPI
  return kClockManagerSuccess;
}

#ifndef CONFIG_EXTERNAL_TARGET_RPI
STATIC EsfClockManagerReturnValue EsfClockManagerStopInternal(void) {
  const EsfClockManagerReturnValue result_in_fin_notifier =
      EsfClockManagerDestroyNotifier();

  const EsfClockManagerReturnValue result_in_fin =
      EsfClockManagerDestroyMonitorThread();

  const EsfClockManagerReturnValue result_in_daemon_fin =
      EsfClockManagerWaitForTerminatingDaemon();

  EsfClockManagerMarkCompletedSync();

  WRITE_DLOG_INFO(MODULE_ID_SYSTEM,
                  "%s-%d:%s --- notifier:%d monitor:%d daemon:%d\n",
                  "clock_manager.c", __LINE__, __func__, result_in_fin_notifier,
                  result_in_fin, result_in_daemon_fin);

  if ((result_in_fin_notifier == kClockManagerSuccess ||
       result_in_fin_notifier == kClockManagerNotifierHasAlreadyFinished) &&
      result_in_fin == kClockManagerSuccess &&
      (result_in_daemon_fin == kClockManagerSuccess ||
       result_in_daemon_fin == kClockManagerDaemonHasAleadyFinished)) {
    return kClockManagerSuccess;
  }

  return kClockManagerStateTransitionError;
}
#endif // CONFIG_EXTERNAL_TARGET_RPI
