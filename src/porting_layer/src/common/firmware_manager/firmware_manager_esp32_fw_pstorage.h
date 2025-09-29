/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESF_PORTING_LAYER_FIRMWARE_MANAGER_FIRMWARE_MANAGER_ESP32_FW_PSTORAGE_H_
#define ESF_PORTING_LAYER_FIRMWARE_MANAGER_FIRMWARE_MANAGER_ESP32_FW_PSTORAGE_H_

#include "esp32_flash_operations.h"
#include "parameter_storage_manager.h"
#include "parameter_storage_manager_common.h"

#define FW_MGR_PL_HASH_SIZE 32
#define FW_MGR_PL_UPDATE_DATE_SIZE (32 + 1)

typedef struct HashDate {
  uint8_t hash[FW_MGR_PL_HASH_SIZE];
  char update_date[FW_MGR_PL_UPDATE_DATE_SIZE];
} HashDate;

typedef struct FirmwareInfoData {
  uint32_t size;
  HashDate info[ESP32_FLASH_NUM_OTA_PARTITIONS];
} FirmwareInfoData;

typedef struct FirmwareInfo {
  FirmwareInfoData data;
} FirmwareInfo;

typedef struct FirmwareInfoMask {
  uint8_t firmware_info : 1;
} FirmwareInfoMask;

void FwMgrEsp32FwGetStructInfoForFirmwareInfo(
    EsfParameterStorageManagerMemberInfo *member_info,
    EsfParameterStorageManagerStructInfo *struct_info);
PlErrCode FwMgrEsp32FwSaveHashAndDate(FwMgrPlOtaImgBootseq target_partition,
                                      const uint8_t *hash,
                                      const char *update_date);

#endif  // ESF_PORTING_LAYER_FIRMWARE_MANAGER_FIRMWARE_MANAGER_ESP32_FW_PSTORAGE_H_  // NOLINT
