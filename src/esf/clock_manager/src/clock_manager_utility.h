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
  pthread_mutex_t *m_mutex;
} EsfClockManagerMutexBase;

typedef struct EsfClockManagerCondBase {
  pthread_mutex_t *m_mutex;  // mutex
  pthread_cond_t *m_cond;    // condition
} EsfClockManagerCondBase;

// """Initializes the given object of structure EsfClockManagerMutexBase.

// If the given cond_base is NULL, returns kClockManagerParamError.
// This function initializes an object of pthread_mutex_t.

// Args:
//    mutex_base (EsfClockManagerMutexBase *const): a pointer to
//      an object of EsfClockManagerMutexBase.  This formal parameter is a
//      pointer to an object to be initialized by this function.

// Returns:
//    The following values are returned:
//    kMutexBaseRvIsSuccess: success.
//    kMutexBaseRvIsParamError: invalid parameter.
//    kMutexBaseRvIsMutexError: error with respect to pthread_mutex_t

// """
EsfClockManagerMutexBaseReturnValue EsfClockManagerInitMutexBase(
    EsfClockManagerMutexBase *const mutex_base);

// """Deinitializes the given object of structure EsfClockManagerMutexBase.

// If the given cond_base is NULL, returns kClockManagerParamError.
// This function deinitializes an object of pthread_mutex_t.

// Args:
//    mutex_base (EsfClockManagerMutexBase *const): a pointer to
//      an object of EsfClockManagerMutexBase.  This formal parameter is a
//      pointer to an object to be deinitialized by this function.

// Returns:
//    The following values are returned:
//    kMutexBaseRvIsSuccess: success.
//    kMutexBaseRvIsParamError: invalid parameter.
//    kMutexBaseRvIsMutexError: error with respect to pthread_mutex_t

// """
EsfClockManagerMutexBaseReturnValue EsfClockManagerDeinitMutexBase(
    EsfClockManagerMutexBase *const mutex_base);

// """Initializes the given object of structure EsfClockManagerCondBase.

// If the given cond_base is NULL, returns kClockManagerParamError.
// This function initializes an object of pthread_cond_t and an object of
// pthread_mutex_t.

// Args:
//    cond_base (EsfClockManagerCondBase *const): a pointer to
//      an object of EsfClockManagerCondBase.  This formal parameter is a
//      pointer to an object to be initialized by this function.

// Returns:
//    The following values are returned:
//    kCondBaseRvIsSuccess: success.
//    kCondBaseRvIsParamError: invalid parameter.
//    kCondBaseRvIsMutexError: error with respect to pthread_mutex_t
//    kCondBaseRvIsCondError: error with respect to pthread_cond_t

// """
EsfClockManagerCondBaseReturnValue EsfClockManagerInitCondBase(
    EsfClockManagerCondBase *const cond_base);

// """Deinitializes the given object of structure EsfClockManagerCondBase.

// If the given cond_base is NULL, returns kClockManagerParamError.
// This function deinitializes an object of pthread_cond_t and an object of
// pthread_mutex_t.

// Args:
//    cond_base (EsfClockManagerCondBase *const): a pointer to
//      an object of EsfClockManagerCondBase.  This formal parameter is a
//      pointer to an object to be deinitialized by this function.

// Returns:
//    The following values are returned:
//    kCondBaseRvIsSuccess: success.
//    kCondBaseRvIsParamError: invalid parameter.
//    kCondBaseRvIsMutexError: error with respect to pthread_mutex_t
//    kCondBaseRvIsCondError: error with respect to pthread_cond_t

// """
EsfClockManagerCondBaseReturnValue EsfClockManagerDeinitCondBase(
    EsfClockManagerCondBase *const cond_base);

// """Calculates absolute real time.

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
