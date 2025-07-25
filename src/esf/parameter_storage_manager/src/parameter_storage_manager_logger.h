/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef ESF_PARAMETER_STORAGE_MANAGER_SRC_PARAMETER_STORAGE_MANAGER_LOGGER_H_
#define ESF_PARAMETER_STORAGE_MANAGER_SRC_PARAMETER_STORAGE_MANAGER_LOGGER_H_

#include <inttypes.h>
#include <pthread.h>

#include "parameter_storage_manager/src/parameter_storage_manager_config.h"

#ifdef CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_LOG

#ifdef CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_UTILITY_LOG_ENABLE

#ifdef CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_UTILITY_LOG_STUB
#define UtilityLogWriteDLog PSM_UtilityLogWriteDLog
#define UtilityLogWriteVDLog PSM_UtilityLogWriteVDLog
#define UtilityLogWriteELog PSM_UtilityLogWriteELog

#include "parameter_storage_manager/src/stub/include/utility/utility_log.h"
#include "parameter_storage_manager/src/stub/include/utility/utility_log_module_id.h"

#else  // CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_UTILITY_LOG_STUB
#include "utility_log.h"
#include "utility_log_module_id.h"
#endif  // CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_UTILITY_LOG_STUB

// """Logs a message using the utility logging mechanism.
//
// Args:
//     fmt: The format string for the message.
//     ...: Variable arguments for the format string.
//
// Returns:
//     None. These macros expand to a utility logging call.
// """

