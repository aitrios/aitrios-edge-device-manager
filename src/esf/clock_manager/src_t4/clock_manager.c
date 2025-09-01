/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "clock_manager.h"

#include <pthread.h>

#include "clock_manager_internal.h"
#include "clock_manager_monitor.h"
#include "clock_manager_notification.h"
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

typedef enum EsfClockManagerState {
  kEsfClockManagerStateIdle,
  kEsfClockManagerStateReady,
  kEsfClockManagerStateRunning,
  kEsfClockManagerStateLock,
} EsfClockManagerState;

STATIC EsfClockManagerState s_state = kEsfClockManagerStateIdle;
STATIC pthread_mutex_t s_state_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * Declarations of private functions --- i.e., functions given storage-class
 * specifier static.
 */

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
STATIC EsfClockManagerReturnValue EsfClockManagerStopInternal(void);

/**
 * Definitions of public functions
 */

EsfClockManagerReturnValue EsfClockManagerInit(void) {
  EsfClockManagerReturnValue rv = kClockManagerSuccess;

  {
    // Status check
    if (pthread_mutex_lock(&s_state_mutex)) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- pthread_mutex_lock failed.\n",
                       "clock_manager.c", __LINE__, __func__);
      return kClockManagerInternalError;
    }

    if (s_state == kEsfClockManagerStateIdle) {
      s_state = kEsfClockManagerStateLock;

    } else if (s_state == kEsfClockManagerStateReady) {
      rv = kClockManagerSuccess;
      goto unlock_exit;

    } else if (s_state == kEsfClockManagerStateRunning) {
      rv = kClockManagerStateTransitionError;
      goto unlock_exit;

    } else {
      rv = kClockManagerStateTransitionError;
      goto unlock_exit;
    }

    if (pthread_mutex_unlock(&s_state_mutex)) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- pthread_mutex_unlock failed.\n",
                       "clock_manager.c", __LINE__, __func__);
      return kClockManagerInternalError;
    }
  }

  rv = EsfClockManagerInitSetting();
  if (rv != kClockManagerSuccess) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- EsfClockManagerInitSetting failed:%d\n",
                     "clock_manager.c", __LINE__, __func__, rv);
    rv = kClockManagerInternalError;
    goto error_exit;
  }

  rv = EsfClockManagerInitMonitor();
  if (rv != kClockManagerSuccess) {
    (void)EsfClockManagerDeinitSetting();
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- EsfClockManagerInitMonitor failed:%d\n",
                     "clock_manager.c", __LINE__, __func__, rv);
    rv = kClockManagerInternalError;
    goto error_exit;
  }

  rv = EsfClockManagerInitNotifier();
  if (rv != kClockManagerSuccess) {
    (void)EsfClockManagerDeinitSetting();
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- EsfClockManagerInitNotifier failed:%d\n",
                     "clock_manager.c", __LINE__, __func__, rv);
    rv = kClockManagerInternalError;
    goto error_exit;
  }

  rv = EsfClockManagerRegisterFactoryResetCb();
  if (rv != kClockManagerSuccess) {
    (void)EsfClockManagerDeinitMonitor();
    (void)EsfClockManagerDeinitSetting();
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:%s --- EsfClockManagerRegisterFactoryResetCb failed:%d\n",
        "clock_manager.c", __LINE__, __func__, rv);
    rv = kClockManagerInternalError;
    goto error_exit;
  }

normal_exit:
  (void)pthread_mutex_lock(&s_state_mutex);
  s_state = kEsfClockManagerStateReady;
  (void)pthread_mutex_unlock(&s_state_mutex);
  return kClockManagerSuccess;

error_exit:
  (void)pthread_mutex_lock(&s_state_mutex);
  s_state = kEsfClockManagerStateIdle;
  (void)pthread_mutex_unlock(&s_state_mutex);
  return rv;

unlock_exit:
  (void)pthread_mutex_unlock(&s_state_mutex);
  return rv;
}

