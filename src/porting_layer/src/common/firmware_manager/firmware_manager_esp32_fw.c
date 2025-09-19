/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// This is the "Firmware Manager Porting Layer (Fw Mgr PL) ESP32 FW Block",  a
// part of Firmware Manager Porting Layer. This block handles operations
// specific to update, rollback, or factory reset firmware (FW) of ESP32-S3.
//
// This block receives (through `FwMgrEsp32FwWrite`) a binary that is created by
// combining
// - [A]: the firmware encrypted for ota_0 and
// - [B]: the same firmware as [A] but encrypted for ota_1.
// ([A] and [B] is different because an encryption result depends on the address
// of flash memory on which the binary is written.) The first half of the binary
// is [A], and the second half is [B].
// `total_write_size` which `FwMgrEsp32FwOpen` receives is the total size of [A]
// and [B].
// When the currently running firmware is stored in ota_0 partition of the flash
// memory, a new firmware should be written to ota_1. So, in that case, the
// first half of the binary (i.e., [A]) received through `FwMgrPlEsp32FwWrite`
// is discarded, and only the second half (i.e., [B]) is written to the flash
// memory. (And when the currently running firmware is in ota_1, the second half
// of the received binary is discard.)

// This block does NOT manage the state of Fw Mgr PL, as it is done by the Fw
// Mgr PL Common block (written in firmware_manager_porting_layer.c).
// The public functions in this block MUST NOT be executed at the same time by
// multiple threads/processes. (To make sure of it is the responsibility of the
// Fw Mgr PL Common block.)

// TODO: delete the macro ESF_FW_MGR_PL_USE_MBEDTLS_V2
// This config is enabled only for T3P.
// Currently, T3P uses mbedtls v2, while T5 uses v3. This is because Nuttx and
// EVP for T3P only support v2. In the near future, Nuttx and EVP for T3P will
// support v3, and then mbedtls v3 will be used for both T3 and T5. Until then,
// the Firmware Manager needs to switch between mbedtls v2 and v3 because the
// APIs for v2 and v3 are slightly different.

#include "firmware_manager_esp32_fw.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "esp32_flash_operations.h"
#include "firmware_manager_esp32_fw_context.h"
#include "firmware_manager_esp32_fw_processor_specific.h"
#include "firmware_manager_esp32_fw_pstorage.h"
#include "firmware_manager_porting_layer_log.h"
#include "mbedtls/sha256.h"
#include "parameter_storage_manager.h"
#include "verify_handle.h"

#ifdef CONFIG_EXTERNAL_FIRMWARE_MANAGER_USE_MBEDTLS_V2
#define ESF_FW_MGR_HANDLE_MBEDTLS_ERROR(operation, error_handling) operation
#else
#define ESF_FW_MGR_HANDLE_MBEDTLS_ERROR(operation, error_handling) \
  do {                                                             \
    int r = operation;                                             \
    if (r != 0) error_handling;                                    \
  } while (0)
#endif

// Internal enumerated type and structures -------------------------------------

typedef struct TagEsp32FirmwareInfo {
  // Whether SHA256 hash is appended at the end of the firmware image.
  bool hash_appended;
  // The size of firmware (NOT including the appended hash)
  uint32_t firmware_size;
} Esp32FirmwareInfo;

// Global variables ------------------------------------------------------------

// Currently used context. Only one context can exist.
static FwMgrEsp32FwContext *s_active_context = NULL;

// Internal Functions ---------------------------------------------------------

