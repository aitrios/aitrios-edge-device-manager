/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "firmware_manager_processor.h"

#include <string.h>

#include "firmware_manager_common.h"
#include "firmware_manager_log.h"
#include "firmware_manager_porting_layer.h"
#include "firmware_manager_processor_binary_header_ops.h"

// Internal structure ---------------------------------------------
typedef struct EsfFwMgrProcessorContext {
  FwMgrPlType type;
  EsfFwMgrTarget target;
  FwMgrPlHandle pl_handle;
  EsfMemoryManagerHandle internal_buffer_handle;
  int32_t internal_buffer_size;

  int32_t total_size;
  uint8_t hash[ESF_FIRMWARE_MANAGER_TARGET_HASH_SIZE];

  // binary header
  EsfFwMgrProcessorBinaryHeaderLoadInfo header_load_info;
  EsfFwMgrProcessorBinaryHeaderInfo header_info;
  int32_t total_requested_write_size;
} EsfFwMgrProcessorContext;

// Global variables -----------------------------------------------

static EsfFwMgrProcessorContext *s_active_context = NULL;

// Internal function ----------------------------------------------
static EsfFwMgrResult Target2Type(EsfFwMgrTarget target, FwMgrPlType *type) {
  if (type == NULL) {
    ESF_FW_MGR_DLOG_ERROR("type is NULL.\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x80Processor);
    return kEsfFwMgrResultInvalidArgument;
  }

  switch (target) {
    case kEsfFwMgrTargetProcessorFirmware:
      *type = kFwMgrPlTypeFirmware;
      break;

    case kEsfFwMgrTargetProcessorLoader:
      *type = kFwMgrPlTypeBootloader;
      break;

    default:
      return kEsfFwMgrResultInvalidArgument;
      break;
  }
  return kEsfFwMgrResultOk;
}

// Functions shared via GetOps ------------------------------------

static EsfFwMgrResult EsfFwMgrProcessorInit(void) {
  ESF_FW_MGR_DLOG_INFO("Called.\n");
  PlErrCode pl_ret = FwMgrPlInitialize();
  if (pl_ret != kPlErrCodeOk) {
    ESF_FW_MGR_DLOG_CRITICAL("FwMgrPlInitialize failed. ret = %u\n", pl_ret);
    ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0x90Processor);
    return kEsfFwMgrResultUnavailable;
  }

  return kEsfFwMgrResultOk;
}

static EsfFwMgrResult EsfFwMgrProcessorDeinit(void) {
  ESF_FW_MGR_DLOG_INFO("Called.\n");
  PlErrCode pl_ret = FwMgrPlFinalize();
  if (pl_ret != kPlErrCodeOk) {
    ESF_FW_MGR_DLOG_CRITICAL("FwMgrPlFinalize failed. ret = %u\n", pl_ret);
    ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0x91Processor);
    return kEsfFwMgrResultUnavailable;
  }

  return kEsfFwMgrResultOk;
}

static bool EsfFwMgrProcessorIsSupported(EsfFwMgrTarget target) {
  switch (target) {
    case kEsfFwMgrTargetProcessorLoader:
    case kEsfFwMgrTargetProcessorFirmware:
      break;
    default:
      return false;
  }

  return true;
}

