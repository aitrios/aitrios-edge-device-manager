/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// This is the "Firmware Manager Porting Layer (Fw Mgr PL) ESP32 I/F Block", a
// part of Firmware Manager Porting Layer. This block handles ESP32/ESP32-S3
// specific operations in Fw Mgr PL and called by the functions of the "Fw Mgr
// PL Common Block" (in firmware_manager_porting_layer.c)

// This block does NOT manage the state of Fw Mgr PL, as it is done by the Fw
// Mgr PL Common block (written in firmware_manager_porting_layer.c).
// The public functions in this block MUST NOT be executed at the same time by
// multiple threads/processes. (To make sure of it is the responsibility of the
// Fw Mgr PL Common block.)

#include "firmware_manager_esp32_if.h"

#include <stdlib.h>
#include <string.h>

#include "firmware_manager_esp32_fw.h"
#include "firmware_manager_esp32_loader_processor_specific.h"
#include "firmware_manager_porting_layer_log.h"
#include "verify_handle.h"

// Internal enumerated type and structures -------------------------------------
typedef struct TagFwMgrEsp32IfContext {
  FwMgrEsp32IfHandle impl_handle;
  FwMgrEsp32ImplOps ops;
} FwMgrEsp32IfContext;

// Global variables ------------------------------------------------------------

// Currently used context. Only one context can exist.
static FwMgrEsp32IfContext *s_active_context = NULL;

static const FwMgrPlOperationSupportInfo kSupportInfo = {
    .firmware = {.update_supported = true,
                 .update_abort_supported = true,
                 .rollback_supported = false,
                 .factory_reset_supported = false},
    .bootloader = {.update_supported = false,
                   .update_abort_supported = false,
                   .rollback_supported = false,
                   .factory_reset_supported = false},
    .partition_table = {.update_supported = false,
                        .update_abort_supported = false,
                        .rollback_supported = false,
                        .factory_reset_supported = false}};

// Public functions ------------------------------------------------------------

PlErrCode FwMgrEsp32IfInitialize(void) {
  FW_MGR_PL_DLOG_DEBUG("Start.\n");

  FwMgrEsp32FwMigrateFromV1();

  return kPlErrCodeOk;
}

PlErrCode FwMgrEsp32IfFinalize(void) {
  FW_MGR_PL_DLOG_DEBUG("Start.\n");

  if (s_active_context) {
    if (s_active_context->ops.finalize) {
      PlErrCode ret = s_active_context->ops.finalize();
      if (ret != kPlErrCodeOk) {
        FW_MGR_PL_DLOG_ERROR("s_active_context->ops.finalize() failed.\n");
        FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x40If);
        return ret;
      }
    }
    free(s_active_context);
    s_active_context = NULL;
  }

  return kPlErrCodeOk;
}

/// @brief
/// @param type [in] AP Binary type
/// @param total_write_size [in] Size of AP Binary in bytes.
/// @param handle [out] I/F Handle that is created by this function
/// @param max_write_size [out] Max value that can be set to `write_size` when
/// calling `FwMgrEsp32IfWrite`.
/// @return kOk: success, otherwise: failed
PlErrCode FwMgrEsp32IfOpen(FwMgrPlType type, uint32_t total_write_size,
                           const uint8_t *hash, FwMgrEsp32IfHandle *handle,
                           uint32_t *max_write_size) {
  FW_MGR_PL_DLOG_DEBUG("Start.\n");

  if (s_active_context != NULL) {
    FW_MGR_PL_DLOG_ERROR("There is an active handle.\n");
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x41If);
    return kPlErrInvalidState;
  }

  if (handle == NULL) {
    FW_MGR_PL_DLOG_ERROR("`handle` is NULL.\n");
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x42If);
    return kPlErrInvalidParam;
  }

  FwMgrEsp32IfContext *context =
      (FwMgrEsp32IfContext *)malloc(sizeof(FwMgrEsp32IfContext));
  if (context == NULL) {
    FW_MGR_PL_DLOG_CRITICAL("Failed to allocate memory for context.\n");
    FW_MGR_PL_ELOG_CRITICAL(kFwMgrPlElogCriticalId0x40IfAllocContext);
    return kPlErrMemory;
  }

  context->ops.finalize = NULL;
  context->ops.open = NULL;
  context->ops.close = NULL;
  context->ops.write = NULL;
  context->ops.abort = NULL;

  PlErrCode ret = kPlErrCodeError;

  switch (type) {
    case kFwMgrPlTypeFirmware:
      ret = FwMgrEsp32FwGetOps(&context->ops);
      if (ret != kPlErrCodeOk) {
        FW_MGR_PL_DLOG_ERROR("FwMgrEsp32FwGetOps() failed\n");
        FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x43If);
        goto err_exit;
      }
      break;

    case kFwMgrPlTypeBootloader:
    case kFwMgrPlTypePartitionTable:
      FW_MGR_PL_DLOG_ERROR("`type` = %u is not supported.\n", type);
      FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x44If);
      ret = kPlErrNoSupported;
      goto err_exit;

    default:
      FW_MGR_PL_DLOG_ERROR("`type` = %u is out of the range.\n", type);
      FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x45If);
      ret = kPlErrInvalidParam;
      goto err_exit;
  }

  if (context->ops.open == NULL) {
    FW_MGR_PL_DLOG_ERROR("context->ops.open() is not registered.\n");
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x46If);
    ret = kPlErrInternal;
    goto err_exit;
  }

  ret = context->ops.open(total_write_size, hash, &context->impl_handle,
                          max_write_size);
  if (ret != kPlErrCodeOk) {
    FW_MGR_PL_DLOG_ERROR("context->ops.open() failed.\n");
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x47If);
    goto err_exit;
  }

  s_active_context = context;
  *handle = (FwMgrEsp32IfHandle)context;

  return kPlErrCodeOk;

