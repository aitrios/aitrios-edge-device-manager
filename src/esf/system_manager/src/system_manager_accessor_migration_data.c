/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "system_manager_accessor_migration_data.h"

#include <stdio.h>
#include <string.h>

#include "mbedtls/sha256.h"
#include "parameter_storage_manager.h"
#include "parameter_storage_manager_common.h"
#include "pl.h"
#include "pl_system_manager.h"
#include "system_manager.h"
#include "system_manager_accessor_device_manifest.h"
#include "system_manager_accessor_evp.h"
#include "system_manager_accessor_hwinfo.h"
#include "system_manager_accessor_root_auth.h"
#include "utility_log.h"
#include "utility_log_module_id.h"

#ifndef ESF_SYSTEM_MANAGER_REMOVE_STATIC
#define STATIC static
#else  // ESF_SYSTEM_MANAGER_REMOVE_STATIC
#define STATIC
#endif  // ESF_SYSTEM_MANAGER_REMOVE_STATIC

// Define constants for EVP setup data parsing
#define EVP_SETUP_ADDRESS_PREFIX "Address:"
#define EVP_SETUP_PORT_PREFIX "Port:"

// This helper function locates a prefix in the data, extracts the value
// following it until the next newline, allocates memory for the value,
// and copies it to the allocated buffer.
// Args:
//   [IN] data (const char*): The EVP setup data to parse.
//   [IN] prefix (const char*): The prefix to search for.
// Returns:
//   char*: Pointer to allocated string containing the extracted value,
//          or NULL if extraction fails or memory allocation fails.
// """
STATIC char *EsfSystemManagerExtractEvpSetupValue(const char *data,
                                                  const char *prefix) {
  char *prefix_start = strstr(data, prefix);
  if (!prefix_start) {
    WRITE_DLOG_WARN(MODULE_ID_SYSTEM,
                    "%s-%d:Could not find prefix in EVP Setup Info data",
                    "system_manager_accessor_migration_data.c", __LINE__);
    return NULL;
  }

  // Move past the prefix
  char *value_start = prefix_start + strlen(prefix);

  // Find the end of the value (newline or end of string)
  char *value_end = strchr(value_start, '\n');
  if (!value_end) {
    value_end = value_start + strlen(value_start);
  }

  // Calculate length and allocate memory
  size_t value_len = value_end - value_start;
  char *value = calloc(1, value_len + 1);
  if (value == NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to allocate memory for EVP value.",
                     "system_manager_accessor_migration_data.c", __LINE__);
    return NULL;
  }

  // Copy the value
  strncpy(value, value_start, value_len);
  return value;
}

// """Gets migration data by ID.
// This function retrieves migration data based on the provided ID and
// stores it in the destination buffer. The function calls the platform
// layer implementation to perform the actual data retrieval.
// Args:
//     [IN] id (PlSystemManagerMigrationDataId): The migration data ID to
//     retrieve. [OUT] dst (void*): The destination buffer to store the
//     retrieved data. [IN] dst_size (size_t): The size of the destination
//     buffer.
// Returns:
//     EsfSystemManagerResult: kEsfSystemManagerResultOk if successful,
//       otherwise an error code.
// """

EsfSystemManagerResult EsfSystemManagerMigrationData(
    PlSystemManagerMigrationDataId id, void *dst, size_t dst_size) {
  EsfSystemManagerResult ret = kEsfSystemManagerResultOk;

  PlErrCode pl_ret = PlSystemManagerGetMigrationData(id, dst, dst_size);
  if (pl_ret != kPlErrCodeOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM, "%s-%d:Failed to get migration data. result=%d",
        "system_manager_accessor_migration_data.c", __LINE__, pl_ret);
    return kEsfSystemManagerResultInternalError;
  }
  return ret;
}

