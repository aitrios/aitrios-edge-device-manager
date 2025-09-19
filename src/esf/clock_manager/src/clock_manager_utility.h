/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESF_CLOCK_MANAGER_UTILITY_H_
#define ESF_CLOCK_MANAGER_UTILITY_H_

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include "clock_manager.h"
#include "pl_clock_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Definitions of macros
 */
#define NOT_USED(x) ((void)(x))

#define CALLER_IS_INIT_SETTING (0)
#define CALLER_IS_GET_PARAMS (1)
#define CALLER_IS_START_INTERNAL (2)

/**
 * Declaration of types
 */

typedef int32_t EsfClockManagerMillisecondT;

/**
 * Definitions of package private enumerations
 */
typedef struct EsfClockManagerCondBase {
  pthread_mutex_t m_mutex;  // mutex
  pthread_cond_t m_cond;    // condition
} EsfClockManagerCondBase;

typedef enum EsfClockManagerMutexBaseReturnValue {
  kMutexBaseRvIsSuccess,
  kMutexBaseRvIsParamError,
  kMutexBaseRvIsMutexError,
} EsfClockManagerMutexBaseReturnValue;

typedef enum EsfClockManagerCondBaseReturnValue {
  kCondBaseRvIsSuccess,
  kCondBaseRvIsParamError,
  kCondBaseRvIsMutexError,
  kCondBaseRvIsCondError,
} EsfClockManagerCondBaseReturnValue;

/**
 * Definition of package private structures
 */

typedef struct EsfClockManagerMutexBase {
  pthread_mutex_t m_mutex;
} EsfClockManagerMutexBase;

// This structure prevents a thread id of the thread from race condition.
typedef struct EsfClockManagerThreadId {
  EsfClockManagerMutexBase m_mutex_base;  // mutex
  pthread_t m_thread_id;                  // thread id
} EsfClockManagerThreadId;

EsfClockManagerCondBaseReturnValue EsfClockManagerInitCondBase(
    pthread_cond_t *cond);

EsfClockManagerCondBaseReturnValue EsfClockManagerDeinitCondBase(
    pthread_cond_t *cond);

// This function adds the given duration to now real time.  Then the result
// is returned.

// Args:
//    abs_time (struct timespec * const): result
//    duration (EsfClockManagerMillisecondT): duration which adds to now
//      (realtime) clock time.

// Returns:
//    true: success.
//    false: failure.
//
// """
bool EsfClockManagerCalculateAbstimeInMonotonic(
    struct timespec *const abs_time,
    const EsfClockManagerMillisecondT duration);

#ifdef __cplusplus
}
#endif

#endif  // ESF_CLOCK_MANAGER_UTILITY_H_
