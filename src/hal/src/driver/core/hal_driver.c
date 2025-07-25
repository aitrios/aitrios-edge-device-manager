/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#ifdef __linux__
#include <bsd/sys/queue.h>
#endif
#ifdef __NuttX__
#include <sys/queue.h>
#endif
#include <errno.h>

#include "hal.h"
#include "hal_driver.h"
#include "hal_driver_impl.h"
#include "utility_log.h"
#include "utility_log_module_id.h"

// Macros ---------------------------------------------------------------------
#define EVENT_ID        (0x9A00)
#define EVENT_ID_START  (0x00)
#define LOG_E(event_id, format, ...) \
  WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:" format, \
                   __FILE__, __LINE__, ##__VA_ARGS__); \
  WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (EVENT_ID | (EVENT_ID_START + event_id)));

#define LOG_ERR_WITH_ID(event_id, format, ...) \
  WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:" format, \
                   __FILE__, __LINE__, ##__VA_ARGS__); \
  WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (EVENT_ID | (EVENT_ID_START + event_id)));

#define EVENT_UID_START (0x25)
#define HAL_DRIVER_ELOG_LOCK_ERROR    (EVENT_UID_START + 0x00)
#define HAL_DRIVER_ELOG_UNLOCK_ERROR  (EVENT_UID_START + 0x01)

TAILQ_HEAD(DriverHandleList, HandleList);

struct HandleList {
  TAILQ_ENTRY(HandleList) head;
  uint32_t device_id;
  struct HalDriverOps *ops;
  pthread_mutex_t *mutex;
};

// Global Variables -----------------------------------------------------------
static struct DriverHandleList s_handle_list = {0};
static struct DriverInfoList s_drivers_list  = {0};
static pthread_mutex_t s_api_mutex  = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t s_list_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool s_is_initialized = false;
// Local functions ------------------------------------------------------------
static HalErrCode HalDriverAddDriver_(uint32_t device_id, const char *name,
                                      const struct HalDriverOps *ops);
