/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "utility_log.h"

#include <malloc.h>
#if defined(__NuttX__)
#include <nuttx/arch.h>
#endif
#include <inttypes.h>
#include <pthread.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>

#include "log_manager.h"
#include "utility_log_definitions.h"

#ifndef UTILITY_LOG_REMOVE_STATIC
#define STATIC static
#else  // UTILITY_LOG_REMOVE_STATIC
#include "utility_log_static.h"
#define STATIC
#endif  // UTILITY_LOG_REMOVE_STATIC

#include "utility_log_module_id.h"

#if !defined(__NuttX__)
/* Wrapping up_puts() and get_errno() */
#include "internal/compatibility.h"
#endif

/****************************************************************************
 * private Data
 ****************************************************************************/

// This variable represents the state transition of UtilityLog.
STATIC UtilityLogState static_log_state = kUtilityLogStateInactive;

// This variable is a table of dlog params.
STATIC UtilityLogModuleData static_module_data_list[LOG_SUM_OF_MODULE_ID] = {
    {.module_id = MODULE_ID_SYSTEM,
     .params.dlog_dest = kUtilityLogDlogDestUart,
     .params.dlog_level = kUtilityLogDlogLevelInfo,
     .params.dlog_filter = LOG_FILTER_NONE,
     .callback = NULL},
    {.module_id = MODULE_ID_SENSOR,
     .params.dlog_dest = kUtilityLogDlogDestUart,
     .params.dlog_level = kUtilityLogDlogLevelInfo,
     .params.dlog_filter = LOG_FILTER_NONE,
     .callback = NULL},
    {.module_id = MODULE_ID_AI_ISP,
     .params.dlog_dest = kUtilityLogDlogDestUart,
     .params.dlog_level = kUtilityLogDlogLevelInfo,
     .params.dlog_filter = LOG_FILTER_NONE,
     .callback = NULL},
    {.module_id = MODULE_ID_VIC_APP,
     .params.dlog_dest = kUtilityLogDlogDestUart,
     .params.dlog_level = kUtilityLogDlogLevelInfo,
     .params.dlog_filter = LOG_FILTER_NONE,
     .callback = NULL}};

// This variable is a mutex object and is used for exclusive control of the
// static_handle_list.
static pthread_mutex_t static_log_mutex = PTHREAD_MUTEX_INITIALIZER;

// """Write to uart and Dlog.

// Write to uart and Dlog.

// Args:
//     level (UtilityLogDlogLevel): Dlog level.
//     format (const char *): Text of format.

// """
static void UtilityLogWriteDlogUart(UtilityLogDlogLevel level,
                                    const char *format, ...);

#ifdef CONFIG_UTILITY_LOG_ENABLE_SYSLOG
// """Transform the DLog level to the Syslog level.

// The following code converts the enum values of DLog level to Syslog level
// settings.

// Args:
//     dlog_level (UtilityLogDlogLevel): Dlog level.

// Returns:
//     uint16_t: Syslog level. The definition of this code can be found in
//       syslog.h.

// """
STATIC uint16_t UtilityLogDlogLevel2SyslogLevel(UtilityLogDlogLevel dlog_level);
#endif  // CONFIG_UTILITY_LOG_ENABLE_SYSLOG

#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
// """Transform the LogManager DLog level to the UtilityLog DLog level.

// The following code converts the enum values of LogManager DLog level to
// UtilityLog DLog level settings.

// Args:
//     log_manager_dlog_level (EsfLogManagerDlogLevel): LogManager Dlog level.

// Returns:
//     UtilityLogDlogLevel: UtilityLog Dlog level.

// """
STATIC UtilityLogDlogLevel
UtilityLogConvertDlogLevel(EsfLogManagerDlogLevel log_manager_dlog_level);
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

// """Transform the UtilityLog DLog level to the LogManager DLog level.

// The following code converts the enum values of UtilityLog DLog level to
// LogManager DLog level settings.

// Args:
//     utility_log_elog_level (UtilityLogElogLevel): UtilityLog Dlog level.

// Returns:
//     EsfLogManagerElogLevel: LogManager Dlog level.

// """
STATIC EsfLogManagerElogLevel
UtilityLogConvertElogLevel(UtilityLogElogLevel utility_log_elog_level);

#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
// """Transform the LogManager DLog dest to the UtilityLog DLog dest.

// The following code converts the enum values of LogManager DLog dest to
// UtilityLog DLog dest settings.

// Args:
//     log_manager_dlog_dest (EsfLogManagerDlogDest): LogManager Dlog dest.

// Returns:
//     UtilityLogDlogDest: UtilityLog Dlog dest.

// """
STATIC UtilityLogDlogDest
UtilityLogConvertDlogDest(EsfLogManagerDlogDest log_manager_dlog_dest);
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

// """To create a time string for inclusion in DLog.

// To create a time string for inclusion in DLog.

// Args:
//     str_buf (char *): string buffer.
//     str_buf_len (uint32_t): length of string buffer.

// Returns:
//     uint32_t: Returns the length of the string written to the buffer. Returns
//       0 if str_buf is null.

// """
STATIC uint32_t UtilityLogCreateTimeString(char *str_buf, uint32_t str_buf_len);

// """Validate the utility log state.

// To check if ensure that utility log is active.

// Returns:
//     UtilityLogStatus: The code returns one of the values
//       UtilityLogStatus depending on the execution result.
//       Returns the value kUtilityLogStatusOk if the process is successful.

// Raises:
//     kUtilityLogStatusFailed: State is inactive.

// """
static UtilityLogStatus UtilityLogValidateState(void);

// """DLog writing process internal function.

