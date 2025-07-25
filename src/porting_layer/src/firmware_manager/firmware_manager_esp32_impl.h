/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESF_PORTING_LAYER_FIRMWARE_MANAGER_FIRMWARE_MANAGER_ESP32_IMPL_H_
#define ESF_PORTING_LAYER_FIRMWARE_MANAGER_FIRMWARE_MANAGER_ESP32_IMPL_H_
#include "firmware_manager_porting_layer.h"

typedef void *FwMgrEsp32ImplHandle;

typedef struct TagFwMgrEsp32ImplOps {
  PlErrCode (*finalize)(void);
  PlErrCode (*open)(uint32_t total_write_size, const uint8_t *hash,
                    FwMgrEsp32ImplHandle *handle, uint32_t *max_write_size);
  PlErrCode (*close)(FwMgrEsp32ImplHandle handle, bool aborted,
                     bool switch_partition, bool *updated);
  PlErrCode (*write)(FwMgrEsp32ImplHandle handle, uint32_t offset,
                     EsfMemoryManagerHandle buffer_handle,
                     uint32_t buffer_offset, uint32_t write_size,
                     uint32_t *written_size);
  PlErrCode (*abort)(FwMgrEsp32ImplHandle handle);
} FwMgrEsp32ImplOps;

#endif  // ESF_PORTING_LAYER_FIRMWARE_MANAGER_FIRMWARE_MANAGER_ESP32_IMPL_H_
