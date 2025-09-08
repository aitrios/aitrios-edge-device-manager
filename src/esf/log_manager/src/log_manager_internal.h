/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESF_LOG_MANAGER_LOG_MANAGER_INTERNAL_H_
#define ESF_LOG_MANAGER_LOG_MANAGER_INTERNAL_H_

#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <syslog.h>

#include "log_manager.h"

#ifndef LOG_MANAGER_REMOVE_STATIC
#define STATIC static
#else  // LOG_MANAGER_REMOVE_STATIC
#define STATIC
#endif  // LOG_MANAGER_REMOVE_STATIC

#if !defined(__FILE_NAME__)
#define __FILE_NAME__ \
  (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#define ESF_LOG_MANAGER_INTERNAL_LEVEL_CRITICAL (0)
#define ESF_LOG_MANAGER_INTERNAL_LEVEL_ERROR (1)
#define ESF_LOG_MANAGER_INTERNAL_LEVEL_WARNING (2)
#define ESF_LOG_MANAGER_INTERNAL_LEVEL_INFO (3)
#define ESF_LOG_MANAGER_INTERNAL_LEVEL_DEBUG (4)
#define ESF_LOG_MANAGER_INTERNAL_LEVEL_TRACE (5)

#ifdef LOG_MANAGER_LOCAL_BUILD
#define LOG_MANAGER_TRACE_PRINT(fmt, ...)                                \
  EsfLogManagerInternalErrorOutput(ESF_LOG_MANAGER_INTERNAL_LEVEL_TRACE, \
                                   __FILE_NAME__, __LINE__, fmt,         \
                                   ##__VA_ARGS__)
#else /* LOG_MANAGER_LOCAL_BUILD */
#define LOG_MANAGER_TRACE_PRINT(...)
#endif /* LOG_MANAGER_LOCAL_BUILD */

// #define ESF_LOG_MANAGER_USE_ENCRYPT

#define ESF_LOG_MANAGER_INTERNAL_INVALID_THREAD_ID ((pthread_t) - 1)
#define ESF_LOG_MANAGER_INTERNAL_NON_MODULE_ID (0)

// """Performs Log Manager internal initialization list.
// Args:
//    no arguments
// Returns:
//    no returns.
// """
void EsfLogManagerInternalInitializeList(void);
// """Performs Log Manager internal initialization process.
// Args:
//    no arguments
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination.
// """
EsfLogManagerStatus EsfLogManagerInternalSetup(void);
// """DLog accumulation processing is performed.
// Args:
//    *str(uint8_t): String to be logged
//    size(uint32_t): Size of string to be logged
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination.
// """
EsfLogManagerStatus EsfLogManagerInternalWriteDlog(const uint8_t *str,
                                                   uint32_t size,
                                                   bool is_critical);

// """Terminate the Log manager internal.
// Args:
//    no arguments
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination.
EsfLogManagerStatus EsfLogManagerInternalDeinit(void);
// """Obtains log setting information from the Paramater storage manager.
// Args:
//    no arguments
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination.
EsfLogManagerStatus EsfLogManagerInternalLoadParameter(void);
// """Initializes a bytebuffer.
// Args:
//    no arguments
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination.
EsfLogManagerStatus EsfLogManagerInternalInitializeByteBuffer(void);
// """Terminate the bytebuffer.
// Args:
//    no arguments
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination.
EsfLogManagerStatus EsfLogManagerInternalDeinitByteBuffer(void);
// """Gets the size and number of each buffer
// Args:
//    log_info(EsfLogManagerLogInfo): Structure that stores log information
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination.
bool EsfLogManagerInternalGetLogInfo(EsfLogManagerLogInfo *const log_info);
// """Saves the parameter settings for the specified group.
// Args:
//    block_type(EsfLogManagerSettingBlockType): Blocks to be parameterized
//    value(EsfLogManagerParameterValue): The value to set for the parameter
//    mask(EsfLogManagerParameterMask): Mask value of the item to set the
//    parameter
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination.
EsfLogManagerStatus EsfLogManagerInternalSetParameter(
    EsfLogManagerSettingBlockType const block_type,
    EsfLogManagerParameterValue const *value,
    EsfLogManagerParameterMask const *mask);
// """Gets the parameter settings for the specified group.
// Args:
//    block_type(EsfLogManagerSettingBlockType): Group to get log setting
//    information
//    value(EsfLogManagerParameterValue): The value to set for the
//    parameter
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination.
EsfLogManagerStatus EsfLogManagerInternalGetParameter(
    EsfLogManagerSettingBlockType const block_type,
    EsfLogManagerParameterValue *value);
// """Retrieve the group ID of the specified module ID".
// Args:
//    module_id(uint32_t): Module ID to retrieve group
// Returns:
//    A value of 0 or greater: found
//    -1: not found
EsfLogManagerSettingBlockType EsfLogManagerInternalGetGroupID(
    uint32_t module_id);

void EsfLogManagerInternalChangeDlogCallback(
    EsfLogManagerSettingBlockType const block_type);

// """The callback registration process for notification when Dlog settings
//    are changed.
// Args:
//    module_id(uint32_t): Module ID to call
//    *callback(EsfLogManagerChangeDlogCallback): Callback notification function
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination.
EsfLogManagerStatus EsfLogManagerInternalRegisterChangeDlogCallback(
    uint32_t module_id, const EsfLogManagerChangeDlogCallback *const callback);
// """Unregister the callback for notification when Dlog settings are changed.
// Args:
//    module_id(uint32_t): Module ID to unregister callback
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination.
EsfLogManagerStatus EsfLogManagerInternalUnregisterChangeDlogCallback(
    uint32_t module_id);

EsfLogManagerStatus EsfLogManagerInternalSendBulkDlog(
    uint32_t module_id, size_t size, uint8_t *bulk_log,
    EsfLogManagerBulkDlogCallback callback, void *user_data);

EsfLogManagerStatus EsfLogManagerInternalSendElog(
    const EsfLogManagerElogMessage *message);

EsfLogManagerStatus EsfLogManagerInternalClearDlogCallbackList(void);

EsfLogManagerStatus EsfLogManagerInternalClearDlogList(void);

// """ Clear Elog Message
// Args:
//    no arguments
// Returns:
//    kEsfLogManagerStatusOk: success.
//    kEsfLogManagerStatusFailed: abnormal termination.
EsfLogManagerStatus EsfLogManagerInternalClearElog(void);

// """Error Output
// Args:
//    level(uint16_t): Error level
//    *func(const char): function name
//    line(int): File line
//    *format(char): Format string
//    ...:Arguments to output
// Returns:
//    no return

void EsfLogManagerInternalErrorOutput(uint16_t level, const char *func,
                                      int line, const char *format, ...);

//  Specifying a macro when outputting an error
#define ESF_LOG_MANAGER_ERROR(fmt, ...)                                  \
  EsfLogManagerInternalErrorOutput(ESF_LOG_MANAGER_INTERNAL_LEVEL_ERROR, \
                                   __FILE_NAME__, __LINE__, fmt,         \
                                   ##__VA_ARGS__)
#define ESF_LOG_MANAGER_INFO(fmt, ...)                                  \
  EsfLogManagerInternalErrorOutput(ESF_LOG_MANAGER_INTERNAL_LEVEL_INFO, \
                                   __FILE_NAME__, __LINE__, fmt,        \
                                   ##__VA_ARGS__)
#define ESF_LOG_MANAGER_DEBUG(fmt, ...)                                  \
  EsfLogManagerInternalErrorOutput(ESF_LOG_MANAGER_INTERNAL_LEVEL_DEBUG, \
                                   __FILE_NAME__, __LINE__, fmt,         \
                                   ##__VA_ARGS__)

#endif  // ESF_LOG_MANAGER_LOG_MANAGER_INTERNAL_H_
