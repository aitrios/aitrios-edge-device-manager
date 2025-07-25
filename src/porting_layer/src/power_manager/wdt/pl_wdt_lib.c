/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <nuttx/irq.h>
#include <nuttx/timers/watchdog.h>

#include "utility_log_module_id.h"
#include "utility_log.h"

#include "pl.h"
#include "pl_wdt_lib.h"
#include "pl_wdt_lib_impl.h"
#include "pl_system_control.h"

// Macros ---------------------------------------------------------------------
#define MAX_TIMEOUT_SEC (60)
#define EVENT_ID  0x9800
#define EVENT_ID_START 0x008A
#define LOG_ERR(event_id, format, ...) \
  WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, \
    "%s-%d:" format, __FILE__, __LINE__, ##__VA_ARGS__); \
  if (event_id >= 0) { \
    WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (EVENT_ID | \
                    (0x00FF & (EVENT_ID_START + event_id)))); \
  }

#define LOG_INFO(format, ...) \
  WRITE_DLOG_INFO(MODULE_ID_SYSTEM, \
    "%s-%d:" format, __FILE__, __LINE__, ##__VA_ARGS__);

// Global Variables -----------------------------------------------------------

static pthread_mutex_t s_api_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool s_is_initialized       = false;
static WdtHandleInfo s_wdt_handle_info[CONFIG_EXTERNAL_PL_WDT_NUM];

// Local functions ------------------------------------------------------------
static void *WdtCallback(int irq, FAR void *context, FAR void *arg);
static PlErrCode PlWdtLibClose_(WdtHandleInfo *handle_info);
static PlErrCode PlWdtLibStop_(WdtHandleInfo *handle_info);
static PlErrCode PlWdtLibUnregisterIrqHandler_(WdtHandleInfo *info);

// Functions ------------------------------------------------------------------
static void *WdtCallback(int irq, FAR void *context, FAR void *arg) {
  (void)context;  // Avoid compiler warning
  (void)arg;      // Avoid compiler warning

  WdtCallbackImpl(irq, s_wdt_handle_info, CONFIG_EXTERNAL_PL_WDT_NUM);
  PlSystemCtlExecOperation(kPlSystemCtlRebootCpu);

  return NULL;
}

// ----------------------------------------------------------------------------
PlErrCode PlWdtLibInitialize(void) {
  PlErrCode err_code = kPlErrCodeOk;
  int ret = 0;

  ret = pthread_mutex_lock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x00, "Lock error. errno=%d", errno);
    err_code = kPlErrLock;
    goto fin;
  }

  if (s_is_initialized) {
    LOG_ERR(0x01, "Already initialized.");
    err_code = kPlErrInvalidState;
    goto unlock;
  }

  for (uint32_t i = 0; i < CONFIG_EXTERNAL_PL_WDT_NUM; i++) {
    s_wdt_handle_info[i].wdt_handle = 0xFFFFFFFF;
    s_wdt_handle_info[i].state = kWdtLibStateClose;
    s_wdt_handle_info[i].irq_handler = NULL;
    s_wdt_handle_info[i].private_data = NULL;
  }

  s_is_initialized = true;

unlock:
  ret = pthread_mutex_unlock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x02, "Unlock error. errno=%d", errno);
  }

fin:
  return err_code;
}

// ----------------------------------------------------------------------------
PlErrCode PlWdtLibFinalize(void) {
  PlErrCode err_code = kPlErrCodeOk;
  int ret = 0;

  ret = pthread_mutex_lock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x03, "Lock error. errno=%d", errno);
    err_code = kPlErrLock;
    goto fin;
  }

  if (!s_is_initialized) {
    LOG_ERR(0x04, "Not initialized.");
    err_code = kPlErrInvalidState;
    goto unlock;
  }

  for (uint32_t i = 0; i < CONFIG_EXTERNAL_PL_WDT_NUM; i++) {
    if (s_wdt_handle_info[i].state == kWdtLibStateStart) {
      err_code = PlWdtLibStop_(&s_wdt_handle_info[i]);
    }

    if (s_wdt_handle_info[i].irq_handler != NULL) {
      err_code = PlWdtLibUnregisterIrqHandler_(&s_wdt_handle_info[i]);
    }

    if (s_wdt_handle_info[i].state == kWdtLibStateStop) {
      err_code = PlWdtLibClose_(&s_wdt_handle_info[i]);
    }
  }

  s_is_initialized = false;

unlock:
  ret = pthread_mutex_unlock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x05, "Unlock error. errno=%d", errno);
  }

fin:
  return err_code;
}

