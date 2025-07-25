/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

// Includes --------------------------------------------------------------------
#include "utility_timer.h"

#include <errno.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include "utility_log.h"
#include "utility_log_module_id.h"

#if defined(__NuttX__)
#include <nuttx/clock.h>
#else
/* Wrapping up_puts() and get_errno() */
#include "internal/compatibility.h"
#ifndef NSEC_PER_SEC
#define NSEC_PER_SEC      (1000000000L)
#endif
#endif

// Macros ----------------------------------------------------------------------
#define kTimerObjMax UTILITY_TIMER_MAX
#define kTimerEventMax (kTimerObjMax + 2)

#define kIntervalMinSec  (CLOCKRES_MIN / NSEC_PER_SEC)
#define kIntervalMinNSec (CLOCKRES_MIN % NSEC_PER_SEC)
#define kIntervalMinNs   ((int64_t)CLOCKRES_MIN)
#define kIntervalMaxSec  (0x7FFFFFFE)  // (LONG_MAX - 1) = 2,147,483,646
#define kIntervalMaxNSec (((NSEC_PER_SEC - 1) / CLOCKRES_MIN) * CLOCKRES_MIN)
#define kIntervalMaxNs \
  (((int64_t)kIntervalMaxSec * NSEC_PER_SEC) + kIntervalMaxNSec)

#define TIMER_ADD_NSEC(tv1p, tv2p, calcp)               \
  (calcp)->tv_sec = (tv1p)->tv_sec + (tv2p)->tv_sec;    \
  (calcp)->tv_nsec = (tv1p)->tv_nsec + (tv2p)->tv_nsec; \
  if ((calcp)->tv_nsec >= NSEC_PER_SEC) {               \
    (calcp)->tv_sec++;                                  \
    (calcp)->tv_nsec -= NSEC_PER_SEC;                   \
  }

// count up for TimerEvent's index
#define ADD_EVENT_INDEX(index)       \
  {                                  \
    (index)++;                       \
    if ((index) >= kTimerEventMax) { \
      (index) = 0;                   \
    }                                \
  }

#define EVENT_ID  0xA200
#define EVENT_UID_START (0x00)
#define EVENT_ID_START (EVENT_UID_START + 0x01)

#define ERR_PRINTF(event_id, format, ...) \
  WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:" \
                   format, __FILE__, __LINE__, ##__VA_ARGS__); \
  WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (EVENT_ID | (EVENT_ID_START + event_id)));

#define ERR_PRINTF_WITH_ID(event_id, format, ...) \
  WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:" \
                   format, __FILE__, __LINE__, ##__VA_ARGS__); \
  WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (EVENT_ID | event_id));