EsfClockManagerReturnValue EsfClockManagerDeinit(void) {
  EsfClockManagerReturnValue rv = kClockManagerSuccess;

  {
    // Status check
    if (pthread_mutex_lock(&s_state_mutex)) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- pthread_mutex_lock failed.\n",
                       "clock_manager.c", __LINE__, __func__);
      return kClockManagerInternalError;
    }

    if (s_state == kEsfClockManagerStateIdle) {
      rv = kClockManagerSuccess;
      goto unlock_exit;

    } else if (s_state == kEsfClockManagerStateReady) {
      s_state = kEsfClockManagerStateLock;

    } else if (s_state == kEsfClockManagerStateRunning) {
      rv = kClockManagerStateTransitionError;
      goto unlock_exit;

    } else {
      rv = kClockManagerStateTransitionError;
      goto unlock_exit;
    }

    if (pthread_mutex_unlock(&s_state_mutex)) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- pthread_mutex_unlock failed.\n",
                       "clock_manager.c", __LINE__, __func__);
      return kClockManagerInternalError;
    }
  }

  EsfClockManagerUnregisterFactoryResetCb();
  EsfClockManagerDeinitSetting();
  EsfClockManagerDeinitNotifier();
  EsfClockManagerDeinitMonitor();

normal_exit:
  (void)pthread_mutex_lock(&s_state_mutex);
  s_state = kEsfClockManagerStateIdle;
  (void)pthread_mutex_unlock(&s_state_mutex);
  return kClockManagerSuccess;

error_exit:
  (void)pthread_mutex_lock(&s_state_mutex);
  s_state = kEsfClockManagerStateReady;
  (void)pthread_mutex_unlock(&s_state_mutex);
  return rv;

unlock_exit:
  (void)pthread_mutex_unlock(&s_state_mutex);
  return rv;
}

EsfClockManagerReturnValue EsfClockManagerStart(void) {
  EsfClockManagerReturnValue rv = kClockManagerSuccess;

  {
    // Status check
    if (pthread_mutex_lock(&s_state_mutex)) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- pthread_mutex_lock failed.\n",
                       "clock_manager.c", __LINE__, __func__);
      return kClockManagerInternalError;
    }

    if (s_state == kEsfClockManagerStateIdle) {
      rv = kClockManagerStateTransitionError;
      goto unlock_exit;

    } else if (s_state == kEsfClockManagerStateReady) {
      s_state = kEsfClockManagerStateLock;

    } else if (s_state == kEsfClockManagerStateRunning) {
      rv = kClockManagerStateTransitionError;
      goto unlock_exit;

    } else {
      rv = kClockManagerStateTransitionError;
      goto unlock_exit;
    }

    if (pthread_mutex_unlock(&s_state_mutex)) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- pthread_mutex_unlock failed.\n",
                       "clock_manager.c", __LINE__, __func__);
      return kClockManagerInternalError;
    }
  }

  EsfClockManagerMarkStartingSync();
  rv = EsfClockManagerStartInternal();
  if (rv != kClockManagerSuccess) {
    EsfClockManagerMarkCompletedSync();
    goto error_exit;
  }

normal_exit:
  (void)pthread_mutex_lock(&s_state_mutex);
  s_state = kEsfClockManagerStateRunning;
  (void)pthread_mutex_unlock(&s_state_mutex);
  return rv;

error_exit:
  (void)pthread_mutex_lock(&s_state_mutex);
  s_state = kEsfClockManagerStateReady;
  (void)pthread_mutex_unlock(&s_state_mutex);
  return rv;

unlock_exit:
  (void)pthread_mutex_unlock(&s_state_mutex);
  return rv;
}

EsfClockManagerReturnValue EsfClockManagerStop(void) {
  EsfClockManagerReturnValue rv = kClockManagerSuccess;

  {
    // Status check
    if (pthread_mutex_lock(&s_state_mutex)) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- pthread_mutex_lock failed.\n",
                       "clock_manager.c", __LINE__, __func__);
      return kClockManagerInternalError;
    }

    if (s_state == kEsfClockManagerStateIdle) {
      rv = kClockManagerStateTransitionError;
      goto unlock_exit;

    } else if (s_state == kEsfClockManagerStateReady) {
      rv = kClockManagerStateTransitionError;
      goto unlock_exit;

    } else if (s_state == kEsfClockManagerStateRunning) {
      s_state = kEsfClockManagerStateLock;

    } else {
      rv = kClockManagerStateTransitionError;
      goto unlock_exit;
    }

    if (pthread_mutex_unlock(&s_state_mutex)) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- pthread_mutex_unlock failed.\n",
                       "clock_manager.c", __LINE__, __func__);
      return kClockManagerInternalError;
    }
  }

  rv = EsfClockManagerStopInternal();
  if (rv != kClockManagerSuccess) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- EsfClockManagerStopInternal failed:%d\n",
                     "clock_manager.c", __LINE__, __func__, rv);
    goto error_exit;
  }

