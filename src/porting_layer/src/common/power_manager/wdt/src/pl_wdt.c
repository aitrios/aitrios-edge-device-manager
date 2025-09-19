/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#include "utility_msg.h"
#include "utility_log_module_id.h"
#include "utility_log.h"

#include "pl_wdt.h"
#include "pl_wdt_lib.h"

int pthread_setname_np(pthread_t thread, const char *name);
// Config check ---------------------------------------------------------------
#if CONFIG_EXTERNAL_PL_WDT0_TIMEOUT_SEC >= CONFIG_EXTERNAL_PL_WDT1_TIMEOUT_SEC
#  error "PL_WDT0_TIMEOUT_SEC is longer than PL_WDT1_TIMEOUT_SEC."
#endif

// Macros ---------------------------------------------------------------------
#define EVENT_ID        (0x9800)
#define EVENT_ID_START  (0x0055)
#define LOG_ERR(event_id, format, ...) \
  WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, \
    "%s-%d:" format, __FILE__, __LINE__, ##__VA_ARGS__); \
  WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (EVENT_ID | \
                  (0x00FF & (EVENT_ID_START + event_id))));

#define LOG_INFO(format, ...) \
  WRITE_DLOG_INFO(MODULE_ID_SYSTEM, \
    "%s-%d:" format, __FILE__, __LINE__, ##__VA_ARGS__);

// Global Variables -----------------------------------------------------------
typedef enum {
  kWdtStateReady = 0,
  kWdtStateStop,
  kWdtStateStart
} WdtState;

typedef enum {
  kMsgTypeKeepAlive = 0,
  kMsgTypeTerminate
} MsgType;

static pthread_mutex_t s_api_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t s_thread_low = 0;
static pthread_t s_thread_high = 0;
static int32_t s_msg_handle = 0;
static WdtState s_state     = kWdtStateReady;
static bool s_is_registered = false;
static bool s_is_exit       = false;
static PlWdtLibHandle  s_wdt_handle[CONFIG_EXTERNAL_PL_WDT_NUM]     = {0};
static struct timespec s_last_recv_time = {0};
static struct timespec s_last_sent_time[CONFIG_EXTERNAL_PL_WDT_NUM] = {0};
static bool s_is_wdt0_timeover = false;
static bool s_is_wdt1_timeover = false;

// Local functions ------------------------------------------------------------
static void *KeepAliveLowThread(void *arg);
static void *KeepAliveHighThread(void *arg);
static PlErrCode WdtStartup(void);
static int GetCurrentTime(struct timespec *time);
static long CalcSubSec(struct timespec current_time,  //NOLINT
                                              struct timespec old_time);
static PlErrCode PlWdtStop_(void);
static PlErrCode PlWdtUnregisterIrqHandler_(void);
// Functions ------------------------------------------------------------------
// ----------------------------------------------------------------------------
PlErrCode PlWdtInitialize(void) {
  PlErrCode err_code = kPlErrCodeOk;
  int ret = 0;

  ret = pthread_mutex_lock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x00, "Lock error. errno=%d", errno);
    err_code = kPlErrLock;
    goto fin;
  }

  if (s_state != kWdtStateReady) {
    LOG_ERR(0x01, "Not Ready state. s_state=%d", s_state);
    err_code = kPlErrInvalidState;
    goto unlock;
  }

  s_is_exit = false;
  memset(s_wdt_handle, 0,
                      CONFIG_EXTERNAL_PL_WDT_NUM * sizeof(PlWdtLibHandle));

  err_code = PlWdtLibInitialize();
  if (err_code != kPlErrCodeOk) {
    LOG_ERR(0x02, "Initialized error. err_code=%d", err_code);
    goto unlock;
  }

  uint32_t queue_size = 5;
  UtilityMsgErrCode msg_err = UtilityMsgOpen(&s_msg_handle, queue_size,
                                                            sizeof(MsgType));
  if (msg_err != kUtilityMsgOk) {
    LOG_ERR(0x03, "Message Open error. msg_err=%d", msg_err);
    err_code = kPlErrInternal;
    goto unlock;
  }

  err_code = WdtStartup();
  if (err_code != kPlErrCodeOk) {
    LOG_ERR(0x04, "OpenAndSetTimeout error. err_code=%d", err_code);
    PlWdtLibFinalize();
    goto unlock;
  }

  pthread_attr_t attr_low = {0}, attr_high = {0};
  struct sched_param low_param = {0}, high_param = {0};

  pthread_attr_init(&attr_low);
  pthread_attr_getschedparam(&attr_low, &low_param);
  low_param.sched_priority = CONFIG_EXTERNAL_PL_WDT_LOW_THREAD_PRIORITY;
  pthread_attr_setschedparam(&attr_low, &low_param);
