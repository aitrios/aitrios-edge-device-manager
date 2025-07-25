/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef WAMR_APP_NATIVE_EXPORT_WASM_BIDING_H_
#define WAMR_APP_NATIVE_EXPORT_WASM_BIDING_H_

#include <stdint.h>

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

typedef enum {
  kJpegSuccess,               // No errors.
  kJpegParamError,            // Parameter error.
  kJpegOssInternalError,      // Internal error in OSS.
  kJpegMemAllocError,         // Memory allocation error.
  kJpegOtherError,            // Other errors.
  kJpegOutputBufferFullError  // Output buffer full error.
} EsfCodecJpegError;

typedef enum {
  kEsfDeviceIdResultOk,
  kEsfDeviceIdResultParamError,
  kEsfDeviceIdResultInternalError,
  kEsfDeviceIdResultEmptyData
} EsfDeviceIdResult;

typedef uint32_t EsfMemoryManagerHandle;

#define WASM_BINDING_DEVICEID_MAX_SIZE (41)

EsfCodecJpegError EsfCodecEncodeJpeg(const EsfCodecJpegEncParam *enc_param,
                                     int32_t *jpeg_size);
EsfDeviceIdResult EsfSystemGetDeviceID(char *data);

EsfCodecJpegError EsfCodecJpegEncodeHandle(
    EsfMemoryManagerHandle input_file_handle,
    EsfMemoryManagerHandle *output_file_handle, const EsfCodecJpegInfo *info,
    int32_t *jpeg_size);

EsfCodecJpegError EsfCodecJpegEncodeRelease(
    EsfMemoryManagerHandle release_file_handle);

uint32_t EsfMemoryManagerPread(uint32_t handle, void *buf, uint32_t sz,
                               uint64_t offset, uint32_t *resultp);

#endif  // WAMR_APP_NATIVE_EXPORT_WASM_BIDING_H_