// DLog writing process internal function. If log string creation and output
// destination is "kUtilityLogDlogDestUart", call Syslog. If
// "kUtilityLogDlogDestStore", request log accumulation to Log_Manager. If
// "kUtilityLogDlogDestBoth", do both.

// Args:
//     module_id (uint32_t): module id.
//     level (UtilityLogDlogLevel): level of DLog.
//     dlog_dest (EsfLogManagerDlogDest): dest of Dlog.
//     format (const char *): Text of format.
//     list (va_list): Provide a variable length argument for the format.

// Returns:
//     UtilityLogStatus: The code returns one of the values
//       UtilityLogStatus depending on the execution result.
//       Returns the value kUtilityLogStatusOk if the process is successful.

// Raises:
//     kUtilityLogStatusFailed: When a log accumulation request to Log_Manager
//       fails.
//     kUtilityLogStatusParamError: format is null.

// """
STATIC UtilityLogStatus UtilityLogWriteDlogInternal(
    uint32_t module_id, UtilityLogDlogLevel level, UtilityLogDlogDest dlog_dest,
    const char *format, va_list list);

#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
// """Clean up module data list.

// Clean up module data list. Set default paramater.

// """
static void UtilityLogCleanUpModuleDataList(void);

// """Register the notification dlog params callback.

// Register a callback to receive notifications when LogManager receives
// parameter changes.

// """
static void UtilityLogRegisterNotificationDlogParamsCallback(void);

// """Unregister the notification dlog params callback.

// Unregister a callback to stop receiving notifications when LogManager
// receives parameter changes

// """
static void UtilityLogUnregisterNotificationDlogParamsCallback(void);

// """

// Update the internal dlog parameters with the Dlog parameters received from
// LogManager, and execute if a level-setting callback is registered.

// Args:
//     info (EsfLogManagerDlogChangeInfo): The dlog params infomation from
//       LogManager.

// """
STATIC void UtilityLogUpdateDlogCallback(EsfLogManagerDlogChangeInfo *info);
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

/****************************************************************************
 * Private Functions
 ****************************************************************************/
#if 0
// This macro is defined to validate module id.
// Step1: Check if it is the lower 4 bits.
// Step2: Check if it is a power of 2.
// Caution: Modify the logic if the module ID increases.
// clang-format off
#define IS_VALID_MODULE_ID(x) \
  ((((x) & ~0x0000000F) == 0) && ((x) & ((x) - 1)) == 0)
// clang-format on

// This macro is defined to count trailing zeros.
#define CONVERT_BIT_TO_INDEX(i) (int32_t)__builtin_ctz(i)
#else
// clang-format off
#define IS_VALID_MODULE_ID(x) \
  ((((x) & ~0x00F00000) == 0) && ((x) & ((x) - 1)) == 0)
// clang-format on

#define CONVERT_BIT_TO_INDEX(i) (int32_t)__builtin_ctz((i >> 20))
#endif

