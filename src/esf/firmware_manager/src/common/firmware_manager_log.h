/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESF_FIRMWARE_MANAGER_SRC_FIRMWARE_MANAGER_LOG_H
#define ESF_FIRMWARE_MANAGER_SRC_FIRMWARE_MANAGER_LOG_H

#include <stdio.h>

#define ESF_FW_MGR_MAJOR_EVENT_ID (0x8500u)
#define ESF_FW_MGR_EVENT_ID_MASK (0xffffu)
#define ESF_FW_MGR_MINOR_EVENT_ID_MASK (0x00ffu)

#define ESF_FW_MGR_EVENT_ID(minor_id)                      \
  (ESF_FW_MGR_EVENT_ID_MASK & (ESF_FW_MGR_MAJOR_EVENT_ID | \
                               (ESF_FW_MGR_MINOR_EVENT_ID_MASK & (minor_id))))

#define ESF_FW_MGR_DLOG_FORMAT(format) \
  "%s-%d: %s " format, __FILE__, __LINE__, __func__

#ifdef CONFIG_EXTERNAL_FIRMWARE_MANAGER_USE_UTILITY_LOG
#include "utility_log.h"
#include "utility_log_module_id.h"

#define ESF_FW_MGR_MODULE_ID MODULE_ID_SYSTEM

