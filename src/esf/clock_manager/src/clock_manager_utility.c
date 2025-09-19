/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define _POSIX_C_SOURCE 200809L

#include "clock_manager_utility.h"

#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include "clock_manager.h"
#include "utility_log.h"
#include "utility_log_module_id.h"

#ifndef CLOCK_MANAGER_REMOVE_STATIC
#define STATIC static
#else  // CLOCK_MANAGER_REMOVE_STATIC
#define STATIC
#endif  // CLOCK_MANAGER_REMOVE_STATIC

EsfClockManagerCondBaseReturnValue EsfClockManagerInitCondBase(
    pthread_cond_t *cond) {
  if (cond == NULL) {
    return kCondBaseRvIsParamError;
  }

  pthread_condattr_t condattr;
  int rv = pthread_condattr_init(&condattr);
  if (rv != 0) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- pthread_condattr_init failed:%d\n",
                     "clock_manager_utility.c", __LINE__, __func__, rv);
    return kCondBaseRvIsCondError;
  }

  rv = pthread_condattr_setclock(&condattr, CLOCK_MONOTONIC);
  if (rv != 0) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- pthread_condattr_setclock failed:%d\n",
                     "clock_manager_utility.c", __LINE__, __func__, rv);
    (void)pthread_condattr_destroy(&condattr);
    return kCondBaseRvIsCondError;
  }

  rv = pthread_cond_init(cond, &condattr);
  if (rv != 0) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- pthread_cond_init failed:%d\n",
                     "clock_manager_utility.c", __LINE__, __func__, rv);
    (void)pthread_condattr_destroy(&condattr);
    return kCondBaseRvIsCondError;
  }

  rv = pthread_condattr_destroy(&condattr);
  if (rv != 0) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- pthread_condattr_destroy failed:%d\n",
                     "clock_manager_utility.c", __LINE__, __func__, rv);
    (void)pthread_cond_destroy(cond);
    return kCondBaseRvIsCondError;
  }

  return kCondBaseRvIsSuccess;
}

EsfClockManagerCondBaseReturnValue EsfClockManagerDeinitCondBase(
    pthread_cond_t *cond) {
  if (cond == NULL) {
    return kCondBaseRvIsParamError;
  }

  int rv = pthread_cond_destroy(cond);
  if (rv != 0) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- pthread_cond_destroy failed:%d\n",
                     "clock_manager_utility.c", __LINE__, __func__, rv);
    return kCondBaseRvIsCondError;
  }

  return kCondBaseRvIsSuccess;
}

STATIC bool EsfClockManagerAddTimespaces(const struct timespec *const lhs,
                                         const struct timespec *const rhs,
                                         struct timespec *const out) {
  if (lhs == NULL || rhs == NULL || out == NULL) {
    return false;
  }
  out->tv_sec = lhs->tv_sec + rhs->tv_sec;
  out->tv_nsec = lhs->tv_nsec + rhs->tv_nsec;
  if (out->tv_nsec >= 1000000000L) {
    out->tv_sec = out->tv_sec + (time_t)1;
    out->tv_nsec = out->tv_nsec - 1000000000L;
  }

  return true;
}

bool EsfClockManagerCalculateAbstimeInMonotonic(
    struct timespec *const abs_time,
    const EsfClockManagerMillisecondT duration) {
  struct timespec now;
  const int clock_ret = clock_gettime(CLOCK_MONOTONIC, &now);
  if (clock_ret) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:%s --- clock_gettime failed:%d\n",
                     "clock_manager_utility.c", __LINE__, __func__, clock_ret);
    return false;
  }
  struct timespec diff;
  diff.tv_sec = (time_t)(duration / 1000);       // Seconds
  diff.tv_nsec = ((duration % 1000) * 1000000);  // Nanoseconds
  EsfClockManagerAddTimespaces(&now, &diff, abs_time);

  return true;
}
