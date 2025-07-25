/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "clock_manager_utility.h"

#include <stdbool.h>
#include <stdlib.h>

#include "clock_manager.h"
#include "utility_log.h"
#include "utility_log_module_id.h"

#ifndef CLOCK_MANAGER_REMOVE_STATIC
#define STATIC static
#else  // CLOCK_MANAGER_REMOVE_STATIC
#define STATIC
#endif  // CLOCK_MANAGER_REMOVE_STATIC

/**
 * Declarations of private functions --- i.e., functions given storage-class
 * specifier static.
 */

// """Adds two timespec objects.

// This function is lhs + rhs.

// Args:
//    lhs (const struct timespec * const): value.
//    rhs (const struct timespec * const): other value.
//    out (struct timespec * const): result; lhs + rhs.

// Returns:
//    true: success.
//    false: failure.
//
// """
STATIC bool EsfClockManagerAddTimespaces(const struct timespec *const lhs,
                                         const struct timespec *const rhs,
                                         struct timespec *const out);

/**
 * Definitions of package private functions
 */

EsfClockManagerMutexBaseReturnValue EsfClockManagerInitMutexBase(
    EsfClockManagerMutexBase *const mutex_base) {
  if (mutex_base == NULL) {
    return kMutexBaseRvIsParamError;
  }

  if (mutex_base->m_mutex == NULL) {
    mutex_base->m_mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    if (mutex_base->m_mutex == NULL) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:%s --- malloc failed.\n",
                       "clock_manager_utility.c", __LINE__, __func__);
      return kMutexBaseRvIsMutexError;
    }

    const int rv = pthread_mutex_init(mutex_base->m_mutex, NULL);
    if (rv) {
      free(mutex_base->m_mutex);
      mutex_base->m_mutex = NULL;
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- pthread_mutex_init failed:%d\n",
                       "clock_manager_utility.c", __LINE__, __func__, rv);
      return kMutexBaseRvIsMutexError;
    }
  }

  return kMutexBaseRvIsSuccess;
}

EsfClockManagerMutexBaseReturnValue EsfClockManagerDeinitMutexBase(
    EsfClockManagerMutexBase *const mutex_base) {
  if (mutex_base != NULL) {
    if (mutex_base->m_mutex != NULL) {
      pthread_mutex_destroy(mutex_base->m_mutex);
    }

    free(mutex_base->m_mutex);
    mutex_base->m_mutex = NULL;
  }

  return kMutexBaseRvIsSuccess;
}

EsfClockManagerCondBaseReturnValue EsfClockManagerInitCondBase(
    EsfClockManagerCondBase *const cond_base) {
  if (cond_base == NULL) {
    return kCondBaseRvIsParamError;
  }

  if (cond_base->m_mutex == NULL) {
    cond_base->m_mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    if (cond_base->m_mutex == NULL) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:%s --- malloc failed.\n",
                       "clock_manager_utility.c", __LINE__, __func__);
      return kCondBaseRvIsMutexError;
    }

    const int rv = pthread_mutex_init(cond_base->m_mutex, NULL);
    if (rv) {
      free(cond_base->m_mutex);
      cond_base->m_mutex = NULL;
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- pthread_mutex_init failed:%d\n",
                       "clock_manager_utility.c", __LINE__, __func__, rv);
      return kCondBaseRvIsMutexError;
    }
  }

  if (cond_base->m_cond == NULL) {
    cond_base->m_cond = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
    if (cond_base->m_cond == NULL) {
      pthread_mutex_destroy(cond_base->m_mutex);
      free(cond_base->m_mutex);
      cond_base->m_mutex = NULL;
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:%s --- malloc failed.\n",
                       "clock_manager_utility.c", __LINE__, __func__);
      return kCondBaseRvIsCondError;
    }

#ifdef __NuttX__
    const pthread_condattr_t attr = {0, (clockid_t)CLOCK_MONOTONIC};
    const int rv2 = pthread_cond_init(cond_base->m_cond, &attr);
#else
    const int rv2 = pthread_cond_init(cond_base->m_cond, NULL);
#endif
    if (rv2) {
      free(cond_base->m_cond);
      cond_base->m_cond = NULL;
      pthread_mutex_destroy(cond_base->m_mutex);
      free(cond_base->m_mutex);
      cond_base->m_mutex = NULL;
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- pthread_cond_init failed:%d\n",
                       "clock_manager_utility.c", __LINE__, __func__, rv2);
      return kCondBaseRvIsCondError;
    }
  }

  return kCondBaseRvIsSuccess;
}

EsfClockManagerCondBaseReturnValue EsfClockManagerDeinitCondBase(
    EsfClockManagerCondBase *const cond_base) {
  if (cond_base != NULL) {
    if (cond_base->m_mutex != NULL) {
      pthread_mutex_destroy(cond_base->m_mutex);
    }
    if (cond_base->m_cond != NULL) {
      pthread_cond_destroy(cond_base->m_cond);
    }

    free(cond_base->m_mutex);
    cond_base->m_mutex = NULL;
    free(cond_base->m_cond);
    cond_base->m_cond = NULL;
  }

  return kCondBaseRvIsSuccess;
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
