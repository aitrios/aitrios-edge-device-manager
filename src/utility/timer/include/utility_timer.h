/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef __UTILITY_TIMER_H
#define __UTILITY_TIMER_H

/*******************************************************************************
 * Included Files
 ******************************************************************************/
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include <sys/queue.h>
#if defined(__NuttX__)
#include <nuttx/timers/timer.h>
#endif

/*******************************************************************************
 * Pre-preprocessor Definitions
 ******************************************************************************/
#define UTILITY_TIMER_MAX (10)
/*******************************************************************************
 * Public Types
 ******************************************************************************/
typedef void (*UtilityTimerCallback)(void *timer_cb_params);
/*******************************************************************************
 * Public Data
 ******************************************************************************/
typedef struct UtilityTimerSystemInfo {
  struct timespec interval_min_ts;
  struct timespec interval_max_ts;
  // T.B.D.
} UtilityTimerSystemInfo;

typedef enum {
  kUtilityTimerOk = 0,
  kUtilityTimerErrInvalidStatus,
  kUtilityTimerErrInvalidParams,
  kUtilityTimerErrNotFound,
  kUtilityTimerErrBusy,
  kUtilityTimerErrInternal,
  // T.B.D.
} UtilityTimerErrCode;

typedef enum {
  kUtilityTimerOneShot = 0,
  kUtilityTimerRepeat,
} UtilityTimerRepeatType;

typedef void *UtilityTimerHandle;

/*******************************************************************************
 * Inline Functions
 ******************************************************************************/

/*******************************************************************************
 * Public Function Prototypes
 ******************************************************************************/
UtilityTimerErrCode UtilityTimerInitialize(void);
UtilityTimerErrCode UtilityTimerFinalize(void);
UtilityTimerErrCode UtilityTimerCreate(
    const UtilityTimerCallback utility_timer_cb,
    void *timer_cb_params,
    UtilityTimerHandle *utility_timer_handle);
UtilityTimerErrCode UtilityTimerCreateEx(
    const UtilityTimerCallback callback,
    void *cb_params,
    int priority,
    size_t stacksize,
    UtilityTimerHandle *timer_handle);
UtilityTimerErrCode UtilityTimerStart(
    const UtilityTimerHandle utility_timer_handle,
    const struct timespec *interval_ts,
    const UtilityTimerRepeatType utility_timer_repeat_type);
UtilityTimerErrCode UtilityTimerStop(
    const UtilityTimerHandle utility_timer_handle);
UtilityTimerErrCode UtilityTimerDelete(UtilityTimerHandle utility_timer_handle);
UtilityTimerErrCode UtilityTimerGetSystemInfo(
    UtilityTimerSystemInfo *utility_timer_sysinfo);
#endif /* __UTILITY_TIMER_H */
