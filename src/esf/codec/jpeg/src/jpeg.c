/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#include "jpeg.h"

// clang-format off
// stdio.h needs to be included before setjmp.h.
#include <stdio.h>
#include <setjmp.h>
// clang-format on
#include <inttypes.h>
#include <stddef.h>

#if defined(CONFIG_EXTERNAL_CODEC_JPEG_OSS_LIBJPEG)
#include "libjpeg/jpeglib.h"
#elif defined(CONFIG_EXTERNAL_CODEC_JPEG_OSS_LIBJPEG_TURBO)
#if defined(__NuttX__)
#include "libjpeg-turbo/jpeglib.h"
#else
#include "jpeglib.h"
#endif /* __NuttX__ */
#endif

#include "jpeg_internal.h"
#include "utility_log.h"
#include "utility_log_module_id.h"

EsfCodecJpegError EsfCodecJpegEncode(const EsfCodecJpegEncParam *enc_param,
                                     int32_t *jpeg_size) {
  EsfCodecJpegError result = kJpegSuccess;

  if ((enc_param == (const EsfCodecJpegEncParam *)NULL) ||
      (jpeg_size == (int32_t *)NULL)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Parameter error. enc_param=%p jpeg_size=%p",
                     "jpeg.c", __LINE__, enc_param, jpeg_size);
    result = kJpegParamError;
    goto exit;
  }

  EsfCodecJpegCompressManager *manager =
      EsfCodecJpegCreateManager(kEsfCodecJpegMemoryAccess);
  if (manager == (EsfCodecJpegCompressManager *)NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Failed to create jpeg manager.",
                     "jpeg.c", __LINE__);
    result = kJpegMemAllocError;
    goto exit;
  }

  if (setjmp(manager->error_manager->setjmp_buffer)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Error during jpeg encoding.",
                     "jpeg.c", __LINE__);
    (void)EsfCodecJpegDestroyManager(manager);
    result = kJpegOssInternalError;
    goto exit;
  }

  result = EsfCodecJpegSetParam(enc_param, manager);
  if (result != kJpegSuccess) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to set jpeg parameter. result=%d", "jpeg.c",
                     __LINE__, result);
    (void)EsfCodecJpegDestroyManager(manager);
    goto exit;
  }

  result = EsfCodecJpegSetDestManager(
      (uint8_t *)(uintptr_t)enc_param->out_buf.output_adr_handle,
      enc_param->out_buf.output_buf_size, manager);
  if (result != kJpegSuccess) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to set dest manager. result=%d", "jpeg.c",
                     __LINE__, result);
    (void)EsfCodecJpegDestroyManager(manager);
    goto exit;
  }

  int32_t tmp_size = 0;
  result = EsfCodecJpegCompressImage(&tmp_size, manager);
  if (result != kJpegSuccess) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to compress image. result=%d", "jpeg.c",
                     __LINE__, result);
    (void)EsfCodecJpegDestroyManager(manager);
    goto exit;
  }

  result = EsfCodecJpegDestroyManager(manager);
  if (result != kJpegSuccess) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Failed to destroy jpeg manager.",
                     "jpeg.c", __LINE__);
    goto exit;
  }

  *jpeg_size = tmp_size;

exit:

  return result;
}