// ----------------------------------------------------------------------------
PlErrCode PlWdtLibOpen(PlWdtLibHandle *handle, uint32_t wdt_num) {
  PlErrCode err_code = kPlErrCodeOk;
  int ret = 0;

  ret = pthread_mutex_lock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x06, "Lock error. errno=%d", errno);
    err_code = kPlErrLock;
    goto fin;
  }

  if (!s_is_initialized) {
    LOG_ERR(0x07, "Not initialized.");
    err_code = kPlErrInvalidState;
    goto unlock;
  }

  if (handle == NULL) {
    LOG_ERR(0x08, "Handler is NULL.");
    err_code = kPlErrInvalidParam;
    goto unlock;
  }

  if (s_wdt_handle_info[wdt_num].state != kWdtLibStateClose) {
    LOG_ERR(0x09, "Already opend. num=%d", wdt_num);
    err_code = kPlErrInvalidState;
    goto unlock;
  }

  err_code = PlWdtLibOpenImpl(wdt_num,
                                s_wdt_handle_info, CONFIG_EXTERNAL_PL_WDT_NUM);
  if (err_code != kPlErrCodeOk) {
    LOG_ERR(0x0A, "WDT Open error. err_code=%d", err_code);
    goto unlock;
  }

  s_wdt_handle_info[wdt_num].wdt_no = wdt_num;
  s_wdt_handle_info[wdt_num].state = kWdtLibStateStop;
  *handle = (struct PlWdtLibHandle*)&s_wdt_handle_info[wdt_num];


unlock:
  ret = pthread_mutex_unlock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x0B, "Unlock error. errno=%d", errno);
  }

fin:
  return err_code;
}

// ----------------------------------------------------------------------------
static PlErrCode PlWdtLibClose_(WdtHandleInfo *handle_info) {
  int ret = close(handle_info->wdt_handle);
  if (ret < 0) {
    LOG_ERR(0x0C, "WDT Close error. errno=%d", errno);
    return kPlErrInternal;
  }
  handle_info->wdt_handle = 0xFFFFFFFF;
  handle_info->state = kWdtLibStateClose;
  handle_info->wdt_no = -1;

  return kPlErrCodeOk;
}

// ----------------------------------------------------------------------------
PlErrCode PlWdtLibClose(PlWdtLibHandle handle) {
  PlErrCode err_code = kPlErrCodeOk;
  int ret = 0;

  ret = pthread_mutex_lock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x0D, "Lock error. errno=%d", errno);
    err_code = kPlErrLock;
    goto fin;
  }

  if (!s_is_initialized) {
    LOG_ERR(0x0E, "Not initialized.");
    err_code = kPlErrInvalidState;
    goto unlock;
  }

  if (handle == NULL) {
    LOG_ERR(0x0F, "Handler is NULL.");
    err_code = kPlErrInvalidParam;
    goto unlock;
  }

  WdtHandleInfo *handle_info = (WdtHandleInfo*)handle;
  if (handle_info->state != kWdtLibStateStop) {
    LOG_ERR(0x10, "Not STOP status. state=%d", handle_info->state);
    err_code = kPlErrInvalidState;
    goto unlock;
  }

  err_code = PlWdtLibClose_(handle_info);

unlock:
  ret = pthread_mutex_unlock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x11, "Unlock error. errno=%d", errno);
  }

fin:
  return err_code;
}

// ----------------------------------------------------------------------------
PlErrCode PlWdtLibStart(PlWdtLibHandle handle) {
  PlErrCode err_code = kPlErrCodeOk;
  int ret = 0;

  ret = pthread_mutex_lock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x12, "Lock error. errno=%d", errno);
    err_code = kPlErrLock;
    goto fin;
  }

  if (!s_is_initialized) {
    LOG_ERR(0x13, "Not initialized.");
    err_code = kPlErrInvalidState;
    goto unlock;
  }

  if (handle == NULL) {
    LOG_ERR(0x14, "Handle is NULL.");
    err_code = kPlErrInvalidParam;
    goto unlock;
  }

  WdtHandleInfo *handle_info = (WdtHandleInfo*)handle;
  if (handle_info->state != kWdtLibStateStop) {
    LOG_ERR(0x15, "Not STOP status. state=%d", handle_info->state);
    err_code = kPlErrInvalidState;
    goto unlock;
  }

  ret = ioctl(handle_info->wdt_handle, WDIOC_START, 0);
  if (ret < 0) {
    LOG_ERR(0x16, "[WDT Start error. ret=%d", ret);
    err_code = kPlErrInternal;
    goto unlock;
  }

  handle_info->state = kWdtLibStateStart;

unlock:
  ret = pthread_mutex_unlock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x17, "Unlock error. errno=%d", errno);
  }

fin:
  return err_code;
}

