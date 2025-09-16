/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "system_manager_accessor_device_manifest.h"

#include <stdlib.h>
#include <string.h>

#include "base64/include/base64.h"
#include "json/include/json.h"
#include "parameter_storage_manager.h"
#include "parameter_storage_manager_common.h"
#include "pl.h"
#include "pl_system_manager.h"
#include "system_manager.h"
#include "utility_log.h"
#include "utility_log_module_id.h"

#ifndef ESF_SYSTEM_MANAGER_REMOVE_STATIC
#define STATIC static
#else  // ESF_SYSTEM_MANAGER_REMOVE_STATIC
#define STATIC
#endif  // ESF_SYSTEM_MANAGER_REMOVE_STATIC

// """Checks if the Device Manifest mask is enabled.
// This function examines the provided mask to determine whether the
// Device Manifest bit is enabled. The function uses the macro
// ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED to perform the bitwise
// check and return the result.
// Args:
//     [IN] mask (EsfParameterStorageManagerMask): The mask that contains
//        various flags, including the Device Manifest bit.
// Returns:
//     bool: `true` if the Device Manifest mask is enabled, otherwise
//       `false`.
// """
STATIC bool EsfParameterStorageManagerMaskEnabledDeviceManifest(
    EsfParameterStorageManagerMask mask);

// Helper function to parse JWT and extract AITRIOSCertUUID
STATIC EsfSystemManagerResult EsfSystemManagerParseJwtPayload(
    const char *jwt_token, char *serial_number, size_t serial_number_size);

// Information for accessing the Device Manifest structure members.
static const EsfParameterStorageManagerMemberInfo kMemberInfo[] = {
    {
        .id = kEsfParameterStorageManagerItemDeviceManifest,
        .type = kEsfParameterStorageManagerItemTypeOffsetBinaryPointer,
        .offset = offsetof(EsfParameterStorageManagerDeviceManifest,
                           device_manifest),
        .size = 0,
        .enabled = EsfParameterStorageManagerMaskEnabledDeviceManifest,
        .custom = NULL,
    },
};

// Information for accessing the Device Manifest structure.
static const EsfParameterStorageManagerStructInfo kStructInfo = {
    .items_num = sizeof(kMemberInfo) / sizeof(kMemberInfo[0]),
    .items = kMemberInfo,
};

EsfSystemManagerResult EsfSystemManagerLoadDeviceManifestFromPsm(
    EsfParameterStorageManagerHandle handle,
    const EsfParameterStorageManagerDeviceManifestMask *mask,
    EsfParameterStorageManagerDeviceManifest *data) {
  if ((mask == NULL) || (data == NULL)) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM, "%s-%d:Parameter error. mask=%p data=%p.",
        "system_manager_accessor_device_manifest.c", __LINE__, mask, data);
    return kEsfSystemManagerResultParamError;
  }

  EsfParameterStorageManagerStatus status = EsfParameterStorageManagerLoad(
      handle, (EsfParameterStorageManagerMask)mask,
      (EsfParameterStorageManagerData)data, &kStructInfo, NULL);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to load from Parameter Storage Manager.",
                     "system_manager_accessor_device_manifest.c", __LINE__);
    return kEsfSystemManagerResultInternalError;
  }

  if (mask->device_manifest) {
    if (EsfParameterStorageManagerIsDataEmpty(
            (EsfParameterStorageManagerData)data, &kStructInfo, 0)) {
      WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (uint16_t)0x8d10);
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Device Manifest is empty.",
                       "system_manager_accessor_device_manifest.c", __LINE__);
      return kEsfSystemManagerResultEmptyData;
    }
  }
  return kEsfSystemManagerResultOk;
}

EsfSystemManagerResult EsfSystemManagerSaveDeviceManifestToPsm(
    EsfParameterStorageManagerHandle handle,
    const EsfParameterStorageManagerDeviceManifestMask *mask,
    const EsfParameterStorageManagerDeviceManifest *data) {
  if ((mask == NULL) || (data == NULL)) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM, "%s-%d:Parameter error. mask=%p data=%p.",
        "system_manager_accessor_device_manifest.c", __LINE__, mask, data);
    return kEsfSystemManagerResultParamError;
  }

  EsfParameterStorageManagerStatus status = EsfParameterStorageManagerSave(
      handle, (EsfParameterStorageManagerMask)mask,
      (EsfParameterStorageManagerData)data, &kStructInfo, NULL);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to save to Parameter Storage Manager.",
                     "system_manager_accessor_device_manifest.c", __LINE__);
    return kEsfSystemManagerResultInternalError;
  }
  return kEsfSystemManagerResultOk;
}

