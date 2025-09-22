/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FIRMWARE_MANAGER_PORTING_LAYER_LOG_H_
#define FIRMWARE_MANAGER_PORTING_LAYER_LOG_H_
#include <stdio.h>

#define FW_MGR_PL_MAJOR_EVENT_ID (0x9200u)
#define FW_MGR_PL_EVENT_ID_MASK (0xffffu)
#define FW_MGR_PL_MINOR_EVENT_ID_MASK (0x00ffu)

#define FW_MGR_PL_EVENT_ID(minor_id) \
  (FW_MGR_PL_EVENT_ID_MASK &         \
   (FW_MGR_PL_MAJOR_EVENT_ID | (FW_MGR_PL_MINOR_EVENT_ID_MASK & (minor_id))))

#define FW_MGR_PL_DLOG_FORMAT(format) \
  "%s-%d: %s " format, __FILE__, __LINE__, __func__

#ifdef CONFIG_EXTERNAL_FIRMWARE_MANAGER_USE_UTILITY_LOG
#include "utility_log.h"
#include "utility_log_module_id.h"

#define FW_MGR_PL_MODULE_ID MODULE_ID_SYSTEM

#define FW_MGR_PL_DLOG(level, format, ...)        \
  UtilityLogWriteDLog(FW_MGR_PL_MODULE_ID, level, \
                      FW_MGR_PL_DLOG_FORMAT(format), ##__VA_ARGS__)
#define FW_MGR_PL_ELOG(level, minor_id) \
  UtilityLogWriteELog(FW_MGR_PL_MODULE_ID, level, FW_MGR_PL_EVENT_ID(minor_id))

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

#define FW_MGR_PL_DLOG(level, format, ...) \
  printf("[" level "] " FW_MGR_PL_DLOG_FORMAT(format), ##__VA_ARGS__)
#define FW_MGR_PL_ELOG(level, minor_id) \
  printf("[" level "] ELOG: 0x%04x\n", FW_MGR_PL_EVENT_ID(minor_id))

#endif  // else CONFIG_EXTERNAL_FIRMWARE_MANAGER_USE_UTILITY_LOG

#define FW_MGR_PL_DLOG_CRITICAL(format, ...) \
  FW_MGR_PL_DLOG(kUtilityLogDlogLevelCritical, format, ##__VA_ARGS__)
#define FW_MGR_PL_DLOG_ERROR(format, ...) \
  FW_MGR_PL_DLOG(kUtilityLogDlogLevelError, format, ##__VA_ARGS__)
#define FW_MGR_PL_DLOG_WARNING(format, ...) \
  FW_MGR_PL_DLOG(kUtilityLogDlogLevelWarn, format, ##__VA_ARGS__)
#define FW_MGR_PL_DLOG_INFO(format, ...) \
  FW_MGR_PL_DLOG(kUtilityLogDlogLevelInfo, format, ##__VA_ARGS__)
#define FW_MGR_PL_DLOG_DEBUG(format, ...) \
  FW_MGR_PL_DLOG(kUtilityLogDlogLevelDebug, format, ##__VA_ARGS__)
#define FW_MGR_PL_DLOG_TRACE(format, ...) \
  FW_MGR_PL_DLOG(kUtilityLogDlogLevelTrace, format, ##__VA_ARGS__)

#define FW_MGR_PL_ELOG_CRITICAL(minor_id) \
  FW_MGR_PL_ELOG(kUtilityLogElogLevelCritical, minor_id);
#define FW_MGR_PL_ELOG_ERROR(minor_id) \
  FW_MGR_PL_ELOG(kUtilityLogElogLevelError, minor_id);
#define FW_MGR_PL_ELOG_WARNING(minor_id) \
  FW_MGR_PL_ELOG(kUtilityLogElogLevelWarn, minor_id);
#define FW_MGR_PL_ELOG_INFO(minor_id) \
  FW_MGR_PL_ELOG(kUtilityLogElogLevelInfo, minor_id);
#define FW_MGR_PL_ELOG_DEBUG(minor_id) \
  FW_MGR_PL_ELOG(kUtilityLogElogLevelDebug, minor_id);
#define FW_MGR_PL_ELOG_TRACE(minor_id) \
  FW_MGR_PL_ELOG(kUtilityLogElogLevelTrace, minor_id);

typedef enum FwMgrPlElogCriticalId {
  kFwMgrPlElogCriticalId0x00CoreAllocContext = 0x0,
  kFwMgrPlElogCriticalId0x01CoreExceedsTotalSize,

  kFwMgrPlElogCriticalId0x40IfAllocContext = 0x40,

  kFwMgrPlElogCriticalId0x60FwInvalidMagic = 0x60,
  kFwMgrPlElogCriticalId0x61FwInvalidChipId,
  kFwMgrPlElogCriticalId0x62FwInvalidHash,
  kFwMgrPlElogCriticalId0x63FwInvalidSize,  // a multiple of 32
  kFwMgrPlElogCriticalId0x64FwAllocContext,
  kFwMgrPlElogCriticalId0x65FwAllocTmpBuffer,
  kFwMgrPlElogCriticalId0x64InvalidSize,  // 0 < size <= partition size

  kFwMgrPlElogCriticalId0x80Fw = 0x80,
  kFwMgrPlElogCriticalId0x81Fw,
  kFwMgrPlElogCriticalId0x82Fw,
  kFwMgrPlElogCriticalId0x83Fw,
  kFwMgrPlElogCriticalId0x84Fw,
  kFwMgrPlElogCriticalId0x85Fw,
  kFwMgrPlElogCriticalId0x86Fw,
  kFwMgrPlElogCriticalId0x87Fw,
  kFwMgrPlElogCriticalId0x88Fw,

  kFwMgrPlElogCriticalId0xa0t3p = 0xa0,
  kFwMgrPlElogCriticalId0xa1t3p,
  kFwMgrPlElogCriticalId0xa2t3p,
  kFwMgrPlElogCriticalId0xa3t3p,
  kFwMgrPlElogCriticalId0xa4t3p,
  kFwMgrPlElogCriticalId0xa5t3p,
  kFwMgrPlElogCriticalId0xa6t3p,
  kFwMgrPlElogCriticalId0xa7t3p,
  kFwMgrPlElogCriticalId0xa8t3p,
  kFwMgrPlElogCriticalId0xa9t3p,
  kFwMgrPlElogCriticalId0xaat3p,
  kFwMgrPlElogCriticalId0xabt3p,

  kFwMgrPlElogCriticalId0xb0t5 = 0xb0,
  kFwMgrPlElogCriticalId0xb1t5,
  kFwMgrPlElogCriticalId0xb2t5,
  kFwMgrPlElogCriticalId0xb3t5,
  kFwMgrPlElogCriticalId0xb4t5,

  kFwMgrPlElogCriticalId0xc0Reserved = 0xc0,

  kFwMgrPlElogCriticalId0xe0FlashOps = 0xe0,
  kFwMgrPlElogCriticalId0xe1FlashOps,
  kFwMgrPlElogCriticalId0xe2FlashOps,
  kFwMgrPlElogCriticalId0xe3FlashOps,
  kFwMgrPlElogCriticalId0xe4FlashOps,
  kFwMgrPlElogCriticalId0xe5FlashOps,
  kFwMgrPlElogCriticalId0xe6FlashOps,
  kFwMgrPlElogCriticalId0xe7FlashOps,
  kFwMgrPlElogCriticalId0xe8FlashOps,
  kFwMgrPlElogCriticalId0xe9FlashOps,
  kFwMgrPlElogCriticalId0xeaFlashOps,
  kFwMgrPlElogCriticalId0xebFlashOps,
  kFwMgrPlElogCriticalId0xecFlashOps,
  kFwMgrPlElogCriticalId0xedFlashOps,
  kFwMgrPlElogCriticalId0xeeFlashOps,
  kFwMgrPlElogCriticalId0xefFlashOps,
  kFwMgrPlElogCriticalId0xf0FlashOps,
} FwMgrPlElogCriticalId;

typedef enum FwMgrPlElogErrorId {
  kFwMgrPlElogErrorId0x00Core = 0x00,
  kFwMgrPlElogErrorId0x01Core,
  kFwMgrPlElogErrorId0x02Core,
  kFwMgrPlElogErrorId0x03Core,
  kFwMgrPlElogErrorId0x04Core,
  kFwMgrPlElogErrorId0x05Core,
  kFwMgrPlElogErrorId0x06Core,
  kFwMgrPlElogErrorId0x07Core,
  kFwMgrPlElogErrorId0x08Core,
  kFwMgrPlElogErrorId0x09Core,
  kFwMgrPlElogErrorId0x0aCore,
  kFwMgrPlElogErrorId0x0bCore,
  kFwMgrPlElogErrorId0x0cCore,
  kFwMgrPlElogErrorId0x0dCore,
  kFwMgrPlElogErrorId0x0eCore,
  kFwMgrPlElogErrorId0x0fCore,
  kFwMgrPlElogErrorId0x10Core,
  kFwMgrPlElogErrorId0x11Core,
  kFwMgrPlElogErrorId0x12Core,
  kFwMgrPlElogErrorId0x13Core,
  kFwMgrPlElogErrorId0x14Core,
  kFwMgrPlElogErrorId0x15Core,
  kFwMgrPlElogErrorId0x16Core,
  kFwMgrPlElogErrorId0x17Core,
  kFwMgrPlElogErrorId0x18Core,
  kFwMgrPlElogErrorId0x19Core,
  kFwMgrPlElogErrorId0x1aCore,
  kFwMgrPlElogErrorId0x1bCore,
  kFwMgrPlElogErrorId0x1cCore,

  kFwMgrPlElogErrorId0x40If = 0x40,
  kFwMgrPlElogErrorId0x41If,
  kFwMgrPlElogErrorId0x42If,
  kFwMgrPlElogErrorId0x43If,
  kFwMgrPlElogErrorId0x44If,
  kFwMgrPlElogErrorId0x45If,
  kFwMgrPlElogErrorId0x46If,
  kFwMgrPlElogErrorId0x47If,
  kFwMgrPlElogErrorId0x48If,
  kFwMgrPlElogErrorId0x49If,
  kFwMgrPlElogErrorId0x4aIf,
  kFwMgrPlElogErrorId0x4bIf,
  kFwMgrPlElogErrorId0x4cIf,
  kFwMgrPlElogErrorId0x4dIf,
  kFwMgrPlElogErrorId0x4eIf,
  kFwMgrPlElogErrorId0x4fIf,
  kFwMgrPlElogErrorId0x50If,

  kFwMgrPlElogErrorId0x60Fw = 0x60,
  kFwMgrPlElogErrorId0x61Fw,
  kFwMgrPlElogErrorId0x62Fw,
  kFwMgrPlElogErrorId0x63Fw,
  kFwMgrPlElogErrorId0x64Fw,
  kFwMgrPlElogErrorId0x65Fw,
  kFwMgrPlElogErrorId0x66Fw,
  kFwMgrPlElogErrorId0x67Fw,
  kFwMgrPlElogErrorId0x68Fw,
  kFwMgrPlElogErrorId0x69Fw,
  kFwMgrPlElogErrorId0x6aFw,
  kFwMgrPlElogErrorId0x6bFw,
  kFwMgrPlElogErrorId0x6cFw,
  kFwMgrPlElogErrorId0x6dFw,
  kFwMgrPlElogErrorId0x6eFw,
  kFwMgrPlElogErrorId0x6fFw,
  kFwMgrPlElogErrorId0x70Fw,
  kFwMgrPlElogErrorId0x71Fw,
  kFwMgrPlElogErrorId0x72Fw,
  kFwMgrPlElogErrorId0x73Fw,
  kFwMgrPlElogErrorId0x74Fw,
  kFwMgrPlElogErrorId0x75Fw,
  kFwMgrPlElogErrorId0x76Fw,
  kFwMgrPlElogErrorId0x77Fw,
  kFwMgrPlElogErrorId0x78Fw,
  kFwMgrPlElogErrorId0x79Fw,
  kFwMgrPlElogErrorId0x7aFw,
  kFwMgrPlElogErrorId0x7bFw,
  kFwMgrPlElogErrorId0x7cFw,
  kFwMgrPlElogErrorId0x7dFw,
  kFwMgrPlElogErrorId0x7eFw,
  kFwMgrPlElogErrorId0x7fFw,
  kFwMgrPlElogErrorId0x80Fw,
  kFwMgrPlElogErrorId0x81Fw,
  kFwMgrPlElogErrorId0x82Fw,
  kFwMgrPlElogErrorId0x83Fw,
  kFwMgrPlElogErrorId0x84Fw,
  kFwMgrPlElogErrorId0x85Fw,
  kFwMgrPlElogErrorId0x86Fw,
  kFwMgrPlElogErrorId0x87Fw,
  kFwMgrPlElogErrorId0x88Fw,

  kFwMgrPlElogErrorId0xa0t3p = 0xa0,
  kFwMgrPlElogErrorId0xa1t3p,
  kFwMgrPlElogErrorId0xa2t3p,
  kFwMgrPlElogErrorId0xa3t3p,
  kFwMgrPlElogErrorId0xa4t3p,
  kFwMgrPlElogErrorId0xa5t3p,
  kFwMgrPlElogErrorId0xa6t3p,
  kFwMgrPlElogErrorId0xa7t3p,
  kFwMgrPlElogErrorId0xa8t3p,
  kFwMgrPlElogErrorId0xa9t3p,

  kFwMgrPlElogErrorId0xb0t5 = 0xb0,
  kFwMgrPlElogErrorId0xb1t5,

  kFwMgrPlElogErrorId0xc0Reserved = 0xc0,

  kFwMgrPlElogErrorId0xe0FlashOps = 0xe0,
  kFwMgrPlElogErrorId0xe1FlashOps,
  kFwMgrPlElogErrorId0xe2FlashOps,
  kFwMgrPlElogErrorId0xe3FlashOps,
  kFwMgrPlElogErrorId0xe4FlashOps,
  kFwMgrPlElogErrorId0xe5FlashOps,
  kFwMgrPlElogErrorId0xe6FlashOps,
  kFwMgrPlElogErrorId0xe7FlashOps,
  kFwMgrPlElogErrorId0xe8FlashOps,
  kFwMgrPlElogErrorId0xe9FlashOps,
  kFwMgrPlElogErrorId0xeaFlashOps,
  kFwMgrPlElogErrorId0xebFlashOps,
  kFwMgrPlElogErrorId0xecFlashOps,
  kFwMgrPlElogErrorId0xedFlashOps,
  kFwMgrPlElogErrorId0xeeFlashOps,
  kFwMgrPlElogErrorId0xefFlashOps,
  kFwMgrPlElogErrorId0xf0FlashOps,
  kFwMgrPlElogErrorId0xf1FlashOps,
} FwMgrPlElogErrorId;

typedef enum FwMgrPlElogInfoId {
  kFwMgrPlElogInfoId0x00Core = 0x00,
  kFwMgrPlElogInfoId0x40If = 0x40,
  kFwMgrPlElogInfoId0x60FwSkipSwitchPartition = 0x60,
  kFwMgrPlElogInfoId0xa0t3p = 0xa0,
  kFwMgrPlElogInfoId0xb0t5 = 0xb0,
  kFwMgrPlElogInfoId0xc0Reserved = 0xc0,
  kFwMgrPlElogInfoId0xe0FlashOps = 0xe0,
} FwMgrPlElogInfoId;

#endif  // FIRMWARE_MANAGER_PORTING_LAYER_LOG_H_
