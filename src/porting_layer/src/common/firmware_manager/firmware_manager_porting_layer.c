/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "firmware_manager_porting_layer.h"

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>

#include "firmware_manager_esp32_if.h"
#include "firmware_manager_esp32_switch_firmware_partition.h"
#include "firmware_manager_porting_layer_log.h"
#include "verify_handle.h"

// The prefix FwMgrPl stands for Firmware Manager Porting Layer

// Internal enumerated type and structures -------------------------------------

// States of the Firmware Manager Porting Layer
typedef enum TagFwMgrPlState {
  kFwMgrPlNotInitialized,
  kFwMgrPlClosed,
  kFwMgrPlOpen,
  kFwMgrPlAborted,
} FwMgrPlState;

// Structure of the internal state of the Firmware Manager Porting Layer
typedef struct TagFwMgrPlContext {
  uint32_t remaining_write_size;
  uint32_t offset;
  FwMgrEsp32IfHandle if_handle;
} FwMgrPlContext;

// Global variables ------------------------------------------------------------

// `s_fw_mgr_pl_mutex` is used to avoid executing functions at the same time.
// Each function locks `s_fw_mgr_pl_mutex` at the beginning of the function, and
// unlocks it at the end.
static pthread_mutex_t s_fw_mgr_pl_mutex = PTHREAD_MUTEX_INITIALIZER;

// Current state of the Firmware Manager Porting Layer
static FwMgrPlState s_fw_mgr_pl_state = kFwMgrPlNotInitialized;

// Currently used context
static FwMgrPlContext *s_active_context = NULL;

// Public Functions -----------------------------------------------------------

/// @brief Initialize the Firmware Manager Porting Layer. This function must be
/// called before the other functions of the Firmware Manager Porting Layer are
/// called. (In usual cases, this function is called at the start-up of the
/// system.)
/// This function can be called in the `NotInitialized` state.
/// When this function successfully returns, the state transitions to `Closed`.
/// @return kPlErrCodeOk: success, otherwise: failed
PlErrCode FwMgrPlInitialize(void) {
  FW_MGR_PL_DLOG_DEBUG("Start.\n");

  if (pthread_mutex_trylock(&s_fw_mgr_pl_mutex) != 0) {
    FW_MGR_PL_DLOG_ERROR("Failed to lock the mutex. errno = %d\n", errno);
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x00Core);
    return kPlErrLock;
  }

  PlErrCode ret = kPlErrCodeError;

  if (s_fw_mgr_pl_state != kFwMgrPlNotInitialized) {
    FW_MGR_PL_DLOG_ERROR("State must be 'NotInitialized', but it is %u.\n",
                         s_fw_mgr_pl_state);
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x01Core);
    ret = kPlErrInvalidState;
    goto unlock_mutex_then_exit;
  }

  ret = FwMgrEsp32IfInitialize();
  if (ret != kPlErrCodeOk) {
    FW_MGR_PL_DLOG_ERROR("FwMgrEsp32IfInitialize() failed. ret = %u\n", ret);
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x02Core);
    goto unlock_mutex_then_exit;
  }

  s_fw_mgr_pl_state = kFwMgrPlClosed;

unlock_mutex_then_exit:
  pthread_mutex_unlock(&s_fw_mgr_pl_mutex);

  return ret;
}

/// @brief Finalize the Firmware Manager Porting Layer. If FwMgrPlInitialize is
/// called, this function must be called before shutting down the system. (In
/// usual cases, this function is called just before shutting down the system.)
/// This function can be called in `Closed`, `Open`, and `Aborted` states.
/// When this function successfully returns, the state transitions to
/// `NotInitialized`.
/// @return kPlErrCodeOk: success, otherwise, failed.
PlErrCode FwMgrPlFinalize(void) {
  FW_MGR_PL_DLOG_DEBUG("Start.\n");

  if (pthread_mutex_trylock(&s_fw_mgr_pl_mutex) != 0) {
    FW_MGR_PL_DLOG_ERROR("Failed to lock the mutex. errno = %d\n", errno);
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x03Core);
    return kPlErrLock;
  }

  PlErrCode ret = kPlErrCodeError;

  if (s_fw_mgr_pl_state == kFwMgrPlNotInitialized) {
    FW_MGR_PL_DLOG_ERROR("State must not be 'NotInitialized', but it is.\n");
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x04Core);
    ret = kPlErrInvalidState;
    goto unlock_mutex_then_exit;
  }

  if (s_fw_mgr_pl_state != kFwMgrPlClosed) {
    // In usual use case, FwMgrPlClose() should be called before
    // FwMgrPlFinalize() is called.
    FW_MGR_PL_DLOG_WARNING("FwMgrPlClose() has not been called. state = %u\n",
                           s_fw_mgr_pl_state);
    free(s_active_context);
    s_active_context = NULL;
  }

  ret = FwMgrEsp32IfFinalize();
  if (ret != kPlErrCodeOk) {
    FW_MGR_PL_DLOG_ERROR("FwMgrEsp32IfFinalize() failed. ret = %u\n", ret);
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x05Core);
    goto unlock_mutex_then_exit;
  }

  s_fw_mgr_pl_state = kFwMgrPlNotInitialized;

