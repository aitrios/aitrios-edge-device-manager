/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "system_manager.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "log_manager.h"
#include "parameter_storage_manager.h"
#include "pl_system_control.h"
#include "pl_system_manager.h"
#include "power_manager.h"
#include "system_manager_accessor_device_manifest.h"
#include "system_manager_accessor_enrollment.h"
#include "system_manager_accessor_evp.h"
#include "system_manager_accessor_exception_info.h"
#include "system_manager_accessor_hwinfo.h"
#include "system_manager_accessor_initial_setting_flag.h"
#include "system_manager_accessor_qr_mode_timeout_value.h"
#include "system_manager_accessor_reset_cause.h"
#include "system_manager_accessor_root_auth.h"
#include "utility_log.h"
#include "utility_log_module_id.h"

#define SYSTEM_MANAGER_NULL_TERMINATED_SIZE (1)

EsfSystemManagerResult EsfSystemManagerGetDeviceManifest(char *data,
                                                         size_t *data_size) {
  if ((data == (char *)NULL) || (data_size == (size_t *)NULL) ||
      (*data_size == 0)) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Parameter error. data=%p data_size=%p *data_size=%zu",
        "system_manager.c", __LINE__, data, data_size,
        data_size ? *data_size : 0);
    return kEsfSystemManagerResultParamError;
  }

  EsfParameterStorageManagerHandle handle =
      ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE;
  EsfParameterStorageManagerStatus status =
      EsfParameterStorageManagerOpen(&handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to open parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  uint32_t loadable_size = 0;
  status = EsfParameterStorageManagerGetSize(
      handle, kEsfParameterStorageManagerItemDeviceManifest, &loadable_size);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to get size parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    (void)EsfParameterStorageManagerClose(handle);
    return kEsfSystemManagerResultInternalError;
  }

  if (loadable_size == 0) {
    WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (uint16_t)0x8d10);
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Device Manifest is empty.",
                     "system_manager.c", __LINE__);
    (void)EsfParameterStorageManagerClose(handle);
    return kEsfSystemManagerResultEmptyData;
  }

  if (*data_size < (loadable_size + SYSTEM_MANAGER_NULL_TERMINATED_SIZE)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:data_size=%lu is less than loadable_size=%d",
                     "system_manager.c", __LINE__, *data_size,
                     loadable_size + SYSTEM_MANAGER_NULL_TERMINATED_SIZE);
    (void)EsfParameterStorageManagerClose(handle);
    return kEsfSystemManagerResultOutOfRange;
  }

  EsfParameterStorageManagerDeviceManifestMask mask = {.device_manifest = 1};

  EsfParameterStorageManagerDeviceManifest data_struct;
  memset(&data_struct, 0, sizeof(data_struct));
  data_struct.device_manifest.offset = 0;
  data_struct.device_manifest.size = loadable_size;
  data_struct.device_manifest.data = (uint8_t *)data;

  EsfSystemManagerResult result =
      EsfSystemManagerLoadDeviceManifestFromPsm(handle, &mask, &data_struct);
  if (result != kEsfSystemManagerResultOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to load Device Manifest from parameter storage "
        "manager. result=%d",
        "system_manager.c", __LINE__, result);
    (void)EsfParameterStorageManagerClose(handle);
    return result;
  }

  status = EsfParameterStorageManagerClose(handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to close parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  // If there is no null character, it is given.
  if (((char *)(data_struct.device_manifest
                    .data))[data_struct.device_manifest.size - 1] != '\0') {
    if (data_struct.device_manifest.size < *data_size) {
      // Insert Null character at the end.
      ((char *)(data_struct.device_manifest
                    .data))[data_struct.device_manifest.size] = '\0';
      ++data_struct.device_manifest.size;
    } else {
      WRITE_DLOG_ERROR(
          MODULE_ID_SYSTEM,
          "%s-%d:data_size=%lu data_struct.device_manifest.size=%d",
          "system_manager.c", __LINE__, *data_size,
          data_struct.device_manifest.size);
      return kEsfSystemManagerResultOutOfRange;
    }
  }

  *data_size = data_struct.device_manifest.size;

  return kEsfSystemManagerResultOk;
}

EsfSystemManagerResult EsfSystemManagerGetDpsUrl(char *data,
                                                 size_t *data_size) {
  if ((data == (char *)NULL) || (data_size == (size_t *)NULL) ||
      (*data_size == 0)) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Parameter error. data=%p data_size=%p *data_size=%zu",
        "system_manager.c", __LINE__, data, data_size,
        data_size ? *data_size : 0);
    return kEsfSystemManagerResultParamError;
  }

  EsfParameterStorageManagerHandle handle =
      ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE;
  EsfParameterStorageManagerStatus status =
      EsfParameterStorageManagerOpen(&handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to open parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  uint32_t loadable_size = 0;
  status = EsfParameterStorageManagerGetSize(
      handle, kEsfParameterStorageManagerItemDpsURL, &loadable_size);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to get size parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    (void)EsfParameterStorageManagerClose(handle);
    return kEsfSystemManagerResultInternalError;
  }

  if (*data_size < (loadable_size + SYSTEM_MANAGER_NULL_TERMINATED_SIZE)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:data_size=%lu is less than loadable_size=%d",
                     "system_manager.c", __LINE__, *data_size,
                     loadable_size + SYSTEM_MANAGER_NULL_TERMINATED_SIZE);
    (void)EsfParameterStorageManagerClose(handle);
    return kEsfSystemManagerResultOutOfRange;
  }

  EsfParameterStorageManagerEnrollmentMask mask = {.dps_url = 1};

  EsfParameterStorageManagerEnrollment *data_struct =
      calloc(1, sizeof(EsfParameterStorageManagerEnrollment));
  if (data_struct == NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Failed to calloc.",
                     "system_manager.c", __LINE__);
    (void)EsfParameterStorageManagerClose(handle);
    return kEsfSystemManagerResultInternalError;
  }

  EsfSystemManagerResult result =
      EsfSystemManagerLoadEnrollmentFromPsm(handle, &mask, data_struct);
  if (result != kEsfSystemManagerResultOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to load Dps URL from parameter storage "
                     "manager. result=%d",
                     "system_manager.c", __LINE__, result);
    free(data_struct);
    (void)EsfParameterStorageManagerClose(handle);
    return result;
  }

  status = EsfParameterStorageManagerClose(handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to close parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    free(data_struct);
    return kEsfSystemManagerResultInternalError;
  }

  size_t load_size = strlen(data_struct->dps_url) +
                     SYSTEM_MANAGER_NULL_TERMINATED_SIZE;
  if (load_size > *data_size) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:data_size is insufficient for load_size. "
                     "data_size=%zu load_size=%zu",
                     "system_manager.c", __LINE__, *data_size, load_size);
    free(data_struct);
    return kEsfSystemManagerResultOutOfRange;
  }

  strncpy(data, data_struct->dps_url, *data_size);
  free(data_struct);
  *data_size = load_size;

  return kEsfSystemManagerResultOk;
}

EsfSystemManagerResult EsfSystemManagerSetDpsUrl(const char *data,
                                                 size_t data_size) {
  if ((data == (char *)NULL) || (data_size == 0) ||
      (data_size > ESF_SYSTEM_MANAGER_DPS_URL_MAX_SIZE)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Parameter error. data=%p data_size=%zu",
                     "system_manager.c", __LINE__, data, data_size);
    return kEsfSystemManagerResultParamError;
  }

  if (strnlen(data, data_size) == data_size) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:NULL-terminated character does not exist",
                     "system_manager.c", __LINE__);
    return kEsfSystemManagerResultParamError;
  }

  EsfParameterStorageManagerHandle handle =
      ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE;
  EsfParameterStorageManagerStatus status =
      EsfParameterStorageManagerOpen(&handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to open parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  EsfParameterStorageManagerEnrollmentMask mask = {.dps_url = 1};

  EsfParameterStorageManagerEnrollment *data_struct =
      calloc(1, sizeof(EsfParameterStorageManagerEnrollment));
  if (data_struct == NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Failed to calloc.",
                     "system_manager.c", __LINE__);
    (void)EsfParameterStorageManagerClose(handle);
    return kEsfSystemManagerResultInternalError;
  }
  strncpy(data_struct->dps_url, data, sizeof(data_struct->dps_url));

  EsfSystemManagerResult result =
      EsfSystemManagerSaveEnrollmentToPsm(handle, &mask, data_struct);

  free(data_struct);

  if (result != kEsfSystemManagerResultOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to save Dps URL to parameter storage "
                     "manager. result=%d",
                     "system_manager.c", __LINE__, result);
    (void)EsfParameterStorageManagerClose(handle);
    return result;
  }

  status = EsfParameterStorageManagerClose(handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to close parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  return kEsfSystemManagerResultOk;
}

EsfSystemManagerResult EsfSystemManagerGetCommonName(char *data,
                                                     size_t *data_size) {
  if ((data == (char *)NULL) || (data_size == (size_t *)NULL) ||
      (*data_size == 0)) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Parameter error. data=%p data_size=%p *data_size=%zu",
        "system_manager.c", __LINE__, data, data_size,
        data_size ? *data_size : 0);
    return kEsfSystemManagerResultParamError;
  }

  EsfParameterStorageManagerHandle handle =
      ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE;
  EsfParameterStorageManagerStatus status =
      EsfParameterStorageManagerOpen(&handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to open parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  uint32_t loadable_size = 0;
  status = EsfParameterStorageManagerGetSize(
      handle, kEsfParameterStorageManagerItemCommonName, &loadable_size);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to get size parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    (void)EsfParameterStorageManagerClose(handle);
    return kEsfSystemManagerResultInternalError;
  }

  if (*data_size < (loadable_size + SYSTEM_MANAGER_NULL_TERMINATED_SIZE)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:data_size=%lu is less than loadable_size=%d",
                     "system_manager.c", __LINE__, *data_size,
                     loadable_size + SYSTEM_MANAGER_NULL_TERMINATED_SIZE);
    (void)EsfParameterStorageManagerClose(handle);
    return kEsfSystemManagerResultOutOfRange;
  }

  EsfParameterStorageManagerEnrollmentMask mask = {.common_name = 1};

  EsfParameterStorageManagerEnrollment *data_struct =
      calloc(1, sizeof(EsfParameterStorageManagerEnrollment));
  if (data_struct == NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Failed to calloc.",
                     "system_manager.c", __LINE__);
    (void)EsfParameterStorageManagerClose(handle);
    return kEsfSystemManagerResultInternalError;
  }

  EsfSystemManagerResult result =
      EsfSystemManagerLoadEnrollmentFromPsm(handle, &mask, data_struct);
  if (result != kEsfSystemManagerResultOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to load Common Name from parameter storage "
                     "manager. result=%d",
                     "system_manager.c", __LINE__, result);
    free(data_struct);
    (void)EsfParameterStorageManagerClose(handle);
    return result;
  }

  status = EsfParameterStorageManagerClose(handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to close parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    free(data_struct);
    return kEsfSystemManagerResultInternalError;
  }

  size_t load_size = strlen(data_struct->common_name) +
                     SYSTEM_MANAGER_NULL_TERMINATED_SIZE;
  if (load_size > *data_size) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:data_size is insufficient for load_size. "
                     "data_size=%zu load_size=%zu",
                     "system_manager.c", __LINE__, *data_size, load_size);
    free(data_struct);
    return kEsfSystemManagerResultOutOfRange;
  }

  strncpy(data, data_struct->common_name, *data_size);
  free(data_struct);
  *data_size = load_size;

  return kEsfSystemManagerResultOk;
}