static EsfFwMgrResult EsfFwMgrProcessorOpen(
    const EsfFwMgrOpenRequest *request,
    const EsfFwMgrSubmodulePrepareWriteRequest *prepare_write,
    EsfFwMgrDummyUpdateHandleInfo *dummy_update_handle_info,
    EsfFwMgrSubmoduleHandle *handle, int32_t *writable_size) {
  ESF_FW_MGR_DLOG_INFO("Called.\n");

  // dummy_update_handle_info is not used. (This argument is used by submodules
  // that uses the Sensor AI Lib to update binary.)
  (void)dummy_update_handle_info;

  if (s_active_context != NULL) {
    ESF_FW_MGR_DLOG_ERROR("Submodule Processor has already been opened.\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x81Processor);
    return kEsfFwMgrResultFailedPrecondition;
  }

  // writable_size MUST NOT be NULL because the Processor submodule does not
  // support erasing all the target.
  if ((request == NULL) || (handle == NULL) || (prepare_write == NULL) ||
      (writable_size == NULL)) {
    ESF_FW_MGR_DLOG_ERROR(
        "request, handle or writable_size is NULL. %p, %p, %p, %p\n", request,
        handle, prepare_write, writable_size);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x82Processor);
    return kEsfFwMgrResultInvalidArgument;
  }

  if (request->target == kEsfFwMgrTargetProcessorLoader) {
    ESF_FW_MGR_DLOG_ERROR("Not supported: Processor loader");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x83Processor);
    return kEsfFwMgrResultUnimplemented;
  }

  if (prepare_write->total_size <= 0) {
    ESF_FW_MGR_DLOG_ERROR("prepare_write->total_size must be positive: %d.\n",
                          prepare_write->total_size);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x8dProcessor);
    return kEsfFwMgrResultInvalidArgument;
  }

  EsfFwMgrProcessorContext *context =
      (EsfFwMgrProcessorContext *)malloc(sizeof(EsfFwMgrProcessorContext));
  if (context == NULL) {
    ESF_FW_MGR_DLOG_CRITICAL("Failed to allocate memory for the context.\n");
    ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0x80ProcessorAllocContext);
    return kEsfFwMgrResultResourceExhausted;
  }

  context->target = request->target;

  EsfFwMgrResult ret = Target2Type(request->target, &context->type);
  if (ret != kEsfFwMgrResultOk) {
    ESF_FW_MGR_DLOG_ERROR("Target2Type failed. ret = %u\n", ret);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x84Processor);
    goto err_exit;
  }

  context->internal_buffer_handle = prepare_write->internal_buffer_handle;
  context->internal_buffer_size = prepare_write->internal_buffer_size;
  context->total_size = prepare_write->total_size;
  memcpy(context->hash, request->hash, sizeof(context->hash));

  EsfFwMgrProcessorInitBinaryHeaderLoadInfo(&context->header_load_info);
  context->total_requested_write_size = 0;

  uint32_t max_write_size = 0;
  PlErrCode pl_ret = FwMgrPlGetMaxWriteSize(context->type, &max_write_size);
  if (pl_ret != kPlErrCodeOk) {
    ESF_FW_MGR_DLOG_CRITICAL("FwMgrPlGetMaxWriteSize failed. ret = %u\n",
                             pl_ret);
    ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0x99Processor);
    ret = kEsfFwMgrResultUnavailable;
    goto err_exit;
  }

  if (max_write_size > INT32_MAX) max_write_size = INT32_MAX;
  if ((int32_t)max_write_size > prepare_write->total_size)
    max_write_size = prepare_write->total_size;
  *writable_size = max_write_size;

  context->pl_handle = FW_MGR_PL_INVALID_HANDLE;

  s_active_context = context;
  *handle = (EsfFwMgrSubmoduleHandle)context;

  return kEsfFwMgrResultOk;

err_exit:
  free(context);
  return ret;
}

static EsfFwMgrResult EsfFwMgrProcessorClose(EsfFwMgrSubmoduleHandle handle,
                                             bool aborted) {
  ESF_FW_MGR_DLOG_INFO("Called.\n");

  if (!ESF_FW_MGR_VERIFY_HANDLE(handle)) {
    ESF_FW_MGR_DLOG_ERROR("Invalid handle.\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x85Processor);
    return kEsfFwMgrResultInvalidArgument;
  }

  EsfFwMgrProcessorContext *context = (EsfFwMgrProcessorContext *)handle;

  if (aborted && context->pl_handle != FW_MGR_PL_INVALID_HANDLE) {
    PlErrCode pl_ret = FwMgrPlAbort(context->pl_handle);
    if (pl_ret != kPlErrCodeOk) {
      ESF_FW_MGR_DLOG_CRITICAL("FwMgrPlAbort failed. ret = %u\n", pl_ret);
      ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0x93Processor);
      return kEsfFwMgrResultUnavailable;
    }

    pl_ret = FwMgrPlClose(context->pl_handle, NULL);
    if (pl_ret != kPlErrCodeOk) {
      ESF_FW_MGR_DLOG_CRITICAL("FwMgrPlClose failed. ret = %u\n", pl_ret);
      ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0x94Processor);
      return kEsfFwMgrResultUnavailable;
    }
    context->pl_handle = FW_MGR_PL_INVALID_HANDLE;
  }

  EsfFwMgrProcessorDeinitBinaryHeaderLoadInfo(&context->header_load_info);
  free(context);
  s_active_context = NULL;

  return kEsfFwMgrResultOk;
}

