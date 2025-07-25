/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef ESF_PORTING_LAYER_FIRMWARE_MANAGER_FIRMWARE_MANAGER_PORTING_LAYER_H_
#define ESF_PORTING_LAYER_FIRMWARE_MANAGER_FIRMWARE_MANAGER_PORTING_LAYER_H_
#include <inttypes.h>
#include <stdbool.h>

#include "memory_manager.h"
#include "pl.h"

// The prefix FwMgrPl stands for Firmware Manager Porting Layer

// Handle for HAL APBinary API
typedef void *FwMgrPlHandle;
#define FW_MGR_PL_INVALID_HANDLE ((FwMgrPlHandle)NULL)

// AP binary type
typedef enum TagFwMgrPlType {
  kFwMgrPlTypeFirmware,
  kFwMgrPlTypeBootloader,
  kFwMgrPlTypePartitionTable,
} FwMgrPlType;

typedef struct TagFwMgrPlSupportInfo {
  bool update_supported;
  bool update_abort_supported;
  bool rollback_supported;
  bool factory_reset_supported;
} FwMgrPlSupportInfo;

typedef struct TagFwMgrPlOperationSupportInfo {
  FwMgrPlSupportInfo firmware;
  FwMgrPlSupportInfo bootloader;
  FwMgrPlSupportInfo partition_table;
} FwMgrPlOperationSupportInfo;

PlErrCode FwMgrPlInitialize(void);
PlErrCode FwMgrPlFinalize(void);

PlErrCode FwMgrPlOpen(FwMgrPlType type, uint32_t total_write_size,
                      const uint8_t *hash, FwMgrPlHandle *handle,
                      uint32_t *max_write_size);
PlErrCode FwMgrPlClose(FwMgrPlHandle handle, bool *updated);
PlErrCode FwMgrPlWrite(FwMgrPlHandle handle,
                       EsfMemoryManagerHandle buffer_handle,
                       uint32_t buffer_offset, uint32_t write_size,
                       uint32_t *written_size);
PlErrCode FwMgrPlAbort(FwMgrPlHandle handle);

PlErrCode FwMgrPlGetInfo(FwMgrPlType type, int32_t version_size, char *version,
                         int32_t hash_size, uint8_t *hash,
                         int32_t update_date_size, char *update_date);

PlErrCode FwMgrPlGetOperationSupportInfo(
    FwMgrPlOperationSupportInfo *support_info);

PlErrCode FwMgrPlGetMaxWriteSize(FwMgrPlType type, uint32_t *max_write_size);
PlErrCode FwMgrPlCloseWithoutPartitionSwitch(FwMgrPlHandle handle,
                                             bool *updated);
PlErrCode FwMgrPlSwitchFirmwarePartition(void);

#endif  // ESF_PORTING_LAYER_FIRMWARE_MANAGER_FIRMWARE_MANAGER_PORTING_LAYER_H_
