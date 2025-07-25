/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "jpeg_export_internal.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "wasm_binding_log.h"

#define JPEG_HEADER_INFO_SIZE 512  // tentative definition

/****************************************************************************
 * Public Functions
 ****************************************************************************/
int32_t CalculateInputBufferSize(int32_t stride, int32_t height,
                                 EsfCodecJpegInputFormat input_fmt) {
  if (stride < (int32_t)0 || height <= (int32_t)0) {
    WASM_BINDING_ERR("stride: %" PRId32 " or height: %" PRId32 " is invalid.",
                     stride, height);
    return -1;
  }
  switch (input_fmt) {
    case kJpegInputRgbPlanar_8:
      return (stride * height * 3);
    case kJpegInputRgbPacked_8:
    case kJpegInputBgrPacked_8:
    case kJpegInputGray_8:
      return (stride * height);
    case kJpegInputYuv_8:
      return ((stride * height) + (stride * height) / 2);
    default:
      WASM_BINDING_ERR("Unsupported input format: %d", input_fmt);
      return -1;
  }
}

int32_t CalculateOutputBufferSize(int32_t width, int32_t height,
                                  EsfCodecJpegInputFormat input_fmt) {
  if (width <= (int32_t)0 || height <= (int32_t)0) {
    WASM_BINDING_ERR("width: %" PRId32 " or height: %" PRId32 " is invalid.",
                     width, height);
    return -1;
  }
  switch (input_fmt) {
    case kJpegInputRgbPlanar_8:
    case kJpegInputRgbPacked_8:
    case kJpegInputBgrPacked_8:
    case kJpegInputYuv_8:
      return (sizeof(uint8_t) * width * height * 3 + JPEG_HEADER_INFO_SIZE);
      break;
    case kJpegInputGray_8:
      return (sizeof(uint8_t) * width * height + JPEG_HEADER_INFO_SIZE);
      break;
    default:
      WASM_BINDING_ERR("Invalid parameter: Not match Format.");
      return -1;
  }
}