unlock_mutex_then_exit:
  pthread_mutex_unlock(&s_fw_mgr_pl_mutex);

  return ret;
}

/// @brief Start updating the AP binary. Calling this function deletes data in
/// the partition of the flash memory where the new AP binary will be written.
/// This function can be called in the `Closed` state. When this function
/// successfully returns, the state transitions to the `Open` state.
/// @param type [in] Type of AP binary (firmware, boot loader, or partition
/// table) to be updated.
/// @param total_write_size [in] Size (in bytes) of the AP binary. Must be (a)
/// non-zero, (b) less than or equal to the size of the partition of the flash
/// memory, and (c) (when `type == kFwMgrPlTypeFirmware`) a multiple of 32.
/// @param handle [out] A handle for updating the AP binary. Use this handle
/// when calling `FwMgrPlWrite`, `FwMgrPlClose`, and `FwMgrPlAbort`. Must not be
/// NULL.
/// @param max_write_size [out] The maximum value of `write_size` that can be
/// specified when calling FwMgrPlWrite. Must not be NULL.
/// @return kPlErrCodeOk: success, otherwise, failed.
PlErrCode FwMgrPlOpen(FwMgrPlType type, uint32_t total_write_size,
                      const uint8_t *hash, FwMgrPlHandle *handle,
                      uint32_t *max_write_size) {
  FW_MGR_PL_DLOG_DEBUG("Start.\n");

  if (pthread_mutex_trylock(&s_fw_mgr_pl_mutex) != 0) {
    FW_MGR_PL_DLOG_ERROR("Failed to lock the mutex.\n");
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x06Core);
    return kPlErrLock;
  }

  PlErrCode ret = kPlErrCodeError;

  if (s_fw_mgr_pl_state != kFwMgrPlClosed) {
    FW_MGR_PL_DLOG_ERROR("State must be 'Closed', but it is %u.\n",
                         s_fw_mgr_pl_state);
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x07Core);
    ret = kPlErrInvalidState;
    goto unlock_mutex_then_exit;
  }

  if (handle == NULL) {
    FW_MGR_PL_DLOG_ERROR("`handle` is NULL.\n");
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x08Core);
    ret = kPlErrInvalidParam;
    goto unlock_mutex_then_exit;
  }
  *handle = NULL;

  if (s_active_context != NULL) {
    // This block will not be executed.
    // But we ensure that `s_active_context` is NOT overwritten because it could
    // cause a memory leak, which should be avoided.
    FW_MGR_PL_DLOG_ERROR("There is an active context.\n");
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x09Core);
    ret = kPlErrInternal;
    goto unlock_mutex_then_exit;
  }

  FwMgrPlContext *context = (FwMgrPlContext *)malloc(sizeof(FwMgrPlContext));
  if (context == NULL) {
    FW_MGR_PL_DLOG_CRITICAL("Failed to allocate memory for context.\n");
    FW_MGR_PL_ELOG_CRITICAL(kFwMgrPlElogCriticalId0x00CoreAllocContext);
    ret = kPlErrMemory;
    goto unlock_mutex_then_exit;
  }

  ret = FwMgrEsp32IfOpen(type, total_write_size, hash, &context->if_handle,
                         max_write_size);
  if (ret != kPlErrCodeOk) {
    FW_MGR_PL_DLOG_ERROR("FwMgrEsp32IfOpen(type:%u) failed. ret = %u\n", type,
                         ret);
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x0aCore);
    free(context);
    goto unlock_mutex_then_exit;
  }

  context->remaining_write_size = total_write_size;
  context->offset = 0;

  s_active_context = context;
  *handle = (FwMgrPlHandle)context;

  s_fw_mgr_pl_state = kFwMgrPlOpen;