EsfCodecJpegError EsfCodecJpegEncodeFileIo(
    EsfMemoryManagerHandle input_file_handle,
    EsfMemoryManagerHandle output_file_handle, const EsfCodecJpegInfo *info,
    int32_t *jpeg_size) {
  EsfCodecJpegError result = kJpegSuccess;
  uint8_t *tmp_output_buffer = NULL;
  volatile uintptr_t tmp_output_buffer_addr = 0;

  if ((info == NULL) || (jpeg_size == NULL)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Parameter error. info=%p jpeg_size=%p", "jpeg.c",
                     __LINE__, info, jpeg_size);
    result = kJpegParamError;
    goto exit;
  }

  if (!EsfCodecJpegIsFileHandleOpen(input_file_handle)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Parameter error. input_file_handle is either not a "
                     "valid file handle or not open.",
                     "jpeg.c", __LINE__);
    result = kJpegParamError;
    goto exit;
  }

  if (!EsfCodecJpegIsFileHandleOpen(output_file_handle)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Parameter error. output_file_handle is either not "
                     "a valid file handle or not open.",
                     "jpeg.c", __LINE__);
    result = kJpegParamError;
    goto exit;
  }

  tmp_output_buffer =
      (uint8_t *)malloc(CONFIG_EXTERNAL_CODEC_JPEG_FILE_IO_WRITE_BUFFER_SIZE);
  if (tmp_output_buffer == NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to malloc tmp_output_buffer.", "jpeg.c",
                     __LINE__);
    result = kJpegMemAllocError;
    goto exit;
  }

  // Keep the address of tmp_output_buffer in a volatile variable so that
  // tmp_output_buffer can be released after it is returned by longjmp().
  tmp_output_buffer_addr = (uintptr_t)tmp_output_buffer;

  EsfCodecJpegCompressManager *manager =
      EsfCodecJpegCreateManager(kEsfCodecJpegFileIoAccess);
  if (manager == (EsfCodecJpegCompressManager *)NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Failed to create jpeg manager.",
                     "jpeg.c", __LINE__);
    result = kJpegMemAllocError;
    goto exit;
  }

  if (setjmp(manager->error_manager->setjmp_buffer)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Error during jpeg encoding.",
                     "jpeg.c", __LINE__);
    (void)EsfCodecJpegDestroyManager(manager);
    result = kJpegOssInternalError;
    goto exit;
  }

  {
    EsfCodecJpegEncParam enc_param = {.input_adr_handle = 0,
                                      .input_fmt = info->input_fmt,
                                      .width = info->width,
                                      .height = info->height,
                                      .stride = info->stride,
                                      .quality = info->quality};
    result = EsfCodecJpegSetParam(&enc_param, manager);
    if (result != kJpegSuccess) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:Failed to set jpeg parameter. result=%d",
                       "jpeg.c", __LINE__, result);
      (void)EsfCodecJpegDestroyManager(manager);
      goto exit;
    }
    manager->input_file_handle = input_file_handle;
  }

  result = EsfCodecJpegSetDestManagerFileIo(
      tmp_output_buffer, CONFIG_EXTERNAL_CODEC_JPEG_FILE_IO_WRITE_BUFFER_SIZE,
      output_file_handle, manager);
  if (result != kJpegSuccess) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to set dest manager. result=%d", "jpeg.c",
                     __LINE__, result);
    (void)EsfCodecJpegDestroyManager(manager);
    goto exit;
  }

  int32_t tmp_output_size = 0;
  result = EsfCodecJpegCompressImage(&tmp_output_size, manager);
  if (result != kJpegSuccess) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to compress image. result=%d", "jpeg.c",
                     __LINE__, result);
    (void)EsfCodecJpegDestroyManager(manager);
    goto exit;
  }

  result = EsfCodecJpegDestroyManager(manager);
  if (result != kJpegSuccess) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Failed to destroy jpeg manager.",
                     "jpeg.c", __LINE__);
    goto exit;
  }

  *jpeg_size = tmp_output_size;

exit:
  if (tmp_output_buffer_addr != 0) {
    free((uint8_t *)tmp_output_buffer_addr);
  }

  return result;
}