EsfSystemManagerResult EsfSystemManagerMigrateRootAuth(void) {
  EsfSystemManagerResult ret = kEsfSystemManagerResultOk;
  char *data = calloc(
      1, ESF_SYSTEM_MANAGER_ROOT_CA_MAX_SIZE);  // Buffer for RootAuth data
  if (data == NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to allocate memory for RootAuth data.",
                     "system_manager_accessor_migration_data.c", __LINE__);
    return kEsfSystemManagerResultInternalError;
  }

  ret = EsfSystemManagerMigrationData(kPlSystemManagerMigrationDataIdRootAuth,
                                      data,
                                      ESF_SYSTEM_MANAGER_ROOT_CA_MAX_SIZE);
  if (ret != kEsfSystemManagerResultOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to get migration data for RootAuth. result=%d",
        "system_manager_accessor_migration_data.c", __LINE__, ret);
    free(data);
    return ret;
  }

  // Check if data size is 0, skip PSM write if so
  if (strlen(data) == 0) {
    WRITE_DLOG_INFO(MODULE_ID_SYSTEM,
                    "%s-%d:RootAuth data size is 0, skipping PSM write.",
                    "system_manager_accessor_migration_data.c", __LINE__);
    free(data);
    return kEsfSystemManagerResultOk;
  }

  // Calculate SHA-256 hash of Root CA
  uint8_t hash_binary[32];  // SHA-256 produces 32 bytes
  int mbedtls_ret = mbedtls_sha256((uint8_t *)data, strlen(data), hash_binary,
                                   0);  // 0 for SHA-256

  // Save RootAuth data to PSM
  EsfParameterStorageManagerRootAuth root_auth = {0};
  EsfParameterStorageManagerRootAuthMask mask = {.root_ca = 1,
                                                 .root_ca_hash = 1};

  root_auth.root_ca.size = strlen(data);
  root_auth.root_ca.data = (uint8_t *)data;

  if (mbedtls_ret == 0) {
    // Set binary hash directly to root_auth structure
    memcpy(root_auth.root_ca_hash, hash_binary, 32);
    root_auth.root_ca_hash[32] = '\0';
  } else {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to calculate SHA-256 hash for Root CA. mbedtls_ret=%d",
        "system_manager_accessor_migration_data.c", __LINE__, mbedtls_ret);
    // Continue without hash
    mask.root_ca_hash = 0;  // Don't save hash if calculation failed
  }

  // Open PSM handle only for saving
  EsfParameterStorageManagerHandle handle =
      ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE;
  EsfParameterStorageManagerStatus status =
      EsfParameterStorageManagerOpen(&handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to open Parameter Storage Manager. status=%d",
        "system_manager_accessor_migration_data.c", __LINE__, status);
    ret = kEsfSystemManagerResultInternalError;
  } else {
    ret = EsfSystemManagerSaveRootAuthToPsm(handle, &mask, &root_auth);
    if (ret != kEsfSystemManagerResultOk) {
      WRITE_DLOG_ERROR(
          MODULE_ID_SYSTEM, "%s-%d:Failed to save RootAuth to PSM. result=%d",
          "system_manager_accessor_migration_data.c", __LINE__, ret);
    }

    // Close PSM handle immediately after saving
    status = EsfParameterStorageManagerClose(handle);
    if (status != kEsfParameterStorageManagerStatusOk) {
      WRITE_DLOG_ERROR(
          MODULE_ID_SYSTEM,
          "%s-%d:Failed to close Parameter Storage Manager. status=%d",
          "system_manager_accessor_migration_data.c", __LINE__, status);
      if (ret == kEsfSystemManagerResultOk) {
        ret = kEsfSystemManagerResultInternalError;
      }
    }
  }

  free(data);
  return ret;
}

/*TODO : This section may need to be updated after the Device Manifest
 * specification is finalized. */
EsfSystemManagerResult EsfSystemManagerMigrateDeviceManifest(void) {
  EsfSystemManagerResult ret = kEsfSystemManagerResultOk;
  char *data = calloc(
      1, ESF_SYSTEM_MANAGER_DEVICE_MANIFEST_MAX_SIZE);  // Buffer for
                                                        // DeviceManifest data
  if (data == NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to allocate memory for DeviceManifest data.",
                     "system_manager_accessor_migration_data.c", __LINE__);
    return kEsfSystemManagerResultInternalError;
  }

  ret = EsfSystemManagerMigrationData(
      kPlSystemManagerMigrationDataIdDeviceManifest, data,
      ESF_SYSTEM_MANAGER_DEVICE_MANIFEST_MAX_SIZE);
  if (ret != kEsfSystemManagerResultOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to get migration data for DeviceManifest. result=%d",
        "system_manager_accessor_migration_data.c", __LINE__, ret);
    free(data);
    return ret;
  }

  // Check if data size is 0, skip PSM write if so
  if (strlen(data) == 0) {
    WRITE_DLOG_INFO(MODULE_ID_SYSTEM,
                    "%s-%d:DeviceManifest data size is 0, skipping PSM write.",
                    "system_manager_accessor_migration_data.c", __LINE__);
    free(data);
    return kEsfSystemManagerResultOk;
  }

  // Save DeviceManifest data to PSM
  EsfParameterStorageManagerDeviceManifest device_manifest = {0};
  EsfParameterStorageManagerDeviceManifestMask mask = {.device_manifest = 1};

  // Set up device_manifest data
  device_manifest.device_manifest.size = strlen(data);
  device_manifest.device_manifest.data = (uint8_t *)data;

  // Open PSM handle only for saving
  EsfParameterStorageManagerHandle handle =
      ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE;
  EsfParameterStorageManagerStatus status =
      EsfParameterStorageManagerOpen(&handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to open Parameter Storage Manager. status=%d",
        "system_manager_accessor_migration_data.c", __LINE__, status);
    ret = kEsfSystemManagerResultInternalError;
  } else {
    ret = EsfSystemManagerSaveDeviceManifestToPsm(handle, &mask,
                                                  &device_manifest);
    if (ret != kEsfSystemManagerResultOk) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:Failed to save DeviceManifest to PSM. result=%d",
                       "system_manager_accessor_migration_data.c", __LINE__,
                       ret);
    }

    // Close PSM handle immediately after saving
    status = EsfParameterStorageManagerClose(handle);
    if (status != kEsfParameterStorageManagerStatusOk) {
      WRITE_DLOG_ERROR(
          MODULE_ID_SYSTEM,
          "%s-%d:Failed to close Parameter Storage Manager. status=%d",
          "system_manager_accessor_migration_data.c", __LINE__, status);
      if (ret == kEsfSystemManagerResultOk) {
        ret = kEsfSystemManagerResultInternalError;
      }
    }
  }

  free(data);
  return ret;
}