// ----------------------------------------------------------------------------
static PlErrCode PlWdtLibStop_(WdtHandleInfo *handle_info) {
  int ret = ioctl(handle_info->wdt_handle, WDIOC_STOP, 0);
  if (ret < 0) {
    LOG_ERR(0x18, "WDT Stop error. ret=%d", ret);
    return kPlErrInternal;
  }
  handle_info->state = kWdtLibStateStop;

  return kPlErrCodeOk;
}

// ----------------------------------------------------------------------------
PlErrCode PlWdtLibStop(PlWdtLibHandle handle) {
  PlErrCode err_code = kPlErrCodeOk;
  int ret = 0;

  ret = pthread_mutex_lock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x19, "Lock error. errno=%d", errno);
    err_code = kPlErrLock;
    goto fin;
  }

  if (!s_is_initialized) {
    LOG_ERR(0x1A, "Not initialized.");
    err_code = kPlErrInvalidState;
    goto unlock;
  }

  if (handle == NULL) {
    LOG_ERR(0x1B, "Handle is NULL.");
    err_code = kPlErrInvalidParam;
    goto unlock;
  }

  WdtHandleInfo *handle_info = (WdtHandleInfo*)handle;
  if (handle_info->state != kWdtLibStateStart) {
    LOG_ERR(0x1C, "Not Start status. state=%d", handle_info->state);
    err_code = kPlErrInvalidState;
    goto unlock;
  }

  err_code = PlWdtLibStop_(handle_info);

unlock:
  ret = pthread_mutex_unlock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x1D, "Unlock error. errno=%d", errno);
  }

fin:
  return err_code;
}

// ----------------------------------------------------------------------------
PlErrCode PlWdtLibSetTimeout(PlWdtLibHandle handle, uint32_t timeout_sec) {
  PlErrCode err_code = kPlErrCodeOk;
  int ret = 0;

  ret = pthread_mutex_lock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x1E, "Lock error. errno=%d", errno);
    err_code = kPlErrLock;
    goto fin;
  }

  if (!s_is_initialized) {
    LOG_ERR(0x1F, "Not initialized.");
    err_code = kPlErrInvalidState;
    goto unlock;
  }

  if (handle == NULL) {
    LOG_ERR(0x20, "Handle is NULL.");
    err_code = kPlErrInvalidParam;
    goto unlock;
  }

  WdtHandleInfo *handle_info = (WdtHandleInfo*)handle;
  if (handle_info->state != kWdtLibStateStop) {
    LOG_ERR(0x21, "Not STOP status. state=%d", handle_info->state);
    err_code = kPlErrInvalidState;
    goto unlock;
  }

  unsigned long msec = (timeout_sec > MAX_TIMEOUT_SEC) ?  //NOLINT
                            (unsigned long)(MAX_TIMEOUT_SEC * 1000) :  //NOLINT
                            (unsigned long)(timeout_sec * 1000);  //NOLINT

  // If wdt0's timeout_sec is 60[sec], subtract 1.
  if ((handle_info->wdt_no == 0) &&
      (msec == (unsigned long)(MAX_TIMEOUT_SEC * 1000))) {  //NOLINT
    msec -= 1000;
  }

  ret = ioctl(handle_info->wdt_handle, WDIOC_SETTIMEOUT, msec);
  if (ret < 0) {
    LOG_ERR(0x22, "WDT SetTimeout error. ret=%d", ret);
    err_code = kPlErrInternal;
    goto unlock;
  }

unlock:
  ret = pthread_mutex_unlock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x23, "Unlock error. errno=%d", errno);
  }

fin:
  return err_code;
}


// ----------------------------------------------------------------------------
PlErrCode PlWdtLibRegisterIrqHandler(PlWdtLibHandle handle,
                              PlWdtLibIrqHandler handler, void *private_data) {
  PlErrCode err_code = kPlErrCodeOk;
  int ret = 0;

  ret = pthread_mutex_lock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x24, "Lock error. errno=%d", errno);
    err_code = kPlErrLock;
    goto fin;
  }

  if (!s_is_initialized) {
    LOG_ERR(0x25, "Not initialized.");
    err_code = kPlErrInvalidState;
    goto unlock;
  }

  if (handle == NULL) {
    LOG_ERR(0x26, "Handle is NULL.");
    err_code = kPlErrInvalidParam;
    goto unlock;
  }

  WdtHandleInfo *handle_info = (WdtHandleInfo*)handle;
  if (handle_info->state != kWdtLibStateStop) {
    LOG_ERR(0x27, "Not STOP status. state=%d", handle_info->state);
    err_code = kPlErrInvalidState;
    goto unlock;
  }

  if (handle_info->irq_handler != NULL) {
    LOG_ERR(0x28, "Already registered.");
    err_code = kPlErrInvalidParam;
    goto unlock;
  }

  struct watchdog_capture_s wdt_cap = {
    .newhandler = (void *)WdtCallback,
    .oldhandler = NULL,
  };
  ret = ioctl(handle_info->wdt_handle, WDIOC_CAPTURE, &wdt_cap);
  if (ret < 0) {
    LOG_ERR(0x29, "WDT Registered error. errno=%d", errno);
    err_code = kPlErrInternal;
    goto unlock;
  }

  handle_info->irq_handler = handler;
  handle_info->private_data = private_data;