#ifdef CONFIG_SMP
  cpu_set_t cpu_mask_low;
  CPU_ZERO(&cpu_mask_low);
  CPU_SET(0, &cpu_mask_low);
  pthread_attr_setaffinity_np(&attr_low, sizeof(cpu_set_t), &cpu_mask_low);
#endif
  ret = pthread_create(&s_thread_low, &attr_low, KeepAliveLowThread, NULL);
  if (ret) {
    LOG_ERR(0x05, "Failure to start KeepAliveLowThread. errno=%d", errno);
    err_code = kPlErrInternal;
    goto unlock;
  }
  pthread_setname_np(s_thread_low, "WDT_Low");

  pthread_attr_init(&attr_high);
  pthread_attr_getschedparam(&attr_high, &high_param);
  high_param.sched_priority = CONFIG_EXTERNAL_PL_WDT_HIGH_THREAD_PRIORITY;
  pthread_attr_setschedparam(&attr_high, &high_param);
#ifdef CONFIG_SMP
  cpu_set_t cpu_mask_high;
  CPU_ZERO(&cpu_mask_high);
  CPU_SET(1, &cpu_mask_high);
  pthread_attr_setaffinity_np(&attr_high, sizeof(cpu_set_t), &cpu_mask_high);
#endif
  ret = pthread_create(&s_thread_high, &attr_high, KeepAliveHighThread, NULL);
  if (ret) {
    LOG_ERR(0x06, "Failure to start KeepAliveHighThread. errno=%d", errno);
    err_code = kPlErrInternal;
    goto unlock;
  }
  pthread_setname_np(s_thread_high, "WDT_High");

  s_state = kWdtStateStop;

unlock:
  ret = pthread_mutex_unlock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x07, "Unlock error. errno=%d", errno);
  }

fin:
  return err_code;
}

// ----------------------------------------------------------------------------
PlErrCode PlWdtFinalize(void) {
  PlErrCode err_code = kPlErrCodeOk;
  int ret = 0;

  ret = pthread_mutex_lock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x08, "Lock error. errno=%d", errno);
    err_code = kPlErrLock;
    goto fin;
  }

  if (s_state == kWdtStateReady) {
    LOG_ERR(0x09, "Not Ready state. s_state=%d", s_state);
    err_code = kPlErrInvalidState;
    goto unlock;
  }

  if (s_state == kWdtStateStart) {
    if (PlWdtStop_() != kPlErrCodeOk) {
      LOG_INFO("PlWdtStop_() error.\n");
    }
  }

  if (s_is_registered) {
    if (PlWdtUnregisterIrqHandler_() != kPlErrCodeOk) {
      LOG_INFO("PlWdtUnregisterIrqHandler_() error.\n");
    }
  }

  for (uint32_t i = 0; i < CONFIG_EXTERNAL_PL_WDT_NUM; i++) {
    PlWdtLibClose(s_wdt_handle[i]);
  }

  err_code = PlWdtLibFinalize();

  s_is_exit = true;
  s_state = kWdtStateReady;

  pthread_join(s_thread_high, NULL);
  pthread_join(s_thread_low, NULL);

  UtilityMsgErrCode msg_err = UtilityMsgClose(s_msg_handle);
  if (msg_err != kUtilityMsgOk) {
    LOG_ERR(0x31, "UtilityMsgClose error:%u", msg_err);
    goto unlock;
  }