STATIC bool EsfParameterStorageManagerMaskEnabledDeviceManifest(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfParameterStorageManagerDeviceManifestMask, device_manifest, mask);
}

STATIC EsfSystemManagerResult EsfSystemManagerParseJwtPayload(
    const char *jwt_token, char *serial_number, size_t serial_number_size) {
  if (jwt_token == NULL || serial_number == NULL || serial_number_size == 0) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Parameter error.",
                     "system_manager_accessor_device_manifest.c", __LINE__);
    return kEsfSystemManagerResultParamError;
  }

  // Find the dots that separate JWT parts (header.payload.signature)
  char *first_dot = strchr(jwt_token, '.');
  if (first_dot == NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Invalid JWT format - no first dot found.",
                     "system_manager_accessor_device_manifest.c", __LINE__);
    return kEsfSystemManagerResultInternalError;
  }

  char *second_dot = strchr(first_dot + 1, '.');
  if (second_dot == NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Invalid JWT format - no second dot found.",
                     "system_manager_accessor_device_manifest.c", __LINE__);
    return kEsfSystemManagerResultInternalError;
  }

  // Extract the payload part (between first and second dot)
  size_t payload_length = second_dot - (first_dot + 1);
  char *payload = malloc(payload_length + 1);
  if (payload == NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to allocate memory for JWT payload.",
                     "system_manager_accessor_device_manifest.c", __LINE__);
    return kEsfSystemManagerResultInternalError;
  }

  strncpy(payload, first_dot + 1, payload_length);
  payload[payload_length] = '\0';

  // Convert Base64URL to Base64 by adding necessary padding
  // Base64 requires strings to be multiples of 4 characters
  // Base64URL omits padding, so we need to calculate and add it
  size_t payload_len = strlen(payload);
  static const size_t padding_table[] = {0, 3, 2, 1};
  size_t padding_needed = padding_table[payload_len % 4];
  char *padded_payload = malloc(payload_len + padding_needed + 1);
  if (padded_payload == NULL) {
    free(payload);
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to allocate memory for padded payload.",
                     "system_manager_accessor_device_manifest.c", __LINE__);
    return kEsfSystemManagerResultInternalError;
  }

  strncpy(padded_payload, payload, payload_len);
  padded_payload[payload_len] = '\0';

  // Convert Base64URL to Base64: replace '-' with '+' and '_' with '/'
  for (size_t i = 0; i < payload_len; i++) {
    if (padded_payload[i] == '-') {
      padded_payload[i] = '+';
    } else if (padded_payload[i] == '_') {
      padded_payload[i] = '/';
    }
  }

  // Add padding characters
  for (size_t i = 0; i < padding_needed; i++) {
    padded_payload[payload_len + i] = '=';
  }
  padded_payload[payload_len + padding_needed] = '\0';

  free(payload);

  // Decode Base64URL payload using EsfCodecBase64Decode
  uint8_t decoded_payload[1024];
  size_t decoded_size = sizeof(decoded_payload);
  EsfCodecBase64ResultEnum codec_result = EsfCodecBase64Decode(
      padded_payload, strlen(padded_payload), decoded_payload, &decoded_size);
  free(padded_payload);

  if (codec_result != kEsfCodecBase64ResultSuccess) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to decode JWT payload with "
                     "EsfCodecBase64Decode. result=%d",
                     "system_manager_accessor_device_manifest.c", __LINE__,
                     codec_result);
    return kEsfSystemManagerResultInternalError;
  }

  // Null-terminate the decoded payload
  if (decoded_size < sizeof(decoded_payload)) {
    decoded_payload[decoded_size] = '\0';
  } else {
    decoded_payload[sizeof(decoded_payload) - 1] = '\0';
  }

  EsfJsonHandle json_handle = ESF_JSON_HANDLE_INITIALIZER;
  EsfJsonErrorCode json_result = EsfJsonOpen(&json_handle);
  if (json_result != kEsfJsonSuccess) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM, "%s-%d:Failed to open JSON handle. result=%d",
        "system_manager_accessor_device_manifest.c", __LINE__, json_result);
    return kEsfSystemManagerResultInternalError;
  }

  EsfJsonValue root_value = ESF_JSON_VALUE_INVALID;
  json_result = EsfJsonDeserialize(json_handle, (char *)decoded_payload,
                                   &root_value);
  if (json_result != kEsfJsonSuccess) {
    EsfJsonClose(json_handle);
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to parse JWT payload as JSON. result=%d",
                     "system_manager_accessor_device_manifest.c", __LINE__,
                     json_result);
    return kEsfSystemManagerResultInternalError;
  }

  // Get AITRIOSCertUUID from JSON object
  EsfJsonValue cert_uuid_value = ESF_JSON_VALUE_INVALID;
  json_result = EsfJsonObjectGet(json_handle, root_value, "AITRIOSCertUUID",
                                 &cert_uuid_value);
  if (json_result != kEsfJsonSuccess) {
    EsfJsonClose(json_handle);
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:AITRIOSCertUUID not found in JWT payload. result=%d",
        "system_manager_accessor_device_manifest.c", __LINE__, json_result);
    return kEsfSystemManagerResultInternalError;
  }

  // Get string value from JSON
  const char *cert_uuid = NULL;
  json_result = EsfJsonStringGet(json_handle, cert_uuid_value, &cert_uuid);
  if (json_result != kEsfJsonSuccess || cert_uuid == NULL) {
    EsfJsonClose(json_handle);
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to get AITRIOSCertUUID string value. result=%d",
        "system_manager_accessor_device_manifest.c", __LINE__, json_result);
    return kEsfSystemManagerResultInternalError;
  }

  // Copy to serial_number
  size_t uuid_length = strlen(cert_uuid);
  if (uuid_length >= serial_number_size) {
    uuid_length = serial_number_size - 1;
  }
  strncpy(serial_number, cert_uuid, uuid_length);
  serial_number[uuid_length] = '\0';

  EsfJsonClose(json_handle);
  return kEsfSystemManagerResultOk;
}

