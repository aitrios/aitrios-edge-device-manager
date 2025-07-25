/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef WAMR_APP_NATIVE_EXPORT_ESF_CODEC_JPEG_JPEG_EXPORT_API_H_
#define WAMR_APP_NATIVE_EXPORT_ESF_CODEC_JPEG_JPEG_EXPORT_API_H_

#include "jpeg.h"
#include "memory_manager.h"
#include "wasm_export.h"

EsfCodecJpegError EsfCodecJpegEncode_wasm(wasm_exec_env_t exec_env,
                                          uint32_t enc_param_offset,
                                          uint32_t jpeg_size_offset);

EsfCodecJpegError EsfCodecJpegEncodeHandle_wasm(
    wasm_exec_env_t exec_env, EsfMemoryManagerHandle input_file_handle,
    uint32_t output_file_handle_offset, uint32_t info_offset,
    uint32_t jpeg_size_offset);
EsfCodecJpegError EsfCodecJpegEncodeRelease_wasm(
    wasm_exec_env_t exec_env, EsfMemoryManagerHandle release_file_handle);

#endif  // WAMR_APP_NATIVE_EXPORT_ESF_CODEC_JPEG_JPEG_EXPORT_API_H_