// Functions ------------------------------------------------------------------
HalErrCode HalDriverOpen(uint32_t device_id, void *arg,
                          HalDriverHandle *handle) {
  struct HandleList *entry_item = NULL, *entry_temp = NULL;
  (void)arg;  // Avoid compiler warning
  int ret = pthread_mutex_lock(&s_api_mutex);
  if (ret) {
    LOG_ERR_WITH_ID(HAL_DRIVER_ELOG_LOCK_ERROR,
                "pthread_mutex_lock() failed.(ret=%d, errno=%d)", ret, errno);
    return kHalErrLock;
  }

  HalErrCode ret_code = kHalErrCodeOk;
  if (!s_is_initialized) {
    LOG_E(0x00, "Not initialized.");
    ret_code = kHalErrInvalidState;
    goto func_exit;
  }
  if (handle == NULL) {
    LOG_E(0x01, "argument(handle) NULL.");
    ret_code = kHalErrInvalidParam;
    goto func_exit;
  }

  bool found = false;
  struct HalDriverInfo *info = NULL;
  struct HalDriverInfo *entry = NULL, *temp = NULL;
  TAILQ_FOREACH_SAFE(entry, &s_drivers_list, head, temp) {
    if (((struct HalDriverInfo *)entry)->device_id == device_id) {
      found = true;
      info = (struct HalDriverInfo *)entry;
    }
  }
  if (!found) {
    LOG_E(0x02, "device not found. device_id=%d", device_id);
    ret_code = kHalErrNotFound;
    goto func_exit;
  }

  if (info->ops->open == NULL) {
    LOG_E(0x03, "info->ops->open is NULL.");
    ret_code = kHalErrNoSupported;
    goto func_exit;
  }

  struct HandleList *item = malloc(sizeof(struct HandleList));
  if (item == NULL) {
    LOG_E(0x04, "Failed to malloc. errno=%d", errno);
    ret_code = kHalErrMemory;
    goto func_exit;
  }
  pthread_mutex_t *mutex = malloc(sizeof(pthread_mutex_t));
  if (mutex == NULL) {
    free(item);
    LOG_E(0x05, "Failed to malloc. errno=%d", errno);
    ret_code = kHalErrMemory;
    goto func_exit;
  }

  ret = pthread_mutex_init(mutex, NULL);
  if (ret) {
    LOG_E(0x06, "pthread_mutex_init() failed.(ret=%d, errno=%d)", ret, errno);
    free(mutex);
    free(item);
    ret_code = kHalErrInternal;
    goto func_exit;
  }

  item->device_id = device_id;
  item->ops = info->ops;
  item->mutex = mutex;

  ret = pthread_mutex_lock(&s_list_mutex);
  if (ret) {
    LOG_ERR_WITH_ID(HAL_DRIVER_ELOG_LOCK_ERROR,
                "pthread_mutex_lock() failed.(ret=%d, errno=%d)", ret, errno);
    ret_code = kHalErrLock;
    free(mutex);
    free(item);
    goto func_exit;
  }

  TAILQ_INSERT_TAIL(&s_handle_list, item, head);

  ret = pthread_mutex_unlock(&s_list_mutex);
  if (ret) {
    LOG_ERR_WITH_ID(HAL_DRIVER_ELOG_UNLOCK_ERROR,
                "pthread_mutex_unlock() failed.(ret=%d, errno=%d)", ret, errno);
    ret_code = kHalErrUnlock;
    goto err_recovery;
  }

  ret = pthread_mutex_lock(item->mutex);
  if (ret) {
    LOG_ERR_WITH_ID(HAL_DRIVER_ELOG_LOCK_ERROR,
                "pthread_mutex_lock() failed.(ret=%d, errno=%d)", ret, errno);
    ret_code = kHalErrLock;
    goto err_recovery;
  }
  ret_code = item->ops->open(device_id);
  ret = pthread_mutex_unlock(item->mutex);
  if (ret_code != kHalErrCodeOk) {
    LOG_E(0x08, "open failed. ret_code=%u, device_id=%u", ret_code, device_id);
    goto err_recovery;
  }
  if (ret) {
    LOG_ERR_WITH_ID(HAL_DRIVER_ELOG_UNLOCK_ERROR,
                "pthread_mutex_unlock() failed.(ret=%d, errno=%d)", ret, errno);
  }

  *handle = (HalDriverHandle)item;
  goto func_exit;

err_recovery:
  // Recovering from an error, remove entry and free related memory.
  TAILQ_FOREACH_SAFE(entry_item, &s_handle_list, head, entry_temp) {
    if (item == entry_item) {
      TAILQ_REMOVE(&s_handle_list, entry_item, head);
      pthread_mutex_destroy(item->mutex);
      free(item->mutex);
      free(item);
      break;
    }
  }

func_exit:
  ret = pthread_mutex_unlock(&s_api_mutex);
  if (ret) {
    LOG_ERR_WITH_ID(HAL_DRIVER_ELOG_UNLOCK_ERROR,
                "pthread_mutex_unlock() failed.(ret=%d, errno=%d)", ret, errno);
  }
  return ret_code;
}
// ----------------------------------------------------------------------------
HalErrCode HalDriverClose(HalDriverHandle handle) {
  int ret = pthread_mutex_lock(&s_api_mutex);
  if (ret) {
    LOG_ERR_WITH_ID(HAL_DRIVER_ELOG_LOCK_ERROR,
                "pthread_mutex_lock() failed.(ret=%d, errno=%d)", ret, errno);
    return kHalErrLock;
  }

  bool found = false;
  HalErrCode ret_code = kHalErrCodeError;
  if (!s_is_initialized) {
    LOG_E(0x09, "Not initialized.");
    ret_code = kHalErrInvalidState;
    goto func_exit;
  }

  ret = pthread_mutex_lock(&s_list_mutex);
  if (ret) {
    LOG_ERR_WITH_ID(HAL_DRIVER_ELOG_LOCK_ERROR,
                "pthread_mutex_lock() failed.(ret=%d, errno=%d)", ret, errno);
    ret_code = kHalErrLock;
    goto func_exit;
  }

  struct HandleList *entry = NULL, *temp = NULL;
  TAILQ_FOREACH_SAFE(entry, &s_handle_list, head, temp) {
    if (handle == (HalDriverHandle)entry) {
      if (((struct HandleList *)entry)->ops->close == NULL) {
        LOG_E(0x0A, "entry->ops->close is NULL");
        ret_code = kHalErrNoSupported;
        goto release_list_mutex;
      }

      ret_code = ((struct HandleList *)entry)->ops->close(
                  ((struct HandleList *)entry)->device_id);
      if (ret_code != kHalErrCodeOk) {
        LOG_E(0x0B, "close failed. ret_code=%u, device_id=%u",
                             ret_code, entry->device_id);
        break;
      }
      pthread_mutex_destroy(((struct HandleList *)entry)->mutex);
      free(((struct HandleList *)entry)->mutex);
      TAILQ_REMOVE(&s_handle_list, entry, head);
      free(entry);
      found = true;
      break;
    }
  }

release_list_mutex:
  ret = pthread_mutex_unlock(&s_list_mutex);
  if (ret) {
    LOG_ERR_WITH_ID(HAL_DRIVER_ELOG_UNLOCK_ERROR,
                "pthread_mutex_unlock() failed.(ret=%d, errno=%d)", ret, errno);
  }

func_exit:
  ret = pthread_mutex_unlock(&s_api_mutex);
  if (ret) {
    LOG_ERR_WITH_ID(HAL_DRIVER_ELOG_UNLOCK_ERROR,
                "pthread_mutex_unlock() failed.(ret=%d, errno=%d)", ret, errno);
  }
  return found ? kHalErrCodeOk : ret_code;
}
// ----------------------------------------------------------------------------
HalErrCode HalDriverRead(HalDriverHandle handle, void *buf,
                         uint32_t size, uint32_t *read_size) {
  int ret = pthread_mutex_lock(&s_api_mutex);
  if (ret) {
    LOG_ERR_WITH_ID(HAL_DRIVER_ELOG_LOCK_ERROR,
                "pthread_mutex_lock() failed.(ret=%d, errno=%d)", ret, errno);
    return kHalErrLock;
  }

  bool found = false;
  HalErrCode ret_code = kHalErrCodeError;
  if (!s_is_initialized) {
    LOG_E(0x0C, "Not initialized.");
    ret_code = kHalErrInvalidState;
    goto func_exit;
  }
  if ((buf == NULL) || (read_size == NULL)) {
    LOG_E(0x0D, "argument NULL.");
    ret_code = kHalErrInvalidParam;
    goto func_exit;
  }

  ret = pthread_mutex_lock(&s_list_mutex);
  if (ret) {
    LOG_ERR_WITH_ID(HAL_DRIVER_ELOG_LOCK_ERROR,
                "pthread_mutex_lock() failed.(ret=%d, errno=%d)", ret, errno);
    ret_code = kHalErrLock;
    goto func_exit;
  }

  struct HandleList *driver = NULL, *entry = NULL, *temp = NULL;
  TAILQ_FOREACH_SAFE(entry, &s_handle_list, head, temp) {
    if (handle == (HalDriverHandle)entry) {
      driver = (struct HandleList*)entry;
      found = true;
      break;
    }
  }

  ret = pthread_mutex_unlock(&s_list_mutex);
  if (ret) {
    LOG_ERR_WITH_ID(HAL_DRIVER_ELOG_UNLOCK_ERROR,
                "pthread_mutex_unlock() failed.(ret=%d, errno=%d)", ret, errno);
    ret_code = kHalErrUnlock;
    goto func_exit;
  }

  if (!found) {
    LOG_E(0x0E, "handle not found.");
    ret_code = kHalErrNotFound;
    goto func_exit;
  }

  if (driver->ops->read == NULL) {
    LOG_E(0x0F, "driver->ops->read is NULL");
    ret_code = kHalErrNoSupported;
    goto func_exit;
  }
  ret = pthread_mutex_lock(driver->mutex);
  if (ret) {
    LOG_ERR_WITH_ID(HAL_DRIVER_ELOG_LOCK_ERROR,
                "pthread_mutex_lock() failed.(ret=%d, errno=%d)", ret, errno);
    ret_code = kHalErrLock;
    goto func_exit;
  }
  ret_code = driver->ops->read(buf, size, read_size);
  ret = pthread_mutex_unlock(driver->mutex);
  if (ret) {
    LOG_ERR_WITH_ID(HAL_DRIVER_ELOG_UNLOCK_ERROR,
                "pthread_mutex_unlock() failed.(ret=%d, errno=%d)", ret, errno);
  }
  if (ret_code != kHalErrCodeOk) {
    LOG_E(0x10, "read failed. ret_code=%u, size=%u, read_size=%u",
                                  ret_code, size, *read_size);
  }

func_exit:
  ret = pthread_mutex_unlock(&s_api_mutex);
  if (ret) {
    LOG_ERR_WITH_ID(HAL_DRIVER_ELOG_UNLOCK_ERROR,
                "pthread_mutex_unlock() failed.(ret=%d, errno=%d)", ret, errno);
  }
  return ret_code;
}
// ----------------------------------------------------------------------------
HalErrCode HalDriverWrite(HalDriverHandle handle, const void *buf,
                          uint32_t size, uint32_t *written_size) {
  int ret = pthread_mutex_lock(&s_api_mutex);
  if (ret) {
    LOG_ERR_WITH_ID(HAL_DRIVER_ELOG_LOCK_ERROR,
                "pthread_mutex_lock() failed.(ret=%d, errno=%d)", ret, errno);
    return kHalErrLock;
  }

  bool found = false;
  HalErrCode ret_code = kHalErrCodeError;
  if (!s_is_initialized) {
    LOG_E(0x11, "Not initialized.");
    ret_code = kHalErrInvalidState;
    goto func_exit;
  }
  if ((buf == NULL) || (written_size == NULL)) {
    LOG_E(0x12, "argument NULL.");
    ret_code = kHalErrInvalidParam;
    goto func_exit;
  }

  ret = pthread_mutex_lock(&s_list_mutex);
  if (ret) {
    LOG_ERR_WITH_ID(HAL_DRIVER_ELOG_LOCK_ERROR,
                "pthread_mutex_lock() failed.(ret=%d, errno=%d)", ret, errno);
    ret_code = kHalErrLock;
    goto func_exit;
  }

  struct HandleList *driver = NULL, *entry = NULL, *temp = NULL;
  TAILQ_FOREACH_SAFE(entry, &s_handle_list, head, temp) {
    if (handle == (HalDriverHandle)entry) {
      driver = (struct HandleList*)entry;
      found = true;
      break;
    }
  }
  ret = pthread_mutex_unlock(&s_list_mutex);
  if (ret) {
    LOG_ERR_WITH_ID(HAL_DRIVER_ELOG_UNLOCK_ERROR,
                "pthread_mutex_unlock() failed.(ret=%d, errno=%d)", ret, errno);
    ret_code = kHalErrUnlock;
    goto func_exit;
  }

  if (!found) {
    LOG_E(0x13, "handle not found.");
    ret_code = kHalErrNotFound;
    goto func_exit;
  }

  if (driver->ops->write == NULL) {
    LOG_E(0x14, "driver->ops->write is NULL");
    ret_code = kHalErrNoSupported;
    goto func_exit;
  }
  ret = pthread_mutex_lock(driver->mutex);
  if (ret) {
    LOG_ERR_WITH_ID(HAL_DRIVER_ELOG_LOCK_ERROR,
                "pthread_mutex_lock() failed.(ret=%d, errno=%d)", ret, errno);
    ret_code = kHalErrLock;
    goto func_exit;
  }
  ret_code = driver->ops->write(buf, size, written_size);
  ret = pthread_mutex_unlock(driver->mutex);
  if (ret) {
    LOG_ERR_WITH_ID(HAL_DRIVER_ELOG_UNLOCK_ERROR,
                "pthread_mutex_unlock() failed.(ret=%d, errno=%d)", ret, errno);
  }
  if (ret_code != kHalErrCodeOk) {
    LOG_E(0x15, "write failed. ret_code=%u, size=%u, written_size=%u",
                         ret_code, size, *written_size);
  }

func_exit:
  ret = pthread_mutex_unlock(&s_api_mutex);
  if (ret) {
    LOG_ERR_WITH_ID(HAL_DRIVER_ELOG_UNLOCK_ERROR,
                "pthread_mutex_unlock() failed.(ret=%d, errno=%d)", ret, errno);
  }
  return ret_code;
}
// ----------------------------------------------------------------------------
HalErrCode HalDriverIoctl(HalDriverHandle handle, void *arg, uint32_t cmd) {
  int ret = pthread_mutex_lock(&s_api_mutex);
  if (ret) {
    LOG_ERR_WITH_ID(HAL_DRIVER_ELOG_LOCK_ERROR,
                "pthread_mutex_lock() failed.(ret=%d, errno=%d)", ret, errno);
    return kHalErrLock;
  }

  bool found = false;
  HalErrCode ret_code = kHalErrCodeError;
  if (!s_is_initialized) {
    LOG_E(0x16, "Not initialized.");
    ret_code = kHalErrInvalidState;
    goto func_exit;
  }
  if (arg == NULL) {
    LOG_E(0x17, "argument(arg) NULL.");
    ret_code = kHalErrInvalidParam;
    goto func_exit;
  }

  ret = pthread_mutex_lock(&s_list_mutex);
  if (ret) {
    LOG_ERR_WITH_ID(HAL_DRIVER_ELOG_LOCK_ERROR,
                "pthread_mutex_lock() failed.(ret=%d, errno=%d)", ret, errno);
    ret_code = kHalErrLock;
    goto func_exit;
  }
  struct HandleList *driver = NULL, *entry = NULL, *temp = NULL;
  TAILQ_FOREACH_SAFE(entry, &s_handle_list, head, temp) {
    if (handle == (HalDriverHandle)entry) {
      driver = (struct HandleList*)entry;
      found = true;
      break;
    }
  }
  ret = pthread_mutex_unlock(&s_list_mutex);
  if (ret) {
    LOG_ERR_WITH_ID(HAL_DRIVER_ELOG_UNLOCK_ERROR,
                "pthread_mutex_unlock() failed.(ret=%d, errno=%d)", ret, errno);
    ret_code = kHalErrUnlock;
    goto func_exit;
  }

  if (!found) {
    LOG_E(0x18, "handle not found.");
    ret_code = kHalErrNotFound;
    goto func_exit;
  }

  if (driver->ops->ioctl == NULL) {
    LOG_E(0x19, "driver->ops->ioctl is NULL");
    ret_code = kHalErrNoSupported;
    goto func_exit;
  }
  ret = pthread_mutex_lock(driver->mutex);
  if (ret) {
    LOG_ERR_WITH_ID(HAL_DRIVER_ELOG_LOCK_ERROR,
                "pthread_mutex_lock() failed.(ret=%d, errno=%d)", ret, errno);
    ret_code = kHalErrLock;
    goto func_exit;
  }
  ret_code = driver->ops->ioctl(arg, cmd);
  ret = pthread_mutex_unlock(driver->mutex);
  if (ret) {
    LOG_ERR_WITH_ID(HAL_DRIVER_ELOG_UNLOCK_ERROR,
                "pthread_mutex_unlock() failed.(ret=%d, errno=%d)", ret, errno);
  }
  if (ret_code != kHalErrCodeOk) {
    LOG_E(0x1A, "ioctl failed. ret_code=%u, cmd=%u", ret_code, cmd);
  }

func_exit:
  ret = pthread_mutex_unlock(&s_api_mutex);
  if (ret) {
    LOG_ERR_WITH_ID(HAL_DRIVER_ELOG_UNLOCK_ERROR,
                "pthread_mutex_unlock() failed.(ret=%d, errno=%d)", ret, errno);
  }
  return ret_code;
}
// ----------------------------------------------------------------------------
HalErrCode HalDriverInitialize(void) {
  int ret = pthread_mutex_lock(&s_api_mutex);
  if (ret) {
    LOG_ERR_WITH_ID(HAL_DRIVER_ELOG_LOCK_ERROR,
                "pthread_mutex_lock() failed.(ret=%d, errno=%d)", ret, errno);
    return kHalErrLock;
  }
  HalErrCode ret_code = kHalErrCodeOk;
  if (s_is_initialized) {
    LOG_E(0x1B, "Already initialized.");
    ret_code = kHalErrInvalidState;
    goto func_exit;
  }

  ret = pthread_mutex_lock(&s_list_mutex);
  if (ret) {
    LOG_ERR_WITH_ID(HAL_DRIVER_ELOG_LOCK_ERROR,
                "pthread_mutex_lock() failed.(ret=%d, errno=%d)", ret, errno);
    ret_code = kHalErrLock;
    goto func_exit;
  }
  TAILQ_INIT(&s_handle_list);
  TAILQ_INIT(&s_drivers_list);
  ret = pthread_mutex_unlock(&s_list_mutex);
  if (ret) {
    LOG_ERR_WITH_ID(HAL_DRIVER_ELOG_UNLOCK_ERROR,
                "pthread_mutex_unlock() failed.(ret=%d, errno=%d)", ret, errno);
    ret_code = kHalErrUnlock;
    goto func_exit;
  }

  ret_code = HalDriverInitializeImpl();
  if (ret_code != kHalErrCodeOk) {
    LOG_E(0x1E, "HalDriverInitializeImpl() failed. ret_code=%u", ret_code);
    goto func_exit;
  }

  struct DriverInfoList *init_drivers_list = NULL;

  ret_code = HalDriverGetInitInfoImpl(&init_drivers_list);
  if (ret_code != kHalErrCodeOk) {
    LOG_E(0x1F, "HalDriverGetInitInfoImpl() failed. ret_code=%u", ret_code);
    HalDriverFinalizeImpl();
    goto func_exit;
  }

  struct HalDriverInfo *entry = NULL, *temp = NULL;
  TAILQ_FOREACH_SAFE(entry, init_drivers_list, head, temp) {
    struct HalDriverInfo *info = (struct HalDriverInfo *)entry;
    HalDriverAddDriver_(info->device_id, info->name, info->ops);
  }

  s_is_initialized = true;

func_exit:
  ret = pthread_mutex_unlock(&s_api_mutex);
  if (ret) {
    LOG_ERR_WITH_ID(HAL_DRIVER_ELOG_UNLOCK_ERROR,
                "pthread_mutex_unlock() failed.(ret=%d, errno=%d)", ret, errno);
  }
  return ret_code;
}
// ----------------------------------------------------------------------------
HalErrCode HalDriverFinalize(void) {
  int ret = pthread_mutex_lock(&s_api_mutex);
  if (ret) {
    LOG_ERR_WITH_ID(HAL_DRIVER_ELOG_LOCK_ERROR,
                "pthread_mutex_lock() failed.(ret=%d, errno=%d)", ret, errno);
    return kHalErrLock;
  }

  HalErrCode ret_code = kHalErrCodeOk;
  if (!s_is_initialized) {
    LOG_E(0x20, "Not initialized.");
    ret_code = kHalErrInvalidState;
    goto func_exit;
  }

  ret = pthread_mutex_lock(&s_list_mutex);
  if (ret) {
    LOG_ERR_WITH_ID(HAL_DRIVER_ELOG_LOCK_ERROR,
                "pthread_mutex_lock() failed.(ret=%d, errno=%d)", ret, errno);
    ret_code = kHalErrLock;
    goto func_exit;
  }
  struct HandleList *list = NULL, *entry = NULL, *temp = NULL;
  TAILQ_FOREACH_SAFE(entry, &s_handle_list, head, temp) {
    list = (struct HandleList*)entry;
    pthread_mutex_destroy(list->mutex);
    free(list->mutex);
    TAILQ_REMOVE(&s_handle_list, entry, head);
    free(entry);
  }
  struct HalDriverInfo *driver_entry = NULL, *driver_temp = NULL;
  TAILQ_FOREACH_SAFE(driver_entry, &s_drivers_list, head, driver_temp) {
    TAILQ_REMOVE(&s_drivers_list, driver_entry, head);
    free(driver_entry);
  }
  ret = pthread_mutex_unlock(&s_list_mutex);
  if (ret) {
    LOG_ERR_WITH_ID(HAL_DRIVER_ELOG_UNLOCK_ERROR,
                "pthread_mutex_unlock() failed.(ret=%d, errno=%d)", ret, errno);
    ret_code = kHalErrUnlock;
    goto func_exit;
  }

  ret_code = HalDriverFinalizeImpl();

  s_is_initialized = false;

func_exit:
  ret = pthread_mutex_unlock(&s_api_mutex);
  if (ret) {
    LOG_ERR_WITH_ID(HAL_DRIVER_ELOG_UNLOCK_ERROR,
                "pthread_mutex_unlock() failed.(ret=%d, errno=%d)", ret, errno);
  }
  return ret_code;
}

