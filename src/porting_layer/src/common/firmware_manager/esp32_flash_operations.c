/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp32_flash_operations.h"

#include <errno.h>
#include <fcntl.h>
#include <nuttx/fs/fs.h>
#include <nuttx/mtd/mtd.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "firmware_manager_porting_layer_log.h"
#include "verify_handle.h"

// Internal enumerated type and structures -------------------------------------

typedef struct TagEsp32FlashOpContext {
  int ota_fd;
} Esp32FlashOpContext;

// Global variables ------------------------------------------------------------

static const char *kOtadataPath = ESF_PL_FW_MGR_PARTITION_MOUNT_POINT "otadata";
static const char *kOta0Path = ESF_PL_FW_MGR_PARTITION_MOUNT_POINT "ota_0";
static const char *kOta1Path = ESF_PL_FW_MGR_PARTITION_MOUNT_POINT "ota_1";

// Currently used context. Only one context can exist.
static Esp32FlashOpContext *s_active_context = NULL;

// Internal functions ----------------------------------------------------------

/// @brief Verify `partition` is valid.
/// @param partition
/// @return true: valid, false: invalid
static bool VerifyPartition(FwMgrPlOtaImgBootseq partition) {
  bool ret = false;
  switch (partition) {
    case OTA_IMG_BOOT_FACTORY:
    case OTA_IMG_BOOT_OTA_0:
    case OTA_IMG_BOOT_OTA_1:
      ret = true;
      break;

    default:
      ret = false;
      break;
  }

  return ret;
}

/// @brief Open file descriptor of the specified partition.
/// @param partition [in]
/// @param flag [in] flag for open()
/// @param fd [out] file descriptor
/// @return kOk: success, otherwise: failed.
static PlErrCode OpenPartition(FwMgrPlOtaImgBootseq partition, int flag,
                               int *fd) {
  if (fd == NULL) {
    FW_MGR_PL_DLOG_ERROR("`fd` is NULL.\n");
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0xe0FlashOps);
    return kPlErrInvalidParam;
  }

  switch (partition) {
    case OTA_IMG_BOOT_OTA_0:
      *fd = open(kOta0Path, flag);
      if (*fd < 0) {
        FW_MGR_PL_DLOG_CRITICAL("Fail to open %s: errno = %d.\n", kOta0Path,
                                errno);
        FW_MGR_PL_ELOG_CRITICAL(kFwMgrPlElogCriticalId0xe0FlashOps);
        return kPlErrInternal;
      }
      break;

    case OTA_IMG_BOOT_OTA_1:
      *fd = open(kOta1Path, flag);
      if (*fd < 0) {
        FW_MGR_PL_DLOG_CRITICAL("Fail to open %s: errno = %d.\n", kOta1Path,
                                errno);
        FW_MGR_PL_ELOG_CRITICAL(kFwMgrPlElogCriticalId0xe1FlashOps);
        return kPlErrInternal;
      }
      break;

    default:
      return kPlErrInvalidParam;
  }

  return kPlErrCodeOk;
}

/// @brief Convert an integer to the corresponding FwMgrPlOtaImgBootseq (e.g.,
/// 0 to OTA_IMG_BOOT_OTA_0)
/// @param ota [in] integer
/// @return enum ota_img_boot_e
static FwMgrPlOtaImgBootseq Int2OtaPartition(uint32_t ota) {
  FwMgrPlOtaImgBootseq partition = OTA_IMG_BOOT_INVALID;
  switch (ota) {
    case 0:
      partition = OTA_IMG_BOOT_OTA_0;
      break;

    case 1:
      partition = OTA_IMG_BOOT_OTA_1;
      break;

    default:
      FW_MGR_PL_DLOG_CRITICAL("other BOOT OTA. ota=%u", ota);
      FW_MGR_PL_ELOG_CRITICAL(kFwMgrPlElogCriticalId0xe2FlashOps);
      partition = OTA_IMG_BOOT_INVALID;
      break;
  }

  return partition;
}

// Public functions ----------------------------------------------------------