unlock:
  ret = pthread_mutex_unlock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x2A, "Unlock error. errno=%d", errno);
  }

fin:
  return err_code;
}

// ----------------------------------------------------------------------------
static PlErrCode PlWdtLibUnregisterIrqHandler_(WdtHandleInfo *info) {
  struct watchdog_capture_s wdt_cap = {
    .newhandler = NULL,
    .oldhandler = (void *)WdtCallback,
  };
  int ret = ioctl(info->wdt_handle, WDIOC_CAPTURE, &wdt_cap);
  if (ret < 0) {
    LOG_ERR(0x2B, "WDT Unregistered error. errno=%d", errno);
    return kPlErrInternal;
  }
  info->irq_handler = NULL;
  info->private_data = NULL;

  return kPlErrCodeOk;
}

// ----------------------------------------------------------------------------
PlErrCode PlWdtLibUnregisterIrqHandler(PlWdtLibHandle handle) {
  PlErrCode err_code = kPlErrCodeOk;
  int ret = 0;

  ret = pthread_mutex_lock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x2C, "Lock error. errno=%d", errno);
    err_code = kPlErrLock;
    goto fin;
  }

  if (!s_is_initialized) {
    LOG_ERR(0x2D, "Not initialized.");
    err_code = kPlErrInvalidState;
    goto unlock;
  }

  if (handle == NULL) {
    LOG_ERR(0x2E, "Handle is NULL.");
    err_code = kPlErrInvalidParam;
    goto unlock;
  }

  WdtHandleInfo *handle_info = (WdtHandleInfo*)handle;
  if (handle_info->state != kWdtLibStateStop) {
    LOG_ERR(0x2F, "Not STOP status. state=%d", handle_info->state);
    err_code = kPlErrInvalidState;
    goto unlock;
  }

  if (handle_info->irq_handler == NULL) {
    LOG_ERR(0x30, "already unregistered.");
    err_code = kPlErrInvalidParam;
    goto unlock;
  }

  err_code = PlWdtLibUnregisterIrqHandler_(handle_info);

unlock:
  ret = pthread_mutex_unlock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x31, "Unlock error. errno=%d", errno);
  }

fin:
  return err_code;
}

// ----------------------------------------------------------------------------
PlErrCode PlWdtLibKeepAlive(PlWdtLibHandle handle) {
  PlErrCode err_code = kPlErrCodeOk;
  int ret = 0;

  ret = pthread_mutex_lock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x32, "Lock error. errno=%d", errno);
    err_code = kPlErrLock;
    goto fin;
  }

  if (!s_is_initialized) {
    LOG_ERR(0x33, "Not initialized.");
    err_code = kPlErrInvalidState;
    goto unlock;
  }

  WdtHandleInfo *handle_info = (WdtHandleInfo*)handle;
  ret = ioctl(handle_info->wdt_handle, WDIOC_KEEPALIVE, 0);
  if (ret < 0) {
    LOG_ERR(0x34, "WDT KeepAlive error. errno=%d", errno);
    err_code = kPlErrInternal;
    goto unlock;
  }
#ifdef CONFIG_EXTERNAL_PL_WDT_KEEP_ALIVE_LOG
  LOG_INFO("KeepAlive to WDT%d.", handle_info->wdt_no);
#endif

unlock:
  ret = pthread_mutex_unlock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x35, "Unlock error. errno=%d", errno);
  }

fin:
  return err_code;
}

// ----------------------------------------------------------------------------
#ifdef CONFIG_DEV_CONSOLE
static void UpPuts(const char *format, ...) {
  va_list list;
  char buf[128] = {0};
  va_start(list, format);
  vsnprintf(buf, sizeof(buf) - 1, format, list);
  va_end(list);
  up_puts(buf);
}
#endif
// ----------------------------------------------------------------------------
PlErrCode PlWdtLibKeepAliveIrqContext(uint32_t wdt_num) {
#ifdef CONFIG_DEV_CONSOLE
  UpPuts("WDT Keepalive to WDT%u.\n", wdt_num);
#endif
  return PlWdtLibKeepAliveIrqContextImpl(wdt_num);
}
// ----------------------------------------------------------------------------
