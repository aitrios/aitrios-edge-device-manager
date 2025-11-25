/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#include "pl_led.h"
#include "pl_led_hw.h"
#include "utility_timer.h"
#include "utility_log.h"
#include "utility_log_module_id.h"

// Macros ---------------------------------------------------------------------
#define EVENT_ID        (0x9400)
#define EVENT_ID_START  (0x00)
#define LOG_E(event_id, format, ...) \
  WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:" \
                   format, __FILE__, __LINE__, ##__VA_ARGS__); \
  WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (EVENT_ID | (EVENT_ID_START + event_id)));

#define LOG_W(event_id, format, ...) \
  WRITE_DLOG_WARN(MODULE_ID_SYSTEM, "%s-%d:" \
                  format, __FILE__, __LINE__, ##__VA_ARGS__); \
  WRITE_ELOG_WARN(MODULE_ID_SYSTEM, (EVENT_ID | (EVENT_ID_START + event_id)));

#define LOG_D(format, ...) \
  WRITE_DLOG_DEBUG(MODULE_ID_SYSTEM, "%s-%d:" \
                   format, __FILE__, __LINE__, ##__VA_ARGS__);

#define LOG_T(format, ...) \
  WRITE_DLOG_TRACE(MODULE_ID_SYSTEM, "%s-%d:" \
                   format, __FILE__, __LINE__, ##__VA_ARGS__);

#define MAX(a, b) (a < b) ? b : a
#define MIN(a, b) (a < b) ? a : b
// typedef --------------------------------------------------------------------
typedef enum {
  kLedStateStop,
  kLedStateStart,
  kLedStateStartSync,
  kLedStateStartSeq,
} LedState;

struct LedCtrlInfo {
  LedState       state;
  int64_t        elapsed_ns;
  uint32_t       seq_no;
  uint32_t       seq_len;
  bool           waiting_interval;
  PlLedSequence *seq;
};
// Local Variables ------------------------------------------------------------
static bool                   s_is_initialized = false;
static pthread_mutex_t        s_mutex_api = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t        s_mutex_ctrlinfo = PTHREAD_MUTEX_INITIALIZER;
static PlLedInfo             *s_info = NULL;
static struct LedCtrlInfo     s_ctrl_info[CONFIG_PL_LED_LEDS_NUM];
static int64_t                s_interval_nsec_min = 0;
static int64_t                s_interval_nsec_max = 0;
static UtilityTimerHandle     s_timer_handle = 0;
// On or Off Minimum Unit time=50ms
static const int kIntervalNs = 50 * 1000 * 1000;
static const struct timespec  kInterval = {0, kIntervalNs};

// Local functions ------------------------------------------------------------
static void      TimerCallback(void *arg);
static PlErrCode LedInfoInit(void);
static void      LedInfoDeinit(void);
static PlErrCode LedCtrlInfoInit(void);
static bool      IsValidStartParam(const PlLedStartParamEx *param,
                                    PlErrCode *ret);
static bool      IsValidStartSyncParam(const PlLedStartParamEx *param,
                                     uint32_t param_len,
                                     PlErrCode *ret);
static bool IsValidStartSeqParam(uint32_t led_id,
                                 const PlLedSequence *seq,
                                 uint32_t seq_len,
                                 PlErrCode *ret);
static bool      IsValidStopParam(uint32_t led_id, PlErrCode *ret);
static bool      IsValidStopSyncParam(uint32_t *led_ids,
                                      uint32_t len, PlErrCode *ret);
static bool      IsValidTimespec(const struct timespec *interval_on,
                                 const struct timespec *interval_off);
static int64_t   CalcNsec(const struct timespec *time);
static PlErrCode RegisterSeq(uint32_t led_id,
                             const PlLedSequence *seq,
                             uint32_t seq_len);
static PlErrCode RegisterOnSeq(uint32_t led_id, uint32_t color_id);
static PlErrCode RegisterOnOffSeq(uint32_t led_id,
                                  uint32_t color_id,
                                  const struct timespec *interval_on,
                                  const struct timespec *interval_off);
static void UnregisterSeq(uint32_t led_id);
static void ExecuteSeq(struct LedCtrlInfo *ctrl_info,
                        uint32_t led_id, int64_t elapsed_time_ns);
// Functions ------------------------------------------------------------------
PlErrCode PlLedStart(const PlLedStartParam *param) {
  if (param == NULL) {
    LOG_E(0x00, "param is NULL.");
    return kPlErrInvalidParam;
  }
  PlLedStartParamEx ex_param = {0};
  ex_param.led_id       = param->led_id;
  ex_param.color_id     = param->color_id;
  ex_param.interval_on  = param->interval;
  ex_param.interval_off = param->interval;
  return PlLedStartEx(&ex_param);
}
// -----------------------------------------------------------------------------
PlErrCode PlLedStartEx(const PlLedStartParamEx *param) {
  int ret_os = pthread_mutex_lock(&s_mutex_api);
  if (ret_os) {
    LOG_E(0x01, "pthread_mutex_lock() fail:%d", ret_os);
    return kPlErrLock;
  }
  PlErrCode ret = kPlErrCodeOk;
  if (!s_is_initialized) {
    LOG_E(0x02, "Not initialized.");
    ret = kPlErrInvalidState;
    goto unlock_api;
  }
  ret_os = pthread_mutex_lock(&s_mutex_ctrlinfo);
  if (ret_os) {
    LOG_E(0x03, "pthread_mutex_lock() fail:%d", ret_os);
    ret = kPlErrLock;
    goto unlock_api;
  }
  if (!IsValidStartParam(param, &ret)) {
    LOG_E(0x04, "IsValidStartParam() fail");
    goto unlock_ctrlinfo;
  }
  int64_t interval_on  = CalcNsec(&param->interval_on);
  int64_t interval_off = CalcNsec(&param->interval_off);
  if ((interval_on == 0) || (interval_off == 0)) {
    ret = RegisterOnSeq(param->led_id, param->color_id);
  } else {
    ret = RegisterOnOffSeq(param->led_id, param->color_id,
                          &param->interval_on, &param->interval_off);
  }
  if (ret != kPlErrCodeOk) {
    LOG_E(0x05, "Failed to RegisterSeq. ret:%u, led_id:%u color_id:%u",
                  ret, param->led_id, param->color_id);
    goto unlock_ctrlinfo;
  }
  s_ctrl_info[param->led_id].state = kLedStateStart;

unlock_ctrlinfo:
  ret_os = pthread_mutex_unlock(&s_mutex_ctrlinfo);
  if (ret_os) {
    LOG_E(0x06, "pthread_mutex_unlock() fail:%d", ret_os);
    goto unlock_api;
  }
unlock_api:
  ret_os = pthread_mutex_unlock(&s_mutex_api);
  if (ret_os) {
    LOG_E(0x07, "pthread_mutex_unlock() fail:%d", ret_os);
  }
  LOG_T("[OUT] ret:%u", ret);
  return ret;
}
// -----------------------------------------------------------------------------
PlErrCode PlLedStop(uint32_t led_id) {
  LOG_T("[IN] led_id:%u", led_id);
  int ret_os = pthread_mutex_lock(&s_mutex_api);
  if (ret_os) {
    LOG_E(0x08, "pthread_mutex_lock() fail:%d", ret_os);
    return kPlErrLock;
  }
  PlErrCode ret = kPlErrCodeOk;
  if (!s_is_initialized) {
    LOG_E(0x09, "Not initialized.");
    ret = kPlErrInvalidState;
    goto unlock_api;
  }
  ret_os = pthread_mutex_lock(&s_mutex_ctrlinfo);
  if (ret_os) {
    LOG_E(0x0A, "pthread_mutex_lock() fail:%d", ret_os);
    ret = kPlErrLock;
    goto unlock_api;
  }

  if (!IsValidStopParam(led_id, &ret)) {
    goto unlock_ctrlinfo;
  }
  UnregisterSeq(led_id);
  // Force the LED off.
  ret = PlLedHwOff(led_id);
  if (ret != kPlErrCodeOk) {
    LOG_E(0x0B, "PlLedHwOff() fail:%u, led_id:%u", ret, led_id);
    goto unlock_ctrlinfo;
  }
  s_ctrl_info[led_id].state = kLedStateStop;

unlock_ctrlinfo:
  ret_os = pthread_mutex_unlock(&s_mutex_ctrlinfo);
  if (ret_os) {
    LOG_W(0x0C, "pthread_mutex_unlock() fail:%d", ret_os);
    goto unlock_api;
  }
unlock_api:
  ret_os = pthread_mutex_unlock(&s_mutex_api);
  if (ret_os) {
    LOG_E(0x0D, "pthread_mutex_unlock() fail:%d", ret_os);
  }
  LOG_T("[OUT] ret:%u", ret);
  return ret;
}
// -----------------------------------------------------------------------------
PlErrCode PlLedStartSync(const PlLedStartParam *param, uint32_t param_len) {
  if (param == NULL) {
    LOG_E(0x0E, "param is NULL.");
    return kPlErrInvalidParam;
  }
  if (param_len == 0) {
    LOG_E(0x0F, "param_len is 0.");
    return kPlErrInvalidParam;
  }
  PlLedStartParamEx *param_ex = calloc(param_len, sizeof(PlLedStartParamEx));
  if (param_ex == NULL) {
    LOG_E(0x10, "Failed to calloc(%u, %lu)",
          param_len, sizeof(PlLedStartParamEx));
    return kPlErrMemory;
  }
  for (uint32_t i = 0; i < param_len; i++) {
    param_ex[i].interval_on  = param[i].interval;
    param_ex[i].interval_off = param[i].interval;
    param_ex[i].led_id       = param[i].led_id;
    param_ex[i].color_id     = param[i].color_id;
  }
  PlErrCode ret = PlLedStartSyncEx(param_ex, param_len);
  free(param_ex);
  return ret;
}
// -----------------------------------------------------------------------------
PlErrCode PlLedStartSyncEx(const PlLedStartParamEx *param, uint32_t param_len) {
  int ret_os = pthread_mutex_lock(&s_mutex_api);
  if (ret_os) {
    LOG_E(0x11, "pthread_mutex_lock() fail:%d", ret_os);
    return kPlErrLock;
  }
  PlErrCode ret = kPlErrCodeOk;
  if (!s_is_initialized) {
    LOG_E(0x12, "Not initialized.");
    ret = kPlErrInvalidState;
    goto unlock_api;
  }
  ret_os = pthread_mutex_lock(&s_mutex_ctrlinfo);
  if (ret_os) {
    LOG_E(0x13, "pthread_mutex_lock() fail:%d", ret_os);
    ret = kPlErrLock;
    goto unlock_api;
  }
  if (!IsValidStartSyncParam(param, param_len, &ret)) {
    LOG_E(0x14, "IsValidStartSyncParam() fail. len:%u", param_len);
    goto unlock_ctrlinfo;
  }

  for (uint32_t i = 0; i < param_len; i++) {
    uint32_t led_id = param[i].led_id;
    uint32_t color_id = param[i].color_id;
    int64_t interval_on  = CalcNsec(&param[i].interval_off);
    int64_t interval_off = CalcNsec(&param[i].interval_off);
    if ((interval_on == 0) || (interval_off == 0)) {
      ret = RegisterOnSeq(led_id, color_id);
    } else {
      ret = RegisterOnOffSeq(led_id, color_id,
                            &param[i].interval_on, &param[i].interval_off);
    }
    if (ret != kPlErrCodeOk) {
      LOG_E(0x15, "Failed to RegisterSeq. ret:%u, led_id:%u color_id:%u",
                    ret, led_id, color_id);
      goto unregister_seq;
    }
    s_ctrl_info[led_id].state = kLedStateStartSync;
  }
  // Success case
  goto unlock_ctrlinfo;

unregister_seq:
  for (uint32_t i = 0; i < param_len; i++) {
    UnregisterSeq(param[i].led_id);
    s_ctrl_info[param[i].led_id].state = kLedStateStop;
  }
unlock_ctrlinfo:
  ret_os = pthread_mutex_unlock(&s_mutex_ctrlinfo);
  if (ret_os) {
    LOG_E(0x16, "pthread_mutex_unlock() fail:%d", ret_os);
    goto unlock_api;
  }
unlock_api:
  ret_os = pthread_mutex_unlock(&s_mutex_api);
  if (ret_os) {
    LOG_E(0x17, "pthread_mutex_unlock() fail:%d", ret_os);
  }
  LOG_T("[OUT] ret:%u", ret);
  return ret;
}
// -----------------------------------------------------------------------------
PlErrCode PlLedStopSync(uint32_t *led_ids, uint32_t len) {
  PlErrCode ret = kPlErrCodeOk;
  int ret_os = pthread_mutex_lock(&s_mutex_api);
  if (ret_os) {
    LOG_E(0x18, "pthread_mutex_lock() fail:%d", ret_os);
    return kPlErrLock;
  }
  if (!s_is_initialized) {
    LOG_E(0x19, "Not initialized.");
    ret = kPlErrInvalidState;
    goto unlock_api;
  }
  ret_os = pthread_mutex_lock(&s_mutex_ctrlinfo);
  if (ret_os) {
    LOG_E(0x1A, "pthread_mutex_lock() fail:%d", ret_os);
    ret = kPlErrLock;
    goto unlock_api;
  }
  if (!IsValidStopSyncParam(led_ids, len, &ret)) {
    LOG_E(0x1B, "IsValidStopSyncParam() fail. len:%u", len);
    goto unlock_ctrlinfo;
  }

  for (uint32_t i = 0; i < len; i++) {
    uint32_t led_id = led_ids[i];
    // Force the LED off.
    ret = PlLedHwOff(led_id);
    if (ret != kPlErrCodeOk) {
      LOG_E(0x1C, "PlLedHwOff() fail:%u, led_id:%u", ret, led_id);
      goto unlock_ctrlinfo;
    }
    s_ctrl_info[led_id].state = kLedStateStop;
  }

unlock_ctrlinfo:
  ret_os = pthread_mutex_unlock(&s_mutex_ctrlinfo);
  if (ret_os) {
    LOG_E(0x1D, "pthread_mutex_unlock() fail:%d", ret_os);
  }
unlock_api:
  ret_os = pthread_mutex_unlock(&s_mutex_api);
  if (ret_os) {
    LOG_E(0x1E, "pthread_mutex_unlock() fail:%d", ret_os);
  }
  LOG_T("[OUT] ret:%u", ret);
  return ret;
}
// -----------------------------------------------------------------------------
PlErrCode PlLedStartSeq(uint32_t led_id,
                        const PlLedSequence *seq,
                        uint32_t seq_len) {
  int ret_os = pthread_mutex_lock(&s_mutex_api);
  if (ret_os) {
    LOG_E(0x1F, "pthread_mutex_lock() fail:%d", ret_os);
    return kPlErrLock;
  }
  PlErrCode ret = kPlErrCodeOk;
  if (!s_is_initialized) {
    LOG_E(0x20, "Not initialized.");
    ret = kPlErrInvalidState;
    goto unlock_api;
  }
  ret_os = pthread_mutex_lock(&s_mutex_ctrlinfo);
  if (ret_os) {
    LOG_E(0x21, "pthread_mutex_lock() fail:%d", ret_os);
    ret = kPlErrLock;
    goto unlock_api;
  }
  if (!IsValidStartSeqParam(led_id, seq, seq_len, &ret)) {
    LOG_E(0x22, "Failed to IsValidStartSeqParam. led_id:%u len:%u",
                                                led_id, seq_len);
    goto unlock_ctrlinfo;
  }
  ret = RegisterSeq(led_id, seq, seq_len);
  if (ret != kPlErrCodeOk) {
    LOG_E(0x23, "Failed to RegisterSeq. ret:%u led_id:%u seq:%p len:%u",
                  ret, led_id, seq, seq_len);
    goto unlock_ctrlinfo;
  }
  s_ctrl_info[led_id].state = kLedStateStartSeq;

unlock_ctrlinfo:
  ret_os = pthread_mutex_unlock(&s_mutex_ctrlinfo);
  if (ret_os) {
    LOG_E(0x24, "pthread_mutex_unlock() fail:%d", ret_os);
    goto unlock_api;
  }
unlock_api:
  ret_os = pthread_mutex_unlock(&s_mutex_api);
  if (ret_os) {
    LOG_E(0x25, "pthread_mutex_unlock() fail:%d", ret_os);
  }
  LOG_T("[OUT] ret:%u", ret);
  return ret;
}
// -----------------------------------------------------------------------------
PlErrCode PlLedStopSeq(uint32_t led_id) {
  int ret_os = pthread_mutex_lock(&s_mutex_api);
  if (ret_os) {
    LOG_E(0x26, "pthread_mutex_lock() fail:%d", ret_os);
    return kPlErrLock;
  }
  PlErrCode ret = kPlErrCodeOk;
  if (!s_is_initialized) {
    LOG_E(0x27, "Not initialized.");
    ret = kPlErrInvalidState;
    goto unlock_api;
  }
  ret_os = pthread_mutex_lock(&s_mutex_ctrlinfo);
  if (ret_os) {
    LOG_E(0x28, "pthread_mutex_lock() fail:%d", ret_os);
    ret = kPlErrLock;
    goto unlock_api;
  }
  if (CONFIG_PL_LED_LEDS_NUM <= led_id) {
    LOG_E(0x29, "Invalid led id.%u", led_id);
    ret = kPlErrInvalidParam;
    goto unlock_ctrlinfo;
  }
  if (s_ctrl_info[led_id].state != kLedStateStartSeq) {
    LOG_E(0x2A, "led:%u invalid state:%u", led_id, s_ctrl_info[led_id].state);
    ret = kPlErrInvalidState;
    goto unlock_ctrlinfo;
  }
  UnregisterSeq(led_id);
  // Force the LED off.
  ret = PlLedHwOff(led_id);
  if (ret != kPlErrCodeOk) {
    LOG_E(0x2B, "PlLedHwOff() fail:%u, led_id:%u", ret, led_id);
    goto unlock_ctrlinfo;
  }
  s_ctrl_info[led_id].state = kLedStateStop;

unlock_ctrlinfo:
  ret_os = pthread_mutex_unlock(&s_mutex_ctrlinfo);
  if (ret_os) {
    LOG_E(0x2C, "pthread_mutex_unlock() fail:%d", ret_os);
    goto unlock_api;
  }
unlock_api:
  ret_os = pthread_mutex_unlock(&s_mutex_api);
  if (ret_os) {
    LOG_E(0x2D, "pthread_mutex_unlock() fail:%d", ret_os);
  }
  LOG_T("[OUT] ret:%u", ret);
  return ret;
}
// -----------------------------------------------------------------------------
PlErrCode PlLedGetInfo(PlLedInfo *dev_info) {
  LOG_T("[IN]");
  int ret_os = pthread_mutex_lock(&s_mutex_api);
  if (ret_os) {
    LOG_E(0x2E, "pthread_mutex_lock() fail:%d", ret_os);
    return kPlErrLock;
  }

  PlErrCode ret = kPlErrCodeOk;
  if (!s_is_initialized) {
    LOG_E(0x2F, "Not initialized.");
    ret = kPlErrInvalidState;
    goto unlock;
  }
  if (dev_info == NULL) {
    LOG_E(0x30, "dev_info is NULL.");
    ret = kPlErrInvalidParam;
    goto unlock;
  }
  memcpy(dev_info, s_info, sizeof(PlLedInfo));

unlock:
  ret_os = pthread_mutex_unlock(&s_mutex_api);
  if (ret_os) {
    LOG_E(0x31, "pthread_mutex_unlock() fail:%d", ret_os);
  }
  LOG_T("[OUT] ret=%u", ret);
  return ret;
}
// -----------------------------------------------------------------------------
PlErrCode PlLedInitialize(void) {
  LOG_T("[IN]");
  int ret_os = pthread_mutex_lock(&s_mutex_api);
  if (ret_os) {
    LOG_E(0x32, "pthread_mutex_lock() fail:%d", ret_os);
    return kPlErrLock;
  }

  PlErrCode ret_hw = kPlErrCodeOk;
  PlErrCode ret = kPlErrCodeOk;
  if (s_is_initialized) {
    LOG_E(0x33, "Already initialized.");
    ret = kPlErrInvalidState;
    goto unlock_api;
  }
  ret = PlLedHwInitialize();
  if (ret != kPlErrCodeOk) {
    LOG_E(0x34, "PlLedHwInitialize fail:%u", ret);
    goto unlock_api;
  }

  UtilityTimerErrCode ret_util = kUtilityTimerOk;
  ret = LedInfoInit();
  if (ret != kPlErrCodeOk) {
    LOG_E(0x35, "LedInfoInit fail:%u", ret);
    goto exit_ledhw_finalize;
  }
  ret = LedCtrlInfoInit();
  if (ret != kPlErrCodeOk) {
    LOG_E(0x36, "LedCtrlInfoInit fail:%u", ret);
    goto exit_info_deinit;
  }

  ret_util = UtilityTimerCreate(TimerCallback, NULL, &s_timer_handle);
  if (ret_util != kUtilityTimerOk) {
    LOG_E(0x37, "UtilityTimerCreate() fail:%u", ret_util);
    goto exit_info_deinit;
  }
  ret_util = UtilityTimerStart(s_timer_handle, &kInterval, kUtilityTimerRepeat);
  if (ret_util != kUtilityTimerOk) {
    LOG_E(0x38, "UtilityTimerStart() fail:%u", ret_util);
    UtilityTimerErrCode ret_util2 = UtilityTimerDelete(s_timer_handle);
    if (ret_util2 != kUtilityTimerOk) {
      LOG_E(0x39, "UtilityTimerDelete() fail:%u", ret_util2);
      // Continue with the termination process.
    }
    s_timer_handle = NULL;
    ret = kPlErrCodeError;
    goto exit_info_deinit;
  }
  s_is_initialized = true;
  goto unlock_api;

exit_info_deinit:
  LedInfoDeinit();
exit_ledhw_finalize:
  ret_hw = PlLedHwFinalize();
  if (ret_hw != kPlErrCodeOk) {
    LOG_E(0x3A, "PlLedHwFinalize fail:%u", ret_hw);
  }
  if (ret_util != kUtilityTimerOk) {
    ret = kPlErrCodeError;
  }
unlock_api:
  ret_os = pthread_mutex_unlock(&s_mutex_api);
  if (ret_os) {
    LOG_E(0x3B, "pthread_mutex_unlock() fail:%d", ret_os);
  }
  LOG_T("[OUT] ret=%u", ret);
  return ret;
}
// -----------------------------------------------------------------------------
PlErrCode PlLedFinalize(void) {
  LOG_T("[IN]");
  int ret_os = pthread_mutex_lock(&s_mutex_api);
  if (ret_os) {
    LOG_E(0x3C, "pthread_mutex_lock() fail:%d", ret_os);
    return kPlErrLock;
  }
  PlErrCode ret = kPlErrCodeOk;
  if (!s_is_initialized) {
    LOG_E(0x3D, "Not initialized.");
    ret = kPlErrInvalidState;
    goto unlock_api;
  }

  UtilityTimerErrCode ret_util = kUtilityTimerOk;
  ret_util = UtilityTimerStop(s_timer_handle);
  if (ret_util != kUtilityTimerOk) {
    LOG_E(0x3E, "UtilityTimerStop() fail:%u", ret_util);
    // Continue with the termination process.
  }

  if (s_timer_handle) {
    ret_util = UtilityTimerDelete(s_timer_handle);
    if (ret_util != kUtilityTimerOk) {
      LOG_E(0x3F, "UtilityTimerDelete() fail:%u", ret_util);
      // Continue with the termination process.
    }
    s_timer_handle = NULL;
  }

  ret_os = pthread_mutex_lock(&s_mutex_ctrlinfo);
  if (ret_os) {
    LOG_E(0x40, "pthread_mutex_lock() fail:%d", ret_os);
    ret = kPlErrLock;
    goto unlock_api;
  }
  for (uint32_t led_id = 0; led_id < CONFIG_PL_LED_LEDS_NUM; led_id++) {
    if (s_ctrl_info[led_id].state != kLedStateStop) {
      ret = PlLedHwOff(led_id);
      if (ret != kPlErrCodeOk) {
        LOG_E(0x41, "PlLedHwOff() fail:%u, led_id:%u", ret, led_id);
        // Continue with the termination process.
      }
      s_ctrl_info[led_id].state = kLedStateStop;
    }
    if (s_ctrl_info[led_id].seq) {
      free(s_ctrl_info[led_id].seq);
      s_ctrl_info[led_id].seq = NULL;
    }
  }
  ret_os = pthread_mutex_unlock(&s_mutex_ctrlinfo);
  if (ret_os) {
    LOG_E(0x42, "pthread_mutex_unlock() fail:%d", ret_os);
    // Continue with the termination process.
  }

  LedInfoDeinit();
  ret = PlLedHwFinalize();
  if (ret != kPlErrCodeOk) {
    LOG_E(0x43, "PlLedHwFinalize fail:%u", ret);
    // Continue with the termination process.
  }
  s_is_initialized = false;

unlock_api:
  ret_os = pthread_mutex_unlock(&s_mutex_api);
  if (ret_os) {
    LOG_E(0x44, "pthread_mutex_unlock() fail:%d", ret_os);
  }
  LOG_T("[OUT] ret=%u", ret);
  return ret;
}
// -----------------------------------------------------------------------------
static void TimerCallback(void *arg) {
  (void)arg;  // Avoid compile error
  int ret_os = pthread_mutex_lock(&s_mutex_ctrlinfo);
  if (ret_os) {
    LOG_E(0x45, "pthread_mutex_lock() fail:%d", ret_os);
    return;
  }
  if (s_timer_handle == NULL) {
    LOG_E(0x46, "timer_handle is NULL.");
    goto unlock;
  }

  for (uint32_t led_id = 0; led_id < CONFIG_PL_LED_LEDS_NUM; led_id++) {
     struct LedCtrlInfo *ctrl_info = &s_ctrl_info[led_id];
    if ((ctrl_info->state == kLedStateStop) ||
        (ctrl_info->seq == NULL)) {
      continue;
    }
    ExecuteSeq(ctrl_info, led_id, kIntervalNs);
  }

unlock:
  ret_os = pthread_mutex_unlock(&s_mutex_ctrlinfo);
  if (ret_os) {
    LOG_E(0x47, "pthread_mutex_unlock() fail:%d", ret_os);
  }
  return;
}
// -----------------------------------------------------------------------------
static PlErrCode LedInfoInit(void) {
  UtilityTimerSystemInfo timer_sysinfo = {0};
  UtilityTimerErrCode ret_util = UtilityTimerGetSystemInfo(&timer_sysinfo);
  if (ret_util != kUtilityTimerOk) {
    LOG_E(0x48, "UtilityTimerGetSystemInfo() fail:%u", ret_util);
    return kPlErrCodeError;
  }

  s_interval_nsec_min = kIntervalNs;
  s_interval_nsec_max = CalcNsec(&timer_sysinfo.interval_max_ts);
  LOG_D("interval_nsec_min=%lld - interval_nsec_max=%lld",
        (int64_t)s_interval_nsec_min, (int64_t)s_interval_nsec_max);

  PlLedHwInfo  led_hw_info = {0};
  PlErrCode ret = PlLedHwGetInfo(&led_hw_info);
  if (ret != kPlErrCodeOk) {
    LOG_E(0x49, "PlLedHwGetInfo ret=%u", ret);
    return ret;
  }

  s_info = calloc(1, sizeof(PlLedInfo));
  if (s_info == NULL) {
    LOG_E(0x4A, "calloc fail");
    return kPlErrMemory;
  }

  memcpy(s_info->leds, led_hw_info.leds,
          sizeof(PlLedLedsInfo) * CONFIG_PL_LED_LEDS_NUM);
  s_info->leds_num = led_hw_info.leds_num;
  s_info->interval_ts_min = kInterval;
  s_info->interval_ts_max = timer_sysinfo.interval_max_ts;
  s_info->interval_resolution_ms = kIntervalNs / 1000 / 1000;  // ms
  return kPlErrCodeOk;
}
// -----------------------------------------------------------------------------
static void LedInfoDeinit(void) {
  if (s_info != NULL) {
    free(s_info);
    s_info = NULL;
  }
  return;
}
// -----------------------------------------------------------------------------
static PlErrCode LedCtrlInfoInit(void) {
  int ret_os = pthread_mutex_lock(&s_mutex_ctrlinfo);
  if (ret_os) {
    LOG_E(0x4B, "pthread_mutex_lock() fail:%d", ret_os);
    return kPlErrLock;
  }
  for (uint32_t led_id = 0; led_id < CONFIG_PL_LED_LEDS_NUM; led_id++) {
    s_ctrl_info[led_id].state           = kLedStateStop;
    s_ctrl_info[led_id].seq             = NULL;
    s_ctrl_info[led_id].seq_no          = 0;
    s_ctrl_info[led_id].seq_len         = 0;
    s_ctrl_info[led_id].elapsed_ns      = 0;
    s_ctrl_info[led_id].waiting_interval = false;
  }

  ret_os = pthread_mutex_unlock(&s_mutex_ctrlinfo);
  if (ret_os) {
    LOG_E(0x4C, "pthread_mutex_unlock() fail:%d", ret_os);
  }
  return kPlErrCodeOk;
}
// -----------------------------------------------------------------------------
static bool IsValidStartParam(const PlLedStartParamEx *param, PlErrCode *ret) {
  // Parameter ret is guaranteed to be not NULL.
  if (param == NULL) {
    LOG_E(0x4D, "param is NULL.");
    *ret = kPlErrInvalidParam;
    return false;
  }
  uint32_t led_id = param->led_id;
  if ((param->color_id != CONFIG_PL_LED_COLOR0_ID) &&
      (param->color_id != CONFIG_PL_LED_COLOR1_ID) &&
      (param->color_id != CONFIG_PL_LED_COLOR2_ID) &&
      (param->color_id != CONFIG_PL_LED_COLOR_OFF_ID)) {
    LOG_E(0x4E, "Invalid color_id:%u", param->color_id);
    *ret = kPlErrInvalidParam;
    return false;
  }
  if (CONFIG_PL_LED_LEDS_NUM <= led_id) {
    LOG_E(0x4F, "Invalid led_id:%u", led_id);
    *ret = kPlErrInvalidParam;
    return false;
  }
  if ((s_ctrl_info[led_id].state != kLedStateStop) &&
      (s_ctrl_info[led_id].state != kLedStateStart)) {
    LOG_E(0x50, "led:%u invalid state:%u", led_id, s_ctrl_info[led_id].state);
    *ret = kPlErrInvalidState;
    return false;
  }
  if (!IsValidTimespec(&param->interval_on, &param->interval_off)) {
    LOG_E(0x51, "IsValidTimespec() fail");
    *ret = kPlErrInvalidParam;
    return false;
  }
  *ret = kPlErrCodeOk;
  return true;
}
// -----------------------------------------------------------------------------
static bool IsValidStopParam(uint32_t led_id, PlErrCode *ret) {
  // Parameter ret is guaranteed to be not NULL.
  if (CONFIG_PL_LED_LEDS_NUM <= led_id) {
    LOG_E(0x52, "Invalid led_id:%u", led_id);
    *ret = kPlErrInvalidParam;
    return false;
  }
  if (s_ctrl_info[led_id].state != kLedStateStart) {
    LOG_E(0x53, "led:%u invalid state:%u", led_id, s_ctrl_info[led_id].state);
    *ret = kPlErrInvalidState;
    return false;
  }
  *ret = kPlErrCodeOk;
  return true;
}
// -----------------------------------------------------------------------------
static bool IsValidStartSyncParam(const PlLedStartParamEx *param,
                                uint32_t param_len, PlErrCode *ret) {
  // Parameter ret is guaranteed to be not NULL.
  if (param == NULL) {
    LOG_E(0x54, "param is NULL.");
    *ret = kPlErrInvalidParam;
    return false;
  }
  if (param_len == 0) {
    LOG_E(0x55, "param_len is 0");
    *ret = kPlErrInvalidParam;
    return false;
  }
  bool selected_leds[CONFIG_PL_LED_LEDS_NUM] = {false};
  for (uint32_t i = 0; i < param_len; i++) {
    if (!IsValidTimespec(&param[i].interval_on, &param[i].interval_off)) {
      LOG_E(0x56, "IsValidTimespec() fail");
      *ret = kPlErrInvalidParam;
      return false;
    }
    LOG_D("param[%u].led_id:%u", i, param[i].led_id);
    uint32_t led_id = param[i].led_id;
    uint32_t color_id = param[i].color_id;
    if (CONFIG_PL_LED_LEDS_NUM <= led_id) {
      LOG_E(0x57, "Invalid led id.%u", led_id);
      *ret = kPlErrInvalidParam;
      return false;
    }
    if ((s_ctrl_info[led_id].state != kLedStateStop) &&
        (s_ctrl_info[led_id].state != kLedStateStartSync)) {
      LOG_E(0x58, "led:%u invalid state:%u",
            led_id, s_ctrl_info[led_id].state);
      *ret = kPlErrInvalidState;
      return false;
    }
    if ((color_id != CONFIG_PL_LED_COLOR0_ID) &&
        (color_id != CONFIG_PL_LED_COLOR1_ID) &&
        (color_id != CONFIG_PL_LED_COLOR2_ID) &&
        (color_id != CONFIG_PL_LED_COLOR_OFF_ID)) {
      LOG_E(0x59, "Invalid color id.%u", color_id);
      *ret = kPlErrInvalidParam;
      return false;
    }
    if (selected_leds[led_id]) {
      LOG_E(0x5A, "Duplicated led id.%u", led_id);
      *ret = kPlErrInvalidParam;
      return false;
    }
    selected_leds[led_id] = true;
    LOG_D("selected_leds[%u]=true", led_id);
  }
  *ret = kPlErrCodeOk;
  return true;
}
// -----------------------------------------------------------------------------
static bool IsValidStartSeqParam(uint32_t led_id,
                                 const PlLedSequence *seq,
                                 uint32_t seq_len, PlErrCode *ret) {
  // Parameter ret is guaranteed to be not NULL.
  if (CONFIG_PL_LED_LEDS_NUM <= led_id) {
    LOG_E(0x5B, "Invalid led id.%u", led_id);
    *ret = kPlErrInvalidParam;
    return false;
  }
  if (seq == NULL) {
    LOG_E(0x5C, "param is NULL.");
    *ret = kPlErrInvalidParam;
    return false;
  }
  if (seq_len == 0) {
    LOG_E(0x5D, "seq_len is 0");
    *ret = kPlErrInvalidParam;
    return false;
  }
  if ((s_ctrl_info[led_id].state != kLedStateStop) &&
      (s_ctrl_info[led_id].state != kLedStateStartSeq)) {
    LOG_E(0x5E, "led:%u invalid state:%u", led_id, s_ctrl_info[led_id].state);
    *ret = kPlErrInvalidState;
    return false;
  }
  for (uint32_t i = 0; i < seq_len; i++) {
    if (!IsValidTimespec(&seq[i].interval, &seq[i].interval)) {
      LOG_E(0x5F, "IsValidTimespec() fail");
      *ret = kPlErrInvalidParam;
      return false;
    }
    uint32_t color_id = seq[i].color_id;
    if ((color_id != CONFIG_PL_LED_COLOR0_ID) &&
        (color_id != CONFIG_PL_LED_COLOR1_ID) &&
        (color_id != CONFIG_PL_LED_COLOR2_ID) &&
        (color_id != CONFIG_PL_LED_COLOR_OFF_ID)) {
      LOG_E(0x60, "Invalid color id.%u", color_id);
      *ret = kPlErrInvalidParam;
      return false;
    }
  }
  *ret = kPlErrCodeOk;
  return true;
}
// -----------------------------------------------------------------------------
static bool IsValidStopSyncParam(uint32_t *led_ids, uint32_t len,
                                  PlErrCode *ret) {
  // Parameter ret is guaranteed to be not NULL.
  if (led_ids == NULL) {
    LOG_E(0x61, "led_ids is NULL.");
    *ret = kPlErrInvalidParam;
    return false;
  }
  if ((len == 0) || CONFIG_PL_LED_LEDS_NUM < len) {
    LOG_E(0x62, "Invalid len:%u", len);
    *ret = kPlErrInvalidParam;
    return false;
  }
  bool selected_leds[CONFIG_PL_LED_LEDS_NUM] = {false};
  for (uint32_t i = 0; i < len; i++) {
    uint32_t led_id = led_ids[i];
    if (CONFIG_PL_LED_LEDS_NUM <= led_id) {
      LOG_E(0x63, "Invalid led id.%u", led_id);
      *ret = kPlErrInvalidParam;
      return false;
    }
    if (s_ctrl_info[led_id].state != kLedStateStartSync) {
      LOG_E(0x64, "led:%u invalid state:%u",
            led_id, s_ctrl_info[led_id].state);
      *ret = kPlErrInvalidState;
      return false;
    }
    if (selected_leds[led_id]) {
      LOG_E(0x65, "Duplicated led id.%u", led_id);
      *ret = kPlErrInvalidParam;
      return false;
    }
    selected_leds[led_id] = true;
  }
  *ret = kPlErrCodeOk;
  return true;
}
// -----------------------------------------------------------------------------
static bool IsValidTimespec(const struct timespec *interval_on,
                            const struct timespec *interval_off) {
  int64_t nsec_on  = CalcNsec(interval_on);
  int64_t nsec_off = CalcNsec(interval_off);
  if (nsec_on == 0 || nsec_off == 0) {
    return true;  // Always on
  }
  int64_t nsec_max = MAX(nsec_on, nsec_off);
  int64_t nsec_min = MIN(nsec_on, nsec_off);
  if ((nsec_min < s_interval_nsec_min) || (s_interval_nsec_max < nsec_max)) {
    LOG_E(0x66,
          "interval_on(%lld) interval_off(%lld) out of range(%lld - %lld).",
          nsec_on, nsec_off, s_interval_nsec_min, s_interval_nsec_max);
    return false;
  }
  return true;
}
// -----------------------------------------------------------------------------
static PlErrCode RegisterSeq(uint32_t led_id,
                             const PlLedSequence *seq,
                             uint32_t seq_len) {
  PlLedSequence *tmp_seq = calloc(seq_len, sizeof(PlLedSequence));
  if (tmp_seq == NULL) {
    LOG_E(0x67, "Failed to calloc:%lu", seq_len * sizeof(PlLedSequence));
    return kPlErrMemory;
  }
  if (s_ctrl_info[led_id].seq != NULL) {
    free(s_ctrl_info[led_id].seq);
  }
  s_ctrl_info[led_id].seq = tmp_seq;
  s_ctrl_info[led_id].seq_len = seq_len;
  s_ctrl_info[led_id].seq_no = 0;
  s_ctrl_info[led_id].elapsed_ns = 0;
  s_ctrl_info[led_id].waiting_interval = false;
  for (uint32_t i = 0; i < seq_len; i++) {
    s_ctrl_info[led_id].seq[i].color_id = seq[i].color_id;
    s_ctrl_info[led_id].seq[i].interval = seq[i].interval;
    LOG_T("[IN] seq[%u].color:%u", i, seq[i].color_id);
  }
  ExecuteSeq(&s_ctrl_info[led_id], led_id, 0);
  return kPlErrCodeOk;
}
// -----------------------------------------------------------------------------
static PlErrCode RegisterOnSeq(uint32_t led_id, uint32_t color_id) {
  LOG_T("[IN] seq_len=1, led_id:%u color:%u", led_id, color_id);
  struct timespec interval = {0};
  PlLedSequence seq = {
    .color_id = color_id,
    .interval = interval
  };
  return RegisterSeq(led_id, &seq, 1);
}
// -----------------------------------------------------------------------------
static PlErrCode RegisterOnOffSeq(uint32_t led_id,
                                  uint32_t color_id,
                                  const struct timespec *interval_on,
                                  const struct timespec *interval_off) {
  LOG_T("[IN] seq_len=2, led_id:%u seq[0].color:%u, seq[1].color:%d",
              led_id, color_id, CONFIG_PL_LED_COLOR_OFF_ID);
  PlLedSequence seq[2] = {
    {
      .color_id = color_id,
      .interval = *interval_on
    },
    {
      .color_id = CONFIG_PL_LED_COLOR_OFF_ID,
      .interval = *interval_off
    }
  };
  return RegisterSeq(led_id, seq, 2);
}
// -----------------------------------------------------------------------------
static void UnregisterSeq(uint32_t led_id) {
  if (s_ctrl_info[led_id].seq != NULL) {
    free(s_ctrl_info[led_id].seq);
    s_ctrl_info[led_id].seq = NULL;
    s_ctrl_info[led_id].seq_len = 0;
    s_ctrl_info[led_id].seq_no = 0;
    s_ctrl_info[led_id].waiting_interval = false;
  }
}
// -----------------------------------------------------------------------------
static void ExecuteSeq(struct LedCtrlInfo *ctrl_info,
                        uint32_t led_id, int64_t elapsed_time_ns) {
  PlLedSequence *seq = &ctrl_info->seq[ctrl_info->seq_no];

  if (ctrl_info->waiting_interval) {
    int64_t interval = CalcNsec(&seq->interval);
    if (interval == 0) {
      return;
    }
    ctrl_info->elapsed_ns += elapsed_time_ns;
    if (interval <= ctrl_info->elapsed_ns) {
      ctrl_info->elapsed_ns = 0;
      ctrl_info->waiting_interval = false;
      // Set next sequence
      ctrl_info->seq_no++;
      if (ctrl_info->seq_len <= ctrl_info->seq_no) {
        ctrl_info->seq_no = 0;
      }
      seq = &ctrl_info->seq[ctrl_info->seq_no];
    }
  }

  if (ctrl_info->waiting_interval == false) {
    PlErrCode ret = PlLedHwOn(led_id, seq->color_id);
    if (ret != kPlErrCodeOk) {
      LOG_E(0x68, "Failed to PlLedHwOn:%u led_id:%u color_id:%u",
                    ret, led_id, seq->color_id);
      return;
    }
    ctrl_info->waiting_interval = true;
  }
}
// -----------------------------------------------------------------------------
static int64_t CalcNsec(const struct timespec *time) {
  const int64_t nsec = 1000 * 1000 * 1000;
  return (int64_t)time->tv_sec * nsec + (int64_t)time->tv_nsec;
}
