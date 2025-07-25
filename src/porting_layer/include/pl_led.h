/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef __PL_LED_H__
#define __PL_LED_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>

#include "pl.h"

// Typedef ---------------------------------------------------------------------

typedef struct {
  uint32_t        color_id;    // red = 0, green = 1, orange = 2
} PlLedColorsInfo;

typedef struct {
  uint32_t        led_id;
  uint32_t        colors_num;
  PlLedColorsInfo colors[CONFIG_PL_LED_COLORS_NUM];
} PlLedLedsInfo;

typedef struct {
  uint32_t        leds_num;
  PlLedLedsInfo   leds[CONFIG_PL_LED_LEDS_NUM];
  uint32_t        interval_resolution_ms;
  struct timespec interval_ts_min;
  struct timespec interval_ts_max;
} PlLedInfo;

typedef struct {
  uint32_t        led_id;
  uint32_t        color_id;
  struct timespec interval;
} PlLedStartParam;

typedef struct {
  uint32_t        led_id;
  uint32_t        color_id;
  struct timespec interval_on;
  struct timespec interval_off;
} PlLedStartParamEx;

typedef struct {
  uint32_t        color_id;
  struct timespec interval;
} PlLedSequence;

// Public API-------------------------------------------------------------------
PlErrCode PlLedStart(const PlLedStartParam *param);
PlErrCode PlLedStartEx(const PlLedStartParamEx *param);
PlErrCode PlLedStartSync(const PlLedStartParam *param, uint32_t param_len);
PlErrCode PlLedStartSyncEx(const PlLedStartParamEx *param, uint32_t param_len);
PlErrCode PlLedStartSeq(uint32_t led_id, const PlLedSequence *seq,
                        uint32_t seq_len);
PlErrCode PlLedStop(uint32_t led_id);
PlErrCode PlLedStopSync(uint32_t *led_ids, uint32_t len);
PlErrCode PlLedStopSeq(uint32_t led_id);
PlErrCode PlLedGetInfo(PlLedInfo *dev_info);
PlErrCode PlLedInitialize(void);
PlErrCode PlLedFinalize(void);

#endif /* __PL_LED_H__ */
