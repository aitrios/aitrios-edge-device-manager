/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Define the external public API for Jpeg.

#ifndef ESF_CODEC_JPEG_INCLUDE_JPEG_H_
#define ESF_CODEC_JPEG_INCLUDE_JPEG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "memory_manager.h"

// This code defines an enumeration type for the result of executing an API.
typedef enum {
  kJpegSuccess,               // No errors.
  kJpegParamError,            // Parameter error.
  kJpegOssInternalError,      // Internal error in OSS.
  kJpegMemAllocError,         // Memory allocation error.
  kJpegOtherError,            // Other errors.
  kJpegOutputBufferFullError  // Output buffer full error.
} EsfCodecJpegError;

// This is a definition of an enumeration type for defining the input data
// format.
typedef enum {
  kJpegInputRgbPlanar_8,  // RGB Planar 8bit.
  kJpegInputRgbPacked_8,  // RGB Packed 8bit.
  kJpegInputBgrPacked_8,  // BGR Packed 8bit.
  kJpegInputGray_8,       // GrayScale 8bit.
  kJpegInputYuv_8         // YUV(NV12) 8bit.
} EsfCodecJpegInputFormat;

// The structure defines an output buffer.
typedef struct {
  // The starting address of the JPEG image output destination. Setting zero is
  // not allowed.
  uint64_t output_adr_handle;

  // Output buffer size.
  int32_t output_buf_size;
} EsfCodecJpegOutputBuf;

// The struct defines the parameters for JPEG encoding.
typedef struct {
  // The starting address of the input data. Setting zero is not allowed.
  uint64_t input_adr_handle;

  // Output buffer information.
  EsfCodecJpegOutputBuf out_buf;

  // Input data format.
  EsfCodecJpegInputFormat input_fmt;

  // Horizontal size of the input image (in pixels). A setting of 0 or less is
  // not allowed.
  int32_t width;

  // Vertical size of the input image (in pixels). A setting of 0 or less is
  // not allowed.
  int32_t height;

  // The stride (in bytes) of the input image, including padding, must not be
  // set to a value smaller than the number of bytes in one row of the input
  // image.
  int32_t stride;

  // Image quality (0: low quality ~ 100: high quality).
  int32_t quality;
} EsfCodecJpegEncParam;

// The struct defines the parameters for JPEG encoding.
typedef struct {
  EsfCodecJpegInputFormat input_fmt;  // Input data format.
  int32_t width;   // Horizontal size of the input image (in pixels). A setting
                   // of 0 or less is not allowed.
  int32_t height;  // Vertical size of the input image (in pixels). A setting of
                   // 0 or less is not allowed.
  int32_t stride;  // The stride (in bytes) of the input image, including
                   // padding, must not be set to a value smaller than the
                   // number of bytes in one row of the input image.
  int32_t quality;  // Image quality (0: low quality ~ 100: high quality).
} EsfCodecJpegInfo;

// """Input data is encoded in JPEG format, and a JPEG image is output.

// Translate the input data to JPEG encoding and output a JPEG image. Carry out
// preprocessing and parameter setting appropriate for the JPEG encoder, and
// perform JPEG encoding.
// For processing efficiency, the input data and output destination addresses
// (enc_param->input_adr_handle, enc_param->out_buf.output_adr_handle) have a
// 4-byte alignment. If 4-byte alignment is not achieved, the processing
// efficiency will decrease. The output area should be allocated the same size
// as the input data by the caller. Multiple simultaneous operation possible.

// Args:
//     enc_param (const struct EsfCodecJpegEncParam *): JPEG encoding
//       parameters. NULL assignment not allowed.
//     jpeg_size (int32_t *): The size of the JPEG image after outputting the
//       encoded. NULL assignment not allowed.

