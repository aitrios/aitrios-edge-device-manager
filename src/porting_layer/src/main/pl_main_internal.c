/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pl_main_internal.h"

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/mount.h>

#ifndef CONFIG_EXTERNAL_TARGET_RPI
#include "fsutils/flash_eraseall.h"
#include "fsutils/mkfatfs.h"
#endif

// Macros ---------------------------------------------------------------------
#define PL_MAIN_FAT_FILESYSTEM_TYPE "vfat"
#define PL_MAIN_FAT32_FAT_TYPE 32
#define PL_MAIN_LITTLEFS_FILESYSTEM_TYPE "littlefs"
#define PL_MAIN_LITTLEFS_FORMAT_DATA "autoformat"
#define PL_MAIN_LITTLEFS_FORMAT_TMP_PATH "_tmp_format"

// typedef --------------------------------------------------------------------
typedef PlErrCode (*PlMainFileSystemFunc)(const PlMainDeviceInformation*);
typedef struct PlMainFileSystemOps {
  PlMainFileSystemFunc format;
  PlMainFileSystemFunc mount;
  PlMainFileSystemFunc unmount;
} PlMainFileSystemOps;

// Local Functions ------------------------------------------------------------
static PlErrCode PlMainDeviceInformationValidate(
    const PlMainDeviceInformation* info, size_t info_size);

static PlErrCode PlMainFileSystemFormatFat32(
    const PlMainDeviceInformation* info);
static PlErrCode PlMainFileSystemMountFat32(
    const PlMainDeviceInformation* info);
static PlErrCode PlMainFileSystemUnmountFat32(
    const PlMainDeviceInformation* info);

static PlErrCode PlMainFileSystemFormatLittleFs(
    const PlMainDeviceInformation* info);
static PlErrCode PlMainFileSystemMountLittleFs(
    const PlMainDeviceInformation* info);
static PlErrCode PlMainFileSystemUnmountLittleFs(
    const PlMainDeviceInformation* info);

static PlErrCode PlMainFileSystemFormatOther(
    const PlMainDeviceInformation* info);
static PlErrCode PlMainFileSystemMountOther(
    const PlMainDeviceInformation* info);
static PlErrCode PlMainFileSystemUnmountOther(
    const PlMainDeviceInformation* info);

// Local Variables ------------------------------------------------------------
static const PlMainFileSystemOps kFileSystemOps[kPlMainFileSystemMax] = {
    {
        PlMainFileSystemFormatFat32,
        PlMainFileSystemMountFat32,
        PlMainFileSystemUnmountFat32,
    },
    {
        PlMainFileSystemFormatLittleFs,
        PlMainFileSystemMountLittleFs,
        PlMainFileSystemUnmountLittleFs,
    },
    {
        PlMainFileSystemFormatOther,
        PlMainFileSystemMountOther,
        PlMainFileSystemUnmountOther,
    },
};