static EsfFwMgrResult EsfFwMgrProcessorWrite(
    EsfFwMgrSubmoduleHandle handle, const EsfFwMgrWriteRequest *request) {
  ESF_FW_MGR_DLOG_INFO("Called.\n");

  if (!ESF_FW_MGR_VERIFY_HANDLE(handle)) {
    ESF_FW_MGR_DLOG_ERROR("Invalid handle.\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x86Processor);
    return kEsfFwMgrResultInvalidArgument;
  }

  EsfFwMgrProcessorContext *context = (EsfFwMgrProcessorContext *)handle;

  if (request == NULL) {
    ESF_FW_MGR_DLOG_ERROR("request is NULL.\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x87Processor);
    return kEsfFwMgrResultInvalidArgument;
  }

  if (request->offset < 0 || request->size <= 0 ||
      context->internal_buffer_size < request->offset + request->size) {
    ESF_FW_MGR_DLOG_ERROR(
        "Invalid offset or size. offset: %d, size: %d (Internal buffer size: "
        "%d\n",
        request->offset, request->size, context->internal_buffer_size);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x8cProcessor);
    return kEsfFwMgrResultInvalidArgument;
  }

  if (!EsfFwMgrProcessorIsBinaryHeaderLoaded(&context->header_load_info)) {
    EsfFwMgrProcessorLoadBinaryHeaderRequest load_request = {
        .src_buffer_handle = context->internal_buffer_handle,
        .src_size = request->size,
        .src_offset = request->offset,
    };
    EsfFwMgrResult ret = EsfFwMgrProcessorLoadBinaryHeader(
        &load_request, &context->header_load_info, &context->header_info);
    if (ret != kEsfFwMgrResultOk) {
      ESF_FW_MGR_DLOG_ERROR("EsfFwMgrProcessorCopyBinaryHeader failed.\n");
      ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x8eProcessor);
      return ret;
    }

    if (!EsfFwMgrProcessorIsBinaryHeaderLoaded(&context->header_load_info)) {
      // If the write_size in the first EsfFwMgrWrite is less than the header
      // magic size, it cannot determine whether the header is appended or not,
      // and therefore cannot decide whether to write the data.
      if (!EsfFwMgrProcessorIsBinaryHeaderMagicLoaded(
              &context->header_load_info)) {
        ESF_FW_MGR_DLOG_CRITICAL("Header magic has not been loaded.\n");
        ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0x9bProcessor);
        return kEsfFwMgrResultAborted;
      }

      context->total_requested_write_size += request->size;
      return kEsfFwMgrResultOk;
    }
  }

  int32_t skipped_size = context->header_info.header_size -
                         context->total_requested_write_size;
  if (skipped_size < 0) skipped_size = 0;

  context->total_requested_write_size += request->size;
  if (request->size <= skipped_size) return kEsfFwMgrResultOk;

  if (context->pl_handle == FW_MGR_PL_INVALID_HANDLE) {
    context->total_size -= context->header_info.header_size;
    uint32_t size;
    PlErrCode pl_ret = FwMgrPlOpen(context->type, context->total_size,
                                   context->hash, &context->pl_handle, &size);
    if (pl_ret != kPlErrCodeOk) {
      ESF_FW_MGR_DLOG_CRITICAL("FwMgrPlOpen failed. ret = %u\n", pl_ret);
      ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0x92Processor);
      context->pl_handle = FW_MGR_PL_INVALID_HANDLE;
      return kEsfFwMgrResultAborted;
    }
  }

  uint32_t size = request->size - skipped_size;
  uint32_t offset = request->offset + skipped_size;

  uint32_t written_size = 0;
  PlErrCode pl_ret = FwMgrPlWrite(context->pl_handle,
                                  context->internal_buffer_handle, offset, size,
                                  &written_size);
  if (pl_ret != kPlErrCodeOk) {
    ESF_FW_MGR_DLOG_CRITICAL("FwMgrPlWrite failed. ret = %u\n", pl_ret);
    ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0x95Processor);
    return kEsfFwMgrResultAborted;
  }

  // TODO: Try to write the remaining data if
  // written_size < data_size.
  if (written_size != (uint32_t)size) {
    ESF_FW_MGR_DLOG_CRITICAL(
        "Failed to write. data_size: 0x%x, written_size = 0x%x\n", size,
        written_size);
    ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0x96Processor);
    return kEsfFwMgrResultAborted;
  }

  return kEsfFwMgrResultOk;
}

static EsfFwMgrResult EsfFwMgrProcessorPostProcess(
    EsfFwMgrSubmoduleHandle handle) {
  ESF_FW_MGR_DLOG_INFO("Called.\n");

  if (!ESF_FW_MGR_VERIFY_HANDLE(handle)) {
    ESF_FW_MGR_DLOG_ERROR("Invalid handle.\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x88Processor);
    return kEsfFwMgrResultInvalidArgument;
  }

  EsfFwMgrProcessorContext *context = (EsfFwMgrProcessorContext *)handle;

  bool switch_slot = true;
  if (EsfFwMgrProcessorIsBinaryHeaderLoaded(&context->header_load_info) &&
      (context->header_info.sw_arch_version == kEsfFwMgrSwArchVersion1)) {
    switch_slot = false;
  }

  // Close the porting layer
  PlErrCode pl_ret = kPlErrInternal;
  if (switch_slot) {
    pl_ret = FwMgrPlClose(context->pl_handle, NULL);
  } else {
    pl_ret = FwMgrPlCloseWithoutPartitionSwitch(context->pl_handle, NULL);
  }

  if (pl_ret != kPlErrCodeOk) {
    ESF_FW_MGR_DLOG_CRITICAL(
        "FwMgrPlClose (switch_slot = %d) failed. ret = %u\n", switch_slot,
        pl_ret);
    ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0x97Processor);
    return kEsfFwMgrResultAborted;
  }
  context->pl_handle = FW_MGR_PL_INVALID_HANDLE;

  return kEsfFwMgrResultOk;
}