unlock:
  ret = pthread_mutex_unlock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x0B, "Unlock error. errno=%d", errno);
  }

fin:
  return err_code;
}

// ----------------------------------------------------------------------------
PlErrCode PlWdtStart(void) {
  PlErrCode err_code = kPlErrCodeOk;
  int ret = 0;

  ret = pthread_mutex_lock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x0C, "Lock error. errno=%d", errno);
    err_code = kPlErrLock;
    goto fin;
  }

  if (s_state != kWdtStateStop) {
    LOG_ERR(0x0D, "Not Stop state. s_state=%d", s_state);
    err_code = kPlErrInvalidState;
    goto unlock;
  }

  for (uint32_t i = 0; i < CONFIG_EXTERNAL_PL_WDT_NUM; i++) {
    err_code = PlWdtLibStart(s_wdt_handle[i]);
    if (err_code != kPlErrCodeOk) {
      LOG_ERR(0x0E, "WDT start error. s_wdt_handle[%d], err_code=%d",
              i, err_code);
      goto unlock;
    }

    // Get the current time at PlWdtLibStart as initialization
    ret = GetCurrentTime(&s_last_sent_time[i]);
    if (ret) {
      LOG_ERR(0x0F, "GetCurrentTime error. ret=%d", ret);
      err_code = kPlErrInternal;
      goto unlock;
    }
  }
  // Get the current time at PlWdtLibStart as initialization
  ret = GetCurrentTime(&s_last_recv_time);
  if (ret) {
    LOG_ERR(0x10, "GetCurrentTime error. ret=%d", ret);
    err_code = kPlErrInternal;
    goto unlock;
  }

  LOG_INFO("WDT Start.");

  s_state = kWdtStateStart;

unlock:
  ret = pthread_mutex_unlock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x11, "Unlock error. errno=%d", errno);
  }

fin:
  return err_code;
}

// ----------------------------------------------------------------------------
static PlErrCode PlWdtStop_(void) {
  PlErrCode err_code = kPlErrCodeOk;

  for (uint32_t i = 0; i < CONFIG_EXTERNAL_PL_WDT_NUM; i++) {
    err_code = PlWdtLibStop(s_wdt_handle[i]);
    if (err_code != kPlErrCodeOk) {
      LOG_ERR(0x12, "WDT Stop error. s_wdt_handle[%d], err_code=%d",
              i, err_code);
      goto fin;
    }
  }
  LOG_INFO("WDT Stop.");

  s_state = kWdtStateStop;

fin:
  return err_code;
}

// ----------------------------------------------------------------------------
PlErrCode PlWdtStop(void) {
  PlErrCode err_code = kPlErrCodeOk;
  int ret = 0;

  ret = pthread_mutex_lock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x13, "Lock error. errno=%d", errno);
    err_code = kPlErrLock;
    goto fin;
  }

  if (s_state != kWdtStateStart) {
    LOG_ERR(0x14, "Not Start state. s_state=%d", s_state);
    err_code = kPlErrInvalidState;
    goto unlock;
  }

  err_code = PlWdtStop_();

unlock:
  ret = pthread_mutex_unlock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x15, "Unlock error. errno=%d", errno);
  }

fin:
  return err_code;
}

// ----------------------------------------------------------------------------
PlErrCode PlWdtRegisterIrqHandler(PlWdtIrqHandler handler,
                                                        void *private_data) {
  PlErrCode err_code = kPlErrCodeOk;
  int ret = 0;

  ret = pthread_mutex_lock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x16, "Lock error. errno=%d", errno);
    err_code = kPlErrLock;
    goto fin;
  }

  if (s_state != kWdtStateStop) {
    LOG_ERR(0x17, "Not Stop state. s_state=%d", s_state);
    err_code = kPlErrInvalidState;
    goto unlock;
  }

  if (s_is_registered) {
    LOG_ERR(0x18, "wdt is already registered.");
    err_code = kPlErrInvalidState;
    goto unlock;
  }

  if (handler == NULL) {
    LOG_ERR(0x19, "handler is NULL.");
    err_code = kPlErrInvalidParam;
    goto unlock;
  }

  // Registered only WDT0
  err_code = PlWdtLibRegisterIrqHandler(s_wdt_handle[0],
                                                    handler, private_data);
  if (err_code != kPlErrCodeOk) {
    LOG_ERR(0x1A, "Registered error. err_code=%d", err_code);
    goto unlock;
  }

  s_is_registered = true;

