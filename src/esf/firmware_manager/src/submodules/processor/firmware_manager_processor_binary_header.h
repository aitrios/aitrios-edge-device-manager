/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESF_FIRMWARE_MANAGER_INCLUDE_FIRMWARE_MANGER_PROCESSOR_BINARY_HEADER_H_
#define ESF_FIRMWARE_MANAGER_INCLUDE_FIRMWARE_MANGER_PROCESSOR_BINARY_HEADER_H_

#include <stdint.h>

#define ESF_FW_MGR_PROCESSOR_BINARY_HEADER_MAGIC_SIZE (8)
#define ESF_FW_MGR_PROCESSOR_BINARY_HEADER_DEVICE_TYPE_SIZE (4)
#define ESF_FW_MGR_PROCESSOR_BINARY_HEADER_DEVICE_VARIANT_SIZE (4)
#define ESF_FW_MGR_PROCESSOR_BINARY_HEADER_HASH_SIZE (32)

static const uint8_t kEsfFwMgrProcessorBinaryHeaderMagic
    [ESF_FW_MGR_PROCESSOR_BINARY_HEADER_MAGIC_SIZE] = {0x41, 0x49, 0x54, 0x52,
                                                       0x49, 0x4f, 0x53, 0x70};

typedef struct EsfFwMgrProcessorBinaryHeader {
  uint8_t magic[ESF_FW_MGR_PROCESSOR_BINARY_HEADER_MAGIC_SIZE];
  uint8_t header_version;

  uint8_t reserved_0[3];
  uint32_t header_size;
  uint64_t reserved_1;  // for body_size

  uint8_t sw_arch_version;
  uint8_t reserved_2[3];
  char device_type[ESF_FW_MGR_PROCESSOR_BINARY_HEADER_DEVICE_TYPE_SIZE];
  char device_variant[ESF_FW_MGR_PROCESSOR_BINARY_HEADER_DEVICE_VARIANT_SIZE];

  uint8_t reserved_3[60];

  uint8_t hash[ESF_FW_MGR_PROCESSOR_BINARY_HEADER_HASH_SIZE];
} EsfFwMgrProcessorBinaryHeader;

#endif  // ESF_FIRMWARE_MANAGER_INCLUDE_FIRMWARE_MANGER_PROCESSOR_BINARY_HEADER_H_