EsfSystemManagerResult EsfSystemManagerSetCommonName(const char *data,
                                                     size_t data_size) {
  if ((data == (char *)NULL) || (data_size == 0) ||
      (data_size > ESF_SYSTEM_MANAGER_COMMON_NAME_MAX_SIZE)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Parameter error. data=%p data_size=%zu",
                     "system_manager.c", __LINE__, data, data_size);
    return kEsfSystemManagerResultParamError;
  }

  if (strnlen(data, data_size) == data_size) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:NULL-terminated character does not exist",
                     "system_manager.c", __LINE__);
    return kEsfSystemManagerResultParamError;
  }

  EsfParameterStorageManagerHandle handle =
      ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE;
  EsfParameterStorageManagerStatus status =
      EsfParameterStorageManagerOpen(&handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to open parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  EsfParameterStorageManagerEnrollmentMask mask = {.common_name = 1};

  EsfParameterStorageManagerEnrollment *data_struct =
      calloc(1, sizeof(EsfParameterStorageManagerEnrollment));
  if (data_struct == NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Failed to calloc.",
                     "system_manager.c", __LINE__);
    (void)EsfParameterStorageManagerClose(handle);
    return kEsfSystemManagerResultInternalError;
  }
  strncpy(data_struct->common_name, data, sizeof(data_struct->common_name));

  EsfSystemManagerResult result =
      EsfSystemManagerSaveEnrollmentToPsm(handle, &mask, data_struct);

  free(data_struct);

  if (result != kEsfSystemManagerResultOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to save Common Name to parameter storage "
                     "manager. result=%d",
                     "system_manager.c", __LINE__, result);
    (void)EsfParameterStorageManagerClose(handle);
    return result;
  }

  status = EsfParameterStorageManagerClose(handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to close parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  return kEsfSystemManagerResultOk;
}

EsfSystemManagerResult EsfSystemManagerGetDpsScopeId(char *data,
                                                     size_t *data_size) {
  if ((data == (char *)NULL) || (data_size == (size_t *)NULL) ||
      (*data_size == 0)) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Parameter error. data=%p data_size=%p *data_size=%zu",
        "system_manager.c", __LINE__, data, data_size,
        data_size ? *data_size : 0);
    return kEsfSystemManagerResultParamError;
  }

  EsfParameterStorageManagerHandle handle =
      ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE;
  EsfParameterStorageManagerStatus status =
      EsfParameterStorageManagerOpen(&handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to open parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  uint32_t loadable_size = 0;
  status = EsfParameterStorageManagerGetSize(
      handle, kEsfParameterStorageManagerItemDpsScopeID, &loadable_size);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to get size parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    (void)EsfParameterStorageManagerClose(handle);
    return kEsfSystemManagerResultInternalError;
  }

  if (*data_size < (loadable_size + SYSTEM_MANAGER_NULL_TERMINATED_SIZE)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:data_size=%lu is less than loadable_size=%d",
                     "system_manager.c", __LINE__, *data_size,
                     loadable_size + SYSTEM_MANAGER_NULL_TERMINATED_SIZE);
    (void)EsfParameterStorageManagerClose(handle);
    return kEsfSystemManagerResultOutOfRange;
  }

  EsfParameterStorageManagerEnrollmentMask mask = {.dps_scope_id = 1};

  EsfParameterStorageManagerEnrollment *data_struct =
      calloc(1, sizeof(EsfParameterStorageManagerEnrollment));
  if (data_struct == NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Failed to calloc.",
                     "system_manager.c", __LINE__);
    (void)EsfParameterStorageManagerClose(handle);
    return kEsfSystemManagerResultInternalError;
  }

  EsfSystemManagerResult result =
      EsfSystemManagerLoadEnrollmentFromPsm(handle, &mask, data_struct);
  if (result != kEsfSystemManagerResultOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to load Dps Scope ID from parameter storage "
                     "manager. result=%d",
                     "system_manager.c", __LINE__, result);
    free(data_struct);
    (void)EsfParameterStorageManagerClose(handle);
    return result;
  }

  status = EsfParameterStorageManagerClose(handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to close parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    free(data_struct);
    return kEsfSystemManagerResultInternalError;
  }

  size_t load_size = strlen(data_struct->dps_scope_id) +
                     SYSTEM_MANAGER_NULL_TERMINATED_SIZE;
  if (load_size > *data_size) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:data_size is insufficient for load_size. "
                     "data_size=%zu load_size=%zu",
                     "system_manager.c", __LINE__, *data_size, load_size);
    free(data_struct);
    return kEsfSystemManagerResultOutOfRange;
  }

  strncpy(data, data_struct->dps_scope_id, *data_size);
  free(data_struct);
  *data_size = load_size;

  return kEsfSystemManagerResultOk;
}

EsfSystemManagerResult EsfSystemManagerSetDpsScopeId(const char *data,
                                                     size_t data_size) {
  if ((data == (char *)NULL) || (data_size == 0) ||
      (data_size > ESF_SYSTEM_MANAGER_DPS_SCOPE_ID_MAX_SIZE)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Parameter error. data=%p data_size=%zu",
                     "system_manager.c", __LINE__, data, data_size);
    return kEsfSystemManagerResultParamError;
  }

  if (strnlen(data, data_size) == data_size) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:NULL-terminated character does not exist",
                     "system_manager.c", __LINE__);
    return kEsfSystemManagerResultParamError;
  }

  EsfParameterStorageManagerHandle handle =
      ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE;
  EsfParameterStorageManagerStatus status =
      EsfParameterStorageManagerOpen(&handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to open parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  EsfParameterStorageManagerEnrollmentMask mask = {.dps_scope_id = 1};

  EsfParameterStorageManagerEnrollment *data_struct =
      calloc(1, sizeof(EsfParameterStorageManagerEnrollment));
  if (data_struct == NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Failed to calloc.",
                     "system_manager.c", __LINE__);
    (void)EsfParameterStorageManagerClose(handle);
    return kEsfSystemManagerResultInternalError;
  }
  strncpy(data_struct->dps_scope_id, data, sizeof(data_struct->dps_scope_id));

  EsfSystemManagerResult result =
      EsfSystemManagerSaveEnrollmentToPsm(handle, &mask, data_struct);

  free(data_struct);

  if (result != kEsfSystemManagerResultOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to save Dps Scope ID to parameter storage "
                     "manager. result=%d",
                     "system_manager.c", __LINE__, result);
    (void)EsfParameterStorageManagerClose(handle);
    return result;
  }

  status = EsfParameterStorageManagerClose(handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to close parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  return kEsfSystemManagerResultOk;
}

EsfSystemManagerResult EsfSystemManagerGetProjectId(char *data,
                                                    size_t *data_size) {
  if ((data == (char *)NULL) || (data_size == (size_t *)NULL) ||
      (*data_size == 0)) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Parameter error. data=%p data_size=%p *data_size=%zu",
        "system_manager.c", __LINE__, data, data_size,
        data_size ? *data_size : 0);
    return kEsfSystemManagerResultParamError;
  }

  EsfParameterStorageManagerHandle handle =
      ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE;
  EsfParameterStorageManagerStatus status =
      EsfParameterStorageManagerOpen(&handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to open parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  uint32_t loadable_size = 0;
  status = EsfParameterStorageManagerGetSize(
      handle, kEsfParameterStorageManagerItemProjectID, &loadable_size);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to get size parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    (void)EsfParameterStorageManagerClose(handle);
    return kEsfSystemManagerResultInternalError;
  }

  if (*data_size < (loadable_size + SYSTEM_MANAGER_NULL_TERMINATED_SIZE)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:data_size=%lu is less than loadable_size=%d",
                     "system_manager.c", __LINE__, *data_size,
                     loadable_size + SYSTEM_MANAGER_NULL_TERMINATED_SIZE);
    (void)EsfParameterStorageManagerClose(handle);
    return kEsfSystemManagerResultOutOfRange;
  }

  EsfParameterStorageManagerEnrollmentMask mask = {.project_id = 1};

  EsfParameterStorageManagerEnrollment *data_struct =
      calloc(1, sizeof(EsfParameterStorageManagerEnrollment));
  if (data_struct == NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Failed to calloc.",
                     "system_manager.c", __LINE__);
    (void)EsfParameterStorageManagerClose(handle);
    return kEsfSystemManagerResultInternalError;
  }

  EsfSystemManagerResult result =
      EsfSystemManagerLoadEnrollmentFromPsm(handle, &mask, data_struct);
  if (result != kEsfSystemManagerResultOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to load Project ID from parameter storage "
                     "manager. result=%d",
                     "system_manager.c", __LINE__, result);
    free(data_struct);
    (void)EsfParameterStorageManagerClose(handle);
    return result;
  }

  status = EsfParameterStorageManagerClose(handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to close parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    free(data_struct);
    return kEsfSystemManagerResultInternalError;
  }

  size_t load_size = strlen(data_struct->project_id) +
                     SYSTEM_MANAGER_NULL_TERMINATED_SIZE;
  if (load_size > *data_size) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:data_size is insufficient for load_size. "
                     "data_size=%zu load_size=%zu",
                     "system_manager.c", __LINE__, *data_size, load_size);
    free(data_struct);
    return kEsfSystemManagerResultOutOfRange;
  }

  strncpy(data, data_struct->project_id, *data_size);
  free(data_struct);
  *data_size = load_size;

  return kEsfSystemManagerResultOk;
}

