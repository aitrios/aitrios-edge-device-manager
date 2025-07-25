/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

// Includes --------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

#include "pl.h"
#include "pl_lheap.h"
#include "utility_log.h"
#include "utility_log_module_id.h"

// Macros ----------------------------------------------------------------------
#define EVENT_ID  0x9500
#define EVENT_ID_START 0x90

#define LOG_E(event_id, format, ...) \
  WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:" \
                   format, __FILE__, __LINE__, ##__VA_ARGS__); \
  WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (EVENT_ID | (EVENT_ID_START + event_id)))

// Global Variables ------------------------------------------------------------
static pthread_mutex_t s_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool s_is_initialized = false;

// External functions ----------------------------------------------------------
PlErrCode PlLheapInitialize(void) {
  PlErrCode pl_ercd = kPlErrCodeOk;

  int32_t ret = pthread_mutex_lock(&s_mutex);
  if (ret != 0) {
    pl_ercd = kPlErrLock;
    LOG_E(0x00, "pthread_mutex_lock() error(ret:%d errno:%d)", ret, errno);
    goto err_mutex;
  }

  if (s_is_initialized) {
    pl_ercd = kPlErrInvalidState;
    LOG_E(0x01, "Already initialized.");
    goto err_end;
  }

  s_is_initialized = true;

err_end:
  ret = pthread_mutex_unlock(&s_mutex);
  if (ret != 0) {
    LOG_E(0x03, "pthread_mutex_unlock() error(ret:%d errno:%d)", ret, errno);
  }

err_mutex:
  return pl_ercd;
}

// -----------------------------------------------------------------------------
PlErrCode PlLheapFinalize(void) {
  PlErrCode pl_ercd = kPlErrCodeOk;

  int32_t ret = pthread_mutex_lock(&s_mutex);
  if (ret != 0) {
    pl_ercd = kPlErrLock;
    LOG_E(0x04, "pthread_mutex_lock() error(ret:%d errno:%d)", ret, errno);
    goto err_mutex;
  }

  if (!s_is_initialized) {
    pl_ercd = kPlErrInvalidState;
    LOG_E(0x05, "Not initialized.");
    goto err_end;
  }

  s_is_initialized = false;

err_end:
  ret = pthread_mutex_unlock(&s_mutex);
  if (ret != 0) {
    LOG_E(0x06, "pthread_mutex_unlock() error(ret:%d errno:%d)", ret, errno);
  }

err_mutex:
  return pl_ercd;
}

// -----------------------------------------------------------------------------
PlLheapHandle PlLheapAlloc(uint32_t size) {
  int32_t ret = pthread_mutex_lock(&s_mutex);
  if (ret != 0) {
    LOG_E(0x07, "pthread_mutex_lock() error(ret:%d errno:%d)", ret, errno);
    return NULL;
  }

  void* handle = NULL;
  if (!s_is_initialized) {
    LOG_E(0x08, "Not initialized.");
    goto err_end;
  }

  if (size == 0) {
    LOG_E(0x09, "argument(size) error.");
    goto err_end;
  }

  handle = (void *)malloc(size);
  if (handle == NULL) {
    LOG_E(0x0A, "malloc() failed. size=%d", size);
    goto err_end;
  }

err_end:
  ret = pthread_mutex_unlock(&s_mutex);
  if (ret != 0) {
    LOG_E(0x0C, "pthread_mutex_unlock() error(ret:%d errno:%d)", ret, errno);
  }
  return handle;
}

// -----------------------------------------------------------------------------
PlErrCode PlLheapFree(PlLheapHandle handle) {
  PlErrCode pl_ercd = kPlErrCodeOk;

  int32_t ret = pthread_mutex_lock(&s_mutex);
  if (ret != 0) {
    pl_ercd = kPlErrLock;
    LOG_E(0x0D, "pthread_mutex_lock() error(ret:%d errno:%d)", ret, errno);
    goto err_mutex;
  }

  if (!s_is_initialized) {
    pl_ercd = kPlErrInvalidState;
    LOG_E(0x0E, "Not initialized.");
    goto err_end;
  }

  if (handle == NULL) {
    pl_ercd = kPlErrInvalidParam;
    LOG_E(0x0F, "argument(handle) NULL.");
    goto err_end;
  }

  free(handle);

err_end:
  ret = pthread_mutex_unlock(&s_mutex);
  if (ret != 0) {
    LOG_E(0x13, "pthread_mutex_unlock() error(ret:%d errno:%d)", ret, errno);
  }

err_mutex:
  return pl_ercd;
}