// Returns:
//     kJpegSuccess: Normal termination.
//     kJpegParamError: When enc_param is NULL.
//                      When the value of enc_param is invalid.
//                      When jpeg_size is NULL.
//     kJpegOssInternalError: An error occurred internally in the OSS.
//     kJpegMemAllocError: If memory allocation fails.
//     kJpegOtherError: Other Errors.
//     kJpegOutputBufferFullError: If the output buffer is insufficient during
//       JPEG compression, return.

// """
EsfCodecJpegError EsfCodecJpegEncode(const EsfCodecJpegEncParam *enc_param,
                                     int32_t *jpeg_size);

// """Reads input data from FileIO, performs JPEG encoding, and outputs to
//   FileIO.
// Reads input data from FileIO, performs JPEG encoding, and outputs to FileIO.
// input_file_handle and output_file_handle should be passed in an open state by
// EsfMemoryManagerFopen() from the MemoryManager. Ensure the output area is
// large enough to accommodate the encoded JPEG image. If it is not sufficient,
// kJpegOutputBufferFullError will be returned.
// Args:
//     input_file_handle (EsfMemoryManagerHandle): Input side MemoryManager's
//       FileIO handle.
//     output_file_handle (EsfMemoryManagerHandle): Output side MemoryManager's
//       FileIO handle.
//     info (const struct EsfCodecJpegInfo *): JPEG encoding
//       parameters. NULL assignment not allowed.
//     jpeg_size (int32_t *): The size of the JPEG image after outputting the
//       encoded. NULL assignment not allowed.
// Returns:
//     kJpegSuccess: Normal termination.
//     kJpegParamError: When info is NULL.
//                      When the value of info is invalid.
//                      When jpeg_size is NULL.
//                      When input_file_handle or output_file_handle is other
//                      than a FileIO handle (such as LargeHeap, WasmHeap, DMA
//                      memory).
//                      When input_file_handle or output_file_handle is a closed
//                      FileIO handle.
//     kJpegOssInternalError: An error occurred internally in the OSS.
//     kJpegMemAllocError: If memory allocation fails.
//     kJpegOtherError: Other Errors.
//     kJpegOutputBufferFullError: If the output buffer is insufficient during
//       JPEG compression, return.
// """
EsfCodecJpegError EsfCodecJpegEncodeFileIo(
    EsfMemoryManagerHandle input_file_handle,
    EsfMemoryManagerHandle output_file_handle, const EsfCodecJpegInfo *info,
    int32_t *jpeg_size);

// """Processes input data through a JPEG encoder using memory handles and
//   outputs a JPEG image.
// The function checks the validity of the memory handle areas and selects the
// appropriate encoding method based on memory map support for the given
// handles.

// Args:
//     input_handle (EsfMemoryManagerHandle):
//       Input side MemoryManager's handle.
//     output_handle (EsfMemoryManagerHandle):
//       Output side MemoryManager's handle.
//     info (const struct EsfCodecJpegInfo *):
//       JPEG encoding parameters. NULL assignment not allowed.
//     jpeg_size (int32_t *):
//       The size of the JPEG image after outputting the encoded. NULL
//       assignment not allowed.

// Returns:
//     kJpegSuccess: Normal termination.
//     kJpegParamError: When info is NULL.
//                      When the value of info is invalid.
//                      When jpeg_size is NULL.
//                      When input_handle or output_handle is not a
//                      LargeHeap handle.
//     kJpegOssInternalError: An error occurred internally in the OSS.
//     kJpegMemAllocError: If memory allocation fails.
//     kJpegOtherError: Other Errors.
//     kJpegOutputBufferFullError: If the output buffer is insufficient during
//       JPEG compression, return.
// """
EsfCodecJpegError EsfCodecJpegEncodeHandle(EsfMemoryManagerHandle input_handle,
                                           EsfMemoryManagerHandle output_handle,
                                           const EsfCodecJpegInfo *info,
                                           int32_t *jpeg_size);

#ifdef __cplusplus
}
#endif

#endif  // ESF_CODEC_JPEG_INCLUDE_JPEG_H_