EsfSystemManagerResult EsfSystemManagerSetProjectId(const char *data,
                                                    size_t data_size) {
  if ((data == (char *)NULL) || (data_size == 0) ||
      (data_size > ESF_SYSTEM_MANAGER_PROJECT_ID_MAX_SIZE)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Parameter error. data=%p data_size=%zu",
                     "system_manager.c", __LINE__, data, data_size);
    return kEsfSystemManagerResultParamError;
  }

  if (strnlen(data, data_size) == data_size) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:NULL-terminated character does not exist",
                     "system_manager.c", __LINE__);
    return kEsfSystemManagerResultParamError;
  }

  EsfParameterStorageManagerHandle handle =
      ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE;
  EsfParameterStorageManagerStatus status =
      EsfParameterStorageManagerOpen(&handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to open parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  EsfParameterStorageManagerEnrollmentMask mask = {.project_id = 1};

  EsfParameterStorageManagerEnrollment *data_struct =
      calloc(1, sizeof(EsfParameterStorageManagerEnrollment));
  if (data_struct == NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Failed to calloc.",
                     "system_manager.c", __LINE__);
    (void)EsfParameterStorageManagerClose(handle);
    return kEsfSystemManagerResultInternalError;
  }
  strncpy(data_struct->project_id, data, sizeof(data_struct->project_id));

  EsfSystemManagerResult result =
      EsfSystemManagerSaveEnrollmentToPsm(handle, &mask, data_struct);

  free(data_struct);

  if (result != kEsfSystemManagerResultOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to save Project ID to parameter storage "
                     "manager. result=%d",
                     "system_manager.c", __LINE__, result);
    (void)EsfParameterStorageManagerClose(handle);
    return result;
  }

  status = EsfParameterStorageManagerClose(handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to close parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  return kEsfSystemManagerResultOk;
}

EsfSystemManagerResult EsfSystemManagerGetRegisterToken(char *data,
                                                        size_t *data_size) {
  if ((data == (char *)NULL) || (data_size == (size_t *)NULL) ||
      (*data_size == 0)) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Parameter error. data=%p data_size=%p *data_size=%zu",
        "system_manager.c", __LINE__, data, data_size,
        data_size ? *data_size : 0);
    return kEsfSystemManagerResultParamError;
  }

  EsfParameterStorageManagerHandle handle =
      ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE;
  EsfParameterStorageManagerStatus status =
      EsfParameterStorageManagerOpen(&handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to open parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  uint32_t loadable_size = 0;
  status = EsfParameterStorageManagerGetSize(
      handle, kEsfParameterStorageManagerItemRegisterToken, &loadable_size);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to get size parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    (void)EsfParameterStorageManagerClose(handle);
    return kEsfSystemManagerResultInternalError;
  }

  if (*data_size < (loadable_size + SYSTEM_MANAGER_NULL_TERMINATED_SIZE)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:data_size=%lu is less than loadable_size=%d",
                     "system_manager.c", __LINE__, *data_size,
                     loadable_size + SYSTEM_MANAGER_NULL_TERMINATED_SIZE);
    (void)EsfParameterStorageManagerClose(handle);
    return kEsfSystemManagerResultOutOfRange;
  }

  EsfParameterStorageManagerEnrollmentMask mask = {.register_token = 1};

  EsfParameterStorageManagerEnrollment *data_struct =
      calloc(1, sizeof(EsfParameterStorageManagerEnrollment));
  if (data_struct == NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Failed to calloc.",
                     "system_manager.c", __LINE__);
    (void)EsfParameterStorageManagerClose(handle);
    return kEsfSystemManagerResultInternalError;
  }

  EsfSystemManagerResult result =
      EsfSystemManagerLoadEnrollmentFromPsm(handle, &mask, data_struct);
  if (result != kEsfSystemManagerResultOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to load Register Token from parameter storage "
        "manager. result=%d",
        "system_manager.c", __LINE__, result);
    free(data_struct);
    (void)EsfParameterStorageManagerClose(handle);
    return result;
  }

  status = EsfParameterStorageManagerClose(handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to close parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    free(data_struct);
    return kEsfSystemManagerResultInternalError;
  }

  size_t load_size = strlen(data_struct->register_token) +
                     SYSTEM_MANAGER_NULL_TERMINATED_SIZE;
  if (load_size > *data_size) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:data_size is insufficient for load_size. "
                     "data_size=%zu load_size=%zu",
                     "system_manager.c", __LINE__, *data_size, load_size);
    free(data_struct);
    return kEsfSystemManagerResultOutOfRange;
  }

  strncpy(data, data_struct->register_token, *data_size);
  free(data_struct);
  *data_size = load_size;

  return kEsfSystemManagerResultOk;
}

EsfSystemManagerResult EsfSystemManagerSetRegisterToken(const char *data,
                                                        size_t data_size) {
  if ((data == (char *)NULL) || (data_size == 0) ||
      (data_size > ESF_SYSTEM_MANAGER_REGISTER_TOKEN_MAX_SIZE)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Parameter error. data=%p data_size=%zu",
                     "system_manager.c", __LINE__, data, data_size);
    return kEsfSystemManagerResultParamError;
  }

  if (strnlen(data, data_size) == data_size) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:NULL-terminated character does not exist",
                     "system_manager.c", __LINE__);
    return kEsfSystemManagerResultParamError;
  }

  EsfParameterStorageManagerHandle handle =
      ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE;
  EsfParameterStorageManagerStatus status =
      EsfParameterStorageManagerOpen(&handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to open parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  EsfParameterStorageManagerEnrollmentMask mask = {.register_token = 1};

  EsfParameterStorageManagerEnrollment *data_struct =
      calloc(1, sizeof(EsfParameterStorageManagerEnrollment));
  if (data_struct == NULL) {
    (void)EsfParameterStorageManagerClose(handle);
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Failed to calloc.",
                     "system_manager.c", __LINE__);
    return kEsfSystemManagerResultInternalError;
  }
  strncpy(data_struct->register_token, data,
          sizeof(data_struct->register_token));

  EsfSystemManagerResult result =
      EsfSystemManagerSaveEnrollmentToPsm(handle, &mask, data_struct);

  free(data_struct);

  if (result != kEsfSystemManagerResultOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to save Register Token to parameter storage "
                     "manager. result=%d",
                     "system_manager.c", __LINE__, result);
    (void)EsfParameterStorageManagerClose(handle);
    return result;
  }

  status = EsfParameterStorageManagerClose(handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to close parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  return kEsfSystemManagerResultOk;
}

EsfSystemManagerResult EsfSystemManagerGetEvpHubUrl(char *data,
                                                    size_t *data_size) {
  if ((data == (char *)NULL) || (data_size == (size_t *)NULL) ||
      (*data_size == 0)) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Parameter error. data=%p data_size=%p *data_size=%zu",
        "system_manager.c", __LINE__, data, data_size,
        data_size ? *data_size : 0);
    return kEsfSystemManagerResultParamError;
  }

  EsfParameterStorageManagerHandle handle =
      ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE;
  EsfParameterStorageManagerStatus status =
      EsfParameterStorageManagerOpen(&handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to open parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  uint32_t loadable_size = 0;
  status = EsfParameterStorageManagerGetSize(
      handle, kEsfParameterStorageManagerItemEvpHubURL, &loadable_size);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to get size parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    (void)EsfParameterStorageManagerClose(handle);
    return kEsfSystemManagerResultInternalError;
  }

  if (*data_size < (loadable_size + SYSTEM_MANAGER_NULL_TERMINATED_SIZE)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:data_size=%lu is less than loadable_size=%d",
                     "system_manager.c", __LINE__, *data_size,
                     loadable_size + SYSTEM_MANAGER_NULL_TERMINATED_SIZE);
    (void)EsfParameterStorageManagerClose(handle);
    return kEsfSystemManagerResultOutOfRange;
  }

  EsfParameterStorageManagerEvpMask mask = {.evp_url = 1};

  EsfParameterStorageManagerEvp data_struct;
  memset(&data_struct, 0, sizeof(data_struct));

  EsfSystemManagerResult result = EsfSystemManagerLoadEvpFromPsm(handle, &mask,
                                                                 &data_struct);
  if (result != kEsfSystemManagerResultOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to load EVP Hub URL from parameter storage "
                     "manager. result=%d",
                     "system_manager.c", __LINE__, result);
    (void)EsfParameterStorageManagerClose(handle);
    return result;
  }

  status = EsfParameterStorageManagerClose(handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to close parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  size_t load_size = strlen(data_struct.evp_url) +
                     SYSTEM_MANAGER_NULL_TERMINATED_SIZE;
  if (load_size > *data_size) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:data_size is insufficient for load_size. "
                     "data_size=%zu load_size=%zu",
                     "system_manager.c", __LINE__, *data_size, load_size);
    return kEsfSystemManagerResultOutOfRange;
  }

  strncpy(data, data_struct.evp_url, *data_size);
  *data_size = load_size;

  return kEsfSystemManagerResultOk;
}

EsfSystemManagerResult EsfSystemManagerSetEvpHubUrl(const char *data,
                                                    size_t data_size) {
  if ((data == (char *)NULL) || (data_size == 0) ||
      (data_size > ESF_SYSTEM_MANAGER_EVP_HUB_URL_MAX_SIZE)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Parameter error. data=%p data_size=%zu",
                     "system_manager.c", __LINE__, data, data_size);
    return kEsfSystemManagerResultParamError;
  }

  if (strnlen(data, data_size) == data_size) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:NULL-terminated character does not exist",
                     "system_manager.c", __LINE__);
    return kEsfSystemManagerResultParamError;
  }

  EsfParameterStorageManagerHandle handle =
      ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE;
  EsfParameterStorageManagerStatus status =
      EsfParameterStorageManagerOpen(&handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to open parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  EsfParameterStorageManagerEvpMask mask = {.evp_url = 1};

  EsfParameterStorageManagerEvp data_struct;
  strncpy(data_struct.evp_url, data, sizeof(data_struct.evp_url));

  EsfSystemManagerResult result = EsfSystemManagerSaveEvpToPsm(handle, &mask,
                                                               &data_struct);
  if (result != kEsfSystemManagerResultOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to save EVP Hub URL to parameter storage "
                     "manager. result=%d",
                     "system_manager.c", __LINE__, result);
    (void)EsfParameterStorageManagerClose(handle);
    return result;
  }

  status = EsfParameterStorageManagerClose(handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to close parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  return kEsfSystemManagerResultOk;
}