#define ESF_PARAMETER_STORAGE_MANAGER_LOGGER_ERROR(fmt, ...) \
  (WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, fmt, ##__VA_ARGS__))

#define ESF_PARAMETER_STORAGE_MANAGER_LOGGER_WARN(fmt, ...) \
  (WRITE_DLOG_WARN(MODULE_ID_SYSTEM, fmt, ##__VA_ARGS__))

#define ESF_PARAMETER_STORAGE_MANAGER_LOGGER_INFO(fmt, ...) \
  (WRITE_DLOG_INFO(MODULE_ID_SYSTEM, fmt, ##__VA_ARGS__))

#define ESF_PARAMETER_STORAGE_MANAGER_LOGGER_DEBUG(fmt, ...) \
  (WRITE_DLOG_DEBUG(MODULE_ID_SYSTEM, fmt, ##__VA_ARGS__))

#define ESF_PARAMETER_STORAGE_MANAGER_LOGGER_TRACE(fmt, ...) \
  (WRITE_DLOG_TRACE(MODULE_ID_SYSTEM, fmt, ##__VA_ARGS__))

#define ESF_PARAMETER_STORAGE_MANAGER_EVENT_LOGGER_ERROR(event_id) \
  (WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, event_id))

#define ESF_PARAMETER_STORAGE_MANAGER_EVENT_LOGGER_WARN(event_id) \
  (WRITE_ELOG_WARN(MODULE_ID_SYSTEM, event_id))

#define ESF_PARAMETER_STORAGE_MANAGER_EVENT_LOGGER_INFO(event_id) \
  (WRITE_ELOG_INFO(MODULE_ID_SYSTEM, event_id))

#define ESF_PARAMETER_STORAGE_MANAGER_EVENT_LOGGER_DEBUG(event_id) \
  (WRITE_ELOG_DEBUG(MODULE_ID_SYSTEM, event_id))

#define ESF_PARAMETER_STORAGE_MANAGER_EVENT_LOGGER_TRACE(event_id) \
  (WRITE_ELOG_TRACE(MODULE_ID_SYSTEM, event_id))

#else  // CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_UTILITY_LOG_ENABLE

#include <stdio.h>

// """Logs a message using printf when utility logging is disabled.
//
// Args:
//     fmt: The format string for the message.
//     ...: Variable arguments for the format string.
//
// Returns:
//     None. These macros expand to printf calls with level-specific prefixes
//     (E:, W:, I:, D:, T:).
// """

#define ESF_PARAMETER_STORAGE_MANAGER_LOGGER_ERROR(fmt, ...) \
  (printf("E:" fmt "\n", ##__VA_ARGS__))

#define ESF_PARAMETER_STORAGE_MANAGER_LOGGER_WARN(fmt, ...) \
  (printf("W:" fmt "\n", ##__VA_ARGS__))

#define ESF_PARAMETER_STORAGE_MANAGER_LOGGER_INFO(fmt, ...) \
  (printf("I:" fmt "\n", ##__VA_ARGS__))

#define ESF_PARAMETER_STORAGE_MANAGER_LOGGER_DEBUG(fmt, ...) \
  (printf("D:" fmt "\n", ##__VA_ARGS__))

#define ESF_PARAMETER_STORAGE_MANAGER_LOGGER_TRACE(fmt, ...) \
  (printf("T:" fmt "\n", ##__VA_ARGS__))

#define ESF_PARAMETER_STORAGE_MANAGER_EVENT_LOGGER_ERROR(event_id) \
  (printf("E:%04" PRIx16 "\n", (unsigned int)event_id))

#define ESF_PARAMETER_STORAGE_MANAGER_EVENT_LOGGER_WARN(event_id) \
  (printf("W:%04" PRIx16 "\n", (unsigned int)event_id))

#define ESF_PARAMETER_STORAGE_MANAGER_EVENT_LOGGER_INFO(event_id) \
  (printf("I:%04" PRIx16 "\n", (unsigned int)event_id))

#define ESF_PARAMETER_STORAGE_MANAGER_EVENT_LOGGER_DEBUG(event_id) \
  (printf("D:%04" PRIx16 "\n", (unsigned int)event_id))

#define ESF_PARAMETER_STORAGE_MANAGER_EVENT_LOGGER_TRACE(event_id) \
  (printf("T:%04" PRIx16 "\n", (unsigned int)event_id))

#endif  // CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_UTILITY_LOG_ENABLE

// """Format specifier for printing pthread IDs.
//
// This macro defines the format specifier to be used when printing pthread IDs.
// It uses the PRIuPTR macro from <inttypes.h> to ensure portability across
// different platforms and architectures.
//
// Returns:
//     A string literal containing the format specifier for pthread IDs.
// """
#define ESF_PARAMETER_STORAGE_MANAGER_PTHREAD_ID_FORMAT "%" PRIuPTR

// """Converts a pthread ID to a value suitable for printing.
//
// This macro casts the given pthread ID to uintptr_t, which is an unsigned
// integer type capable of holding a pointer. This conversion ensures that
// the pthread ID can be safely printed using the
// ESF_PARAMETER_STORAGE_MANAGER_PTHREAD_ID_FORMAT specifier.
//
// Args:
//     id: The pthread ID to be converted.
//
// Returns:
//     The pthread ID cast to uintptr_t.
// """
#define ESF_PARAMETER_STORAGE_MANAGER_PTHREAD_ID_VALUE(id) (uintptr_t)(id)

// """Formats a log message with pthread ID, function name, and line number.
//
// This macro creates a formatted log message that includes the pthread ID of
// the calling thread, the function name, and the line number where the log was
// called. It then passes this formatted message to the specified logger
// function.
//
// Args:
//     logger: The logging function to use.
//     fmt: The format string for the log message.
//     ...: Variable length argument list for the format string.
//
// Returns:
//     None. This macro expands to a call to the logger function with the
//     formatted message.
//
// Note:
//     This macro uses pthread_self() to get the current thread ID, and the
//     ESF_PARAMETER_STORAGE_MANAGER_PTHREAD_ID_FORMAT and
//     ESF_PARAMETER_STORAGE_MANAGER_PTHREAD_ID_VALUE macros to format the
//     thread ID.
// """
#define ESF_PARAMETER_STORAGE_MANAGER_LOG_FORMAT(logger, fmt, ...)          \
  (logger("%s-%d:" ESF_PARAMETER_STORAGE_MANAGER_PTHREAD_ID_FORMAT ":" fmt, \
          __FILE__, __LINE__,                                               \
          ESF_PARAMETER_STORAGE_MANAGER_PTHREAD_ID_VALUE(pthread_self()),   \
          ##__VA_ARGS__))

#endif  // CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_LOG

#endif  // ESF_PARAMETER_STORAGE_MANAGER_SRC_PARAMETER_STORAGE_MANAGER_LOGGER_H_