static EsfFwMgrResult EsfFwMgrProcessorGetBinaryHeaderInfo(
    EsfFwMgrSubmoduleHandle handle, EsfFwMgrBinaryHeaderInfo *info) {
  ESF_FW_MGR_DLOG_INFO("Called.\n");

  if (!ESF_FW_MGR_VERIFY_HANDLE(handle)) {
    ESF_FW_MGR_DLOG_ERROR("Invalid handle.\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x8fProcessor);
    return kEsfFwMgrResultInvalidArgument;
  }

  if (info == NULL) {
    ESF_FW_MGR_DLOG_ERROR("info is NULL\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x90Processor);
    return kEsfFwMgrResultInvalidArgument;
  }

  EsfFwMgrProcessorContext *context = (EsfFwMgrProcessorContext *)handle;

  if (!EsfFwMgrProcessorIsBinaryHeaderLoaded(&context->header_load_info)) {
    ESF_FW_MGR_DLOG_ERROR(
        "Loading the binary header has not been completed.\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x91Processor);
    return kEsfFwMgrResultFailedPrecondition;
  }

  info->sw_arch_version = context->header_info.sw_arch_version;

  return kEsfFwMgrResultOk;
}

static EsfFwMgrResult EsfFwMgrProcessorGetInfo(EsfFwMgrGetInfoData *data) {
  ESF_FW_MGR_DLOG_INFO("Called.\n");

  if (data == NULL || data->response == NULL) {
    ESF_FW_MGR_DLOG_ERROR("data or data->response is NULL.\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x89Processor);
    return kEsfFwMgrResultInvalidArgument;
  }

  if (data->in_length < 1) {
    ESF_FW_MGR_DLOG_ERROR("data->in_length is less than 1. %d\n",
                          data->in_length);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x8aProcessor);
    return kEsfFwMgrResultInvalidArgument;
  }

  FwMgrPlType type;
  EsfFwMgrResult ret = Target2Type(data->target, &type);
  if (ret != kEsfFwMgrResultOk) {
    ESF_FW_MGR_DLOG_ERROR("Target2Type failed. ret = %u\n", ret);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x8bProcessor);
    return ret;
  }

  data->out_length = 1;

  PlErrCode pl_ret = FwMgrPlGetInfo(
      type, sizeof(data->response->version), data->response->version,
      sizeof(data->response->hash), data->response->hash,
      sizeof(data->response->last_update), data->response->last_update);
  if (pl_ret != kPlErrCodeOk) {
    ESF_FW_MGR_DLOG_CRITICAL("FwMgrPlGetInfo failed. ret = %u\n", pl_ret);
    ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0x98Processor);
    return kEsfFwMgrResultInternal;
  }

  if (data->target == kEsfFwMgrTargetProcessorFirmware) {
    SAFE_STRNCPY(data->response->version, CONFIG_FIRMWARE_VERSION,
                 sizeof(data->response->version));
  }

  return kEsfFwMgrResultOk;
}

// Public functions -----------------------------------------------
static const EsfFwMgrSubmoduleOps kFwMgrProcessorOps = {
    .init = EsfFwMgrProcessorInit,
    .deinit = EsfFwMgrProcessorDeinit,
    .is_supported = EsfFwMgrProcessorIsSupported,
    .open = EsfFwMgrProcessorOpen,
    .close = EsfFwMgrProcessorClose,
    .write = EsfFwMgrProcessorWrite,
    .erase = NULL,
    .post_process = EsfFwMgrProcessorPostProcess,
    .get_binary_header_info = EsfFwMgrProcessorGetBinaryHeaderInfo,
    .get_info = EsfFwMgrProcessorGetInfo,
};

EsfFwMgrResult EsfFwMgrProcessorGetOps(EsfFwMgrSubmoduleOps *ops) {
  ESF_FW_MGR_DLOG_INFO("Called.\n");

  *ops = kFwMgrProcessorOps;
  return kEsfFwMgrResultOk;
}