/// @brief Get the partition which is currently running by checking which ota
/// partition is currently mapped as IROM.
/// @param partition [out]
/// @return kOk: success, otherwise: failed.
PlErrCode Esp32FlashOpGetActivePartition(FwMgrPlOtaImgBootseq *partition) {
  FW_MGR_PL_DLOG_DEBUG("Start.\n");

  if (partition == NULL) {
    FW_MGR_PL_DLOG_ERROR("`partition` is NULL.\n");
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0xe1FlashOps);
    return kPlErrInvalidParam;
  }

  *partition = OTA_IMG_BOOT_INVALID;

  for (uint32_t ota = 0; ota < kEsp32FlashNumOtaPartitions; ++ota) {
    int fd = -1;
    bool mapped = false;
    FwMgrPlOtaImgBootseq target = Int2OtaPartition(ota);

    PlErrCode ret = OpenPartition(target, O_RDONLY, &fd);
    if (ret != kPlErrCodeOk) {
      FW_MGR_PL_DLOG_ERROR("Failed to open partition = %u.\n", *partition);
      FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0xe2FlashOps);
      continue;
    }

    int r = ioctl(fd, _MTDIOC(OTA_IMG_IS_MAPPED_AS_TEXT), &mapped);
    close(fd);
    if (r < 0) {
      FW_MGR_PL_DLOG_CRITICAL("Failed to check if ota_%u is mapped as IROM.\n",
                              ota);
      FW_MGR_PL_ELOG_CRITICAL(kFwMgrPlElogCriticalId0xe3FlashOps);
      continue;
    }

    if (mapped) {
      *partition = target;
      break;
    }
  }

  if (*partition == OTA_IMG_BOOT_INVALID) {
    FW_MGR_PL_DLOG_CRITICAL("No ota partition is mapped as IROM\n");
    FW_MGR_PL_ELOG_CRITICAL(kFwMgrPlElogCriticalId0xe4FlashOps);
    return kPlErrInvalidValue;
  }

  FW_MGR_PL_DLOG_INFO("Active partition = %u\n", *partition);

  return kPlErrCodeOk;
}

/// @brief Get the ota partition which will be used in the next boot by reading
/// "otadata" partition of the flash memory.
/// @param partition [out]
/// @return kOk: success, otherwise: failed.
PlErrCode Esp32FlashOpGetNextBootPartition(FwMgrPlOtaImgBootseq *partition) {
  FW_MGR_PL_DLOG_DEBUG("Start.\n");

  if (partition == NULL) {
    FW_MGR_PL_DLOG_ERROR("`partition` is NULL.\n");
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0xe3FlashOps);
    return kPlErrInvalidParam;
  }

  int fd = open(kOtadataPath, O_RDONLY);
  if (fd < 0) {
    FW_MGR_PL_DLOG_CRITICAL("Fail to open %s: errno = %d.\n", kOtadataPath,
                            errno);
    FW_MGR_PL_ELOG_CRITICAL(kFwMgrPlElogCriticalId0xe5FlashOps);
    return kPlErrInternal;
  }

  PlErrCode ret = kPlErrCodeError;
  *partition = OTA_IMG_BOOT_INVALID;

  int r = ioctl(fd, _MTDIOC(OTA_IMG_GET_BOOT), partition);
  if (r < 0) {
    FW_MGR_PL_DLOG_CRITICAL(
        "Fail to get partition used in the next boot: errno = %d.\n", errno);
    FW_MGR_PL_ELOG_CRITICAL(kFwMgrPlElogCriticalId0xe6FlashOps);
    ret = kPlErrInternal;
    goto close_file_then_exit;
  }

  if (!VerifyPartition(*partition)) {
    FW_MGR_PL_DLOG_CRITICAL("Obtained partition is invalid\n");
    FW_MGR_PL_ELOG_CRITICAL(kFwMgrPlElogCriticalId0xe7FlashOps);
    ret = kPlErrInvalidValue;
    goto close_file_then_exit;
  }

  FW_MGR_PL_DLOG_INFO("Next boot partition = %u\n", *partition);

  ret = kPlErrCodeOk;

close_file_then_exit:
  close(fd);
  return ret;
}

