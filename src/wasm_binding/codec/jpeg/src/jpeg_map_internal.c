/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "jpeg_map_internal.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "jpeg_export_internal.h"
#include "wasm_binding_log.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/
/****************************************************************************
 * Public Functions
 ****************************************************************************/

EsfCodecJpegError EsfCodecJpegEncodeInternalAllocMap(
    EsfMemoryManagerHandle *output_file_handle, EsfCodecJpegEncParam *enc_param,
    int32_t *jpeg_size) {
  EsfMemoryManagerResult mm_ret;

  if ((output_file_handle == NULL) || (enc_param == NULL) ||
      (jpeg_size == NULL)) {
    WASM_BINDING_ERR(
        "Parameter error. output_file_handle=%p enc_param=%p jpeg_size=%p",
        output_file_handle, enc_param, jpeg_size);
    return kJpegParamError;
  }

  enc_param->out_buf.output_buf_size = CalculateOutputBufferSize(
      enc_param->width, enc_param->height, enc_param->input_fmt);
  if (enc_param->out_buf.output_buf_size < 0) {
    WASM_BINDING_ERR("CalculateBufferSize failed");
    return kJpegParamError;
  }
  // Allocate output buffer
  EsfMemoryManagerHandle user_handle = 0;
  void *output_data = NULL;
  mm_ret = EsfMemoryManagerAllocate(kEsfMemoryManagerTargetLargeHeap, NULL,
                                    enc_param->out_buf.output_buf_size,
                                    &user_handle);
  if (mm_ret != kEsfMemoryManagerResultSuccess) {
    WASM_BINDING_ERR("EsfMemoryManagerAllocate=%d", mm_ret);
    return kJpegOtherError;
  }

  mm_ret = EsfMemoryManagerMap(
      user_handle, NULL, enc_param->out_buf.output_buf_size, &output_data);
  if (mm_ret != kEsfMemoryManagerResultSuccess) {
    WASM_BINDING_ERR("EsfMemoryManagerMap=%d", mm_ret);
    mm_ret = EsfMemoryManagerFree(user_handle, NULL);
    if (mm_ret != kEsfMemoryManagerResultSuccess) {
      WASM_BINDING_ERR("EsfMemoryManagerFree=%d", mm_ret);
    }
    return kJpegOtherError;
  }
  enc_param->out_buf.output_adr_handle = (uint64_t)(uintptr_t)output_data;

  // Call API
  EsfCodecJpegError jpeg_error_ret = EsfCodecJpegEncode(enc_param, jpeg_size);

  if (jpeg_error_ret != kJpegSuccess) {
    WASM_BINDING_ERR("EsfCodecJpegEncode=%d", jpeg_error_ret);
  } else {
    *output_file_handle = user_handle;
  }
  mm_ret = EsfMemoryManagerUnmap(user_handle, (void **)&output_data);
  if (mm_ret != kEsfMemoryManagerResultSuccess) {
    WASM_BINDING_ERR("EsfMemoryManagerUnmap=%d", mm_ret);
    jpeg_error_ret = kJpegOtherError;
  }
  if (jpeg_error_ret != kJpegSuccess) {
    mm_ret = EsfMemoryManagerFree(user_handle, NULL);
    if (mm_ret != kEsfMemoryManagerResultSuccess) {
      WASM_BINDING_ERR("EsfMemoryManagerFree=%d", mm_ret);
    }
  }
  return jpeg_error_ret;
}