#define INFO_PRINTF(fmt, ...) \
  WRITE_DLOG_INFO(MODULE_ID_SYSTEM, "%s-%d:" fmt, \
                  __FILE__, __LINE__, ##__VA_ARGS__);

#define DBG_PRINTF(fmt, ...) \
  WRITE_DLOG_DEBUG(MODULE_ID_SYSTEM, "%s-%d:" fmt, \
                    __FILE__, __LINE__, ##__VA_ARGS__);

#define TRACE_PRINTF(fmt, ...) \
  WRITE_DLOG_TRACE(MODULE_ID_SYSTEM, "%s-%d:" fmt, \
                    __FILE__, __LINE__, ##__VA_ARGS__);

#define UTILITY_TIMER_ELOG_OS_ERROR            (EVENT_UID_START + 0x00)

// Typedefs --------------------------------------------------------------------
typedef struct {
  bool is_using;
  bool is_running;
  int timer_id;
  timer_t os_timer_handle;
  UtilityTimerCallback callback;
  void *cb_params;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  UtilityTimerErrCode thread_result;
} TimerObj;

typedef enum {
  kMsgTypeEmpty = 0,
  kMsgTypeTimerCreate,
  kMsgTypeExecCallback,
  kMsgTypeKillThread
} MsgType;

// timer message a block
typedef struct Message {
  MsgType type;
  union {
    TimerObj *timer_obj;
    UtilityTimerErrCode *thread_result;
  } msg;
} Message;

// Store the Read and Write position for Event
typedef struct TimerEventList {
  uint16_t read_index;                 // read index for RcvEvent buffers
  uint16_t write_index;                // write index for RcvEvent buffers
  Message  msg[kTimerEventMax];
} TimerEvent;

typedef enum {
  kStateReady = 0,
  kStateInitialized,
  kStateFinalizing,
} State;

// External functions ----------------------------------------------------------
extern int pthread_setname_np(pthread_t thread, const char *name);
// Local functions -------------------------------------------------------------
static void *TimerThreadMain(void *p);

static bool IsValidInterval(const struct timespec *interval_ts);
static UtilityTimerErrCode KillTimerThread(TimerEvent *event, int timer_id);
static UtilityTimerErrCode TimerSetTime(const UtilityTimerHandle timer_handle,
                                        const struct itimerspec *itval,
                                        bool is_stop_timer);
static UtilityTimerErrCode InitializeTimerObj(void);
static UtilityTimerErrCode DestroyTimerObj(TimerObj *timer_obj);
static UtilityTimerErrCode TimerCreate(const UtilityTimerCallback callback,
                                       void *cb_params,
                                       int priority,
                                       size_t stack_size,
                                       TimerObj *timer_obj);
static UtilityTimerErrCode TimerDelete(UtilityTimerHandle timer_handle);
static void SetSigact(void);
static bool IsValidTimerObj(TimerObj *obj);
static UtilityTimerErrCode GetTimerObj(TimerObj **obj);
static UtilityTimerErrCode FreeTimerObj(TimerObj *obj);
static bool IsExistEvent(TimerObj *obj);
static UtilityTimerErrCode TimerSetEvent(TimerObj *obj, MsgType type,
                                         TimerEvent *event_mng);
static UtilityTimerErrCode TimerGetEvent(Message *msg,
                                         TimerEvent *event_mng);
static void SigPrint(const char *format, ...);

// Global Variables ------------------------------------------------------------
static State s_state = kStateReady;
static pthread_t s_thread_handle[kTimerObjMax];
static pthread_mutex_t s_obj_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t s_api_mutex = PTHREAD_MUTEX_INITIALIZER;
static sem_t s_event_sem[kTimerObjMax];
static TimerObj s_timer_obj[kTimerObjMax] = {0};
static TimerEvent s_user_event[kTimerObjMax] = {0};
static TimerEvent s_signal_event[kTimerObjMax] = {0};

// Functions -------------------------------------------------------------------
UtilityTimerErrCode UtilityTimerInitialize(void) {
  UtilityTimerErrCode timer_err = kUtilityTimerOk;
  int os_err = 0;

  os_err = pthread_mutex_lock(&s_api_mutex);
  if (os_err != 0) {
    ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR,
                      "pthread_mutex_lock=%d", os_err);
    return kUtilityTimerErrInternal;
  }
  if (s_state != kStateReady) {
    ERR_PRINTF(0x00, "s_status is %d", s_state);
    timer_err = kUtilityTimerErrInvalidStatus;
    goto unlock;
  }

  timer_err = InitializeTimerObj();
  if (timer_err != kUtilityTimerOk) {
    ERR_PRINTF(0x01, "InitializeTimerObj=%d", timer_err);
    goto unlock;
  }

  memset(&s_user_event, 0, sizeof(s_user_event));
  memset(&s_signal_event, 0, sizeof(s_signal_event));

  for (int i = 0; i < kTimerObjMax; i++) {
    os_err = sem_init(&s_event_sem[i], 0, 0);
    if (os_err != 0) {
      ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR,
                          "sem_init=%d", os_err);
      timer_err = kUtilityTimerErrInternal;
      goto unlock;
    }
    s_thread_handle[i] = -1;
  }
  s_state = kStateInitialized;

unlock:
  os_err = pthread_mutex_unlock(&s_api_mutex);
  if (os_err != 0) {
    ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR,
                      "pthread_mutex_unlock=%d", os_err);
    if (timer_err == kUtilityTimerOk) {
      timer_err = kUtilityTimerErrInternal;
    }
  }

  return timer_err;
}
//------------------------------------------------------------------------------
UtilityTimerErrCode UtilityTimerCreateEx(
                                  const UtilityTimerCallback callback,
                                  void *cb_params,
                                  int priority,
                                  size_t stacksize,
                                  UtilityTimerHandle *timer_handle) {
  UtilityTimerErrCode timer_err = kUtilityTimerOk;
  int os_err;
  os_err = pthread_mutex_lock(&s_api_mutex);
  if (os_err != 0) {
    ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR,
                        "pthread_mutex_lock=%d", os_err);
    return kUtilityTimerErrInternal;
  }
  if (s_state != kStateInitialized) {
    ERR_PRINTF(0x02, "s_state is %d", s_state);
    timer_err = kUtilityTimerErrInvalidStatus;
    goto unlock;
  }
  if (callback == NULL) {
    ERR_PRINTF(0x03, "callback NULL");
    timer_err = kUtilityTimerErrInvalidParams;
    goto unlock;
  }
  if (timer_handle == NULL) {
    ERR_PRINTF(0x04, "timer_handle NULL");
    timer_err = kUtilityTimerErrInvalidParams;
    goto unlock;
  }

  TimerObj *timer_obj = NULL;

  timer_err = GetTimerObj(&timer_obj);
  if (timer_err != kUtilityTimerOk) {
    ERR_PRINTF(0x05, "GetTimerObj()=%d", timer_err);
    goto unlock;
  }

  timer_err = TimerCreate(callback, cb_params, priority, stacksize, timer_obj);
  if (timer_err != kUtilityTimerOk) {
    ERR_PRINTF(0x06, "TimerCreate=%u", timer_err);
    FreeTimerObj(timer_obj);
    timer_obj = NULL;
    goto unlock;
  }

  *timer_handle = (UtilityTimerHandle *)timer_obj;

  DBG_PRINTF("(%p) OK", *timer_handle);

unlock:
  os_err = pthread_mutex_unlock(&s_api_mutex);
  if (os_err != 0) {
    ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR,
                        "pthread_mutex_unlock()=%d", os_err);
    if (timer_err == kUtilityTimerOk) {
      TimerDelete(timer_obj);
      *timer_handle = (UtilityTimerHandle *)NULL;
      timer_err = kUtilityTimerErrInternal;
    }
  }

  return timer_err;
}
//------------------------------------------------------------------------------
UtilityTimerErrCode UtilityTimerCreate(
    const UtilityTimerCallback utility_timer_cb, void *timer_cb_params,
    UtilityTimerHandle *utility_timer_handle) {
  return UtilityTimerCreateEx(utility_timer_cb,
                            timer_cb_params,
                            CONFIG_UTILITY_TIMER_THREAD_PRIORITY,
                            4096,
                            utility_timer_handle);
}
//------------------------------------------------------------------------------
UtilityTimerErrCode UtilityTimerStart(const UtilityTimerHandle timer_handle,
                                      const struct timespec *interval_ts,
                                      UtilityTimerRepeatType repeat_type) {
  UtilityTimerErrCode timer_err = kUtilityTimerOk;
  int os_err = pthread_mutex_lock(&s_api_mutex);
  if (os_err != 0) {
    ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR,
                      "pthread_mutex_lock()=%d", os_err);
    return kUtilityTimerErrInternal;
  }
  if (s_state != kStateInitialized) {
    ERR_PRINTF(0x07, "s_state is %d", s_state);
    timer_err = kUtilityTimerErrInvalidStatus;
    goto unlock;
  }
  if (timer_handle == NULL) {
    ERR_PRINTF(0x08, "timer_handle NULL");
    timer_err = kUtilityTimerErrInvalidParams;
    goto unlock;
  }
  if (interval_ts == NULL) {
    ERR_PRINTF(0x09, "interval_ts NULL");
    timer_err = kUtilityTimerErrInvalidParams;
    goto unlock;
  }
  if (!IsValidTimerObj((TimerObj *)timer_handle)) {
    ERR_PRINTF(0x0A, "Invalid timer_handle=%p", timer_handle);
    timer_err = kUtilityTimerErrNotFound;
    goto unlock;
  }

  if (!IsValidInterval(interval_ts)) {
    ERR_PRINTF(0x0B, "IsValidInterval(sec=%lld, nsec=%ld)", interval_ts->tv_sec,
               interval_ts->tv_nsec);
    timer_err = kUtilityTimerErrInvalidParams;
    goto unlock;
  }

  uint64_t fix_interval_ns = 0;
  struct timespec fix_interval_ts = {0};

  fix_interval_ns = (interval_ts->tv_sec * NSEC_PER_SEC) +
                    (uint64_t)interval_ts->tv_nsec;
  fix_interval_ns += (CLOCKRES_MIN - 1);
  fix_interval_ns /= CLOCKRES_MIN;
  fix_interval_ns *= CLOCKRES_MIN;
  fix_interval_ts.tv_sec = fix_interval_ns / NSEC_PER_SEC;
  fix_interval_ts.tv_nsec = fix_interval_ns % NSEC_PER_SEC;

  struct itimerspec itval = {0};

  itval.it_value = fix_interval_ts;
  if (repeat_type == kUtilityTimerRepeat) {
    itval.it_interval = fix_interval_ts;
  }

  timer_err = TimerSetTime(timer_handle, &itval, true);
  if (timer_err != kUtilityTimerOk) {
    ERR_PRINTF(0x0D, "(%p) TimerSetTime()=%d", timer_handle, timer_err);
  }

  DBG_PRINTF("(%p) OK", timer_handle);

