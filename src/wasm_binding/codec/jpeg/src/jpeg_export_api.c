/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "jpeg_export_api.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "jpeg_export_internal.h"
#include "jpeg_fileio_internal.h"
#include "jpeg_map_internal.h"
#include "wasm_binding_log.h"

#ifndef ESF_WASM_BINDING_CODEC_JPEG_REMOVE_STATIC
#define STATIC static
#else  // ESF_WASM_BINDING_CODEC_JPEG_REMOVE_STATIC
#define STATIC
#endif  // ESF_WASM_BINDING_CODEC_JPEG_REMOVE_STATIC
/****************************************************************************
 * Private Functions
 ****************************************************************************/
STATIC EsfCodecJpegError EsfCodecJpegEncodeHandleMap(
    EsfMemoryManagerHandle input_file_handle,
    EsfMemoryManagerHandle *output_file_handle, EsfCodecJpegInfo *info,
    int32_t *jpeg_size) {
  if ((output_file_handle == NULL) || (info == NULL) || (jpeg_size == NULL)) {
    WASM_BINDING_ERR(
        "Parameter error. output_file_handle=%p info=%p jpeg_size=%p",
        output_file_handle, info, jpeg_size);
    return kJpegParamError;
  }

  EsfMemoryManagerResult mm_ret;
  int32_t input_buf_size = -1;

  EsfCodecJpegEncParam enc_param = {.input_fmt = info->input_fmt,
                                    .width = info->width,
                                    .height = info->height,
                                    .stride = info->stride,
                                    .quality = info->quality};

  input_buf_size = CalculateInputBufferSize(enc_param.stride, enc_param.height,
                                            enc_param.input_fmt);
  if (input_buf_size < 0) {
    WASM_BINDING_ERR("CalculateBufferSize failed");
    return kJpegParamError;
  }

  // MAP input file handle
  uint8_t *input_data = NULL;
  mm_ret = EsfMemoryManagerMap(input_file_handle, NULL, input_buf_size,
                               (void **)&input_data);
  if (mm_ret != kEsfMemoryManagerResultSuccess) {
    WASM_BINDING_ERR("EsfMemoryManagerMap=%d", mm_ret);
    return kJpegOtherError;
  }
  enc_param.input_adr_handle = (uint64_t)(uintptr_t)input_data;

  EsfCodecJpegError jpeg_error_ret = EsfCodecJpegEncodeInternalAllocMap(
      output_file_handle, &enc_param, jpeg_size);
  if (jpeg_error_ret != kJpegSuccess) {
    WASM_BINDING_ERR("EsfCodecJpegEncodeInternalAllocMap failed. ret = %d",
                     jpeg_error_ret);
  } else {
    WASM_BINDING_TRC("jpeg_size=%d", *jpeg_size);
  }

  mm_ret = EsfMemoryManagerUnmap(input_file_handle, (void **)&input_data);
  if (mm_ret != kEsfMemoryManagerResultSuccess) {
    WASM_BINDING_ERR("EsfMemoryManagerUnmap=%d", mm_ret);
    jpeg_error_ret = kJpegOtherError;
  }

  return jpeg_error_ret;
}