// Functions ------------------------------------------------------------------
PlErrCode PlMainInternalEmmcFormat(const PlMainDeviceInformation* info,
                                   size_t info_size, PlMainKeepAliveCallback cb,
                                   void* user_data) {
  LOG_D("eMMC format entry.");
  PlErrCode ret = PlMainDeviceInformationValidate(info, info_size);
  if (ret != kPlErrCodeOk) {
    LOG_E(0x00, "parameter error.");
    return ret;
  }
  for (size_t i = 0; i < info_size; ++i) {
    if (info[i].device_type != kPlMainDeviceEmmc) {
      continue;
    }
    if (!info[i].format) {
      continue;
    }
    if (cb != NULL) {
      cb(user_data);
    }
    PlErrCode ops_ret = kFileSystemOps[info[i].fs_type].format(&info[i]);
    if (ops_ret != kPlErrCodeOk) {
      ret = ops_ret;
      // The process will continue even if an error occurs.
    }
  }
  LOG_D("eMMC format exit.");
  return ret;
}
// -----------------------------------------------------------------------------
PlErrCode PlMainInternalEmmcMount(const PlMainDeviceInformation* info,
                                  size_t info_size) {
  LOG_D("eMMC mount entry.");
  PlErrCode ret = PlMainDeviceInformationValidate(info, info_size);
  if (ret != kPlErrCodeOk) {
    LOG_E(0x01, "parameter error.");
    return ret;
  }
  for (size_t i = 0; i < info_size; ++i) {
    if (info[i].device_type != kPlMainDeviceEmmc) {
      continue;
    }
    if (info[i].target == NULL) {
      continue;
    }
    PlErrCode ops_ret = kFileSystemOps[info[i].fs_type].mount(&info[i]);
    if (ops_ret != kPlErrCodeOk) {
      ret = ops_ret;
      // The process will continue even if an error occurs.
    }
  }
  LOG_D("eMMC mount exit.");
  return ret;
}
// -----------------------------------------------------------------------------
PlErrCode PlMainInternalEmmcUnmount(const PlMainDeviceInformation* info,
                                    size_t info_size) {
  LOG_D("eMMC unmount entry.");
  PlErrCode ret = PlMainDeviceInformationValidate(info, info_size);
  if (ret != kPlErrCodeOk) {
    LOG_E(0x02, "parameter error.");
    return ret;
  }
  for (size_t i = 0; i < info_size; ++i) {
    if (info[i].device_type != kPlMainDeviceEmmc) {
      continue;
    }
    if (info[i].target == NULL) {
      continue;
    }
    PlErrCode ops_ret = kFileSystemOps[info[i].fs_type].unmount(&info[i]);
    if (ops_ret != kPlErrCodeOk) {
      ret = ops_ret;
      // The process will continue even if an error occurs.
    }
  }
  LOG_D("eMMC unmount exit.");
  return ret;
}
// -----------------------------------------------------------------------------
PlErrCode PlMainInternalFlashFormat(const PlMainDeviceInformation* info,
                                    size_t info_size,
                                    PlMainKeepAliveCallback cb,
                                    void* user_data) {
  LOG_D("Flash format entry.");
  PlErrCode ret = PlMainDeviceInformationValidate(info, info_size);
  if (ret != kPlErrCodeOk) {
    LOG_E(0x03, "parameter error.");
    return ret;
  }
  for (size_t i = 0; i < info_size; ++i) {
    if (info[i].device_type != kPlMainDeviceFlash) {
      continue;
    }
    if (!info[i].format) {
      continue;
    }
    if (cb != NULL) {
      cb(user_data);
    }
    PlErrCode ops_ret = kFileSystemOps[info[i].fs_type].format(&info[i]);
    if (ops_ret != kPlErrCodeOk) {
      ret = ops_ret;
      // The process will continue even if an error occurs.
    }
  }
  LOG_D("Flash format exit.");
  return ret;
}
// -----------------------------------------------------------------------------
PlErrCode PlMainInternalFlashMount(const PlMainDeviceInformation* info,
                                   size_t info_size) {
  LOG_D("Flash mount entry.");
  PlErrCode ret = PlMainDeviceInformationValidate(info, info_size);
  if (ret != kPlErrCodeOk) {
    LOG_E(0x04, "parameter error.");
    return ret;
  }
  for (size_t i = 0; i < info_size; ++i) {
    if (info[i].device_type != kPlMainDeviceFlash) {
      continue;
    }
    if (info[i].target == NULL) {
      continue;
    }
    PlErrCode ops_ret = kFileSystemOps[info[i].fs_type].mount(&info[i]);
    if (ops_ret != kPlErrCodeOk) {
      ret = ops_ret;
      // The process will continue even if an error occurs.
    }
  }
  LOG_D("Flash mount exit.");
  return ret;
}
// -----------------------------------------------------------------------------
PlErrCode PlMainInternalFlashUnmount(const PlMainDeviceInformation* info,
                                     size_t info_size) {
  LOG_D("Flash unmount entry.");
  PlErrCode ret = PlMainDeviceInformationValidate(info, info_size);
  if (ret != kPlErrCodeOk) {
    LOG_E(0x05, "parameter error.");
    return ret;
  }
  for (size_t i = 0; i < info_size; ++i) {
    if (info[i].device_type != kPlMainDeviceFlash) {
      continue;
    }
    if (info[i].target == NULL) {
      continue;
    }
    PlErrCode ops_ret = kFileSystemOps[info[i].fs_type].unmount(&info[i]);
    if (ops_ret != kPlErrCodeOk) {
      ret = ops_ret;
      // The process will continue even if an error occurs.
    }
  }
  LOG_D("Flash unmount exit.");
  return ret;
}
// -----------------------------------------------------------------------------
PlErrCode PlMainInternalIsFeatureSupported(const PlMainFeatureType* support,
                                           size_t support_size,
                                           PlMainFeatureType type) {
  if (type < 0 || PlMainFeatureMax <= type) {
    LOG_E(0x06, "Parameter error. type:%d", (int)type);
    return kPlErrInvalidParam;
  }
  for (size_t i = 0; i < support_size; ++i) {
    if (support[i] == type) {
      return kPlErrCodeOk;
    }
  }
  LOG_E(0x06, "Not supported. type:%d", (int)type);
  return kPlErrNoSupported;
}
// -----------------------------------------------------------------------------
static PlErrCode PlMainDeviceInformationValidate(
    const PlMainDeviceInformation* info, size_t info_size) {
  if (info == NULL) {
    return kPlErrInternal;
  }
  for (size_t i = 0; i < info_size; ++i) {
    if (info[i].device_type < 0 || kPlMainDeviceMax <= info[i].device_type) {
      return kPlErrInternal;
    }
    if (info[i].fs_type < 0 || kPlMainFileSystemMax <= info[i].fs_type) {
      return kPlErrInternal;
    }
    if (info[i].source == NULL) {
      return kPlErrInternal;
    }
    if (info[i].device_type == kPlMainDeviceEmmc &&
        info[i].fs_type != kPlMainFileSystemFat32) {
      return kPlErrInternal;
    }
    if (info[i].device_type == kPlMainDeviceFlash &&
        info[i].fs_type == kPlMainFileSystemFat32) {
      return kPlErrInternal;
    }
  }
  return kPlErrCodeOk;
}
// -----------------------------------------------------------------------------
static PlErrCode PlMainFileSystemFormatFat32(
    const PlMainDeviceInformation* info) {
  LOG_I("FAT32 format begin. device:%s", info->source);
#if defined(CONFIG_FSUTILS_MKFATFS)
  struct fat_format_s fmt = FAT_FORMAT_INITIALIZER;
  fmt.ff_fattype = PL_MAIN_FAT32_FAT_TYPE;
  int ret = mkfatfs(info->source, &fmt);
  if (ret < 0) {
    LOG_E(0x00, "mkfatfs error. errno:%d device:%s", errno, info->source);
    return kPlErrWrite;
  }
  LOG_I("FAT32 format end. device:%s", info->source);
  return kPlErrCodeOk;
#else   // CONFIG_FSUTILS_MKFATFS
  LOG_E(0x00, "Unexpected operation. device:%s type:%d", info->source,
        (int)info->device_type);
  return kPlErrInternal;
#endif  // CONFIG_FSUTILS_MKFATFS
}
// -----------------------------------------------------------------------------
static PlErrCode PlMainFileSystemMountFat32(
    const PlMainDeviceInformation* info) {
  LOG_I("FAT32 mount begin. device:%s target:%s", info->source, info->target);
  int ret = mount(info->source, info->target, PL_MAIN_FAT_FILESYSTEM_TYPE, 0,
                  NULL);
  if (ret < 0) {
    LOG_E(0x01, "mount error. errno:%d device:%s target:%s", errno,
          info->source, info->target);
    return kPlErrWrite;
  }
  LOG_I("FAT32 mount end. device:%s target:%s", info->source, info->target);
  return kPlErrCodeOk;
}
// -----------------------------------------------------------------------------
static PlErrCode PlMainFileSystemUnmountFat32(
    const PlMainDeviceInformation* info) {
  LOG_I("FAT32 unmount begin. device:%s target:%s", info->source, info->target);
  int ret = umount(info->target);
  if (ret < 0) {
    LOG_E(0x02, "unmount error. errno:%d device:%s target:%s", errno,
          info->source, info->target);
    return kPlErrWrite;
  }
  LOG_I("FAT32 unmount end. device:%s target:%s", info->source, info->target);
  return kPlErrCodeOk;
}
// -----------------------------------------------------------------------------
static PlErrCode PlMainFileSystemFormatLittleFs(
    const PlMainDeviceInformation* info) {
#ifdef CONFIG_EXTERNAL_TARGET_RPI
  return kPlErrNoSupported;
#else
  LOG_I("LittleFS format begin. device:%s", info->source);
  int ret = flash_eraseall(info->source);
  if (ret < 0) {
    LOG_E(0x03, "flash erase error. device:%s ret:%d errno:%d", info->source,
          ret, errno);
    return kPlErrWrite;
  }
  ret = mount(info->source, PL_MAIN_LITTLEFS_FORMAT_TMP_PATH,
              PL_MAIN_LITTLEFS_FILESYSTEM_TYPE, 0,
              PL_MAIN_LITTLEFS_FORMAT_DATA);
  if (ret < 0) {
    LOG_E(0x03, "autoformat error. device:%s ret:%d errno:%d", info->source,
          ret, errno);
    return kPlErrWrite;
  }
  ret = umount(PL_MAIN_LITTLEFS_FORMAT_TMP_PATH);
  if (ret < 0) {
    LOG_E(0x03, "autoformat umount failure:%s ret:%d errno:%d",
          PL_MAIN_LITTLEFS_FORMAT_TMP_PATH, ret, errno);
    return kPlErrWrite;
  }
  LOG_I("LittleFS format end. device:%s", info->source);
  return kPlErrCodeOk;
#endif
}
// -----------------------------------------------------------------------------
static PlErrCode PlMainFileSystemMountLittleFs(
    const PlMainDeviceInformation* info) {
  LOG_I("LittleFS mount begin. device:%s target:%s", info->source,
        info->target);
  int ret = mount(info->source, info->target, PL_MAIN_LITTLEFS_FILESYSTEM_TYPE,
                  0, NULL);
  if (ret < 0) {
    LOG_E(0x04, "mount error. errno:%d device:%s target:%s", errno,
          info->source, info->target);
    return kPlErrWrite;
  }
  LOG_I("LittleFS mount end. device:%s target:%s", info->source, info->target);
  return kPlErrCodeOk;
}
// -----------------------------------------------------------------------------
static PlErrCode PlMainFileSystemUnmountLittleFs(
    const PlMainDeviceInformation* info) {
  LOG_I("LittleFS unmount begin. device:%s target:%s", info->source,
        info->target);
  int ret = umount(info->target);
  if (ret < 0) {
    LOG_E(0x05, "unmount error. errno:%d device:%s target:%s", errno,
          info->source, info->target);
    return kPlErrWrite;
  }
  LOG_I("LittleFS unmount end. device:%s target:%s", info->source,
        info->target);
  return kPlErrCodeOk;
}
// -----------------------------------------------------------------------------
static PlErrCode PlMainFileSystemFormatOther(
    const PlMainDeviceInformation* info) {
#ifdef CONFIG_EXTERNAL_TARGET_RPI
  return kPlErrNoSupported;
#else
  LOG_I("Other format begin. device:%s", info->source);
  int ret = flash_eraseall(info->source);
  if (ret < 0) {
    LOG_E(0x03, "flash erase error. device:%s ret:%d errno:%d", info->source,
          ret, errno);
    return kPlErrWrite;
  }
  LOG_I("Other format end. device:%s", info->source);
  return kPlErrCodeOk;
#endif
}
// -----------------------------------------------------------------------------
static PlErrCode PlMainFileSystemMountOther(
    const PlMainDeviceInformation* info) {
  LOG_I("Other mount begin. device:%s target:%s", info->source, info->target);
  LOG_E(0x04, "Unexpected operation. device:%s target:%s", info->source,
        info->target);
  LOG_I("Other mount end. device:%s target:%s", info->source, info->target);
  return kPlErrInternal;
}
// -----------------------------------------------------------------------------
static PlErrCode PlMainFileSystemUnmountOther(
    const PlMainDeviceInformation* info) {
  LOG_I("Other unmount begin. device:%s target:%s", info->source, info->target);
  LOG_E(0x05, "Unexpected operation. device:%s target:%s", info->source,
        info->target);
  LOG_I("Other unmount end. device:%s target:%s", info->source, info->target);
  return kPlErrInternal;
}