unlock:
  ret = pthread_mutex_unlock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x1B, "Unlock error. errno=%d", errno);
  }

fin:
  return err_code;
}

// ----------------------------------------------------------------------------
static PlErrCode PlWdtUnregisterIrqHandler_(void) {
  PlErrCode err_code = kPlErrCodeOk;

  // Unregistered only WDT0
  err_code = PlWdtLibUnregisterIrqHandler(s_wdt_handle[0]);
  if (err_code != kPlErrCodeOk) {
    LOG_ERR(0x1C, "Unregistered error. err_code=%d", err_code);
    goto fin;
  }

  s_is_registered = false;

fin:
  return err_code;
}

// ----------------------------------------------------------------------------
PlErrCode PlWdtUnregisterIrqHandler(void) {
  PlErrCode err_code = kPlErrCodeOk;
  int ret = 0;

  ret = pthread_mutex_lock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x1D, "Lock error. errno=%d", errno);
    err_code = kPlErrLock;
    goto fin;
  }

  if (s_state != kWdtStateStop) {
    LOG_ERR(0x1E, "Not Stop state. s_state=%d", s_state);
    err_code = kPlErrInvalidState;
    goto unlock;
  }

  if (!s_is_registered) {
    LOG_ERR(0x1F, "wdt is already unregistered.");
    err_code = kPlErrInvalidState;
    goto unlock;
  }

  err_code = PlWdtUnregisterIrqHandler_();

unlock:
  ret = pthread_mutex_unlock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x20, "Unlock error. errno=%d", errno);
  }

fin:
  return err_code;
}

// ----------------------------------------------------------------------------
PlErrCode PlWdtTerminate(void) {
  PlErrCode err_code = kPlErrCodeOk;
  int ret = 0;

  ret = pthread_mutex_lock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x21, "Lock error. errno=%d", errno);
    err_code = kPlErrLock;
    goto fin;
  }

  if (s_state != kWdtStateStart) {
    LOG_ERR(0x22, "Not Start state. s_state=%d", s_state);
    err_code = kPlErrInvalidState;
    goto unlock;
  }

  MsgType send_msg = kMsgTypeTerminate;
  int32_t sent_size = sizeof(MsgType);
  int msg_pri = 2;
  UtilityMsgErrCode msg_err = UtilityMsgSend(s_msg_handle, (void*)&send_msg,
                                        sizeof(send_msg), msg_pri, &sent_size);
  if (msg_err != kUtilityMsgOk) {
    LOG_ERR(0x23, "Terminate Msg send error.");
    err_code = kPlErrInternal;
    goto unlock;
  }
  s_is_exit = true;
  LOG_INFO("WDT Thread terminated.");

unlock:
  ret = pthread_mutex_unlock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x24, "Unlock error. errno=%d", errno);
  }

fin:
  return err_code;
}

// ----------------------------------------------------------------------------
PlErrCode PlWdtKeepAlive(void) {
  PlErrCode err_code = kPlErrCodeOk;
  int ret = 0;

  ret = pthread_mutex_lock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x97, "Lock error. errno=%d", errno);
    err_code = kPlErrLock;
    goto fin;
  }

  // Get the current time at PlWdtKeepAlive
  struct timespec current_time = {0};
  ret = GetCurrentTime(&current_time);
  if (ret) {
    LOG_ERR(0x32, "GetCurrentTime error. ret=%d", ret);
    err_code = kPlErrInternal;
    goto unlock;
  }

  for (uint32_t i = 0; i < CONFIG_EXTERNAL_PL_WDT_NUM; i++) {
    err_code = PlWdtLibKeepAliveIrqContext(i);
    if (err_code != kPlErrCodeOk) {
      LOG_ERR(0x33, "WDT KeepAlive error. WDT%u, err_code=%u", i, err_code);
      goto unlock;
    }
    s_last_sent_time[i] = current_time;
  }
  // timeout sec reset.
  s_last_recv_time = current_time;
  s_is_wdt0_timeover = false;
  s_is_wdt1_timeover = false;

