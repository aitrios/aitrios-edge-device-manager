/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PL_FIRMWARE_MANAGER_CAM_IMPL_H_
#define PL_FIRMWARE_MANAGER_CAM_IMPL_H_

#include <inttypes.h>
#include <stdbool.h>

#include "firmware_manager_porting_layer.h"
#include "memory_manager.h"
#include "pl.h"

PlErrCode FwMgrPlInitializeCamImpl(void);

PlErrCode FwMgrPlFinalizeCamImpl(void);

PlErrCode FwMgrPlOpenCamImpl(FwMgrPlType type, uint32_t total_write_size,
                             const uint8_t *hash, FwMgrPlHandle *handle,
                             uint32_t *max_write_size);

PlErrCode FwMgrPlCloseCamImpl(FwMgrPlHandle handle, bool *updated);

PlErrCode FwMgrPlWriteCamImpl(FwMgrPlHandle handle,
                              EsfMemoryManagerHandle buffer_handle,
                              uint32_t buffer_offset, uint32_t write_size,
                              uint32_t *written_size);

PlErrCode FwMgrPlAbortCamImpl(FwMgrPlHandle handle);

PlErrCode FwMgrPlGetInfoCamImpl(FwMgrPlType type, int32_t version_size,
                                char *version, int32_t hash_size, uint8_t *hash,
                                int32_t update_date_size, char *update_date);

PlErrCode FwMgrPlGetOperationSupportInfoCamImpl(
    FwMgrPlOperationSupportInfo *support_info);

PlErrCode FwMgrPlGetMaxWriteSizeCamImpl(FwMgrPlType type,
                                        uint32_t *max_write_size);

PlErrCode FwMgrPlCloseWithoutPartitionSwitchCamImpl(FwMgrPlHandle handle,
                                                    bool *updated);

PlErrCode FwMgrPlSwitchFirmwarePartitionCamImpl(void);

#endif /* PL_FIRMWARE_MANAGER_CAM_IMPL_H_ */
