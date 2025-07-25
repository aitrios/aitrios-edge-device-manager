/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#ifdef __linux__
#include <stdbool.h>
#include <bsd/sys/queue.h>
#endif
#ifdef __NuttX__
#include <nuttx/spinlock.h>
#include <nuttx/irq.h>
#include <sys/queue.h>
#endif
#include <errno.h>

#include "hal_ioexp.h"
#include "hal_driver.h"
#include "hal_driver_ioexp.h"
#include "hal_ioexp_impl.h"
#include "utility_log.h"
#include "utility_log_module_id.h"

// Macros ---------------------------------------------------------------------
#define EVENT_ID        (0x9C00)
#define EVENT_ID_START  (0x00)
#define EVENT_UID_START (0x00)
#define LOG_E(event_id, format, ...) \
  WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:" format, \
                   __FILE__, __LINE__, ##__VA_ARGS__); \
  WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, \
                   (EVENT_ID | (EVENT_ID_START + EVENT_UID_START + event_id)));

#define LOG_W(event_id, format, ...) \
  WRITE_DLOG_WARN(MODULE_ID_SYSTEM, "%s-%d:" format, \
                  __FILE__, __LINE__, ##__VA_ARGS__); \
  WRITE_ELOG_WARN(MODULE_ID_SYSTEM, \
                  (EVENT_ID | (EVENT_ID_START + EVENT_UID_START + event_id)));

#define LOG_I(format, ...) \
  WRITE_DLOG_INFO(MODULE_ID_SYSTEM, "%s-%d:" format, \
                  __FILE__, __LINE__, ##__VA_ARGS__)

// Global Variables -----------------------------------------------------------
TAILQ_HEAD(IoexpDataList, IoexpData);

struct IoexpData {
  TAILQ_ENTRY(IoexpData) head;
  bool is_opened;
  uint32_t ioexp_id;
  uint32_t device_id;
  uint32_t pin;
  bool reverse;
  uint32_t irq_num;
  HalIoexpIrqHandler irq_handler;
  void* irq_private_data;
  struct DeviceInfo *dev_info;
};

struct DeviceInfo {
  HalDriverHandle driver_handle;
  uint32_t pin_total_num;
};

static pthread_mutex_t s_api_mutex     = PTHREAD_MUTEX_INITIALIZER;
static struct IoexpDataList s_ioexp_list = {0};
static bool s_is_initialized           = false;
static struct DeviceInfo *s_dev_info   = NULL;
static uint32_t s_dev_total_num        = 0;
#ifdef __NuttX__
static spinlock_t s_irqlock;
#endif
// Local functions ------------------------------------------------------------
static void WrapperHandler(IoexpValue val, void *private_data);
static HalErrCode HalIoexpClose_(struct IoexpData *ioexp_info);
static HalErrCode HalIoexpUnregisterIrqHandler_(struct IoexpData *ioexp_info);
static HalErrCode GetIoexpItem(uint32_t ioexp_id, struct IoexpData** item);
static bool IsValidHandle(HalIoexpHandle handle_);
static void DeleteAllDevInfo(void);
static void DeleteAllIoexpList(void);
static HalErrCode InitDeviceInfoAndList(void);

// Functions ------------------------------------------------------------------
static void WrapperHandler(IoexpValue val, void *private_data) {
#ifdef __NuttX__
#ifdef CONFIG_SPINLOCK
  spin_lock(&s_irqlock);
#endif
#endif

  struct IoexpData *ioexp_info = private_data;
  HalIoexpIrqHandler handler = ioexp_info->irq_handler;
  if (handler == NULL) {
    LOG_E(0x00, "handler is NULL.");
    goto unlock;
  }

  HalIoexpValue value = (val == kIoexpValueLow) ?
                                      kHalIoexpValueLow : kHalIoexpValueHigh;
  if (ioexp_info->reverse) {
    value = (val == kIoexpValueLow) ? kHalIoexpValueHigh : kHalIoexpValueLow;
  }
  handler(value, ioexp_info->irq_private_data);

unlock:
#ifdef __NuttX__
#ifdef CONFIG_SPINLOCK
  spin_unlock(&s_irqlock);
#endif
#endif

  return;
}

// ----------------------------------------------------------------------------
HalErrCode HalIoexpInitialize(void) {
  HalErrCode err_code = kHalErrCodeOk;
  int lock_ret = 0;

  lock_ret = pthread_mutex_lock(&s_api_mutex);
  if (lock_ret) {
    LOG_E(0x01, "Lock error. errno=%d", errno);
    err_code = kHalErrLock;
    goto fin;
  }

  if (s_is_initialized) {
    LOG_E(0x02, "Already initialized.");
    err_code = kHalErrInvalidState;
    goto unlock;
  }

  TAILQ_INIT(&s_ioexp_list);

  err_code = InitDeviceInfoAndList();
  if (err_code != kHalErrCodeOk) {
    LOG_E(0x03, "InitDeviceInfoAndList is failed. err_code=%u", err_code);
    goto unlock;
  }

  s_is_initialized = true;

unlock:
  lock_ret = pthread_mutex_unlock(&s_api_mutex);
  if (lock_ret) {
    LOG_E(0x05, "Unlock error. errno=%d", errno);
  }

fin:
  return err_code;
}

//-----------------------------------------------------------------------------
HalErrCode HalIoexpFinalize(void) {
  HalErrCode err_code = kHalErrCodeOk;
  int lock_ret = 0;

  lock_ret = pthread_mutex_lock(&s_api_mutex);
  if (lock_ret) {
    LOG_E(0x06, "Lock error. errno=%d", errno);
    return kHalErrLock;
  }

  if (!s_is_initialized) {
    LOG_E(0x07, "Not initialized.");
    err_code = kHalErrInvalidState;
    goto unlock;
  }

  struct IoexpData *ioexp_entry = NULL, *ioexp_temp = NULL;
  TAILQ_FOREACH_SAFE(ioexp_entry, &s_ioexp_list, head, ioexp_temp) {
    if (ioexp_entry->is_opened) {
      err_code = HalIoexpClose_(ioexp_entry);
      if (err_code != kHalErrCodeOk) {
        LOG_E(0x08, "HalIoexpClose_ is failed. err_code=%d", err_code);
        goto unlock;
      }
    }
  }

  DeleteAllIoexpList();
  DeleteAllDevInfo();

  s_is_initialized = false;

unlock:
  lock_ret = pthread_mutex_unlock(&s_api_mutex);
  if (lock_ret) {
    LOG_E(0x09, "Unlock error. errno=%d", errno);
  }

  return err_code;
}

//-----------------------------------------------------------------------------
HalErrCode HalIoexpOpen(uint32_t ioexp_id, HalIoexpHandle *handle) {
  int lock_ret = pthread_mutex_lock(&s_api_mutex);
  if (lock_ret) {
    LOG_E(0x0A, "Lock error. errno=%d", errno);
    return kHalErrLock;
  }

  HalErrCode err_code = kHalErrCodeOk;
  if (!s_is_initialized) {
    LOG_E(0x0B, "Not initialized.");
    err_code = kHalErrInvalidState;
    goto unlock;
  }

  if (handle == NULL) {
    LOG_E(0x0C, "handle is NULL.");
    err_code = kHalErrInvalidParam;
    goto unlock;
  }

  struct IoexpData *ioexp_info = NULL;
  err_code = GetIoexpItem(ioexp_id, &ioexp_info);
  if (err_code != kHalErrCodeOk) {
    LOG_E(0x0D, "ioexp_id[%d] is not found.", ioexp_id);
    goto unlock;
  }

  if (ioexp_info->is_opened) {
    LOG_I("ioexp_id[%u] is already opened[%d].",
          ioexp_id, ioexp_info->is_opened);
    err_code = kHalErrAlready;
    goto unlock;
  }

  ioexp_info->is_opened = true;
  *handle = (HalIoexpHandle*)ioexp_info;

unlock:
  lock_ret = pthread_mutex_unlock(&s_api_mutex);
  if (lock_ret) {
    LOG_E(0x0E, "Unlock error. errno=%d", errno);
  }

  return err_code;
}

//-----------------------------------------------------------------------------
static HalErrCode HalIoexpClose_(struct IoexpData *ioexp_info) {
  // Wait until Irq Handler execution is complite.
#ifdef __NuttX__
  irqstate_t flags = spin_lock_irqsave(&s_irqlock);
#endif

  HalErrCode err_code = kHalErrCodeOk;
  if (ioexp_info->irq_handler != NULL) {
    err_code = HalIoexpUnregisterIrqHandler_(ioexp_info);
    if (err_code != kHalErrCodeOk) {
      LOG_E(0x0F, "HalIoexpUnregisterIrqHandler_ is failed. err_code=%d",
            err_code);
      goto unlock;
    }
  }
  ioexp_info->is_opened = false;

unlock:
#ifdef __NuttX__
  spin_unlock_irqrestore(&s_irqlock, flags);
#endif

  return err_code;
}

//-----------------------------------------------------------------------------
HalErrCode HalIoexpClose(const HalIoexpHandle handle) {
  int lock_ret = pthread_mutex_lock(&s_api_mutex);
  if (lock_ret) {
    LOG_E(0x10, "Lock error. errno=%d", errno);
    return  kHalErrLock;
  }

  HalErrCode err_code = kHalErrCodeOk;
  if (!s_is_initialized) {
    LOG_E(0x11, "Not initialized.");
    err_code = kHalErrInvalidState;
    goto unlock;
  }

  bool found = IsValidHandle(handle);
  if (!found) {
    LOG_E(0x12, "handle is invalid.");
    err_code = kHalErrInvalidParam;
    goto unlock;
  }

  struct IoexpData *ioexp_info = (struct IoexpData*)handle;
  if (!ioexp_info->is_opened) {
    LOG_E(0x13, "This handle is not opened. dev_id=%d, pin=%d",
          ioexp_info->device_id, ioexp_info->pin);
    err_code = kHalErrInvalidParam;
    goto unlock;
  }

  err_code = HalIoexpClose_(ioexp_info);
  if (err_code != kHalErrCodeOk) {
    LOG_E(0x14, "HalIoexpClose_ is failed. err_code=%d", err_code);
    goto unlock;
  }

unlock:
  lock_ret = pthread_mutex_unlock(&s_api_mutex);
  if (lock_ret) {
    LOG_E(0x15, "Unlock error. errno=%d", errno);
  }

  return err_code;
}

//-----------------------------------------------------------------------------
HalErrCode HalIoexpSetConfigure(const HalIoexpHandle handle,
                                  const struct HalIoexpConfig *config) {
  int lock_ret = pthread_mutex_lock(&s_api_mutex);
  if (lock_ret) {
    LOG_E(0x16, "Lock error. errno=%d", errno);
    return kHalErrLock;
  }

  HalErrCode err_code = kHalErrCodeOk;
  if (!s_is_initialized) {
    LOG_E(0x17, "Not initialized.");
    err_code = kHalErrInvalidState;
    goto unlock;
  }

  if (config == NULL) {
    LOG_E(0x18, "config is NULL.");
    err_code = kHalErrInvalidParam;
    goto unlock;
  }

  if (config->direction != kHalIoexpDirectionInput &&
      config->direction != kHalIoexpDirectionOutput) {
    LOG_E(0x19, "direction[%d] is invalid param.", config->direction);
    err_code = kHalErrInvalidParam;
    goto unlock;
  }

  bool found = IsValidHandle(handle);
  if (!found) {
    LOG_E(0x1A, "handle is invalid.");
    err_code = kHalErrInvalidParam;
    goto unlock;
  }

  struct IoexpData *ioexp_info = (struct IoexpData*)handle;
  if (!ioexp_info->is_opened) {
    LOG_I("This handle is not opened.");
    err_code = kHalErrInvalidParam;
    goto unlock;
  }

  if (ioexp_info->irq_handler != NULL) {
    LOG_E(0x1B, "This handle is registered fo interrupt.");
    err_code = kHalErrInvalidParam;
    goto unlock;
  }

  if (ioexp_info->dev_info->pin_total_num <= ioexp_info->pin) {
    LOG_E(0x1C, "pin does not exist. pin_total_num=%d, pin=%d",
          ioexp_info->dev_info->pin_total_num, ioexp_info->pin);
    err_code = kHalErrInvalidParam;
    goto unlock;
  }

  struct IoexpIoctlArg arg = {0};
  arg.device_id        = ioexp_info->device_id;
  arg.pin              = ioexp_info->pin;
  arg.config.direction = (config->direction == kHalIoexpDirectionOutput) ?
                              kIoexpDirectionOutput : kIoexpDirectionInput;

  err_code = HalDriverIoctl(ioexp_info->dev_info->driver_handle,
                          &arg, kIoexpIoctlCmdSetDirection);
  if (err_code != kHalErrCodeOk) {
    LOG_E(0x1D, "HalDriverIoctl is failed. err_code=%d, dev_id=%d, pin=%d",
          err_code, arg.device_id, arg.pin);
    goto unlock;
  }

unlock:
  lock_ret = pthread_mutex_unlock(&s_api_mutex);
  if (lock_ret) {
    LOG_E(0x1E, "Unlock error. errno=%d", errno);
  }

  return err_code;
}

//-----------------------------------------------------------------------------
HalErrCode HalIoexpGetConfigure(const HalIoexpHandle handle,
                                  struct HalIoexpConfig *config) {
  int lock_ret = pthread_mutex_lock(&s_api_mutex);
  if (lock_ret) {
    LOG_E(0x1F, "Lock error. errno=%d", errno);
    return kHalErrLock;
  }

  HalErrCode err_code = kHalErrCodeOk;
  if (!s_is_initialized) {
    LOG_E(0x20, "Not initialized.");
    err_code = kHalErrInvalidState;
    goto unlock;
  }

  if (config == NULL) {
    LOG_E(0x21, "config is NULL.");
    err_code = kHalErrInvalidParam;
    goto unlock;
  }

  bool found = IsValidHandle(handle);
  if (!found) {
    LOG_E(0x22, "handle is invalid.");
    err_code = kHalErrInvalidParam;
    goto unlock;
  }

  struct IoexpData *ioexp_info = (struct IoexpData*)handle;
  if (!ioexp_info->is_opened) {
    LOG_E(0x23, "This handle is not opened. dev_id=%d, pin=%d",
          ioexp_info->device_id, ioexp_info->pin);
    err_code = kHalErrInvalidParam;
    goto unlock;
  }

  if (ioexp_info->dev_info->pin_total_num <= ioexp_info->pin) {
    LOG_E(0x24, "pin does not exist. pin_total_num=%d, pin=%d",
          ioexp_info->dev_info->pin_total_num, ioexp_info->pin);
    err_code = kHalErrInvalidParam;
    goto unlock;
  }

  struct IoexpIoctlArg arg = {0};
  arg.device_id = ioexp_info->device_id;
  arg.pin       = ioexp_info->pin;

  err_code = HalDriverIoctl(ioexp_info->dev_info->driver_handle,
                            &arg, kIoexpIoctlCmdGetDirection);
  if (err_code != kHalErrCodeOk) {
    LOG_E(0x25, "HalDriverIoctl is failed. err_code=%d, dev_id=%d, pin=%d",
          err_code, arg.device_id, arg.pin);
    goto unlock;
  }

  config->direction = (arg.config.direction == kIoexpDirectionOutput) ?
                      kHalIoexpDirectionOutput : kHalIoexpDirectionInput;

unlock:
  lock_ret = pthread_mutex_unlock(&s_api_mutex);
  if (lock_ret) {
    LOG_E(0x26, "Unlock error. errno=%d", errno);
  }

  return err_code;
}

//-----------------------------------------------------------------------------
HalErrCode HalIoexpWrite(const HalIoexpHandle handle, HalIoexpValue value) {
  int lock_ret = pthread_mutex_lock(&s_api_mutex);
  if (lock_ret) {
    LOG_E(0x27, "Lock error. errno=%d", errno);
    return kHalErrLock;
  }

  HalErrCode err_code = kHalErrCodeOk;
  if (!s_is_initialized) {
    LOG_E(0x28, "Not initialized.");
    err_code = kHalErrInvalidState;
    goto unlock;
  }

  if ((value != kHalIoexpValueLow) && (value != kHalIoexpValueHigh)) {
    LOG_E(0x29, "value[%d] is invalid param.", value);
    err_code = kHalErrInvalidParam;
    goto unlock;
  }

  bool found = IsValidHandle(handle);
  if (!found) {
    LOG_E(0x2A, "handle is invalid.");
    err_code = kHalErrInvalidParam;
    goto unlock;
  }

  struct IoexpData *ioexp_info = (struct IoexpData*)handle;
  if (!ioexp_info->is_opened) {
    LOG_E(0x2B, "This handle is not opened. dev_id=%d, pin=%d",
          ioexp_info->device_id, ioexp_info->pin);
    err_code = kHalErrInvalidParam;
    goto unlock;
  }

  if (ioexp_info->irq_handler != NULL) {
    LOG_E(0x2C, "This handle is registered for interrupt.");
    err_code = kHalErrInvalidParam;
    goto unlock;
  }

  if (ioexp_info->dev_info->pin_total_num <= ioexp_info->pin) {
    LOG_E(0x2D, "pin does not exist. pin_total_num=%d, pin=%d",
          ioexp_info->dev_info->pin_total_num, ioexp_info->pin);
    err_code = kHalErrInvalidParam;
    goto unlock;
  }

  struct IoexpIoctlArg arg = {0};
  arg.device_id = ioexp_info->device_id;
  arg.pin = ioexp_info->pin;

  err_code = HalDriverIoctl(ioexp_info->dev_info->driver_handle,
                            &arg, kIoexpIoctlCmdGetDirection);
  if (err_code != kHalErrCodeOk) {
    LOG_E(0x2E, "HalDriverIoctl is failed. err_code=%d, dev_id=%d, pin=%d",
          err_code, arg.device_id, arg.pin);
    goto unlock;
  }

  if (arg.config.direction == kIoexpDirectionInput) {
    LOG_E(0x2F, "Direction is Input.");
    err_code = kHalErrInputDirection;
    goto unlock;
  }

  if (ioexp_info->reverse) {
    arg.value = (value == kHalIoexpValueLow)
              ? kIoexpValueHigh : kIoexpValueLow;
  } else {
    arg.value = (value == kHalIoexpValueLow)
              ? kIoexpValueLow : kIoexpValueHigh;
  }

  err_code = HalDriverIoctl(ioexp_info->dev_info->driver_handle,
                            &arg, kIoexpIoctlCmdWrite);
  if (err_code != kHalErrCodeOk) {
    LOG_E(0x31, "HalDriverIoctl is failed. err_code=%d, dev_id=%d, pin=%d",
          err_code, ioexp_info->device_id, ioexp_info->pin);
    goto unlock;
  }

unlock:
  lock_ret = pthread_mutex_unlock(&s_api_mutex);
  if (lock_ret) {
    LOG_E(0x32, "Unlock error. errno=%d", errno);
  }

  return err_code;
}

//-----------------------------------------------------------------------------
HalErrCode HalIoexpWriteMulti(const HalIoexpHandle *handle_array,
                              uint32_t handle_num,
                              const HalIoexpValue *value_array,
                              uint32_t value_num) {
  HalErrCode err_code = kHalErrCodeOk;
  int lock_ret = 0;
  lock_ret = pthread_mutex_lock(&s_api_mutex);
  if (lock_ret) {
    LOG_E(0x55, "Lock error. errno=%d", errno);
    err_code = kHalErrLock;
    goto fin;
  }

  if (!s_is_initialized) {
    LOG_E(0x56, "Not initialized.");
    err_code = kHalErrInvalidState;
    goto unlock;
  }

  if ((handle_array == NULL) || (handle_num == 0) ||
      (value_array == NULL) || (value_num == 0) ||
      (handle_num != value_num)) {
    LOG_E(0x57, "invalid param.");
    err_code = kHalErrInvalidParam;
    goto unlock;
  }

  for (uint32_t i = 0; i < value_num; i++) {
    HalIoexpValue value = value_array[i];
    if ((value != kHalIoexpValueLow) && (value != kHalIoexpValueHigh)) {
      LOG_E(0x58, "value_array[%u]:[%d] is invalid param.", i, value);
      err_code = kHalErrInvalidParam;
      goto unlock;
    }
  }

  struct IoexpIoctlArg *args =
                            malloc(sizeof(struct IoexpIoctlArg) * handle_num);
  if (args == NULL) {
    LOG_E(0x59, "malloc error. errno=%d", errno);
    err_code = kHalErrMemory;
    goto unlock;
  }
  memset(args, 0x00, sizeof(struct IoexpIoctlArg) * handle_num);

  struct IoexpData *ioexp_info = NULL;
  for (uint32_t i = 0; i < handle_num; i++) {
    bool found = IsValidHandle(handle_array[i]);
    if (!found) {
      LOG_E(0x5A, "handle is invalid. handle_idx=%u", i);
      err_code = kHalErrInvalidParam;
      goto err;
    }

    ioexp_info = (struct IoexpData*)(handle_array[i]);

    if (!ioexp_info->is_opened) {
      LOG_E(0x5B,
            "This handle is not opened. handle_idx=%u, dev_id=%d, pin=%d",
            i, ioexp_info->device_id, ioexp_info->pin);
      err_code = kHalErrInvalidParam;
      goto err;
    }

    if (ioexp_info->irq_handler != NULL) {
      LOG_E(0x5C, "This handle is registered for interrupt. handle_idx=%u", i);
      err_code = kHalErrInvalidParam;
      goto err;
    }

    if (ioexp_info->dev_info->pin_total_num <= ioexp_info->pin) {
      LOG_E(0x5D, "pin does not exist. handle_idx=%u, pin_total_num=%d, pin=%d",
            i, ioexp_info->dev_info->pin_total_num, ioexp_info->pin);
      err_code = kHalErrInvalidParam;
      goto err;
    }

    args[i].device_id = ioexp_info->device_id;
    args[i].pin = ioexp_info->pin;

    err_code = HalDriverIoctl(ioexp_info->dev_info->driver_handle,
                              &(args[i]), kIoexpIoctlCmdGetDirection);
    if (err_code != kHalErrCodeOk) {
      LOG_E(0x5E,
            "HalDriverIoctl is failed. err_code=%d, handle_idx=%u, "
            "dev_id=%d, pin=%d",
            err_code, i, args[i].device_id, args[i].pin);
      goto err;
    }

    if (args[i].config.direction == kIoexpDirectionInput) {
      LOG_E(0x5F, "Direction is Input. handle_idx=%u", i);
      err_code = kHalErrInputDirection;
      goto err;
    }

    HalIoexpValue value = value_array[i];
    if (ioexp_info->reverse) {
      args[i].value = (value == kHalIoexpValueLow) ?
                      kIoexpValueHigh : kIoexpValueLow;
    } else {
      args[i].value = (value == kHalIoexpValueLow) ?
                      kIoexpValueLow : kIoexpValueHigh;
    }
  }

  struct IoexpIoctlArgArray arg_array;
  arg_array.arg_num = handle_num;
  arg_array.arg_array = args;

  ioexp_info = (struct IoexpData*)(handle_array[0]);
  err_code = HalDriverIoctl(ioexp_info->dev_info->driver_handle,
                            &arg_array, kIoexpIoctlCmdWriteMulti);
  if (err_code != kHalErrCodeOk) {
    LOG_E(0x60, "HalDriverIoctl is failed. err_code=%d, dev_id=%d, pin=%d",
          err_code, ioexp_info->device_id, ioexp_info->pin);
    goto err;
  }

err:
  free(args);

unlock:
  lock_ret = pthread_mutex_unlock(&s_api_mutex);
  if (lock_ret) {
    LOG_E(0x61, "Unlock error. errno=%d", errno);
    err_code = kHalErrUnlock;
  }

fin:
  return err_code;
}

//-----------------------------------------------------------------------------
HalErrCode HalIoexpRead(const HalIoexpHandle handle,
                          HalIoexpValue *value) {
  int lock_ret = pthread_mutex_lock(&s_api_mutex);
  if (lock_ret) {
    LOG_E(0x33, "Lock error. errno=%d", errno);
    return kHalErrLock;
  }

  HalErrCode err_code = kHalErrCodeOk;
  if (!s_is_initialized) {
    LOG_E(0x34, "Not initialized.");
    err_code = kHalErrInvalidState;
    goto unlock;
  }

  if (value == NULL) {
    LOG_E(0x35, "value is NULL.");
    err_code = kHalErrInvalidParam;
    goto unlock;
  }

  bool found = IsValidHandle(handle);
  if (!found) {
    LOG_E(0x36, "handle is invalid.");
    err_code = kHalErrInvalidParam;
    goto unlock;
  }

  struct IoexpData *ioexp_info = (struct IoexpData*)handle;
  if (!ioexp_info->is_opened) {
    LOG_E(0x37, "This handle is not opened. dev_id=%d, pin=%d",
          ioexp_info->device_id, ioexp_info->pin);
    err_code = kHalErrInvalidParam;
    goto unlock;
  }

  if (ioexp_info->dev_info->pin_total_num <= ioexp_info->pin) {
    LOG_E(0x38, "pin does not exist. pin_total_num=%d, pin=%d",
          ioexp_info->dev_info->pin_total_num, ioexp_info->pin);
    err_code = kHalErrInvalidParam;
    goto unlock;
  }

  struct IoexpIoctlArg arg = {0};
  arg.device_id = ioexp_info->device_id;
  arg.pin       = ioexp_info->pin;

  err_code = HalDriverIoctl(ioexp_info->dev_info->driver_handle,
                            &arg, kIoexpIoctlCmdRead);
  if (err_code != kHalErrCodeOk) {
    LOG_E(0x39, "HalDriverIoctl is failed. err_code=%d, dev_id=%d, pin=%d",
          err_code, ioexp_info->device_id, ioexp_info->pin);
    err_code = kHalErrCodeError;
    goto unlock;
  }

  IoexpValue read_value = arg.value;
  if (ioexp_info->reverse) {
    read_value = (arg.value == kIoexpValueLow) ?
                 kIoexpValueHigh : kIoexpValueLow;
  }
  *value = (read_value == kIoexpValueLow) ?
                        kHalIoexpValueLow : kHalIoexpValueHigh;

unlock:
  lock_ret = pthread_mutex_unlock(&s_api_mutex);
  if (lock_ret) {
    LOG_E(0x3B, "Unlock error. errno=%d", errno);
  }

  return err_code;
}

//-----------------------------------------------------------------------------
HalErrCode HalIoexpRegisterIrqHandler(const HalIoexpHandle handle,
                                        HalIoexpIrqHandler handler,
                                        void *private_data,
                                        HalIoexpIrqType type) {
  int lock_ret = pthread_mutex_lock(&s_api_mutex);
  if (lock_ret) {
    LOG_E(0x3C, "Lock error. errno=%d", errno);
    return kHalErrLock;
  }

  HalErrCode err_code = kHalErrCodeOk;
  if (!s_is_initialized) {
    LOG_E(0x3D, "Not initialized.");
    err_code = kHalErrInvalidState;
    goto unlock;
  }

  HalGpioIrqType irq_type = 0;
  switch (type) {
    case kHalIoexpIrqTypeRisingEdge:
      irq_type = kHalGpioIrqTypeInputRisingEdge;
      break;
    case kHalIoexpIrqTypeFallingEdge:
      irq_type = kHalGpioIrqTypeInputFallingEdge;
      break;
    case kHalIoexpIrqTypeBothEdge:
      irq_type = kHalGpioIrqTypeBothEdge;
      break;
    case kHalIoexpIrqTypeLowLevel:
      irq_type = kHalGpioIrqTypeLowLevel;
      break;
    case kHalIoexpIrqTypeHighLevel:
      irq_type = kHalGpioIrqTypeHighLevel;
      break;
    default:
      LOG_E(0x3E, "type[%d] is invalid param.", type);
      err_code = kHalErrInvalidParam;
      goto unlock;
      break;
  }

  if (handler == NULL) {
    LOG_E(0x3F, "handler is NULL.");
    err_code = kHalErrInvalidParam;
    goto unlock;
  }

  bool found = IsValidHandle(handle);
  if (!found) {
    LOG_E(0x40, "handle is invalid.");
    err_code = kHalErrInvalidParam;
    goto unlock;
  }

  struct IoexpData *ioexp_info = (struct IoexpData*)handle;
  if (!ioexp_info->is_opened) {
    LOG_E(0x41, "This handle is not opened. dev_id=%d, pin=%d",
          ioexp_info->device_id, ioexp_info->pin);
    err_code = kHalErrInvalidParam;
    goto unlock;
  }

  if (ioexp_info->irq_handler != NULL) {
    LOG_E(0x42, "This handle is registered for interrupt.");
    err_code = kHalErrInvalidParam;
    goto unlock;
  }

  if (ioexp_info->dev_info->pin_total_num <= ioexp_info->pin) {
    LOG_E(0x43, "pin does not exist. pin_total_num=%d, pin=%d",
          ioexp_info->dev_info->pin_total_num, ioexp_info->pin);
    err_code = kHalErrInvalidParam;
    goto unlock;
  }

  struct IoexpIrqInfo irq_info = {0};
  irq_info.irq_type     = irq_type;
  irq_info.irq_num      = ioexp_info->irq_num;
  irq_info.irq_handler  = (IoexpIrqHandler)WrapperHandler;
  irq_info.private_data = ioexp_info;

  struct IoexpIoctlArg arg = {0};
  arg.device_id = ioexp_info->device_id;
  arg.pin       = ioexp_info->pin;
  arg.irq_info  = irq_info;

  err_code = HalDriverIoctl(ioexp_info->dev_info->driver_handle,
                            &arg, kIoexpIoctlCmdRegIrqHandler);
  if (err_code != kHalErrCodeOk) {
    LOG_E(0x44, "HalDriverIoctl is failed. err_code=%d, dev_id=%d, pin=%d",
          err_code, ioexp_info->device_id, ioexp_info->pin);
    goto unlock;
  }

  ioexp_info->irq_handler      = handler;
  ioexp_info->irq_private_data = private_data;

unlock:
  lock_ret = pthread_mutex_unlock(&s_api_mutex);
  if (lock_ret) {
    LOG_E(0x45, "Unlock error. errno=%d", errno);
  }

  return err_code;
}

//-----------------------------------------------------------------------------
static HalErrCode HalIoexpUnregisterIrqHandler_(struct IoexpData *ioexp_info) {
  struct IoexpIoctlArg arg = {0};
  arg.device_id        = ioexp_info->device_id;
  arg.pin              = ioexp_info->pin;
  arg.irq_info.irq_num = ioexp_info->irq_num;

  HalErrCode err_code = HalDriverIoctl(ioexp_info->dev_info->driver_handle,
                            &arg, kIoexpIoctlCmdUnregIrqHandler);
  if (err_code == kHalErrCodeOk) {
    ioexp_info->irq_handler      = NULL;
    ioexp_info->irq_private_data = NULL;
  } else {
    LOG_E(0x46, "HalDriverIoctl is failed. err_code=%d, dev_id=%d, pin=%d",
          err_code, arg.device_id, arg.pin);
  }
  return err_code;
}

//-----------------------------------------------------------------------------
HalErrCode HalIoexpUnregisterIrqHandler(const HalIoexpHandle handle) {
  int lock_ret = pthread_mutex_lock(&s_api_mutex);
  if (lock_ret) {
    LOG_E(0x47, "Lock error. errno=%d", errno);
    return kHalErrLock;
  }

  HalErrCode err_code = kHalErrCodeOk;
  if (!s_is_initialized) {
    LOG_E(0x48, "Not initialized.");
    err_code = kHalErrInvalidState;
    goto unlock;
  }

  bool found = IsValidHandle(handle);
  if (!found) {
    LOG_E(0x49, "handle is invalid.");
    err_code = kHalErrInvalidParam;
    goto unlock;
  }

  struct IoexpData *ioexp_info = (struct IoexpData*)handle;
  if (!ioexp_info->is_opened) {
    LOG_E(0x4A, "This handle is not opened. dev_id=%d, pin=%d",
          ioexp_info->device_id, ioexp_info->pin);
    err_code = kHalErrInvalidParam;
    goto unlock;
  }

  // Wait until Irq Handler execution is complite.
#ifdef __NuttX__
  irqstate_t flags = spin_lock_irqsave(&s_irqlock);
#endif

  if (ioexp_info->irq_handler == NULL) {
    LOG_E(0x4B, "This handle is not registered for interrupt.");
    err_code = kHalErrInvalidParam;
    goto irq_unlock;
  }

  if (ioexp_info->dev_info->pin_total_num <= ioexp_info->pin) {
    LOG_E(0x4C, "pin does not exist. pin_total_num=%d, pin=%d",
          ioexp_info->dev_info->pin_total_num, ioexp_info->pin);
    err_code = kHalErrInvalidParam;
    goto irq_unlock;
  }

  err_code = HalIoexpUnregisterIrqHandler_(ioexp_info);
  if (err_code != kHalErrCodeOk) {
    LOG_E(0x4D, "pin does not exist. pin_total_num=%d, pin=%d",
          ioexp_info->dev_info->pin_total_num, ioexp_info->pin);
    goto irq_unlock;
  }

irq_unlock:
#ifdef __NuttX__
  spin_unlock_irqrestore(&s_irqlock, flags);
#endif

unlock:
  lock_ret = pthread_mutex_unlock(&s_api_mutex);
  if (lock_ret) {
    LOG_E(0x4E, "Unlock error. errno=%d", errno);
  }

  return err_code;
}

// Local Functions ------------------------------------------------------------
// ----------------------------------------------------------------------------
static HalErrCode GetIoexpItem(uint32_t ioexp_id, struct IoexpData** item) {
  struct IoexpData *ioexp_entry = NULL, *ioexp_temp = NULL;
  TAILQ_FOREACH_SAFE(ioexp_entry, &s_ioexp_list, head, ioexp_temp) {
    if (ioexp_entry->ioexp_id == ioexp_id) {
      *item = ioexp_entry;
      return kHalErrCodeOk;
    }
  }
  item = NULL;
  return kHalErrNotFound;
}

// ----------------------------------------------------------------------------
static void DeleteAllDevInfo(void) {
  if (s_dev_info != NULL) {
    for (uint32_t device_id = 0; device_id < s_dev_total_num; device_id++) {
      HalErrCode err_code = HalDriverClose(s_dev_info[device_id].driver_handle);
      if (err_code != kHalErrCodeOk) {
        LOG_E(0x4F, "HalDriverClose is failed. err_code=%d, device_id=%d",
              err_code, device_id);
      }
    }
    free(s_dev_info);
    s_dev_info = NULL;
  }
}

// ----------------------------------------------------------------------------
static void DeleteAllIoexpList(void) {
  struct IoexpData *ioexp_entry = NULL, *ioexp_temp = NULL;
  TAILQ_FOREACH_SAFE(ioexp_entry, &s_ioexp_list, head, ioexp_temp) {
    TAILQ_REMOVE(&s_ioexp_list, ioexp_entry, head);
    free(ioexp_entry);
  }
}

// ----------------------------------------------------------------------------
static bool IsValidHandle(HalIoexpHandle handle_) {
  if (handle_ == NULL) {
    return false;
  }

  struct IoexpData *handle = (struct IoexpData *)handle_;
  struct IoexpData *ioexp_entry = NULL, *ioexp_temp = NULL;
  TAILQ_FOREACH_SAFE(ioexp_entry, &s_ioexp_list, head, ioexp_temp) {
    if (ioexp_entry == handle) {
      return true;
    }
  }
  return false;
}

// ----------------------------------------------------------------------------
static HalErrCode InitDeviceInfoAndList(void) {
  HalErrCode err_code = kHalErrCodeOk;
  bool dev_ids[CONFIG_EXTERNAL_HAL_IOEXP_NUM] = {false};
  uint32_t count = 0, ioexp_id = 0;
  s_dev_total_num = 0;
  for (ioexp_id = 0; ioexp_id < CONFIG_EXTERNAL_HAL_IOEXP_NUM; ioexp_id++) {
    uint32_t device_id = 0, pin = 0, irq_num = 0;
    bool reverse = false;
    err_code = HalIoexpGetDataImpl(ioexp_id, &device_id,
                                   &pin, &irq_num, &reverse);
    if (err_code != kHalErrCodeOk) {
      LOG_E(0x53, "HalIoexpGetDataImpl() error=%u\n", err_code);
      goto err;
    }
    if (dev_ids[device_id] == false) {
      dev_ids[device_id] = true;
      count++;
    }

    struct IoexpData *ioexp_data = malloc(sizeof(struct IoexpData));
    if (ioexp_data == NULL) {
      LOG_E(0x54, "malloc error. errno=%d", errno);
      err_code = kHalErrMemory;
      goto err;
    }

    ioexp_data->ioexp_id         = ioexp_id;
    ioexp_data->device_id        = device_id;
    ioexp_data->pin              = pin;
    ioexp_data->reverse          = reverse;
    ioexp_data->irq_num          = irq_num;
    ioexp_data->irq_handler      = NULL;
    ioexp_data->irq_private_data = NULL;
    ioexp_data->is_opened        = false;
    ioexp_data->dev_info         = NULL;

    TAILQ_INSERT_TAIL(&s_ioexp_list, ioexp_data, head);
  }
  s_dev_total_num = count;

  s_dev_info = malloc(s_dev_total_num * sizeof(struct DeviceInfo));
  if (s_dev_info == NULL) {
    LOG_E(0x50, "malloc error. errno=%d", errno);
    err_code = kHalErrMemory;
    goto err;
  }
  for (uint32_t device_id = 0; device_id < s_dev_total_num; device_id++) {
    HalDriverHandle handle = 0;
    err_code = HalDriverOpen(device_id, NULL, &handle);
    if (err_code == kHalErrCodeOk) {
      s_dev_info[device_id].driver_handle = handle;

      struct IoexpIoctlArg arg = {0};
      arg.device_id = device_id;
      err_code = HalDriverIoctl(handle, &arg, kIoexpIoctlCmdGetPinTotalNum);
      if (err_code != kHalErrCodeOk) {
        LOG_E(0x51, "HalDriverIoctl is failed. err_code=%d, dev_id=%d",
              err_code, device_id);
        goto err;
      }
      s_dev_info[device_id].pin_total_num = arg.total_pin_num;
    } else {
      LOG_E(0x52, "HalDriverOpen is failed. err_code=%u", err_code);
      goto err;
    }
  }

  // Update ioexp_data->dev_info
  struct IoexpData *ioexp_entry = NULL, *ioexp_temp = NULL;
  TAILQ_FOREACH_SAFE(ioexp_entry, &s_ioexp_list, head, ioexp_temp) {
    ioexp_entry->dev_info = &s_dev_info[ioexp_entry->device_id];
  }

  return kHalErrCodeOk;

err:
  DeleteAllDevInfo();    // s_dev_info is freed in this func.
  DeleteAllIoexpList();  // all ioexp_data are freed in this func.
  return err_code;
}

// ----------------------------------------------------------------------------