EsfSystemManagerResult EsfSystemManagerGetEvpHubPort(char *data,
                                                     size_t *data_size) {
  if ((data == (char *)NULL) || (data_size == (size_t *)NULL) ||
      (*data_size == 0)) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Parameter error. data=%p data_size=%p *data_size=%zu",
        "system_manager.c", __LINE__, data, data_size,
        data_size ? *data_size : 0);
    return kEsfSystemManagerResultParamError;
  }

  EsfParameterStorageManagerHandle handle =
      ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE;
  EsfParameterStorageManagerStatus status =
      EsfParameterStorageManagerOpen(&handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to open parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  uint32_t loadable_size = 0;
  status = EsfParameterStorageManagerGetSize(
      handle, kEsfParameterStorageManagerItemEvpHubPort, &loadable_size);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to get size parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    (void)EsfParameterStorageManagerClose(handle);
    return kEsfSystemManagerResultInternalError;
  }

  if (*data_size < (loadable_size + SYSTEM_MANAGER_NULL_TERMINATED_SIZE)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:data_size=%lu is less than loadable_size=%d",
                     "system_manager.c", __LINE__, *data_size,
                     loadable_size + SYSTEM_MANAGER_NULL_TERMINATED_SIZE);
    (void)EsfParameterStorageManagerClose(handle);
    return kEsfSystemManagerResultOutOfRange;
  }

  EsfParameterStorageManagerEvpMask mask = {.evp_port = 1};

  EsfParameterStorageManagerEvp data_struct;
  memset(&data_struct, 0, sizeof(data_struct));

  EsfSystemManagerResult result = EsfSystemManagerLoadEvpFromPsm(handle, &mask,
                                                                 &data_struct);
  if (result != kEsfSystemManagerResultOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to load EVP Hub Port from parameter storage "
                     "manager. result=%d",
                     "system_manager.c", __LINE__, result);
    (void)EsfParameterStorageManagerClose(handle);
    return result;
  }

  status = EsfParameterStorageManagerClose(handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to close parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  size_t load_size = strlen(data_struct.evp_port) +
                     SYSTEM_MANAGER_NULL_TERMINATED_SIZE;
  if (load_size > *data_size) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:data_size is insufficient for load_size. "
                     "data_size=%zu load_size=%zu",
                     "system_manager.c", __LINE__, *data_size, load_size);
    return kEsfSystemManagerResultOutOfRange;
  }

  strncpy(data, data_struct.evp_port, *data_size);
  *data_size = load_size;

  return kEsfSystemManagerResultOk;
}

EsfSystemManagerResult EsfSystemManagerSetEvpHubPort(const char *data,
                                                     size_t data_size) {
  if ((data == (char *)NULL) || (data_size == 0) ||
      (data_size > ESF_SYSTEM_MANAGER_EVP_HUB_PORT_MAX_SIZE)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Parameter error. data=%p data_size=%zu",
                     "system_manager.c", __LINE__, data, data_size);
    return kEsfSystemManagerResultParamError;
  }

  if (strnlen(data, data_size) == data_size) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:NULL-terminated character does not exist",
                     "system_manager.c", __LINE__);
    return kEsfSystemManagerResultParamError;
  }

  EsfParameterStorageManagerHandle handle =
      ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE;
  EsfParameterStorageManagerStatus status =
      EsfParameterStorageManagerOpen(&handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to open parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  EsfParameterStorageManagerEvpMask mask = {.evp_port = 1};

  EsfParameterStorageManagerEvp data_struct;
  strncpy(data_struct.evp_port, data, sizeof(data_struct.evp_port));

  EsfSystemManagerResult result = EsfSystemManagerSaveEvpToPsm(handle, &mask,
                                                               &data_struct);
  if (result != kEsfSystemManagerResultOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to save EVP Hub Port to parameter storage "
                     "manager. result=%d",
                     "system_manager.c", __LINE__, result);
    (void)EsfParameterStorageManagerClose(handle);
    return result;
  }

  status = EsfParameterStorageManagerClose(handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to close parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  return kEsfSystemManagerResultOk;
}

EsfSystemManagerResult EsfSystemManagerGetEvpIotPlatform(char *data,
                                                         size_t *data_size) {
  if ((data == (char *)NULL) || (data_size == (size_t *)NULL) ||
      (*data_size == 0)) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Parameter error. data=%p data_size=%p *data_size=%zu",
        "system_manager.c", __LINE__, data, data_size,
        data_size ? *data_size : 0);
    return kEsfSystemManagerResultParamError;
  }

  EsfParameterStorageManagerHandle handle =
      ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE;
  EsfParameterStorageManagerStatus status =
      EsfParameterStorageManagerOpen(&handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to open parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  uint32_t loadable_size = 0;
  status = EsfParameterStorageManagerGetSize(
      handle, kEsfParameterStorageManagerItemEvpIotPlatform, &loadable_size);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to get size parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    (void)EsfParameterStorageManagerClose(handle);
    return kEsfSystemManagerResultInternalError;
  }

  if (*data_size < (loadable_size + SYSTEM_MANAGER_NULL_TERMINATED_SIZE)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:data_size=%lu is less than loadable_size=%d",
                     "system_manager.c", __LINE__, *data_size,
                     loadable_size + SYSTEM_MANAGER_NULL_TERMINATED_SIZE);
    (void)EsfParameterStorageManagerClose(handle);
    return kEsfSystemManagerResultOutOfRange;
  }

  EsfParameterStorageManagerEvpMask mask = {.iot_platform = 1};

  EsfParameterStorageManagerEvp data_struct;
  memset(&data_struct, 0, sizeof(data_struct));

  EsfSystemManagerResult result = EsfSystemManagerLoadEvpFromPsm(handle, &mask,
                                                                 &data_struct);
  if (result != kEsfSystemManagerResultOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to load EVP IoT Platform from parameter storage "
        "manager. result=%d",
        "system_manager.c", __LINE__, result);
    (void)EsfParameterStorageManagerClose(handle);
    return result;
  }

  status = EsfParameterStorageManagerClose(handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to close parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  size_t load_size = strlen(data_struct.iot_platform) +
                     SYSTEM_MANAGER_NULL_TERMINATED_SIZE;
  if (load_size > *data_size) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:data_size is insufficient for load_size. "
                     "data_size=%zu load_size=%zu",
                     "system_manager.c", __LINE__, *data_size, load_size);
    return kEsfSystemManagerResultOutOfRange;
  }

  strncpy(data, data_struct.iot_platform, *data_size);
  *data_size = load_size;

  return kEsfSystemManagerResultOk;
}

EsfSystemManagerResult EsfSystemManagerSetEvpIotPlatform(const char *data,
                                                         size_t data_size) {
  if ((data == (char *)NULL) || (data_size == 0) ||
      (data_size > ESF_SYSTEM_MANAGER_IOT_PLATFORM_MAX_SIZE)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Parameter error. data=%p data_size=%zu",
                     "system_manager.c", __LINE__, data, data_size);
    return kEsfSystemManagerResultParamError;
  }

  if (strnlen(data, data_size) == data_size) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:NULL-terminated character does not exist",
                     "system_manager.c", __LINE__);
    return kEsfSystemManagerResultParamError;
  }

  EsfParameterStorageManagerHandle handle =
      ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE;
  EsfParameterStorageManagerStatus status =
      EsfParameterStorageManagerOpen(&handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to open parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  EsfParameterStorageManagerEvpMask mask = {.iot_platform = 1};

  EsfParameterStorageManagerEvp data_struct;
  strncpy(data_struct.iot_platform, data, sizeof(data_struct.iot_platform));

  EsfSystemManagerResult result = EsfSystemManagerSaveEvpToPsm(handle, &mask,
                                                               &data_struct);
  if (result != kEsfSystemManagerResultOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to save EVP IoT Platform to parameter storage "
        "manager. result=%d",
        "system_manager.c", __LINE__, result);
    (void)EsfParameterStorageManagerClose(handle);
    return result;
  }

  status = EsfParameterStorageManagerClose(handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to close parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  return kEsfSystemManagerResultOk;
}

EsfSystemManagerResult EsfSystemManagerGetEvpTls(
    EsfSystemManagerEvpTlsValue *data) {
  if (data == (EsfSystemManagerEvpTlsValue *)NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Parameter error. data=%p",
                     "system_manager.c", __LINE__, data);
    return kEsfSystemManagerResultParamError;
  }

  EsfParameterStorageManagerHandle handle =
      ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE;
  EsfParameterStorageManagerStatus status =
      EsfParameterStorageManagerOpen(&handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to open parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  EsfParameterStorageManagerEvpMask mask = {.evp_tls = 1};

  EsfParameterStorageManagerEvp data_struct;
  memset(&data_struct, 0, sizeof(data_struct));

  EsfSystemManagerResult result = EsfSystemManagerLoadEvpFromPsm(handle, &mask,
                                                                 &data_struct);
  if (result != kEsfSystemManagerResultOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to load EVP TLS from parameter storage "
                     "manager. result=%d",
                     "system_manager.c", __LINE__, result);
    (void)EsfParameterStorageManagerClose(handle);
    return result;
  }

  status = EsfParameterStorageManagerClose(handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to close parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  if (strncmp(data_struct.evp_tls, "0", 1) == 0) {
    // "0" means TLS on
    *data = kEsfSystemManagerEvpTlsEnable;
  } else if (strncmp(data_struct.evp_tls, "1", 1) == 0) {
    // "1" means TLS off
    *data = kEsfSystemManagerEvpTlsDisable;
  } else {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Loaded data is invalid.",
                     "system_manager.c", __LINE__);
    return kEsfSystemManagerResultOutOfRange;
  }

  return kEsfSystemManagerResultOk;
}

EsfSystemManagerResult EsfSystemManagerSetEvpTls(
    EsfSystemManagerEvpTlsValue data) {
  EsfParameterStorageManagerEvp data_struct;
  switch (data) {
    case kEsfSystemManagerEvpTlsDisable:
      // "1" means TLS off
      strncpy(data_struct.evp_tls, "1", sizeof(data_struct.evp_tls));
      break;
    case kEsfSystemManagerEvpTlsEnable:
      // "0" means TLS on
      strncpy(data_struct.evp_tls, "0", sizeof(data_struct.evp_tls));
      break;
    default:
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Parameter error. data=%d",
                       "system_manager.c", __LINE__, data);
      return kEsfSystemManagerResultParamError;
  }

  EsfParameterStorageManagerHandle handle =
      ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE;
  EsfParameterStorageManagerStatus status =
      EsfParameterStorageManagerOpen(&handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to open parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  EsfParameterStorageManagerEvpMask mask = {.evp_tls = 1};

  EsfSystemManagerResult result = EsfSystemManagerSaveEvpToPsm(handle, &mask,
                                                               &data_struct);
  if (result != kEsfSystemManagerResultOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to save EVP TLS to parameter storage "
                     "manager. result=%d",
                     "system_manager.c", __LINE__, result);
    (void)EsfParameterStorageManagerClose(handle);
    return result;
  }

  status = EsfParameterStorageManagerClose(handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to close parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  return kEsfSystemManagerResultOk;
}

