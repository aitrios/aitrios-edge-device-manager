/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Note: This module is used after all other modules have been finalized, so it
// must not depend on any other modules (including the Log Manager).

#include "firmware_manager_esp32_switch_firmware_partition.h"

#include <fcntl.h>
#include <nuttx/fs/fs.h>
#include <nuttx/mtd/mtd.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "esp32_flash_operations_processor_specific.h"

#define LOG(level, format, ...) \
  printf("[" level              \
         "] "                   \
         "%s-%d: %s " format,   \
         __FILE__, __LINE__, __func__, ##__VA_ARGS__)

static const char *kOtadataPath = ESF_PL_FW_MGR_PARTITION_MOUNT_POINT "otadata";
static const char *kOta0Path = ESF_PL_FW_MGR_PARTITION_MOUNT_POINT "ota_0";
static const char *kOta1Path = ESF_PL_FW_MGR_PARTITION_MOUNT_POINT "ota_1";
static const int kNumPartitions = 2;

PlErrCode FwMgrEsp32SwitchFirmwarePartition(void) {
  LOG("INFO", "Called.\n");

  FwMgrPlOtaImgBootseq non_active_partition;

  // Check which Partition (ota_0 or ota_1) is currently active, and set
  // `non_active_partition` to the opposite Partition.

  bool not_found = true;  // set false when the active partition is found

  for (int i = 0; i < kNumPartitions; ++i) {
    FwMgrPlOtaImgBootseq partition;
    int fd = -1;

    if (i == 0) {
      fd = open(kOta0Path, O_RDONLY);
      partition = OTA_IMG_BOOT_OTA_1;
    } else {
      fd = open(kOta1Path, O_RDONLY);
      partition = OTA_IMG_BOOT_OTA_0;
    }
    if (fd < 0) {
      LOG("WARNING", "Failed to open ota_%d.\n", i);
      continue;
    }

    bool mapped = false;
    int r = ioctl(fd, _MTDIOC(OTA_IMG_IS_MAPPED_AS_TEXT), &mapped);
    close(fd);
    if (r < 0) {
      LOG("WARNING", "ioctl(OTA_IMG_IS_MAPPED_AS_TEXT) failed. (ota_%d).\n", i);
      continue;
    }

    if (mapped) {
      non_active_partition = partition;
      not_found = false;
      break;
    }
  }

  if (not_found) {
    LOG("ERROR", "Failed to find the active partition.\n");
    return kPlErrInvalidValue;
  }

  int fd = open(kOtadataPath, O_RDWR);
  if (fd < 0) {
    LOG("ERROR", "Failed to open %s\n", kOtadataPath);
    return kPlErrInternal;
  }

  int r = ioctl(fd, _MTDIOC(OTA_IMG_SET_BOOT), non_active_partition);
  close(fd);
  if (r < 0) {
    LOG("ERROR", "ioctl(OTA_IMG_SET_BOOT) failed.\n");
    return kPlErrInternal;
  }

  return kPlErrCodeOk;
}
