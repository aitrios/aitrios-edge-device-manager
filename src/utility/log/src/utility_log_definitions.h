/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UTILITY_LOG_DEFINITIONS_H_
#define UTILITY_LOG_DEFINITIONS_H_

#include <stdint.h>
#include <sys/queue.h>

#include "utility_log.h"

#define LOG_SUM_OF_MODULE_ID (4)
#define LOG_FILTER_NONE (0x00000000)
#define LOG_TIMESTAMP_SIZE (24)
#define LOG_LEVEL_STRING_SIZE (1)
#define LOG_MODULE_ID_STRING_SIZE (10)
#define LOG_DESCRIPTION_MAX_SIZE (512)
#define LOG_STRING_SUM_OF_COLON (3)
#define LOG_STRING_CR_SIZE (1)
#define LOG_STRING_NULL_TERMINATION_SIZE (1)
// clang-format off
#define LOG_OTHER_STRING_SIZE                     \
  (LOG_STRING_SUM_OF_COLON + LOG_STRING_CR_SIZE + \
    LOG_STRING_NULL_TERMINATION_SIZE)
#define LOG_STRING_SIZE                                                     \
  (LOG_TIMESTAMP_SIZE + LOG_LEVEL_STRING_SIZE + LOG_MODULE_ID_STRING_SIZE + \
    LOG_DESCRIPTION_MAX_SIZE + LOG_OTHER_STRING_SIZE)
// clang-format on

// This is an enum that tracks the status of utility log.
typedef enum {
  kUtilityLogStateInactive,
  kUtilityLogStateActive
} UtilityLogState;

// This is an enum that definition of dlog destination.
typedef enum {
  kUtilityLogDlogDestUart,   // UART output
  kUtilityLogDlogDestStore,  // Memory write
  kUtilityLogDlogDestBoth,   // UART output & Memory write
} UtilityLogDlogDest;

// This structure aggregates the parameters of Dlog.
typedef struct UtilityLogParams {
  UtilityLogDlogDest dlog_dest;
  UtilityLogDlogLevel dlog_level;
  uint32_t dlog_filter;
} UtilityLogParams;

// The following is a structure that defines a module data.
typedef struct UtilityLogModuleData {
  const uint32_t module_id;
  UtilityLogParams params;
  UtilityLogSetDlogLevelCallback callback;
} UtilityLogModuleData;

#endif  // UTILITY_LOG_DEFINITIONS_H_