EsfSystemManagerResult EsfSystemManagerGetRootCa(char *data,
                                                 size_t *data_size) {
  if ((data == (char *)NULL) || (data_size == (size_t *)NULL) ||
      (*data_size == 0)) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Parameter error. data=%p data_size=%p *data_size=%zu",
        "system_manager.c", __LINE__, data, data_size,
        data_size ? *data_size : 0);
    return kEsfSystemManagerResultParamError;
  }

  EsfParameterStorageManagerHandle handle =
      ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE;
  EsfParameterStorageManagerStatus status =
      EsfParameterStorageManagerOpen(&handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to open parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  uint32_t loadable_size = 0;
  status = EsfParameterStorageManagerGetSize(
      handle, kEsfParameterStorageManagerItemPkiRootCerts, &loadable_size);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to get size parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    (void)EsfParameterStorageManagerClose(handle);
    return kEsfSystemManagerResultInternalError;
  }

  if (loadable_size == 0) {
    WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (uint16_t)0x8d11);
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Root CA is empty.",
                     "system_manager.c", __LINE__);
    (void)EsfParameterStorageManagerClose(handle);
    return kEsfSystemManagerResultEmptyData;
  }

  if (*data_size < (loadable_size + SYSTEM_MANAGER_NULL_TERMINATED_SIZE)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:data_size=%lu is less than loadable_size=%d",
                     "system_manager.c", __LINE__, *data_size,
                     loadable_size + SYSTEM_MANAGER_NULL_TERMINATED_SIZE);
    (void)EsfParameterStorageManagerClose(handle);
    return kEsfSystemManagerResultOutOfRange;
  }

  EsfParameterStorageManagerRootAuthMask mask = {.root_ca = 1};

  EsfParameterStorageManagerRootAuth *data_struct =
      calloc(1, sizeof(EsfParameterStorageManagerRootAuth));
  if (data_struct == NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Failed to calloc.",
                     "system_manager.c", __LINE__);
    (void)EsfParameterStorageManagerClose(handle);
    return kEsfSystemManagerResultInternalError;
  }
  data_struct->root_ca.offset = 0;
  data_struct->root_ca.size = loadable_size;
  data_struct->root_ca.data = (uint8_t *)data;

  EsfSystemManagerResult result =
      EsfSystemManagerLoadRootAuthFromPsm(handle, &mask, data_struct);
  if (result != kEsfSystemManagerResultOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to load Root CA from parameter storage "
                     "manager. result=%d",
                     "system_manager.c", __LINE__, result);
    free(data_struct);
    (void)EsfParameterStorageManagerClose(handle);
    return result;
  }

  status = EsfParameterStorageManagerClose(handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to close parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    free(data_struct);
    return kEsfSystemManagerResultInternalError;
  }

  // If there is no null character, it is given.
  if (((char *)(data_struct->root_ca.data))[data_struct->root_ca.size - 1] !=
      '\0') {
    if (data_struct->root_ca.size < *data_size) {
      // Insert Null character at the end.
      ((char *)(data_struct->root_ca.data))[data_struct->root_ca.size] = '\0';
      ++data_struct->root_ca.size;
    } else {
      WRITE_DLOG_ERROR(
          MODULE_ID_SYSTEM, "%s-%d:data_size=%lu data_struct->root_ca.size=%d",
          "system_manager.c", __LINE__, *data_size, data_struct->root_ca.size);
      free(data_struct);
      return kEsfSystemManagerResultOutOfRange;
    }
  }

  *data_size = data_struct->root_ca.size;
  free(data_struct);

  return kEsfSystemManagerResultOk;
}

EsfSystemManagerResult EsfSystemManagerGetRootCaHash(char *data,
                                                     size_t *data_size) {
  if ((data == (char *)NULL) || (data_size == (size_t *)NULL) ||
      (*data_size == 0)) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Parameter error. data=%p data_size=%p *data_size=%zu",
        "system_manager.c", __LINE__, data, data_size,
        data_size ? *data_size : 0);
    return kEsfSystemManagerResultParamError;
  }

  EsfParameterStorageManagerHandle handle =
      ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE;
  EsfParameterStorageManagerStatus status =
      EsfParameterStorageManagerOpen(&handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to open parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  uint32_t loadable_size = 0;
  status = EsfParameterStorageManagerGetSize(
      handle, kEsfParameterStorageManagerItemPkiRootCertsHash, &loadable_size);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to get size parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    (void)EsfParameterStorageManagerClose(handle);
    return kEsfSystemManagerResultInternalError;
  }

  if (*data_size < (loadable_size + SYSTEM_MANAGER_NULL_TERMINATED_SIZE)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:data_size=%lu is less than loadable_size=%d",
                     "system_manager.c", __LINE__, *data_size,
                     loadable_size + SYSTEM_MANAGER_NULL_TERMINATED_SIZE);
    (void)EsfParameterStorageManagerClose(handle);
    return kEsfSystemManagerResultOutOfRange;
  }

  EsfParameterStorageManagerRootAuthMask mask = {.root_ca_hash = 1};

  EsfParameterStorageManagerRootAuth *data_struct =
      calloc(1, sizeof(EsfParameterStorageManagerRootAuth));
  if (data_struct == NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Failed to calloc.",
                     "system_manager.c", __LINE__);
    (void)EsfParameterStorageManagerClose(handle);
    return kEsfSystemManagerResultInternalError;
  }

  EsfSystemManagerResult result =
      EsfSystemManagerLoadRootAuthFromPsm(handle, &mask, data_struct);
  if (result != kEsfSystemManagerResultOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to load Root CA Hash from parameter storage "
                     "manager. result=%d",
                     "system_manager.c", __LINE__, result);
    free(data_struct);
    (void)EsfParameterStorageManagerClose(handle);
    return result;
  }

  status = EsfParameterStorageManagerClose(handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to close parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    free(data_struct);
    return kEsfSystemManagerResultInternalError;
  }

  size_t load_size = strlen(data_struct->root_ca_hash) +
                     SYSTEM_MANAGER_NULL_TERMINATED_SIZE;
  if (load_size > *data_size) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:data_size is insufficient for load_size. "
                     "data_size=%zu load_size=%zu",
                     "system_manager.c", __LINE__, *data_size, load_size);
    free(data_struct);
    return kEsfSystemManagerResultOutOfRange;
  }

  strncpy(data, data_struct->root_ca_hash, *data_size);
  free(data_struct);
  *data_size = load_size;

  return kEsfSystemManagerResultOk;
}

EsfSystemManagerResult EsfSystemManagerGetQrModeTimeoutValue(int32_t *data) {
  if (data == (int32_t *)NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Parameter error. data=%p",
                     "system_manager.c", __LINE__, data);
    return kEsfSystemManagerResultParamError;
  }

  EsfParameterStorageManagerHandle handle =
      ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE;
  EsfParameterStorageManagerStatus status =
      EsfParameterStorageManagerOpen(&handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to open parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  EsfParameterStorageManagerQrModeTimeoutValueMask mask = {
      .qr_mode_timeout_value = 1};

  EsfParameterStorageManagerQrModeTimeoutValue data_struct;
  memset(&data_struct, 0, sizeof(data_struct));

  EsfSystemManagerResult result = EsfSystemManagerLoadQrModeTimeoutValueFromPsm(
      handle, &mask, &data_struct);
  if (result != kEsfSystemManagerResultOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to load QR mode timeout value from parameter storage "
        "manager. result=%d",
        "system_manager.c", __LINE__, result);
    (void)EsfParameterStorageManagerClose(handle);
    return result;
  }

  status = EsfParameterStorageManagerClose(handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to close parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  *data = data_struct.qr_mode_timeout_value.data;

  return kEsfSystemManagerResultOk;
}

EsfSystemManagerResult EsfSystemManagerSetQrModeTimeoutValue(int32_t data) {
  EsfParameterStorageManagerHandle handle =
      ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE;
  EsfParameterStorageManagerStatus status =
      EsfParameterStorageManagerOpen(&handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to open parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  EsfParameterStorageManagerQrModeTimeoutValueMask mask = {
      .qr_mode_timeout_value = 1};

  EsfParameterStorageManagerQrModeTimeoutValue data_struct;
  data_struct.qr_mode_timeout_value.data = data;

  EsfSystemManagerResult result =
      EsfSystemManagerSaveQrModeTimeoutValueToPsm(handle, &mask, &data_struct);
  if (result != kEsfSystemManagerResultOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to save QR mode timeout value to parameter storage "
        "manager. result=%d",
        "system_manager.c", __LINE__, result);
    (void)EsfParameterStorageManagerClose(handle);
    return result;
  }

  status = EsfParameterStorageManagerClose(handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to close parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  return kEsfSystemManagerResultOk;
}

EsfSystemManagerResult EsfSystemManagerGetInitialSettingFlag(
    EsfSystemManagerInitialSettingFlag *data) {
  if (data == (EsfSystemManagerInitialSettingFlag *)NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Parameter error. data=%p",
                     "system_manager.c", __LINE__, data);
    return kEsfSystemManagerResultParamError;
  }

  EsfParameterStorageManagerHandle handle =
      ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE;
  EsfParameterStorageManagerStatus status =
      EsfParameterStorageManagerOpen(&handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to open parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  EsfParameterStorageManagerInitialSettingFlagMask mask = {
      .initial_setting_flag = 1};

  EsfParameterStorageManagerInitialSettingFlag data_struct;
  memset(&data_struct, 0, sizeof(data_struct));

  EsfSystemManagerResult result = EsfSystemManagerLoadInitialSettingFlagFromPsm(
      handle, &mask, &data_struct);
  if (result != kEsfSystemManagerResultOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to load Initial Setting Flag from parameter storage "
        "manager. result=%d",
        "system_manager.c", __LINE__, result);
    (void)EsfParameterStorageManagerClose(handle);
    return result;
  }

  status = EsfParameterStorageManagerClose(handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to close parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  switch (data_struct.initial_setting_flag.data) {
    case 0:
      *data = kEsfSystemManagerInitialSettingNotCompleted;
      break;
    case 1:
      *data = kEsfSystemManagerInitialSettingCompleted;
      break;
    default:
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Loaded data is invalid.",
                       "system_manager.c", __LINE__);
      return kEsfSystemManagerResultOutOfRange;
  }

  return kEsfSystemManagerResultOk;
}