static void UtilityLogWriteDlogUart(UtilityLogDlogLevel level,
                                    const char *format, ...) {
  va_list list;
  va_start(list, format);
  UtilityLogWriteDlogInternal(MODULE_ID_SYSTEM, level, kUtilityLogDlogDestUart,
                              format, list);
  va_end(list);
}
// This macro is defined to notify errors that occur in the utility log.
#define UTILITY_LOG_ERROR(fmt, ...)                                \
  UtilityLogWriteDlogUart(kUtilityLogDlogLevelError, "%s-%d:" fmt, \
                          "utility_log.c", __LINE__, ##__VA_ARGS__)

#ifdef CONFIG_UTILITY_LOG_ENABLE_SYSLOG
STATIC uint16_t
UtilityLogDlogLevel2SyslogLevel(UtilityLogDlogLevel dlog_level) {
  uint16_t level = LOG_DEBUG;
  switch (dlog_level) {
    case kUtilityLogDlogLevelCritical:
      level = LOG_CRIT;
      break;
    case kUtilityLogDlogLevelError:
      level = LOG_ERR;
      break;
    case kUtilityLogDlogLevelWarn:
      level = LOG_WARNING;
      break;
    case kUtilityLogDlogLevelInfo:
      level = LOG_INFO;
      break;
    case kUtilityLogDlogLevelDebug:
      level = LOG_DEBUG;
      break;
    case kUtilityLogDlogLevelTrace:
      level = LOG_DEBUG;
      break;
    default:
      level = LOG_INFO;
      break;
  }
  return level;
}
#endif  // CONFIG_UTILITY_LOG_ENABLE_SYSLOG

#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
STATIC UtilityLogDlogLevel
UtilityLogConvertDlogLevel(EsfLogManagerDlogLevel log_manager_dlog_level) {
  UtilityLogDlogLevel level = kUtilityLogDlogLevelDebug;
  switch (log_manager_dlog_level) {
    case kEsfLogManagerDlogLevelCritical:
      level = kUtilityLogDlogLevelCritical;
      break;
    case kEsfLogManagerDlogLevelError:
      level = kUtilityLogDlogLevelError;
      break;
    case kEsfLogManagerDlogLevelWarn:
      level = kUtilityLogDlogLevelWarn;
      break;
    case kEsfLogManagerDlogLevelInfo:
      level = kUtilityLogDlogLevelInfo;
      break;
    case kEsfLogManagerDlogLevelDebug:
      level = kUtilityLogDlogLevelDebug;
      break;
    case kEsfLogManagerDlogLevelTrace:
      level = kUtilityLogDlogLevelTrace;
      break;
    default:
      level = kUtilityLogDlogLevelInfo;
      break;
  }
  return level;
}
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

STATIC EsfLogManagerElogLevel
UtilityLogConvertElogLevel(UtilityLogElogLevel utility_log_elog_level) {
  EsfLogManagerElogLevel level = kEsfLogManagerElogLevelDebug;
  switch (utility_log_elog_level) {
    case kUtilityLogElogLevelCritical:
      level = kEsfLogManagerElogLevelCritical;
      break;
    case kUtilityLogElogLevelError:
      level = kEsfLogManagerElogLevelError;
      break;
    case kUtilityLogElogLevelWarn:
      level = kEsfLogManagerElogLevelWarn;
      break;
    case kUtilityLogElogLevelInfo:
      level = kEsfLogManagerElogLevelInfo;
      break;
    case kUtilityLogElogLevelDebug:
      level = kEsfLogManagerElogLevelDebug;
      break;
    case kUtilityLogElogLevelTrace:
      level = kEsfLogManagerElogLevelTrace;
      break;
    default:
      level = kEsfLogManagerElogLevelInfo;
      break;
  }
  return level;
}

#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
STATIC UtilityLogDlogDest
UtilityLogConvertDlogDest(EsfLogManagerDlogDest log_manager_dlog_dest) {
  UtilityLogDlogDest dest = kUtilityLogDlogDestUart;
  switch (log_manager_dlog_dest) {
    case kEsfLogManagerDlogDestUart:
      dest = kUtilityLogDlogDestUart;
      break;
    case kEsfLogManagerDlogDestStore:
      dest = kUtilityLogDlogDestStore;
      break;
    case kEsfLogManagerDlogDestBoth:
      dest = kUtilityLogDlogDestBoth;
      break;
    default:
      dest = kUtilityLogDlogDestUart;
      break;
  }
  return dest;
}
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

STATIC uint32_t UtilityLogCreateTimeString(char *str_buf,
                                           uint32_t str_buf_len) {
  if ((str_buf == (char *)NULL) ||
      (str_buf_len < (LOG_TIMESTAMP_SIZE + LOG_STRING_NULL_TERMINATION_SIZE))) {
    UTILITY_LOG_ERROR(
        "Invalid paramater. Please ensure that the buffer is not null and set "
        "its size to be at least %d. str_buf=%p str_buf_len=%d",
        (LOG_TIMESTAMP_SIZE + LOG_STRING_NULL_TERMINATION_SIZE), str_buf,
        str_buf_len);
    return 0;
  }

  // Create time string "<YYYY>-<MM>-<DD>T<HH>:<MM>:<SS>.<sss><(+or-)HH:MM>".
  // "T" means delimiter before the time.
  // "sss" means ms order.
  // "(+or-)HH:MM" means time zone offset. Currently, it is fixed to UTC, so it
  // is denoted as "Z".
  // For example : "1997-01-01T12:15:10.123Z"
  int len = 0;
  struct timespec ts = {0};
  struct tm time = {0};
  clock_gettime(CLOCK_REALTIME, &ts);

  memset(&time, 0, sizeof(time));

  gmtime_r(&ts.tv_sec, &time);  // Get UTC time.
  char time_str[64] = {0};
  len = snprintf(time_str, sizeof(time_str),
                 "%04d-%02d-%02dT%02d:%02d:%02d.%03ldZ", time.tm_year + 1900,
                 time.tm_mon + 1, time.tm_mday % 32, time.tm_hour % 24,
                 time.tm_min % 60, time.tm_sec % 60, ts.tv_nsec / 1000000);
  if (len < 0) {
    strncpy(str_buf, "1970-01-01T00:00:00.000Z", str_buf_len);
  } else {
    strncpy(str_buf, time_str, str_buf_len);
  }

  return (uint32_t)strlen(str_buf);
}

static UtilityLogStatus UtilityLogValidateState(void) {
  if (static_log_state == kUtilityLogStateInactive) {
    UTILITY_LOG_ERROR(
        "The utility log has not been initialized yet. Please refer to the "
        "UtilityLogInit function.");
    return kUtilityLogStatusFailed;
  }

  return kUtilityLogStatusOk;
}

STATIC UtilityLogStatus UtilityLogWriteDlogInternal(
    uint32_t module_id, UtilityLogDlogLevel level, UtilityLogDlogDest dlog_dest,
    const char *format, va_list list) {
  if ((format == (const char *)NULL) ||
      (level < kUtilityLogDlogLevelCritical) ||
      (kUtilityLogDlogLevelTrace < level)) {
    UTILITY_LOG_ERROR(
        "Invalid paramater. Format is null or level setting is incorrect. "
        "format=%p level=%d",
        format, level);
    return kUtilityLogStatusParamError;
  }

  char *log_str = (char *)calloc(LOG_STRING_SIZE, sizeof(char));
  if (log_str == NULL) {
    // When calling UTILITY_LOG_ERROR, it causes a loop with calloc error so I
    // don't call it.
    return kUtilityLogStatusFailed;
  }

  const char level_str[kUtilityLogDlogLevelNum] = {'C', 'E', 'W',
                                                   'I', 'D', 'T'};
  int32_t idx = 0;

  idx = UtilityLogCreateTimeString(log_str, LOG_STRING_SIZE);

  // Insert the log level character and module id.
  snprintf(log_str + idx, LOG_STRING_SIZE - idx,
           ":%c:0x%08X:", level_str[level], module_id);
  idx = strnlen(log_str, LOG_STRING_SIZE);

  // Insert the description.
  vsnprintf(log_str + idx, LOG_STRING_SIZE - idx, format, list);

  idx = strnlen(log_str, LOG_STRING_SIZE);
  // Insert cr code.
  if (log_str[idx - LOG_STRING_NULL_TERMINATION_SIZE] != '\n') {
    // If the string reaches its maximum length, insert a newline character so
    // that the last character gets overwritten.
    if (((LOG_STRING_SIZE - LOG_STRING_NULL_TERMINATION_SIZE) - idx) == 0) {
      log_str[idx - LOG_STRING_NULL_TERMINATION_SIZE] = '\n';
    } else {
      // Define newline code with warning prevention variable and then execute
      // strncat.
      const char cr_char = '\n';
      strncat(log_str, &cr_char, LOG_STRING_CR_SIZE);
      idx++;
    }
  }

#ifndef CONFIG_EXTERNAL_DLOG_DISABLE

  if ((dlog_dest == kUtilityLogDlogDestUart) ||
      (dlog_dest == kUtilityLogDlogDestBoth)) {
#ifdef CONFIG_UTILITY_LOG_ENABLE_SYSLOG
    int sys_level = UtilityLogDlogLevel2SyslogLevel(level);
    syslog(sys_level, "%s", log_str);
#else   // CONFIG_UTILITY_LOG_ENABLE_SYSLOG
    printf("%s", log_str);
#endif  // CONFIG_UTILITY_LOG_ENABLE_SYSLOG
  }
  if ((dlog_dest == kUtilityLogDlogDestStore) ||
      (dlog_dest == kUtilityLogDlogDestBoth)) {
    bool is_critical = (level == kUtilityLogDlogLevelCritical);
    EsfLogManagerStatus ret = EsfLogManagerStoreDlog((uint8_t *)log_str, idx,
                                                     is_critical);
    if (ret != kEsfLogManagerStatusOk) {
      UTILITY_LOG_ERROR("EsfLogManagerStoreDlog Failed. ret=%d", ret);
      free(log_str);
      return kUtilityLogStatusFailed;
    }
  }

#else  // CONFIG_EXTERNAL_DLOG_DISABLE
  // Unused argument.
  (void)dlog_dest;
  printf("%s", log_str);

#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

  free(log_str);
  return kUtilityLogStatusOk;
}

#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
static void UtilityLogCleanUpModuleDataList(void) {
  for (int32_t i = 0; i < LOG_SUM_OF_MODULE_ID; i++) {
    static_module_data_list[i].params.dlog_dest = kUtilityLogDlogDestUart;
    static_module_data_list[i].params.dlog_level = kUtilityLogDlogLevelInfo;
    static_module_data_list[i].params.dlog_filter = LOG_FILTER_NONE;
    static_module_data_list[i].callback = NULL;
  }
}

static void UtilityLogRegisterNotificationDlogParamsCallback(void) {
  for (int32_t i = 0; i < LOG_SUM_OF_MODULE_ID; i++) {
    EsfLogManagerStatus manager_ret = EsfLogManagerRegisterChangeDlogCallback(
        static_module_data_list[i].module_id, UtilityLogUpdateDlogCallback);
    if (manager_ret != kEsfLogManagerStatusOk) {
      UTILITY_LOG_ERROR(
          "Failed to register the callback with LogManager, so UtilityLog will "
          "operate with default parameters. group=%d ret=%d",
          i, manager_ret);
      // Use default parameters instead of returning an error when an error
      // occurs.
    }
  }
}

static void UtilityLogUnregisterNotificationDlogParamsCallback(void) {
  for (int32_t i = 0; i < LOG_SUM_OF_MODULE_ID; i++) {
    EsfLogManagerStatus manager_ret = EsfLogManagerUnregisterChangeDlogCallback(
        static_module_data_list[i].module_id);
    if (manager_ret != kEsfLogManagerStatusOk) {
      UTILITY_LOG_ERROR(
          "EsfLogManagerUnregisterChangeDlogCallback failed. group=%d ret=%d",
          i, manager_ret);
      // The LogManager's processing failure does not result in an error return.
      // This is because, when the LogManager might be closed, restarting only
      // the LogManager could lead to a discrepancy in the callback holding
      // information of UtilityLog, so it is essential to ensure the release of
      // UtilityLog's retained callback information.
    }
  }
}

STATIC void UtilityLogUpdateDlogCallback(EsfLogManagerDlogChangeInfo *info) {
  if (pthread_mutex_lock(&static_log_mutex) != 0) {
    UTILITY_LOG_ERROR("Mutex lock error.");
    return;
  }

  if (IS_VALID_MODULE_ID(info->module_id) == 0) {
    UTILITY_LOG_ERROR("Invalid id.");
    pthread_mutex_unlock(&static_log_mutex);
    return;
  }

  // Update Dlog parameters.
  int32_t index = CONVERT_BIT_TO_INDEX(info->module_id);
  UtilityLogDlogDest dlog_dest =
      UtilityLogConvertDlogDest(info->value.dlog_dest);
  static_module_data_list[index].params.dlog_dest = dlog_dest;
  UtilityLogDlogLevel dlog_level =
      UtilityLogConvertDlogLevel(info->value.dlog_level);
  static_module_data_list[index].params.dlog_level = dlog_level;
  static_module_data_list[index].params.dlog_filter = info->value.dlog_filter;
  // To rearrange for executing a callback after exiting the critical section.
  UtilityLogSetDlogLevelCallback callback =
      static_module_data_list[index].callback;

  if (pthread_mutex_unlock(&static_log_mutex) != 0) {
    UTILITY_LOG_ERROR("Mutex unlock error.");
    return;
  }

  if (callback != NULL) {
    // Notify newest Dlog level.
    callback(dlog_level);
  }
}
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

/****************************************************************************
 * Public Functions
 ****************************************************************************/

UtilityLogStatus UtilityLogInit(void) {
  if (pthread_mutex_lock(&static_log_mutex) != 0) {
    UTILITY_LOG_ERROR("Mutex lock error.");
    return kUtilityLogStatusFailed;
  }

  if (static_log_state == kUtilityLogStateActive) {
    UTILITY_LOG_ERROR("The utility log has already been initialized.");
    pthread_mutex_unlock(&static_log_mutex);
    return kUtilityLogStatusFailed;
  }

#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
  UtilityLogRegisterNotificationDlogParamsCallback();
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

  static_log_state = kUtilityLogStateActive;

  if (pthread_mutex_unlock(&static_log_mutex) != 0) {
    UTILITY_LOG_ERROR("Mutex unlock error.");
    return kUtilityLogStatusFailed;
  }

  return kUtilityLogStatusOk;
}

UtilityLogStatus UtilityLogDeinit(void) {
  if (pthread_mutex_lock(&static_log_mutex) != 0) {
    UTILITY_LOG_ERROR("Mutex lock error.");
    return kUtilityLogStatusFailed;
  }

  if (static_log_state == kUtilityLogStateInactive) {
    UTILITY_LOG_ERROR(
        "The utility log has not been initialized yet. Please refer to the "
        "UtilityLogInit function.");
    pthread_mutex_unlock(&static_log_mutex);
    return kUtilityLogStatusFailed;
  }

#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
  UtilityLogUnregisterNotificationDlogParamsCallback();
  UtilityLogCleanUpModuleDataList();
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

  static_log_state = kUtilityLogStateInactive;

  if (pthread_mutex_unlock(&static_log_mutex) != 0) {
    UTILITY_LOG_ERROR("Mutex unlock error.");
    return kUtilityLogStatusFailed;
  }

  return kUtilityLogStatusOk;
}

UtilityLogStatus UtilityLogWriteDLog(uint32_t module_id,
                                     UtilityLogDlogLevel level,
                                     const char *format, ...) {
  va_list list;
  va_start(list, format);
  UtilityLogStatus ret = UtilityLogWriteVDLog(module_id, level, format, list);
  va_end(list);

  return ret;
}

UtilityLogStatus UtilityLogWriteVDLog(uint32_t module_id,
                                      UtilityLogDlogLevel level,
                                      const char *format, va_list list) {
  if ((IS_VALID_MODULE_ID(module_id) == 0) ||
      (level < kUtilityLogDlogLevelCritical) ||
      (kUtilityLogDlogLevelTrace < level)) {
    UTILITY_LOG_ERROR(
        "Invalid paramater. The module id or level has an invalid value. "
        "module_id=0x%08X, level=%d",
        module_id, level);
    return kUtilityLogStatusParamError;
  }

  if ((format == (const char *)NULL) || (format[0] == '\0')) {
    // If format is NULL or an empty string, exit without returning an error.
    goto process_fin;
  }

  if (pthread_mutex_lock(&static_log_mutex) != 0) {
    UTILITY_LOG_ERROR("Mutex lock error.");
    return kUtilityLogStatusFailed;
  }

  UtilityLogStatus ret = UtilityLogValidateState();
  if (ret != kUtilityLogStatusOk) {
    UTILITY_LOG_ERROR("Invalid state error.");
    pthread_mutex_unlock(&static_log_mutex);
    return ret;
  }

  // Obtain the Dlog parameters corresponding to the group.
  UtilityLogParams log_param = {0};
  int32_t index = CONVERT_BIT_TO_INDEX(module_id);
  log_param.dlog_dest = static_module_data_list[index].params.dlog_dest;
  log_param.dlog_level = static_module_data_list[index].params.dlog_level;
  log_param.dlog_filter = static_module_data_list[index].params.dlog_filter;

  if (pthread_mutex_unlock(&static_log_mutex) != 0) {
    UTILITY_LOG_ERROR("Mutex unlock error.");
    return kUtilityLogStatusFailed;
  }

#ifndef CONFIG_EXTERNAL_DLOG_DISABLE

  if (level > log_param.dlog_level) {
    // If the argument 'level' is lower than the specified level, data will be
    // discarded.
    goto process_fin;
  }

  if (log_param.dlog_filter != LOG_FILTER_NONE) {
    if ((module_id & log_param.dlog_filter) != module_id) {
      // Filtered out items are not output.
      goto process_fin;
    }
  }

#else  // CONFIG_EXTERNAL_DLOG_DISABLE

  if (level > CONFIG_UTILITY_LOG_DEFAULT_DLOG_LEVEL) {
    // If the argument 'level' is lower than the specified level, data will be
    // discarded.
    goto process_fin;
  }

#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

  ret = UtilityLogWriteDlogInternal(module_id, level, log_param.dlog_dest,
                                    format, list);
  if (ret != kUtilityLogStatusOk) {
    UTILITY_LOG_ERROR("Write Dlog failed.");
    return kUtilityLogStatusFailed;
  }

process_fin:
  return kUtilityLogStatusOk;
}

UtilityLogStatus UtilityLogWriteELog(uint32_t module_id,
                                     UtilityLogElogLevel level,
                                     uint16_t event_id) {
  if ((IS_VALID_MODULE_ID(module_id) == 0) ||
      (level < kUtilityLogElogLevelCritical) ||
      (kUtilityLogElogLevelTrace < level)) {
    UTILITY_LOG_ERROR(
        "Invalid paramater. The module id or level has an invalid value. "
        "module_id=0x%08X, level=%d",
        module_id, level);
    return kUtilityLogStatusParamError;
  }

  if (pthread_mutex_lock(&static_log_mutex) != 0) {
    UTILITY_LOG_ERROR("Mutex lock error.");
    return kUtilityLogStatusFailed;
  }

  UtilityLogStatus ret = UtilityLogValidateState();
  if (ret != kUtilityLogStatusOk) {
    UTILITY_LOG_ERROR("Invalid state error.");
    pthread_mutex_unlock(&static_log_mutex);
    return ret;
  }

  if (pthread_mutex_unlock(&static_log_mutex) != 0) {
    UTILITY_LOG_ERROR("Mutex unlock error.");
    return kUtilityLogStatusFailed;
  }

#ifdef CONFIG_EXTERNAL_ELOG_DISABLE

  if (level > CONFIG_UTILITY_LOG_DEFAULT_ELOG_LEVEL) {
    // If the argument 'level' is lower than the specified level, data will be
    // discarded.
    return kUtilityLogStatusOk;
  }

#endif  // CONFIG_EXTERNAL_ELOG_DISABLE

  char time_str[ESF_LOG_DATATIME_SIZE] = {0};
  // Not use the returned index.
  (void)UtilityLogCreateTimeString(time_str, ESF_LOG_DATATIME_SIZE);

#ifndef CONFIG_EXTERNAL_ELOG_DISABLE
  EsfLogManagerElogMessage *message =
      (EsfLogManagerElogMessage *)malloc(sizeof(EsfLogManagerElogMessage));
  if (message == NULL) {
    // When calling UTILITY_LOG_ERROR, it causes a loop with calloc error so I
    // don't call it.
    return kUtilityLogStatusFailed;
  }
  // Store the parameters to the message.
  message->elog_level = UtilityLogConvertElogLevel(level);
  strncpy(message->time, time_str, sizeof(message->time) - 1);
  message->time[sizeof(message->time) - 1] = '\0';
  message->component_id = module_id;
  message->event_id = (uint32_t)event_id;

  EsfLogManagerStatus send_ret = EsfLogManagerSendElog(message);
  if (send_ret != kEsfLogManagerStatusOk) {
    free(message);
    //    UTILITY_LOG_ERROR("Send Elog failed.");
    return kUtilityLogStatusFailed;
  }
  free(message);
#endif  // CONFIG_EXTERNAL_ELOG_DISABLE

  printf("ELog: module_id=%u, level=%u, event_id=%u, time=%.*s\n", module_id,
         level, event_id, ESF_LOG_DATATIME_SIZE, time_str);

  return kUtilityLogStatusOk;
}

UtilityLogStatus UtilityLogForcedOutputToUart(const char *format, ...) {
  if ((format == (const char *)NULL) || (format[0] == '\0')) {
    // If format is NULL or an empty string, exit without returning an error.
    goto process_fin;
  }

  if (pthread_mutex_lock(&static_log_mutex) != 0) {
    UTILITY_LOG_ERROR("Mutex lock error.");
    return kUtilityLogStatusFailed;
  }

  UtilityLogStatus ret = UtilityLogValidateState();
  if (ret != kUtilityLogStatusOk) {
    UTILITY_LOG_ERROR("Invalid state error.");
    pthread_mutex_unlock(&static_log_mutex);
    return ret;
  }

  if (pthread_mutex_unlock(&static_log_mutex) != 0) {
    UTILITY_LOG_ERROR("Mutex unlock error.");
    return kUtilityLogStatusFailed;
  }

  char *log_str = (char *)calloc(
      (LOG_DESCRIPTION_MAX_SIZE + LOG_STRING_NULL_TERMINATION_SIZE),
      sizeof(char));
  if (log_str == NULL) {
    // When calling UTILITY_LOG_ERROR, it causes a loop with calloc error so I
    // don't call it.
    return kUtilityLogStatusFailed;
  }

  va_list list;
  va_start(list, format);
  vsnprintf(log_str,
            (LOG_DESCRIPTION_MAX_SIZE + LOG_STRING_NULL_TERMINATION_SIZE),
            format, list);
  up_puts(log_str);

  va_end(list);
  free(log_str);

process_fin:
  return kUtilityLogStatusOk;
}

UtilityLogStatus UtilityLogWriteBulkDLog(
    uint32_t module_id, UtilityLogDlogLevel level, size_t size,
    const char *bulk_log, const UtilityLogNotificationCallback callback,
    void *user_data) {
  if ((IS_VALID_MODULE_ID(module_id) == 0) ||
      (level < kUtilityLogDlogLevelCritical) ||
      (kUtilityLogDlogLevelTrace < level) ||
      (size > CONFIG_UTILITY_LOG_BULK_DLOG_MAX_SIZE) || (bulk_log == NULL) ||
      (callback == NULL)) {
    UTILITY_LOG_ERROR(
        "Invalid parameter. This includes one of the following: module id is "
        "invalid, level has an "
        "invalid value, size exceeds the maximum value, log "
        "pointer is NULL, or callback is NULL."
        "module_id=0x%08X, level=%d, size=%lu, bulk_log=%p, callback=%p",
        module_id, level, size, bulk_log, callback);
    return kUtilityLogStatusParamError;
  }

  if ((size == 0) || (bulk_log[0] == '\0')) {
    // If bulk_log is empty string, exit without returning an error.
    goto process_fin;
  }

  if (pthread_mutex_lock(&static_log_mutex) != 0) {
    UTILITY_LOG_ERROR("Mutex lock error.");
    return kUtilityLogStatusFailed;
  }

  UtilityLogStatus ret = UtilityLogValidateState();
  if (ret != kUtilityLogStatusOk) {
    UTILITY_LOG_ERROR("Invalid state error.");
    pthread_mutex_unlock(&static_log_mutex);
    return ret;
  }

#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
  // Obtain the Dlog parameters corresponding to the group.
  UtilityLogParams log_param = {0};
  int32_t index = CONVERT_BIT_TO_INDEX(module_id);
  log_param.dlog_dest = static_module_data_list[index].params.dlog_dest;
  log_param.dlog_level = static_module_data_list[index].params.dlog_level;
  log_param.dlog_filter = static_module_data_list[index].params.dlog_filter;
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

  if (pthread_mutex_unlock(&static_log_mutex) != 0) {
    UTILITY_LOG_ERROR("Mutex unlock error.");
    return kUtilityLogStatusFailed;
  }

#ifndef CONFIG_EXTERNAL_DLOG_DISABLE

  if (level > log_param.dlog_level) {
    // If the argument 'level' is lower than the specified level, data will be
    // discarded.
    goto process_fin;
  }

  if (log_param.dlog_filter != LOG_FILTER_NONE) {
    if ((module_id & log_param.dlog_filter) != module_id) {
      // Filtered out items are not output.
      goto process_fin;
    }
  }

  if ((log_param.dlog_dest == kUtilityLogDlogDestUart) ||
      (log_param.dlog_dest == kUtilityLogDlogDestBoth)) {
#ifdef CONFIG_UTILITY_LOG_ENABLE_SYSLOG
    int sys_level = UtilityLogDlogLevel2SyslogLevel(level);
    syslog(sys_level, "%.*s", LOG_DESCRIPTION_MAX_SIZE, bulk_log);
#else   // CONFIG_UTILITY_LOG_ENABLE_SYSLOG
    printf("%.*s", LOG_DESCRIPTION_MAX_SIZE, bulk_log);
#endif  // CONFIG_UTILITY_LOG_ENABLE_SYSLOG
    if (log_param.dlog_dest == kUtilityLogDlogDestUart) {
      callback(size, user_data);
    }
  }
  if ((log_param.dlog_dest == kUtilityLogDlogDestStore) ||
      (log_param.dlog_dest == kUtilityLogDlogDestBoth)) {
    EsfLogManagerStatus manager_ret = EsfLogManagerSendBulkDlog(
        module_id, size, (uint8_t *)bulk_log,
        (EsfLogManagerBulkDlogCallback)callback, user_data);
    if (manager_ret != kEsfLogManagerStatusOk) {
      UTILITY_LOG_ERROR("EsfLogManagerSendBulkDlog Failed. ret=%d",
                        manager_ret);
      return kUtilityLogStatusFailed;
    }
  }

#else  // CONFIG_EXTERNAL_DLOG_DISABLE
  // Unused arguments.
  (void)size;
  (void)callback;
  (void)user_data;

  if (level > CONFIG_UTILITY_LOG_DEFAULT_DLOG_LEVEL) {
    // If the argument 'level' is lower than the specified level, data will be
    // discarded.
    goto process_fin;
  }

  printf("%.*s", LOG_DESCRIPTION_MAX_SIZE, bulk_log);

#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

process_fin:
  return kUtilityLogStatusOk;
}

UtilityLogStatus UtilityLogWriteBulkDLogNonNotify(uint32_t module_id,
                                                  UtilityLogDlogLevel level,
                                                  size_t size,
                                                  const char *bulk_log) {
  if ((IS_VALID_MODULE_ID(module_id) == 0) ||
      (level < kUtilityLogDlogLevelCritical) ||
      (kUtilityLogDlogLevelTrace < level) ||
      (size > CONFIG_UTILITY_LOG_BULK_DLOG_MAX_SIZE) || (bulk_log == NULL)) {
    UTILITY_LOG_ERROR(
        "Invalid parameter. This includes one of the following: module id is "
        "invalid, level has an invalid value, size exceeds the maximum value "
        "or log pointer is NULL. module_id=0x%08X, level=%d, size=%lu, "
        "bulk_log=%p",
        module_id, level, size, bulk_log);
    return kUtilityLogStatusParamError;
  }

  if ((size == 0) || (bulk_log[0] == '\0')) {
    // If bulk_log is empty string, exit without returning an error.
    goto process_fin;
  }

  if (pthread_mutex_lock(&static_log_mutex) != 0) {
    UTILITY_LOG_ERROR("Mutex lock error.");
    return kUtilityLogStatusFailed;
  }

  UtilityLogStatus ret = UtilityLogValidateState();
  if (ret != kUtilityLogStatusOk) {
    UTILITY_LOG_ERROR("Invalid state error.");
    pthread_mutex_unlock(&static_log_mutex);
    return ret;
  }

#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
  // Obtain the Dlog parameters corresponding to the group.
  UtilityLogParams log_param = {0};
  int32_t index = CONVERT_BIT_TO_INDEX(module_id);
  log_param.dlog_dest = static_module_data_list[index].params.dlog_dest;
  log_param.dlog_level = static_module_data_list[index].params.dlog_level;
  log_param.dlog_filter = static_module_data_list[index].params.dlog_filter;
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

  if (pthread_mutex_unlock(&static_log_mutex) != 0) {
    UTILITY_LOG_ERROR("Mutex unlock error.");
    return kUtilityLogStatusFailed;
  }

#ifndef CONFIG_EXTERNAL_DLOG_DISABLE

  if (level > log_param.dlog_level) {
    // If the argument 'level' is lower than the specified level, data will be
    // discarded.
    goto process_fin;
  }

  if (log_param.dlog_filter != LOG_FILTER_NONE) {
    if ((module_id & log_param.dlog_filter) != module_id) {
      // Filtered out items are not output.
      goto process_fin;
    }
  }

  if ((log_param.dlog_dest == kUtilityLogDlogDestUart) ||
      (log_param.dlog_dest == kUtilityLogDlogDestBoth)) {
#ifdef CONFIG_UTILITY_LOG_ENABLE_SYSLOG
    int sys_level = UtilityLogDlogLevel2SyslogLevel(level);
    syslog(sys_level, "%.*s", LOG_DESCRIPTION_MAX_SIZE, bulk_log);
#else   // CONFIG_UTILITY_LOG_ENABLE_SYSLOG
    printf("%.*s", LOG_DESCRIPTION_MAX_SIZE, bulk_log);
#endif  // CONFIG_UTILITY_LOG_ENABLE_SYSLOG
  }
  if ((log_param.dlog_dest == kUtilityLogDlogDestStore) ||
      (log_param.dlog_dest == kUtilityLogDlogDestBoth)) {
    EsfLogManagerStatus manager_ret = EsfLogManagerSendBulkDlog(
        module_id, size, (uint8_t *)bulk_log, NULL, NULL);
    if (manager_ret != kEsfLogManagerStatusOk) {
      UTILITY_LOG_ERROR("EsfLogManagerSendBulkDlog Failed. ret=%d",
                        manager_ret);
      return kUtilityLogStatusFailed;
    }
  }

#else  // CONFIG_EXTERNAL_DLOG_DISABLE
  // Unused argument.
  (void)size;

  if (level > CONFIG_UTILITY_LOG_DEFAULT_DLOG_LEVEL) {
    // If the argument 'level' is lower than the specified level, data will be
    // discarded.
    goto process_fin;
  }

  printf("%.*s", LOG_DESCRIPTION_MAX_SIZE, bulk_log);

#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

process_fin:
  return kUtilityLogStatusOk;
}

UtilityLogStatus UtilityLogRegisterSetDLogLevelCallback(
    uint32_t module_id, UtilityLogSetDlogLevelCallback callback) {
  if ((IS_VALID_MODULE_ID(module_id) == 0) || (callback == NULL)) {
    UTILITY_LOG_ERROR(
        "Invalid parameter. Module id is invalid or callback is NULL."
        "module_id=0x%08X, callback=%p",
        module_id, callback);
    return kUtilityLogStatusParamError;
  }

  if (pthread_mutex_lock(&static_log_mutex) != 0) {
    UTILITY_LOG_ERROR("Mutex lock error.");
    return kUtilityLogStatusFailed;
  }

  UtilityLogStatus ret = UtilityLogValidateState();
  if (ret != kUtilityLogStatusOk) {
    UTILITY_LOG_ERROR("Invalid state error.");
    pthread_mutex_unlock(&static_log_mutex);
    return ret;
  }

#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
  int32_t index = CONVERT_BIT_TO_INDEX(module_id);
  if (static_module_data_list[index].callback != NULL) {
    UTILITY_LOG_ERROR("Invalid parameter. The callback is already set.");
    pthread_mutex_unlock(&static_log_mutex);
    return kUtilityLogStatusParamError;
  }
  // Register the callback.
  static_module_data_list[index].callback = callback;
  // To rearrange for notifying the latest Dlog level.
  UtilityLogDlogLevel dlog_level =
      static_module_data_list[index].params.dlog_level;
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

  if (pthread_mutex_unlock(&static_log_mutex) != 0) {
    UTILITY_LOG_ERROR("Mutex unlock error.");
    return kUtilityLogStatusFailed;
  }

#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
  // Notify newest Dlog level.
  callback(dlog_level);
#else   // CONFIG_EXTERNAL_DLOG_DISABLE
  callback(CONFIG_UTILITY_LOG_DEFAULT_DLOG_LEVEL);
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

  return kUtilityLogStatusOk;
}

UtilityLogStatus UtilityLogUnregisterSetDLogLevelCallback(uint32_t module_id) {
  if (IS_VALID_MODULE_ID(module_id) == 0) {
    UTILITY_LOG_ERROR(
        "Invalid parameter. Module id is invalid. module_id=0x%08X", module_id);
    return kUtilityLogStatusParamError;
  }

  if (pthread_mutex_lock(&static_log_mutex) != 0) {
    UTILITY_LOG_ERROR("Mutex lock error.");
    return kUtilityLogStatusFailed;
  }

  UtilityLogStatus ret = UtilityLogValidateState();
  if (ret != kUtilityLogStatusOk) {
    UTILITY_LOG_ERROR("Invalid state error.");
    pthread_mutex_unlock(&static_log_mutex);
    return ret;
  }

  int32_t index = CONVERT_BIT_TO_INDEX(module_id);
  if (static_module_data_list[index].callback == NULL) {
    UTILITY_LOG_ERROR("Invalid parameter. The callback is not set.");
    pthread_mutex_unlock(&static_log_mutex);
    return kUtilityLogStatusParamError;
  }
  // Unregister the callback.
  static_module_data_list[index].callback = NULL;

  if (pthread_mutex_unlock(&static_log_mutex) != 0) {
    UTILITY_LOG_ERROR("Mutex unlock error.");
    return kUtilityLogStatusFailed;
  }

  return kUtilityLogStatusOk;
}

UtilityLogStatus UtilityLogOpen(uint32_t module_id, UtilityLogHandle *handle) {
  return kUtilityLogStatusOk;
}
UtilityLogStatus UtilityLogClose(UtilityLogHandle handle) {
  return kUtilityLogStatusOk;
}
UtilityLogStatus UtilityLogWriteDlog(UtilityLogHandle handle,
                                     UtilityLogDlogLevel level,
                                     const char *format, ...) {
  return kUtilityLogStatusOk;
}