// -----------------------------------------------------------------------------
PlErrCode PlLheapMap(const PlLheapHandle handle, void **vaddr) {
  PlErrCode pl_ercd = kPlErrCodeOk;

  int32_t ret = pthread_mutex_lock(&s_mutex);
  if (ret != 0) {
    pl_ercd = kPlErrLock;
    LOG_E(0x14, "pthread_mutex_lock() error(ret:%d errno:%d)", ret, errno);
    goto err_mutex;
  }

  if (!s_is_initialized) {
    pl_ercd = kPlErrInvalidState;
    LOG_E(0x15, "Not initialized.");
    goto err_end;
  }

  if (handle == NULL) {
    pl_ercd = kPlErrInvalidParam;
    LOG_E(0x16, "argument(handle) error.");
    goto err_end;
  }
  if (vaddr == NULL) {
    pl_ercd = kPlErrInvalidParam;
    LOG_E(0x17, "argument(vaddr) error.");
    goto err_end;
  }

  *vaddr = (void *)handle;  // Do nothing

err_end:
  ret = pthread_mutex_unlock(&s_mutex);
  if (ret != 0) {
    LOG_E(0x1A, "pthread_mutex_unlock() error(ret:%d errno:%d)", ret, errno);
  }

err_mutex:
  return pl_ercd;
}

// -----------------------------------------------------------------------------
PlErrCode PlLheapUnmap(void *vaddr) {
  PlErrCode pl_ercd = kPlErrCodeOk;

  int32_t ret = pthread_mutex_lock(&s_mutex);
  if (ret != 0) {
    pl_ercd = kPlErrLock;
    LOG_E(0x1B, "pthread_mutex_lock() error(ret:%d errno:%d)", ret, errno);
    goto err_mutex;
  }

  if (!s_is_initialized) {
    pl_ercd = kPlErrInvalidState;
    LOG_E(0x1C, "Not initialized.");
    goto err_end;
  }

  if (vaddr == NULL) {
    pl_ercd = kPlErrInvalidParam;
    LOG_E(0x1D, "argument(vaddr) error.");
    goto err_end;
  }

  (void)vaddr;  // Avoid compiler warning
  // Do nothing

err_end:
  ret = pthread_mutex_unlock(&s_mutex);
  if (ret != 0) {
    LOG_E(0x21, "pthread_mutex_unlock() error(ret:%d errno:%d)", ret, errno);
  }

err_mutex:
  return pl_ercd;
}

// -----------------------------------------------------------------------------
PlErrCode PlLheapGetMeminfo(PlLheapMeminfo *info) {
  LOG_E(0x36, "Not supported");
  return kPlErrNoSupported;
}
// -----------------------------------------------------------------------------
bool PlLheapIsValid(const PlLheapHandle handle) {
  bool valid = false;

  int32_t ret = pthread_mutex_lock(&s_mutex);
  if (ret != 0) {
    LOG_E(0x26, "pthread_mutex_lock() error(ret:%d errno:%d)", ret, errno);
    goto err_mutex;
  }

  if (!s_is_initialized) {
    LOG_E(0x27, "Not initialized.");
    goto err_end;
  }
  // argument check
  if (handle == NULL) {
    LOG_E(0x28, "argument(handle) error.");
    goto err_end;
  }

  valid = true;

err_end:
  ret = pthread_mutex_unlock(&s_mutex);
  if (ret != 0) {
    LOG_E(0x29, "pthread_mutex_unlock() error(ret:%d errno:%d)", ret, errno);
  }

err_mutex:
  return valid;
}

// -----------------------------------------------------------------------------
PlErrCode PlLheapPwrite(const PlLheapHandle handle, const char *buf,
                            uint32_t count, uint32_t offset) {
  LOG_E(0x37, "Not supported");
  return kPlErrNoSupported;
}
// -----------------------------------------------------------------------------
PlErrCode PlLheapFopen(const PlLheapHandle handle, int *pfd) {
  LOG_E(0x38, "Not supported");
  return kPlErrNoSupported;
}
// -----------------------------------------------------------------------------
PlErrCode PlLheapFclose(const PlLheapHandle handle, int fd) {
  LOG_E(0x39, "Not supported");
  return kPlErrNoSupported;
}
// -----------------------------------------------------------------------------
PlErrCode PlLheapFseek(const PlLheapHandle handle, int fd,
                        off_t offset, int whence, off_t *roffset) {
  LOG_E(0x3A, "Not supported");
  return kPlErrNoSupported;
}
// -----------------------------------------------------------------------------
PlErrCode PlLheapFwrite(const PlLheapHandle handle, int fd,
                        const void *buf, size_t size, size_t *rsize) {
  LOG_E(0x3B, "Not supported");
  return kPlErrNoSupported;
}
// -----------------------------------------------------------------------------
PlErrCode PlLheapFread(const PlLheapHandle handle, int fd, void *buf,
                        size_t size, size_t *rsize) {
  LOG_E(0x3C, "Not supported");
  return kPlErrNoSupported;
}
// -----------------------------------------------------------------------------
bool PlLheapIsMapSupport(const PlLheapHandle handle) {
  (void)handle;  // Avoid compiler warning
  return true;
}
// -----------------------------------------------------------------------------
