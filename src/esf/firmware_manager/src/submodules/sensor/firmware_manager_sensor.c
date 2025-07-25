/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "firmware_manager_sensor.h"

#include "firmware_manager_common.h"
#include "firmware_manager_log.h"

#ifdef CONFIG_EXTERNAL_FIRMWARE_MANAGER_USE_SENSOR_FW_UPDATE_LIB
#include "sensor_fw_update_lib.h"
#define SsfSensorLibFwUpdateBegin2 EdcSensorFwUpdateLibBegin2
#define SsfSensorLibFwUpdateComplete EdcSensorFwUpdateLibComplete
#define SsfSensorLibFwUpdateCancel EdcSensorFwUpdateLibCancel
#define SsfSensorLibFwUpdateWrite EdcSensorFwUpdateLibWrite
#define SsfSensorLibFwUpdateErase EdcSensorFwUpdateLibErase
#define SsfSensorLibFwUpdateGetMaxDataSizeOnce \
  EdcSensorFwUpdateLibGetMaxDataSizeOnce
#define SsfSensorLibFwUpdateGetComponentInfoList \
  EdcSensorFwUpdateLibGetComponentInfoList
#define SsfSensorLibFwUpdateHandle EdcSensorFwUpdateLibHandle
#define SsfSensorLibResult EdcSensorFwUpdateLibResult
#define kSsfSensorLibResultOk kEdcSensorFwUpdateLibResultOk
#define kSsfSensorLibResultInvalidArgument \
  kEdcSensorFwUpdateLibResultInvalidArgument
#define SsfSensorLibFwUpdateTarget EdcSensorFwUpdateLibTarget
#define kSsfSensorLibFwUpdateTargetLoader kEdcSensorFwUpdateLibTargetLoader
#define kSsfSensorLibFwUpdateTargetFirmware kEdcSensorFwUpdateLibTargetFirmware
#define kSsfSensorLibFwUpdateTargetAIModel kEdcSensorFwUpdateLibTargetAIModel
#define kSsfSensorLibFwUpdateTargetNum kEdcSensorFwUpdateLibTargetNum
#define SsfSensorLibComponentInfo EdcSensorFwUpdateLibComponentInfo

#else /* CONFIG_EXTERNAL_FIRMWARE_MANAGER_USE_SENSOR_FW_UPDATE_LIB */
#include "sensor_ai_lib/sensor_ai_lib_fwupdate.h"
#endif /* else CONFIG_EXTERNAL_FIRMWARE_MANAGER_USE_SENSOR_FW_UPDATE_LIB */

// typedef ---------------------------------------------------------------------
typedef struct EsfFwMgrSensorContext {
  // Sensor AI Library handle
  SsfSensorLibFwUpdateHandle handle;
  // Subject to renewal.
  EsfFwMgrTarget target;
  // OpenAPI Operation Write or Erase
  bool is_ope_write;
  // memory manager handle
  EsfMemoryManagerHandle internal_buffer_handle;
  // Size of internal buffer allocated with the Memory Manager
  int32_t internal_buffer_size;
} EsfFwMgrSensorContext;

// Internal function define ----------------------------------------------------
static bool InvokeIsSupported(EsfFwMgrTarget target);
static SsfSensorLibFwUpdateTarget ConvertTarget(EsfFwMgrTarget target);

// Internal function impl ------------------------------------------------------
static bool InvokeIsSupported(EsfFwMgrTarget target) {
  switch (target) {
    case kEsfFwMgrTargetSensorLoader:
    case kEsfFwMgrTargetSensorFirmware:
    case kEsfFwMgrTargetAIModel:
      break;
    default:
      return false;
  }
  return true;
}

static SsfSensorLibFwUpdateTarget ConvertTarget(EsfFwMgrTarget target) {
  SsfSensorLibFwUpdateTarget ssf_target = kSsfSensorLibFwUpdateTargetNum;
  switch (target) {
    case kEsfFwMgrTargetSensorLoader:
      ssf_target = kSsfSensorLibFwUpdateTargetLoader;
      break;
    case kEsfFwMgrTargetSensorFirmware:
      ssf_target = kSsfSensorLibFwUpdateTargetFirmware;
      break;
    case kEsfFwMgrTargetAIModel:
      ssf_target = kSsfSensorLibFwUpdateTargetAIModel;
      break;
    default:
      ssf_target = kSsfSensorLibFwUpdateTargetNum;
      break;
  }
  return ssf_target;
}

