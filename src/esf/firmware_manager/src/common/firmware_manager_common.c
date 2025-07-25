/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "firmware_manager_common.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "firmware_manager_log.h"

// Public functions ############################################################

/// @brief Map or Open the large heap memory.
/// @param mem_handle [in] The handle for the large heap memory which has
/// already been allocated.
/// @param size [in]
/// @param map_supported [in] if true, map the memory and if false, open the
/// memory.
/// @param mapped_address [out] Valid only when map_supported is true.
/// @return kEsfFwMgrResultOk on success.
EsfFwMgrResult EsfFwMgrMapOrOpenLargeHeapMemory(
    EsfMemoryManagerHandle mem_handle, int32_t size, bool map_supported,
    void **mapped_address) {
  if (mapped_address == NULL) {
    ESF_FW_MGR_DLOG_ERROR(" mapped_address is NULL");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0xe0Common);
    return kEsfFwMgrResultInvalidArgument;
  }

  if (map_supported) {
    ESF_FW_MGR_DLOG_INFO("Map large heap memory.\n");

    EsfMemoryManagerResult mem_ret = EsfMemoryManagerMap(mem_handle, NULL, size,
                                                         mapped_address);
    if (mem_ret != kEsfMemoryManagerResultSuccess || *mapped_address == NULL) {
      ESF_FW_MGR_DLOG_CRITICAL("EsfMemoryManagerMap failed. ret = %u\n",
                               mem_ret);
      ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0xe0Common);
      if (mem_ret == kEsfMemoryManagerResultMapError) {
        return kEsfFwMgrResultResourceExhausted;
      } else {
        return kEsfFwMgrResultInternal;
      }
    }

  } else {
    ESF_FW_MGR_DLOG_INFO("Open large heap memory.\n");

    EsfMemoryManagerResult mem_ret = EsfMemoryManagerFopen(mem_handle);
    if (mem_ret != kEsfMemoryManagerResultSuccess) {
      ESF_FW_MGR_DLOG_CRITICAL("EsfMemoryManagerFopen failed. ret = %u\n",
                               mem_ret);
      ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0xe1Common);
      if (mem_ret == kEsfMemoryManagerResultFileIoError) {
        return kEsfFwMgrResultResourceExhausted;
      } else {
        return kEsfFwMgrResultInternal;
      }
    }
  }

  return kEsfFwMgrResultOk;
}

/// @brief Unmap or close
/// @param mem_handle [in]
/// @param map_supported [in] if true, unmap the memory; if false, close the
/// memory.
/// @return kEsfFwMgrResultOk on success.
EsfFwMgrResult EsfFwMgrUnmapOrCloseLargeHeapMemory(
    EsfMemoryManagerHandle mem_handle, bool map_supported) {
  if (map_supported) {
    ESF_FW_MGR_DLOG_INFO("Unmap large heap memory.\n");

    EsfMemoryManagerResult mem_ret = EsfMemoryManagerUnmap(mem_handle, NULL);
    if (mem_ret != kEsfMemoryManagerResultSuccess) {
      ESF_FW_MGR_DLOG_CRITICAL("EsfMemoryManagerUnmap failed. ret = %u.\n",
                               mem_ret);
      ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0xe2Common);

      // The error from EsfMemoryManagerUnmap indicates either an invalid
      // argument or an attempt to unmap memory that hasn't been mapped, both
      // of which are implementation bugs of the FW Manager. Therefore, this
      // function returns an internal error.
      return kEsfFwMgrResultInternal;
    }

  } else {
    ESF_FW_MGR_DLOG_INFO("Close large heap memory.\n");

    EsfMemoryManagerResult mem_ret = EsfMemoryManagerFclose(mem_handle);
    if (mem_ret != kEsfMemoryManagerResultSuccess) {
      ESF_FW_MGR_DLOG_CRITICAL("EsfMemoryManagerFclose failed. ret = %u\n",
                               mem_ret);
      ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0xe3Common);

      // The error from EsfMemoryManagerFclose indicates either an invalid
      // argument or an attempt to close memory that hasn't been opened, both
      // of which are implementation bugs of the FW Manager. Therefore, this
      // function returns an internal error.
      return kEsfFwMgrResultInternal;
    }
  }

  return kEsfFwMgrResultOk;
}

void EsfFwMgrHashToHexString(int32_t hash_size, const uint8_t *hash,
                             char *hex_string) {
  if (hash == NULL || hex_string == NULL) return;

  for (int i = 0; i < hash_size; i++) {
    snprintf(&hex_string[i * 2], 3, "%02x", hash[i]);
  }
  hex_string[hash_size * 2] = '\0';
}