/// @brief Read firmware and confirm that the header magic is correct and chip
/// id corresponds to the target AP.
/// @param partition: [in] Partition where the firmware is written.
/// @param header: [out] Infomation of the firmware (which is obtained by
/// reading the header). `firmware_size` will be set only when `hash_appended ==
/// true`.
/// @return kOk: success, otherwise: failed
static PlErrCode ReadAndVerifyFirmwareHeader(FwMgrPlOtaImgBootseq partition,
                                             Esp32FirmwareInfo *header) {
  struct {
    uint8_t magic;
    uint8_t segment_count;
    uint8_t padding_1[10];
    uint16_t chip_id;
    uint8_t padding_2[9];
    uint8_t hash_appended;
  } __attribute__((packed)) header_buf;

  if (header == NULL) {
    FW_MGR_PL_DLOG_ERROR("`header` is NULL.\n");
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x60Fw);
    return kPlErrInvalidParam;
  }

  // Read the firmware header.
  PlErrCode ret = Esp32FlashOpReadPartitionData(
      partition, /* offset = */ 0, sizeof(header_buf), (uint8_t *)&header_buf);
  if (ret != kPlErrCodeOk) {
    FW_MGR_PL_DLOG_ERROR("Failed to read the firmware header.\n");
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x61Fw);
    return ret;
  }
  FW_MGR_PL_DLOG_DEBUG("header magic: 0x%x\n", header_buf.magic);
  FW_MGR_PL_DLOG_DEBUG("header chip_id: 0x%x\n", header_buf.chip_id);
  FW_MGR_PL_DLOG_DEBUG("header hash_appended: 0x%x\n",
                       header_buf.hash_appended);
  FW_MGR_PL_DLOG_DEBUG("header segment_count: %u\n", header_buf.segment_count);

  const uint8_t kEsp32BootImgIdentifierMagic = 0xE9;
  // Verify the header magic.
  if (header_buf.magic != kEsp32BootImgIdentifierMagic) {
    FW_MGR_PL_DLOG_CRITICAL(
        "The magic in the firmware header must be 0x%x, but it is 0x%x\n",
        (uint32_t)kEsp32BootImgIdentifierMagic, header_buf.magic);
    FW_MGR_PL_ELOG_CRITICAL(kFwMgrPlElogCriticalId0x60FwInvalidMagic);
    return kPlErrInvalidValue;
  }

  // Verify chip id specified in the header.
  if (header_buf.chip_id != kEsp32ChipId) {
    FW_MGR_PL_DLOG_CRITICAL(
        "Chip ID in the firmware header must be 0x%x, but it is 0x%x\n",
        (uint32_t)kEsp32ChipId, header_buf.chip_id);
    FW_MGR_PL_ELOG_CRITICAL(kFwMgrPlElogCriticalId0x61FwInvalidChipId);
    return kPlErrInvalidValue;
  }

  const uint8_t kEsp32BootImgHashAppended = 0x01;
  header->hash_appended =
      (header_buf.hash_appended == kEsp32BootImgHashAppended);
  header->firmware_size = 0;

  // Calculate the firmware size by summing up the segment size of all segments.
  // (The segment size is written in the header of each segment.)
  // The firmware size will be used only when SHA256 hash is appended. So when
  // the hash is not appended, this calculation is skipped.
  if (header->hash_appended) {
    uint32_t offset = sizeof(header_buf);
    uint32_t segment_count = (uint32_t)header_buf.segment_count;

    struct {
      uint32_t padding;
      uint32_t segment_size;
    } segment_header_buf;

    for (uint32_t i = 0; i < segment_count; ++i) {
      ret = Esp32FlashOpReadPartitionData(partition, offset,
                                          sizeof(segment_header_buf),
                                          (uint8_t *)&segment_header_buf);
      if (ret != kPlErrCodeOk) {
        FW_MGR_PL_DLOG_ERROR(
            "Failed to read the segment header of firmware. Segment No: %u, "
            "partition: %u, ret: %u\n",
            i, partition, ret);
        FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x62Fw);
        return kPlErrInternal;
      }
      offset += sizeof(segment_header_buf) + segment_header_buf.segment_size;
    }

    // 1 byte for checksum
    offset += 1;

    // Add size of padding to make firmware size a multiple of 16 is added.
    // This operation corresponds to the one in the function `process_checksum`
    // in esp_image_format.c of the 2nd bootloader. (padding_size == length in
    // the process_checksum)
    uint32_t last_four_bits = offset & 0x0000000f;
    uint32_t padding_size = 0;
    if (last_four_bits != 0) padding_size = 0x10 - last_four_bits;

    header->firmware_size = offset + padding_size;
    FW_MGR_PL_DLOG_DEBUG("Firmware size = 0x%x\n", header->firmware_size);
  }

  return kPlErrCodeOk;
}

/// @brief Verify the hash appended at the end of firmware matches the SHA256
/// hash of the firmware.
/// @param context [in]
/// @param partition [in] Partition where the firmware is written.
/// @param firmware_size [in] The size of firmware (NOT including the appended
/// hash). Must be a multiple of 16.
/// @return kOk: success, otherwise: failed
static PlErrCode VerifyFirmwareHash(FwMgrEsp32FwContext *context,
                                    FwMgrPlOtaImgBootseq partition,
                                    uint32_t firmware_size) {
  if (context == NULL) {
    FW_MGR_PL_DLOG_ERROR("context is NULL.\n");
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x63Fw);
    return kPlErrInvalidParam;
  }

  if ((context->tmp_buffer == NULL) || (context->tmp_buffer_size <= 0)) {
    FW_MGR_PL_DLOG_ERROR("Invalid tmp buffer: address = %p, size = %d.\n",
                         context->tmp_buffer, context->tmp_buffer_size);
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x64Fw);
    return kPlErrInvalidParam;
  }

  mbedtls_sha256_context sha256_ctx;

  const int32_t kSha256HashSize = 32;
  uint8_t calculated_hash[kSha256HashSize];
  uint8_t appended_hash[kSha256HashSize];

  // firmware_size must be a multiple of 16.
  if (firmware_size & 0x0000000f) {
    FW_MGR_PL_DLOG_ERROR(
        "`firmware_size` must be a multiple of 16, but it is 0x%x.\n",
        firmware_size);
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x87Fw);
    return kPlErrInvalidParam;
  }

  PlErrCode ret = kPlErrCodeError;

  mbedtls_sha256_init(&sha256_ctx);
  ESF_FW_MGR_HANDLE_MBEDTLS_ERROR(mbedtls_sha256_starts(&sha256_ctx, 0), {
    FW_MGR_PL_DLOG_ERROR("mbedtls_sha256_starts() failed.\n");
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x65Fw);
    ret = kPlErrInternal;
    goto free_sha256_ctx_then_exit;
  });

  uint32_t offset = 0;
  while (firmware_size > 0) {
    uint32_t read_size = context->tmp_buffer_size;
    if (read_size > firmware_size) read_size = firmware_size;

    ret = Esp32FlashOpReadPartitionData(partition, offset, read_size,
                                        context->tmp_buffer);
    if (ret != kPlErrCodeOk) {
      FW_MGR_PL_DLOG_ERROR("Failed to read the firmware.\n");
      FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x66Fw);
      FW_MGR_PL_DLOG_DEBUG("partition: %u, offset: 0x%x, read_size: 0x%x).\n",
                           partition, offset, read_size);
      goto free_sha256_ctx_then_exit;
    }

    ESF_FW_MGR_HANDLE_MBEDTLS_ERROR(
        mbedtls_sha256_update(&sha256_ctx, context->tmp_buffer, read_size), {
          FW_MGR_PL_DLOG_ERROR("mbedtls_sha256_update() failed.\n");
          FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x67Fw);
          ret = kPlErrInternal;
          goto free_sha256_ctx_then_exit;
        });

    offset += read_size;
    firmware_size -= read_size;
  }

  ESF_FW_MGR_HANDLE_MBEDTLS_ERROR(
      mbedtls_sha256_finish(&sha256_ctx, calculated_hash), {
        FW_MGR_PL_DLOG_ERROR("mbedtls_sha256_finish() failed.\n");
        FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x68Fw);
        ret = kPlErrInternal;
        goto free_sha256_ctx_then_exit;
      });

  ret = Esp32FlashOpReadPartitionData(partition, offset, kSha256HashSize,
                                      appended_hash);
  if (ret != kPlErrCodeOk) {
    FW_MGR_PL_DLOG_ERROR(
        "Failed to read the SHA256 hash appended to the firmware.\n");
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x69Fw);
    goto free_sha256_ctx_then_exit;
  }

  int r = memcmp(calculated_hash, appended_hash, kSha256HashSize);
  if (r != 0) {
    FW_MGR_PL_DLOG_CRITICAL(
        "The calculated hash does not match the appended hash.\n");
    FW_MGR_PL_ELOG_CRITICAL(kFwMgrPlElogCriticalId0x62FwInvalidHash);

    ret = kPlErrInvalidValue;
    goto free_sha256_ctx_then_exit;
  }