unlock:
  os_err = pthread_mutex_unlock(&s_api_mutex);
  if (os_err != 0) {
    ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR,
                        "pthread_mutex_unlock()=%d", os_err);
    if (timer_err == kUtilityTimerOk) {
      memset(&itval, 0, sizeof(itval));
      TimerSetTime(timer_handle, &itval, false);
      timer_err = kUtilityTimerErrInternal;
    }
  }
  return timer_err;
}
//------------------------------------------------------------------------------
UtilityTimerErrCode UtilityTimerStop(const UtilityTimerHandle timer_handle) {
  UtilityTimerErrCode timer_err = kUtilityTimerOk;
  int os_err;
  os_err = pthread_mutex_lock(&s_api_mutex);
  if (os_err != 0) {
    ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR,
                      "pthread_mutex_lock()=%d", os_err);
    return kUtilityTimerErrInternal;
  }
  if (s_state != kStateInitialized) {
    ERR_PRINTF(0x0E, "s_state is %d", s_state);
    timer_err = kUtilityTimerErrInvalidStatus;
    goto unlock;
  }
  if (timer_handle == NULL) {
    ERR_PRINTF(0x0F, "timer_handle NULL");
    timer_err = kUtilityTimerErrInvalidParams;
    goto unlock;
  }

  if (!IsValidTimerObj((TimerObj *)timer_handle)) {
    ERR_PRINTF(0x10, "Invalid timer_handle=%p", timer_handle);
    timer_err = kUtilityTimerErrNotFound;
    goto unlock;
  }

  struct itimerspec itval;
  memset(&itval, 0, sizeof(itval));

  timer_err = TimerSetTime(timer_handle, &itval, false);
  if (timer_err != kUtilityTimerOk) {
    ERR_PRINTF(0x11, "(%p) TimerSetTime=%u", timer_handle, timer_err);
    goto unlock;
  }

  DBG_PRINTF("(%p) OK", timer_handle);

unlock:
  os_err = pthread_mutex_unlock(&s_api_mutex);
  if (os_err != 0) {
    ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR,
                      "pthread_mutex_unlock()=%d", os_err);
    if (timer_err == kUtilityTimerOk) {
      timer_err = kUtilityTimerErrInternal;
    }
  }
  return timer_err;
}
//------------------------------------------------------------------------------
UtilityTimerErrCode UtilityTimerDelete(UtilityTimerHandle timer_handle) {
  UtilityTimerErrCode timer_err = kUtilityTimerOk;
  int os_err;
  os_err = pthread_mutex_lock(&s_api_mutex);
  if (os_err != 0) {
    ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR,
                      "pthread_mutex_lock()=%d", os_err);
    return kUtilityTimerErrInternal;
  }
  if (s_state != kStateInitialized) {
    ERR_PRINTF(0x12, "s_state is %d", s_state);
    timer_err = kUtilityTimerErrInvalidStatus;
    goto unlock;
  }
  if (timer_handle == NULL) {
    ERR_PRINTF(0x13, "timer_handle NULL");
    timer_err = kUtilityTimerErrInvalidParams;
    goto unlock;
  }

  if (!IsValidTimerObj((TimerObj *)timer_handle)) {
    ERR_PRINTF(0x14, "Invalid timer_handle=%p", timer_handle);
    timer_err = kUtilityTimerErrNotFound;
    goto unlock;
  }

  TimerObj *timer_obj = (TimerObj *)timer_handle;

  timer_err = TimerDelete(timer_obj);
  if (timer_err != kUtilityTimerOk) {
    ERR_PRINTF(0x15, "(%p) TimerDelete()=%d", timer_handle, timer_err);
    goto unlock;
  }

  DBG_PRINTF("(%p) OK", timer_handle);
