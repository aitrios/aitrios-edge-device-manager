/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pl.h"

#include <inttypes.h>
#include <stdbool.h>

#include "memory_manager.h"
#include "pl_firmware_manager_cam_impl.h"

PlErrCode FwMgrPlInitializeOsImpl(void) { return FwMgrPlInitializeCamImpl(); }

PlErrCode FwMgrPlFinalizeOsImpl(void) { return FwMgrPlFinalizeCamImpl(); }

PlErrCode FwMgrPlOpenOsImpl(FwMgrPlType type, uint32_t total_write_size,
                            const uint8_t *hash, FwMgrPlHandle *handle,
                            uint32_t *max_write_size) {
  return FwMgrPlOpenCamImpl(type, total_write_size, hash, handle,
                            max_write_size);
}

PlErrCode FwMgrPlCloseOsImpl(FwMgrPlHandle handle, bool *updated) {
  return FwMgrPlCloseCamImpl(handle, updated);
}

PlErrCode FwMgrPlWriteOsImpl(FwMgrPlHandle handle,
                             EsfMemoryManagerHandle buffer_handle,
                             uint32_t buffer_offset, uint32_t write_size,
                             uint32_t *written_size) {
  return FwMgrPlWriteCamImpl(handle, buffer_handle, buffer_offset, write_size,
                             written_size);
}

PlErrCode FwMgrPlAbortOsImpl(FwMgrPlHandle handle) {
  return FwMgrPlAbortCamImpl(handle);
}

PlErrCode FwMgrPlGetInfoOsImpl(FwMgrPlType type, int32_t version_size,
                               char *version, int32_t hash_size, uint8_t *hash,
                               int32_t update_date_size, char *update_date) {
  return FwMgrPlGetInfoCamImpl(type, version_size, version, hash_size, hash,
                               update_date_size, update_date);
}

PlErrCode FwMgrPlGetOperationSupportInfoOsImpl(
    FwMgrPlOperationSupportInfo *support_info) {
  return FwMgrPlGetOperationSupportInfoCamImpl(support_info);
}

PlErrCode FwMgrPlGetMaxWriteSizeOsImpl(FwMgrPlType type,
                                       uint32_t *max_write_size) {
  return FwMgrPlGetMaxWriteSizeCamImpl(type, max_write_size);
}

PlErrCode FwMgrPlCloseWithoutPartitionSwitchOsImpl(FwMgrPlHandle handle,
                                                   bool *updated) {
  return FwMgrPlCloseWithoutPartitionSwitchCamImpl(handle, updated);
}

PlErrCode FwMgrPlSwitchFirmwarePartitionOsImpl(void) {
  return FwMgrPlSwitchFirmwarePartitionCamImpl();
}