normal_exit:
  (void)pthread_mutex_lock(&s_state_mutex);
  s_state = kEsfClockManagerStateReady;
  (void)pthread_mutex_unlock(&s_state_mutex);
  return rv;

error_exit:
  (void)pthread_mutex_lock(&s_state_mutex);
  s_state = kEsfClockManagerStateRunning;
  (void)pthread_mutex_unlock(&s_state_mutex);
  return rv;

unlock_exit:
  (void)pthread_mutex_unlock(&s_state_mutex);
  return rv;
}

EsfClockManagerReturnValue EsfClockManagerRegisterCbOnNtpSyncComplete(
    void (*on_ntp_sync_complete)(bool)) {
  EsfClockManagerReturnValue rv = kClockManagerSuccess;

  {
    // Status check
    if (pthread_mutex_lock(&s_state_mutex)) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- pthread_mutex_lock failed.\n",
                       "clock_manager.c", __LINE__, __func__);
      return kClockManagerInternalError;
    }

    if (s_state == kEsfClockManagerStateIdle) {
      rv = kClockManagerStateTransitionError;
      goto unlock_exit;

    } else if (s_state == kEsfClockManagerStateReady) {
      // Do nothing.

    } else if (s_state == kEsfClockManagerStateRunning) {
      // Do nothing.

    } else {
      rv = kClockManagerStateTransitionError;
      goto unlock_exit;
    }

    if (pthread_mutex_unlock(&s_state_mutex)) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- pthread_mutex_unlock failed.\n",
                       "clock_manager.c", __LINE__, __func__);
      return kClockManagerInternalError;
    }
  }

  if (on_ntp_sync_complete == NULL) {
    WRITE_DLOG_WARN(MODULE_ID_SYSTEM,
                    "%s-%d:%s --- The formal parameter is NULL.\n",
                    "clock_manager.c", __LINE__, __func__);
    return kClockManagerParamError;
  }

  rv = EsfClockManagerRegisterNtpSyncCompleteCb(on_ntp_sync_complete);
  return rv;

unlock_exit:
  (void)pthread_mutex_unlock(&s_state_mutex);
  return rv;
}

EsfClockManagerReturnValue EsfClockManagerUnregisterCbOnNtpSyncComplete(void) {
  EsfClockManagerReturnValue rv = kClockManagerSuccess;

  {
    // Status check
    if (pthread_mutex_lock(&s_state_mutex)) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- pthread_mutex_lock failed.\n",
                       "clock_manager.c", __LINE__, __func__);
      return kClockManagerInternalError;
    }

    if (s_state == kEsfClockManagerStateIdle) {
      rv = kClockManagerStateTransitionError;
      goto unlock_exit;

    } else if (s_state == kEsfClockManagerStateReady) {
      // Do nothing.

    } else if (s_state == kEsfClockManagerStateRunning) {
      // Do nothing.

    } else {
      rv = kClockManagerStateTransitionError;
      goto unlock_exit;
    }

    if (pthread_mutex_unlock(&s_state_mutex)) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- pthread_mutex_unlock failed.\n",
                       "clock_manager.c", __LINE__, __func__);
      return kClockManagerInternalError;
    }
  }

  rv = EsfClockManagerUnregisterNtpSyncCompleteCb();

  return rv;

unlock_exit:
  (void)pthread_mutex_unlock(&s_state_mutex);
  return rv;
}

EsfClockManagerReturnValue EsfClockManagerSetParamsForcibly(
    const EsfClockManagerParams *data, const EsfClockManagerParamsMask *mask) {
  EsfClockManagerReturnValue rv = kClockManagerSuccess;

  {
    // Status check
    if (pthread_mutex_lock(&s_state_mutex)) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- pthread_mutex_lock failed.\n",
                       "clock_manager.c", __LINE__, __func__);
      return kClockManagerInternalError;
    }

    if (s_state == kEsfClockManagerStateIdle) {
      rv = kClockManagerStateTransitionError;
      goto unlock_exit;

    } else if (s_state == kEsfClockManagerStateReady) {
      // Do nothing.

    } else if (s_state == kEsfClockManagerStateRunning) {
      // Do nothing.

    } else {
      rv = kClockManagerStateTransitionError;
      goto unlock_exit;
    }

    if (pthread_mutex_unlock(&s_state_mutex)) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- pthread_mutex_unlock failed.\n",
                       "clock_manager.c", __LINE__, __func__);
      return kClockManagerInternalError;
    }
  }

  return EsfClockManagerSetParamsForciblyMain(data, mask);