unlock:
  os_err = pthread_mutex_unlock(&s_api_mutex);
  if (os_err != 0) {
    ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR,
                      "pthread_mutex_unlock()=%d", os_err);
    if (timer_err == kUtilityTimerOk) {
      timer_err = kUtilityTimerErrInternal;
    }
  }
  return timer_err;
}
//------------------------------------------------------------------------------
UtilityTimerErrCode UtilityTimerFinalize(void) {
  UtilityTimerErrCode timer_err = kUtilityTimerOk;
  int os_err = 0;
  os_err = pthread_mutex_lock(&s_api_mutex);
  if (os_err != 0) {
    ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR,
                      "pthread_mutex_lock()=%d", os_err);
    return kUtilityTimerErrInternal;
  }

  if (s_state != kStateInitialized) {
    ERR_PRINTF(0x16, "s_state is %d", s_state);
    timer_err = kUtilityTimerErrInvalidStatus;
    goto err_mutex_end;
  }

  // For timeout signals during Finalize
  s_state = kStateFinalizing;

  // finalize timer info list
  // Note: Even if an error occurs, processing continues and all elements
  // are processed.

  for (uint16_t i = 0; i < kTimerObjMax; i++) {
    TimerObj *obj = &s_timer_obj[i];
    if (obj->is_running) {
      struct itimerspec itval;
      memset(&itval, 0, sizeof(itval));
      timer_err = TimerSetTime(obj, &itval, false);
      if (timer_err != kUtilityTimerOk) {
        ERR_PRINTF(0x17, "(%p) TimerSetTime()=%d", obj, timer_err);
      }
    }
    if (obj->is_using) {
      UtilityTimerErrCode timer_ret = kUtilityTimerOk;
      timer_ret = TimerDelete(obj);
      if (timer_ret != kUtilityTimerOk) {
        ERR_PRINTF(0x18, "(%p) TimerDelete=%u", obj, timer_ret);
        if (timer_err == kUtilityTimerOk) {
          timer_err = timer_ret;
        }
      }
    }
  }

  for (uint16_t i = 0; i < kTimerObjMax; i++) {
    UtilityTimerErrCode timer_ret = kUtilityTimerOk;
    timer_ret = DestroyTimerObj(&s_timer_obj[i]);
    if (timer_ret != kUtilityTimerOk) {
      ERR_PRINTF(0x19, "DestroyTimerObj=%u", timer_ret);
      if (timer_err == kUtilityTimerOk) {
        timer_err = timer_ret;
      }
    }
  }

  for (uint16_t i = 0; i < kTimerObjMax; i++) {
    if (s_thread_handle[i] != -1) {
      UtilityTimerErrCode timer_ret_ = kUtilityTimerOk;
      timer_ret_ = KillTimerThread(&s_user_event[i], i);
      if (timer_ret_ != kUtilityTimerOk) {
        ERR_PRINTF(0x1A, "KillTimerThread=%d", timer_ret_);
        if (timer_err == kUtilityTimerOk) {
          timer_err = kUtilityTimerErrInternal;
        }
      }
    }
  }

  for (int i = 0; i < kTimerObjMax; i++) {
    os_err = sem_destroy(&s_event_sem[i]);
    if (os_err != 0) {
      ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR,
                          "sem_destroy=%d", os_err);
      if (timer_err == kUtilityTimerOk) {
        timer_err = kUtilityTimerErrInternal;
      }
    }
  }

  s_state = kStateReady;

err_mutex_end:
  os_err = pthread_mutex_unlock(&s_api_mutex);
  if (os_err != 0) {
    ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR,
                      "pthread_mutex_unlock()=%d", os_err);
    if (timer_err == kUtilityTimerOk) {
      timer_err = kUtilityTimerErrInternal;
    }
  }
  return timer_err;
}
//------------------------------------------------------------------------------
UtilityTimerErrCode UtilityTimerGetSystemInfo(
    UtilityTimerSystemInfo *utility_timer_sysinfo) {
  UtilityTimerErrCode timer_err = kUtilityTimerOk;
  int os_err;
  os_err = pthread_mutex_lock(&s_api_mutex);
  if (os_err != 0) {
    ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR,
                      "pthread_mutex_lock()=%d", os_err);
    return kUtilityTimerErrInternal;
  }
  if (s_state != kStateInitialized) {
    ERR_PRINTF(0x1B, "s_state is %d", s_state);
    timer_err = kUtilityTimerErrInvalidStatus;
    goto unlock;
  }

  if (utility_timer_sysinfo == NULL) {
    ERR_PRINTF(0x1C, "utility_timer_sysinfo NULL");
    timer_err = kUtilityTimerErrInvalidParams;
    goto unlock;
  }

  utility_timer_sysinfo->interval_min_ts.tv_sec = kIntervalMinSec;
  utility_timer_sysinfo->interval_min_ts.tv_nsec = kIntervalMinNSec;
  utility_timer_sysinfo->interval_max_ts.tv_sec = kIntervalMaxSec;
  utility_timer_sysinfo->interval_max_ts.tv_nsec = kIntervalMaxNSec;