STATIC EsfCodecJpegError EsfCodecJpegEncodeHandleFileIo(
    EsfMemoryManagerHandle input_file_handle,
    EsfMemoryManagerHandle *output_file_handle, EsfCodecJpegInfo *info,
    int32_t *jpeg_size) {
  // API Call to open the file.
  if ((output_file_handle == NULL) || (info == NULL) || (jpeg_size == NULL)) {
    WASM_BINDING_ERR(
        "Parameter error. output_file_handle=%p info=%p jpeg_size=%p",
        output_file_handle, info, jpeg_size);
    return kJpegParamError;
  }
  EsfMemoryManagerResult mem_ret = EsfMemoryManagerFopen(input_file_handle);
  if (mem_ret != kEsfMemoryManagerResultSuccess) {
    WASM_BINDING_ERR("EsfMemoryManagerFopen failed. ret = %u", mem_ret);
    return kJpegOtherError;
  }

  EsfCodecJpegError jpeg_error_ret = EsfCodecJpegEncodeInternalAllocOpenFileIo(
      input_file_handle, output_file_handle, info, jpeg_size);
  if (jpeg_error_ret != kJpegSuccess) {
    WASM_BINDING_ERR(
        "EsfCodecJpegEncodeInternalAllocOpenFileIo failed. ret = %d",
        jpeg_error_ret);
  } else {
    WASM_BINDING_TRC("output_file_handle=%" PRIu32, *output_file_handle);
    WASM_BINDING_TRC("jpeg_size=%d", *jpeg_size);
  }

  mem_ret = EsfMemoryManagerFclose(input_file_handle);
  if (mem_ret != kEsfMemoryManagerResultSuccess) {
    WASM_BINDING_ERR(
        "EsfMemoryManagerFclose for input_file_handle failed. ret = %u",
        mem_ret);
    jpeg_error_ret = kJpegOtherError;
  }

  return jpeg_error_ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
EsfCodecJpegError EsfCodecJpegEncodeRelease_wasm(
    wasm_exec_env_t exec_env, EsfMemoryManagerHandle release_file_handle) {
  EsfMemoryManagerResult mem_ret = kEsfMemoryManagerResultSuccess;
  mem_ret = EsfMemoryManagerFree(release_file_handle,
                                 (const wasm_exec_env_t *)exec_env);
  if (mem_ret != kEsfMemoryManagerResultSuccess) {
    WASM_BINDING_ERR("EsfMemoryManagerAllocate=%d", mem_ret);
    return kJpegOtherError;
  }
  return kJpegSuccess;
}

EsfCodecJpegError EsfCodecJpegEncodeHandle_wasm(
    wasm_exec_env_t exec_env, EsfMemoryManagerHandle input_file_handle,
    uint32_t output_file_handle_offset, uint32_t info_offset,
    uint32_t jpeg_size_offset) {
  if (output_file_handle_offset == 0 || info_offset == 0 ||
      jpeg_size_offset == 0) {
    WASM_BINDING_ERR("Param error! output_file_handle_offset is :%" PRIu32
                     " info_offset is %" PRIu32 " jpeg_size_offset is %" PRIu32,
                     output_file_handle_offset, info_offset, jpeg_size_offset);
    return kJpegParamError;
  }
  EsfCodecJpegError jpeg_error_ret = kJpegSuccess;
  wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);
  EsfMemoryManagerHandle *output_file_handle = NULL;
  EsfCodecJpegInfo *info = NULL;
  int32_t *jpeg_size = NULL;

  // 1-1. output_file_handle_offset
  if (!wasm_runtime_validate_app_addr(module_inst, output_file_handle_offset,
                                      sizeof(EsfMemoryManagerHandle))) {
    WASM_BINDING_ERR("bounds check error! output_file_handle_offset=%" PRIu32,
                     output_file_handle_offset);
    return kJpegParamError;
  }
  WASM_BINDING_TRC("input_file_handle: %" PRIu32, input_file_handle);
  WASM_BINDING_TRC("output_file_handle_offset=%d", output_file_handle_offset);

  // 1-2. convert wasm addresses to native addresses.
  output_file_handle =
      (EsfMemoryManagerHandle *)wasm_runtime_addr_app_to_native(
          module_inst, output_file_handle_offset);
  if (output_file_handle == NULL) {
    WASM_BINDING_ERR("address conversion error! output_file_handle is NULL");
    return kJpegParamError;
  }
  WASM_BINDING_TRC("output_file_handle=%" PRIu32, *output_file_handle);

  // 2-1. info_offset
  if (!wasm_runtime_validate_app_addr(module_inst, info_offset,
                                      sizeof(EsfCodecJpegInfo))) {
    WASM_BINDING_TRC("bounds check error! info_offset=%" PRIu32, info_offset);
    return kJpegParamError;
  }
  WASM_BINDING_TRC("info_offset=%" PRIu32, info_offset);

  // 2-2. convert wasm addresses to native addresses.
  info = (EsfCodecJpegInfo *)wasm_runtime_addr_app_to_native(module_inst,
                                                             info_offset);
  if (info == NULL) {
    WASM_BINDING_ERR("address conversion error! info is NULL");
    return kJpegParamError;
  }

  // 3-1. jpeg_size
  if (!wasm_runtime_validate_app_addr(module_inst, jpeg_size_offset,
                                      sizeof(int32_t))) {
    WASM_BINDING_ERR("bounds check error! jpeg_size_offset=%d",
                     jpeg_size_offset);
    return kJpegParamError;
  }
  WASM_BINDING_TRC("jpeg_size_offset=%" PRIu32, jpeg_size_offset);
  // 3-2. convert wasm addresses to native addresses.
  jpeg_size = (int32_t *)wasm_runtime_addr_app_to_native(module_inst,
                                                         jpeg_size_offset);
  if (jpeg_size == NULL) {
    WASM_BINDING_ERR("address conversion error! jpeg_size is NULL");
    return kJpegParamError;
  }

  // 4.HandleCheck
  EsfMemoryManagerHandleInfo handleinfo = {kEsfMemoryManagerTargetLargeHeap, 0};
  EsfMemoryManagerResult result =
      EsfMemoryManagerGetHandleInfo((uint32_t)input_file_handle, &handleinfo);
  if (result != kEsfMemoryManagerResultSuccess) {
    WASM_BINDING_ERR("Failed to get handle info.");
    return kJpegOtherError;
  } else {
    switch (handleinfo.target_area) {
      case kEsfMemoryManagerTargetWasmHeap:
      case kEsfMemoryManagerTargetLargeHeap:
      case kEsfMemoryManagerTargetDma:
        break;
      case kEsfMemoryManagerTargetOtherHeap:
        WASM_BINDING_ERR("Handle allocated by regular malloc()");
        return kJpegOtherError;
      default:
        WASM_BINDING_ERR("Invalid handle target area.");
        return kJpegOtherError;
    }
  }

  // 5. check MAP support
  EsfMemoryManagerMapSupport support = kEsfMemoryManagerMapIsSupport;
  result = EsfMemoryManagerIsMapSupport(input_file_handle, &support);
  if (result != kEsfMemoryManagerResultSuccess) {
    WASM_BINDING_ERR(
        "EsfMemoryManagerIsMapSupport func failed. ret = %u, mem_handle = "
        "%" PRIu32 "",
        result, input_file_handle);
    return kJpegOtherError;
  }
  if (support == kEsfMemoryManagerMapIsSupport) {
    WASM_BINDING_TRC(
        "EsfMemoryManagerMap is supported. Call EsfCodecJpegEncodeHandleMap()");
    jpeg_error_ret = EsfCodecJpegEncodeHandleMap(
        input_file_handle, output_file_handle, info, jpeg_size);
  } else if (support == kEsfMemoryManagerMapIsNotSupport) {
    WASM_BINDING_TRC(
        "EsfMemoryManagerMap is not supported. Call "
        "EsfCodecJpegEncodeHandleFileIo()");
    jpeg_error_ret = EsfCodecJpegEncodeHandleFileIo(
        input_file_handle, output_file_handle, info, jpeg_size);
  } else {
    WASM_BINDING_ERR("Invalid EsfMemoryManagerMapSupport");
    return kJpegOtherError;
  }
  return jpeg_error_ret;
}