unlock:
  ret = pthread_mutex_unlock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x34, "Unlock error. errno=%d", errno);
  }

fin:
  return err_code;
}

// ----------------------------------------------------------------------------
static void *KeepAliveLowThread(void *arg) {
  (void)arg;  // Avoid compiler warning
  MsgType send_msg = kMsgTypeKeepAlive;
  int32_t sent_size = sizeof(MsgType);
  int msg_pri = 1;

  LOG_INFO("WDT Low Thread Start.");
  while (!s_is_exit) {
    if (s_state == kWdtStateStart) {
      UtilityMsgErrCode msg_err = UtilityMsgSend(s_msg_handle, (void*)&send_msg,
                                        sizeof(send_msg), msg_pri, &sent_size);
      if (msg_err != kUtilityMsgOk) {
        LOG_ERR(0x25, "Keepalive instruction send error. msg_err=%d", msg_err);
      }
    }
    usleep(300 * 1000);
  }
  LOG_INFO("WDT Low Thread End.");

  return 0;
}

// ----------------------------------------------------------------------------
static void *KeepAliveHighThread(void *arg) {
  (void)arg;  // Avoid compiler warning
  MsgType rcvdata = 0;
  int32_t recv_size = 0;
  struct timespec current_time = {0};
  int ret = 0;
  const int msg_timeout_ms = 500;
  long sub_sec = 0;  //NOLINT
  bool is_low_thread_stoped = false;
  s_is_wdt0_timeover = false;
  s_is_wdt1_timeover = false;

  LOG_INFO("WDT High Thread Start.");
  PlErrCode err_code = kPlErrCodeOk;
  while (!s_is_exit) {
    if (s_state != kWdtStateStart) {
      usleep(500 * 1000);
      continue;
    }
    UtilityMsgErrCode msg_err = UtilityMsgRecv(s_msg_handle, (void *)&rcvdata,
                                sizeof(MsgType), msg_timeout_ms, &recv_size);
    ret = GetCurrentTime(&current_time);
    if (ret != 0) {
      LOG_ERR(0x26, "GetCurrentTime error. ret=%d", ret);
      continue;
    }

    if (msg_err == kUtilityMsgOk) {
      // Outputs log only when WDT Low Thread restarts.
      if (is_low_thread_stoped) {
        LOG_INFO("WDT Low Thread Restart.");
        is_low_thread_stoped = false;
      }

      switch (rcvdata) {
        case kMsgTypeKeepAlive:
          s_is_wdt0_timeover = false;
          s_is_wdt1_timeover = false;
          s_last_recv_time = current_time;
          break;
        case kMsgTypeTerminate:
          goto end;
        default:
          LOG_ERR(0x27, "MsgType Error. rcvdata=%d", rcvdata);
          continue;
      }
    } else if (msg_err == kUtilityMsgErrTimedout) {
      // Outputs log only when MsgRecv Timeout error for the first time.
      if (!is_low_thread_stoped) {
        LOG_INFO("WDT Low Thread stopped working.");
        s_last_recv_time = current_time;
        is_low_thread_stoped = true;
      }

      sub_sec = CalcSubSec(current_time, s_last_recv_time);
      if (sub_sec >= CONFIG_EXTERNAL_PL_WDT0_TIMEOUT_SEC - 60) {
        // KeepAlive only when WDT0 timeout for the first time.
        if (!s_is_wdt0_timeover) {
          err_code = PlWdtLibKeepAlive(s_wdt_handle[0]);
          if (err_code == kPlErrCodeOk) {
            s_last_sent_time[0] = current_time;
          } else {
            LOG_ERR(0x28, "Sent Keepalive error. err_code=%u", err_code);
          }
        }
        s_is_wdt0_timeover = true;
      }
      if (sub_sec >= CONFIG_EXTERNAL_PL_WDT1_TIMEOUT_SEC - 60) {
        // KeepAlive only when WDT1 timeout for the first time.
        if (!s_is_wdt1_timeover) {
          err_code = PlWdtLibKeepAlive(s_wdt_handle[1]);
          if (err_code == kPlErrCodeOk) {
            s_last_sent_time[1] = current_time;
          } else {
            LOG_ERR(0x29, "Sent Keepalive error. err_code=%u", err_code);
          }
        }
        s_is_wdt1_timeover = true;
      }
    } else {
      LOG_ERR(0x2A, "msg_err=%d", msg_err);
      continue;
    }

    sub_sec = CalcSubSec(current_time, s_last_sent_time[0]);
    if ((sub_sec >= CONFIG_EXTERNAL_PL_WDT_KEEP_ALIVE_SEC) &&
                                              (s_is_wdt0_timeover == false)) {
      err_code = PlWdtLibKeepAlive(s_wdt_handle[0]);
      if (err_code == kPlErrCodeOk) {
        s_last_sent_time[0] = current_time;
      } else {
        LOG_ERR(0x2B, "Sent Keepalive error. err_code=%d", err_code);
      }
    }
    sub_sec = CalcSubSec(current_time, s_last_sent_time[1]);
    if ((sub_sec >= CONFIG_EXTERNAL_PL_WDT_KEEP_ALIVE_SEC) &&
                                              (s_is_wdt1_timeover == false)) {
      err_code = PlWdtLibKeepAlive(s_wdt_handle[1]);
      if (err_code == kPlErrCodeOk) {
        s_last_sent_time[1] = current_time;
      } else {
        LOG_ERR(0x2C, "Sent Keepalive error. err_code=%d", err_code);
      }
    }
  }

end:
  LOG_INFO("WDT High Thread End.");

  return 0;
}

