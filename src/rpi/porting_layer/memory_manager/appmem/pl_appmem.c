/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#include <pthread.h>
#include <stddef.h>
#include "utility_log.h"
#include "utility_log_module_id.h"
#include "pl_appmem.h"
#include "pl.h"

#define EVENT_ID  0x9500
#define EVENT_ID_START 0x00
#define LOG_E(event_id, format, ...) \
  WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:" \
                   format, __FILE__, __LINE__, ##__VA_ARGS__); \
  WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (EVENT_ID | (EVENT_ID_START + event_id)));

static pthread_mutex_t s_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool s_is_initialized = false;
// -----------------------------------------------------------------------------
PlErrCode PlAppmemInitialize(void) {
  PlErrCode pl_ret = kPlErrCodeOk;
  int ret;
  ret = pthread_mutex_lock(&s_mutex);
  if (ret) {
    LOG_E(0x00, "Failed to lock.");
    return kPlErrLock;
  }
  if (s_is_initialized) {
    LOG_E(0x01, "Already initialized.");
    pl_ret = kPlErrInvalidState;
    goto unlock;
  }
  s_is_initialized = true;

unlock:
  ret = pthread_mutex_unlock(&s_mutex);
  if (ret) {
    LOG_E(0x02, "Failed to unlock.");
  }
  return pl_ret;
}
// -----------------------------------------------------------------------------
PlErrCode PlAppmemFinalize(void) {
  PlErrCode pl_ret = kPlErrCodeOk;
  int ret;
  ret = pthread_mutex_lock(&s_mutex);
  if (ret) {
    LOG_E(0x03, "Failed to lock.");
    return kPlErrLock;
  }
  if (!s_is_initialized) {
    LOG_E(0x04, "Already finalized.");
    pl_ret = kPlErrInvalidState;
    goto unlock;
  }
  s_is_initialized = false;

unlock:
  ret = pthread_mutex_unlock(&s_mutex);
  if (ret) {
    LOG_E(0x05, "Failed to unlock.");
  }
  return pl_ret;
}
// -----------------------------------------------------------------------------
PlErrCode PlAppmemSetBlock(int32_t div_num) {
  // Do nothing.
  return kPlErrCodeOk;
}
// -----------------------------------------------------------------------------
PlAppMemory PlAppmemMalloc(PlAppmemUsage mem_usage, uint32_t size) {
  // No supported.
  return NULL;
}
// -----------------------------------------------------------------------------
PlAppMemory PlAppmemRealloc(PlAppmemUsage mem_usage, PlAppMemory oldmem,
                              uint32_t size) {
  // No supported.
  return NULL;
}
// -----------------------------------------------------------------------------
void PlAppmemFree(PlAppmemUsage mem_usage, PlAppMemory mem) {
  // No supported.
  return;
}
// -----------------------------------------------------------------------------
PlErrCode PlAppmemMap(const void *native_addr, uint32_t size, void **vaddr) {
  PlErrCode pl_ret = kPlErrCodeOk;
  int ret;
  ret = pthread_mutex_lock(&s_mutex);
  if (ret) {
    LOG_E(0x06, "Failed to lock.");
    return kPlErrLock;
  }
  if (!s_is_initialized) {
    LOG_E(0x07, "Not initialized.");
    pl_ret = kPlErrInvalidState;
    goto unlock;
  }
  if (vaddr == NULL) {
    LOG_E(0x08, "vaddr is NULL.");
    pl_ret = kPlErrInvalidParam;
    goto unlock;
  }

  (void)size;                   // Avoid compiler warning
  *vaddr = (void*)native_addr;  // Do nothing

unlock:
  ret = pthread_mutex_unlock(&s_mutex);
  if (ret) {
    LOG_E(0x09, "Failed to unlock.");
  }
  return pl_ret;
}
// -----------------------------------------------------------------------------
PlErrCode PlAppmemUnmap(void *vaddr) {
  PlErrCode pl_ret = kPlErrCodeOk;
  int ret;
  ret = pthread_mutex_lock(&s_mutex);
  if (ret) {
    LOG_E(0x0A, "Failed to lock.");
    return kPlErrLock;
  }
  if (!s_is_initialized) {
    LOG_E(0x0B, "Not initialized.");
    pl_ret = kPlErrInvalidState;
    goto unlock;
  }

  (void)vaddr;  // Avoid compiler warning
  // Do nothing

unlock:
  ret = pthread_mutex_unlock(&s_mutex);
  if (ret) {
    LOG_E(0x0C, "Failed to unlock.");
  }
  return pl_ret;
}
// -----------------------------------------------------------------------------
PlErrCode PlAppmemPwrite(void *native_addr, const char *buf, uint32_t size,
                         uint32_t offset) {
  return kPlErrNoSupported;
}
// -----------------------------------------------------------------------------
bool PlAppmemIsMapSupport(const PlAppMemory mem) {
  (void)mem;  // Avoid compiler warning
  return true;
}
// -----------------------------------------------------------------------------
