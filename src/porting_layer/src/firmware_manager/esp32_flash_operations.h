/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESF_PORTING_LAYER_FIRMWARE_MANAGER_ESP32_FLASH_OPERATION_H_
#define ESF_PORTING_LAYER_FIRMWARE_MANAGER_ESP32_FLASH_OPERATION_H_
#include <inttypes.h>

#include "esp32_flash_operations_processor_specific.h"
#include "firmware_manager_porting_layer.h"

// The numbers of partitions of flash memory for OTA.
// ESP32 has two partitions (ota_0 and ota_1).
#define ESP32_FLASH_NUM_OTA_PARTITIONS 2
#if (ESP32_FLASH_NUM_OTA_PARTITIONS < 2)
#error "ESP32_FLASH_NUM_OTA_PARTITIONS MUST be greater than or equal to 2."
#endif
static const uint32_t kEsp32FlashNumOtaPartitions =
    ESP32_FLASH_NUM_OTA_PARTITIONS;

static const uint32_t OTA_IMG_BOOT_INVALID = OTA_IMG_BOOT_SEQ_MAX;

typedef void *Esp32FlashOpHandle;

PlErrCode Esp32FlashOpGetActivePartition(FwMgrPlOtaImgBootseq *partition);
PlErrCode Esp32FlashOpGetNextBootPartition(FwMgrPlOtaImgBootseq *partition);
PlErrCode Esp32FlashOpSetNextBootPartition(FwMgrPlOtaImgBootseq partition);

PlErrCode Esp32FlashOpGetPartitionSize(FwMgrPlOtaImgBootseq partition,
                                       uint32_t *partition_size);

PlErrCode Esp32FlashOpInvalidatePartition(FwMgrPlOtaImgBootseq partition);

PlErrCode Esp32FlashOpErasePartitionData(FwMgrPlOtaImgBootseq partition);

PlErrCode Esp32FlashOpReadPartitionData(FwMgrPlOtaImgBootseq partition,
                                        uint32_t offset, uint32_t read_size,
                                        uint8_t *buf);

PlErrCode Esp32FlashOpOpenPartition(FwMgrPlOtaImgBootseq partition,
                                    Esp32FlashOpHandle *handle);
PlErrCode Esp32FlashOpClosePartition(Esp32FlashOpHandle handle);
PlErrCode Esp32FlashOpWritePartitionData(Esp32FlashOpHandle handle,
                                         uint32_t write_size,
                                         const uint8_t *buf,
                                         uint32_t *written_size);

#endif  // ESF_PORTING_LAYER_FIRMWARE_MANAGER_ESP32_FLASH_OPERATION_H_
