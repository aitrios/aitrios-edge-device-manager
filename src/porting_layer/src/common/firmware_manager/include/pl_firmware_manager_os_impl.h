/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PL_FIRMWARE_MANAGER_OS_IMPL_H_
#define PL_FIRMWARE_MANAGER_OS_IMPL_H_

#include <inttypes.h>
#include <stdbool.h>

#include "firmware_manager_porting_layer.h"
#include "memory_manager.h"
#include "pl.h"

PlErrCode FwMgrPlInitializeOsImpl(void);

PlErrCode FwMgrPlFinalizeOsImpl(void);

PlErrCode FwMgrPlOpenOsImpl(FwMgrPlType type, uint32_t total_write_size,
                            const uint8_t *hash, FwMgrPlHandle *handle,
                            uint32_t *max_write_size);

PlErrCode FwMgrPlCloseOsImpl(FwMgrPlHandle handle, bool *updated);

PlErrCode FwMgrPlWriteOsImpl(FwMgrPlHandle handle,
                             EsfMemoryManagerHandle buffer_handle,
                             uint32_t buffer_offset, uint32_t write_size,
                             uint32_t *written_size);

PlErrCode FwMgrPlAbortOsImpl(FwMgrPlHandle handle);

PlErrCode FwMgrPlGetInfoOsImpl(FwMgrPlType type, int32_t version_size,
                               char *version, int32_t hash_size, uint8_t *hash,
                               int32_t update_date_size, char *update_date);

PlErrCode FwMgrPlGetOperationSupportInfoOsImpl(
    FwMgrPlOperationSupportInfo *support_info);

PlErrCode FwMgrPlGetMaxWriteSizeOsImpl(FwMgrPlType type,
                                       uint32_t *max_write_size);

PlErrCode FwMgrPlCloseWithoutPartitionSwitchOsImpl(FwMgrPlHandle handle,
                                                   bool *updated);

PlErrCode FwMgrPlSwitchFirmwarePartitionOsImpl(void);

#endif /* PL_FIRMWARE_MANAGER_OS_IMPL_H_ */