static HalErrCode HalDriverAddDriver_(uint32_t device_id, const char *name,
                                      const struct HalDriverOps *ops) {
  int ret = 0;
  ret = pthread_mutex_lock(&s_list_mutex);
  if (ret) {
    LOG_ERR_WITH_ID(HAL_DRIVER_ELOG_LOCK_ERROR,
                "pthread_mutex_lock() failed.(ret=%d, errno=%d)", ret, errno);
    return kHalErrLock;
  }
  HalErrCode ret_code = kHalErrCodeOk;
  struct HalDriverInfo *entry = NULL, *temp = NULL;
  TAILQ_FOREACH_SAFE(entry, &s_drivers_list, head, temp) {
    if (device_id == ((struct HalDriverInfo *)entry)->device_id) {
      LOG_E(0x21, "same device_id already registered:%u", device_id);
      ret_code = kHalErrInvalidParam;
      goto func_exit;
    }
  }
  struct HalDriverInfo *info = malloc(sizeof(struct HalDriverInfo));
  if (info == NULL) {
    LOG_E(0x22, "Failed to malloc. errno=%d", errno);
    ret_code = kHalErrMemory;
    goto func_exit;
  }
  info->device_id = device_id;
  strncpy(info->name, name, DRIVER_NAME_LEN);
  info->ops = (struct HalDriverOps*)ops;
  TAILQ_INSERT_TAIL(&s_drivers_list, info, head);

func_exit:
  ret = pthread_mutex_unlock(&s_list_mutex);
  if (ret) {
    LOG_ERR_WITH_ID(HAL_DRIVER_ELOG_UNLOCK_ERROR,
                "pthread_mutex_unlock() failed.(ret=%d, errno=%d)", ret, errno);
  }
  return ret_code;
}
// ----------------------------------------------------------------------------
HalErrCode HalDriverAddDriver(uint32_t device_id, const char *name,
                              const struct HalDriverOps *ops) {
  int ret = pthread_mutex_lock(&s_api_mutex);
  if (ret) {
    LOG_ERR_WITH_ID(HAL_DRIVER_ELOG_LOCK_ERROR,
                "pthread_mutex_lock() failed.(ret=%d, errno=%d)", ret, errno);
    return kHalErrLock;
  }
  HalErrCode ret_code = kHalErrCodeOk;
  if (!s_is_initialized) {
    LOG_E(0x23, "Not initialized.");
    ret_code = kHalErrInvalidState;
    goto func_exit;
  }
  ret_code = HalDriverAddDriver_(device_id, name, ops);
  if (ret_code != kHalErrCodeOk) {
    LOG_E(0x24, "HalDriverAddDriver_() failed. device_id=%u", device_id);
  }

func_exit:
  ret = pthread_mutex_unlock(&s_api_mutex);
  if (ret) {
    LOG_ERR_WITH_ID(HAL_DRIVER_ELOG_UNLOCK_ERROR,
                "pthread_mutex_unlock() failed.(ret=%d, errno=%d)", ret, errno);
  }
  return ret_code;
}
// ----------------------------------------------------------------------------
