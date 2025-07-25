/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef WAMR_APP_NATIVE_EXPORT_ESF_CODEC_JPEG_JPEG_MAP_INTERNAL_H_
#define WAMR_APP_NATIVE_EXPORT_ESF_CODEC_JPEG_JPEG_MAP_INTERNAL_H_

#include "jpeg.h"
#include "memory_manager.h"

EsfCodecJpegError EsfCodecJpegEncodeInternalAllocMap(
    EsfMemoryManagerHandle *output_file_handle, EsfCodecJpegEncParam *enc_param,
    int32_t *jpeg_size);

#endif  // WAMR_APP_NATIVE_EXPORT_ESF_CODEC_JPEG_JPEG_MAP_INTERNAL_H_