/// @brief Update the flag (in "otadata") of the specified partition so that the
/// partition is valid and has the highest priority.
/// @param partition [in]
/// @return kOk: success, otherwise: failed.
PlErrCode Esp32FlashOpSetNextBootPartition(FwMgrPlOtaImgBootseq partition) {
  FW_MGR_PL_DLOG_DEBUG("Start.\n");

  if (!VerifyPartition(partition)) {
    FW_MGR_PL_DLOG_ERROR("`partition` is Invalid.\n");
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0xe4FlashOps);
    return kPlErrInvalidParam;
  }

  int fd = open(kOtadataPath, O_RDWR);
  if (fd < 0) {
    FW_MGR_PL_DLOG_CRITICAL("Fail to open %s: errno = %d.\n", kOtadataPath,
                            errno);
    FW_MGR_PL_ELOG_CRITICAL(kFwMgrPlElogCriticalId0xe8FlashOps);
    return kPlErrInternal;
  }

  PlErrCode ret = kPlErrCodeError;

  int r = ioctl(fd, _MTDIOC(OTA_IMG_SET_BOOT), partition);
  if (r < 0) {
    FW_MGR_PL_DLOG_CRITICAL(
        "Fail to set running partition number: partition = %u, errno = %d.\n",
        partition, errno);
    FW_MGR_PL_ELOG_CRITICAL(kFwMgrPlElogCriticalId0xe9FlashOps);
    ret = kPlErrInternal;
    goto close_file_then_exit;
  }

  FW_MGR_PL_DLOG_INFO("Set partition = %u\n", partition);

  ret = kPlErrCodeOk;

close_file_then_exit:
  close(fd);
  return ret;
}

/// @brief Get the size of the ota_0/ota_1 partition.
/// @param partition [in]
/// @param partition_size [out] size in bytes.
/// @return kOk: Success, otherwise: failed.
PlErrCode Esp32FlashOpGetPartitionSize(FwMgrPlOtaImgBootseq partition,
                                       uint32_t *partition_size) {
  FW_MGR_PL_DLOG_DEBUG("Start.\n");

  if (partition_size == NULL) {
    FW_MGR_PL_DLOG_ERROR("`partition_size` is NULL.\n");
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0xe5FlashOps);
    return kPlErrInvalidParam;
  }

  int fd = -1;
  PlErrCode ret = OpenPartition(partition, O_RDONLY, &fd);
  if (ret != kPlErrCodeOk) {
    FW_MGR_PL_DLOG_ERROR("OpenPartition() failed. partition = %u\n", partition);
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0xe6FlashOps);
    return ret;
  }

  struct partition_info_s partition_info = {
      .numsectors = 0, .sectorsize = 0, .startsector = 0, .parent = "\0"};
  int r = ioctl(fd, BIOC_PARTINFO, &partition_info);
  if (r != 0) {
    FW_MGR_PL_DLOG_CRITICAL("ioctl(BIOC_PARTINFO) failed. errno=%d\n", errno);
    FW_MGR_PL_ELOG_CRITICAL(kFwMgrPlElogCriticalId0xeaFlashOps);
    ret = kPlErrInternal;
    goto close_file_then_exit;
  }

  *partition_size = partition_info.numsectors * partition_info.sectorsize;
  ret = kPlErrCodeOk;

  FW_MGR_PL_DLOG_DEBUG("numsectors = 0x%zx\n", partition_info.numsectors);
  FW_MGR_PL_DLOG_DEBUG("sectorsize = 0x%zx\n", partition_info.sectorsize);
  FW_MGR_PL_DLOG_DEBUG("startsector = 0x%x\n",
                       (uint32_t)partition_info.startsector);
  FW_MGR_PL_DLOG_DEBUG("parent = %s\n", partition_info.parent);

  FW_MGR_PL_DLOG_DEBUG("partition_size = 0x%x\n", *partition_size);

  // TODO: Make sure that partition_size is smaller
  // than flash memory size.

close_file_then_exit:
  close(fd);
  return ret;
}

