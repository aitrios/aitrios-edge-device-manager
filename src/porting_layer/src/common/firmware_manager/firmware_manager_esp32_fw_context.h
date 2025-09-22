/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESF_PORTING_LAYER_FIRMWARE_MANAGER_FIRMWARE_MANAGER_ESP32_FW_CONTEXT_H_
#define ESF_PORTING_LAYER_FIRMWARE_MANAGER_FIRMWARE_MANAGER_ESP32_FW_CONTEXT_H_

#include "esp32_flash_operations.h"

typedef struct TagFwMgrEsp32FwContext {
  Esp32FlashOpHandle flash_handle;
  // Data within the range of starting_offset <= offset < ending_offset will be
  // written to the flash memory
  uint32_t starting_offset;
  uint32_t ending_offset;
  uint32_t max_write_size;

  // Used only for T3P
  int32_t tmp_buffer_size;
  uint8_t *tmp_buffer;
} FwMgrEsp32FwContext;

#endif  // ESF_PORTING_LAYER_FIRMWARE_MANAGER_FIRMWARE_MANAGER_ESP32_FW_CONTEXT_H_