unlock:
  os_err = pthread_mutex_unlock(&s_api_mutex);
  if (os_err != 0) {
    ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR,
                      "pthread_mutex_unlock()=%d", os_err);
    if (timer_err == kUtilityTimerOk) {
      timer_err = kUtilityTimerErrInternal;
    }
  }
  return timer_err;
}
//------------------------------------------------------------------------------
static bool IsValidInterval(const struct timespec *interval) {
  int64_t interval_ns = ((int64_t)interval->tv_sec * NSEC_PER_SEC) +
                        interval->tv_nsec;
  if (interval_ns < kIntervalMinNs) {
    ERR_PRINTF(0x1E, "interval_ns %lld < min %lld",
               interval_ns, kIntervalMinNs);
    return false;
  }
  if (interval_ns > kIntervalMaxNs) {
    ERR_PRINTF(0x1F, "interval_ns %lld > max %lld",
               interval_ns, kIntervalMaxNs);
    return false;
  }
  return true;
}
//------------------------------------------------------------------------------
static UtilityTimerErrCode KillTimerThread(TimerEvent *event, int timer_id) {
  UtilityTimerErrCode timer_err = kUtilityTimerOk;
  int os_err = 0;

  if (s_thread_handle[timer_id] != -1) {
    timer_err = TimerSetEvent(&s_timer_obj[timer_id], kMsgTypeKillThread,
                              event);
    if (timer_err != kUtilityTimerOk) {
      ERR_PRINTF(0x20, "(%d) TimerSetEvent=%u", kMsgTypeKillThread, timer_err);
      goto end;
    }
    os_err = pthread_join(s_thread_handle[timer_id], NULL);
    if (os_err != 0) {
      ERR_PRINTF(0x37, "Failed to pthread_join:%d errno:%d", os_err, errno);
      timer_err = kUtilityTimerErrInternal;
      goto end;
    }
    s_thread_handle[timer_id] = -1;
  }

end:
  return timer_err;
}
// -----------------------------------------------------------------------------
static UtilityTimerErrCode TimerSetTime(const UtilityTimerHandle timer_handle,
                                        const struct itimerspec *itval,
                                        bool is_start_timer) {
  TimerObj *timer_obj = (TimerObj *)timer_handle;

  if (timer_obj->is_using == false) {
    ERR_PRINTF(0x23, "(%p) element already free", timer_handle);
    return kUtilityTimerErrNotFound;
  }

  int os_err = pthread_mutex_lock(&timer_obj->mutex);
  if (os_err != 0) {
    ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR,
                      "(%p) pthread_mutex_lock()=%d", timer_handle, os_err);
    return kUtilityTimerErrInternal;
  }

  UtilityTimerErrCode timer_err = kUtilityTimerOk;
  if (timer_obj->is_running == is_start_timer) {
    ERR_PRINTF(0x24, "(%p) is_running=%d", timer_handle, timer_obj->is_running);
    timer_err = kUtilityTimerErrInvalidStatus;
    goto err_mutex_unlock;
  }

  struct itimerspec itval_addtime = *itval;
  if ((0 < itval->it_value.tv_sec) || (0 < itval->it_value.tv_nsec)) {
    struct timespec currtime = {0};
    clock_gettime(CLOCK_MONOTONIC, &currtime);
    TIMER_ADD_NSEC(&(itval->it_value), &currtime, &(itval_addtime.it_value));
  }

  os_err = timer_settime(timer_obj->os_timer_handle, TIMER_ABSTIME,
                         &itval_addtime, NULL);
  if (os_err != 0) {
    ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR,
                      "(%p) timer_settime()=%d", timer_handle, os_err);
    timer_err = kUtilityTimerErrInternal;
    goto err_mutex_unlock;
  }

  timer_obj->is_running = is_start_timer;

  os_err = pthread_mutex_unlock(&timer_obj->mutex);
  if (os_err != 0) {
    ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR,
                      "(%p) pthread_mutex_unlock()=%d", timer_handle, os_err);
    if (is_start_timer) {
      struct itimerspec itval_stop = {0};
      memset(&itval_stop, 0, sizeof(itval_stop));
      timer_settime(timer_obj->os_timer_handle, 0, &itval_stop, NULL);
      timer_obj->is_running = false;
    }
    return kUtilityTimerErrInternal;
  }

  DBG_PRINTF("(%p) OK", timer_handle);
  return kUtilityTimerOk;

err_mutex_unlock:
  pthread_mutex_unlock(&timer_obj->mutex);
  return timer_err;
}
// -----------------------------------------------------------------------------
static UtilityTimerErrCode InitializeTimerObj(void) {
  for (int i = 0; i < kTimerObjMax; i++) {
    TimerObj *obj = &s_timer_obj[i];
    obj->thread_result = kUtilityTimerErrInternal;
    obj->timer_id = i;

    int ret = pthread_mutex_init(&obj->mutex, NULL);
    if (ret != 0) {
      ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR,
                        "(%p) pthread_mutex_init=%d", obj, ret);
      return kUtilityTimerErrInternal;
    }

    ret = pthread_cond_init(&obj->cond, NULL);
    if (ret != 0) {
      ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR,
                        "(%p) pthread_cond_init=%d", obj, ret);
      return kUtilityTimerErrInternal;
    }
  }
  return kUtilityTimerOk;
}
// -----------------------------------------------------------------------------
static UtilityTimerErrCode DestroyTimerObj(TimerObj *timer_obj) {
  int os_err = pthread_cond_destroy(&timer_obj->cond);
  if (os_err != 0) {
    ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR,
                      "(%p) pthread_cond_destroy=%d", timer_obj, os_err);
    return kUtilityTimerErrInternal;
  }

  os_err = pthread_mutex_destroy(&timer_obj->mutex);
  if (os_err != 0) {
    ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR,
                      "(%p) pthread_mutex_destroy()=%d", timer_obj, os_err);
    return kUtilityTimerErrInternal;
  }

  return kUtilityTimerOk;
}
// -----------------------------------------------------------------------------
static UtilityTimerErrCode TimerCreate(const UtilityTimerCallback callback,
                                       void *cb_params,
                                       int priority,
                                       size_t stack_size,
                                       TimerObj *timer_obj) {
  int os_err = pthread_mutex_lock(&timer_obj->mutex);
  if (os_err != 0) {
    ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR,
                      "(%p) pthread_mutex_lock=%d", timer_obj, os_err);
    return kUtilityTimerErrInternal;
  }

  UtilityTimerErrCode timer_err = kUtilityTimerOk;
  pthread_attr_t thread_attr = {0};
  os_err = pthread_attr_init(&thread_attr);
  if (os_err != 0) {
    ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR,
                      "pthread_attr_init=%d", os_err);
    timer_err = kUtilityTimerErrInternal;
    goto err_free_and_unlock;
  }
  os_err = pthread_attr_setschedpolicy(&thread_attr, SCHED_RR);
  if (os_err != 0) {
    ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR,
                      "pthread_attr_setschedpolicy=%d", os_err);
    timer_err = kUtilityTimerErrInternal;
    goto err_free_and_unlock;
  }
  struct sched_param sch_param = {0};
  os_err = pthread_attr_getschedparam(&thread_attr, &sch_param);
  if (os_err != 0) {
    ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR,
                      "pthread_attr_getschedparam=%d", os_err);
    timer_err = kUtilityTimerErrInternal;
    goto err_free_and_unlock;
  }

  sch_param.sched_priority = priority;
  os_err = pthread_attr_setschedparam(&thread_attr, &sch_param);
  if (os_err != 0) {
    ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR,
                       "Failed to pthread_attr_setschedparam:%d errno=%d",
                       os_err, errno);
    timer_err = kUtilityTimerErrInternal;
    goto err_free_and_unlock;
  }

