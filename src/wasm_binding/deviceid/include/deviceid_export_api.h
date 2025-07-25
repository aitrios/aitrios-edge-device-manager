/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef WAMR_APP_NATIVE_EXPORT_DEVICE_ID_API_H_
#define WAMR_APP_NATIVE_EXPORT_DEVICE_ID_API_H_

#include "wasm_export.h"

// This code defines an enumeration type for the result of executing an API.
typedef enum {
  kEsfDeviceIdResultOk,
  kEsfDeviceIdResultParamError,
  kEsfDeviceIdResultInternalError,
  kEsfDeviceIdResultEmptyData
} EsfDeviceIdResult;

EsfDeviceIdResult EsfGetDeviceId_wasm(wasm_exec_env_t exec_env,
                                      uint32_t data_offset);

#endif  // WAMR_APP_NATIVE_EXPORT_DEVICE_ID_API_H_