EsfCodecJpegError EsfCodecJpegEncode_wasm(wasm_exec_env_t exec_env,
                                          uint32_t enc_param_offset,
                                          uint32_t jpeg_size_offset) {
  if (enc_param_offset == 0 || jpeg_size_offset == 0) {
    WASM_BINDING_ERR("Param error! enc_param_offset is :%" PRIu32
                     " jpeg_size_offset is %" PRIu32,
                     enc_param_offset, jpeg_size_offset);
    return kJpegParamError;
  }

  EsfCodecJpegError jpeg_error_ret = kJpegSuccess;
  uint64_t input_buff_addr = 0;
  uint64_t output_buff_addr = 0;
  int32_t input_buff_size = 0;
  int32_t output_buff_size = 0;

  wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);
  EsfCodecJpegEncParam *enc_param = NULL;
  int32_t *jpeg_size = NULL;

  // encode parameter & jpeg size check
  if (!wasm_runtime_validate_app_addr(module_inst, enc_param_offset,
                                      sizeof(EsfCodecJpegEncParam))) {
    WASM_BINDING_ERR("bounds check error! enc_param_offset=%d",
                     enc_param_offset);
    return kJpegParamError;
  }
  if (!wasm_runtime_validate_app_addr(module_inst, jpeg_size_offset,
                                      sizeof(int32_t))) {
    WASM_BINDING_ERR("bounds check error! jpeg_size_offset=%d",
                     jpeg_size_offset);
    return kJpegParamError;
  }

  // do address conversion
  enc_param = (EsfCodecJpegEncParam *)wasm_runtime_addr_app_to_native(
      module_inst, enc_param_offset);
  if (enc_param == NULL) {
    WASM_BINDING_ERR("address conversion error! enc_param is NULL");
    return kJpegParamError;
  }

  if (enc_param->input_adr_handle == 0 ||
      enc_param->out_buf.output_adr_handle == 0) {
    WASM_BINDING_ERR(
        "address conversion error! enc_param->input_adr_handle:%llu, "
        "enc_param->out_buf.output_adr_handle:%llu",
        enc_param->input_adr_handle, enc_param->out_buf.output_adr_handle);
    return kJpegParamError;
  }

  jpeg_size = (int32_t *)wasm_runtime_addr_app_to_native(module_inst,
                                                         jpeg_size_offset);
  if (jpeg_size == NULL) {
    WASM_BINDING_ERR("address conversion error! jpeg_size is NULL");
    return kJpegParamError;
  }

  // back up the addresses of the input and output buffers.
  input_buff_addr = enc_param->input_adr_handle;
  output_buff_addr = enc_param->out_buf.output_adr_handle;

  // get the input buffer size using the new helper function
  input_buff_size = CalculateInputBufferSize(
      enc_param->stride, enc_param->height, enc_param->input_fmt);
  if (input_buff_size < 0) {
    return kJpegParamError;
  }
  output_buff_size = enc_param->out_buf.output_buf_size;

  // check the addresses of the input and output buffers.
  if (wasm_runtime_validate_app_addr(module_inst,
                                     (uint32_t)enc_param->input_adr_handle,
                                     input_buff_size)) {
    // convert wasm addresses to native addresses.
    enc_param->input_adr_handle =
        (uint64_t)(uintptr_t)wasm_runtime_addr_app_to_native(
            module_inst, (uint32_t)enc_param->input_adr_handle);
  } else {
    WASM_BINDING_ERR(
        "address validation error! input buffer address is invalid or out of "
        "bounds");
    return kJpegParamError;
  }
  if (wasm_runtime_validate_app_addr(
          module_inst, (uint32_t)enc_param->out_buf.output_adr_handle,
          output_buff_size)) {
    // convert wasm addresses to native addresses.
    enc_param->out_buf.output_adr_handle =
        (uint64_t)(uintptr_t)wasm_runtime_addr_app_to_native(
            module_inst, (uint32_t)enc_param->out_buf.output_adr_handle);
  } else {
    WASM_BINDING_ERR(
        "address validation error! output buffer address is invalid or out of "
        "bounds");
    // reset the input buffer addresses to their original values.
    enc_param->input_adr_handle = input_buff_addr;
    return kJpegParamError;
  }

  // API Call.
  jpeg_error_ret = EsfCodecJpegEncode(enc_param, jpeg_size);

  // reset the input and output buffer addresses to their original values.
  enc_param->input_adr_handle = input_buff_addr;
  enc_param->out_buf.output_adr_handle = output_buff_addr;

  return jpeg_error_ret;
}