EsfSystemManagerResult EsfSystemManagerSetInitialSettingFlag(
    EsfSystemManagerInitialSettingFlag data) {
  EsfParameterStorageManagerInitialSettingFlag data_struct;
  switch (data) {
    case kEsfSystemManagerInitialSettingNotCompleted:
      data_struct.initial_setting_flag.data = 0;
      break;
    case kEsfSystemManagerInitialSettingCompleted:
      data_struct.initial_setting_flag.data = 1;
      break;
    default:
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Parameter error. data=%d",
                       "system_manager.c", __LINE__, data);
      return kEsfSystemManagerResultParamError;
  }

  EsfParameterStorageManagerHandle handle =
      ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE;
  EsfParameterStorageManagerStatus status =
      EsfParameterStorageManagerOpen(&handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to open parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  EsfParameterStorageManagerInitialSettingFlagMask mask = {
      .initial_setting_flag = 1};

  EsfSystemManagerResult result =
      EsfSystemManagerSaveInitialSettingFlagToPsm(handle, &mask, &data_struct);
  if (result != kEsfSystemManagerResultOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to save EVP TLS to parameter storage "
                     "manager. result=%d",
                     "system_manager.c", __LINE__, result);
    (void)EsfParameterStorageManagerClose(handle);
    return result;
  }

  status = EsfParameterStorageManagerClose(handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to close parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  return kEsfSystemManagerResultOk;
}

EsfSystemManagerResult EsfSystemManagerGetHwInfo(EsfSystemManagerHwInfo *data) {
  if (data == NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Parameter error. data is NULL.",
                     "system_manager.c", __LINE__);
    return kEsfSystemManagerResultParamError;
  }

  EsfSystemManagerResult result = kEsfSystemManagerResultOk;
  EsfParameterStorageManagerHwInfo data_struct = {0};
  char *hw_info = NULL;

  if (PlSystemManagerIsHwInfoSupported()) {
    EsfParameterStorageManagerHandle handle =
        ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE;
    EsfParameterStorageManagerStatus status =
        EsfParameterStorageManagerOpen(&handle);
    if (status != kEsfParameterStorageManagerStatusOk) {
      WRITE_DLOG_ERROR(
          MODULE_ID_SYSTEM,
          "%s-%d:Failed to open parameter storage manager. status=%d",
          "system_manager.c", __LINE__, status);
      return kEsfSystemManagerResultInternalError;
    }

    EsfParameterStorageManagerHwInfoMask mask = {.hw_info = 1};
    size_t hw_info_size = PlSystemManagerGetHwInfoSize();
    hw_info = (char *)calloc(1, hw_info_size);
    if (hw_info == NULL) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Failed to calloc.",
                       "system_manager.c", __LINE__);
      (void)EsfParameterStorageManagerClose(handle);
      return kEsfSystemManagerResultInternalError;
    }

    data_struct.hw_info.size = hw_info_size;
    data_struct.hw_info.data = (uint8_t *)hw_info;

    result = EsfSystemManagerLoadHwInfoFromPsm(handle, &mask, &data_struct);

    if (result != kEsfSystemManagerResultOk) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:Failed to load HW info from parameter storage "
                       "manager. result=%d",
                       "system_manager.c", __LINE__, result);
      free(hw_info);
      (void)EsfParameterStorageManagerClose(handle);
      return result;
    }

    status = EsfParameterStorageManagerClose(handle);
    if (status != kEsfParameterStorageManagerStatusOk) {
      WRITE_DLOG_ERROR(
          MODULE_ID_SYSTEM,
          "%s-%d:Failed to close parameter storage manager. status=%d",
          "system_manager.c", __LINE__, status);
      free(hw_info);
      return kEsfSystemManagerResultInternalError;
    }
  }

  result = EsfSystemManagerParseHwInfo(data, &data_struct);

  free(hw_info);

  if (result != kEsfSystemManagerResultOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to parse HW info. result=%d",
                     "system_manager.c", __LINE__, result);
    return result;
  }

  return kEsfSystemManagerResultOk;
}

EsfSystemManagerResult EsfSystemManagerGetEvpResetCause(
    EsfSystemManagerEvpResetCause *evp_reset_cause) {
#ifdef CONFIG_EXTERNAL_SYSTEM_MANAGER_RESET_CAUSE_DISABLE
  return kEsfSystemManagerResultOk;
#else
  if (evp_reset_cause == NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Parameter error. evp_reset_cause is NULL.",
                     "system_manager.c", __LINE__);
    return kEsfSystemManagerResultParamError;
  }

  EsfParameterStorageManagerHandle handle =
      ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE;
  EsfParameterStorageManagerStatus status =
      EsfParameterStorageManagerOpen(&handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to open parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  EsfParameterStorageManagerResetCauseMask mask = {.evp_reset_cause = 1};
  EsfParameterStorageManagerResetCause data_struct = {.evp_reset_cause = ""};

  EsfSystemManagerResult result =
      EsfSystemManagerLoadResetCauseFromPsm(handle, &mask, &data_struct);
  if (result != kEsfSystemManagerResultOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to load EvpResetCause from parameter storage "
        "manager. result=%d",
        "system_manager.c", __LINE__, result);
    (void)EsfParameterStorageManagerClose(handle);
    return kEsfSystemManagerResultInternalError;
  }

  status = EsfParameterStorageManagerClose(handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to close parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  result = EsfSystemManagerConvertStringToEvpResetCause(
      data_struct.evp_reset_cause, evp_reset_cause);
  if (result != kEsfSystemManagerResultOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to convert string to EvpResetCause. "
                     "result=%d",
                     "system_manager.c", __LINE__, result);
    return kEsfSystemManagerResultInternalError;
  }

  return kEsfSystemManagerResultOk;
#endif  // CONFIG_EXTERNAL_SYSTEM_MANAGER_RESET_CAUSE_DISABLE
}

EsfSystemManagerResult EsfSystemManagerSetEvpResetCause(
    EsfSystemManagerEvpResetCause evp_reset_cause) {
#ifdef CONFIG_EXTERNAL_SYSTEM_MANAGER_RESET_CAUSE_DISABLE
  return kEsfSystemManagerResultOk;
#else
  if ((evp_reset_cause < kEsfSystemManagerEvpResetCauseClear) ||
      (evp_reset_cause >= kEsfSystemManagerEvpResetCauseMax)) {
    return kEsfSystemManagerResultParamError;
  }

  EsfParameterStorageManagerHandle handle =
      ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE;
  EsfParameterStorageManagerStatus status =
      EsfParameterStorageManagerOpen(&handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to open parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  /* write flash EvpResetCause */
  EsfParameterStorageManagerResetCauseMask mask = {.evp_reset_cause = 1};
  EsfParameterStorageManagerResetCause data_struct = {.evp_reset_cause = ""};

  if (evp_reset_cause != kEsfSystemManagerEvpResetCauseClear) {
    // If the evp_reset_cause is not disable, the value is set to the
    // evp_reset_cause.
    // If the evp_reset_cause is disable, the value is set to empty.
    data_struct.evp_reset_cause[0] = '0' + (evp_reset_cause % 10);
    data_struct.evp_reset_cause[1] = '\0';
  }

  EsfSystemManagerResult result =
      EsfSystemManagerSaveResetCauseToPsm(handle, &mask, &data_struct);
  if (result != kEsfSystemManagerResultOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to save EvpResetCause to parameter storage "
                     "manager. result=%d",
                     "system_manager.c", __LINE__, result);
    (void)EsfParameterStorageManagerClose(handle);
    return result;
  }

  status = EsfParameterStorageManagerClose(handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to close parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  return kEsfSystemManagerResultOk;
#endif  // CONFIG_EXTERNAL_SYSTEM_MANAGER_RESET_CAUSE_DISABLE
}

EsfSystemManagerResult EsfSystemManagerGetResetCause(
    EsfSystemManagerResetCause *reset_cause) {
#ifdef CONFIG_EXTERNAL_SYSTEM_MANAGER_RESET_CAUSE_DISABLE
  return kEsfSystemManagerResultOk;
#else
  if (reset_cause == NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Parameter error. reset_cause is NULL.",
                     "system_manager.c", __LINE__);
    return kEsfSystemManagerResultParamError;
  }

  EsfParameterStorageManagerHandle handle =
      ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE;
  EsfParameterStorageManagerStatus status =
      EsfParameterStorageManagerOpen(&handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to open parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  EsfParameterStorageManagerResetCauseMask mask = {.reset_cause = 1};
  EsfParameterStorageManagerResetCause data_struct = {.reset_cause = ""};

  EsfSystemManagerResult result =
      EsfSystemManagerLoadResetCauseFromPsm(handle, &mask, &data_struct);
  if (result != kEsfSystemManagerResultOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to load ResetCause from parameter storage "
                     "manager. result=%d",
                     "system_manager.c", __LINE__, result);
    (void)EsfParameterStorageManagerClose(handle);
    return kEsfSystemManagerResultInternalError;
  }

  status = EsfParameterStorageManagerClose(handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to close parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  result = EsfSystemManagerConvertStringToResetCause(data_struct.reset_cause,
                                                     reset_cause);
  if (result != kEsfSystemManagerResultOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to convert string to ResetCause. "
                     "result=%d",
                     "system_manager.c", __LINE__, result);
    return result;
  }

  return kEsfSystemManagerResultOk;
#endif  // CONFIG_EXTERNAL_SYSTEM_MANAGER_RESET_CAUSE_DISABLE
}

EsfSystemManagerResult EsfSystemManagerSetResetCause(
    EsfSystemManagerResetCause reset_cause) {
#ifdef CONFIG_EXTERNAL_SYSTEM_MANAGER_RESET_CAUSE_DISABLE
  return kEsfSystemManagerResultOk;
#else
  if ((reset_cause <= kEsfSystemManagerResetCauseUnknown) ||
      (reset_cause >= kEsfSystemManagerResetCauseMax)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Parameter error. reset_cause=%d",
                     "system_manager.c", __LINE__, reset_cause);
    return kEsfSystemManagerResultParamError;
  }

  EsfParameterStorageManagerHandle handle =
      ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE;
  EsfParameterStorageManagerStatus status =
      EsfParameterStorageManagerOpen(&handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to open parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  EsfParameterStorageManagerResetCauseMask mask = {.reset_cause = 1};
  EsfParameterStorageManagerResetCause data_struct = {.reset_cause = ""};

  EsfSystemManagerResult result = EsfSystemManagerConvertResetCauseToString(
      reset_cause, data_struct.reset_cause, sizeof(data_struct.reset_cause));
  if (result != kEsfSystemManagerResultOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to convert ResetCause to string. result=%d",
                     "system_manager.c", __LINE__, result);
    (void)EsfParameterStorageManagerClose(handle);
    return result;
  }

  result = EsfSystemManagerSaveResetCauseToPsm(handle, &mask, &data_struct);
  if (result != kEsfSystemManagerResultOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to save ResetCause to parameter storage "
                     "manager. result=%d",
                     "system_manager.c", __LINE__, result);
    (void)EsfParameterStorageManagerClose(handle);
    return kEsfSystemManagerResultInternalError;
  }

  status = EsfParameterStorageManagerClose(handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to close parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  return kEsfSystemManagerResultOk;
#endif  // CONFIG_EXTERNAL_SYSTEM_MANAGER_RESET_CAUSE_DISABLE
}

