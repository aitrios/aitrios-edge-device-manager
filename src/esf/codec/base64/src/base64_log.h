/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef ESF_CODEC_BASE64_BASE64_LOG_H_
#define ESF_CODEC_BASE64_BASE64_LOG_H_

#ifdef __NuttX__
#include <nuttx/config.h>
#endif  // __NuttX__

#include "utility_log.h"
#include "utility_log_module_id.h"

// ELOG eventID.
#define ESF_CODEC_BASE64_ELOG_INVALID_PARAM (0x8251u)
#define ESF_CODEC_BASE64_ELOG_DECODE_FAILURE (0x8252u)
#define ESF_CODEC_BASE64_ELOG_ENCODE_FAILURE (0x8253u)

#ifndef CONFIG_EXTERNAL_CODEC_BASE64_LOGCTL_ENABLE
#define ESF_CODEC_BASE64_PRINT(fmt, ...)                          \
  (printf("[%s][%s][%d] " fmt "\n", __FILE__, __func__, __LINE__, \
          ##__VA_ARGS__))
#define ESF_CODEC_BASE64_ERR(fmt, ...)                                 \
  (printf("[ERR][%s][%s][%d] " fmt "\n", __FILE__, __func__, __LINE__, \
          ##__VA_ARGS__))
#define ESF_CODEC_BASE64_WARN(fmt, ...)                                \
  (printf("[WRN][%s][%s][%d] " fmt "\n", __FILE__, __func__, __LINE__, \
          ##__VA_ARGS__))
#define ESF_CODEC_BASE64_INFO(fmt, ...)                                \
  (printf("[INF][%s][%s][%d] " fmt "\n", __FILE__, __func__, __LINE__, \
          ##__VA_ARGS__))
#ifdef ENABLE_DEBUG_LOG
#define ESF_CODEC_BASE64_DBG(fmt, ...)                                 \
  (printf("[DBG][%s][%s][%d] " fmt "\n", __FILE__, __func__, __LINE__, \
          ##__VA_ARGS__))
#define ESF_CODEC_BASE64_TRACE(fmt, ...)                               \
  (printf("[TRC][%s][%s][%d] " fmt "\n", __FILE__, __func__, __LINE__, \
          ##__VA_ARGS__))
#else  // ENABLE_DEBUG_LOG
#define ESF_CODEC_BASE64_DBG(fmt, ...)
#define ESF_CODEC_BASE64_TRACE(fmt, ...)
#endif  // ENABLE_DEBUG_LOG
#define ESF_CODEC_BASE64_ELOG_ERR(event_id)                                 \
  (printf("[ELOG][ERR][%s][%s][%d] 0x%04x\n", __FILE__, __func__, __LINE__, \
          event_id))
#define ESF_CODEC_BASE64_ELOG_WARN(event_id)                                \
  (printf("[ELOG][WRN][%s][%s][%d] 0x%04x\n", __FILE__, __func__, __LINE__, \
          event_id))
#define ESF_CODEC_BASE64_ELOG_INFO(event_id)                                \
  (printf("[ELOG][INF][%s][%s][%d] 0x%04x\n", __FILE__, __func__, __LINE__, \
          event_id))
#else
#define ESF_CODEC_BASE64_PRINT(fmt, ...)
#define ESF_CODEC_BASE64_ERR(fmt, ...)                                  \
  (WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:" fmt, __FILE__, __LINE__, \
                    ##__VA_ARGS__))
#define ESF_CODEC_BASE64_WARN(fmt, ...)                                \
  (WRITE_DLOG_WARN(MODULE_ID_SYSTEM, "%s-%d:" fmt, __FILE__, __LINE__, \
                   ##__VA_ARGS__))
#define ESF_CODEC_BASE64_INFO(fmt, ...)                                \
  (WRITE_DLOG_INFO(MODULE_ID_SYSTEM, "%s-%d:" fmt, __FILE__, __LINE__, \
                   ##__VA_ARGS__))
#define ESF_CODEC_BASE64_DBG(fmt, ...)                                  \
  (WRITE_DLOG_DEBUG(MODULE_ID_SYSTEM, "%s-%d:" fmt, __FILE__, __LINE__, \
                    ##__VA_ARGS__))
#ifdef ENABLE_DEBUG_LOG
#define ESF_CODEC_BASE64_TRACE(fmt, ...)                                \
  (WRITE_DLOG_TRACE(MODULE_ID_SYSTEM, "%s-%d:" fmt, __FILE__, __LINE__, \
                    ##__VA_ARGS__))
#else  // ENABLE_DEBUG_LOG
#define ESF_CODEC_BASE64_TRACE(fmt, ...)
#endif  // ENABLE_DEBUG_LOG
#define ESF_CODEC_BASE64_ELOG_ERR(event_id) \
  (WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, event_id))
#define ESF_CODEC_BASE64_ELOG_WARN(event_id) \
  (WRITE_ELOG_WARN(MODULE_ID_SYSTEM, event_id))
#define ESF_CODEC_BASE64_ELOG_INFO(event_id) \
  (WRITE_ELOG_INFO(MODULE_ID_SYSTEM, event_id))
#endif  // CONFIG_EXTERNAL_CODEC_BASE64_LOGCTL_ENABLE

#endif  // ESF_CODEC_BASE64_BASE64_LOG_H_