free_sha256_ctx_then_exit:
  mbedtls_sha256_free(&sha256_ctx);

  return ret;
}

/// @brief Get target Partition
/// @param target_partition [out]
/// @return kOk: success, kError: failed
static PlErrCode GetTargetPartition(FwMgrPlOtaImgBootseq *target_partition) {
  if (target_partition == NULL) {
    FW_MGR_PL_DLOG_ERROR("target_partition is NULL.\n");
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x6aFw);
    return kPlErrInvalidParam;
  }

  FwMgrPlOtaImgBootseq active_partition = OTA_IMG_BOOT_INVALID;

  PlErrCode ret = Esp32FlashOpGetActivePartition(&active_partition);
  if (ret != kPlErrCodeOk) {
    FW_MGR_PL_DLOG_ERROR("Esp32FlashOpGetActivePartition() failed.\n");
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x6bFw);
    return ret;
  }

  switch (active_partition) {
    case OTA_IMG_BOOT_FACTORY:
    case OTA_IMG_BOOT_OTA_1:
      *target_partition = OTA_IMG_BOOT_OTA_0;
      break;

    case OTA_IMG_BOOT_OTA_0:
      *target_partition = OTA_IMG_BOOT_OTA_1;
      break;

    default:
      FW_MGR_PL_DLOG_CRITICAL("Invalid `active_partition`: %u.\n",
                              active_partition);
      FW_MGR_PL_ELOG_CRITICAL(kFwMgrPlElogCriticalId0x80Fw);
      return kPlErrInternal;
  }

  return kPlErrCodeOk;
}

/// @brief Verify that `total_write_size` is a multiple of 32 and in the range
/// of (0, partition size].
/// @param target_partition [in]
/// @param total_write_size [in]
/// @return kOk: Verification success; kInvalid Argument: Verification failed;
/// Internal: Internal Error
static PlErrCode VerifyTotalWriteSize(FwMgrPlOtaImgBootseq target_partition,
                                      uint32_t total_write_size) {
  uint32_t partition_size = 0;

  // Verify total_write_size is a multitude of 32.
  if (total_write_size & 0x0000001f) {
    FW_MGR_PL_DLOG_CRITICAL(
        "`total_write_size` must be a multiple of 32, but it is 0x%x.\n",
        total_write_size);
    FW_MGR_PL_ELOG_CRITICAL(kFwMgrPlElogCriticalId0x63FwInvalidSize);
    return kPlErrInvalidParam;
  }

  PlErrCode ret = Esp32FlashOpGetPartitionSize(target_partition,
                                               &partition_size);
  if (ret != kPlErrCodeOk) {
    FW_MGR_PL_DLOG_ERROR("Esp32FlashOpGetPartitionSize() failed.\n");
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x6cFw);
    return ret;
  }

  // TODO: Decide whether we check the partition_size
  // is not TOO LARGE.
  FW_MGR_PL_DLOG_DEBUG("partition_size = 0x%x\n", partition_size);

  if ((total_write_size == 0) ||
      (total_write_size > partition_size * kEsp32FlashNumOtaPartitions)) {
    FW_MGR_PL_DLOG_CRITICAL(
        "Invalid `total_write_size`. It must be in the range of (0, 0x%x], "
        "but it is 0x%x.\n",
        partition_size * kEsp32FlashNumOtaPartitions, total_write_size);
    FW_MGR_PL_ELOG_CRITICAL(kFwMgrPlElogCriticalId0x81Fw);
    return kPlErrInvalidParam;
  }

  return kPlErrCodeOk;
}

/// @brief Free the context
/// @param context [in/out]
static void FreeContext(FwMgrEsp32FwContext *context) {
  free(context->tmp_buffer);
  free(context);
  s_active_context = NULL;
}