// Functions shared via GetOps
// -------------------------------------------------
static EsfFwMgrResult EsfFwMgrSensorInit(void) {
  ESF_FW_MGR_DLOG_INFO("Called.\n");
  return kEsfFwMgrResultOk;
}

static EsfFwMgrResult EsfFwMgrSensorDeinit(void) {
  ESF_FW_MGR_DLOG_INFO("Called.\n");
  return kEsfFwMgrResultOk;
}

static bool EsfFwMgrSensorIsSupported(EsfFwMgrTarget target) {
  ESF_FW_MGR_DLOG_INFO("Called.\n");
  return InvokeIsSupported(target);
}

static EsfFwMgrResult EsfFwMgrSensorOpen(
    const EsfFwMgrOpenRequest* request,
    const EsfFwMgrSubmodulePrepareWriteRequest* prepare_write,
    EsfFwMgrDummyUpdateHandleInfo* dummy_update_handle_info,
    EsfFwMgrSubmoduleHandle* handle, int32_t* writable_size) {
  ESF_FW_MGR_DLOG_INFO("Called.\n");

  if (request == NULL || dummy_update_handle_info == NULL || handle == NULL) {
    ESF_FW_MGR_DLOG_ERROR("request, sensor_handle_info or handle NULL\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0xa0Sensor);
    return kEsfFwMgrResultInvalidArgument;
  }

  if (InvokeIsSupported(request->target) == false) {
    ESF_FW_MGR_DLOG_ERROR(
        "Skip as it is not subject to update in Sensor AI.\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0xa1Sensor);
    return kEsfFwMgrResultInvalidArgument;
  }

  SsfSensorLibResult ret_ssf;
#if defined(CONFIG_EXTERNAL_TARGET_T3P) || defined(CONFIG_EXTERNAL_TARGET_T5)
  // Stop the dummy FW Update that the core block began
  ret_ssf = SsfSensorLibFwUpdateCancel(dummy_update_handle_info->handle);
  if (ret_ssf != kSsfSensorLibResultOk) {
    ESF_FW_MGR_DLOG_CRITICAL(
        "SsfSensorLibFwUpdateCancel func failed. ret = %u\n", ret_ssf);
    ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0xb2Sensor);
    return kEsfFwMgrResultUnavailable;
  }
  dummy_update_handle_info->canceled = true;
#endif /* CONFIG_EXTERNAL_TARGET_T3P || CONFIG_EXTERNAL_TARGET_T5 */

  EsfFwMgrSensorContext* ctx =
      (EsfFwMgrSensorContext*)malloc(sizeof(EsfFwMgrSensorContext));
  if (ctx == NULL) {
    ESF_FW_MGR_DLOG_CRITICAL("memory allocation error\n");
    ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0xa1SensorAllocContext);
    return kEsfFwMgrResultResourceExhausted;
  }

  SsfSensorLibComponentInfo ssf_info = {0};
  ssf_info.version[0] = '\0';
  SAFE_STRNCPY(ssf_info.parameter_name, "", sizeof(ssf_info.parameter_name));
  memcpy(ssf_info.hash, request->hash, sizeof(ssf_info.hash));
  bool is_ope_write = prepare_write ? true : false;
  if (is_ope_write) {
    SAFE_STRNCPY(ssf_info.version, request->version, sizeof(ssf_info.version));

    ssf_info.total_size = prepare_write->total_size;
  } else {
    ssf_info.total_size = 0;
  }
  ret_ssf = SsfSensorLibFwUpdateBegin2(ConvertTarget(request->target),
                                       request->name, &ssf_info, &ctx->handle);
  if (ret_ssf != kSsfSensorLibResultOk) {
    free(ctx);
    ctx = NULL;
    ESF_FW_MGR_DLOG_CRITICAL(
        "SsfSensorLibFwUpdateBegin2 func failed. ret = %u\n", ret_ssf);
    ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0xb3Sensor);
    return kEsfFwMgrResultUnavailable;
  }

  ctx->target = request->target;

  EsfFwMgrResult ret = kEsfFwMgrResultInternal;
  if (is_ope_write) {
    if (writable_size == NULL) {
      ESF_FW_MGR_DLOG_ERROR(
          "writable_size must not be NULL when prepare_write is not NULL\n");
      ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0xa3Sensor);
      ret = kEsfFwMgrResultInvalidArgument;
      goto fwupdate_cancel;
    }

    ctx->is_ope_write = true;
    uint32_t size = 0;
    ret_ssf = SsfSensorLibFwUpdateGetMaxDataSizeOnce(ctx->handle, &size);
    if (ret_ssf != kSsfSensorLibResultOk) {
      ESF_FW_MGR_DLOG_CRITICAL(
          "SsfSensorLibFwUpdateGetMaxDataSizeOnce func failed. ret = %u\n",
          ret_ssf);
      ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0xb4Sensor);
      ret = kEsfFwMgrResultUnavailable;
      goto fwupdate_cancel;
    }

    *writable_size = size;

    ctx->internal_buffer_handle = prepare_write->internal_buffer_handle;
    ctx->internal_buffer_size = prepare_write->internal_buffer_size;
  } else {
    ctx->is_ope_write = false;
  }

  *handle = (EsfFwMgrSubmoduleHandle)ctx;

  return kEsfFwMgrResultOk;