unlock_mutex_then_exit:
  pthread_mutex_unlock(&s_fw_mgr_pl_mutex);

  return ret;
}

/// @brief Finish updating the AP binary. For all handles obtained by calling
/// FwMgrPlOpen, this function must be called. This function can be called in
/// the `Open` and `Aborted` states.
/// [A] When called in the `Open` state:
///     (1) Verify the AP binary written in the flash memory.
///     (2) Ensure that the sum of the written size matches the
///         `total_write_size` specified when calling `FwMgrPlOpen`.
///     (3) [if switch_partition is true] Set the new AP binary as valid so that
///         it will be used in the next boot.
///     (4) Perform other closing processes.
/// [B] When called in the `Aborted` state: Perform the step [A]-(4).
/// When this function successfully returns, the state transitions to the
/// `Closed` state.
/// @param handle [in] A handle that has been obtained by calling
/// `FwMgrPlOpen`.
/// @param updated [out] Indicates whether or not the AP binary has been updated
/// by this function. This argument will be `true` only when this function is
/// called in the `Closed` state and successfully returns. Can be NULL.
/// @return kPlErrCodeOk on success, otherwise an error code.
static PlErrCode FwMgrPlCloseInternal(FwMgrPlHandle handle,
                                      bool switch_partition, bool *updated) {
  FW_MGR_PL_DLOG_DEBUG("Start. (switch_partition = %d)\n", switch_partition);

  if (updated) *updated = false;

  if (pthread_mutex_trylock(&s_fw_mgr_pl_mutex) != 0) {
    FW_MGR_PL_DLOG_ERROR("Failed to lock the mutex. errno = %d\n", errno);
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x0bCore);
    return kPlErrLock;
  }

  PlErrCode ret = kPlErrCodeError;

  if ((s_fw_mgr_pl_state != kFwMgrPlOpen) &&
      (s_fw_mgr_pl_state != kFwMgrPlAborted)) {
    FW_MGR_PL_DLOG_ERROR("State must be 'Open' or 'Aborted', but it is %u.\n",
                         s_fw_mgr_pl_state);
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x0cCore);
    ret = kPlErrInvalidState;
    goto unlock_mutex_then_exit;
  }

  if (!FW_MGR_PL_VERIFY_HANDLE(handle)) {
    FW_MGR_PL_DLOG_ERROR("`handle` is invalid.\n");
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x0dCore);
    ret = kPlErrInvalidParam;
    goto unlock_mutex_then_exit;
  }

  FwMgrPlContext *context = (FwMgrPlContext *)handle;

  if (s_fw_mgr_pl_state == kFwMgrPlOpen) {
    if (context->remaining_write_size > 0) {
      // Sum of written size is less than `total_write_size` specified when
      // calling FwMgrPlOpen()
      FW_MGR_PL_DLOG_ERROR(
          "Sum of written size is less than `total_write_size` specified when "
          "calling FwMgrPlOpen()\n");
      FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x0eCore);
      ret = kPlErrInvalidOperation;
      goto unlock_mutex_then_exit;
    }
  }

  ret = FwMgrEsp32IfClose(context->if_handle,
                          s_fw_mgr_pl_state == kFwMgrPlAborted,
                          switch_partition, updated);
  if (ret != kPlErrCodeOk) {
    FW_MGR_PL_DLOG_ERROR("FwMgrEsp32IfClose failed. state =%u, ret = %u\n",
                         s_fw_mgr_pl_state, ret);
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x0fCore);
    goto unlock_mutex_then_exit;
  }

  free(context);
  s_active_context = NULL;

  s_fw_mgr_pl_state = kFwMgrPlClosed;

unlock_mutex_then_exit:
  pthread_mutex_unlock(&s_fw_mgr_pl_mutex);

  return ret;
}

PlErrCode FwMgrPlClose(FwMgrPlHandle handle, bool *updated) {
  return FwMgrPlCloseInternal(handle, true, updated);
}

PlErrCode FwMgrPlCloseWithoutPartitionSwitch(FwMgrPlHandle handle,
                                             bool *updated) {
  return FwMgrPlCloseInternal(handle, false, updated);
}

