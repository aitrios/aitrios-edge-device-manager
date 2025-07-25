/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef WASM_BINDING_LOG_H_
#define WASM_BINDING_LOG_H_

#ifdef CONFIG_EXTERNAL_UTILITY_LOG
#include "utility_log.h"
#include "utility_log_module_id.h"

#if !defined(__FILE_NAME__)
#define __FILE_NAME__ \
  (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#define WASM_BINDING_CRIT(fmt, ...)                                       \
  WRITE_DLOG_CRITICAL(MODULE_ID_SYSTEM, "%s %d [CRT]" fmt, __FILE_NAME__, \
                      __LINE__, ##__VA_ARGS__)
#define WASM_BINDING_ERR(fmt, ...)                                      \
  WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s %d [ERR] " fmt, __FILE_NAME__, \
                   __LINE__, ##__VA_ARGS__)
#define WASM_BINDING_WARN(fmt, ...)                                    \
  WRITE_DLOG_WARN(MODULE_ID_SYSTEM, "%s %d [WAR] " fmt, __FILE_NAME__, \
                  __LINE__, ##__VA_ARGS__)
#define WASM_BINDING_INFO(fmt, ...)                                    \
  WRITE_DLOG_INFO(MODULE_ID_SYSTEM, "%s %d [INF] " fmt, __FILE_NAME__, \
                  __LINE__, ##__VA_ARGS__)
#define WASM_BINDING_DBG(fmt, ...)                                      \
  WRITE_DLOG_DEBUG(MODULE_ID_SYSTEM, "%s %d [DBG] " fmt, __FILE_NAME__, \
                   __LINE__, ##__VA_ARGS__)
#define WASM_BINDING_TRC(fmt, ...)                                      \
  WRITE_DLOG_TRACE(MODULE_ID_SYSTEM, "%s %d [TRC] " fmt, __FILE_NAME__, \
                   __LINE__, ##__VA_ARGS__)
#else
#define WASM_BINDING_CRIT(fmt, ...) \
  printf("%s %d [CRT] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define WASM_BINDING_ERR(fmt, ...) \
  printf("%s %d [ERR] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define WASM_BINDING_WARN(fmt, ...) \
  printf("%s %d [WAR] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define WASM_BINDING_INFO(fmt, ...) \
  printf("%s %d [INF] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define WASM_BINDING_DBG(fmt, ...) \
  printf("%s %d [DBG] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define WASM_BINDING_TRC(fmt, ...) \
  printf("%s %d [TRC] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif

#endif  // WASM_BINDING_LOG_H_