#if defined(__NuttX__)
  os_err = pthread_attr_setstacksize(&thread_attr, stack_size);
  if (os_err != 0) {
    ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR,
                      "pthread_attr_setstacksize=%d", os_err);
    timer_err = kUtilityTimerErrInternal;
    goto err_free_and_unlock;
  }
#endif

  int thread_id = timer_obj->timer_id;
  os_err = pthread_create(&s_thread_handle[thread_id], &thread_attr,
                          TimerThreadMain, (void*)(intptr_t)thread_id);
  if (os_err != 0) {
    ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR,
                      "pthread_create=%d", os_err);
    timer_err = kUtilityTimerErrInternal;
    goto err_free_and_unlock;
  }

  char name[CONFIG_NAME_MAX + 1] = {0};
  snprintf(name, CONFIG_NAME_MAX, "UtilityTimerThread_%d", thread_id);
  pthread_setname_np(s_thread_handle[thread_id], name);

  timer_obj->thread_result = kUtilityTimerErrInternal;

  timer_err = TimerSetEvent(timer_obj,
                            kMsgTypeTimerCreate, &s_user_event[thread_id]);
  if (timer_err != kUtilityTimerOk) {
    ERR_PRINTF(0x26, "(%d) TimerSetEvent()=%d", kMsgTypeTimerCreate, timer_err);
    goto err_free_and_unlock;
  }

  DBG_PRINTF("(%p) pthread_cond_wait() start", timer_obj);

  os_err = pthread_cond_wait(&timer_obj->cond, &timer_obj->mutex);
  if (os_err != 0) {
    ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR,
                      "(%p) pthread_cond_wait()=%d", timer_obj, os_err);
    timer_err = kUtilityTimerErrInternal;
    goto err_free_and_unlock;
  }

  DBG_PRINTF("(%p) pthread_cond_wait() resume", timer_obj);

  if (timer_obj->thread_result != kUtilityTimerOk) {
    ERR_PRINTF(0x27, "(%p) thread_result=%d",
               timer_obj, timer_obj->thread_result);
    timer_err = timer_obj->thread_result;
    goto err_free_and_unlock;
  }

  timer_obj->callback = callback;
  timer_obj->cb_params = cb_params;

  os_err = pthread_mutex_unlock(&timer_obj->mutex);
  if (os_err != 0) {
    ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR,
                      "(%p) pthread_mutex_unlock()=%d", timer_obj, os_err);
    return kUtilityTimerErrInternal;
  }

  return kUtilityTimerOk;

err_free_and_unlock:
  pthread_mutex_unlock(&timer_obj->mutex);
  return timer_err;
}
// -----------------------------------------------------------------------------
static UtilityTimerErrCode TimerDelete(UtilityTimerHandle timer_handle) {
  TimerObj *timer_obj = (TimerObj *)timer_handle;

  if (timer_obj->is_using == false) {
    // already free element
    ERR_PRINTF(0x29, "(%p) element already free", timer_handle);
    return kUtilityTimerErrNotFound;
  }

  int os_err = pthread_mutex_lock(&timer_obj->mutex);
  if (os_err != 0) {
    ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR,
                      "(%p) pthread_mutex_lock()=%d", timer_handle, os_err);
    return kUtilityTimerErrInternal;
  }

  UtilityTimerErrCode timer_err = kUtilityTimerOk;
  if (timer_obj->is_running != false) {
    ERR_PRINTF(0x2A, "(%p) is_running=%d", timer_handle, timer_obj->is_running);
    timer_err = kUtilityTimerErrInvalidStatus;
    goto err_mutex_unlock;
  }

  if (timer_obj->os_timer_handle != NULL) {
    os_err = timer_delete(timer_obj->os_timer_handle);
    if (os_err != 0) {
      ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR,
                       "(%p) timer_delete=%d", timer_handle, os_err);
      timer_err = kUtilityTimerErrInternal;
      goto err_mutex_unlock;
    }
    memset(&timer_obj->os_timer_handle, 0, sizeof(timer_obj->os_timer_handle));
    timer_obj->callback = NULL;
    timer_obj->cb_params = NULL;
  }

  os_err = pthread_mutex_unlock(&timer_obj->mutex);
  if (os_err != 0) {
    ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR,
                      "(%p) pthread_mutex_unlock()=%d", timer_handle, os_err);
    return kUtilityTimerErrInternal;
  }

  int timer_id = timer_obj->timer_id;
  timer_err = KillTimerThread(&s_user_event[timer_id], timer_id);
  if (timer_err != kUtilityTimerOk) {
    ERR_PRINTF(0x2B, "KillTimerThread=%u", timer_err);
    return kUtilityTimerErrInternal;
  }

  // free timer info element
  // The registerd event is skipped by TimerThreadMain because the element
  // has "timer_obj->is_using"
  timer_err = FreeTimerObj(timer_obj);
  if (timer_err != kUtilityTimerOk) {
    ERR_PRINTF(0x2C, "(%p) FreeTimerObj=%u", timer_handle, timer_err);
    return kUtilityTimerErrInternal;
  }

  return kUtilityTimerOk;