/// @brief Write the AP binary to the device. This function can be called
/// in the `Open` state. This function does not change the state. The data from
/// buffer[buffer_offset] to buffer[buffer_offset+write_size] will be written to
/// the device. (The buffer is specified by the `buffer_handle`.)
/// @param handle [in] A handle that has been obtained by calling
/// `FwMgrPlOpen`.
/// @param buffer_handle [in] The handle of memory manager to access the buffer
/// where the AP binary is stored.
/// @param buffer_offset [in] The offset of the starting point of AP binary in
/// the buffer.
/// @param write_size [in] The size of AP binary (in bytes). Must be larger than
/// zero and less than or equal to the `max_write_size` which has been obtained
/// when calling `FwMgrPlOpen`.
/// @param written_size [out] Size of data this function successfully writes to
/// the flash memory. `written_size` can be less than `write_size` (including
/// zero) even if this function successfully returns.
/// @return kPlErrCodeOk on success, otherwise an error code.
PlErrCode FwMgrPlWrite(FwMgrPlHandle handle,
                       EsfMemoryManagerHandle buffer_handle,
                       uint32_t buffer_offset, uint32_t write_size,
                       uint32_t *written_size) {
  FW_MGR_PL_DLOG_DEBUG("Start.\n");

  if (pthread_mutex_trylock(&s_fw_mgr_pl_mutex) != 0) {
    FW_MGR_PL_DLOG_ERROR("Failed to lock the mutex. errno = %d\n", errno);
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x10Core);
    return kPlErrLock;
  }

  PlErrCode ret = kPlErrCodeError;

  if (s_fw_mgr_pl_state != kFwMgrPlOpen) {
    FW_MGR_PL_DLOG_ERROR("State must be 'Open', but it is %u.\n",
                         s_fw_mgr_pl_state);
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x11Core);
    ret = kPlErrInvalidState;
    goto unlock_mutex_then_exit;
  }

  if (!FW_MGR_PL_VERIFY_HANDLE(handle)) {
    FW_MGR_PL_DLOG_ERROR("`handle` is invalid.\n");
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x12Core);
    ret = kPlErrInvalidParam;
    goto unlock_mutex_then_exit;
  }

  FwMgrPlContext *context = (FwMgrPlContext *)handle;

  if (write_size > context->remaining_write_size) {
    // Sum of written size would exceed the `total_write_size` specified when
    // calling FwMgrPlOpen().
    FW_MGR_PL_DLOG_CRITICAL(
        "Sum of written size would exceed the `total_write_size` specified "
        "when calling FwMgrPlOpen()."
        "write_size(%u) > remaining_write_size(%u)\n",
        write_size, context->remaining_write_size);
    FW_MGR_PL_ELOG_CRITICAL(kFwMgrPlElogCriticalId0x01CoreExceedsTotalSize);
    ret = kPlErrInvalidOperation;
    goto unlock_mutex_then_exit;
  }

  FW_MGR_PL_DLOG_DEBUG("offset = 0x%x\n", context->offset);
  ret = FwMgrEsp32IfWrite(context->if_handle, context->offset, buffer_handle,
                          buffer_offset, write_size, written_size);
  if (ret != kPlErrCodeOk) {
    FW_MGR_PL_DLOG_ERROR("FwMgrEsp32IfWrite() failed. ret = %u\n", ret);
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x13Core);
    goto unlock_mutex_then_exit;
  }

  context->offset += *written_size;
  context->remaining_write_size -= *written_size;

unlock_mutex_then_exit:
  pthread_mutex_unlock(&s_fw_mgr_pl_mutex);

  return ret;
}

