/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "sdk_backdoor.h"
#include "wasm_binding_log.h"

#define REG_NATIVE_FUNCx(func_name, signature) \
  {#func_name, func_name##_wasm, signature, NULL}

#if defined(CONFIG_EXTERNAL_CODEC_JPEG_WASM) || \
    defined(CONFIG_EXTERNAL_CODEC_JPEG_HANDLE_WASM)
#include "jpeg_export_api.h"
#endif  // CONFIG_EXTERNAL_CODEC_JPEG_WASM ||
        // CONFIG_EXTERNAL_CODEC_JPEG_HANDLE_WASM

#ifdef CONFIG_EXTERNAL_CODEC_JPEG_WASM
#define REG_NATIVE_FUNC(func_name, signature) \
  {#func_name, EsfCodecJpegEncode_wasm, signature, NULL}
#endif  // CONFIG_EXTERNAL_CODEC_JPEG_WASM
#ifdef CONFIG_EXTERNAL_CODEC_JPEG_HANDLE_WASM
#define REG_NATIVE_FUNC_FILEIO(func_name, signature) \
  {#func_name, EsfCodecJpegEncodeHandle_wasm, signature, NULL}
#endif  // CONFIG_EXTERNAL_CODEC_JPEG_HANDLE_WASM
#ifdef CONFIG_EXTERNAL_DEVICE_ID_WASM
#include "deviceid_export_api.h"
#define REG_NATIVE_FUNC_DEVICE_ID(func_name, signature) \
  {#func_name, EsfGetDeviceId_wasm, signature}
#endif  // CONFIG_EXTERNAL_DEVICE_ID_WASM

#ifdef CONFIG_EXTERNAL_MEMORY_MANAGER_WASM
#include "memory_manager_export_api.h"
#endif  // CONFIG_EXTERNAL_MEMORY_MANAGER_WASM

static NativeSymbol native_symbols[] = {
#ifdef CONFIG_EXTERNAL_CODEC_JPEG_WASM
    REG_NATIVE_FUNC(EsfCodecEncodeJpeg, "(ii)i"),
#endif  // CONFIG_EXTERNAL_CODEC_JPEG_WASM
#ifdef CONFIG_EXTERNAL_CODEC_JPEG_HANDLE_WASM
    REG_NATIVE_FUNC_FILEIO(EsfCodecJpegEncodeHandle, "(iiii)i"),
    REG_NATIVE_FUNCx(EsfCodecJpegEncodeRelease, "(i)i"),
#endif  // CONFIG_EXTERNAL_CODEC_JPEG_HANDLE_WASM
#ifdef CONFIG_EXTERNAL_DEVICE_ID_WASM
    REG_NATIVE_FUNC_DEVICE_ID(EsfSystemGetDeviceID, "(i)i"),
#endif  // CONFIG_EXTERNAL_DEVICE_ID_WASM

#ifdef CONFIG_EXTERNAL_MEMORY_MANAGER_WASM
    // Note: we probably want to update this in future
    // to support memory64 because '~' is i32 even for memory64.
    REG_NATIVE_FUNCx(EsfMemoryManagerPread, "(i*~I*)i"),
#endif  // CONFIG_EXTERNAL_MEMORY_MANAGER_WASM
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/
/****************************************************************************
 * Public Functions
 ****************************************************************************/
bool WasmBindingInit(void) {
  int n_native_symbols = sizeof(native_symbols) / sizeof(NativeSymbol);
  if (n_native_symbols == 0) {
    WASM_BINDING_INFO(
        "WasmBindingInit is skipped because there is no valid Config API to "
        "register.");
    return true;
  }
  // Wasm binding Symbols
  if (!EVP_wasm_runtime_register_natives("env", native_symbols,
                                         n_native_symbols)) {
    WASM_BINDING_ERR("EVP_wasm_runtime_register_natives failed");
    return false;
  }
  return true;
}