/// @brief Set invalid flag of ota_0/ota_1 partition. (The flag is in the
/// otadata partition)
/// @param partition [in]
/// @return kOk: success, otherwise: failed.
PlErrCode Esp32FlashOpInvalidatePartition(FwMgrPlOtaImgBootseq partition) {
  FW_MGR_PL_DLOG_DEBUG("Start.\n");

  if (!VerifyPartition(partition)) {
    FW_MGR_PL_DLOG_ERROR("`partition` is Invalid. partirion = %u\n", partition);
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0xe7FlashOps);
    return kPlErrInvalidParam;
  }

  int fd = open(kOtadataPath, O_RDWR);
  if (fd < 0) {
    FW_MGR_PL_DLOG_CRITICAL("Fail to open %s: errno = %d.\n", kOtadataPath,
                            errno);
    FW_MGR_PL_ELOG_CRITICAL(kFwMgrPlElogCriticalId0xebFlashOps);
    return kPlErrInternal;
  }

  PlErrCode ret = kPlErrCodeError;

  int r = ioctl(fd, _MTDIOC(OTA_IMG_INVALIDATE_BOOT), partition);
  if (r < 0) {
    FW_MGR_PL_DLOG_CRITICAL(
        "Fail to invalidate partition: partition = %u, errno = %d.\n",
        partition, errno);
    FW_MGR_PL_ELOG_CRITICAL(kFwMgrPlElogCriticalId0xecFlashOps);
    ret = kPlErrInternal;
    goto close_file_then_exit;
  }

  FW_MGR_PL_DLOG_DEBUG("Invalidate partition = %u\n", partition);

  ret = kPlErrCodeOk;

close_file_then_exit:
  close(fd);
  return ret;
}

/// @brief Erase the data in the ota_0/ota_1 partition.
/// @param partition [in]
/// @return kOk: success, otherwise: failed.
PlErrCode Esp32FlashOpErasePartitionData(FwMgrPlOtaImgBootseq partition) {
  FW_MGR_PL_DLOG_DEBUG("Start.\n");

  int fd = -1;
  PlErrCode ret = OpenPartition(partition, O_WRONLY, &fd);
  if (ret != kPlErrCodeOk) {
    FW_MGR_PL_DLOG_ERROR("Failed to open partition: %u\n", partition);
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0xe8FlashOps);
    return ret;
  }

  int r = ioctl(fd, MTDIOC_BULKERASE);
  if (r < 0) {
    FW_MGR_PL_DLOG_CRITICAL("ioctl(MTDIOC_BULKERASE) failed. errno = %d\n",
                            errno);
    FW_MGR_PL_ELOG_CRITICAL(kFwMgrPlElogCriticalId0xedFlashOps);
    ret = kPlErrInternal;
    goto close_file_then_exit;
  }

close_file_then_exit:
  close(fd);
  return ret;
}

/// @brief Read the data in the ota_0/ota_1 partition. If HW encryption of the
/// flash memory is enabled, data is DECRYPTED.
/// @param partition [in]
/// @param offset [in]
/// @param read_size [in]
/// @param buf [out]
/// @return kOk: success, otherwise: failed.
PlErrCode Esp32FlashOpReadPartitionData(FwMgrPlOtaImgBootseq partition,
                                        uint32_t offset, uint32_t read_size,
                                        uint8_t *buf) {
  FW_MGR_PL_DLOG_DEBUG("Start.\n");

  const int kLabelSize = 8;
  char label[kLabelSize];
  switch (partition) {
    case OTA_IMG_BOOT_OTA_0:
      snprintf(label, kLabelSize, "ota_0");
      break;

    case OTA_IMG_BOOT_OTA_1:
      snprintf(label, kLabelSize, "ota_1");
      break;

    default:
      FW_MGR_PL_DLOG_ERROR("Invalid partition: %u\n", partition);
      FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0xe9FlashOps);
      return kPlErrInvalidParam;
  }

  uint32_t partition_size;
  PlErrCode ret = Esp32FlashOpGetPartitionSize(partition, &partition_size);
  if (ret != kPlErrCodeOk) {
    FW_MGR_PL_DLOG_ERROR("Esp32FlashOpGetPartitionSize failed. ret = %u\n",
                         ret);
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0xf0FlashOps);
    return kPlErrInternal;
  }

  // uint64_t variable is used to avoid overflow.
  uint64_t end = (uint64_t)offset + (uint64_t)read_size;
  if (end > partition_size) {
    FW_MGR_PL_DLOG_ERROR(
        "Invalid arguments. offset = 0x%x, read_size = 0x%x (partition_size = "
        "0x%x\n",
        offset, read_size, partition_size);
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0xf1FlashOps);
    return kPlErrInvalidParam;
  }

  int r = Esp32PartitionReadDecrypt(label, offset, buf, read_size);
  if (r != 0) {
    FW_MGR_PL_DLOG_CRITICAL(
        "Esp32PartitionReadDecrypt() failed. label = %s, ret = %d\n", label, r);
    FW_MGR_PL_ELOG_CRITICAL(kFwMgrPlElogCriticalId0xeeFlashOps);
    return kPlErrInternal;
  }

  return kPlErrCodeOk;
}