#define ESF_FW_MGR_DLOG(level, format, ...)        \
  UtilityLogWriteDLog(ESF_FW_MGR_MODULE_ID, level, \
                      ESF_FW_MGR_DLOG_FORMAT(format), ##__VA_ARGS__)
#define ESF_FW_MGR_ELOG(level, minor_id)           \
  UtilityLogWriteELog(ESF_FW_MGR_MODULE_ID, level, \
                      ESF_FW_MGR_EVENT_ID(minor_id))

#else  // CONFIG_EXTERNAL_FIRMWARE_MANAGER_USE_UTILITY_LOG

#define kUtilityLogDlogLevelCritical "CRITICAL"
#define kUtilityLogDlogLevelError "ERROR"
#define kUtilityLogDlogLevelWarn "WARNING"
#define kUtilityLogDlogLevelInfo "INFO"
#define kUtilityLogDlogLevelDebug "DEBUG"
#define kUtilityLogDlogLevelTrace "TRACE"

#define kUtilityLogElogLevelCritical "CRITICAL"
#define kUtilityLogElogLevelError "ERROR"
#define kUtilityLogElogLevelWarn "WARNING"
#define kUtilityLogElogLevelInfo "INFO"
#define kUtilityLogElogLevelDebug "DEBUG"
#define kUtilityLogElogLevelTrace "TRACE"

#define ESF_FW_MGR_DLOG(level, format, ...) \
  printf("[" level "] " ESF_FW_MGR_DLOG_FORMAT(format), ##__VA_ARGS__)
#define ESF_FW_MGR_ELOG(level, minor_id) \
  printf("[" level "] ELOG: 0x%04x\n", ESF_FW_MGR_EVENT_ID(minor_id))

#endif  // else CONFIG_EXTERNAL_FIRMWARE_MANAGER_USE_UTILITY_LOG

#define ESF_FW_MGR_DLOG_CRITICAL(format, ...) \
  ESF_FW_MGR_DLOG(kUtilityLogDlogLevelCritical, format, ##__VA_ARGS__)
#define ESF_FW_MGR_DLOG_ERROR(format, ...) \
  ESF_FW_MGR_DLOG(kUtilityLogDlogLevelError, format, ##__VA_ARGS__)
#define ESF_FW_MGR_DLOG_WARNING(format, ...) \
  ESF_FW_MGR_DLOG(kUtilityLogDlogLevelWarn, format, ##__VA_ARGS__)
#define ESF_FW_MGR_DLOG_INFO(format, ...) \
  ESF_FW_MGR_DLOG(kUtilityLogDlogLevelInfo, format, ##__VA_ARGS__)
#define ESF_FW_MGR_DLOG_DEBUG(format, ...) \
  ESF_FW_MGR_DLOG(kUtilityLogDlogLevelDebug, format, ##__VA_ARGS__)
#define ESF_FW_MGR_DLOG_TRACE(format, ...) \
  ESF_FW_MGR_DLOG(kUtilityLogDlogLevelTrace, format, ##__VA_ARGS__)

#define ESF_FW_MGR_ELOG_CRITICAL(minor_id) \
  ESF_FW_MGR_ELOG(kUtilityLogElogLevelCritical, minor_id);
#define ESF_FW_MGR_ELOG_ERROR(minor_id) \
  ESF_FW_MGR_ELOG(kUtilityLogElogLevelError, minor_id);
#define ESF_FW_MGR_ELOG_WARNING(minor_id) \
  ESF_FW_MGR_ELOG(kUtilityLogElogLevelWarn, minor_id);
#define ESF_FW_MGR_ELOG_INFO(minor_id) \
  ESF_FW_MGR_ELOG(kUtilityLogElogLevelInfo, minor_id);
#define ESF_FW_MGR_ELOG_DEBUG(minor_id) \
  ESF_FW_MGR_ELOG(kUtilityLogElogLevelDebug, minor_id);
#define ESF_FW_MGR_ELOG_TRACE(minor_id) \
  ESF_FW_MGR_ELOG(kUtilityLogElogLevelTrace, minor_id);

typedef enum EsfFwMgrElogCriticalId {
  kEsfFwMgrElogCriticalId0x00CoreBlockStreaming = 0x00,
  kEsfFwMgrElogCriticalId0x01CoreAllocOpsList,
  kEsfFwMgrElogCriticalId0x02CoreAllocContext,
  kEsfFwMgrElogCriticalId0x03CoreAllocBuffer,
  kEsfFwMgrElogCriticalId0x04CoreAllocTmpBuffer,
  kEsfFwMgrElogCriticalId0x05CoreNotInUse,  // not used
  kEsfFwMgrElogCriticalId0x06CoreNotInUse,  // not used
  kEsfFwMgrElogCriticalId0x07CoreNotInUse,  // not used
  kEsfFwMgrElogCriticalId0x08CoreNotInUse,  // not used
  kEsfFwMgrElogCriticalId0x09CoreFreeBuffer,
  kEsfFwMgrElogCriticalId0x0aCoreVerifyHash,
  kEsfFwMgrElogCriticalId0x40Core = 0x40,
  kEsfFwMgrElogCriticalId0x41Core,
  kEsfFwMgrElogCriticalId0x42Core,
  kEsfFwMgrElogCriticalId0x43Core,
  kEsfFwMgrElogCriticalId0x44Core,

  kEsfFwMgrElogCriticalId0x80ProcessorAllocContext = 0x80,
  kEsfFwMgrElogCriticalId0x8cProcessor = 0x8c,
  kEsfFwMgrElogCriticalId0x8dProcessor,
  kEsfFwMgrElogCriticalId0x8eProcessor,
  kEsfFwMgrElogCriticalId0x8fProcessor,
  kEsfFwMgrElogCriticalId0x90Processor = 0x90,
  kEsfFwMgrElogCriticalId0x91Processor,
  kEsfFwMgrElogCriticalId0x92Processor,
  kEsfFwMgrElogCriticalId0x93Processor,
  kEsfFwMgrElogCriticalId0x94Processor,
  kEsfFwMgrElogCriticalId0x95Processor,
  kEsfFwMgrElogCriticalId0x96Processor,
  kEsfFwMgrElogCriticalId0x97Processor,
  kEsfFwMgrElogCriticalId0x98Processor,
  kEsfFwMgrElogCriticalId0x99Processor,
  kEsfFwMgrElogCriticalId0x9aProcessor,
  kEsfFwMgrElogCriticalId0x9bProcessor,
  kEsfFwMgrElogCriticalId0x9cProcessor,
  kEsfFwMgrElogCriticalId0x9dProcessor,
  kEsfFwMgrElogCriticalId0x9eProcessor,
  kEsfFwMgrElogCriticalId0x9fProcessor,

  kEsfFwMgrElogCriticalId0xa0SensorNotInUse = 0xa0,  // not used
  kEsfFwMgrElogCriticalId0xa1SensorAllocContext,
  kEsfFwMgrElogCriticalId0xa2SensorAllocContext,
  kEsfFwMgrElogCriticalId0xb0SensorNotInUse = 0xb0,  // not used
  kEsfFwMgrElogCriticalId0xb1SensorNotInUse,         // not used
  kEsfFwMgrElogCriticalId0xb2Sensor,
  kEsfFwMgrElogCriticalId0xb3Sensor,
  kEsfFwMgrElogCriticalId0xb4Sensor,
  kEsfFwMgrElogCriticalId0xb5Sensor,
  kEsfFwMgrElogCriticalId0xb6Sensor,
  kEsfFwMgrElogCriticalId0xb7Sensor,
  kEsfFwMgrElogCriticalId0xb8Sensor,
  kEsfFwMgrElogCriticalId0xb9Sensor,
  kEsfFwMgrElogCriticalId0xbaSensor,
  kEsfFwMgrElogCriticalId0xbbSensor,
  kEsfFwMgrElogCriticalId0xbcSensor,

  kEsfFwMgrElogCriticalId0xc0Reserved = 0xc0,

  kEsfFwMgrElogCriticalId0xe0Common = 0xe0,
  kEsfFwMgrElogCriticalId0xe1Common,
  kEsfFwMgrElogCriticalId0xe2Common,
  kEsfFwMgrElogCriticalId0xe3Common,

  kEsfFwMgrElogCriticalId0xf0FactoryReset = 0xf0,
  kEsfFwMgrElogCriticalId0xf1FactoryReset,
} EsfFwMgrElogCriticalId;

typedef enum EsfFwMgrElogErrorId {
  kEsfFwMgrElogErrorId0x00Core = 0x00,
  kEsfFwMgrElogErrorId0x01Core,
  kEsfFwMgrElogErrorId0x02Core,
  kEsfFwMgrElogErrorId0x03Core,
  kEsfFwMgrElogErrorId0x04Core,
  kEsfFwMgrElogErrorId0x05Core,
  kEsfFwMgrElogErrorId0x06Core,
  kEsfFwMgrElogErrorId0x07Core,
  kEsfFwMgrElogErrorId0x08Core,
  kEsfFwMgrElogErrorId0x09Core,
  kEsfFwMgrElogErrorId0x0aCore,
  kEsfFwMgrElogErrorId0x0bCore,
  kEsfFwMgrElogErrorId0x0cCore,
  kEsfFwMgrElogErrorId0x0dCore,
  kEsfFwMgrElogErrorId0x0eCore,
  kEsfFwMgrElogErrorId0x0fCore,
  kEsfFwMgrElogErrorId0x10Core,
  kEsfFwMgrElogErrorId0x11Core,
  kEsfFwMgrElogErrorId0x12Core,
  kEsfFwMgrElogErrorId0x13Core,
  kEsfFwMgrElogErrorId0x14Core,
  kEsfFwMgrElogErrorId0x15Core,
  kEsfFwMgrElogErrorId0x16Core,
  kEsfFwMgrElogErrorId0x17Core,
  kEsfFwMgrElogErrorId0x18Core,
  kEsfFwMgrElogErrorId0x19Core,
  kEsfFwMgrElogErrorId0x1aCore,
  kEsfFwMgrElogErrorId0x1bCore,
  kEsfFwMgrElogErrorId0x1cCore,
  kEsfFwMgrElogErrorId0x1dCore,
  kEsfFwMgrElogErrorId0x1eCore,
  kEsfFwMgrElogErrorId0x1fCore,
  kEsfFwMgrElogErrorId0x20Core,
  kEsfFwMgrElogErrorId0x21Core,
  kEsfFwMgrElogErrorId0x22Core,
  kEsfFwMgrElogErrorId0x23Core,
  kEsfFwMgrElogErrorId0x24Core,
  kEsfFwMgrElogErrorId0x25Core,
  kEsfFwMgrElogErrorId0x26Core,
  kEsfFwMgrElogErrorId0x27Core,
  kEsfFwMgrElogErrorId0x28Core,
  kEsfFwMgrElogErrorId0x29Core,
  kEsfFwMgrElogErrorId0x2aCore,
  kEsfFwMgrElogErrorId0x2bCore,
  kEsfFwMgrElogErrorId0x2cCore,
  kEsfFwMgrElogErrorId0x2dCore,
  kEsfFwMgrElogErrorId0x2eCore,
  kEsfFwMgrElogErrorId0x2fCore,
  kEsfFwMgrElogErrorId0x30Core,
  kEsfFwMgrElogErrorId0x31Core,
  kEsfFwMgrElogErrorId0x32Core,
  kEsfFwMgrElogErrorId0x33Core,
  kEsfFwMgrElogErrorId0x34Core,
  kEsfFwMgrElogErrorId0x35Core,
  kEsfFwMgrElogErrorId0x36Core,
  kEsfFwMgrElogErrorId0x37Core,
  kEsfFwMgrElogErrorId0x38Core,
  kEsfFwMgrElogErrorId0x39Core,
  kEsfFwMgrElogErrorId0x3aCore,
  kEsfFwMgrElogErrorId0x3bCore,
  kEsfFwMgrElogErrorId0x3cCore,
  kEsfFwMgrElogErrorId0x3dCore,
  kEsfFwMgrElogErrorId0x3eCore,
  kEsfFwMgrElogErrorId0x3fCore,
  kEsfFwMgrElogErrorId0x40Core,
  kEsfFwMgrElogErrorId0x41Core,
  kEsfFwMgrElogErrorId0x42Core,
  kEsfFwMgrElogErrorId0x43Core,
  kEsfFwMgrElogErrorId0x44Core,
  kEsfFwMgrElogErrorId0x45Core,
  kEsfFwMgrElogErrorId0x46Core,
  kEsfFwMgrElogErrorId0x47Core,
  kEsfFwMgrElogErrorId0x48Core,
  kEsfFwMgrElogErrorId0x49Core,
  kEsfFwMgrElogErrorId0x4aCore,
  kEsfFwMgrElogErrorId0x4bCore,
  kEsfFwMgrElogErrorId0x4cCore,
  kEsfFwMgrElogErrorId0x4dCore,
  kEsfFwMgrElogErrorId0x4eCore,
  kEsfFwMgrElogErrorId0x4fCore,
  kEsfFwMgrElogErrorId0x50Core,
  kEsfFwMgrElogErrorId0x51Core,
  kEsfFwMgrElogErrorId0x52Core,
  kEsfFwMgrElogErrorId0x53Core,
  kEsfFwMgrElogErrorId0x54Core,
  kEsfFwMgrElogErrorId0x55Core,
  kEsfFwMgrElogErrorId0x56Core,
  kEsfFwMgrElogErrorId0x57Core,
  kEsfFwMgrElogErrorId0x58Core,
  kEsfFwMgrElogErrorId0x59Core,
  kEsfFwMgrElogErrorId0x5aCore,
  kEsfFwMgrElogErrorId0x5bCore,
  kEsfFwMgrElogErrorId0x5cCore,
  kEsfFwMgrElogErrorId0x5dCore,
  kEsfFwMgrElogErrorId0x5eCore,
  kEsfFwMgrElogErrorId0x5fCore,
  kEsfFwMgrElogErrorId0x60Core,
  kEsfFwMgrElogErrorId0x61Core,
  kEsfFwMgrElogErrorId0x62Core,
  kEsfFwMgrElogErrorId0x63Core,
  kEsfFwMgrElogErrorId0x64Core,
  kEsfFwMgrElogErrorId0x65Core,
  kEsfFwMgrElogErrorId0x66Core,
  kEsfFwMgrElogErrorId0x67Core,
  kEsfFwMgrElogErrorId0x68Core,
  kEsfFwMgrElogErrorId0x69Core,
  kEsfFwMgrElogErrorId0x6aCore,
  kEsfFwMgrElogErrorId0x6bCore,
  kEsfFwMgrElogErrorId0x6cCore,
  kEsfFwMgrElogErrorId0x6dCore,
  kEsfFwMgrElogErrorId0x6eCore,
  kEsfFwMgrElogErrorId0x6fCore,
  kEsfFwMgrElogErrorId0x70Core,
  kEsfFwMgrElogErrorId0x71Core,
  kEsfFwMgrElogErrorId0x72Core,
  kEsfFwMgrElogErrorId0x73Core,
  kEsfFwMgrElogErrorId0x74Core,
  kEsfFwMgrElogErrorId0x75Core,

  kEsfFwMgrElogErrorId0x80Processor = 0x80,
  kEsfFwMgrElogErrorId0x81Processor,
  kEsfFwMgrElogErrorId0x82Processor,
  kEsfFwMgrElogErrorId0x83Processor,
  kEsfFwMgrElogErrorId0x84Processor,
  kEsfFwMgrElogErrorId0x85Processor,
  kEsfFwMgrElogErrorId0x86Processor,
  kEsfFwMgrElogErrorId0x87Processor,
  kEsfFwMgrElogErrorId0x88Processor,
  kEsfFwMgrElogErrorId0x89Processor,
  kEsfFwMgrElogErrorId0x8aProcessor,
  kEsfFwMgrElogErrorId0x8bProcessor,
  kEsfFwMgrElogErrorId0x8cProcessor,
  kEsfFwMgrElogErrorId0x8dProcessor,
  kEsfFwMgrElogErrorId0x8eProcessor,
  kEsfFwMgrElogErrorId0x8fProcessor,
  kEsfFwMgrElogErrorId0x90Processor,
  kEsfFwMgrElogErrorId0x91Processor,
  kEsfFwMgrElogErrorId0x92Processor,
  kEsfFwMgrElogErrorId0x93Processor,
  kEsfFwMgrElogErrorId0x94Processor,
  kEsfFwMgrElogErrorId0x95Processor,
  kEsfFwMgrElogErrorId0x96Processor,
  kEsfFwMgrElogErrorId0x97Processor,
  kEsfFwMgrElogErrorId0x98Processor,
  kEsfFwMgrElogErrorId0x99Processor,
  kEsfFwMgrElogErrorId0x9aProcessor,
  kEsfFwMgrElogErrorId0x9bProcessor,
  kEsfFwMgrElogErrorId0x9cProcessor,
  kEsfFwMgrElogErrorId0x9dProcessor,
  kEsfFwMgrElogErrorId0x9eProcessor,

  kEsfFwMgrElogErrorId0xa0Sensor = 0xa0,
  kEsfFwMgrElogErrorId0xa1Sensor,
  kEsfFwMgrElogErrorId0xa2SensorNotInUse,  // not used
  kEsfFwMgrElogErrorId0xa3Sensor,
  kEsfFwMgrElogErrorId0xa4Sensor,
  kEsfFwMgrElogErrorId0xa5Sensor,
  kEsfFwMgrElogErrorId0xa6Sensor,
  kEsfFwMgrElogErrorId0xa7Sensor,
  kEsfFwMgrElogErrorId0xa8Sensor,
  kEsfFwMgrElogErrorId0xa9Sensor,
  kEsfFwMgrElogErrorId0xaaSensor,
  kEsfFwMgrElogErrorId0xabSensor,
  kEsfFwMgrElogErrorId0xacSensor,
  kEsfFwMgrElogErrorId0xadSensor,
  kEsfFwMgrElogErrorId0xaeSensorNotInUse,  // not used
  kEsfFwMgrElogErrorId0xafSensor,

  kEsfFwMgrElogErrorId0xc0Reserved = 0xc0,

  kEsfFwMgrElogErrorId0xe0Common = 0xe0,

  kEsfFwMgrElogErrorId0xf0FactoryReset = 0xf0,

} EsfFwMgrElogErrorId;

typedef enum EsfFwMgrElogWarningId {
  kEsfFwMgrElogWarningId0x00Core = 0x00,
  kEsfFwMgrElogWarningId0x01Core,
  kEsfFwMgrElogWarningId0x02Core,
  kEsfFwMgrElogWarningId0x03Core,
  kEsfFwMgrElogWarningId0x04Core,
  kEsfFwMgrElogWarningId0x05Core,
  kEsfFwMgrElogWarningId0x06Core,
  kEsfFwMgrElogWarningId0x07Core,
  kEsfFwMgrElogWarningId0x08Core,
  kEsfFwMgrElogWarningId0x09Core,
  kEsfFwMgrElogWarningId0x0aCore,
  kEsfFwMgrElogWarningId0x0bCore,
  kEsfFwMgrElogWarningId0x0cCore,
  kEsfFwMgrElogWarningId0x0dCore,

  kEsfFwMgrElogWarningId0x80Processor = 0x80,  // not used

  kEsfFwMgrElogWarningId0xa0Sensor = 0xa0,  // not used

  kEsfFwMgrElogWarningId0xc0Reserved = 0xc0,  // not used

  kEsfFwMgrElogWarningId0xe0Common = 0xe0,  // not used

  kEsfFwMgrElogWarningId0xf0FactoryReset = 0xf0,  // not used
} EsfFwMgrElogWarningId;

typedef enum EsfFwMgrElogInfoId {
  kEsfFwMgrElogInfoId0x00CoreInit = 0x00,
  kEsfFwMgrElogInfoId0x01CoreOpen,
  kEsfFwMgrElogInfoId0x02CoreWrite,
  kEsfFwMgrElogInfoId0x03CorePostProcess,
  kEsfFwMgrElogInfoId0x04CoreErase,
  kEsfFwMgrElogInfoId0x05CoreClose,
  kEsfFwMgrElogInfoId0x06CoreDeinit,
  kEsfFwMgrElogInfoId0x10CoreGetInfo = 0x10,
  kEsfFwMgrElogInfoId0x11CoreFactoryReset,
  kEsfFwMgrElogInfoId0x20CoreMemAlloc = 0x20,
  kEsfFwMgrElogInfoId0x21CoreMemFree,
  kEsfFwMgrElogInfoId0x22CoreNotInUse,  // not used
  kEsfFwMgrElogInfoId0x23CoreNotInUse,  // not used
  kEsfFwMgrElogInfoId0x24CoreNotInUse,  // not used
  kEsfFwMgrElogInfoId0x25CoreNotInUse,  // not used

  kEsfFwMgrElogInfoId0x80Processor = 0x80,  // not used

  kEsfFwMgrElogInfoId0xa0Sensor = 0xa0,  // not used

  kEsfFwMgrElogInfoId0xc0Reserved = 0xc0,  // not used

  kEsfFwMgrElogInfoId0xe0Common = 0xe0,  // not used

  kEsfFwMgrElogInfoId0xf0FactoryReset = 0xf0,  // not used
} EsfFwMgrElogInfoId;

typedef enum EsfFwMgrElogDebugId {
  kEsfFwMgrElogDebugId0x00Core = 0x00,          // not used
  kEsfFwMgrElogDebugId0x80Processor = 0x80,     // not used
  kEsfFwMgrElogDebugId0xa0Sensor = 0xa0,        // not used
  kEsfFwMgrElogDebugId0xc0Reserved = 0xc0,      // not used
  kEsfFwMgrElogDebugId0xe0Common = 0xe0,        // not used
  kEsfFwMgrElogDebugId0xf0FactoryReset = 0xf0,  // not used
} EsfFwMgrElogDebugId;

typedef enum EsfFwMgrElogTraceId {
  kEsfFwMgrElogTraceId0x00Core = 0x00,          // not used
  kEsfFwMgrElogTraceId0x80Processor = 0x80,     // not used
  kEsfFwMgrElogTraceId0xa0Sensor = 0xa0,        // not used
  kEsfFwMgrElogTraceId0xc0Reserved = 0xc0,      // not used
  kEsfFwMgrElogTraceId0xe0Common = 0xe0,        // not used
  kEsfFwMgrElogTraceId0xf0FactoryReset = 0xf0,  // not used
} EsfFwMgrElogTraceId;

#endif  // ESF_FIRMWARE_MANAGER_SRC_FIRMWARE_MANAGER_LOG_H
