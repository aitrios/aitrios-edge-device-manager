/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <inttypes.h>
#include <stdbool.h>

#include "memory_manager.h"
#include "pl.h"
#include "pl_firmware_manager_os_impl.h"

PlErrCode FwMgrPlInitialize(void) { return FwMgrPlInitializeOsImpl(); }

PlErrCode FwMgrPlFinalize(void) { return FwMgrPlFinalizeOsImpl(); }

PlErrCode FwMgrPlOpen(FwMgrPlType type, uint32_t total_write_size,
                      const uint8_t *hash, FwMgrPlHandle *handle,
                      uint32_t *max_write_size) {
  return FwMgrPlOpenOsImpl(type, total_write_size, hash, handle,
                           max_write_size);
}

PlErrCode FwMgrPlClose(FwMgrPlHandle handle, bool *updated) {
  return FwMgrPlCloseOsImpl(handle, updated);
}

PlErrCode FwMgrPlWrite(FwMgrPlHandle handle,
                       EsfMemoryManagerHandle buffer_handle,
                       uint32_t buffer_offset, uint32_t write_size,
                       uint32_t *written_size) {
  return FwMgrPlWriteOsImpl(handle, buffer_handle, buffer_offset, write_size,
                            written_size);
}

PlErrCode FwMgrPlAbort(FwMgrPlHandle handle) {
  return FwMgrPlAbortOsImpl(handle);
}

PlErrCode FwMgrPlGetInfo(FwMgrPlType type, int32_t version_size, char *version,
                         int32_t hash_size, uint8_t *hash,
                         int32_t update_date_size, char *update_date) {
  return FwMgrPlGetInfoOsImpl(type, version_size, version, hash_size, hash,
                              update_date_size, update_date);
}

PlErrCode FwMgrPlGetOperationSupportInfo(
    FwMgrPlOperationSupportInfo *support_info) {
  return FwMgrPlGetOperationSupportInfoOsImpl(support_info);
}

PlErrCode FwMgrPlGetMaxWriteSize(FwMgrPlType type, uint32_t *max_write_size) {
  return FwMgrPlGetMaxWriteSizeOsImpl(type, max_write_size);
}

PlErrCode FwMgrPlCloseWithoutPartitionSwitch(FwMgrPlHandle handle,
                                             bool *updated) {
  return FwMgrPlCloseWithoutPartitionSwitchOsImpl(handle, updated);
}

PlErrCode FwMgrPlSwitchFirmwarePartition(void) {
  return FwMgrPlSwitchFirmwarePartitionOsImpl();
}
