/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESF_FIRMWARE_MANAGER_INCLUDE_FIRMWARE_MANGER_PROCESSOR_BINARY_HEADER_OPS_H_
#define ESF_FIRMWARE_MANAGER_INCLUDE_FIRMWARE_MANGER_PROCESSOR_BINARY_HEADER_OPS_H_

#include <stdbool.h>
#include <stdint.h>

#include "firmware_manager.h"
#include "firmware_manager_processor_binary_header.h"
#include "memory_manager.h"

// Memo: +1 of the size of device_type and device_variant is for a null charater
// at the end.
typedef struct EsfFwMgrProcessorBinaryHeaderInfo {
  uint32_t header_size;  // 0 means header is not appended.
  EsfFwMgrSwArchVersion sw_arch_version;
  char device_type[ESF_FW_MGR_PROCESSOR_BINARY_HEADER_DEVICE_TYPE_SIZE + 1];
  char device_variant[ESF_FW_MGR_PROCESSOR_BINARY_HEADER_DEVICE_VARIANT_SIZE +
                      1];
} EsfFwMgrProcessorBinaryHeaderInfo;

typedef struct EsfFwMgrProcessorLoadBinaryHeaderRequest {
  EsfMemoryManagerHandle src_buffer_handle;
  int32_t src_size;
  int32_t src_offset;
} EsfFwMgrProcessorLoadBinaryHeaderRequest;

typedef struct EsfFwMgrProcessorBinaryHeaderLoadInfo {
  bool magic_loaded;
  bool completed;
  int32_t total_loaded_size;
  void *buffer;
} EsfFwMgrProcessorBinaryHeaderLoadInfo;

void EsfFwMgrProcessorInitBinaryHeaderLoadInfo(
    EsfFwMgrProcessorBinaryHeaderLoadInfo *info);
void EsfFwMgrProcessorDeinitBinaryHeaderLoadInfo(
    EsfFwMgrProcessorBinaryHeaderLoadInfo *info);

bool EsfFwMgrProcessorIsBinaryHeaderMagicLoaded(
    const EsfFwMgrProcessorBinaryHeaderLoadInfo *load_info);
bool EsfFwMgrProcessorIsBinaryHeaderLoaded(
    const EsfFwMgrProcessorBinaryHeaderLoadInfo *load_info);

EsfFwMgrResult EsfFwMgrProcessorLoadBinaryHeader(
    const EsfFwMgrProcessorLoadBinaryHeaderRequest *request,
    EsfFwMgrProcessorBinaryHeaderLoadInfo *load_info,
    EsfFwMgrProcessorBinaryHeaderInfo *info);

#endif  // ESF_FIRMWARE_MANAGER_INCLUDE_FIRMWARE_MANGER_PROCESSOR_BINARY_HEADER_OPS_H_