EsfSystemManagerResult EsfSystemManagerMigrateHwInfo(void) {
  EsfSystemManagerResult ret = kEsfSystemManagerResultOk;
  size_t data_size = PlSystemManagerGetHwInfoSize();
  char *data = calloc(1, data_size);  // Buffer for HwInfo data
  if (data == NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to allocate memory for HwInfo data.",
                     "system_manager_accessor_migration_data.c", __LINE__);
    return kEsfSystemManagerResultInternalError;
  }

  ret = EsfSystemManagerMigrationData(kPlSystemManagerMigrationDataIdHwInfo,
                                      data, data_size);
  if (ret != kEsfSystemManagerResultOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to get migration data for HwInfo. result=%d",
                     "system_manager_accessor_migration_data.c", __LINE__, ret);
    free(data);
    return ret;
  }

  // Check if data size is 0, skip PSM write if so
  if (strlen(data) == 0) {
    WRITE_DLOG_INFO(MODULE_ID_SYSTEM,
                    "%s-%d:HwInfo data size is 0, skipping PSM write.",
                    "system_manager_accessor_migration_data.c", __LINE__);
    free(data);
    return kEsfSystemManagerResultOk;
  }

  // Save HwInfo data to PSM
  EsfParameterStorageManagerHwInfo hw_info = {0};
  EsfParameterStorageManagerHwInfoMask mask = {.hw_info = 1};

  // Set up hw_info data
  hw_info.hw_info.size = strlen(data);
  hw_info.hw_info.data = (uint8_t *)data;

  // Open PSM handle only for saving
  EsfParameterStorageManagerHandle handle =
      ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE;
  EsfParameterStorageManagerStatus status =
      EsfParameterStorageManagerOpen(&handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to open Parameter Storage Manager. status=%d",
        "system_manager_accessor_migration_data.c", __LINE__, status);
    ret = kEsfSystemManagerResultInternalError;
  } else {
    ret = EsfSystemManagerSaveHwInfoToPsm(handle, &mask, &hw_info);
    if (ret != kEsfSystemManagerResultOk) {
      WRITE_DLOG_ERROR(
          MODULE_ID_SYSTEM, "%s-%d:Failed to save HwInfo to PSM. result=%d",
          "system_manager_accessor_migration_data.c", __LINE__, ret);
    }

    // Close PSM handle immediately after saving
    status = EsfParameterStorageManagerClose(handle);
    if (status != kEsfParameterStorageManagerStatusOk) {
      WRITE_DLOG_ERROR(
          MODULE_ID_SYSTEM,
          "%s-%d:Failed to close Parameter Storage Manager. status=%d",
          "system_manager_accessor_migration_data.c", __LINE__, status);
      if (ret == kEsfSystemManagerResultOk) {
        ret = kEsfSystemManagerResultInternalError;
      }
    }
  }

  free(data);
  return ret;
}