/// @brief
/// @param target_partition [in]
/// @param total_write_size [in]
/// @param context [out]
/// @return kOk: success, kError: failed
/// @note When this function successes, to free `context` is the caller's
/// responsibility.
static PlErrCode AllocateAndInitializeContext(
    FwMgrPlOtaImgBootseq target_partition, uint32_t total_write_size,
    FwMgrEsp32FwContext **context) {
  if (context == NULL) {
    FW_MGR_PL_DLOG_ERROR("`context` is NULL\n");
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x6dFw);
    return kPlErrInvalidParam;
  }
  if (s_active_context != NULL) {
    FW_MGR_PL_DLOG_ERROR("There is an active context.\n");
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x6eFw);
    return kPlErrInvalidState;
  }

  *context = (FwMgrEsp32FwContext *)malloc(sizeof(FwMgrEsp32FwContext));
  if (*context == NULL) {
    FW_MGR_PL_DLOG_CRITICAL("Failed to allocate memory for the context.\n");
    FW_MGR_PL_ELOG_CRITICAL(kFwMgrPlElogCriticalId0x64FwAllocContext);
    return kPlErrMemory;
  }

  PlErrCode ret = kPlErrCodeError;

  (*context)->tmp_buffer_size = 0x8000;  // 32 KB
  (*context)->tmp_buffer =
      (uint8_t *)malloc((*context)->tmp_buffer_size * sizeof(uint8_t));
  if ((*context)->tmp_buffer == NULL) {
    FW_MGR_PL_DLOG_CRITICAL(
        "Failed to allocate memory for the temporary buffer.\n");
    FW_MGR_PL_ELOG_CRITICAL(kFwMgrPlElogCriticalId0x65FwAllocTmpBuffer);
    ret = kPlErrMemory;
    goto err_exit;
  }

  // The context is mainly used in FwMgrEsp32FwWrite(). It receives a binary to
  // be written to the flash memory. The first half of the binary is encrypted
  // for ota_0 and the second half of it is encrypted for ota_1. When the target
  // partition is ota_0, the first half will be written to the flash memory and
  // the second half will be discarded. (When the target partition is ota_1, the
  // first half will be discarded and the second half will be written.)
  // `actual_binary_size` is the size of the binary which will be written to
  // flash memory (when FwMgrEsp32FwWrite() is called.) With
  // FwMgrEsp32FwWrite(), data within the range of `context->starting_offset <=
  // offset < context->ending_offset` will be written to the flash memory
  uint32_t ota_number;
  switch (target_partition) {
    case OTA_IMG_BOOT_OTA_0:
      ota_number = 0;
      break;

    case OTA_IMG_BOOT_OTA_1:
      ota_number = 1;
      break;

    default:
      FW_MGR_PL_DLOG_ERROR("Invalid `target_partition`: %u\n",
                           target_partition);
      FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x6fFw);
      ret = kPlErrInternal;
      goto err_exit;
  }

  (*context)->flash_handle = NULL;

  uint32_t actual_binary_size = total_write_size / kEsp32FlashNumOtaPartitions;
  (*context)->starting_offset = ota_number * actual_binary_size;
  (*context)->ending_offset = (*context)->starting_offset + actual_binary_size;

  (*context)->max_write_size = total_write_size;

  s_active_context = *context;

  return kPlErrCodeOk;

err_exit:
  FreeContext(*context);
  *context = NULL;
  return ret;
}

static void GetCurrentTime(size_t size, char *date) {
  if (date == NULL) {
    FW_MGR_PL_DLOG_WARNING("date is NULL. Does nothing.\n");
    return;
  }

  struct timespec ts;
  struct tm tm_info;

  // Get current time
  clock_gettime(CLOCK_REALTIME, &ts);
  memset(&tm_info, 0, sizeof(tm_info));
  localtime_r(&ts.tv_sec, &tm_info);

  // Get timezone offset
  int offset = tm_info.tm_gmtoff;             // Get offset in seconds
  int offset_hours = offset / 3600;           // Convert to hours
  int offset_minutes = (offset % 3600) / 60;  // Convert to minutes

  // Calculate fractional seconds as an integer
  int fractional_seconds = ts.tv_nsec /
                           1000000;  // Convert nanoseconds to milliseconds

  // Create ISO 8601 format string
  // Common part for both UTC and local time
  snprintf(date, size, "%04d-%02d-%02dT%02d:%02d:%02d.%03d",
           (tm_info.tm_year + 1900) % 10000,  // Year offset from 1900
           (tm_info.tm_mon + 1) % 100,        // Month is 0-11, so +1
           tm_info.tm_mday % 100,             // Day
           tm_info.tm_hour % 100,             // Hour
           tm_info.tm_min % 100,              // Minute
           tm_info.tm_sec % 100,              // Seconds
           fractional_seconds % 1000);        // Milliseconds

  // Add timezone information
  if (offset == 0) {
    // For UTC, use 'Z'
    snprintf(date + strnlen(date, size), size - strnlen(date, size), "Z");
  } else {
    // For other time zones, include offset
    snprintf(date + strnlen(date, size), size - strnlen(date, size),
             "%c%02d:%02d",
             (offset >= 0) ? '+' : '-',  // Sign of timezone offset
             abs(offset_hours),          // Hours of timezone offset
             abs(offset_minutes));       // Minutes of timezone offset
  }

  date[size - 1] = '\0';
}

static int PartitionToHashDateIndex(FwMgrPlOtaImgBootseq target_partition) {
  int index;
  if (target_partition == OTA_IMG_BOOT_OTA_0) {
    index = 0;
  } else {
    index = 1;
  }
  return index;
}

// Internal structures and fuctions for Parameter Storage ----------------------

static bool FirmwareInfoMaskIsEnabled(EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(FirmwareInfoMask,
                                                       firmware_info, mask);
}

/// @brief Get struct info to access data in the parameter storage
/// @param member_info [in] Used as a member of the struct info
/// @param struct_info [out]
/// @note If either of member_info or struct_info is NULL, does nothing
void FwMgrEsp32FwGetStructInfoForFirmwareInfo(
    EsfParameterStorageManagerMemberInfo *member_info,
    EsfParameterStorageManagerStructInfo *struct_info) {
  if (member_info == NULL || struct_info == NULL) {
    FW_MGR_PL_DLOG_WARNING(
        "member_info or struct_info is NULL. Skip the process.\n");
    return;
  }

  member_info->id = kEsfParameterStorageManagerItemFwMgrBinaryInfoMcuFirmware;
  member_info->type = kEsfParameterStorageManagerItemTypeRaw;
  member_info->offset = offsetof(FirmwareInfo, data);
  member_info->size = ESF_PARAMETER_STORAGE_MANAGER_RAW_SIZEOF(FirmwareInfoData,
                                                               info);
  member_info->enabled = FirmwareInfoMaskIsEnabled;
  member_info->custom = NULL;
  struct_info->items_num = 1;
  struct_info->items = member_info;
}