EsfSystemManagerResult EsfSystemManagerSetExceptionInfo(void) {
  EsfSystemManagerResult result = kEsfSystemManagerResultOk;

  /* get PowerManager ResetCause */
  EsfPwrMgrResetCause pm_reset_cause = kEsfPwrMgrResetCauseUnknown;
  EsfPwrMgrError pm_result = EsfPwrMgrGetResetCause(&pm_reset_cause);
  if (pm_result != kEsfPwrMgrOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to get ResetCause. pm_result=%d",
                     "system_manager.c", __LINE__, pm_result);
    return kEsfSystemManagerResultInternalError;
  }

  WRITE_DLOG_INFO(MODULE_ID_SYSTEM, "%s-%d:ResetCause is %d.",
                  "system_manager.c", __LINE__, pm_reset_cause);

  EsfSystemManagerResetCause get_sm_reset_cause =
      kEsfSystemManagerResetCauseUnknown;

  if (pm_reset_cause == kEsfPwrMgrResetCauseWDT) {
#ifndef CONFIG_EXTERNAL_SYSTEM_MANAGER_EXCEPTION_UPLOAD_DISABLE
    WRITE_DLOG_INFO(MODULE_ID_SYSTEM,
                    "%s-%d:Save ExceptionInfo to parameter storage manager.",
                    "system_manager.c", __LINE__);

    /* get ExceptionInfo */
    struct EsfPwrMgrExceptionInfo *info = NULL;
    uint32_t info_size = 0;
    pm_result = EsfPwrMgrGetExceptionInfo(&info, &info_size);
    if (pm_result != kEsfPwrMgrOk) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:Failed to set ExceptionInfo. pm_result=%d",
                       "system_manager.c", __LINE__, pm_result);
      return kEsfSystemManagerResultInternalError;
    }

    {
      /* write flash ExceptionInfo */
      EsfParameterStorageManagerHandle handle =
          ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE;
      EsfParameterStorageManagerStatus status =
          EsfParameterStorageManagerOpen(&handle);
      if (status != kEsfParameterStorageManagerStatusOk) {
        WRITE_DLOG_ERROR(
            MODULE_ID_SYSTEM,
            "%s-%d:Failed to open parameter storage manager. status=%d",
            "system_manager.c", __LINE__, status);
        return kEsfSystemManagerResultInternalError;
      }

      EsfParameterStorageManagerExceptionInfoMask mask = {.exception_info = 1};
      EsfParameterStorageManagerExceptionInfo data_struct = {
          .exception_info = {.size = info_size, .data = (uint8_t *)info}};

      result = EsfSystemManagerSaveExceptionInfoToPsm(handle, &mask,
                                                      &data_struct);
      if (result != kEsfSystemManagerResultOk) {
        WRITE_DLOG_ERROR(
            MODULE_ID_SYSTEM,
            "%s-%d:Failed to save ExceptionInfo to parameter storage "
            "manager. result=%d",
            "system_manager.c", __LINE__, result);
        (void)EsfPwrMgrClearExceptionInfo();
        (void)EsfParameterStorageManagerClose(handle);
        return result;
      }

      status = EsfParameterStorageManagerClose(handle);
      if (status != kEsfParameterStorageManagerStatusOk) {
        WRITE_DLOG_ERROR(
            MODULE_ID_SYSTEM,
            "%s-%d:Failed to close parameter storage manager. status=%d",
            "system_manager.c", __LINE__, status);
        return kEsfSystemManagerResultInternalError;
      }
    }

    /* clear ExceptionInfo */
    pm_result = EsfPwrMgrClearExceptionInfo();
    if (pm_result != kEsfPwrMgrOk) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:Failed to clear ExceptionInfo. pm_result=%d",
                       "system_manager.c", __LINE__, pm_result);
      return kEsfSystemManagerResultInternalError;
    }
#endif  // CONFIG_EXTERNAL_SYSTEM_MANAGER_EXCEPTION_UPLOAD_DISABLE
  } else {
#ifndef CONFIG_EXTERNAL_SYSTEM_MANAGER_RESET_CAUSE_DISABLE
    /* get EvpResetCause */
    EsfSystemManagerEvpResetCause evp_reset_cause =
        kEsfSystemManagerEvpResetCauseClear;

    result = EsfSystemManagerGetEvpResetCause(&evp_reset_cause);
    if (result != kEsfSystemManagerResultOk) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:Failed to get EvpResetCause. result=%d",
                       "system_manager.c", __LINE__, result);
      return result;
    }

    if (evp_reset_cause != kEsfSystemManagerEvpResetCauseClear) {
      // EvpResetCause is already saved. Do nothing.
      WRITE_DLOG_INFO(MODULE_ID_SYSTEM,
                      "%s-%d:Evp Reset Cause is already saved. Do nothing.",
                      "system_manager.c", __LINE__);
      goto normal_exit;
    }

    /* get ResetCause */
    result = EsfSystemManagerGetResetCause(&get_sm_reset_cause);
    if (result != kEsfSystemManagerResultOk) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:Failed to get ResetCause. result=%d",
                       "system_manager.c", __LINE__, result);
      return result;
    }

    if ((get_sm_reset_cause != kEsfSystemManagerResetCauseDefault) &&
        (get_sm_reset_cause != kEsfSystemManagerResetCauseClear)) {
      // ResetCause is already saved. Do nothing.
      WRITE_DLOG_INFO(MODULE_ID_SYSTEM,
                      "%s-%d:Reset Cause is already saved. Do nothing.",
                      "system_manager.c", __LINE__);
      goto normal_exit;
    }
#endif  // CONFIG_EXTERNAL_SYSTEM_MANAGER_RESET_CAUSE_DISABLE
  }

#ifndef CONFIG_EXTERNAL_SYSTEM_MANAGER_RESET_CAUSE_DISABLE
  {
    WRITE_DLOG_INFO(MODULE_ID_SYSTEM,
                    "%s-%d:Save ResetCause to parameter storage manager.",
                    "system_manager.c", __LINE__);

    /* write flash ResetCause */
    EsfSystemManagerResetCause save_sm_reset_cause =
        kEsfSystemManagerResetCauseUnknown;

    if (get_sm_reset_cause == kEsfSystemManagerResetCauseClear) {
      // During initial startup, nothing is set in ResetCause, so PowerOnReset
      // is set.
      save_sm_reset_cause = kEsfSystemManagerResetCauseSysChipPowerOnReset;
    } else {  // kEsfSystemManagerResetCauseDefault
      switch (pm_reset_cause) {
        case kEsfPwrMgrResetCauseSysChipPowerOnReset:
          // fall through

        case kEsfPwrMgrResetCauseCoreSoft:
          // Booting without a saved ResetCause is treated as SoftResetError.
          save_sm_reset_cause = kEsfSystemManagerResetCauseSoftResetError;
          break;

        default:
          save_sm_reset_cause =
              EsfSystemManagerConvertResetCausePmToSm(pm_reset_cause);
          break;
      }
    }

    result = EsfSystemManagerSetResetCause(save_sm_reset_cause);
    if (result != kEsfSystemManagerResultOk) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:Failed to set ResetCause. result=%d",
                       "system_manager.c", __LINE__, result);
      return result;
    }
  }

normal_exit:
#endif  // CONFIG_EXTERNAL_SYSTEM_MANAGER_RESET_CAUSE_DISABLE

  return kEsfSystemManagerResultOk;
}

EsfSystemManagerResult EsfSystemManagerSendResetCause(void) {
#ifdef CONFIG_EXTERNAL_SYSTEM_MANAGER_RESET_CAUSE_DISABLE
  return kEsfSystemManagerResultOk;
#else
  EsfSystemManagerResetCauseType cause_type =
      kEsfSystemManagerResetCauseTypeNone;

  EsfSystemManagerResult result = EsfSystemManagerSelectResetCause(&cause_type);
  if (result != kEsfSystemManagerResultOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to select ResetCause. result=%d",
                     "system_manager.c", __LINE__, result);
    return result;
  }

  if (cause_type == kEsfSystemManagerResetCauseTypeNone) {
    WRITE_DLOG_WARN(MODULE_ID_SYSTEM, "%s-%d:Reset cause not found.",
                    "system_manager.c", __LINE__);
    return kEsfSystemManagerResultOk;
  }

  char reset_cause_str[ESF_SYSTEM_MANAGER_RESET_CAUSE_MAX_SIZE] = {0};

  if (cause_type == kEsfSystemManagerResetCauseTypeEvp) {
    EsfSystemManagerEvpResetCause evp_reset_cause =
        kEsfSystemManagerEvpResetCauseClear;

    result = EsfSystemManagerGetEvpResetCause(&evp_reset_cause);
    if (result != kEsfSystemManagerResultOk) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:Failed to get EvpResetCause. result=%d",
                       "system_manager.c", __LINE__, result);
      return result;
    }

    result = EsfSystemManagerConvertEvpResetCauseToString(
        evp_reset_cause, reset_cause_str, sizeof(reset_cause_str));
    if (result != kEsfSystemManagerResultOk) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:Failed to convert EvpResetCause to string. "
                       "result=%d",
                       "system_manager.c", __LINE__, result);
      return result;
    }

  } else {
    EsfSystemManagerResetCause get_sm_reset_cause =
        kEsfSystemManagerResetCauseUnknown;

    result = EsfSystemManagerGetResetCause(&get_sm_reset_cause);
    if (result != kEsfSystemManagerResultOk) {
      WRITE_DLOG_ERROR(
          MODULE_ID_SYSTEM, "%s-%d:Failed to get ResetCause. result=%d",
          "system_manager_accessor_reset_cause.c", __LINE__, result);
      return result;
    }

    result = EsfSystemManagerConvertResetCauseToString(
        get_sm_reset_cause, reset_cause_str, sizeof(reset_cause_str));
    if (result != kEsfSystemManagerResultOk) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:Failed to convert ResetCause to string. "
                       "result=%d",
                       "system_manager.c", __LINE__, result);
      return result;
    }
  }

  {
    /* send ResetCause elog */
    uint16_t event_id_base = 0x6000;
    uint8_t event_id_reset_cause = 0x00;

    result = EsfSystemManagerGetResetCauseEventId(reset_cause_str, cause_type,
                                                  &event_id_reset_cause);
    if (result != kEsfSystemManagerResultOk) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:Failed to get ResetCause event id. result=%d",
                       "system_manager.c", __LINE__, result);
      return result;
    }

    WRITE_ELOG_CRITICAL(MODULE_ID_SYSTEM,
                        (uint16_t)(event_id_base + event_id_reset_cause));
    WRITE_DLOG_INFO(MODULE_ID_SYSTEM,
                    "%s-%d:Send ResetCause to elog. event_id=%x",
                    "system_manager.c", __LINE__,
                    (uint16_t)(event_id_base + event_id_reset_cause));
  }

  if (cause_type == kEsfSystemManagerResetCauseTypeEvp) {
    // Delete EvpResetCause
    result =
        EsfSystemManagerSetEvpResetCause(kEsfSystemManagerEvpResetCauseClear);
    if (result != kEsfSystemManagerResultOk) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:Failed to set EvpResetCause to disable. "
                       "result=%d",
                       "system_manager.c", __LINE__, result);
      return result;
    }

  } else {
    // Set empty ResetCause value
    result = EsfSystemManagerSetResetCause(kEsfSystemManagerResetCauseDefault);
    if (result != kEsfSystemManagerResultOk) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:Failed to set ResetCause to empty. "
                       "result=%d",
                       "system_manager.c", __LINE__, result);
      return result;
    }
  }

  return kEsfSystemManagerResultOk;
