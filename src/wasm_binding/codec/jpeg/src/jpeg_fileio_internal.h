/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef WAMR_APP_NATIVE_EXPORT_ESF_CODEC_JPEG_JPEG_FILEIO_INTERNAL_H_
#define WAMR_APP_NATIVE_EXPORT_ESF_CODEC_JPEG_JPEG_FILEIO_INTERNAL_H_

#include "jpeg.h"
#include "memory_manager.h"

EsfCodecJpegError EsfCodecJpegEncodeInternalAllocOpenFileIo(
    EsfMemoryManagerHandle input_file_handle,
    EsfMemoryManagerHandle *output_file_handle, const EsfCodecJpegInfo *info,
    int32_t *jpeg_size);

#endif  // WAMR_APP_NATIVE_EXPORT_ESF_CODEC_JPEG_JPEG_FILEIO_INTERNAL_H_