unlock_exit:
  (void)pthread_mutex_unlock(&s_state_mutex);
  return rv;
}

EsfClockManagerReturnValue EsfClockManagerSetParams(
    const EsfClockManagerParams *data, const EsfClockManagerParamsMask *mask) {
  EsfClockManagerReturnValue rv = kClockManagerSuccess;

  {
    // Status check
    if (pthread_mutex_lock(&s_state_mutex)) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- pthread_mutex_lock failed.\n",
                       "clock_manager.c", __LINE__, __func__);
      return kClockManagerInternalError;
    }

    if (s_state == kEsfClockManagerStateIdle) {
      rv = kClockManagerStateTransitionError;
      goto unlock_exit;

    } else if (s_state == kEsfClockManagerStateReady) {
      // Do nothing.

    } else if (s_state == kEsfClockManagerStateRunning) {
      // Do nothing.

    } else {
      rv = kClockManagerStateTransitionError;
      goto unlock_exit;
    }

    if (pthread_mutex_unlock(&s_state_mutex)) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- pthread_mutex_unlock failed.\n",
                       "clock_manager.c", __LINE__, __func__);
      return kClockManagerInternalError;
    }
  }

  return EsfClockManagerSetParamsMain(data, mask);

unlock_exit:
  (void)pthread_mutex_unlock(&s_state_mutex);
  return rv;
}

EsfClockManagerReturnValue EsfClockManagerGetParams(
    EsfClockManagerParams *const data) {
  EsfClockManagerReturnValue rv = kClockManagerSuccess;

  {
    // Status check
    if (pthread_mutex_lock(&s_state_mutex)) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- pthread_mutex_lock failed.\n",
                       "clock_manager.c", __LINE__, __func__);
      return kClockManagerInternalError;
    }

    if (s_state == kEsfClockManagerStateIdle) {
      rv = kClockManagerStateTransitionError;
      goto unlock_exit;

    } else if (s_state == kEsfClockManagerStateReady) {
      // Do nothing.

    } else if (s_state == kEsfClockManagerStateRunning) {
      // Do nothing.

    } else {
      rv = kClockManagerStateTransitionError;
      goto unlock_exit;
    }

    if (pthread_mutex_unlock(&s_state_mutex)) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- pthread_mutex_unlock failed.\n",
                       "clock_manager.c", __LINE__, __func__);
      return kClockManagerInternalError;
    }
  }

  return EsfClockManagerGetParamsMain(data);

unlock_exit:
  (void)pthread_mutex_unlock(&s_state_mutex);
  return rv;
}

/**
 * Definitions of package private functions
 */

STATIC EsfClockManagerReturnValue EsfClockManagerStartInternal(void) {
  EsfClockManagerParams *params_obj =
      (EsfClockManagerParams *)malloc(sizeof(*params_obj));
  if (params_obj == NULL) {
    WRITE_DLOG_CRITICAL(MODULE_ID_SYSTEM,
                        "%s-%d:%s --- malloc(sizeof(*params_obj)) failed.\n",
                        "clock_manager.c", __LINE__, __func__);
    return kClockManagerInternalError;
  }

  EsfClockManagerReturnValue rv =
      EsfClockManagerGetParamsInternal(params_obj, CALLER_IS_START_INTERNAL);
  if (rv != kClockManagerSuccess) {
    free(params_obj);
    return rv;
  }

  int polling_time = params_obj->common.polling_time;

  rv = EsfClockManagerStartNtpClientDaemon(params_obj);
  free(params_obj);
  if (rv != kClockManagerSuccess) {
    EsfClockManagerStopInternal();
    return rv;
  }

  rv = EsfClockManagerCreateMonitorThread(1000 * polling_time);
  if (rv != kClockManagerSuccess) {
    return kClockManagerInternalError;
  }

  rv = EsfClockManagerCreateNotifierThread(1000 * polling_time);
  if (rv != kClockManagerSuccess) {
    EsfClockManagerStopInternal();
    return rv;
  }

  return kClockManagerSuccess;
}

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

  if ((result_in_fin_notifier == kClockManagerSuccess) &&
      (result_in_fin == kClockManagerSuccess) &&
      (result_in_daemon_fin == kClockManagerSuccess)) {
    return kClockManagerSuccess;
  }

  return kClockManagerStateTransitionError;
}
