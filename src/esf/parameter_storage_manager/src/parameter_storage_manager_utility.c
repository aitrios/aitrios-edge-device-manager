/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#include "parameter_storage_manager/src/parameter_storage_manager_utility.h"

// The strings corresponding to the error codes.
static const char* const kStrError[kEsfParameterStorageManagerStatusMax] = {
    [kEsfParameterStorageManagerStatusOk] = "Ok",
    [kEsfParameterStorageManagerStatusInvalidArgument] = "InvalidArgument",
    [kEsfParameterStorageManagerStatusFailedPrecondition] =
        "FailedPrecondition",
    [kEsfParameterStorageManagerStatusNotFound] = "NotFound",
    [kEsfParameterStorageManagerStatusOutOfRange] = "OutOfRange",
    [kEsfParameterStorageManagerStatusPermissionDenied] = "PermissionDenied",
    [kEsfParameterStorageManagerStatusResourceExhausted] = "ResourceExhausted",
    [kEsfParameterStorageManagerStatusDataLoss] = "DataLoss",
    [kEsfParameterStorageManagerStatusUnavailable] = "Unavailable",
    [kEsfParameterStorageManagerStatusInternal] = "Internal",
    [kEsfParameterStorageManagerStatusTimedOut] = "TimedOut",
};

const char* EsfParameterStorageManagerStrError(
    EsfParameterStorageManagerStatus status) {
  if ((int)status < 0 || kEsfParameterStorageManagerStatusMax <= (int)status) {
    return "Unknown";
  }
  return kStrError[status];
}

bool EsfParameterStorageManagerStorageAdapterIsWrittenData(
    uint32_t buf_len, uint32_t buf_offset, const uint8_t* buf,
    uint32_t cache_len, uint32_t cache_offset, const uint8_t* cache) {
  if (buf == NULL || cache == NULL) {
    return false;
  }
  if (buf_offset < cache_offset) {
    return false;
  }
  if (cache_len < buf_len) {
    return false;
  }
  if (cache_len - buf_len < buf_offset - cache_offset) {
    return false;
  }
  return !memcmp(buf, cache + buf_offset - cache_offset, buf_len);
}
