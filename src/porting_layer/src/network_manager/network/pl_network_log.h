/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PORTING_LAYER_SRC_NETWORK_MANAGER_NETWORK_LOG_H_
#define PORTING_LAYER_SRC_NETWORK_MANAGER_NETWORK_LOG_H_

#include "utility_log.h"
#include "utility_log_module_id.h"

#define ELOG_OS_ERROR (0x960C)  // Same number as pl_network.c

#define BASE (MODULE_ID_SYSTEM)
#define DLOGE(fmt, ...)                                                      \
  do {                                                                       \
    WRITE_DLOG_ERROR(BASE, "%s-%d:" fmt, __FILE__, __LINE__, ##__VA_ARGS__); \
  } while (0)

#define DLOGW(fmt, ...)                                                     \
  do {                                                                      \
    WRITE_DLOG_WARN(BASE, "%s-%d:" fmt, __FILE__, __LINE__, ##__VA_ARGS__); \
  } while (0)

#define DLOGI(fmt, ...)                                                     \
  do {                                                                      \
    WRITE_DLOG_INFO(BASE, "%s-%d:" fmt, __FILE__, __LINE__, ##__VA_ARGS__); \
  } while (0)

#define DLOGD(fmt, ...)                                                      \
  do {                                                                       \
    WRITE_DLOG_DEBUG(BASE, "%s-%d:" fmt, __FILE__, __LINE__, ##__VA_ARGS__); \
  } while (0)

#define ELOGE(event_id)               \
  do {                                \
    WRITE_ELOG_ERROR(BASE, event_id); \
  } while (0)

#define ELOGW(event_id)              \
  do {                               \
    WRITE_ELOG_WARN(BASE, event_id); \
  } while (0)

#endif  // PORTING_LAYER_SRC_NETWORK_MANAGER_NETWORK_LOG_H_