// To Write data to a partition of the flash memory, use following three
// functions.
// (1) Esp32FlashOpOpenPartition: Get `handle`.
// (2) Esp32FlashOpWritePartitionData: Write data (using `handle` obtained in
//     (1)). (Esp32FlashOpWritePartitionData can be called multiple times.)
// (3) Esp32FlashOpClosePartition: Close the file (using `handle` obtained
//     in (1)).

/// @brief
/// @param partition [in]
/// @param handle [out]
/// @return
PlErrCode Esp32FlashOpOpenPartition(FwMgrPlOtaImgBootseq partition,
                                    Esp32FlashOpHandle *handle) {
  FW_MGR_PL_DLOG_DEBUG("Start.\n");

  if (handle == NULL) {
    FW_MGR_PL_DLOG_ERROR("`handle` is NULL.\n");
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0xeaFlashOps);
    return kPlErrInvalidParam;
  }

  if (s_active_context != NULL) {
    FW_MGR_PL_DLOG_ERROR("There is an active handle.\n");
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0xebFlashOps);
    return kPlErrInvalidState;
  }

  Esp32FlashOpContext *context =
      (Esp32FlashOpContext *)malloc(sizeof(Esp32FlashOpContext));
  if (context == NULL) {
    FW_MGR_PL_DLOG_CRITICAL("Failed to allocate memory for context");
    FW_MGR_PL_ELOG_CRITICAL(kFwMgrPlElogCriticalId0xefFlashOps);
    return kPlErrInternal;
  }

  context->ota_fd = -1;

  FW_MGR_PL_DLOG_DEBUG("Opening partition = %u\n", partition);
  PlErrCode ret = OpenPartition(partition, O_WRONLY, &context->ota_fd);
  if (ret != 0) {
    FW_MGR_PL_DLOG_ERROR("OpenPartition() failed. ret = %u\n", ret);
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0xecFlashOps);
    goto err_exit;
  }

  s_active_context = context;
  *handle = (Esp32FlashOpHandle)context;

  return kPlErrCodeOk;

err_exit:
  free(context);
  return ret;
}

/// @brief
/// @param handle [in]
/// @return
PlErrCode Esp32FlashOpClosePartition(Esp32FlashOpHandle handle) {
  FW_MGR_PL_DLOG_DEBUG("Start.\n");

  if (!FW_MGR_PL_VERIFY_HANDLE(handle)) {
    FW_MGR_PL_DLOG_ERROR("Invalid `handle`.\n");
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0xedFlashOps);
    return kPlErrInvalidParam;
  }

  Esp32FlashOpContext *context = (Esp32FlashOpContext *)handle;

  close(context->ota_fd);

  free(context);
  s_active_context = NULL;

  return kPlErrCodeOk;
}

/// @brief Write the data to the ota_0/ota_1 partition.
/// @param handle [in]
/// @param write_size [in]
/// @param buf [out]
/// @param written_size [out]
/// @return kOk: success, otherwise: failed
PlErrCode Esp32FlashOpWritePartitionData(Esp32FlashOpHandle handle,
                                         uint32_t write_size,
                                         const uint8_t *buf,
                                         uint32_t *written_size) {
  FW_MGR_PL_DLOG_DEBUG("Start.\n");

  if (!FW_MGR_PL_VERIFY_HANDLE(handle)) {
    FW_MGR_PL_DLOG_ERROR("Invalid `handle`.\n");
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0xeeFlashOps);
    return kPlErrInvalidParam;
  }

  if (written_size == NULL) {
    FW_MGR_PL_DLOG_ERROR("`written_size` is NULL.\n");
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0xefFlashOps);
    return kPlErrInvalidParam;
  }
  Esp32FlashOpContext *context = (Esp32FlashOpContext *)handle;

  *written_size = 0;

  int r = write(context->ota_fd, buf, write_size);
  if (r < 0) {
    FW_MGR_PL_DLOG_CRITICAL("Failed to write partition data. errno = %d\n",
                            errno);
    FW_MGR_PL_ELOG_CRITICAL(kFwMgrPlElogCriticalId0xf0FlashOps);
    return kPlErrInternal;
  }

  *written_size = r;

  return kPlErrCodeOk;
}