// ----------------------------------------------------------------------------
static PlErrCode WdtStartup(void) {
  PlErrCode err_code = kPlErrCodeOk;
  for (uint32_t i = 0; i < CONFIG_EXTERNAL_PL_WDT_NUM; i++) {
    err_code = PlWdtLibOpen(&s_wdt_handle[i], i);
    if (err_code != kPlErrCodeOk) {
      LOG_ERR(0x2D, "WDT Open error. s_wdt_handle[%d], err_code=%d",
              i, err_code);
      goto free;
    }
  }
  err_code = PlWdtLibSetTimeout(s_wdt_handle[0],
                                  CONFIG_EXTERNAL_PL_WDT0_TIMEOUT_SEC);
  if (err_code != kPlErrCodeOk) {
    LOG_ERR(0x2E, "Set Timeout error. err_code=%d", err_code);
    goto free;
  }
  err_code = PlWdtLibSetTimeout(s_wdt_handle[1],
                                  CONFIG_EXTERNAL_PL_WDT1_TIMEOUT_SEC);
  if (err_code != kPlErrCodeOk) {
    LOG_ERR(0x2F, "Set Timeout error. err_code=%d", err_code);
    goto free;
  }
  return err_code;

free:
  for (uint32_t i = 0; i < CONFIG_EXTERNAL_PL_WDT_NUM; i++) {
    PlWdtLibClose(&s_wdt_handle[i]);
  }
  return err_code;
}

// ----------------------------------------------------------------------------
static int GetCurrentTime(struct timespec *time) {
  int ret = clock_gettime(CLOCK_MONOTONIC, time);
  if (ret != 0) {
    LOG_ERR(0x30, "GetCurrentTime error. ret=%d", ret);
  }
  return ret;
}

// ----------------------------------------------------------------------------
static long CalcSubSec(struct timespec current_time,  //NOLINT
                                              struct timespec old_time) {
  long time_sec = current_time.tv_sec - old_time.tv_sec;  //NOLINT
  time_sec += (current_time.tv_nsec - old_time.tv_nsec) / 1000000000;
  return time_sec;
}

// ----------------------------------------------------------------------------
