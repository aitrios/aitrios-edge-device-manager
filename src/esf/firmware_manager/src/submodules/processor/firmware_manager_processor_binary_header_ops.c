/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "firmware_manager_processor_binary_header_ops.h"

#include "firmware_manager_common.h"
#include "firmware_manager_log.h"
#include "firmware_manager_processor_binary_header.h"
#include "mbedtls/sha256.h"

#ifdef CONFIG_EXTERNAL_FIRMWARE_MANAGER_USE_MBEDTLS_V2
#define ESF_FW_MGR_HANDLE_MBEDTLS_ERROR(operation, error_handling) operation
#else
#define ESF_FW_MGR_HANDLE_MBEDTLS_ERROR(operation, error_handling) \
  do {                                                             \
    int r = operation;                                             \
    if (r != 0) error_handling;                                    \
  } while (0)
#endif

typedef struct MemoryInfo {
  EsfMemoryManagerHandle handle;
  bool map_supported;
  void *mapped_address;
} MemoryInfo;

static EsfFwMgrSwArchVersion SwArchVersionInt2Enum(uint8_t version) {
  switch (version) {
    case 1:
      return kEsfFwMgrSwArchVersion1;
    case 2:
      return kEsfFwMgrSwArchVersion2;
    default:
      break;
  }

  return kEsfFwMgrSwArchVersionUnknown;
}

static int SwArchVersionEnum2Int(EsfFwMgrSwArchVersion version) {
  switch (version) {
    case kEsfFwMgrSwArchVersion1:
      return 1;
    case kEsfFwMgrSwArchVersion2:
      return 2;
    default:
      break;
  }

  return -1;
}

static EsfFwMgrResult VerifyHeaderMagic(const uint8_t *header_magic) {
  if (header_magic == NULL) {
    ESF_FW_MGR_DLOG_ERROR("header_magic is NULL\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x92Processor);
    return kEsfFwMgrResultInvalidArgument;
  }

  if (memcmp(kEsfFwMgrProcessorBinaryHeaderMagic, header_magic,
             sizeof(kEsfFwMgrProcessorBinaryHeaderMagic)) != 0) {
    ESF_FW_MGR_DLOG_CRITICAL("Header magic is invalid.\n");
    ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0x8dProcessor);
    return kEsfFwMgrResultInvalidData;
  }
  return kEsfFwMgrResultOk;
}

static EsfFwMgrResult VerifyHeaderHash(
    const EsfFwMgrProcessorBinaryHeader *header) {
  if (header == NULL) {
    ESF_FW_MGR_DLOG_ERROR("header is NULL\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x9cProcessor);
    return kEsfFwMgrResultInvalidArgument;
  }

  mbedtls_sha256_context sha256_context;
  uint8_t calculated_hash[sizeof(header->hash)];

  EsfFwMgrResult ret = kEsfFwMgrResultInternal;

  mbedtls_sha256_init(&sha256_context);
  ESF_FW_MGR_HANDLE_MBEDTLS_ERROR(mbedtls_sha256_starts(&sha256_context, 0), {
    ESF_FW_MGR_DLOG_ERROR("mbedtls_sha256_starts failed. ret = %d\n", r);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x93Processor);
    ret = kEsfFwMgrResultInternal;
    goto free_sha256_context_then_exit;
  });
  ESF_FW_MGR_HANDLE_MBEDTLS_ERROR(
      mbedtls_sha256_update(&sha256_context, (uint8_t *)header,
                            sizeof(EsfFwMgrProcessorBinaryHeader) -
                                sizeof(header->hash)),
      {
        ESF_FW_MGR_DLOG_ERROR("mbedtls_sha256_update failed. ret = %d\n", r);
        ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x94Processor);
        ret = kEsfFwMgrResultInternal;
        goto free_sha256_context_then_exit;
      });
  ESF_FW_MGR_HANDLE_MBEDTLS_ERROR(
      mbedtls_sha256_finish(&sha256_context, calculated_hash), {
        ESF_FW_MGR_DLOG_ERROR("mbedtls_sha256_finish failed. ret = %d\n", r);
        ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x95Processor);
        ret = kEsfFwMgrResultInternal;
        goto free_sha256_context_then_exit;
      });

  if (0 != (memcmp(header->hash, calculated_hash, sizeof(header->hash)))) {
    ESF_FW_MGR_DLOG_CRITICAL("Hash verification failed.\n");
    ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0x9cProcessor);
    ret = kEsfFwMgrResultInvalidData;
    goto free_sha256_context_then_exit;
  }

  ret = kEsfFwMgrResultOk;