/// @brief Save hash and date in the Parameter Storage
/// @param target_partition [in]
/// @param hash [in] The buffer size should be FW_MGR_PL_HASH_SIZE.
/// @param update_date [in] If NULL, use the current date. If not NULL, The
/// buffer size should be FW_MGR_PL_UPDATE_DATE_SIZE
/// @return
PlErrCode FwMgrEsp32FwSaveHashAndDate(FwMgrPlOtaImgBootseq target_partition,
                                      const uint8_t *hash,
                                      const char *update_date) {
  FirmwareInfo firmware_info = {0};
  HashDate *hash_date;
  hash_date =
      &firmware_info.data.info[PartitionToHashDateIndex(target_partition)];

  EsfParameterStorageManagerMemberInfo member_info = {0};
  EsfParameterStorageManagerStructInfo struct_info = {0};
  FwMgrEsp32FwGetStructInfoForFirmwareInfo(&member_info, &struct_info);

  FirmwareInfoMask mask = {.firmware_info = 1};

  EsfParameterStorageManagerHandle handle;
  EsfParameterStorageManagerStatus pstorage_ret =
      EsfParameterStorageManagerOpen(&handle);
  if (pstorage_ret != kEsfParameterStorageManagerStatusOk) {
    FW_MGR_PL_DLOG_CRITICAL(
        "EsfParameterStorageManagerOpen func failed. ret = %u\n", pstorage_ret);
    FW_MGR_PL_ELOG_CRITICAL(kFwMgrPlElogCriticalId0x82Fw);
    return kPlErrInternal;
  }

  PlErrCode ret = kPlErrInternal;

  pstorage_ret = EsfParameterStorageManagerLoad(
      handle, (EsfParameterStorageManagerMask)&mask,
      (EsfParameterStorageManagerData)&firmware_info, &struct_info, NULL);
  if (pstorage_ret != kEsfParameterStorageManagerStatusOk) {
    FW_MGR_PL_DLOG_CRITICAL(
        "EsfParameterStorageManagerLoad func failed. ret = %u\n", pstorage_ret);
    FW_MGR_PL_ELOG_CRITICAL(kFwMgrPlElogCriticalId0x83Fw);
    ret = kPlErrInternal;
    goto close_pstorage_then_exit;
  }

  if (ESF_PARAMETER_STORAGE_MANAGER_RAW_IS_EMPTY(&firmware_info.data)) {
    HashDate *info;
    for (uint32_t i = 0; i < kEsp32FlashNumOtaPartitions; ++i) {
      info = &firmware_info.data.info[i];
      memset(info->hash, 0, sizeof(info->hash));
      strncpy(info->update_date, "", sizeof(info->update_date));
    }
  }

  memcpy(hash_date->hash, hash, sizeof(hash_date->hash));
  if (update_date) {
    strncpy(hash_date->update_date, update_date,
            sizeof(hash_date->update_date));
  } else {
    GetCurrentTime(sizeof(hash_date->update_date), hash_date->update_date);
  }

  pstorage_ret = EsfParameterStorageManagerSave(
      handle, (EsfParameterStorageManagerMask)&mask,
      (EsfParameterStorageManagerData)&firmware_info, &struct_info, NULL);
  if (pstorage_ret != kEsfParameterStorageManagerStatusOk) {
    FW_MGR_PL_DLOG_CRITICAL(
        "EsfParameterStorageManagerSave func failed. ret = %u\n", pstorage_ret);
    FW_MGR_PL_ELOG_CRITICAL(kFwMgrPlElogCriticalId0x84Fw);
    ret = kPlErrInternal;
    goto close_pstorage_then_exit;
  }

  ret = kPlErrCodeOk;

close_pstorage_then_exit:
  pstorage_ret = EsfParameterStorageManagerClose(handle);
  if (pstorage_ret != kEsfParameterStorageManagerStatusOk) {
    FW_MGR_PL_DLOG_CRITICAL(
        "EsfParameterStorageManagerClose func failed. ret = %u\n",
        pstorage_ret);
    FW_MGR_PL_ELOG_CRITICAL(kFwMgrPlElogCriticalId0x85Fw);
    ret = kPlErrInternal;
  }

  return ret;
}

// Functions shared to public via FwMgrEsp32FwGetOps()

static PlErrCode FwMgrEsp32FwFinalize(void);
static PlErrCode FwMgrEsp32FwOpen(uint32_t total_write_size,
                                  const uint8_t *hash,
                                  FwMgrEsp32ImplHandle *handle,
                                  uint32_t *max_write_size);
static PlErrCode FwMgrEsp32FwClose(FwMgrEsp32ImplHandle handle, bool aborted,
                                   bool switch_partition, bool *updated);
static PlErrCode FwMgrEsp32FwWrite(FwMgrEsp32ImplHandle handle, uint32_t offset,
                                   EsfMemoryManagerHandle buffer_handle,
                                   uint32_t buffer_offset, uint32_t write_size,
                                   uint32_t *written_size);
static PlErrCode FwMgrEsp32FwAbort(FwMgrEsp32ImplHandle handle);

/// @brief Do the finalization process of this block.
///        - Free the context if it has not been freed.
/// @return always returns kOk
static PlErrCode FwMgrEsp32FwFinalize(void) {
  FW_MGR_PL_DLOG_DEBUG("Start.\n");

  if (s_active_context != NULL) {
    PlErrCode ret = FwMgrEsp32FwClose((FwMgrEsp32ImplHandle)s_active_context,
                                      true, true, NULL);
    if (ret != kPlErrCodeOk) {
      FW_MGR_PL_DLOG_ERROR("FwMgrEsp32FwClose() failed. ret = %u\n", ret);
      FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x70Fw);
      return ret;
    }
  }

  return kPlErrCodeOk;
}

