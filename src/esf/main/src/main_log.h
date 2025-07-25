/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESF_MAIN_MAIN_LOG_H_
#define ESF_MAIN_MAIN_LOG_H_

#ifdef __NuttX__
#include <nuttx/config.h>
#endif  // __NuttX__

#include "utility_log.h"
#include "utility_log_module_id.h"

// EsfMain ELOG eventID.
#define ESF_MAIN_ELOG_INIT_FAILURE (0x8801)
#define ESF_MAIN_ELOG_EMMC_INIT_FAILURE (0x8802)
#define ESF_MAIN_ELOG_SYSTEM_ERROR (0x8803)
#define ESF_MAIN_ELOG_TERM_FAILURE (0x8851)
#define ESF_MAIN_ELOG_FACTORY_RESET_FAILURE (0x8804)
#define ESF_MAIN_ELOG_INVALID_PARAM (0x8805)

#ifndef CONFIG_EXTERNAL_MAIN_ENABLE_LOG
#define ESF_MAIN_PRINT(fmt, ...)                                  \
  (printf("[%s][%s][%d] " fmt "\n", __FILE__, __func__, __LINE__, \
          ##__VA_ARGS__))
#define ESF_MAIN_ERR(fmt, ...)                                               \
  (printf("[DLOG][ERR][%s][%s][%d] " fmt "\n", __FILE__, __func__, __LINE__, \
          ##__VA_ARGS__))
#define ESF_MAIN_WARN(fmt, ...)                                              \
  (printf("[DLOG][WRN][%s][%s][%d] " fmt "\n", __FILE__, __func__, __LINE__, \
          ##__VA_ARGS__))
#define ESF_MAIN_INFO(fmt, ...)                                              \
  (printf("[DLOG][INF][%s][%s][%d] " fmt "\n", __FILE__, __func__, __LINE__, \
          ##__VA_ARGS__))
#ifdef ESF_MAIN_DEBUG_LOG_ENABLE
#define ESF_MAIN_DBG(fmt, ...)                                               \
  (printf("[DLOG][DBG][%s][%s][%d] " fmt "\n", __FILE__, __func__, __LINE__, \
          ##__VA_ARGS__))
#define ESF_MAIN_TRACE(fmt, ...)                                             \
  (printf("[DLOG][TRC][%s][%s][%d] " fmt "\n", __FILE__, __func__, __LINE__, \
          ##__VA_ARGS__))
#else
#define ESF_MAIN_DBG(...)
#define ESF_MAIN_TRACE(...)
#endif  // ESF_MAIN_DEBUG_LOG_ENABLE
#else
#define ESF_MAIN_PRINT(...)
#define ESF_MAIN_ERR(fmt, ...)                                          \
  (WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:" fmt, __FILE__, __LINE__, \
                    ##__VA_ARGS__))
#define ESF_MAIN_WARN(fmt, ...)                                        \
  (WRITE_DLOG_WARN(MODULE_ID_SYSTEM, "%s-%d:" fmt, __FILE__, __LINE__, \
                   ##__VA_ARGS__))
#define ESF_MAIN_INFO(fmt, ...)                                        \
  (WRITE_DLOG_INFO(MODULE_ID_SYSTEM, "%s-%d:" fmt, __FILE__, __LINE__, \
                   ##__VA_ARGS__))
#define ESF_MAIN_DBG(fmt, ...)                                          \
  (WRITE_DLOG_DEBUG(MODULE_ID_SYSTEM, "%s-%d:" fmt, __FILE__, __LINE__, \
                    ##__VA_ARGS__))
#ifdef ESF_MAIN_DEBUG_LOG_ENABLE
#define ESF_MAIN_TRACE(fmt, ...)                                        \
  (WRITE_DLOG_TRACE(MODULE_ID_SYSTEM, "%s-%d:" fmt, __FILE__, __LINE__, \
                    ##__VA_ARGS__))
#else
#define ESF_MAIN_TRACE(...)
#endif  // ESF_MAIN_DEBUG_LOG_ENABLE
#endif  // CONFIG_EXTERNAL_MAIN_ENABLE_LOG

#ifndef CONFIG_EXTERNAL_MAIN_ENABLE_LOG
#define ESF_MAIN_ELOG_ERR(event_id)                                         \
  (printf("[ELOG][ERR][%s][%s][%d] 0x%04x\n", __FILE__, __func__, __LINE__, \
          event_id))
#define ESF_MAIN_ELOG_WARN(event_id)                                        \
  (printf("[ELOG][WRN][%s][%s][%d] 0x%04x\n", __FILE__, __func__, __LINE__, \
          event_id))
#define ESF_MAIN_ELOG_INFO(event_id)                                        \
  (printf("[ELOG][INF][%s][%s][%d] 0x%04x\n", __FILE__, __func__, __LINE__, \
          event_id))
#else  // CONFIG_EXTERNAL_MAIN_ENABLE_LOG
#define ESF_MAIN_ELOG_ERR(event_id) \
  (WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, event_id))
#define ESF_MAIN_ELOG_WARN(event_id) \
  (WRITE_ELOG_WARN(MODULE_ID_SYSTEM, event_id))
#define ESF_MAIN_ELOG_INFO(event_id) \
  (WRITE_ELOG_INFO(MODULE_ID_SYSTEM, event_id))
#endif  // CONFIG_EXTERNAL_MAIN_ENABLE_LOG

// Log macro to be used when LogManager is not initialized
#define ESF_MAIN_LOG_ERR(message, ...)                                   \
  printf("E:0x%08X:%s-%d:" message "\n", (unsigned int)MODULE_ID_SYSTEM, \
         __FILE__, __LINE__, ##__VA_ARGS__)
#define ESF_MAIN_LOG_WARN(message, ...)                                  \
  printf("W:0x%08X:%s-%d:" message "\n", (unsigned int)MODULE_ID_SYSTEM, \
         __FILE__, __LINE__, ##__VA_ARGS__)
#define ESF_MAIN_LOG_INFO(message, ...)                                  \
  printf("I:0x%08X:%s-%d:" message "\n", (unsigned int)MODULE_ID_SYSTEM, \
         __FILE__, __LINE__, ##__VA_ARGS__)
#ifdef ESF_MAIN_DEBUG_LOG_ENABLE
#define ESF_MAIN_LOG_DBG(message, ...)                                   \
  printf("D:0x%08X:%s-%d:" message "\n", (unsigned int)MODULE_ID_SYSTEM, \
         __FILE__, __LINE__, ##__VA_ARGS__)
#define ESF_MAIN_LOG_TRACE(message, ...)                                 \
  printf("T:0x%08X:%s-%d:" message "\n", (unsigned int)MODULE_ID_SYSTEM, \
         __FILE__, __LINE__, ##__VA_ARGS__)
#else  // ESF_MAIN_DEBUG_LOG_ENABLE
#define ESF_MAIN_LOG_DBG(...)
#define ESF_MAIN_LOG_TRACE(...)
#endif  // ESF_MAIN_DEBUG_LOG_ENABLE

#endif  // ESF_MAIN_MAIN_LOG_H_
