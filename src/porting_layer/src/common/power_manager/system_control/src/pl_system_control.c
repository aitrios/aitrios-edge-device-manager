/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

// Includes --------------------------------------------------------------------
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <pthread.h>

#include "pl_system_control.h"
#include "pl_system_control_os_impl.h"
#include "pl.h"

#include "utility_log_module_id.h"
#include "utility_log.h"

// Macros ----------------------------------------------------------------------
#define EVENT_ID  0x9800
#define EVENT_ID_START 0x0000
#define LOG_ERR(event_id, format, ...) \
  WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, \
    "%s-%d:" format, __FILE__, __LINE__, ##__VA_ARGS__); \
  WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (EVENT_ID | \
                  (0x00FF & (EVENT_ID_START + event_id))));

// Typedefs --------------------------------------------------------------------

// External functions ----------------------------------------------------------

// Local functions -------------------------------------------------------------

// Global Variables ------------------------------------------------------------
static bool s_is_initialized = false;
static pthread_mutex_t s_mutex = PTHREAD_MUTEX_INITIALIZER;

// Functions -------------------------------------------------------------------
PlErrCode PlSystemCtlInitialize(void) {
  int lock_ret = pthread_mutex_lock(&s_mutex);
  if (lock_ret != 0) {
    LOG_ERR(0x00, "Lock error. errno=%d\n", errno);
    return kPlErrLock;
  }

  // nothing to do.
  s_is_initialized = true;

  lock_ret = pthread_mutex_unlock(&s_mutex);
  if (lock_ret != 0) {
    LOG_ERR(0x01, "Unlock error. errno=%d\n", errno);
  }

  return kPlErrCodeOk;
}
PlErrCode PlSystemCtlFinalize(void) {
  int lock_ret = pthread_mutex_lock(&s_mutex);
  if (lock_ret != 0) {
    LOG_ERR(0x02, "Lock error. errno=%d\n", errno);
    return kPlErrLock;
  }

  // nothing to do.
  s_is_initialized = false;

  lock_ret = pthread_mutex_unlock(&s_mutex);
  if (lock_ret != 0) {
    LOG_ERR(0x03, "Unlock error. errno=%d\n", errno);
  }

  return kPlErrCodeOk;
}

// -----------------------------------------------------------------------------
PlErrCode PlSystemCtlExecOperation(PlSystemCtlOperation operation) {
  // In the case of reboot, whether PllInitialize() has run or not is ignored.
  PlErrCode ret_code = kPlErrCodeOk;
  int lock_ret = pthread_mutex_lock(&s_mutex);
  if (lock_ret != 0) {
    LOG_ERR(0x04, "Lock error. errno=%d\n", errno);
    return kPlErrLock;
  }

  if (((operation != kPlSystemCtlRebootCpu) &&
       (operation != kPlSystemCtlRebootEdgeDevice)) &&
      (!s_is_initialized)) {
    LOG_ERR(0x05, "State error. operation=%d, s_is_initialized=%d",
          operation, s_is_initialized);
    ret_code = kPlErrInvalidState;
    goto unlock;
  }

  switch (operation) {
    case kPlSystemCtlRebootCpu:
      ret_code = PlSystemCtlRebootCpuOsImpl();
      goto unlock;
    case kPlSystemCtlRebootEdgeDevice:
      ret_code = PlSystemCtlRebootEdgeDeviceOsImpl();
      goto unlock;
    case kPlSystemCtlPowerOff:
      // T.B.D.
    default:
      LOG_ERR(0x06, "Parameter error(%d).", operation);
      ret_code = kPlErrInvalidParam;
      goto unlock;
  }

unlock:
  lock_ret = pthread_mutex_unlock(&s_mutex);
  if (lock_ret != 0) {
    LOG_ERR(0x07, "Unlock error. errno=%d\n", errno);
  }

  return ret_code;
}

// -----------------------------------------------------------------------------
PlErrCode PlSystemCtlGetResetCause(PlSystemCtlResetCause *cause) {
  return PlSystemCtlGetResetCauseOsImpl(cause);
}

// -----------------------------------------------------------------------------
PlErrCode PlSystemCtlGetExceptionInfo(
            struct PlSystemCtlExceptionInfo *info) {
  return PlSystemCtlGetExceptionInfoOsImpl(info);
}

// -----------------------------------------------------------------------------
PlErrCode PlSystemCtlSetExceptionInfo(void) {
  return PlSystemCtlSetExceptionInfoOsImpl();
}

// -----------------------------------------------------------------------------
PlErrCode PlSystemCtlConvExceptionInfo(
            struct PlSystemCtlExceptionInfo *info,
            char *dst, uint32_t dst_size) {
  return PlSystemCtlConvExceptionInfoOsImpl(info, dst, dst_size);
}

// -----------------------------------------------------------------------------
PlErrCode PlSystemCtlClearExceptionInfo(void) {
  return PlSystemCtlClearExceptionInfoOsImpl();
}

// -----------------------------------------------------------------------------
PlErrCode PlSystemCtlDumpAllStack(void) {
  return PlSystemCtlDumpAllStackOsImpl();
}

// -----------------------------------------------------------------------------