free_sha256_context_then_exit:
  mbedtls_sha256_free(&sha256_context);
  return ret;
}

static EsfFwMgrResult VerifyHeaderInfo(
    const EsfFwMgrProcessorBinaryHeaderInfo *info) {
  if (info == NULL) {
    ESF_FW_MGR_DLOG_ERROR("info is NULL.\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x9dProcessor);
    return kEsfFwMgrResultInvalidArgument;
  }

  if (info->sw_arch_version == kEsfFwMgrSwArchVersionUnknown) {
    ESF_FW_MGR_DLOG_CRITICAL("Invalid SW Arch\n");
    ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0x8eProcessor);
    return kEsfFwMgrResultInvalidData;
  }

  return kEsfFwMgrResultOk;
}

/// @brief
/// @param src_info [in]
/// @param src_offset [in]
/// @param src_size [in]
/// @param size [in]
/// @param loaded_size [in/out]
/// @param dest [in/out]
/// @return
static EsfFwMgrResult CopyFromLargeHeapMemory(
    const MemoryInfo *src_info, int32_t src_offset, int32_t src_size,
    int32_t size, EsfFwMgrProcessorBinaryHeaderLoadInfo *load_info,
    int32_t *loaded_size) {
  uint8_t *dest = (uint8_t *)load_info->buffer + load_info->total_loaded_size;
  int32_t load_size = size - load_info->total_loaded_size;
  if (load_size > src_size) load_size = src_size;

  if (src_info->map_supported) {
    memcpy(dest, (uint8_t *)src_info->mapped_address + src_offset, load_size);
  } else {
    off_t result_offset = 0;
    EsfMemoryManagerResult mem_ret = EsfMemoryManagerFseek(
        src_info->handle, src_offset, SEEK_SET, &result_offset);
    if ((mem_ret != kEsfMemoryManagerResultSuccess) ||
        (src_offset != result_offset)) {
      ESF_FW_MGR_DLOG_CRITICAL(
          "EsfMemoryManagerFseek failed. ret = %u, requested offset = %d, "
          "result offset = %ld.\n",
          mem_ret, src_offset, result_offset);
      ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0x9dProcessor);
      return kEsfFwMgrResultUnavailable;
    }

    size_t rsize = 0;
    mem_ret = EsfMemoryManagerFread(src_info->handle, dest, load_size, &rsize);
    if ((mem_ret != kEsfMemoryManagerResultSuccess) ||
        ((size_t)load_size != rsize)) {
      ESF_FW_MGR_DLOG_CRITICAL(
          "EsfMemoryManagerFwrite failed. ret = %u, requested read size = %d, "
          "actually read size = %lu\n",
          mem_ret, load_size, rsize);
      ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0x9eProcessor);
      return kEsfFwMgrResultUnavailable;
    }
  }

  load_info->total_loaded_size += load_size;
  if (loaded_size != NULL) *loaded_size = load_size;

  return kEsfFwMgrResultOk;
}

static bool IsValidVersion(uint8_t version) { return (version == 1); }

void EsfFwMgrProcessorInitBinaryHeaderLoadInfo(
    EsfFwMgrProcessorBinaryHeaderLoadInfo *info) {
  if (info == NULL) return;

  info->buffer = NULL;
  info->completed = false;
  info->magic_loaded = false;
  info->total_loaded_size = 0;
}

void EsfFwMgrProcessorDeinitBinaryHeaderLoadInfo(
    EsfFwMgrProcessorBinaryHeaderLoadInfo *info) {
  if (info == NULL) return;

  free(info->buffer);
  info->buffer = NULL;
}