EsfCodecJpegError EsfCodecJpegEncodeHandle(EsfMemoryManagerHandle input_handle,
                                           EsfMemoryManagerHandle output_handle,
                                           const EsfCodecJpegInfo *info,
                                           int32_t *jpeg_size) {
  EsfCodecJpegError jpeg_error_result = kJpegSuccess;
  EsfMemoryManagerResult memory_manager_result = kEsfMemoryManagerResultSuccess;

  // 1. validate parameters
  if ((input_handle == (EsfMemoryManagerHandle)0) ||
      (output_handle == (EsfMemoryManagerHandle)0) || (info == NULL) ||
      (jpeg_size == NULL)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Parameter error. input_handle=%" PRIu32
                     "output_handle=%" PRIu32 " info=%p jpeg_size=%p",
                     "jpeg.c", __LINE__, input_handle, output_handle, info,
                     jpeg_size);
    return kJpegParamError;
  }

  // 2. check MAP support
  EsfMemoryManagerMapSupport in_support = kEsfMemoryManagerMapIsSupport;
  memory_manager_result = EsfMemoryManagerIsMapSupport(input_handle,
                                                       &in_support);
  if (memory_manager_result != kEsfMemoryManagerResultSuccess) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:EsfMemoryManagerIsMapSupport func failed. "
                     "memory_manager_result=%d"
                     " input_handle=%" PRIu32,
                     "jpeg.c", __LINE__, memory_manager_result, input_handle);
    return kJpegOtherError;
  }

  EsfMemoryManagerMapSupport out_support = kEsfMemoryManagerMapIsSupport;
  memory_manager_result = EsfMemoryManagerIsMapSupport(output_handle,
                                                       &out_support);
  if (memory_manager_result != kEsfMemoryManagerResultSuccess) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:EsfMemoryManagerIsMapSupport func failed. "
                     "memory_manager_result=%d"
                     " output_handle=%" PRIu32,
                     "jpeg.c", __LINE__, memory_manager_result, output_handle);
    return kJpegOtherError;
  }

  // 3. check handle info
  EsfMemoryManagerHandleInfo input_handle_info = {
      kEsfMemoryManagerTargetLargeHeap, 0};
  memory_manager_result = EsfMemoryManagerGetHandleInfo((uint32_t)input_handle,
                                                        &input_handle_info);
  if (memory_manager_result != kEsfMemoryManagerResultSuccess) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to get input handle info. memory_manager_result=%d",
        "jpeg.c", __LINE__, memory_manager_result);
    return kJpegOtherError;
  }

  {
    // check input handle target area
    bool no_support = false;
    if (in_support == kEsfMemoryManagerMapIsSupport) {
      if (input_handle_info.target_area == kEsfMemoryManagerTargetOtherHeap) {
        no_support = true;
      }
    } else {
      if (input_handle_info.target_area != kEsfMemoryManagerTargetLargeHeap) {
        no_support = true;
      }
    }

    if (no_support) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:Invalid input handle target area. target_area=%d",
                       "jpeg.c", __LINE__, input_handle_info.target_area);
      return kJpegParamError;
    }
  }

  EsfMemoryManagerHandleInfo output_handle_info = {
      kEsfMemoryManagerTargetLargeHeap, 0};
  memory_manager_result = EsfMemoryManagerGetHandleInfo((uint32_t)output_handle,
                                                        &output_handle_info);
  if (memory_manager_result != kEsfMemoryManagerResultSuccess) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Failed to get output handle info. memory_manager_result=%d",
        "jpeg.c", __LINE__, memory_manager_result);
    return kJpegOtherError;
  }

  {
    // check output handle target area
    bool no_support = false;
    if (out_support == kEsfMemoryManagerMapIsSupport) {
      if (output_handle_info.target_area == kEsfMemoryManagerTargetOtherHeap) {
        no_support = true;
      }
    } else {
      if (output_handle_info.target_area != kEsfMemoryManagerTargetLargeHeap) {
        no_support = true;
      }
    }

    if (no_support) {
      WRITE_DLOG_ERROR(
          MODULE_ID_SYSTEM,
          "%s-%d:Invalid output handle target area. target_area=%d", "jpeg.c",
          __LINE__, output_handle_info.target_area);
      return kJpegParamError;
    }
  }

  // 4. select encoding method
  if ((in_support == kEsfMemoryManagerMapIsSupport) &&
      (out_support == kEsfMemoryManagerMapIsSupport)) {
    WRITE_DLOG_INFO(MODULE_ID_SYSTEM,
                    "%s-%d:EsfMemoryManagerMap is supported. Call "
                    "EsfCodecJpegEncodeHandleMap()",
                    "jpeg.c", __LINE__);
    jpeg_error_result = EsfCodecJpegEncodeHandleMap(input_handle, output_handle,
                                                    info, jpeg_size);
  } else if ((in_support == kEsfMemoryManagerMapIsNotSupport) &&
             (out_support == kEsfMemoryManagerMapIsNotSupport)) {
    WRITE_DLOG_INFO(MODULE_ID_SYSTEM,
                    "%s-%d:EsfMemoryManagerMap is not supported. Call "
                    "EsfCodecJpegEncodeHandleFileIo()",
                    "jpeg.c", __LINE__);
    jpeg_error_result = EsfCodecJpegEncodeHandleFileIo(
        input_handle, output_handle, info, jpeg_size);
  } else {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Invalid EsfMemoryManagerMapSupport. "
                     "input_support=%d, output_support=%d",
                     "jpeg.c", __LINE__, in_support, out_support);
    return kJpegParamError;
  }

  return jpeg_error_result;
}