fwupdate_cancel:
  ret_ssf = SsfSensorLibFwUpdateCancel(ctx->handle);
  if (ret_ssf != kSsfSensorLibResultOk) {
    ESF_FW_MGR_DLOG_CRITICAL(
        "SsfSensorLibFwUpdateCancel func failed. ret = %u\n", ret_ssf);
    ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0xb5Sensor);
    if (ctx->handle != NULL) {
      free(ctx->handle);
    }
  }
  ctx->handle = NULL;
  if (ctx != NULL) {
    free(ctx);
    ctx = NULL;
  }
  return ret;
}

static EsfFwMgrResult EsfFwMgrSensorClose(EsfFwMgrSubmoduleHandle handle,
                                          bool aborted) {
  ESF_FW_MGR_DLOG_INFO("Called.\n");

  EsfFwMgrSensorContext* ctx = (EsfFwMgrSensorContext*)handle;
  if (ctx == NULL) {
    ESF_FW_MGR_DLOG_ERROR("handle NULL\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0xa4Sensor);
    return kEsfFwMgrResultInvalidArgument;
  }

  SsfSensorLibResult ret_ssf = kSsfSensorLibResultOk;
  if (aborted && (ctx->handle != NULL)) {
    ret_ssf = SsfSensorLibFwUpdateCancel(ctx->handle);
    if (ret_ssf != kSsfSensorLibResultOk) {
      ESF_FW_MGR_DLOG_CRITICAL(
          "SsfSensorLibFwUpdateCancel func failed. ret = %u\n", ret_ssf);
      ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0xb6Sensor);
      return kEsfFwMgrResultUnavailable;
    }

    ctx->handle = NULL;
  }

  free(ctx);
  ctx = NULL;

  return kEsfFwMgrResultOk;
}

