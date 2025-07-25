/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef WAMR_APP_NATIVE_EXPORT_ESF_CODEC_JPEG_JPEG_EXPORT_INTERNAL_H_
#define WAMR_APP_NATIVE_EXPORT_ESF_CODEC_JPEG_JPEG_EXPORT_INTERNAL_H_

#include "jpeg.h"
#include "memory_manager.h"

int32_t CalculateInputBufferSize(int32_t stride, int32_t height,
                                 EsfCodecJpegInputFormat input_fmt);
int32_t CalculateOutputBufferSize(int32_t width, int32_t height,
                                  EsfCodecJpegInputFormat input_fmt);

#endif  // WAMR_APP_NATIVE_EXPORT_ESF_CODEC_JPEG_JPEG_EXPORT_INTERNAL_H_
