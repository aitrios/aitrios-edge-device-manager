/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef ESF_PARAMETER_STORAGE_MANAGER_SRC_PARAMETER_STORAGE_MANAGER_LOG_H_
#define ESF_PARAMETER_STORAGE_MANAGER_SRC_PARAMETER_STORAGE_MANAGER_LOG_H_

#include "parameter_storage_manager/src/parameter_storage_manager_config.h"

#ifdef CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_LOG

#include "parameter_storage_manager/src/parameter_storage_manager_logger.h"

// """Logs an error message.
//
// This macro is conditionally compiled based on
// CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_LOG_ERROR. When enabled, it expands
// to a call to the error logging function with a formatted message. When
// disabled, it expands to nothing, effectively removing all error log
// statements.
//
// Args:
//     fmt: The format string for the error message.
//     ...: Variable arguments for the format string.
//
// Returns:
//     None. This macro expands to an error logging statement.
//
// Note:
//     The actual behavior depends on the
//     CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_LOG_ERROR flag.
// """
#ifdef CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_LOG_ERROR
#define ESF_PARAMETER_STORAGE_MANAGER_ERROR(fmt, ...) \
  (ESF_PARAMETER_STORAGE_MANAGER_LOG_FORMAT(          \
      ESF_PARAMETER_STORAGE_MANAGER_LOGGER_ERROR, fmt, ##__VA_ARGS__))
#endif  // CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_LOG_ERROR

// """Logs a warning message.
//
// This macro is conditionally compiled based on
// CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_LOG_WARN. When enabled, it expands
// to a call to the warning logging function with a formatted message. When
// disabled, it expands to nothing, effectively removing all warning log
// statements.
//
// Args:
//     fmt: The format string for the warn message.
//     ...: Variable arguments for the format string.
//
// Returns:
//     None. This macro expands to a warning logging statement.
//
// Note:
//     The actual behavior depends on the
//     CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_LOG_WARN flag.
// """
#ifdef CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_LOG_WARN
#define ESF_PARAMETER_STORAGE_MANAGER_WARN(fmt, ...) \
  (ESF_PARAMETER_STORAGE_MANAGER_LOG_FORMAT(         \
      ESF_PARAMETER_STORAGE_MANAGER_LOGGER_WARN, fmt, ##__VA_ARGS__))
#endif  // CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_LOG_WARN

// """Logs an info message.
//
// This macro is conditionally compiled based on
// CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_LOG_INFO. When enabled, it expands
// to a call to the info logging function with a formatted message. When
// disabled, it expands to nothing, effectively removing all info log
// statements.
//
// Args:
//     fmt: The format string for the info message.
//     ...: Variable arguments for the format string.
//
// Returns:
//     None. This macro expands to an info logging statement.
//
// Note:
//     The actual behavior depends on the
//     CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_LOG_INFO flag.
// """
#ifdef CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_LOG_INFO
#define ESF_PARAMETER_STORAGE_MANAGER_INFO(fmt, ...) \
  (ESF_PARAMETER_STORAGE_MANAGER_LOG_FORMAT(         \
      ESF_PARAMETER_STORAGE_MANAGER_LOGGER_INFO, fmt, ##__VA_ARGS__))
#endif  // CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_LOG_INFO

// """Logs a debug message.
//
// This macro is conditionally compiled based on
// CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_LOG_DEBUG. When enabled, it expands
// to a call to the debug logging function with a formatted message. When
// disabled, it expands to nothing, effectively removing all debug log
// statements.
//
// Args:
//     fmt: The format string for the debug message.
//     ...: Variable arguments for the format string.
//
// Returns:
//     None. This macro expands to a debug logging statement.
//
// Note:
//     The actual behavior depends on the
//     CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_LOG_DEBUG flag.
// """
#ifdef CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_LOG_DEBUG
#define ESF_PARAMETER_STORAGE_MANAGER_DEBUG(fmt, ...) \
  (ESF_PARAMETER_STORAGE_MANAGER_LOG_FORMAT(          \
      ESF_PARAMETER_STORAGE_MANAGER_LOGGER_DEBUG, fmt, ##__VA_ARGS__))
#endif  // CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_LOG_DEBUG

// """Logs a trace message.
//
// This macro is conditionally compiled based on
// CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_LOG_TRACE. When enabled, it expands
// to a call to the trace logging function with a formatted message. When
// disabled, it expands to nothing, effectively removing all trace log
// statements.
//
// Args:
//     fmt: The format string for the trace message.
//     ...: Variable arguments for the format string.
//
// Returns:
//     None. This macro expands to a trace logging statement.
//
// Note:
//     The actual behavior depends on the
//     CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_LOG_TRACE flag.
// """
#ifdef CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_LOG_TRACE
#define ESF_PARAMETER_STORAGE_MANAGER_TRACE(fmt, ...) \
  (ESF_PARAMETER_STORAGE_MANAGER_LOG_FORMAT(          \
      ESF_PARAMETER_STORAGE_MANAGER_LOGGER_TRACE, fmt, ##__VA_ARGS__))
#endif  // CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_LOG_TRACE

#endif  // CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_LOG

// """Placeholder for the logging when logging is disabled.
//
// These macros are defined as empty when
// CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_LOG is not set, effectively
// disabling all log outputs.
//
// Args:
//     fmt: The format string for the message (ignored).
//     ...: Variable arguments for the format string (ignored).
//
// Returns:
//     None. These macros expand to nothing when logging is disabled.
// """
#ifndef ESF_PARAMETER_STORAGE_MANAGER_ERROR
#define ESF_PARAMETER_STORAGE_MANAGER_ERROR(fmt, ...)
#endif
#ifndef ESF_PARAMETER_STORAGE_MANAGER_WARN
#define ESF_PARAMETER_STORAGE_MANAGER_WARN(fmt, ...)
#endif
#ifndef ESF_PARAMETER_STORAGE_MANAGER_INFO
#define ESF_PARAMETER_STORAGE_MANAGER_INFO(fmt, ...)
#endif
#ifndef ESF_PARAMETER_STORAGE_MANAGER_DEBUG
#define ESF_PARAMETER_STORAGE_MANAGER_DEBUG(fmt, ...)
#endif
#ifndef ESF_PARAMETER_STORAGE_MANAGER_TRACE
#define ESF_PARAMETER_STORAGE_MANAGER_TRACE(fmt, ...)
#endif

#endif  // ESF_PARAMETER_STORAGE_MANAGER_SRC_PARAMETER_STORAGE_MANAGER_LOG_H_