/// @brief Abort updating the AP binary. This function can be called in the
/// `Open` state. When this function successfully returns, the state transitions
/// to the `Aborted` state.
/// @param handle [in] A handle that has been obtained by calling
/// `FwMgrPlOpen`.
/// @return kPlErrCodeOk on success, otherwise an error code.
PlErrCode FwMgrPlAbort(FwMgrPlHandle handle) {
  FW_MGR_PL_DLOG_DEBUG("Start.\n");

  if (pthread_mutex_trylock(&s_fw_mgr_pl_mutex) != 0) {
    FW_MGR_PL_DLOG_ERROR("Failed to lock the mutex. errno = %d\n", errno);
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x14Core);
    return kPlErrLock;
  }

  PlErrCode ret = kPlErrCodeError;

  if (s_fw_mgr_pl_state == kFwMgrPlAborted) {
    FW_MGR_PL_DLOG_INFO("State is already 'Aborted'. Do nothing\n");
    ret = kPlErrCodeOk;
    goto unlock_mutex_then_exit;
  }

  if (s_fw_mgr_pl_state != kFwMgrPlOpen) {
    FW_MGR_PL_DLOG_ERROR("State must be 'Open', but it is %u.\n",
                         s_fw_mgr_pl_state);
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x15Core);
    ret = kPlErrInvalidState;
    goto unlock_mutex_then_exit;
  }

  if (!FW_MGR_PL_VERIFY_HANDLE(handle)) {
    FW_MGR_PL_DLOG_ERROR("`handle` is invalid.\n");
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x16Core);
    ret = kPlErrInvalidParam;
    goto unlock_mutex_then_exit;
  }

  FwMgrPlContext *context = (FwMgrPlContext *)handle;
  ret = FwMgrEsp32IfAbort(context->if_handle);
  if (ret != kPlErrCodeOk) {
    FW_MGR_PL_DLOG_ERROR("FwMgrEsp32IfAbort() failed. ret = %u\n", ret);
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x17Core);
    goto unlock_mutex_then_exit;
  }

  s_fw_mgr_pl_state = kFwMgrPlAborted;

unlock_mutex_then_exit:
  pthread_mutex_unlock(&s_fw_mgr_pl_mutex);

  return ret;
}

/// @brief Get version, hash and update date of the binary
/// @param type [in]
/// @param version_size [in] Size of version
/// @param version [out] Can be NULL
/// @param hash_size [in] Size of hash
/// @param hash [out] Can be NULL
/// @param update_date_size [in] Size of update_date
/// @param update_date [out] Can be NULL
/// @return
PlErrCode FwMgrPlGetInfo(FwMgrPlType type, int32_t version_size, char *version,
                         int32_t hash_size, uint8_t *hash,
                         int32_t update_date_size, char *update_date) {
  FW_MGR_PL_DLOG_DEBUG("Start.\n");

  if (pthread_mutex_lock(&s_fw_mgr_pl_mutex) != 0) {
    FW_MGR_PL_DLOG_ERROR("Failed to lock the mutex. errno = %d\n", errno);
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x1aCore);
    return kPlErrLock;
  }

  PlErrCode ret = kPlErrInternal;

  if (s_fw_mgr_pl_state == kFwMgrPlNotInitialized) {
    FW_MGR_PL_DLOG_ERROR("State must not be 'NotInitialized', but it is %u.\n",
                         s_fw_mgr_pl_state);
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x1bCore);
    ret = kPlErrInvalidState;
    goto unlock_mutex_then_exit;
  }

  ret = FwMgrEsp32IfGetInfo(type, version_size, version, hash_size, hash,
                            update_date_size, update_date);
  if (ret != kPlErrCodeOk) {
    FW_MGR_PL_DLOG_ERROR("FwMgrEsp32IfGetInfo failed. ret = %u.\n", ret);
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x1cCore);
    goto unlock_mutex_then_exit;
  }

  ret = kPlErrCodeOk;

unlock_mutex_then_exit:
  pthread_mutex_unlock(&s_fw_mgr_pl_mutex);

  return ret;
}

/// @brief Get information on supported operations.
/// @param support_info [out] Must not be NULL.
/// @return kPlErrCodeOk on success, otherwise an error code.
PlErrCode FwMgrPlGetOperationSupportInfo(
    FwMgrPlOperationSupportInfo *support_info) {
  FW_MGR_PL_DLOG_DEBUG("Start.\n");

  return FwMgrEsp32IfGetOperationSupportInfo(support_info);
}

PlErrCode FwMgrPlGetMaxWriteSize(FwMgrPlType type, uint32_t *max_write_size) {
  (void)type;
  if (max_write_size != NULL) *max_write_size = UINT32_MAX;
  return kPlErrCodeOk;
}

/// @brief Switch the partition of firmware (ota_0/ota_1)
PlErrCode FwMgrPlSwitchFirmwarePartition(void) {
  return FwMgrEsp32SwitchFirmwarePartition();
}