#endif  // CONFIG_EXTERNAL_SYSTEM_MANAGER_RESET_CAUSE_DISABLE
}

EsfSystemManagerResult EsfSystemManagerUploadExceptionInfo(void) {
#ifdef CONFIG_EXTERNAL_SYSTEM_MANAGER_EXCEPTION_UPLOAD_DISABLE
  return kEsfSystemManagerResultOk;
#else  // CONFIG_EXTERNAL_SYSTEM_MANAGER_EXCEPTION_UPLOAD_DISABLE
  EsfParameterStorageManagerHandle handle =
      ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE;
  EsfParameterStorageManagerStatus status =
      EsfParameterStorageManagerOpen(&handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to open parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  /* get ExceptionInfo size */
  EsfParameterStorageManagerItemID id =
      kEsfParameterStorageManagerItemExceptionInfo;
  uint32_t loadable_size = 0;
  status = EsfParameterStorageManagerGetSize(handle, id, &loadable_size);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to get size of ExceptionInfo. status=%d",
                     "system_manager.c", __LINE__, status);
    (void)EsfParameterStorageManagerClose(handle);
    return kEsfSystemManagerResultInternalError;
  }

  if (loadable_size == 0) {
    status = EsfParameterStorageManagerClose(handle);
    if (status != kEsfParameterStorageManagerStatusOk) {
      WRITE_DLOG_ERROR(
          MODULE_ID_SYSTEM,
          "%s-%d:Failed to close parameter storage manager. status=%d",
          "system_manager.c", __LINE__, status);
      return kEsfSystemManagerResultInternalError;
    }
    // No ExceptionInfo
    WRITE_DLOG_INFO(MODULE_ID_SYSTEM, "%s-%d:No ExceptionInfo.",
                    "system_manager.c", __LINE__);
    return kEsfSystemManagerResultOk;
  }

  struct EsfPwrMgrExceptionInfo *info = calloc(1, loadable_size);
  if (info == NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Failed to calloc.",
                     "system_manager.c", __LINE__);
    (void)EsfParameterStorageManagerClose(handle);
    return kEsfSystemManagerResultInternalError;
  }

  EsfParameterStorageManagerExceptionInfoMask info_mask = {.exception_info = 1};
  EsfParameterStorageManagerExceptionInfo info_data = {
      .exception_info = {.size = loadable_size, .data = (uint8_t *)info}};

  /* read flash ExceptionInfo */
  EsfSystemManagerResult result =
      EsfSystemManagerLoadExceptionInfoFromPsm(handle, &info_mask, &info_data);
  if (result != kEsfSystemManagerResultOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to load ExceptionInfo from parameter storage "
        "manager. result=%d",
        "system_manager.c", __LINE__, result);
    free(info);
    (void)EsfParameterStorageManagerClose(handle);
    return result;
  }

  /* Convert ExceptionInfo */
  uint32_t dst_size = ESF_POWER_MANAGER_EXCEPTION_INFO_SIZE;
  char *dst = (char *)calloc(1, dst_size);
  if (dst == NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Failed to calloc.",
                     "system_manager.c", __LINE__);
    free(info);
    (void)EsfParameterStorageManagerClose(handle);
    return kEsfSystemManagerResultInternalError;
  }
  EsfPwrMgrError ret = EsfPwrMgrConvExceptionInfo(
      (struct EsfPwrMgrExceptionInfo *)(info_data.exception_info.data), dst,
      dst_size);
  if (ret != kEsfPwrMgrOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to convert ExceptionInfo.  ret=%d",
                     "system_manager.c", __LINE__, ret);
    free(dst);
    free(info);
    (void)EsfParameterStorageManagerClose(handle);
    return kEsfSystemManagerResultInternalError;
  }

  free(info);
  info = NULL;

  /* Send Dlog */
  WRITE_DLOG_INFO(MODULE_ID_SYSTEM, "%s-%d:Send ExceptionInfo to dlog.",
                  "system_manager.c", __LINE__);

#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
  uint32_t module_id = MODULE_ID_MAIN;
  EsfLogManagerStatus sts = EsfLogManagerSendBulkDlog(
      module_id, dst_size, (uint8_t *)dst, NULL, NULL);
  if (sts != kEsfLogManagerStatusOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to send bulk dlog.  ret=%d",
                     "system_manager.c", __LINE__, sts);
    free(dst);
    (void)EsfParameterStorageManagerClose(handle);
    return kEsfSystemManagerResultInternalError;
  }
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

  free(dst);
  dst = NULL;

  /* delete flash ExceptionInfo */
  info_mask.exception_info = 1;

  result = EsfSystemManagerClearExceptionInfoOfPsm(handle, &info_mask);
  if (result != kEsfSystemManagerResultOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to delete ExceptionInfo to parameter storage "
        "manager. result=%d",
        "system_manager.c", __LINE__, result);
    (void)EsfParameterStorageManagerClose(handle);
    return result;
  }

  status = EsfParameterStorageManagerClose(handle);
  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to close parameter storage manager. status=%d",
        "system_manager.c", __LINE__, status);
    return kEsfSystemManagerResultInternalError;
  }

  return kEsfSystemManagerResultOk;
#endif  // CONFIG_EXTERNAL_SYSTEM_MANAGER_EXCEPTION_UPLOAD_DISABLE
}

EsfSystemManagerResult EsfSystemManagerIsNeedReboot(bool *reset_flag) {
  if (reset_flag == NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Parameter error. reset_flag=%p",
                     "system_manager.c", __LINE__, reset_flag);
    return kEsfSystemManagerResultParamError;
  }

  EsfPwrMgrResetCause pm_reset_cause = kEsfPwrMgrResetCauseSysChipPowerOnReset;
  EsfPwrMgrError pm_result = EsfPwrMgrGetResetCause(&pm_reset_cause);
  if (pm_result != kEsfPwrMgrOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to get ResetCause. pm_result=%d",
                     "system_manager.c", __LINE__, pm_result);
    return kEsfSystemManagerResultInternalError;
  }

  // Convert PowerManager ResetCause to SystemManager ResetCause
  EsfSystemManagerResetCause sm_reset_cause =
      EsfSystemManagerConvertResetCausePmToSm(pm_reset_cause);

  // Convert SystemManager ResetCause to PlSystemManager ResetCause
  PlSystemManagerResetCause pl_sm_reset_cause =
      EsfSystemManagerConvertResetCauseSmToPlSm(sm_reset_cause);

  PlErrCode error_code = PlSystemManagerIsNeedReboot(pl_sm_reset_cause,
                                                     reset_flag);
  if (error_code != kPlErrCodeOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to check reboot flag. error_code=%d",
                     "system_manager.c", __LINE__, error_code);
    return kEsfSystemManagerResultInternalError;
  }

  return kEsfSystemManagerResultOk;
}

void EsfSystemManagerExecReboot(EsfSystemManagerRebootType reboot_type) {
  EsfSystemManagerResult result = kEsfSystemManagerResultOk;
  EsfPwrMgrRebootType reboot_type_pm = EsfPwrMgrRebootTypeSW;

  switch (reboot_type) {
    case kEsfSystemManagerRebootTypeSystemNormal:
      reboot_type_pm = EsfPwrMgrRebootTypeSW;
      result = EsfSystemManagerSetResetCause(
          kEsfSystemManagerResetCauseSoftResetNormal);
      break;

    case kEsfSystemManagerRebootTypeSystemAbnormal:
      reboot_type_pm = EsfPwrMgrRebootTypeHW;
      result = EsfSystemManagerSetResetCause(
          kEsfSystemManagerResetCauseSoftResetError);
      break;

    case kEsfSystemManagerRebootTypeEvpMemoryAllocFailure:
      reboot_type_pm = EsfPwrMgrRebootTypeHW;
      result = EsfSystemManagerSetEvpResetCause(
          kEsfSystemManagerEvpResetCauseMemoryAllocFailure);
      break;

    case kEsfSystemManagerRebootTypeEvpFreezeDetection:
      reboot_type_pm = EsfPwrMgrRebootTypeHW;
      result = EsfSystemManagerSetEvpResetCause(
          kEsfSystemManagerEvpResetCauseFreezeDetection);
      break;

    default:
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:Unknown reboot type. reboot_type=%d",
                       "system_manager.c", __LINE__, reboot_type);
      result = kEsfSystemManagerResultParamError;
      break;
  }

  if (result != kEsfSystemManagerResultOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to set ResetCause. result=%d reboot_type=%d",
                     "system_manager.c", __LINE__, result, reboot_type);
    // not return here, continue to reboot.
  }

  EsfPwrMgrExecuteRebootEx(reboot_type_pm);
}