/// @brief (1) Create the context and hand it as a handle to the caller.
///        (2) Check which partition (ota_0 or ota_1) is currently running
///        firmware stored, register the other partition as the "target
///        partition". (A new firmware will be written to the target partition
///        when the `FwMgrEsp32FwWrite` is called after this function is
///        called.)
///        (3) Delete the data written in the target_partition.
/// @param total_write_size [in] Must be a multiple of 32. (ESP32 flash
///        encryption is performed for each of 16-byte block. So the size of an
///        encrypted firmware is a multiple of 16. FwMgrEsp32FwWrite() will
///        receive a binary which includes a firmware encrypted for ota_0 and
///        one for ota_1. So the total size must be a multiple of 32.)
/// @param hash [in] Must be a 32-byte array.
/// @param handle [out]
/// @param max_write_size [out]
/// @return kOk: success, otherwise: failed.
static PlErrCode FwMgrEsp32FwOpen(uint32_t total_write_size,
                                  const uint8_t *hash,
                                  FwMgrEsp32ImplHandle *handle,
                                  uint32_t *max_write_size) {
  FW_MGR_PL_DLOG_DEBUG("Start.\n");

  if (handle == NULL) {
    FW_MGR_PL_DLOG_ERROR("`handle` is NULL.\n");
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x71Fw);
    return kPlErrInvalidParam;  // Invalid Argument
  }

  if (max_write_size == NULL) {
    FW_MGR_PL_DLOG_ERROR("`max_write_size` is NULL\n");
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x72Fw);
    return kPlErrInvalidParam;
  }

  FwMgrPlOtaImgBootseq target_partition;
  PlErrCode ret = GetTargetPartition(&target_partition);
  if (ret != kPlErrCodeOk) {
    FW_MGR_PL_DLOG_ERROR("GetTargetPartition() failed. ret = %u\n", ret);
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x73Fw);
    return ret;
  }

  ret = VerifyTotalWriteSize(target_partition, total_write_size);
  if (ret != kPlErrCodeOk) {
    FW_MGR_PL_DLOG_ERROR("VerifyTotalWriteSize() failed. ret = %u\n", ret);
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x74Fw);
    return ret;
  }

  FwMgrEsp32FwContext *context;
  ret = AllocateAndInitializeContext(target_partition, total_write_size,
                                     &context);
  if (ret != kPlErrCodeOk) {
    FW_MGR_PL_DLOG_ERROR("AllocateAndInitializeContext() failed. ret = %u\n",
                         ret);
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x75Fw);
    return ret;
  }

  // Set flag of the target partition as invalid.
  ret = Esp32FlashOpInvalidatePartition(target_partition);
  if (ret != kPlErrCodeOk) {
    FW_MGR_PL_DLOG_ERROR(
        "Failed to set the flag of the target partition(%u) as invalid. ret = "
        "%u\n",
        target_partition, ret);
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x76Fw);
    goto err_exit;
  }

  // Erase data of the target partition.
  ret = Esp32FlashOpErasePartitionData(target_partition);
  if (ret != kPlErrCodeOk) {
    FW_MGR_PL_DLOG_ERROR(
        "Esp32FlashOpErasePartitionData(%u) failed. ret = %u\n",
        target_partition, ret);
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x77Fw);
    goto err_exit;
  }

  // Save hash and date in the parameter storage
  ret = FwMgrEsp32FwSaveHashAndDate(target_partition, hash, NULL);
  if (ret != kPlErrCodeOk) {
    FW_MGR_PL_DLOG_ERROR("FwMgrEsp32FwSaveHashAndDate failed. ret = %u\n", ret);
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x78Fw);
    goto err_exit;
  }

  // Open the ota partition
  ret = Esp32FlashOpOpenPartition(target_partition, &context->flash_handle);
  if (ret != kPlErrCodeOk) {
    FW_MGR_PL_DLOG_ERROR("Esp32FlashOpOpenPartition(%u) failed. ret = %u\n",
                         target_partition, ret);
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x79Fw);
    goto err_exit;
  }

  *max_write_size = context->max_write_size;
  *handle = (FwMgrEsp32ImplHandle)context;

  return kPlErrCodeOk;

err_exit:
  FreeContext(context);
  return ret;
}