EsfSystemManagerResult EsfSystemManagerMigrateEvpSetupInfo(void) {
  EsfSystemManagerResult ret = kEsfSystemManagerResultOk;
  char *data = calloc(1, 128);  // Buffer for Old EVP Setup Info data
  if (data == NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to allocate memory for EVP Setup Info data.",
                     "system_manager_accessor_migration_data.c", __LINE__);
    return kEsfSystemManagerResultInternalError;
  }

  ret = EsfSystemManagerMigrationData(
      kPlSystemManagerMigrationDataIdEvpSetupInfo, data, 128);
  if (ret != kEsfSystemManagerResultOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to get migration data for Old EVP Setup Info. result=%d",
        "system_manager_accessor_migration_data.c", __LINE__, ret);
    free(data);
    return ret;
  }

  // Check if data size is 0, skip PSM write if so
  if (strlen(data) == 0) {
    WRITE_DLOG_INFO(MODULE_ID_SYSTEM,
                    "%s-%d:EVP Setup Info data size is 0, skipping PSM write.",
                    "system_manager_accessor_migration_data.c", __LINE__);
    free(data);
    return kEsfSystemManagerResultOk;
  }

  // Parse data and extract EVP configuration values
  // Extract Address and Port from EVP setup data
  char *address_start = strstr(data, EVP_SETUP_ADDRESS_PREFIX);
  char *port_start = strstr(data, EVP_SETUP_PORT_PREFIX);

  if (address_start && port_start) {
    // Extract address and port using helper function
    char *address =
        EsfSystemManagerExtractEvpSetupValue(data, EVP_SETUP_ADDRESS_PREFIX);
    if (address == NULL) {
      free(data);
      return kEsfSystemManagerResultInternalError;
    }

    char *port_str =
        EsfSystemManagerExtractEvpSetupValue(data, EVP_SETUP_PORT_PREFIX);
    if (port_str == NULL) {
      free(address);
      free(data);
      return kEsfSystemManagerResultInternalError;
    }

    // Save EVP configuration using EVP structure
    EsfParameterStorageManagerEvp evp_config = {0};
    EsfParameterStorageManagerEvpMask evp_mask = {.evp_url = 1, .evp_port = 1};

    // Copy address to evp_url field
    size_t address_copy_len =
        (strlen(address) < ESF_SYSTEM_MANAGER_EVP_HUB_URL_MAX_SIZE - 1)
            ? strlen(address)
            : ESF_SYSTEM_MANAGER_EVP_HUB_URL_MAX_SIZE - 1;
    strncpy(evp_config.evp_url, address, address_copy_len);
    evp_config.evp_url[address_copy_len] = '\0';

    // Copy port to evp_port field
    size_t port_copy_len =
        (strlen(port_str) < ESF_SYSTEM_MANAGER_EVP_HUB_PORT_MAX_SIZE - 1)
            ? strlen(port_str)
            : ESF_SYSTEM_MANAGER_EVP_HUB_PORT_MAX_SIZE - 1;
    strncpy(evp_config.evp_port, port_str, port_copy_len);
    evp_config.evp_port[port_copy_len] = '\0';

    // Open PSM handle only for saving
    EsfParameterStorageManagerHandle handle =
        ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE;
    EsfParameterStorageManagerStatus status =
        EsfParameterStorageManagerOpen(&handle);
    if (status != kEsfParameterStorageManagerStatusOk) {
      WRITE_DLOG_ERROR(
          MODULE_ID_SYSTEM,
          "%s-%d:Failed to open Parameter Storage Manager. status=%d",
          "system_manager_accessor_migration_data.c", __LINE__, status);
      ret = kEsfSystemManagerResultInternalError;
    } else {
      ret = EsfSystemManagerSaveEvpToPsm(handle, &evp_mask, &evp_config);
      if (ret != kEsfSystemManagerResultOk) {
        WRITE_DLOG_ERROR(
            MODULE_ID_SYSTEM,
            "%s-%d:Failed to save EVP configuration to PSM. result=%d",
            "system_manager_accessor_migration_data.c", __LINE__, ret);
      }

      // Close PSM handle immediately after saving
      status = EsfParameterStorageManagerClose(handle);
      if (status != kEsfParameterStorageManagerStatusOk) {
        WRITE_DLOG_ERROR(
            MODULE_ID_SYSTEM,
            "%s-%d:Failed to close Parameter Storage Manager. status=%d",
            "system_manager_accessor_migration_data.c", __LINE__, status);
        if (ret == kEsfSystemManagerResultOk) {
          ret = kEsfSystemManagerResultInternalError;
        }
      }
    }

    free(port_str);
    free(address);
  } else {
    WRITE_DLOG_WARN(MODULE_ID_SYSTEM,
                    "%s-%d:Could not find Address/Port in EVP Setup Info data",
                    "system_manager_accessor_migration_data.c", __LINE__);
  }

  free(data);
  return ret;
}

EsfSystemManagerResult EsfSystemManagerEraseMigrationData(
    PlSystemManagerMigrationDataId id) {
  PlErrCode pl_ret = PlSystemManagerEraseMigrationData(id);
  if (pl_ret != kPlErrCodeOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to erase migration data. result=%d id=%d",
                     "system_manager_accessor_migration_data.c", __LINE__,
                     pl_ret, id);
    return kEsfSystemManagerResultInternalError;
  }
  return kEsfSystemManagerResultOk;
}
