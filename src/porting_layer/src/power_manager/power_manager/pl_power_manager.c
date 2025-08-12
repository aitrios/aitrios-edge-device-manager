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
#include <assert.h>


#include "pl_power_manager.h"
#include "pl_power_manager_impl.h"

#include "utility_log_module_id.h"
#include "utility_log.h"
// Macros ----------------------------------------------------------------------
#define EVENT_ID  0x9800
#define EVENT_ID_START 0x0030
#define LOG_ERR(event_id, format, ...) \
  WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, \
    "%s-%d:" format, __FILE__, __LINE__, ##__VA_ARGS__); \
  WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (EVENT_ID | \
                  (0x00FF & (EVENT_ID_START + event_id))));

// Global Variables -----------------------------------------------------------

static pthread_mutex_t s_api_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool s_is_initialized = false;
static pthread_t s_sw_wdt_thread;
static bool s_is_sw_wdt_enable[CONFIG_EXTERNAL_POWER_MANAGER_SW_WDT_ID_NUM];
static struct timespec s_sw_wdt_last_keepalive_time[CONFIG_EXTERNAL_POWER_MANAGER_SW_WDT_ID_NUM];
static bool s_is_exit = false;

// Local functions ------------------------------------------------------------
static void *PlPowerSwWdt(void *arg);

// Functions ------------------------------------------------------------------
PlErrCode PlPowerMgrInitialize(void) {
  PlErrCode err_code = kPlErrCodeOk;
  int ret = 0;

  ret = pthread_mutex_lock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x00, "[ERROR] %s %d\n", __func__, __LINE__);
    err_code = kPlErrLock;
    goto fin;
  }

  if (s_is_initialized) {
    LOG_ERR(0x01, "[ERROR] %s %d\n", __func__, __LINE__);
    err_code = kPlErrInvalidState;
    goto unlock;
  }

  err_code = PlPowerMgrInitializeImpl();
  if (err_code != kPlErrCodeOk) {
    LOG_ERR(0x02, "[ERROR] %s %d\n", __func__, __LINE__);
    goto unlock;
  }

  struct timespec tmp = {0};
  for (int i = 0; i < CONFIG_EXTERNAL_POWER_MANAGER_SW_WDT_ID_NUM; i++) {
    s_is_sw_wdt_enable[i] = false;
    s_sw_wdt_last_keepalive_time[i] = tmp;
  }
  s_is_exit = false;

#ifdef __NuttX__
  pthread_attr_t attr = {0};
  struct sched_param sched_param = {0};
  pthread_attr_init(&attr);
  pthread_attr_getschedparam(&attr, &sched_param);
  sched_param.sched_priority = CONFIG_EXTERNAL_PL_POWER_MGR_SW_WDT_PRIORITY;
  pthread_attr_setschedparam(&attr, &sched_param);

  ret = pthread_create(&s_sw_wdt_thread, &attr, PlPowerSwWdt, NULL);
#else
  ret = pthread_create(&s_sw_wdt_thread, NULL, PlPowerSwWdt, NULL);
#endif
  if (ret != 0) {
    LOG_ERR(0x11, "[ERROR] %s %d\n", __func__, __LINE__);
    goto unlock;
  }
  pthread_setname_np(s_sw_wdt_thread, "PlPowerSwWdt");

  s_is_initialized = true;

unlock:
  ret = pthread_mutex_unlock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x03, "[ERROR] %s %d\n", __func__, __LINE__);
  }

fin:
  return err_code;
}

// ----------------------------------------------------------------------------
PlErrCode PlPowerMgrFinalize(void) {
  PlErrCode err_code = kPlErrCodeOk;
  int ret = 0;

  ret = pthread_mutex_lock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x04, "[ERROR] %s %d\n", __func__, __LINE__);
    err_code = kPlErrLock;
    goto fin;
  }

  if (!s_is_initialized) {
    LOG_ERR(0x05, "[ERROR] %s %d\n", __func__, __LINE__);
    err_code = kPlErrInvalidState;
    goto unlock;
  }
  s_is_exit = true;
  pthread_join(s_sw_wdt_thread, NULL);

  err_code = PlPowerMgrFinalizeImpl();
  if (err_code != kPlErrCodeOk) {
    LOG_ERR(0x06, "[ERROR] %s %d\n", __func__, __LINE__);
    goto unlock;
  }

  s_is_initialized = false;

unlock:
  ret = pthread_mutex_unlock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x07, "[ERROR] %s %d\n", __func__, __LINE__);
  }

fin:
  return err_code;
}

// ----------------------------------------------------------------------------
PlErrCode PlPowerMgrGetSupplyType(PlPowerMgrSupplyType *type) {
  PlErrCode err_code = kPlErrCodeOk;
  int ret = 0;

  ret = pthread_mutex_lock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x08, "[ERROR] %s %d\n", __func__, __LINE__);
    err_code = kPlErrLock;
    goto fin;
  }

  if (!s_is_initialized) {
    LOG_ERR(0x09, "[ERROR] %s %d\n", __func__, __LINE__);
    err_code = kPlErrInvalidState;
    goto unlock;
  }

  if (type == NULL) {
    LOG_ERR(0x0A, "[ERROR] %s %d type is NULL.\n", __func__, __LINE__);
    err_code = kPlErrInvalidParam;
    goto unlock;
  }

  err_code = PlPowerMgrGetSupplyTypeImpl(type);
  if (err_code != kPlErrCodeOk) {
    LOG_ERR(0x0B, "[ERROR] %s %d\n", __func__, __LINE__);
    goto unlock;
  }

unlock:
  ret = pthread_mutex_unlock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x0C, "[ERROR] %s %d\n", __func__, __LINE__);
  }

fin:
  return err_code;
}

// ----------------------------------------------------------------------------
PlErrCode PlPowerMgrSetupUsb(void) {
  PlErrCode err_code = kPlErrCodeOk;
  int ret = 0;

  ret = pthread_mutex_lock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x0D, "[ERROR] %s %d\n", __func__, __LINE__);
    err_code = kPlErrLock;
    goto fin;
  }

  if (!s_is_initialized) {
    LOG_ERR(0x0E, "[ERROR] %s %d\n", __func__, __LINE__);
    err_code = kPlErrInvalidState;
    goto unlock;
  }

  err_code = PlPowerMgrSetupUsbImpl();
  if (err_code != kPlErrCodeOk) {
    LOG_ERR(0x0F, "[ERROR] %s %d\n", __func__, __LINE__);
    goto unlock;
  }

unlock:
  ret = pthread_mutex_unlock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x10, "[ERROR] %s %d\n", __func__, __LINE__);
  }

fin:
  return err_code;
}

// ----------------------------------------------------------------------------
PlErrCode PlPowerMgrSwWdtKeepalive(uint32_t id) {
  PlErrCode pl_ret = kPlErrCodeOk;
  int ret = 0;
  ret = pthread_mutex_lock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x12, "[ERROR] %s %d\n", __func__, __LINE__);
    return kPlErrLock;
  }
  if (s_is_sw_wdt_enable[id] == false) {
    LOG_ERR(0x13, "id=%u wdt not started\n", id);
    pl_ret = kPlErrInvalidParam;
    goto unlock;
  }
  struct timespec now;
  ret = clock_gettime(CLOCK_MONOTONIC, &now);
  if (ret != 0) {
    LOG_ERR(0x14, "Failed to clock_gettime:%d", ret);
  }
  s_sw_wdt_last_keepalive_time[id] = now;

unlock:
  ret = pthread_mutex_unlock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x15, "[ERROR] %s %d\n", __func__, __LINE__);
  }
  return pl_ret;
}

// ----------------------------------------------------------------------------
PlErrCode PlPowerMgrEnableSwWdt(uint32_t id) {
  int ret = 0;
  ret = pthread_mutex_lock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x16, "[ERROR] %s %d\n", __func__, __LINE__);
    return kPlErrLock;
  }
  struct timespec now;
  ret = clock_gettime(CLOCK_MONOTONIC, &now);
  if (ret != 0) {
    LOG_ERR(0x17, "Failed to clock_gettime:%d", ret);
  }
  s_sw_wdt_last_keepalive_time[id] = now;
  s_is_sw_wdt_enable[id] = true;
  ret = pthread_mutex_unlock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x18, "[ERROR] %s %d\n", __func__, __LINE__);
  }
  return kPlErrCodeOk;
}

// ----------------------------------------------------------------------------
PlErrCode PlPowerMgrDisableSwWdt(uint32_t id) {
  int ret = 0;
  ret = pthread_mutex_lock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x19, "[ERROR] %s %d\n", __func__, __LINE__);
    return kPlErrLock;
  }
  s_is_sw_wdt_enable[id] = false;
  ret = pthread_mutex_unlock(&s_api_mutex);
  if (ret) {
    LOG_ERR(0x1A, "[ERROR] %s %d\n", __func__, __LINE__);
  }
  return kPlErrCodeOk;
}

// ----------------------------------------------------------------------------
static void *PlPowerSwWdt(void *arg) {
  while (!s_is_exit) {
    struct timespec now;
    int ret = clock_gettime(CLOCK_MONOTONIC, &now);
    if (ret != 0) {
      LOG_ERR(0x1B, "Failed to clock_gettime:%d", ret);
    }
    for (int id = 0; id < CONFIG_EXTERNAL_POWER_MANAGER_SW_WDT_ID_NUM; id++) {
      if (s_is_sw_wdt_enable[id]) {
        long elapsed = now.tv_sec - s_sw_wdt_last_keepalive_time[id].tv_sec;  //NOLINT
        elapsed += (now.tv_nsec - s_sw_wdt_last_keepalive_time[id].tv_nsec) / 1000000000;

        if (CONFIG_EXTERNAL_POWER_MANAGER_SW_WDT_TIMEOUT_SEC < elapsed) {
          LOG_ERR(0x1C, "SW WDT timeout id:%d", id);
          assert(false); // Restart system with coredump.
        }
      }
    }
    sleep(1);
  }
  return NULL;
}

// ----------------------------------------------------------------------------
