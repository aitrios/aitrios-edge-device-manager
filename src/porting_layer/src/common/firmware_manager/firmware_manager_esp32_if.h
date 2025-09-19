/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESF_PORTING_LAYER_FIRMWARE_MANAGER_FIRMWARE_MANAGER_ESP32_IF_H_
#define ESF_PORTING_LAYER_FIRMWARE_MANAGER_FIRMWARE_MANAGER_ESP32_IF_H_

#include "firmware_manager_porting_layer.h"

typedef void *FwMgrEsp32IfHandle;

PlErrCode FwMgrEsp32IfInitialize(void);
PlErrCode FwMgrEsp32IfFinalize(void);

PlErrCode FwMgrEsp32IfOpen(FwMgrPlType type, uint32_t total_write_size,
                           const uint8_t *hash, FwMgrEsp32IfHandle *handle,
                           uint32_t *max_write_size);
PlErrCode FwMgrEsp32IfClose(FwMgrEsp32IfHandle handle, bool aborted,
                            bool switch_partition, bool *updated);
PlErrCode FwMgrEsp32IfWrite(FwMgrEsp32IfHandle handle, uint32_t offset,
                            EsfMemoryManagerHandle buffer_handle,
                            uint32_t buffer_offset, uint32_t write_size,
                            uint32_t *written_size);
PlErrCode FwMgrEsp32IfAbort(FwMgrEsp32IfHandle handle);

PlErrCode FwMgrEsp32IfGetInfo(FwMgrPlType type, int32_t version_size,
                              char *version, int32_t hash_size, uint8_t *hash,
                              int32_t update_date_size, char *update_date);

PlErrCode FwMgrEsp32IfGetOperationSupportInfo(
    FwMgrPlOperationSupportInfo *support_info);

#endif  // ESF_PORTING_LAYER_FIRMWARE_MANAGER_FIRMWARE_MANAGER_ESP32_IF_H_