static EsfFwMgrResult EsfFwMgrSensorWrite(EsfFwMgrSubmoduleHandle handle,
                                          const EsfFwMgrWriteRequest* request) {
  ESF_FW_MGR_DLOG_INFO("Called.\n");
  EsfFwMgrSensorContext* ctx = (EsfFwMgrSensorContext*)handle;
  if (ctx == NULL || ctx->handle == NULL) {
    ESF_FW_MGR_DLOG_ERROR("ctx or ctx->handle NULL\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0xa5Sensor);
    return kEsfFwMgrResultInvalidArgument;
  }

  if (request == NULL) {
    ESF_FW_MGR_DLOG_ERROR("request is NULL\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0xa6Sensor);
    return kEsfFwMgrResultInvalidArgument;
  }

  if (request->offset < 0 || request->size < 0 ||
      ctx->internal_buffer_size < request->size + request->offset) {
    ESF_FW_MGR_DLOG_ERROR("request->offset or request->size is invalid.\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0xa7Sensor);
    return kEsfFwMgrResultInvalidArgument;
  }

  SsfSensorLibResult ret_ssf = SsfSensorLibFwUpdateWrite(
      ctx->handle, ctx->internal_buffer_handle + request->offset,
      request->size);
  if (ret_ssf != kSsfSensorLibResultOk) {
    ESF_FW_MGR_DLOG_CRITICAL(
        "SsfSensorLibFwUpdateWrite func failed. ret = %u\n", ret_ssf);
    ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0xb9Sensor);
    return kEsfFwMgrResultUnavailable;
  }

  return kEsfFwMgrResultOk;
}

static EsfFwMgrResult EsfFwMgrSensorErase(EsfFwMgrSubmoduleHandle handle) {
  ESF_FW_MGR_DLOG_INFO("Called.\n");

  EsfFwMgrSensorContext* ctx = (EsfFwMgrSensorContext*)handle;
  if (ctx == NULL || ctx->handle == NULL) {
    ESF_FW_MGR_DLOG_ERROR("ctx or ctx->handle NULL\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0xa8Sensor);
    return kEsfFwMgrResultInvalidArgument;
  }

  if (ctx->target != kEsfFwMgrTargetAIModel) {
    ESF_FW_MGR_DLOG_ERROR("%u is not support.\n", ctx->target);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0xa9Sensor);
    return kEsfFwMgrResultInvalidArgument;
  }

  SsfSensorLibResult ret_ssf = SsfSensorLibFwUpdateErase(ctx->handle);
  if (ret_ssf != kSsfSensorLibResultOk) {
    ESF_FW_MGR_DLOG_CRITICAL(
        "SsfSensorLibFwUpdateErase func failed. ret = %u\n", ret_ssf);
    ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0xbaSensor);
    return kEsfFwMgrResultUnavailable;
  }

  return kEsfFwMgrResultOk;
}

static EsfFwMgrResult EsfFwMgrSensorPostProcess(
    EsfFwMgrSubmoduleHandle handle) {
  ESF_FW_MGR_DLOG_INFO("Called.\n");

  EsfFwMgrSensorContext* ctx = (EsfFwMgrSensorContext*)handle;
  if (ctx == NULL || ctx->handle == NULL ||
      (InvokeIsSupported(ctx->target) == false)) {
    ESF_FW_MGR_DLOG_ERROR("Invalid argument value\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0xaaSensor);
    return kEsfFwMgrResultInvalidArgument;
  }

  EsfFwMgrResult ret = kEsfFwMgrResultOk;
  SsfSensorLibResult ret_ssf = SsfSensorLibFwUpdateComplete(ctx->handle);
  if (ret_ssf != kSsfSensorLibResultOk) {
    ESF_FW_MGR_DLOG_CRITICAL(
        "SsfSensorLibFwUpdateComplete func failed. ret = %u\n", ret_ssf);
    ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0xb7Sensor);

    if (ret_ssf != kSsfSensorLibResultInvalidArgument) {
      ret_ssf = SsfSensorLibFwUpdateCancel(ctx->handle);
      if (ret_ssf != kSsfSensorLibResultOk) {
        ESF_FW_MGR_DLOG_CRITICAL(
            "SsfSensorLibFwUpdateCancel func failed. ret = %u\n", ret_ssf);
        ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0xb8Sensor);
        return kEsfFwMgrResultUnavailable;
      }
    }

    ret = kEsfFwMgrResultUnavailable;
  }

  ctx->handle = NULL;

  return ret;
}