/// @brief (1) [if not aborted] Verify the firmware written in the "target
///        partition".
///        (2) [If not aborted] Set "valid flag" of the target partition.
///        (3) Free the context.
/// @param handle [in]
/// @param aborted [in]
/// @param switch_partition [in]
/// @param updated [out] can be NULL
/// @return kOk: success, otherwise: failed
static PlErrCode FwMgrEsp32FwClose(FwMgrEsp32ImplHandle handle, bool aborted,
                                   bool switch_partition, bool *updated) {
  FW_MGR_PL_DLOG_DEBUG("Start.\n");

  if (updated) *updated = false;

  if (!FW_MGR_PL_VERIFY_HANDLE(handle)) {
    FW_MGR_PL_DLOG_ERROR("`handle` is invalid.\n");
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x7aFw);
    return kPlErrInvalidParam;
  }

  FwMgrEsp32FwContext *context = (FwMgrEsp32FwContext *)handle;

  PlErrCode ret;

  // Close the ota partition
  if (context->flash_handle != NULL) {
    ret = Esp32FlashOpClosePartition(context->flash_handle);
    if (ret != kPlErrCodeOk) {
      FW_MGR_PL_DLOG_ERROR("Esp32FlashOpClosePartition() failed. ret = %u\n",
                           ret);
      FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x7bFw);
      return ret;
    }
    context->flash_handle = NULL;
  }

  if (!aborted) {
    FwMgrPlOtaImgBootseq target_partition;
    ret = GetTargetPartition(&target_partition);
    if (ret != kPlErrCodeOk) {
      FW_MGR_PL_DLOG_ERROR("GetTargetPartition() failed. ret = %u\n", ret);
      FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x7cFw);
      return ret;
    }

    Esp32FirmwareInfo firmware_info = {.hash_appended = false,
                                       .firmware_size = 0};
    ret = ReadAndVerifyFirmwareHeader(target_partition, &firmware_info);
    if (ret != kPlErrCodeOk) {
      FW_MGR_PL_DLOG_ERROR("ReadAndVerifyFirmwareHeader(%u) failed. ret = %u\n",
                           target_partition, ret);
      FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x7dFw);
      return ret;
    }

    FW_MGR_PL_DLOG_INFO("Firmware size = 0x%x.\n", firmware_info.firmware_size);

    if (firmware_info.hash_appended) {
      ret = VerifyFirmwareHash(context, target_partition,
                               firmware_info.firmware_size);
      if (ret != kPlErrCodeOk) {
        FW_MGR_PL_DLOG_ERROR("VerifyFirmwareHash(%u) failed. ret = %u\n",
                             target_partition, ret);
        FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x7eFw);
        return ret;
      }
    }

    if (switch_partition) {
      ret = Esp32FlashOpSetNextBootPartition(target_partition);
      if (ret != kPlErrCodeOk) {
        FW_MGR_PL_DLOG_ERROR(
            "Esp32FlashOpSetNextBootPartition(%u) failed. ret = %u\n",
            target_partition, ret);
        FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x7fFw);
        return ret;
      }
    } else {
      FW_MGR_PL_DLOG_INFO("Skip switching partition\n");
      FW_MGR_PL_ELOG_INFO(kFwMgrPlElogInfoId0x60FwSkipSwitchPartition);
    }

    if (updated) *updated = true;
  }

  FreeContext(context);

  return kPlErrCodeOk;
}

/// @brief Receive a binary ([A] and [B] combined), write either [A] or [B] to
/// the flash memory and discard the other.
/// - [A]: the firmware encrypted for ota_0 and
/// - [B]: the same firmware as [A] but encrypted for ota_1.
/// @param handle [in]
/// @param offset [in]
/// @param buffer_handle [in] The buffer where (a part of) the binary is stored.
/// @param buffer_offset [in] The buffer where (a part of) the binary is stored.
/// @param write_size [in] The size of data in the buffer.
/// @param written_size [out] The size of the written data + the size of the
/// discarded data. (The discard data is included because it is a part of
/// "properly processed" data.)
/// @return kOK: success, otherwise: failed
static PlErrCode FwMgrEsp32FwWrite(FwMgrEsp32ImplHandle handle, uint32_t offset,
                                   EsfMemoryManagerHandle buffer_handle,
                                   uint32_t buffer_offset, uint32_t write_size,
                                   uint32_t *written_size) {
  FW_MGR_PL_DLOG_DEBUG("Start.\n");

  if (!FW_MGR_PL_VERIFY_HANDLE(handle)) {
    FW_MGR_PL_DLOG_ERROR("`handle` is invalid.\n");
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x80Fw);
    return kPlErrInvalidParam;
  }

  if (written_size == NULL) {
    FW_MGR_PL_DLOG_ERROR("`written_size` is NULL.\n");
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x81Fw);
    return kPlErrInvalidParam;
  }

  FwMgrEsp32FwContext *context = (FwMgrEsp32FwContext *)handle;

  if ((write_size == 0) || (write_size > context->max_write_size)) {
    FW_MGR_PL_DLOG_CRITICAL(
        "`write_size` must be in the range of (0, 0x%x], but it is 0x%x.\n",
        context->max_write_size, write_size);
    FW_MGR_PL_ELOG_CRITICAL(kFwMgrPlElogCriticalId0x64InvalidSize);
    return kPlErrInvalidParam;
  }

  *written_size = 0;

  // The size of data to be discarded ahead of the data to be written.
  // Can be non-zero only when target partition is ota_1.
  uint32_t discarded_size_0 = 0;
  // The size of data to be discarded behind the data to be written.
  // Can be non-zero only when target partition is ota_0.
  uint32_t discarded_size_1 = 0;

  // With below calculation, discarded_data_size_# can be larger than
  // write_size. But, it is no problem, because writing to the flash memory will
  // be skipped in that case.
  if (offset < context->starting_offset)
    discarded_size_0 = context->starting_offset - offset;

  if (context->ending_offset < offset + write_size)
    discarded_size_1 = offset + write_size - context->ending_offset;

  if (write_size <= discarded_size_0 + discarded_size_1) {
    *written_size = write_size;
    return kPlErrCodeOk;
  }

  FW_MGR_PL_DLOG_DEBUG("offset = 0x%x\n", offset);

  write_size -= discarded_size_0 + discarded_size_1;
  buffer_offset += discarded_size_0;
  FW_MGR_PL_DLOG_DEBUG(
      "write_size = 0x%x, offset = 0x%x, starting_offset = 0x%x, "
      "ending_offset = 0x%x\n",
      write_size, offset, context->starting_offset, context->ending_offset);

  uint32_t actual_written_size;
  PlErrCode ret = FwMgrEsp32FwImplWrite(context, write_size, buffer_handle,
                                        buffer_offset, &actual_written_size);
  if (ret != kPlErrCodeOk) {
    FW_MGR_PL_DLOG_ERROR("FwMgrEsp32FwImplWrite() failed.\n");
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x82Fw);
    return ret;
  }

  *written_size = discarded_size_0 + actual_written_size;
  if (actual_written_size == write_size) *written_size += discarded_size_1;

  return kPlErrCodeOk;
}