err_exit:
  free(context);
  return ret;
}

/// @brief
/// @param handle [in] specify I/F handle which can be obtained with the
/// FwMgrEsp32IfOpen() function.
/// @param aborted [in] Whether the Fw Mgr PL is Aborted state or not.
/// @param updated [out] Whether the AP Binary has been successfully updated.
/// @return kOk: success, otherwise: failed
PlErrCode FwMgrEsp32IfClose(FwMgrEsp32IfHandle handle, bool aborted,
                            bool switch_partition, bool *updated) {
  FW_MGR_PL_DLOG_DEBUG("Start.\n");

  if (updated) *updated = false;

  if (!FW_MGR_PL_VERIFY_HANDLE(handle)) {
    FW_MGR_PL_DLOG_ERROR("`handle` is invalid.\n");
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x48If);
    return kPlErrInvalidParam;
  }

  FwMgrEsp32IfContext *context = (FwMgrEsp32IfContext *)handle;

  if (context->ops.close == NULL) {
    FW_MGR_PL_DLOG_ERROR("context->ops.close() is not registered.\n");
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x49If);
    return kPlErrInternal;
  }

  PlErrCode ret = context->ops.close(context->impl_handle, aborted,
                                     switch_partition, updated);
  if (ret != kPlErrCodeOk) {
    FW_MGR_PL_DLOG_ERROR("context->ops.close() failed. ret = %u\n", ret);
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x4aIf);
    return ret;
  }

  free(handle);
  s_active_context = NULL;

  return kPlErrCodeOk;
}

/// @brief
/// @param handle [in] specify I/F handle which can be obtained with the
/// FwMgrEsp32IfOpen() function.
/// @param offset [in]
/// @param buffer_handle [in]
/// @param buffer_offset [in]
/// @param write_size [in]
/// @param written_size [out] Valid only when the return value is OK.
/// @return kOk: success, otherwise: failed
PlErrCode FwMgrEsp32IfWrite(FwMgrEsp32IfHandle handle, uint32_t offset,
                            EsfMemoryManagerHandle buffer_handle,
                            uint32_t buffer_offset, uint32_t write_size,
                            uint32_t *written_size) {
  FW_MGR_PL_DLOG_DEBUG("Start.\n");

  if (!FW_MGR_PL_VERIFY_HANDLE(handle)) {
    FW_MGR_PL_DLOG_ERROR("`handle` is invalid.\n");
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x4bIf);
    return kPlErrInvalidParam;
  }

  FwMgrEsp32IfContext *context = (FwMgrEsp32IfContext *)handle;

  if (context->ops.write == NULL) {
    FW_MGR_PL_DLOG_ERROR("context->ops.write() is not registered.\n");
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x4cIf);
    return kPlErrInternal;
  }

  FW_MGR_PL_DLOG_DEBUG("offset = 0x%x\n", offset);
  return context->ops.write(context->impl_handle, offset, buffer_handle,
                            buffer_offset, write_size, written_size);
}

/// @brief
/// @param handle [in] specify I/F handle which can be obtained with the
/// FwMgrEsp32IfOpen() function.
/// @return kOk: success, otherwise: failed
PlErrCode FwMgrEsp32IfAbort(FwMgrEsp32IfHandle handle) {
  FW_MGR_PL_DLOG_DEBUG("Start.\n");

  if (!FW_MGR_PL_VERIFY_HANDLE(handle)) {
    FW_MGR_PL_DLOG_ERROR("`handle` is invalid.\n");
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x4dIf);
    return kPlErrInvalidParam;
  }

  FwMgrEsp32IfContext *context = (FwMgrEsp32IfContext *)handle;

  if (context->ops.abort == NULL) {
    FW_MGR_PL_DLOG_ERROR(
        "Aborting the binary update is not supported for the target "
        "binary type.\n");
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x4eIf);
    return kPlErrNoSupported;
  }

  return context->ops.abort(context->impl_handle);
}

PlErrCode FwMgrEsp32IfGetInfo(FwMgrPlType type, int32_t version_size,
                              char *version, int32_t hash_size, uint8_t *hash,
                              int32_t update_date_size, char *update_date) {
  if (version == NULL || hash == NULL || update_date == NULL) {
    FW_MGR_PL_DLOG_ERROR(
        "version (%p) hash (%p) or update_date (%p) is NULL.\n", version, hash,
        update_date);
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x4fIf);
    return kPlErrInvalidParam;
  }

  switch (type) {
    case kFwMgrPlTypeFirmware:
      if (version_size > 0) version[0] = '\0';
      return FwMgrEsp32FwGetHashAndUpdateDate(hash_size, hash, update_date_size,
                                              update_date);

    case kFwMgrPlTypeBootloader:
      if (hash_size > 0) memset(hash, 0, hash_size);
      if (update_date_size > 0) update_date[0] = '\0';
      return FwMgrEsp32LoaderGetVersion(version_size, version);

    default:
      break;
  }

  return kPlErrNoSupported;
}

PlErrCode FwMgrEsp32IfGetOperationSupportInfo(
    FwMgrPlOperationSupportInfo *support_info) {
  FW_MGR_PL_DLOG_DEBUG("Start.\n");

  if (support_info == NULL) {
    FW_MGR_PL_DLOG_ERROR("`support_info` is NULL.\n");
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x50If);
    return kPlErrInvalidParam;
  }

  *support_info = kSupportInfo;

  return kPlErrCodeOk;
}