static EsfFwMgrResult EsfFwMgrSensorGetInfo(EsfFwMgrGetInfoData* data) {
  ESF_FW_MGR_DLOG_INFO("Called.\n");

  if (data == NULL || data->response == NULL) {
    ESF_FW_MGR_DLOG_ERROR("data or data->response is NULL\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0xabSensor);
    return kEsfFwMgrResultInvalidArgument;
  }

  if (InvokeIsSupported(data->target) == false) {
    ESF_FW_MGR_DLOG_ERROR(
        "Skip as it is not subject to update in Sensor AI.\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0xacSensor);
    return kEsfFwMgrResultInvalidArgument;
  }

  bool correct_inlen_other_ai =
      (data->in_length == 1) &&
      (data->target == kEsfFwMgrTargetSensorLoader ||
       data->target == kEsfFwMgrTargetSensorFirmware ||
       data->target == kEsfFwMgrTargetSensorCalibrationParam);
  bool correct_inlen_ai =
      (data->in_length == ESF_FIRMWARE_MANAGER_AI_MODEL_SLOT_NUM) &&
      (data->target == kEsfFwMgrTargetAIModel);

  if ((correct_inlen_other_ai == false) && (correct_inlen_ai == false)) {
    ESF_FW_MGR_DLOG_ERROR("Incorrect value for in_length:%d, target=%u\n",
                          data->in_length, data->target);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0xadSensor);
    return kEsfFwMgrResultInvalidArgument;
  }

  if (SsfSensorLibGetState() == kSsfSensorLibStateFwUpdate) {
    ESF_FW_MGR_DLOG_ERROR("Can not be used during SsfSensorLib status=%d\n",
                          kSsfSensorLibStateFwUpdate);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0xafSensor);
    return kEsfFwMgrResultFailedPrecondition;
  }

  data->out_length = data->in_length;
  SsfSensorLibComponentInfo* ssf_list = NULL;
  uint32_t ssf_info_num = 0;
  if (correct_inlen_other_ai) {
    ssf_info_num = 1;
  } else if (correct_inlen_ai) {
    ssf_info_num = ESF_FIRMWARE_MANAGER_AI_MODEL_SLOT_NUM;
  }
  if (0 < ssf_info_num) {
    ssf_list = (SsfSensorLibComponentInfo*)malloc(
        sizeof(SsfSensorLibComponentInfo) * ssf_info_num);
    if (ssf_list == NULL) {
      ESF_FW_MGR_DLOG_CRITICAL("memory allocation error\n");
      ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0xbcSensor);
      return kEsfFwMgrResultResourceExhausted;
    }
    memset(ssf_list, 0, sizeof(SsfSensorLibComponentInfo) * ssf_info_num);
  }
  SsfSensorLibResult ret_ssf = SsfSensorLibFwUpdateGetComponentInfoList(
      ConvertTarget(data->target), data->name, &ssf_info_num, ssf_list);
  if (ret_ssf != kSsfSensorLibResultOk) {
    ESF_FW_MGR_DLOG_CRITICAL(
        "SsfSensorLibFwUpdateGetComponentInfoList func failed. ret = %u\n",
        ret_ssf);
    ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0xbbSensor);
    free(ssf_list);
    ssf_list = NULL;
    return kEsfFwMgrResultUnavailable;
  }

  uint32_t num_slots = data->out_length;
  for (uint32_t i = 0; i < num_slots; i++) {
    EsfFwMgrGetInfoResponse* res = &data->response[i];
    SsfSensorLibComponentInfo* ssf_info = &ssf_list[i];
    if (ssf_info->valid && (i < ssf_info_num)) {
      SAFE_STRNCPY(res->version, ssf_info->version, sizeof(res->version));
      SAFE_STRNCPY(res->last_update, ssf_info->update_date,
                   sizeof(res->last_update));
      memcpy(res->hash, ssf_info->hash, sizeof(res->hash));

    } else {
      // When info is not found
      res->version[0] = '\0';
      res->last_update[0] = '\0';
      memset(res->hash, 0, sizeof(res->hash));
    }
  }

  free(ssf_list);
  ssf_list = NULL;
  return kEsfFwMgrResultOk;
}

// Public functions
// ------------------------------------------------------------
static const EsfFwMgrSubmoduleOps kFwMgrSensorOps = {
    .init = EsfFwMgrSensorInit,
    .deinit = EsfFwMgrSensorDeinit,
    .is_supported = EsfFwMgrSensorIsSupported,
    .open = EsfFwMgrSensorOpen,
    .close = EsfFwMgrSensorClose,
    .write = EsfFwMgrSensorWrite,
    .erase = EsfFwMgrSensorErase,
    .post_process = EsfFwMgrSensorPostProcess,
    .get_info = EsfFwMgrSensorGetInfo};

EsfFwMgrResult EsfFwMgrSensorGetOps(EsfFwMgrSubmoduleOps* ops) {
  ESF_FW_MGR_DLOG_INFO("Called\n");
  *ops = kFwMgrSensorOps;
  return kEsfFwMgrResultOk;
}