/// @brief Do nothing
/// @param handle
/// @return
static PlErrCode FwMgrEsp32FwAbort(FwMgrEsp32ImplHandle handle) {
  FW_MGR_PL_DLOG_DEBUG("Start.\n");

  if (!FW_MGR_PL_VERIFY_HANDLE(handle)) {
    FW_MGR_PL_DLOG_ERROR("`handle` is invalid.\n");
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x83Fw);
    return kPlErrInvalidParam;
  }

  return kPlErrCodeOk;
}

// Public Functions ---------------------------------------------------------

PlErrCode FwMgrEsp32FwGetOps(FwMgrEsp32ImplOps *ops) {
  FW_MGR_PL_DLOG_DEBUG("Start.\n");

  if (ops == NULL) {
    FW_MGR_PL_DLOG_ERROR("`ops` is NULL.\n");
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x84Fw);
    return kPlErrInvalidParam;
  }

  ops->finalize = FwMgrEsp32FwFinalize;
  ops->open = FwMgrEsp32FwOpen;
  ops->close = FwMgrEsp32FwClose;
  ops->write = FwMgrEsp32FwWrite;
  ops->abort = FwMgrEsp32FwAbort;

  return kPlErrCodeOk;
}

PlErrCode FwMgrEsp32FwGetHashAndUpdateDate(int32_t hash_size, uint8_t *hash,
                                           int32_t update_date_size,
                                           char *update_date) {
  if (hash == NULL || update_date == NULL) {
    FW_MGR_PL_DLOG_ERROR("hash (%p) or update_date (%p) is NULL.\n", hash,
                         update_date);
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x85Fw);
    return kPlErrInvalidParam;
  }

  if (hash_size > FW_MGR_PL_HASH_SIZE) hash_size = FW_MGR_PL_HASH_SIZE;
  if (update_date_size > FW_MGR_PL_UPDATE_DATE_SIZE)
    update_date_size = FW_MGR_PL_UPDATE_DATE_SIZE;

  FwMgrPlOtaImgBootseq active_partition;
  PlErrCode ret = Esp32FlashOpGetActivePartition(&active_partition);
  if (ret != kPlErrCodeOk) {
    FW_MGR_PL_DLOG_ERROR("Esp32FlashOpGetActivePartition failed.\n ret = %u.\n",
                         ret);
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x86Fw);
    return ret;
  }

  FirmwareInfo firmware_info = {0};
  HashDate *hash_date;
  hash_date =
      &firmware_info.data.info[PartitionToHashDateIndex(active_partition)];

  EsfParameterStorageManagerMemberInfo member_info = {0};
  EsfParameterStorageManagerStructInfo struct_info = {0};
  FwMgrEsp32FwGetStructInfoForFirmwareInfo(&member_info, &struct_info);

  FirmwareInfoMask mask = {.firmware_info = 1};

  EsfParameterStorageManagerHandle handle;
  EsfParameterStorageManagerStatus pstorage_ret =
      EsfParameterStorageManagerOpen(&handle);
  if (pstorage_ret != kEsfParameterStorageManagerStatusOk) {
    FW_MGR_PL_DLOG_CRITICAL(
        "EsfParameterStorageManagerOpen func failed. ret = %u\n", pstorage_ret);
    FW_MGR_PL_ELOG_CRITICAL(kFwMgrPlElogCriticalId0x86Fw);
    return kPlErrInternal;
  }

  pstorage_ret = EsfParameterStorageManagerLoad(
      handle, (EsfParameterStorageManagerMask)&mask,
      (EsfParameterStorageManagerData)&firmware_info, &struct_info, NULL);
  if (pstorage_ret != kEsfParameterStorageManagerStatusOk) {
    FW_MGR_PL_DLOG_CRITICAL(
        "EsfParameterStorageManagerLoad func failed. ret = %u\n", pstorage_ret);
    FW_MGR_PL_ELOG_CRITICAL(kFwMgrPlElogCriticalId0x87Fw);
    ret = kPlErrInternal;
    goto close_pstorage_then_exit;
  }

  if (ESF_PARAMETER_STORAGE_MANAGER_RAW_IS_EMPTY(&firmware_info.data)) {
    memset(hash_date->hash, 0, sizeof(hash_date->hash));
    strncpy(hash_date->update_date, "", sizeof(hash_date->update_date));
  }

  if (hash_size > 0) memcpy(hash, hash_date->hash, hash_size);
  if (update_date_size > 0) {
    strncpy(update_date, hash_date->update_date, update_date_size);
    update_date[update_date_size - 1] = '\0';
  }

  ret = kPlErrCodeOk;

close_pstorage_then_exit:
  pstorage_ret = EsfParameterStorageManagerClose(handle);
  if (pstorage_ret != kEsfParameterStorageManagerStatusOk) {
    FW_MGR_PL_DLOG_CRITICAL(
        "EsfParameterStorageManagerClose func failed. ret = %u\n",
        pstorage_ret);
    FW_MGR_PL_ELOG_CRITICAL(kFwMgrPlElogCriticalId0x88Fw);
    ret = kPlErrInternal;
  }

  return ret;
}

// Return nothing because failure of the migration should not stop the operation
// of the FW manager.
void FwMgrEsp32FwMigrateFromV1(void) {
  // Migration from v1 is only for T3P
#ifdef CONFIG_EXTERNAL_TARGET_T3P
  PlErrCode ret = FwMgrEsp32FwImplMigrateUpdateDateFromV1();
  if (ret != kPlErrCodeOk) {
    FW_MGR_PL_DLOG_ERROR(
        "FwMgrEsp32FwImplMigrateUpdateDateFromV1 failed. ret = %u\n", ret);
    FW_MGR_PL_ELOG_ERROR(kFwMgrPlElogErrorId0x88Fw);
  }
#endif
}
