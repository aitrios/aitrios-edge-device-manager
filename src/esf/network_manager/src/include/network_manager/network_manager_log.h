/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESF_NETWORK_MANAGER_SRC_INCLUDE_NETWORK_MANAGER_NETWORK_MANAGER_LOG_H_
#define ESF_NETWORK_MANAGER_SRC_INCLUDE_NETWORK_MANAGER_NETWORK_MANAGER_LOG_H_
#if defined(__NuttX__)
#include <nuttx/config.h>
#endif  // __NuttX__

#include "utility_log.h"
#include "utility_log_module_id.h"

// Network Manager ELOG eventID.
#define ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM (0x8A01)
#define ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2 (0x8A02)
#define ESF_NETWORK_MANAGER_ELOG_INTERNAL_ERROR (0x8A03)
#define ESF_NETWORK_MANAGER_ELOG_PL_FAILURE (0x8A04)
#define ESF_NETWORK_MANAGER_ELOG_FLASH_FAILURE (0x8A05)
#define ESF_NETWORK_MANAGER_ELOG_IP_FAILURE (0x8A06)
#define ESF_NETWORK_MANAGER_ELOG_DHCP_FAILURE (0x8A07)
#define ESF_NETWORK_MANAGER_ELOG_LED_FAILURE (0x8A51)
#define ESF_NETWORK_MANAGER_ELOG_INTERNAL_ERROR_WARN (0x8A52)

#ifdef CONFIG_EXTERNAL_NETWORK_MANAGER_LOGCTL_ENABLE
#define ESF_NETWORK_MANAGER_PRINT(fmt, ...)                       \
  (UtilityLogForcedOutputToUart("%s-%d:" fmt, __FILE__, __LINE__, \
                                ##__VA_ARGS__))
#define ESF_NETWORK_MANAGER_ERR(fmt, ...)                               \
  (WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:" fmt, __FILE__, __LINE__, \
                    ##__VA_ARGS__))
#define ESF_NETWORK_MANAGER_WARN(fmt, ...)                             \
  (WRITE_DLOG_WARN(MODULE_ID_SYSTEM, "%s-%d:" fmt, __FILE__, __LINE__, \
                   ##__VA_ARGS__))
#define ESF_NETWORK_MANAGER_INFO(fmt, ...)                             \
  (WRITE_DLOG_INFO(MODULE_ID_SYSTEM, "%s-%d:" fmt, __FILE__, __LINE__, \
                   ##__VA_ARGS__))
#define ESF_NETWORK_MANAGER_DBG(fmt, ...)                               \
  (WRITE_DLOG_DEBUG(MODULE_ID_SYSTEM, "%s-%d:" fmt, __FILE__, __LINE__, \
                    ##__VA_ARGS__))
#if 0
#define ESF_NETWORK_MANAGER_TRACE(fmt, ...)                             \
  (WRITE_DLOG_TRACE(MODULE_ID_SYSTEM, "%s-%d:" fmt, __FILE__, __LINE__, \
                    ##__VA_ARGS__))
#else
#define ESF_NETWORK_MANAGER_TRACE(fmt, ...)
#endif
#define ESF_NETWORK_MANAGER_ELOG_ERR(event_id) \
  (WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, event_id))
#define ESF_NETWORK_MANAGER_ELOG_WARN(event_id) \
  (WRITE_ELOG_WARN(MODULE_ID_SYSTEM, event_id))
#define ESF_NETWORK_MANAGER_ELOG_INFO(event_id) \
  (WRITE_ELOG_INFO(MODULE_ID_SYSTEM, event_id))

#else  // CONFIG_EXTERNAL_NETWORK_MANAGER_LOGCTL_ENABLE
#define ESF_NETWORK_MANAGER_PRINT(fmt, ...)                       \
  (printf("[%s][%s][%d] " fmt "\n", __FILE__, __func__, __LINE__, \
          ##__VA_ARGS__))
#define ESF_NETWORK_MANAGER_ERR(fmt, ...)                              \
  (printf("[ERR][%s][%s][%d] " fmt "\n", __FILE__, __func__, __LINE__, \
          ##__VA_ARGS__))
#define ESF_NETWORK_MANAGER_WARN(fmt, ...)                             \
  (printf("[WRN][%s][%s][%d] " fmt "\n", __FILE__, __func__, __LINE__, \
          ##__VA_ARGS__))
#define ESF_NETWORK_MANAGER_INFO(fmt, ...)                             \
  (printf("[INF][%s][%s][%d] " fmt "\n", __FILE__, __func__, __LINE__, \
          ##__VA_ARGS__))
#ifdef ENABLE_DEBUG_LOG
#define ESF_NETWORK_MANAGER_DBG(fmt, ...)                              \
  (printf("[DBG][%s][%s][%d] " fmt "\n", __FILE__, __func__, __LINE__, \
          ##__VA_ARGS__))
#define ESF_NETWORK_MANAGER_TRACE(fmt, ...)                            \
  (printf("[TRC][%s][%s][%d] " fmt "\n", __FILE__, __func__, __LINE__, \
          ##__VA_ARGS__))
#else  // ENABLE_DEBUG_LOG
#define ESF_NETWORK_MANAGER_DBG(fmt, ...)
#define ESF_NETWORK_MANAGER_TRACE(fmt, ...)
#endif  // ENABLE_DEBUG_LOG

#define ESF_NETWORK_MANAGER_ELOG_ERR(event_id)                              \
  (printf("[ELOG][ERR][%s][%s][%d] 0x%04x\n", __FILE__, __func__, __LINE__, \
          event_id))
#define ESF_NETWORK_MANAGER_ELOG_WARN(event_id)                             \
  (printf("[ELOG][WRN][%s][%s][%d] 0x%04x\n", __FILE__, __func__, __LINE__, \
          event_id))
#define ESF_NETWORK_MANAGER_ELOG_INFO(event_id)                             \
  (printf("[ELOG][INF][%s][%s][%d] 0x%04x\n", __FILE__, __func__, __LINE__, \
          event_id))
#endif  // CONFIG_EXTERNAL_NETWORK_MANAGER_LOGCTL_ENABLE

#endif  // ESF_NETWORK_MANAGER_SRC_INCLUDE_NETWORK_MANAGER_NETWORK_MANAGER_LOG_H_
