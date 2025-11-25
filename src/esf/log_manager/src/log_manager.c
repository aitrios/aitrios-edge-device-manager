/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "log_manager.h"

#include <inttypes.h>
#include <pthread.h>
#include <stdarg.h>
#include <string.h>

#include "log_manager_internal.h"
#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
#include "log_manager_list.h"
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

#ifdef CONFIG_EXTERNAL_LOG_MANAGER_METRICS
#include "log_manager_metrics.h"
#endif

// Variables ===============================================================
// Setup complete flag.
STATIC EsfLogManagerState s_log_manager_state = kEsfLogManagerStateInvalid;
static bool s_after_callback_flag = false;

// Functions ===============================================================
EsfLogManagerStatus EsfLogManagerInit(void) {
  if (s_log_manager_state != kEsfLogManagerStateInvalid) {
    ESF_LOG_MANAGER_ERROR("State transition error. s_log_manager_state=%d\n",
                          s_log_manager_state);
    return kEsfLogManagerStatusFailed;
  }

#ifdef CONFIG_EXTERNAL_LOG_MANAGER_METRICS
  EsfLogManagerStatus ret = kEsfLogManagerStatusOk;
  ret = EsfLogManagerInitializeMetrics();
  if (ret != kEsfLogManagerStatusOk) {
    ESF_LOG_MANAGER_ERROR("%d\n", ret);
    return kEsfLogManagerStatusFailed;
  }
#endif

  EsfLogManagerInternalInitializeList();

  s_after_callback_flag = false;
  s_log_manager_state = kEsfLogManagerStateInit;

  return kEsfLogManagerStatusOk;
}

EsfLogManagerStatus EsfLogManagerStart(void) {
  EsfLogManagerStatus ret = kEsfLogManagerStatusOk;

  if (s_log_manager_state != kEsfLogManagerStateInit) {
    ESF_LOG_MANAGER_ERROR("State transition error. s_log_manager_state=%d\n",
                          s_log_manager_state);
    return kEsfLogManagerStatusFailed;
  }

  ret = EsfLogManagerInternalLoadParameter();
  if (ret != kEsfLogManagerStatusOk) {
    ESF_LOG_MANAGER_ERROR("LoadParameter failed ret=%d\n", ret);
    return kEsfLogManagerStatusFailed;
  }

  ret = EsfLogManagerInternalInitializeByteBuffer();
  if (ret != kEsfLogManagerStatusOk) {
    ESF_LOG_MANAGER_ERROR("InitializeByteBuffer failed ret=%d\n", ret);
    return kEsfLogManagerStatusFailed;
  }

  ret = EsfLogManagerInternalSetup();
  if (ret != kEsfLogManagerStatusOk) {
    ESF_LOG_MANAGER_ERROR("InternalSetup failed ret=%d\n", ret);
    return kEsfLogManagerStatusFailed;
  }

  s_log_manager_state = kEsfLogManagerStateStart;

#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
  if (s_after_callback_flag == true) {
    EsfLogManagerInternalChangeDlogCallback(kEsfLogManagerBlockTypeSysApp);
    EsfLogManagerInternalChangeDlogCallback(kEsfLogManagerBlockTypeSensor);
    EsfLogManagerInternalChangeDlogCallback(kEsfLogManagerBlockTypeAiisp);
    EsfLogManagerInternalChangeDlogCallback(kEsfLogManagerBlockTypeVicapp);
  }
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

  return kEsfLogManagerStatusOk;
}

EsfLogManagerStatus EsfLogManagerDeinit(void) {
  EsfLogManagerStatus ret = kEsfLogManagerStatusOk;

  if ((s_log_manager_state == kEsfLogManagerStateInvalid) ||
      (s_log_manager_state == kEsfLogManagerStateNum)) {
    ESF_LOG_MANAGER_ERROR("State transition error. s_log_manager_state=%d\n",
                          s_log_manager_state);
    return kEsfLogManagerStatusFailed;
  }

  if (s_log_manager_state == kEsfLogManagerStateInit) {
    s_log_manager_state = kEsfLogManagerStateInvalid;
    return kEsfLogManagerStatusOk;
  }

  s_log_manager_state = kEsfLogManagerStateInvalid;

  ret = EsfLogManagerInternalDeinit();
  if (ret != kEsfLogManagerStatusOk) {
    ESF_LOG_MANAGER_ERROR("InternalDeinit failed=%d\n", ret);
    return kEsfLogManagerStatusFailed;
  }

#ifdef CONFIG_EXTERNAL_LOG_MANAGER_METRICS
  ret = EsfLogManagerDeinitMetrics();
  if (ret != kEsfLogManagerStatusOk) {
    ESF_LOG_MANAGER_ERROR("%d\n", ret);
    return kEsfLogManagerStatusFailed;
  }
#endif
  return kEsfLogManagerStatusOk;
}

