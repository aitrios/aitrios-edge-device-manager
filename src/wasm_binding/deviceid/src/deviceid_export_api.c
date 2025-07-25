/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "deviceid_export_api.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "system_manager.h"
#include "wasm_binding_log.h"

static EsfDeviceIdResult GetId_FromSystemManager(char *wasm_buffer);

EsfDeviceIdResult EsfGetDeviceId_wasm(wasm_exec_env_t exec_env,
                                      uint32_t data_offset) {
  if (data_offset == 0) {
    WASM_BINDING_ERR("Param error! data_offset is: %" PRIu32 "\n", data_offset);
    return kEsfDeviceIdResultParamError;
  }

  wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);

  // Wasm addr
  char *wasm_buffer = (char *)wasm_runtime_addr_app_to_native(module_inst,
                                                              data_offset);

  if (wasm_buffer == NULL) {
    WASM_BINDING_ERR("address conversion error! wasm_buffer is NULL.");
    return kEsfDeviceIdResultParamError;
  }

#ifdef CONFIG_EXTERNAL_DEVICE_ID_WASM_DUMMY_ID
  memcpy(wasm_buffer, "Aid-0001000a-0000-2000-9002-00000000001c",
         ESF_SYSTEM_MANAGER_HWINFO_SERIAL_NUMBER_MAX_SIZE);
  return kEsfDeviceIdResultOk;
#else   // CONFIG_EXTERNAL_DEVICE_ID_WASM_DUMMY_ID

  return GetId_FromSystemManager(wasm_buffer);
#endif  // CONFIG_EXTERNAL_DEVICE_ID_WASM_DUMMY_ID
}

static EsfDeviceIdResult GetId_FromSystemManager(char *wasm_buffer) {
  EsfSystemManagerHwInfo data_struct = {0};
  EsfSystemManagerResult result = EsfSystemManagerGetHwInfo(&data_struct);

  // Copy value to Wasm addr
  switch (result) {
    case kEsfSystemManagerResultOk:
      memcpy(wasm_buffer, data_struct.serial_number,
             ESF_SYSTEM_MANAGER_HWINFO_SERIAL_NUMBER_MAX_SIZE);
      return kEsfDeviceIdResultOk;

    case kEsfSystemManagerResultParamError:
      WASM_BINDING_ERR(
          "Parameter error in EsfSystemManagerGetHwInfo. Result: %d", result);
      return kEsfDeviceIdResultParamError;

    case kEsfSystemManagerResultInternalError:
      WASM_BINDING_ERR(
          "Internal error in EsfSystemManagerGetHwInfo. Result: %d", result);
      return kEsfDeviceIdResultInternalError;

    case kEsfSystemManagerResultEmptyData:
      WASM_BINDING_ERR(
          "Empty data error in EsfSystemManagerGetHwInfo. Result: %d", result);
      return kEsfDeviceIdResultEmptyData;

    default:
      WASM_BINDING_ERR("Unknown error in EsfSystemManagerGetHwInfo. Result: %d",
                       result);
      return kEsfDeviceIdResultInternalError;
  }
}
