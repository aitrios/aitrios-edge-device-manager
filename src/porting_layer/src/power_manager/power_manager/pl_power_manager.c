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

// Local functions ------------------------------------------------------------

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