EsfLogManagerStatus EsfLogManagerSetParameter(
    const EsfLogManagerSettingBlockType block_type,
    const EsfLogManagerParameterValue value,
    const EsfLogManagerParameterMask mask) {
  EsfLogManagerStatus ret = kEsfLogManagerStatusOk;

  if (s_log_manager_state != kEsfLogManagerStateStart) {
    ESF_LOG_MANAGER_ERROR("State transition error. s_log_manager_state=%d\n",
                          s_log_manager_state);
    return kEsfLogManagerStatusFailed;
  }

  ret = EsfLogManagerInternalSetParameter(block_type, &value, &mask);
  if (ret != kEsfLogManagerStatusOk) {
    ESF_LOG_MANAGER_ERROR("SetParameter failed\n");
    return ret;
  }

#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
  EsfLogManagerInternalChangeDlogCallback(block_type);
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

  return kEsfLogManagerStatusOk;
}

EsfLogManagerStatus EsfLogManagerGetParameter(
    EsfLogManagerSettingBlockType block_type,
    EsfLogManagerParameterValue *value) {
  if (s_log_manager_state != kEsfLogManagerStateStart) {
    ESF_LOG_MANAGER_ERROR("State transition error. s_log_manager_state=%d\n",
                          s_log_manager_state);
    return kEsfLogManagerStatusFailed;
  }

  if (EsfLogManagerInternalGetParameter(block_type, value) !=
      kEsfLogManagerStatusOk) {
    ESF_LOG_MANAGER_ERROR("GetParameter failed\n");
    return kEsfLogManagerStatusFailed;
  }

  return kEsfLogManagerStatusOk;
}

EsfLogManagerStatus EsfLogManagerGetModuleParameter(
    uint32_t module_id, EsfLogManagerParameterValue *value) {
  EsfLogManagerSettingBlockType block_type = kEsfLogManagerBlockTypeSysApp;

  if (s_log_manager_state != kEsfLogManagerStateStart) {
    ESF_LOG_MANAGER_ERROR("State transition error. s_log_manager_state=%d\n",
                          s_log_manager_state);
    return kEsfLogManagerStatusFailed;
  }

  block_type = EsfLogManagerInternalGetGroupID(module_id);
  if (block_type >= kEsfLogManagerBlockTypeNum) {
    ESF_LOG_MANAGER_ERROR("GetGroupID failed=%d\n", block_type);
    return kEsfLogManagerStatusParamError;
  }

  if (EsfLogManagerInternalGetParameter(block_type, value) !=
      kEsfLogManagerStatusOk) {
    ESF_LOG_MANAGER_ERROR("GetParameter failed\n");
    return kEsfLogManagerStatusFailed;
  }

  return kEsfLogManagerStatusOk;
}

#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
EsfLogManagerStatus EsfLogManagerStoreDlog(uint8_t *str, uint32_t size,
                                           bool is_critical) {
  EsfLogManagerStatus ret = kEsfLogManagerStatusOk;

  if (s_log_manager_state != kEsfLogManagerStateStart) {
    ESF_LOG_MANAGER_ERROR("State transition error. s_log_manager_state=%d\n",
                          s_log_manager_state);
    return kEsfLogManagerStatusFailed;
  }

  ret = EsfLogManagerInternalWriteDlog(str, size, is_critical);
  if (ret != kEsfLogManagerStatusOk) {
    ESF_LOG_MANAGER_ERROR("WriteDlog failed=%d\n", ret);
    return kEsfLogManagerStatusFailed;
  }

  if (s_log_manager_state != kEsfLogManagerStateStart) {
    ESF_LOG_MANAGER_ERROR(
        "[%s] Errors due to state transitions during processing. "
        "s_log_manager_state=%d\n",
        __func__, s_log_manager_state);
    (void)EsfLogManagerDeleteLocalUploadList();
    (void)EsfLogManagerDeleteCloudUploadList();
    return kEsfLogManagerStatusFailed;
  }

  return kEsfLogManagerStatusOk;
}
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

EsfLogManagerStatus EsfLogManagerSendElog(
    const EsfLogManagerElogMessage *message) {
  EsfLogManagerStatus ret = kEsfLogManagerStatusOk;

  if (s_log_manager_state != kEsfLogManagerStateStart) {
    ESF_LOG_MANAGER_ERROR("State transition error. s_log_manager_state=%d\n",
                          s_log_manager_state);
    return kEsfLogManagerStatusFailed;
  }

  ret = EsfLogManagerInternalSendElog(message);
  if (ret != kEsfLogManagerStatusOk) {
    ESF_LOG_MANAGER_WARN("SendElog failed=%d\n", ret);
    return kEsfLogManagerStatusFailed;
  }

  if (s_log_manager_state != kEsfLogManagerStateStart) {
    ESF_LOG_MANAGER_ERROR(
        "[%s] Errors due to state transitions during processing. "
        "s_log_manager_state=%d\n",
        __func__, s_log_manager_state);
    (void)EsfLogManagerInternalClearElog();
    return kEsfLogManagerStatusFailed;
  }

  return kEsfLogManagerStatusOk;
}

