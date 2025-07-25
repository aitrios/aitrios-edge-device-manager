/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Define the external API for Utility log code.

#ifndef UTILITY_LOG_H_
#define UTILITY_LOG_H_

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// This code defines an enumeration type for the result of executing an API.
typedef enum {
  kUtilityLogStatusOk,          //  No errors.
  kUtilityLogStatusFailed,      //  Status Error.
  kUtilityLogStatusParamError,  //  Parameter Error.
  kUtilityLogStatusNum          //  UtilityLogStatus element count
} UtilityLogStatus;

// This code enumeration type that defines the log level of Dlog.
typedef enum {
  kUtilityLogDlogLevelCritical,  // Critical
  kUtilityLogDlogLevelError,     // Error
  kUtilityLogDlogLevelWarn,      // Warning
  kUtilityLogDlogLevelInfo,      // Info
  kUtilityLogDlogLevelDebug,     // Debug
  kUtilityLogDlogLevelTrace,     // Trace
  kUtilityLogDlogLevelNum        // UtilityLogDlogLevel element count
} UtilityLogDlogLevel;

// This code enumeration type that defines the log level of Elog.
typedef enum {
  kUtilityLogElogLevelCritical,  // Critical
  kUtilityLogElogLevelError,     // Error
  kUtilityLogElogLevelWarn,      // Warning
  kUtilityLogElogLevelInfo,      // Info
  kUtilityLogElogLevelDebug,     // Debug
  kUtilityLogElogLevelTrace,     // Trace
  kUtilityLogElogLevelNum        // UtilityLogElogLevel element count
} UtilityLogElogLevel;

typedef void (*UtilityLogNotificationCallback)(size_t size, void *user_data);

typedef void (*UtilityLogSetDlogLevelCallback)(UtilityLogDlogLevel level);

UtilityLogStatus UtilityLogInit(void);

UtilityLogStatus UtilityLogDeinit(void);

UtilityLogStatus UtilityLogWriteDLog(uint32_t module_id,
                                     UtilityLogDlogLevel level,
                                     const char *format, ...);

UtilityLogStatus UtilityLogWriteVDLog(uint32_t module_id,
                                      UtilityLogDlogLevel level,
                                      const char *format, va_list list);

UtilityLogStatus UtilityLogWriteELog(uint32_t module_id,
                                     UtilityLogElogLevel level,
                                     uint16_t event_id);

UtilityLogStatus UtilityLogForcedOutputToUart(const char *format, ...);

UtilityLogStatus UtilityLogWriteBulkDLog(
    uint32_t module_id, UtilityLogDlogLevel level, size_t size,
    const char *bulk_log, const UtilityLogNotificationCallback callback,
    void *user_data);

UtilityLogStatus UtilityLogWriteBulkDLogNonNotify(uint32_t module_id,
                                                  UtilityLogDlogLevel level,
                                                  size_t size,
                                                  const char *bulk_log);

UtilityLogStatus UtilityLogRegisterSetDLogLevelCallback(
    uint32_t module_id, UtilityLogSetDlogLevelCallback callback);

UtilityLogStatus UtilityLogUnregisterSetDLogLevelCallback(uint32_t module_id);

// Macro definition for Dlog
#define WRITE_DLOG_CRITICAL(module_id, format, ...)                    \
  UtilityLogWriteDLog(module_id, kUtilityLogDlogLevelCritical, format, \
                      ##__VA_ARGS__)
#define WRITE_DLOG_ERROR(module_id, format, ...)                    \
  UtilityLogWriteDLog(module_id, kUtilityLogDlogLevelError, format, \
                      ##__VA_ARGS__)
#define WRITE_DLOG_WARN(module_id, format, ...)                    \
  UtilityLogWriteDLog(module_id, kUtilityLogDlogLevelWarn, format, \
                      ##__VA_ARGS__)
#define WRITE_DLOG_INFO(module_id, format, ...)                    \
  UtilityLogWriteDLog(module_id, kUtilityLogDlogLevelInfo, format, \
                      ##__VA_ARGS__)
#define WRITE_DLOG_DEBUG(module_id, format, ...)                    \
  UtilityLogWriteDLog(module_id, kUtilityLogDlogLevelDebug, format, \
                      ##__VA_ARGS__)
#define WRITE_DLOG_TRACE(module_id, format, ...)                    \
  UtilityLogWriteDLog(module_id, kUtilityLogDlogLevelTrace, format, \
                      ##__VA_ARGS__)

// Macro definition for Elog
#define WRITE_ELOG_CRITICAL(module_id, event_id) \
  UtilityLogWriteELog(module_id, kUtilityLogElogLevelCritical, event_id)
#define WRITE_ELOG_ERROR(module_id, event_id) \
  UtilityLogWriteELog(module_id, kUtilityLogElogLevelError, event_id)
#define WRITE_ELOG_WARN(module_id, event_id) \
  UtilityLogWriteELog(module_id, kUtilityLogElogLevelWarn, event_id)
#define WRITE_ELOG_INFO(module_id, event_id) \
  UtilityLogWriteELog(module_id, kUtilityLogElogLevelInfo, event_id)
#define WRITE_ELOG_DEBUG(module_id, event_id) \
  UtilityLogWriteELog(module_id, kUtilityLogElogLevelDebug, event_id)
#define WRITE_ELOG_TRACE(module_id, event_id) \
  UtilityLogWriteELog(module_id, kUtilityLogElogLevelTrace, event_id)

// [Depricated] DO NOT USE. Only for old modules.
typedef void *UtilityLogHandle;
UtilityLogStatus UtilityLogOpen(uint32_t module_id, UtilityLogHandle *handle);
UtilityLogStatus UtilityLogClose(UtilityLogHandle handle);
UtilityLogStatus UtilityLogWriteDlog(UtilityLogHandle handle,
                                     UtilityLogDlogLevel level,
                                     const char *format, ...);
#define DLOG_ERROR(handle, format, ...) \
  UtilityLogWriteDlog(handle, kUtilityLogDlogLevelError, format, ##__VA_ARGS__)
#define DLOG_INFO(handle, format, ...) \
  UtilityLogWriteDlog(handle, kUtilityLogDlogLevelInfo, format, ##__VA_ARGS__)
#define DLOG_DEBUG(handle, format, ...) \
  UtilityLogWriteDlog(handle, kUtilityLogDlogLevelDebug, format, ##__VA_ARGS__)
#define DLOG_TRACE(handle, format, ...) \
  UtilityLogWriteDlog(handle, kUtilityLogDlogLevelTrace, format, ##__VA_ARGS__)

#endif  // UTILITY_LOG_H_