err_mutex_unlock:
  pthread_mutex_unlock(&timer_obj->mutex);
  return timer_err;
}
// -----------------------------------------------------------------------------
static void TimerThreadCb(int signo, siginfo_t *info, void *ucontext) {
  (void)signo;     // Avoid compiler warning
  (void)ucontext;  // Avoid compiler warning

  if (s_state == kStateFinalizing) {
    // when conflict Finainalize and signal,
    // skip the TimerSetEvent() for signal
    SigPrint("recieve signal is abnormal\n");
    return;
  }

  if (info->si_value.sival_ptr == NULL) {
    SigPrint("sival_ptr NULL\n");
    return;
  }

  TimerObj *timer_obj = (TimerObj *)info->si_value.sival_ptr;
  TimerSetEvent(timer_obj, kMsgTypeExecCallback,
                &s_signal_event[timer_obj->timer_id]);
  return;
}
//------------------------------------------------------------------------------
static void SetSigact(void) {
  struct sigaction sig_act = {0};

  sig_act.sa_sigaction = TimerThreadCb;
  sig_act.sa_flags = SA_SIGINFO;

  sigemptyset(&sig_act.sa_mask);
  int os_err = sigaction(SIGALRM, &sig_act, NULL);
  if (os_err != 0) {
    ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR, "sigaction()=%d", os_err);
  }

  return;
}
//------------------------------------------------------------------------------
static bool IsValidTimerObj(TimerObj *obj) {
  for (int i = 0; i < kTimerObjMax; i++) {
    if (obj == &s_timer_obj[i]) {
      return true;
    }
  }
  return false;
}
//------------------------------------------------------------------------------
static UtilityTimerErrCode GetTimerObj(TimerObj **obj) {
  *obj = (TimerObj *)NULL;

  int os_err = pthread_mutex_lock(&s_obj_mutex);
  if (os_err != 0) {
    ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR,
                      "pthread_mutex_lock()=%d", os_err);
    return kUtilityTimerErrInternal;
  }

  UtilityTimerErrCode timer_err = kUtilityTimerErrInternal;
  uint16_t i = 0;
  for (i = 0; i < kTimerObjMax; i++) {
    if (s_timer_obj[i].is_using == false) {
      // msg_buffer[] was already clearn when is_using[] change to false.
      *obj = &s_timer_obj[i];
      s_timer_obj[i].is_using = true;
      timer_err = kUtilityTimerOk;
      break;
    }
  }

  os_err = pthread_mutex_unlock(&s_obj_mutex);
  if (os_err != 0) {
    ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR,
                      "pthread_mutex_lock(s_obj_mutex)=%d", os_err);

    if (*obj != (TimerObj *)NULL) {
      s_timer_obj[i].is_using = false;
    }
    return kUtilityTimerErrInternal;
  }

  if (*obj == (TimerObj *)NULL) {
    // All timer already used
    ERR_PRINTF(0x2E, "Free timer is Empty. (all running) max.=%d",
               kTimerObjMax);
    timer_err = kUtilityTimerErrBusy;
  }

  return timer_err;
}
//------------------------------------------------------------------------------
static UtilityTimerErrCode FreeTimerObj(TimerObj *obj) {
  if (!IsValidTimerObj(obj)) {
    ERR_PRINTF(0x2F, "Invalid timer_handle=%p", obj);
    return kUtilityTimerErrNotFound;
  }

  int os_err = pthread_mutex_lock(&s_obj_mutex);
  if (os_err != 0) {
    ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR,
                      "pthread_mutex_lock()=%d", os_err);
    return kUtilityTimerErrInternal;
  }

  obj->is_using = false;
  memset(&obj->is_running, false, sizeof(obj->is_running));
  memset(&obj->thread_result, 0, sizeof(obj->thread_result));

  os_err = pthread_mutex_unlock(&s_obj_mutex);
  if (os_err != 0) {
    ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR,
                      "pthread_mutex_lock(s_obj_mutex)=%d", os_err);
    return kUtilityTimerErrInternal;
  }

  return kUtilityTimerOk;
}
//------------------------------------------------------------------------------
static bool IsExistEvent(TimerObj *obj) {
  // Search registerd event for matching TimerObj
  int timer_id = obj->timer_id;
  for (uint16_t i = 0; i < kTimerEventMax; i++) {
    if (s_user_event[timer_id].msg[i].msg.timer_obj == obj) {
      return true;
    }
    if (s_signal_event[timer_id].msg[i].msg.timer_obj == obj) {
      return true;
    }
  }
  return false;
}
//------------------------------------------------------------------------------
static UtilityTimerErrCode TimerSetEvent(TimerObj *obj, MsgType type,
                                         TimerEvent *event_mng) {
  if (type != kMsgTypeKillThread) {
    if (!IsValidTimerObj(obj)) {
      // uncontrolled element
      SigPrint("Invalid timer_handle=%p\n", obj);
      return kUtilityTimerErrNotFound;
    }
    if (IsExistEvent(obj) == true) {
      return kUtilityTimerOk;
    }
  }

  // Register new event
  Message *msg = NULL;
  uint16_t write_id = event_mng->write_index;
  msg = (Message *)&event_mng->msg[write_id];
  msg->type = type;
  if (type != kMsgTypeKillThread) {
    msg->msg.timer_obj = obj;
  }
  ADD_EVENT_INDEX(event_mng->write_index);

  int os_err = sem_post(&s_event_sem[obj->timer_id]);
  if (os_err != 0) {
    SigPrint("sem_post()=%d\n", os_err);
    return kUtilityTimerErrInternal;
  }

  return kUtilityTimerOk;
}
//------------------------------------------------------------------------------
static UtilityTimerErrCode TimerGetEvent(Message *msg,
                                         TimerEvent *event_mng) {
  UtilityTimerErrCode timer_err = kUtilityTimerErrNotFound;
  uint16_t write_id = event_mng->write_index;
  uint16_t read_id = event_mng->read_index;

  if (read_id != write_id) {
    memcpy(msg, &event_mng->msg[read_id], sizeof(Message));
    event_mng->msg[read_id].type = kMsgTypeEmpty;
    event_mng->msg[read_id].msg.timer_obj = (TimerObj *)NULL;
    ADD_EVENT_INDEX(event_mng->read_index);
    timer_err = kUtilityTimerOk;
  }

  return timer_err;
}
// -----------------------------------------------------------------------------
static UtilityTimerErrCode TimerThreadTimerCreate(TimerObj *timer_obj) {
  int os_err = pthread_mutex_lock(&timer_obj->mutex);
  if (os_err != 0) {
    ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR,
                        "pthread_mutex_lock()=%d", os_err);
    timer_obj->thread_result = kUtilityTimerErrInternal;
    goto err_end;
  }

  sigset_t sig_set = {0};

  sigemptyset(&sig_set);
  sigaddset(&sig_set, SIGALRM);

  os_err = sigprocmask(SIG_UNBLOCK, &sig_set, NULL);
  if (os_err != 0) {
    ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR, "sigprocmask()=%d", os_err);
    timer_obj->thread_result = kUtilityTimerErrInternal;
    goto err_mutex_unlock;
  }

  struct sigevent notify = {0};
  timer_t timer_id = 0;

  notify.sigev_notify = SIGEV_SIGNAL;
  notify.sigev_signo = SIGALRM;
  notify.sigev_value.sival_ptr = timer_obj;
  os_err = timer_create(CLOCK_MONOTONIC, &notify, &timer_id);
  if (os_err != 0) {
    ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR,
                        "timer_create()=%d", os_err);
    timer_obj->thread_result = kUtilityTimerErrInternal;
    goto err_mutex_unlock;
  }

  timer_obj->os_timer_handle = timer_id;
  timer_obj->thread_result = kUtilityTimerOk;

  os_err = pthread_cond_signal(&timer_obj->cond);
  if (os_err != 0) {
    ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR,
                        "pthread_cond_signal()=%d", os_err);
    timer_obj->thread_result = kUtilityTimerErrInternal;
    goto err_timer_stop;
  }
  os_err = pthread_mutex_unlock(&timer_obj->mutex);
  if (os_err != 0) {
    ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR,
                        "thread_mutex_unlock()=%d", os_err);
    timer_obj->thread_result = kUtilityTimerErrInternal;
    goto err_timer_stop;
  }

  return timer_obj->thread_result;