EsfLogManagerStatus EsfLogManagerGetLogInfo(
    struct EsfLogManagerLogInfo *log_info) {
  if (EsfLogManagerInternalGetLogInfo(log_info) == false) {
    ESF_LOG_MANAGER_ERROR("GetLogInfo failed\n");
    return kEsfLogManagerStatusFailed;
  }

  LOG_MANAGER_TRACE_PRINT(
      "[%s]"
      "dlog_ram.size:%d, dlog_ram.num:%d, "
      "dlog_flash.size:%d, dlog_flash.num:%d, "
      "elog_ram.size:%d, elog_ram.num:%d, "
      "elog_flash.size:%d, elog_flash.num:%d,\n",
      __func__, log_info->dlog_ram.size, log_info->dlog_ram.num,
      log_info->dlog_flash.size, log_info->dlog_flash.num,
      log_info->elog_ram.size, log_info->elog_ram.num,
      log_info->elog_flash.size, log_info->elog_flash.num);

  return kEsfLogManagerStatusOk;
}

#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
EsfLogManagerStatus EsfLogManagerRegisterChangeDlogCallback(
    uint32_t module_id, EsfLogManagerChangeDlogCallback callback) {
  EsfLogManagerStatus ret = kEsfLogManagerStatusOk;

  if (!((s_log_manager_state == kEsfLogManagerStateInit) ||
        (s_log_manager_state == kEsfLogManagerStateStart))) {
    ESF_LOG_MANAGER_ERROR("State transition error. s_log_manager_state=%d\n",
                          s_log_manager_state);
    return kEsfLogManagerStatusFailed;
  }

  ret = EsfLogManagerInternalRegisterChangeDlogCallback(module_id, &callback);
  if (ret != kEsfLogManagerStatusOk) {
    ESF_LOG_MANAGER_ERROR("RegisterChangeDlogCallback failed=%d\n", ret);
    return kEsfLogManagerStatusFailed;
  }

  if (s_log_manager_state == kEsfLogManagerStateInvalid) {
    ESF_LOG_MANAGER_ERROR(
        "[%s] Errors due to state transitions during processing. "
        "s_log_manager_state=%d\n",
        __func__, s_log_manager_state);
    (void)EsfLogManagerDeleteCallbackList();
    return kEsfLogManagerStatusFailed;

  } else if (s_log_manager_state == kEsfLogManagerStateInit) {
    s_after_callback_flag = true;
  }

  return kEsfLogManagerStatusOk;
}

EsfLogManagerStatus EsfLogManagerUnregisterChangeDlogCallback(
    uint32_t module_id) {
  EsfLogManagerStatus ret = kEsfLogManagerStatusOk;

  if (!((s_log_manager_state == kEsfLogManagerStateInit) ||
        (s_log_manager_state == kEsfLogManagerStateStart))) {
    ESF_LOG_MANAGER_ERROR("State transition error. s_log_manager_state=%d\n",
                          s_log_manager_state);
    return kEsfLogManagerStatusFailed;
  }

  ret = EsfLogManagerInternalUnregisterChangeDlogCallback(module_id);
  if (ret != kEsfLogManagerStatusOk) {
    ESF_LOG_MANAGER_ERROR("UnregisterChangeDlogCallback=%d\n", ret);
    return kEsfLogManagerStatusFailed;
  }

  return kEsfLogManagerStatusOk;
}

EsfLogManagerStatus EsfLogManagerSendBulkDlog(
    uint32_t module_id, size_t size, uint8_t *bulk_log,
    EsfLogManagerBulkDlogCallback callback, void *user_data) {
  EsfLogManagerStatus ret = kEsfLogManagerStatusOk;

  if (s_log_manager_state != kEsfLogManagerStateStart) {
    ESF_LOG_MANAGER_ERROR("State transition error. s_log_manager_state=%d\n",
                          s_log_manager_state);
    return kEsfLogManagerStatusFailed;
  }

  ret = EsfLogManagerInternalSendBulkDlog(module_id, size, bulk_log, callback,
                                          user_data);
  if (ret != kEsfLogManagerStatusOk) {
    ESF_LOG_MANAGER_ERROR("SendBulkDlog failed=%d\n", ret);
    return ret;
  }

  if (s_log_manager_state != kEsfLogManagerStateStart) {
    ESF_LOG_MANAGER_ERROR(
        "[%s] Errors due to state transitions during processing. "
        "s_log_manager_state=%d\n",
        __func__, s_log_manager_state);
    (void)EsfLogManagerDeleteLocalUploadList();
    (void)EsfLogManagerDeleteCloudUploadList();
    return kEsfLogManagerStatusFailed;
  }

  return kEsfLogManagerStatusOk;
}
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE
