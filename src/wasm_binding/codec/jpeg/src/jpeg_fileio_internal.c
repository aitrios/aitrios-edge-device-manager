/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "jpeg_fileio_internal.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "jpeg_export_internal.h"
#include "wasm_binding_log.h"

#define JPEG_HEADER_INFO_SIZE 512  // tentative definition

/****************************************************************************
 * Public Functions
 ****************************************************************************/

EsfCodecJpegError EsfCodecJpegEncodeInternalAllocOpenFileIo(
    EsfMemoryManagerHandle input_file_handle,
    EsfMemoryManagerHandle *output_file_handle, const EsfCodecJpegInfo *info,
    int32_t *jpeg_size) {
  if ((output_file_handle == NULL) || (info == NULL) || (jpeg_size == NULL)) {
    WASM_BINDING_ERR(
        "Parameter error. output_file_handle=%p info=%p jpeg_size=%p",
        output_file_handle, info, jpeg_size);
    return kJpegParamError;
  }

  EsfCodecJpegError jpeg_error_ret = kJpegSuccess;
  EsfMemoryManagerResult mem_ret = kEsfMemoryManagerResultSuccess;
  int32_t output_buf_size = -1;

  output_buf_size = CalculateOutputBufferSize(info->width, info->height,
                                              info->input_fmt);
  if (output_buf_size < 0) {
    WASM_BINDING_ERR("CalculateOutputBufferSize failed");
    return kJpegParamError;
  }

  // Create Output Handle
  EsfMemoryManagerHandle user_handle = (EsfMemoryManagerHandle)0;
  mem_ret = EsfMemoryManagerAllocate(kEsfMemoryManagerTargetLargeHeap, NULL,
                                     output_buf_size, &user_handle);
  if (mem_ret != kEsfMemoryManagerResultSuccess) {
    WASM_BINDING_ERR("EsfMemoryManagerAllocate=%d", mem_ret);
    jpeg_error_ret = kJpegOtherError;
    goto fail;
  }
  mem_ret = EsfMemoryManagerFopen(user_handle);
  if (mem_ret != kEsfMemoryManagerResultSuccess) {
    WASM_BINDING_ERR("EsfMemoryManagerFopen=%d", mem_ret);
    mem_ret = EsfMemoryManagerFree(user_handle, NULL);
    if (mem_ret != kEsfMemoryManagerResultSuccess) {
      WASM_BINDING_ERR("EsfMemoryManagerFree=%d", mem_ret);
    }
    jpeg_error_ret = kJpegOtherError;
    goto fail;
  }

  // Call API
  jpeg_error_ret = EsfCodecJpegEncodeFileIo(input_file_handle, user_handle,
                                            info, jpeg_size);
  if (jpeg_error_ret != kJpegSuccess) {
    WASM_BINDING_ERR("EsfCodecJpegEncodeFileIo=%d", jpeg_error_ret);
    mem_ret = EsfMemoryManagerFclose(user_handle);
    if (mem_ret != kEsfMemoryManagerResultSuccess) {
      WASM_BINDING_ERR("EsfMemoryManagerFclose=%d", mem_ret);
    }
    mem_ret = EsfMemoryManagerFree(user_handle, NULL);
    if (mem_ret != kEsfMemoryManagerResultSuccess) {
      WASM_BINDING_ERR("EsfMemoryManagerFree=%d", mem_ret);
    }
  } else {
    *output_file_handle = user_handle;
    mem_ret = EsfMemoryManagerFclose(*output_file_handle);
    if (mem_ret != kEsfMemoryManagerResultSuccess) {
      WASM_BINDING_ERR(
          "EsfMemoryManagerFclose for output_file_handle failed. ret = %u",
          mem_ret);
      jpeg_error_ret = kJpegOtherError;
    }
  }
fail:
  return jpeg_error_ret;
}