err_timer_stop:
  timer_delete(timer_id);
  timer_obj->os_timer_handle = NULL;
err_mutex_unlock:
  pthread_mutex_unlock(&timer_obj->mutex);
err_end:
  pthread_cond_signal(&timer_obj->cond);
  return timer_obj->thread_result;
}
// -----------------------------------------------------------------------------
UtilityTimerErrCode TimerThreadCallBack(TimerObj *timer_obj) {
  UtilityTimerCallback callback = NULL;
  int os_err = 0;

  int os_err_trylock = pthread_mutex_trylock(&timer_obj->mutex);
  if ((os_err_trylock != 0) && (os_err_trylock != EBUSY)) {
    ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR,
                "(%p) pthread_mutex_trylock()=%d", timer_obj, os_err_trylock);
    timer_obj->thread_result = kUtilityTimerErrInternal;
    return kUtilityTimerErrInternal;
  }

  if (timer_obj->is_running == false) {
    DBG_PRINTF("(%p) is_running=%d", timer_obj, timer_obj->is_running);
    if (os_err_trylock == 0) {
      pthread_mutex_unlock(&timer_obj->mutex);
      timer_obj->thread_result = kUtilityTimerOk;
      return kUtilityTimerOk;
    }
  }

  timer_obj->thread_result = kUtilityTimerOk;
  callback = timer_obj->callback;
  void *cb_params = timer_obj->cb_params;

  if (os_err_trylock == 0) {
    os_err = pthread_mutex_unlock(&timer_obj->mutex);
    if (os_err != 0) {
      ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR,
                          "(%p) pthread_mutex_unlock()=%d", timer_obj, os_err);
      timer_obj->thread_result = kUtilityTimerErrInternal;
      return kUtilityTimerErrInternal;
    }
  }

  callback(cb_params);

  return kUtilityTimerOk;
}
// -----------------------------------------------------------------------------
static void *TimerThreadMain(void *arg) {
  UtilityTimerErrCode timer_err = kUtilityTimerOk;

  SetSigact();  // create with high-priority thread.

  int os_err = 0;
  int thread_id = (intptr_t)arg;

  while (1) {
    os_err = sem_wait(&s_event_sem[thread_id]);
    if (os_err != 0) {
      os_err = get_errno();
      if (os_err != EINTR) {
        ERR_PRINTF_WITH_ID(UTILITY_TIMER_ELOG_OS_ERROR,
                            "sem_wait()=%d", os_err);
      }
      continue;
    }

    Message recv_buf = {0};
    while (1) {
      timer_err = TimerGetEvent(&recv_buf, &s_signal_event[thread_id]);
      if (timer_err == kUtilityTimerErrNotFound) {
        break;
      }
      TRACE_PRINTF("ThreadMain(signal) type=%u body=%p ",
                           recv_buf.type,
                           recv_buf.msg.timer_obj);

      switch (recv_buf.type) {
        case kMsgTypeExecCallback:
          if (recv_buf.msg.timer_obj->is_using == false) {
            break;
          }

          timer_err = TimerThreadCallBack(recv_buf.msg.timer_obj);
          if (timer_err != kUtilityTimerOk) {
            ERR_PRINTF(0x34, "TimerThreadCallBack()=%d", timer_err);
          }
          break;

        default:
          break;
      }
    }

    while (1) {
      timer_err = TimerGetEvent(&recv_buf, &s_user_event[thread_id]);
      if (timer_err == kUtilityTimerErrNotFound) {
        break;
      }
      TRACE_PRINTF("ThreadMain(user) type=%u body=%p ",
                           recv_buf.type,
                           recv_buf.msg.timer_obj);

      switch (recv_buf.type) {
        case kMsgTypeTimerCreate:
          timer_err = TimerThreadTimerCreate(recv_buf.msg.timer_obj);
          if (timer_err != kUtilityTimerOk) {
            ERR_PRINTF(0x36, "TimerThreadTimerCreate()=%d", timer_err);
          }
          break;

        case kMsgTypeKillThread:
          DBG_PRINTF("kMsgTypeKillThread end");
          goto end;

        default:
          break;
      }
    }
  }

end:
  return NULL;
}
// -----------------------------------------------------------------------------
static void SigPrint(const char *format, ...) {
#if defined(__linux__)
  // Do not output messages during interrupts
  return;
#else
  va_list list = {0};
  char logstr[256] = {0};
  va_start(list, format);
  vsnprintf(logstr, sizeof(logstr), format, list);
  va_end(list);
  logstr[sizeof(logstr) - 1] = '\0';  // NULL char
  up_puts(logstr);
  return;
#endif
}