EsfSystemManagerResult EsfSystemManagerParseSerialNumberFromDeviceManifest(
    EsfSystemManagerHwInfo *hw_info_data) {
  if (hw_info_data == NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Parameter error.",
                     "system_manager_accessor_device_manifest.c", __LINE__);
    return kEsfSystemManagerResultParamError;
  }

  // Get the DeviceManifest data which contains the JWT token
  char *jwt_data = calloc(1, ESF_SYSTEM_MANAGER_DEVICE_MANIFEST_MAX_SIZE);
  if (jwt_data == NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to allocate memory for JWT data.",
                     "system_manager_accessor_device_manifest.c", __LINE__);
    return kEsfSystemManagerResultInternalError;
  }

  size_t jwt_data_size = ESF_SYSTEM_MANAGER_DEVICE_MANIFEST_MAX_SIZE;
  EsfSystemManagerResult sm_result =
      EsfSystemManagerGetDeviceManifest(jwt_data, &jwt_data_size);
  if (sm_result != kEsfSystemManagerResultOk) {
    free(jwt_data);
    if (sm_result == kEsfSystemManagerResultEmptyData) {
      WRITE_DLOG_ERROR(
          MODULE_ID_SYSTEM, "%s-%d:DeviceManifest data is empty. result=%d",
          "system_manager_accessor_device_manifest.c", __LINE__, sm_result);
      hw_info_data->serial_number[0] = '\0';
      return kEsfSystemManagerResultEmptyData;
    } else {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:Failed to get DeviceManifest data. result=%d",
                       "system_manager_accessor_device_manifest.c", __LINE__,
                       sm_result);
      return kEsfSystemManagerResultInternalError;
    }
  }

  // Parse JWT to extract serial number
  EsfSystemManagerResult result =
      EsfSystemManagerParseJwtPayload(jwt_data, hw_info_data->serial_number,
                                      sizeof(hw_info_data->serial_number));
  free(jwt_data);

  if (result != kEsfSystemManagerResultOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM, "%s-%d:Failed to parse JWT payload. result=%d",
        "system_manager_accessor_device_manifest.c", __LINE__, result);
    return result;
  }

  return kEsfSystemManagerResultOk;
}