bool EsfFwMgrProcessorIsBinaryHeaderMagicLoaded(
    const EsfFwMgrProcessorBinaryHeaderLoadInfo *load_info) {
  return ((load_info != NULL) && (load_info->magic_loaded));
}

bool EsfFwMgrProcessorIsBinaryHeaderLoaded(
    const EsfFwMgrProcessorBinaryHeaderLoadInfo *load_info) {
  return ((load_info != NULL) && (load_info->completed));
}

EsfFwMgrResult EsfFwMgrProcessorLoadBinaryHeader(
    const EsfFwMgrProcessorLoadBinaryHeaderRequest *request,
    EsfFwMgrProcessorBinaryHeaderLoadInfo *load_info,
    EsfFwMgrProcessorBinaryHeaderInfo *info) {
  if (request == NULL || load_info == NULL || info == NULL) {
    ESF_FW_MGR_DLOG_ERROR("request, load_info or info is NULL.\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x96Processor);
    return kEsfFwMgrResultInvalidArgument;
  }

  if (load_info->buffer == NULL) {
    load_info->buffer = malloc(sizeof(EsfFwMgrProcessorBinaryHeader));
    if (load_info->buffer == NULL) {
      ESF_FW_MGR_DLOG_CRITICAL(
          "Failed to allocate memory for the header buffer\n");
      ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0x9fProcessor);
      return kEsfFwMgrResultResourceExhausted;
    }
  }

  EsfMemoryManagerMapSupport supported;
  EsfMemoryManagerResult mem_ret =
      EsfMemoryManagerIsMapSupport(request->src_buffer_handle, &supported);
  if (mem_ret != kEsfMemoryManagerResultSuccess) {
    ESF_FW_MGR_DLOG_CRITICAL("EsfMemoryManagerIsMapSupport failed. ret = %u\n",
                             mem_ret);
    ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0x8fProcessor);
    return kEsfFwMgrResultUnavailable;
  }

  MemoryInfo memory_info = {
      .handle = request->src_buffer_handle,
      .map_supported = (supported == kEsfMemoryManagerMapIsSupport),
  };
  EsfFwMgrResult ret = EsfFwMgrMapOrOpenLargeHeapMemory(
      request->src_buffer_handle, request->src_offset + request->src_size,
      memory_info.map_supported, &memory_info.mapped_address);
  if (ret != kEsfFwMgrResultOk) {
    ESF_FW_MGR_DLOG_ERROR("EsfFwMgrMapOrOpenLargeHeapMemory failed. ret = %u\n",
                          ret);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x97Processor);
    return ret;
  }

  int32_t src_offset = request->src_offset;
  int32_t src_size = request->src_size;

  // Load the header magic ###############################################
  if (!load_info->magic_loaded) {
    int32_t loaded_size;
    ret = CopyFromLargeHeapMemory(&memory_info, src_offset, src_size,
                                  ESF_FW_MGR_PROCESSOR_BINARY_HEADER_MAGIC_SIZE,
                                  load_info, &loaded_size);
    if (ret != kEsfFwMgrResultOk) {
      ESF_FW_MGR_DLOG_ERROR(
          "EsfFwMgrMapOrOpenLargeHeapMemory failed. ret = %u\n", ret);
      ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x98Processor);
      goto unmap_or_close_data;
    }

    load_info->magic_loaded = (load_info->total_loaded_size ==
                               ESF_FW_MGR_PROCESSOR_BINARY_HEADER_MAGIC_SIZE);

    if (!load_info->magic_loaded) {
      // The loading of the header magic is not yet complete.
      // More data is needed to continue.
      ret = kEsfFwMgrResultOk;
      goto unmap_or_close_data;
    }

    // Verify header magic
    ret = VerifyHeaderMagic(load_info->buffer);
    if (ret != kEsfFwMgrResultOk) {
      ESF_FW_MGR_DLOG_INFO("VerifyHeaderMagic failed. ret = %u\n", ret);
      // ESF_FW_MGR_ELOG_INFO();

      if (ret == kEsfFwMgrResultInvalidData) {
        // Invalid data means header is not appended
        load_info->completed = true;
        free(load_info->buffer);
        load_info->buffer = NULL;
        info->header_size = 0;
        info->sw_arch_version = kEsfFwMgrSwArchVersionUnknown;
        info->device_type[0] = '\0';
        info->device_variant[0] = '\0';
        ret = kEsfFwMgrResultOk;
      }
      goto unmap_or_close_data;
    }

    src_offset += loaded_size;
    src_size -= loaded_size;
  }  // if (!load_info->magic_loaded)

  // Load the rest of the header ##########################################

  const int32_t kHeaderSize = sizeof(EsfFwMgrProcessorBinaryHeader);
  EsfFwMgrProcessorBinaryHeader *header =
      (EsfFwMgrProcessorBinaryHeader *)load_info->buffer;

  ret = CopyFromLargeHeapMemory(&memory_info, src_offset, src_size, kHeaderSize,
                                load_info, NULL);
  if (ret != kEsfFwMgrResultOk) {
    ESF_FW_MGR_DLOG_ERROR("EsfFwMgrMapOrOpenLargeHeapMemory failed. ret = %u\n",
                          ret);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x99Processor);
    goto unmap_or_close_data;
  }

  if (load_info->total_loaded_size < kHeaderSize) {
    // The loading of the header is not yet complete.
    // More data is needed to continue.
    ret = kEsfFwMgrResultOk;
    goto unmap_or_close_data;
  }

  ret = VerifyHeaderHash(header);
  if (ret != kEsfFwMgrResultOk) {
    ESF_FW_MGR_DLOG_ERROR("VerifyHeaderHash failed. ret = %u\n", ret);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x9aProcessor);
    goto unmap_or_close_data;
  }

  load_info->completed = true;
  if (IsValidVersion(header->header_version)) {
    info->header_size = header->header_size;
    info->sw_arch_version = SwArchVersionInt2Enum(header->sw_arch_version);

    memcpy(info->device_type, header->device_type, sizeof(header->device_type));
    info->device_type[sizeof(info->device_type) - 1] = '\0';
    memcpy(info->device_variant, header->device_variant,
           sizeof(header->device_variant));
    info->device_variant[sizeof(info->device_type) - 1] = '\0';

  } else {
    ESF_FW_MGR_DLOG_CRITICAL("Invalid header version: %u\n",
                             header->header_version);
    ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0x8cProcessor);
    ret = kEsfFwMgrResultInvalidData;
    goto unmap_or_close_data;
  }

  ESF_FW_MGR_DLOG_INFO("Header loaded.\n");
  ESF_FW_MGR_DLOG_INFO("  header size: %u\n", info->header_size);
  ESF_FW_MGR_DLOG_INFO("  SW arch version: %d\n",
                       SwArchVersionEnum2Int(info->sw_arch_version));
  ESF_FW_MGR_DLOG_INFO("  Device type: %s\n", info->device_type);
  ESF_FW_MGR_DLOG_INFO("  Device variant: %s\n", info->device_variant);

  ret = VerifyHeaderInfo(info);
  if (ret != kEsfFwMgrResultOk) {
    ESF_FW_MGR_DLOG_ERROR("VerifyHeaderInfo failed. ret = %u\n", ret);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x9eProcessor);
    goto unmap_or_close_data;
  }

  ret = kEsfFwMgrResultOk;

unmap_or_close_data:
  if (load_info->completed) {
    free(load_info->buffer);
    load_info->buffer = NULL;
    header = NULL;
  }

  // Use a new variable for the return value because `ret` should not be
  // overwritten.
  EsfFwMgrResult ret_unmap = EsfFwMgrUnmapOrCloseLargeHeapMemory(
      request->src_buffer_handle, memory_info.map_supported);
  if (ret_unmap != kEsfFwMgrResultOk) {
    ESF_FW_MGR_DLOG_ERROR(
        "EsfFwMgrUnmapOrCloseLargeHeapMemory failed. ret = %u\n", ret);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x9bProcessor);
    return ret_unmap;
  }

  return ret;
}
