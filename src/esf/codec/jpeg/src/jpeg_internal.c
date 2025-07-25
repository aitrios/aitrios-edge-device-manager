/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "jpeg_internal.h"

// clang-format off
// stdio.h needs to be included before setjmp.h.
#include <stdio.h>
#include <setjmp.h>
// clang-format on
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined(CONFIG_EXTERNAL_CODEC_JPEG_OSS_LIBJPEG)
#include "libjpeg/jpeglib.h"
#elif defined(CONFIG_EXTERNAL_CODEC_JPEG_OSS_LIBJPEG_TURBO)
#if defined(__NuttX__)
#include "libjpeg-turbo/jpeglib.h"
#else
#include "jpeglib.h"
#endif /* __NuttX__ */
#endif

#include "jpeg.h"
#include "memory_manager.h"
#include "utility_log.h"
#include "utility_log_module_id.h"

#ifndef JPEG_REMOVE_STATIC
#define STATIC static
#else  // JPEG_REMOVE_STATIC
#define STATIC
#endif  // JPEG_REMOVE_STATIC

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/
// Callback called at the end of Jpeg encoding.
STATIC void EsfCodecJpegTermDestinationCallback(j_compress_ptr cinfo);

// """Callback called at the end of Jpeg encoding.
// Callback called at the end of Jpeg encoding. This is for FileIO access.
// If (cinfo->dest->tmp_output_buffer_size - cinfo->dest->pub.free_in_buffer >
// 0), there is remaining JPEG data in the buffer, so write
// cinfo->dest->tmp_output_buffer using EsfMemoryManagerFwrite(). Add the
// written size to cinfo->dest->jpeg_size.
// Args:
//     cinfo (j_compress_ptr): a pointer to a structure containing the JPEG
//     codec state.
// """
STATIC void EsfCodecJpegTermDestinationCallbackFileIo(j_compress_ptr cinfo);

// """A callback function used in the JPEG compression process.

// It is called when the output buffer is empty and needs to be filled with
// compressed data. The function takes a pointer to a j_compress_ptr structure
// as its parameter, which contains information about the compression process.
// The function casts the dest field of the j_compress_ptr structure to a
// EsfCodecJpegDestManager pointer, and sets the succeed field of the
// EsfCodecJpegDestManager structure to false. Finally, the function returns
// false, indicating that the output buffer is not successfully filled.

// Args:
//     cinfo (j_compress_ptr): a pointer to a structure containing the JPEG
//       codec state.
// """
STATIC boolean EsfCodecJpegEmptyOutputBufferCallback(j_compress_ptr cinfo);

// """A callback function used in the JPEG compression process.
// This is for FileIO access. Write the accumulated JPEG data from the temporary
// storage buffer to FileIO. Perform the following steps. If an error occurs,
// store false in cinfo->dest.pub.succeed and return false. Write
// cinfo->dest->tmp_output_buffer using EsfMemoryManagerFwrite(). Add the
// written size to cinfo->dest->jpeg_size. Set cinfo->dest->pub.next_output_byte
// to cinfo->dest->tmp_output_buffer to reset the output buffer to the start.
// Set cinfo->dest->pub.free_in_buffer to
// CONFIG_EXTERNAL_CODEC_JPEG_FILE_IO_WRITE_BUFFER_SIZE. Return true.
// Args:
//     cinfo (j_compress_ptr): a pointer to a structure containing the JPEG
//       codec state.
// Returns:
//     On success: true
//     On error: false
// """
STATIC boolean
EsfCodecJpegEmptyOutputBufferCallbackFileIo(j_compress_ptr cinfo);

// """A callback function used in the JPEG library to handle error exits.

// The function first casts the 'err' member of the 'cinfo' structure to a
// pointer of type 'EsfCodecJpegErrorManager'. It then frees the memory pointed
// to by 'scan_line_ptr' member of the 'error_manager' structure. Sets the value
// of 'scan_line_ptr' member to NULL. Jumps to the location specified by the
// 'setjmp_buffer' member of the 'error_manager' structure using the longjmp
// function.

// Args:
//     cinfo (j_common_ptr): A pointer to a j_common_ptr structure containing
//       information about the JPEG compression or decompression process.
// Note:
//     Do not use the longjmp() function to return control to a function that
//     has already finished execution.
// """
STATIC void EsfCodecJpegErrorExitCallback(j_common_ptr cinfo);

// """Check if the given JPEG encoding parameters are valid.

// Checks if the provided encoding parameters are valid or not. It
// verifies that the input parameters are not NULL, the output buffer address
// handle is not zero, the input address handle is not zero, the width and
// height are greater than zero, and the quality is between 0 and 100
// (inclusive).

// Args:
//     enc_param (const EsfCodecJpegEncParam *): A pointer to the
//       EsfCodecJpegEncParam structure containing the encoding parameters.
//     access_type (EsfCodecJpegAccessType): Methods of accessing input/output
//       data.

// Returns:
//     If the parameters are valid, it returns kJpegSuccess. Otherwise, it
//     returns kJpegParamError.
// """
STATIC EsfCodecJpegError EsfCodecJpegCheckParam(
    const EsfCodecJpegEncParam *enc_param, EsfCodecJpegAccessType access_type);

// """Convert an RGB planar image to a packed RGB image.

// This function takes an input RGB planar image and converts it to a packed RGB
// image. The input image is represented as three separate planes for each color
// channel (red, green, and blue). The output image is a single plane with the
// RGB values packed together.

// Args:
//     image (const uint8_t *): The input RGB planar image.
//     width (int32_t): Width of the image.
//     height (int32_t): Height of the image.
//     stride (int32_t): Stride of the image (number of bytes per row).
//     target_line (int32_t): Target line to convert.
//     swap_red_blue (bool): Whether to swap the red and blue color channels.
//     converted_image (JSAMPROW): Pointer to the converted image buffer.

// Returns:
//     kJpegSuccess: if the conversion was successful.
//     kJpegParamError: if any of the input parameters are invalid.
// """
STATIC EsfCodecJpegError EsfCodecJpegRgbPlanarToRgbPacked(
    const uint8_t *image, int32_t width, int32_t height, int32_t stride,
    int32_t target_line, JSAMPROW converted_image);

// """Convert an RGB planar image to a packed RGB image.

// This is for FileIO access. This function takes an input RGB planar image and
// converts it to a packed RGB image. The input image is represented as three
// separate planes for each color channel (red, green, and blue). The output
// image is a single plane with the RGB values packed together.

// Args:
//     input_file_handle (EsfMemoryManagerHandle): Input side MemoryManager's
//       FileIO handle.
//     width (int32_t): Width of the image.
//     height (int32_t): Height of the image.
//     stride (int32_t): Stride of the image (number of bytes per row).
//     target_line (int32_t): Target line to convert.
//     swap_red_blue (bool): Whether to swap the red and blue color channels.
//     converted_image (JSAMPROW): Pointer to the converted image buffer.

// Returns:
//     kJpegSuccess: If the conversion was successful.
//     kJpegParamError: If any of the input parameters are invalid.
//     kJpegMemAllocError: If memory allocation fails.
//     kJpegOtherError: If an error occurs in the MemoryManager.
// """
STATIC EsfCodecJpegError EsfCodecJpegRgbPlanarToRgbPackedFileIo(
    EsfMemoryManagerHandle input_file_handle, int32_t width, int32_t height,
    int32_t stride, int32_t target_line, JSAMPROW converted_image);

// """Copy RGB packed image data to a converted image buffer.

// This function takes an input RGB packed image and copies the specified line
// of pixels to a converted image buffer. The width, height, stride,
// target_line, swap_red_blue, and converted_image parameters are used to
// determine the source and destination locations for the pixel data. The
// function checks for valid input parameters and returns an error code if any
// of the parameters are invalid.

// Args:
//     image (const uint8_t *): The input RGB packed image data.
//     width (int32_t): Width of the image.
//     height (int32_t): Height of the image.
//     stride (int32_t): Stride of the image (number of bytes per row).
//     target_line (int32_t): Target line to convert.
//     swap_red_blue (bool): Whether to swap the red and blue color channels.
//     converted_image (JSAMPROW): Pointer to the converted image buffer.

// Returns:
//     kJpegSuccess: if the conversion was successful.
//     kJpegParamError: if any of the input parameters are invalid.
// """
STATIC EsfCodecJpegError EsfCodecJpegCopyRgbPacked(
    const uint8_t *image, int32_t width, int32_t height, int32_t stride,
    int32_t target_line, bool swap_red_blue, JSAMPROW converted_image);

// """Copy RGB packed image data to a converted image buffer.

// This is for FileIO access. This function takes an input RGB packed image and
// copies the specified line of pixels to a converted image buffer. The width,
// height, stride, target_line, swap_red_blue, and converted_image parameters
// are used to determine the source and destination locations for the pixel
// data. The function checks for valid input parameters and returns an error
// code if any of the parameters are invalid.

// Args:
//     input_file_handle (EsfMemoryManagerHandle): Input side MemoryManager's
//       FileIO handle.
//     width (int32_t): Width of the image.
//     height (int32_t): Height of the image.
//     stride (int32_t): Stride of the image (number of bytes per row).
//     target_line (int32_t): Target line to convert.
//     swap_red_blue (bool): Whether to swap the red and blue color channels.
//     converted_image (JSAMPROW): Pointer to the converted image buffer.

// Returns:
//     kJpegSuccess: if the conversion was successful.
//     kJpegParamError: if any of the input parameters are invalid.
//     kJpegMemAllocError: If memory allocation fails.
//     kJpegOtherError: If an error occurs in the MemoryManager.
// """
STATIC EsfCodecJpegError EsfCodecJpegCopyRgbPackedFileIo(
    EsfMemoryManagerHandle input_file_handle, int32_t width, int32_t height,
    int32_t stride, int32_t target_line, bool swap_red_blue,
    JSAMPROW converted_image);

// """Copy a grayscale image line from the source image to the target image.

// This function takes a grayscale image, specified by the 'image' parameter,
// and copies a single line of pixels from the source image to the target image.
// The width and height of the source image are specified by the 'width' and
// 'height' parameters, respectively. The stride parameter specifies the number
// of bytes between each line in the source image. The target_line parameter
// specifies the line number in the source image that should be copied to the
// target image. The converted_image parameter is a pointer to the target image
// where the line will be copied to.
//
// If any of the input parameters are invalid (e.g., width or height is zero,
// image or converted_image is NULL, target_line is out of range, or stride is
// less than width), the function returns kJpegParamError. Otherwise, it copies
// the line of pixels and returns kJpegSuccess.

// Args:
//     image (const uint8_t *): Pointer to the source grayscale image.
//     width (int32_t): Width of the image.
//     height (int32_t): Height of the image.
//     stride (int32_t): Stride of the image (number of bytes per row).
//     target_line (int32_t): Target line to convert.
//     converted_image (JSAMPROW): Pointer to the converted image buffer.

// Returns:
//     kJpegSuccess: if the conversion was successful.
//     kJpegParamError: if any of the input parameters are invalid.
// """
STATIC EsfCodecJpegError EsfCodecJpegCopyGrayScale(
    const uint8_t *image, int32_t width, int32_t height, int32_t stride,
    int32_t target_line, JSAMPROW converted_image);

// """Copy a grayscale image line from the source image to the target image.

// This is for FileIO access. This function takes a grayscale image, specified
// by the 'image' parameter, and copies a single line of pixels from the source
// image to the target image. The width and height of the source image are
// specified by the 'width' and 'height' parameters, respectively. The stride
// parameter specifies the number of bytes between each line in the source
// image. The target_line parameter specifies the line number in the source
// image that should be copied to the target image. The converted_image
// parameter is a pointer to the target image where the line will be copied to.
//
// If any of the input parameters are invalid (e.g., width or height is zero,
// image or converted_image is NULL, target_line is out of range, or stride is
// less than width), the function returns kJpegParamError. Otherwise, it copies
// the line of pixels and returns kJpegSuccess.

// Args:
//     input_file_handle (EsfMemoryManagerHandle): Input side MemoryManager's
//       FileIO handle.
//     width (int32_t): Width of the image.
//     height (int32_t): Height of the image.
//     stride (int32_t): Stride of the image (number of bytes per row).
//     target_line (int32_t): Target line to convert.
//     converted_image (JSAMPROW): Pointer to the converted image buffer.

// Returns:
//     kJpegSuccess: if the conversion was successful.
//     kJpegParamError: if any of the input parameters are invalid.
//     kJpegMemAllocError: If memory allocation fails.
//     kJpegOtherError: If an error occurs in the MemoryManager.
// """
STATIC EsfCodecJpegError EsfCodecJpegCopyGrayScaleFileIo(
    EsfMemoryManagerHandle input_file_handle, int32_t width, int32_t height,
    int32_t stride, int32_t target_line, JSAMPROW converted_image);

// """Converts an NV12 image to a packed YUV format.

// This function converts an NV12 image to a packed YUV format.
// It takes in the image data, width, height, stride, target line, and a pointer
// to the converted image buffer. If any of the input parameters are invalid, it
// returns a kJpegParamError. The function first checks if the input parameters
// are valid. If not, it returns an error. It then calculates the pointers to
// the Y and U/V planes based on the target line and stride. Next, it loops
// through each pixel in the width of the image and copies the Y, U, and V
// values to the converted image buffer. Finally, it returns kJpegSuccess to
// indicate that the conversion was successful.

// Args:
//     image (const uint8_t *): Pointer to the NV12 image data.
//     width (int32_t): Width of the image.
//     height (int32_t): Height of the image.
//     stride (int32_t): Stride of the image (number of bytes per row).
//     target_line (int32_t): Target line to convert.
//     converted_image (JSAMPROW): Pointer to the converted image buffer.

// Returns:
//     kJpegSuccess: if the conversion was successful.
//     kJpegParamError: if any of the input parameters are invalid.
// """
STATIC EsfCodecJpegError EsfCodecJpegNv12ToYuvPacked(
    const uint8_t *image, int32_t width, int32_t height, int32_t stride,
    int32_t target_line, JSAMPROW converted_image);

// """Converts an NV12 image to a packed YUV format.

// This is for FileIO access. This function converts an NV12 image to a packed
// YUV format. It takes in the image data, width, height, stride, target line,
// and a pointer to the converted image buffer. If any of the input parameters
// are invalid, it returns a kJpegParamError. The function first checks if the
// input parameters are valid. If not, it returns an error. It then calculates
// the pointers to the Y and U/V planes based on the target line and stride.
// Next, it loops through each pixel in the width of the image and copies the Y,
// U, and V values to the converted image buffer. Finally, it returns
// kJpegSuccess to indicate that the conversion was successful.

// Args:
//     input_file_handle (EsfMemoryManagerHandle): Input side MemoryManager's
//       FileIO handle.
//     width (int32_t): Width of the image.
//     height (int32_t): Height of the image.
//     stride (int32_t): Stride of the image (number of bytes per row).
//     target_line (int32_t): Target line to convert.
//     converted_image (JSAMPROW): Pointer to the converted image buffer.

// Returns:
//     kJpegSuccess: if the conversion was successful.
//     kJpegParamError: if any of the input parameters are invalid.
//     kJpegMemAllocError: If memory allocation fails.
//     kJpegOtherError: If an error occurs in the MemoryManager.
// """
STATIC EsfCodecJpegError EsfCodecJpegNv12ToYuvPackedFileIo(
    EsfMemoryManagerHandle input_file_handle, int32_t width, int32_t height,
    int32_t stride, int32_t target_line, JSAMPROW converted_image);

// """Convert a line of an image in various formats to a JPEG-compatible format.

// This function takes an input image in various formats (RGB planar, RGB
// packed, BGR packed, grayscale, YUV) and converts it to a format compatible
// with the JPEG compression algorithm. The converted image is stored in a
// temporary buffer and returned as a JSAMPROW pointer.

// Args:
//     format (EsfCodecJpegInputFormat): The input format of the image.
//     image (const uint8_t *): Pointer to the input image data.
//     width (int32_t): Width of the image.
//     height (int32_t): Height of the image.
//     stride (int32_t): Stride of the image (number of bytes per row).
//     target_line (int32_t): Target line to convert.
//     result (EsfCodecJpegError *): Pointer to a variable to store the error
//       code.

// Returns:
//     A pointer to the converted image data, or NULL if an error occurred. The
//     caller is responsible for freeing the memory allocated for the converted
//     image.
// """
STATIC JSAMPROW EsfCodecJpegConvertLine(EsfCodecJpegInputFormat format,
                                        const uint8_t *image, int32_t width,
                                        int32_t height, int32_t stride,
                                        int32_t target_line,
                                        EsfCodecJpegError *result);

// """Convert a line of an image in various formats to a JPEG-compatible format.

// This is for FileIO access. This function takes an input image in various
// formats (RGB planar, RGB packed, BGR packed, grayscale, YUV) and converts it
// to a format compatible with the JPEG compression algorithm. The converted
// image is stored in a temporary buffer and returned as a JSAMPROW pointer.

// Args:
//     format (EsfCodecJpegInputFormat): The input format of the image.
//     input_file_handle (EsfMemoryManagerHandle): Input side MemoryManager's
//       FileIO handle.
//     width (int32_t): Width of the image.
//     height (int32_t): Height of the image.
//     stride (int32_t): Stride of the image (number of bytes per row).
//     target_line (int32_t): Target line to convert.
//     result (EsfCodecJpegError *): Pointer to a variable to store the error
//       code.

// Returns:
//     A pointer to the converted image data, or NULL if an error occurred. The
//     caller is responsible for freeing the memory allocated for the converted
//     image.
// """
STATIC JSAMPROW EsfCodecJpegConvertLineFileIo(
    EsfCodecJpegInputFormat format, EsfMemoryManagerHandle input_file_handle,
    int32_t width, int32_t height, int32_t stride, int32_t target_line,
    EsfCodecJpegError *result);

// """Determines if the file_handle is FileIO handle.

// This is done by verifying that EsfMemoryManagerGetHandleInfo() succeeds,
// the handle's target area is kEsfMemoryManagerTargetLargeHeap,
// and that the return value of EsfMemoryManagerIsMapSupport() is
// kEsfMemoryManagerMapIsNotSupport. If all conditions are met, returns true.
// Otherwise, returns false.

// Args:
//     file_handle (EsfMemoryManagerHandle): Memory manager handle.

// Returns:
//     true:  If file_handle is a valid.
//     false: If file_handle is not a valid.
// """
STATIC bool EsfCodecJpegIsFileHandle(EsfMemoryManagerHandle file_handle);

STATIC int32_t CalculateInputBufferSize(int32_t stride, int32_t height,
                                 EsfCodecJpegInputFormat input_fmt);
/****************************************************************************
 * Private Functions
 ****************************************************************************/
STATIC void EsfCodecJpegTermDestinationCallback(j_compress_ptr cinfo) {
  return;
}

STATIC void EsfCodecJpegTermDestinationCallbackFileIo(j_compress_ptr cinfo) {
  EsfCodecJpegDestManager *dest = ((EsfCodecJpegDestManager *)(cinfo->dest));

  size_t data_size = dest->tmp_output_buffer_size - dest->pub.free_in_buffer;
  size_t rsize = 0;
  if (data_size > 0) {
    EsfMemoryManagerResult result = EsfMemoryManagerFwrite(
        dest->output_file_handle, dest->tmp_output_buffer, data_size, &rsize);
    if (result != kEsfMemoryManagerResultSuccess) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:FileIO write failed. result=%d",
                       "jpeg_internal.c", __LINE__, result);
      dest->succeed = kJpegOtherError;
    } else {
      dest->jpeg_size += rsize;
    }
  }

  return;
}

STATIC boolean EsfCodecJpegEmptyOutputBufferCallback(j_compress_ptr cinfo) {
  ((EsfCodecJpegDestManager *)(cinfo->dest))->succeed =
      kJpegOutputBufferFullError;
  return (boolean) false;
}

STATIC boolean
EsfCodecJpegEmptyOutputBufferCallbackFileIo(j_compress_ptr cinfo) {
  EsfCodecJpegDestManager *dest = ((EsfCodecJpegDestManager *)(cinfo->dest));

  size_t rsize = 0;
  EsfMemoryManagerResult result =
      EsfMemoryManagerFwrite(dest->output_file_handle, dest->tmp_output_buffer,
                             dest->tmp_output_buffer_size, &rsize);
  if (result != kEsfMemoryManagerResultSuccess) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:FileIO write failed. result=%d",
                     "jpeg_internal.c", __LINE__, result);
    dest->succeed = kJpegOtherError;
    return (boolean) false;
  }

  dest->jpeg_size += rsize;
  dest->pub.next_output_byte =
      dest->tmp_output_buffer;  // Return to the beginning of tmp_output_buffer
  size_t remaining_buffer_size = dest->jpeg_size_max - dest->jpeg_size;

  if (remaining_buffer_size == 0) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Jpeg output buffer is full.",
                     "jpeg_internal.c", __LINE__);
    dest->succeed = kJpegOutputBufferFullError;
    return (boolean) false;
  } else if (remaining_buffer_size >=
             CONFIG_EXTERNAL_CODEC_JPEG_FILE_IO_WRITE_BUFFER_SIZE) {
    dest->pub.free_in_buffer =
        CONFIG_EXTERNAL_CODEC_JPEG_FILE_IO_WRITE_BUFFER_SIZE;
    dest->tmp_output_buffer_size =
        CONFIG_EXTERNAL_CODEC_JPEG_FILE_IO_WRITE_BUFFER_SIZE;
  } else {  // remaining_buffer_size <
            // CONFIG_EXTERNAL_CODEC_JPEG_FILE_IO_WRITE_BUFFER_SIZE
    dest->pub.free_in_buffer = remaining_buffer_size;
    dest->tmp_output_buffer_size = remaining_buffer_size;
  }

  return (boolean) true;
}

STATIC void EsfCodecJpegErrorExitCallback(j_common_ptr cinfo) {
  EsfCodecJpegErrorManager *error_manager =
      (EsfCodecJpegErrorManager *)(cinfo->err);
  free(error_manager->scan_line_ptr);
  error_manager->scan_line_ptr = NULL;
  longjmp(error_manager->setjmp_buffer, 1);
}

STATIC EsfCodecJpegError EsfCodecJpegCheckParam(
    const EsfCodecJpegEncParam *enc_param, EsfCodecJpegAccessType access_type) {
  if ((enc_param == (const EsfCodecJpegEncParam *)NULL) ||
      (enc_param->width <= 0) || (enc_param->height <= 0) ||
      (enc_param->quality < 0) || (enc_param->quality > 100)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Parameter error. enc_param=%p"
                     "width=%" PRId32 " height=%" PRId32 " quality=%" PRId32,
                     "jpeg_internal.c", __LINE__, enc_param,
                     enc_param ? enc_param->width : 0,
                     enc_param ? enc_param->height : 0,
                     enc_param ? enc_param->quality : 0);
    return kJpegParamError;
  }

  if (access_type == kEsfCodecJpegMemoryAccess) {
    if ((enc_param->out_buf.output_adr_handle == 0U) ||
        (enc_param->input_adr_handle == 0U)) {
      WRITE_DLOG_ERROR(
          MODULE_ID_SYSTEM,
          "%s-%d:Parameter error. output_adr_handle=%llu input_adr_handle=%llu",
          "jpeg_internal.c", __LINE__, enc_param->out_buf.output_adr_handle,
          enc_param->input_adr_handle);
      return kJpegParamError;
    }
  }

  return kJpegSuccess;
}

STATIC EsfCodecJpegError EsfCodecJpegRgbPlanarToRgbPacked(
    const uint8_t *image, int32_t width, int32_t height, int32_t stride,
    int32_t target_line, JSAMPROW converted_image) {
  if ((width == 0) || (height == 0) || (image == (const uint8_t *)NULL) ||
      (converted_image == (JSAMPROW)NULL) || (target_line < 0) ||
      (target_line >= height) || (stride < width)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Parameter error. width=%" PRId32 " height=%" PRId32
                     " image=%p "
                     "converted_image=%p target_line=%" PRId32
                     " stride=%" PRId32,
                     "jpeg_internal.c", __LINE__, width, height, image,
                     converted_image, target_line, stride);
    return kJpegParamError;
  }

  const uint8_t *red = &image[target_line * stride];
  const uint8_t *green = &image[(target_line * stride) + (stride * height)];
  const uint8_t *blue = &image[(target_line * stride) + (stride * height * 2)];

  for (int32_t j = 0; j < width; j++) {
    converted_image[(j * 3) + 0] = *red;
    converted_image[(j * 3) + 1] = *green;
    converted_image[(j * 3) + 2] = *blue;
    ++red;
    ++green;
    ++blue;
  }

  return kJpegSuccess;
}

STATIC EsfCodecJpegError EsfCodecJpegRgbPlanarToRgbPackedFileIo(
    EsfMemoryManagerHandle input_file_handle, int32_t width, int32_t height,
    int32_t stride, int32_t target_line, JSAMPROW converted_image) {
  if ((width == 0) || (height == 0) || (converted_image == (JSAMPROW)NULL) ||
      (target_line < 0) || (target_line >= height) || (stride < width)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Parameter error. width=%" PRId32 " height=%" PRId32
                     " converted_image=%p target_line=%" PRId32
                     " stride=%" PRId32,
                     "jpeg_internal.c", __LINE__, width, height,
                     converted_image, target_line, stride);
    return kJpegParamError;
  }

  EsfCodecJpegError jpeg_result = kJpegSuccess;
  EsfMemoryManagerResult memory_manager_result = kEsfMemoryManagerResultSuccess;
  off_t offset = 0;
  off_t result_offset = 0;
  size_t result_size = 0;
  uint8_t *red = NULL;
  uint8_t *green = NULL;
  uint8_t *blue = NULL;

  // Red -------------------------------------------------
  offset = target_line * stride;
  memory_manager_result = EsfMemoryManagerFseek(input_file_handle, offset,
                                                SEEK_SET, &result_offset);
  if (memory_manager_result != kEsfMemoryManagerResultSuccess) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:FileIO seek failed. memory_manager_result=%d",
                     "jpeg_internal.c", __LINE__, memory_manager_result);
    jpeg_result = kJpegOtherError;
    goto exit;
  }
  red = malloc(width);
  if (red == NULL) {
    jpeg_result = kJpegMemAllocError;
    goto exit;
  }
  memory_manager_result = EsfMemoryManagerFread(input_file_handle, red, width,
                                                &result_size);
  if (memory_manager_result != kEsfMemoryManagerResultSuccess) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:FileIO read failed. memory_manager_result=%d",
                     "jpeg_internal.c", __LINE__, memory_manager_result);
    jpeg_result = kJpegOtherError;
    goto exit;
  }
  // -------------------------------------------------

  // Green -------------------------------------------------
  offset = (target_line * stride) + (stride * height);
  memory_manager_result = EsfMemoryManagerFseek(input_file_handle, offset,
                                                SEEK_SET, &result_offset);
  if (memory_manager_result != kEsfMemoryManagerResultSuccess) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:FileIO seek failed. memory_manager_result=%d",
                     "jpeg_internal.c", __LINE__, memory_manager_result);
    jpeg_result = kJpegOtherError;
    goto exit;
  }
  green = malloc(width);
  if (green == NULL) {
    jpeg_result = kJpegMemAllocError;
    goto exit;
  }
  memory_manager_result = EsfMemoryManagerFread(input_file_handle, green, width,
                                                &result_size);
  if (memory_manager_result != kEsfMemoryManagerResultSuccess) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:FileIO read failed. memory_manager_result=%d",
                     "jpeg_internal.c", __LINE__, memory_manager_result);
    jpeg_result = kJpegOtherError;
    goto exit;
  }
  // -------------------------------------------------

  // Blue -------------------------------------------------
  offset = (target_line * stride) + (stride * height * 2);
  memory_manager_result = EsfMemoryManagerFseek(input_file_handle, offset,
                                                SEEK_SET, &result_offset);
  if (memory_manager_result != kEsfMemoryManagerResultSuccess) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:FileIO seek failed. memory_manager_result=%d",
                     "jpeg_internal.c", __LINE__, memory_manager_result);
    jpeg_result = kJpegOtherError;
    goto exit;
  }

  blue = malloc(width);
  if (blue == NULL) {
    jpeg_result = kJpegMemAllocError;
    goto exit;
  }
  memory_manager_result = EsfMemoryManagerFread(input_file_handle, blue, width,
                                                &result_size);
  if (memory_manager_result != kEsfMemoryManagerResultSuccess) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:FileIO read failed. memory_manager_result=%d",
                     "jpeg_internal.c", __LINE__, memory_manager_result);
    jpeg_result = kJpegOtherError;
    goto exit;
  }
  // -------------------------------------------------

  for (int32_t j = 0; j < width; j++) {
    converted_image[(j * 3) + 0] = *(red + j);
    converted_image[(j * 3) + 1] = *(green + j);
    converted_image[(j * 3) + 2] = *(blue + j);
  }

exit:
  free(red);
  free(green);
  free(blue);

  return jpeg_result;
}

STATIC EsfCodecJpegError EsfCodecJpegCopyRgbPacked(
    const uint8_t *image, int32_t width, int32_t height, int32_t stride,
    int32_t target_line, bool swap_red_blue, JSAMPROW converted_image) {
  if ((width == 0) || (height == 0) || (image == (const uint8_t *)NULL) ||
      (converted_image == (JSAMPROW)NULL) || (target_line < 0) ||
      (target_line >= height) || (stride < width * 3)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Parameter error. width=%" PRId32 " height=%" PRId32
                     " image=%p "
                     "converted_image=%p target_line=%" PRId32
                     " stride=%" PRId32,
                     "jpeg_internal.c", __LINE__, width, height, image,
                     converted_image, target_line, stride);
    return kJpegParamError;
  }

  for (int32_t j = 0; j < width * 3; j += 3) {
    converted_image[j + 1] = image[(target_line * stride) + j + 1];  // G
    if (swap_red_blue) {
      converted_image[j + 0] = image[(target_line * stride) + j + 2];
      converted_image[j + 2] = image[(target_line * stride) + j + 0];
    } else {
      converted_image[j + 0] = image[(target_line * stride) + j + 0];
      converted_image[j + 2] = image[(target_line * stride) + j + 2];
    }
  }

  return kJpegSuccess;
}

STATIC EsfCodecJpegError EsfCodecJpegCopyRgbPackedFileIo(
    EsfMemoryManagerHandle input_file_handle, int32_t width, int32_t height,
    int32_t stride, int32_t target_line, bool swap_red_blue,
    JSAMPROW converted_image) {
  if ((width == 0) || (height == 0) || (converted_image == (JSAMPROW)NULL) ||
      (target_line < 0) || (target_line >= height) || (stride < width * 3)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Parameter error. width=%" PRId32 " height=%" PRId32
                     " converted_image=%p target_line=%" PRId32
                     " stride=%" PRId32,
                     "jpeg_internal.c", __LINE__, width, height,
                     converted_image, target_line, stride);
    return kJpegParamError;
  }

  EsfCodecJpegError jpeg_result = kJpegSuccess;
  EsfMemoryManagerResult memory_manager_result = kEsfMemoryManagerResultSuccess;
  off_t offset = 0;
  off_t result_offset = 0;
  size_t result_size = 0;
  uint8_t *image = NULL;

  offset = target_line * stride;
  memory_manager_result = EsfMemoryManagerFseek(input_file_handle, offset,
                                                SEEK_SET, &result_offset);
  if (memory_manager_result != kEsfMemoryManagerResultSuccess) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:FileIO seek failed. memory_manager_result=%d",
                     "jpeg_internal.c", __LINE__, memory_manager_result);
    jpeg_result = kJpegOtherError;
    goto exit;
  }
  image = malloc(width * 3);
  if (image == NULL) {
    jpeg_result = kJpegMemAllocError;
    goto exit;
  }
  memory_manager_result = EsfMemoryManagerFread(input_file_handle, image,
                                                (width * 3), &result_size);
  if (memory_manager_result != kEsfMemoryManagerResultSuccess) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:FileIO read failed. memory_manager_result=%d",
                     "jpeg_internal.c", __LINE__, memory_manager_result);
    jpeg_result = kJpegOtherError;
    goto exit;
  }

  for (int32_t j = 0; j < width * 3; j += 3) {
    converted_image[j + 1] = image[j + 1];  // G
    if (swap_red_blue) {
      converted_image[j + 0] = image[j + 2];
      converted_image[j + 2] = image[j + 0];
    } else {
      converted_image[j + 0] = image[j + 0];
      converted_image[j + 2] = image[j + 2];
    }
  }

exit:
  free(image);

  return jpeg_result;
}

STATIC EsfCodecJpegError EsfCodecJpegCopyGrayScale(
    const uint8_t *image, int32_t width, int32_t height, int32_t stride,
    int32_t target_line, JSAMPROW converted_image) {
  if ((width == 0) || (height == 0) || (image == (const uint8_t *)NULL) ||
      (converted_image == (JSAMPROW)NULL) || (target_line < 0) ||
      (target_line >= height) || (stride < width)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Parameter error. width=%" PRId32 " height=%" PRId32
                     " image=%p "
                     "converted_image=%p target_line=%" PRId32
                     " stride=%" PRId32,
                     "jpeg_internal.c", __LINE__, width, height, image,
                     converted_image, target_line, stride);
    return kJpegParamError;
  }

  for (int32_t j = 0; j < width; j++) {
    converted_image[j] = image[(target_line * stride) + j];
  }

  return kJpegSuccess;
}

STATIC EsfCodecJpegError EsfCodecJpegCopyGrayScaleFileIo(
    EsfMemoryManagerHandle input_file_handle, int32_t width, int32_t height,
    int32_t stride, int32_t target_line, JSAMPROW converted_image) {
  if ((width == 0) || (height == 0) || (converted_image == (JSAMPROW)NULL) ||
      (target_line < 0) || (target_line >= height) || (stride < width)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Parameter error. width=%" PRId32 " height=%" PRId32
                     " converted_image=%p target_line=%" PRId32
                     " stride=%" PRId32,
                     "jpeg_internal.c", __LINE__, width, height,
                     converted_image, target_line, stride);
    return kJpegParamError;
  }

  EsfCodecJpegError jpeg_result = kJpegSuccess;
  EsfMemoryManagerResult memory_manager_result = kEsfMemoryManagerResultSuccess;
  off_t offset = 0;
  off_t result_offset = 0;
  size_t result_size = 0;
  uint8_t *image = NULL;

  offset = target_line * stride;
  memory_manager_result = EsfMemoryManagerFseek(input_file_handle, offset,
                                                SEEK_SET, &result_offset);
  if (memory_manager_result != kEsfMemoryManagerResultSuccess) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:FileIO seek failed. memory_manager_result=%d",
                     "jpeg_internal.c", __LINE__, memory_manager_result);
    jpeg_result = kJpegOtherError;
    goto exit;
  }
  image = malloc(width);
  if (image == NULL) {
    jpeg_result = kJpegMemAllocError;
    goto exit;
  }
  memory_manager_result = EsfMemoryManagerFread(input_file_handle, image, width,
                                                &result_size);
  if (memory_manager_result != kEsfMemoryManagerResultSuccess) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:FileIO read failed. memory_manager_result=%d",
                     "jpeg_internal.c", __LINE__, memory_manager_result);
    jpeg_result = kJpegOtherError;
    goto exit;
  }

  for (int32_t j = 0; j < width; j++) {
    converted_image[j] = image[j];
  }

exit:
  free(image);

  return jpeg_result;
}

STATIC EsfCodecJpegError EsfCodecJpegNv12ToYuvPacked(
    const uint8_t *image, int32_t width, int32_t height, int32_t stride,
    int32_t target_line, JSAMPROW converted_image) {
  if ((width == 0) || (height == 0) || (image == (const uint8_t *)NULL) ||
      (converted_image == (JSAMPROW)NULL) || (target_line < 0) ||
      (target_line >= height) || (stride < width)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Parameter error. width=%" PRId32 " height=%" PRId32
                     " image=%p "
                     "converted_image=%p target_line=%" PRId32
                     " stride=%" PRId32,
                     "jpeg_internal.c", __LINE__, width, height, image,
                     converted_image, target_line, stride);
    return kJpegParamError;
  }

  const uint8_t *y = &image[target_line * stride];
  const uint8_t *u_v = &image[(stride * height) + (target_line / 2 * stride)];

  for (int32_t j = 0; j < width; j++) {
    converted_image[(j * 3) + 0] = *y;
    converted_image[(j * 3) + 1] = u_v[0];
    converted_image[(j * 3) + 2] = u_v[1];
    ++y;
    if (j % 2 == 1) {
      u_v += 2;
    }
  }

  return kJpegSuccess;
}

STATIC EsfCodecJpegError EsfCodecJpegNv12ToYuvPackedFileIo(
    EsfMemoryManagerHandle input_file_handle, int32_t width, int32_t height,
    int32_t stride, int32_t target_line, JSAMPROW converted_image) {
  if ((width == 0) || (height == 0) || (converted_image == (JSAMPROW)NULL) ||
      (target_line < 0) || (target_line >= height) || (stride < width)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Parameter error. width=%" PRId32 " height=%" PRId32
                     " converted_image=%p target_line=%" PRId32
                     " stride=%" PRId32,
                     "jpeg_internal.c", __LINE__, width, height,
                     converted_image, target_line, stride);
    return kJpegParamError;
  }

  EsfCodecJpegError jpeg_result = kJpegSuccess;
  EsfMemoryManagerResult memory_manager_result = kEsfMemoryManagerResultSuccess;
  off_t offset = 0;
  off_t result_offset = 0;
  size_t result_size = 0;
  uint8_t *y = NULL;
  uint8_t *u_v = NULL;

  // Y -------------------------------------------------
  offset = target_line * stride;
  memory_manager_result = EsfMemoryManagerFseek(input_file_handle, offset,
                                                SEEK_SET, &result_offset);
  if (memory_manager_result != kEsfMemoryManagerResultSuccess) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:FileIO seek failed. memory_manager_result=%d",
                     "jpeg_internal.c", __LINE__, memory_manager_result);
    jpeg_result = kJpegOtherError;
    goto exit;
  }
  y = malloc(width);
  if (y == NULL) {
    jpeg_result = kJpegMemAllocError;
    goto exit;
  }
  memory_manager_result = EsfMemoryManagerFread(input_file_handle, y, width,
                                                &result_size);
  if (memory_manager_result != kEsfMemoryManagerResultSuccess) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:FileIO read failed. memory_manager_result=%d",
                     "jpeg_internal.c", __LINE__, memory_manager_result);
    jpeg_result = kJpegOtherError;
    goto exit;
  }
  // -------------------------------------------------

  // UV -------------------------------------------------
  offset = (stride * height) + (target_line / 2 * stride);
  memory_manager_result = EsfMemoryManagerFseek(input_file_handle, offset,
                                                SEEK_SET, &result_offset);
  if (memory_manager_result != kEsfMemoryManagerResultSuccess) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:FileIO seek failed. memory_manager_result=%d",
                     "jpeg_internal.c", __LINE__, memory_manager_result);
    jpeg_result = kJpegOtherError;
    goto exit;
  }
  u_v = malloc(width);
  if (u_v == NULL) {
    jpeg_result = kJpegMemAllocError;
    goto exit;
  }
  memory_manager_result = EsfMemoryManagerFread(input_file_handle, u_v, width,
                                                &result_size);
  if (memory_manager_result != kEsfMemoryManagerResultSuccess) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:FileIO read failed. memory_manager_result=%d",
                     "jpeg_internal.c", __LINE__, memory_manager_result);
    jpeg_result = kJpegOtherError;
    goto exit;
  }
  // -------------------------------------------------
  uint8_t *u_v_tmp = u_v;
  for (int32_t j = 0; j < width; j++) {
    converted_image[(j * 3) + 0] = *(y + j);
    converted_image[(j * 3) + 1] = u_v_tmp[0];
    converted_image[(j * 3) + 2] = u_v_tmp[1];
    if (j % 2 == 1) {
      u_v_tmp += 2;
    }
  }

exit:
  free(y);
  free(u_v);

  return jpeg_result;
}

STATIC JSAMPROW EsfCodecJpegConvertLine(EsfCodecJpegInputFormat format,
                                        const uint8_t *image, int32_t width,
                                        int32_t height, int32_t stride,
                                        int32_t target_line,
                                        EsfCodecJpegError *result) {
  if (result == (EsfCodecJpegError *)NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Parameter error. result=%p ",
                     "jpeg_internal.c", __LINE__, result);
    return (JSAMPROW)NULL;
  }

  JSAMPROW tmp_image = (JSAMPROW)NULL;

  switch (format) {
    case kJpegInputRgbPlanar_8:
      tmp_image = (JSAMPROW)malloc((width * 3));
      if (tmp_image == NULL) {
        *result = kJpegMemAllocError;
      } else {
        *result = EsfCodecJpegRgbPlanarToRgbPacked(image, width, height, stride,
                                                   target_line, tmp_image);
      }
      break;
    case kJpegInputRgbPacked_8:
      tmp_image = (JSAMPROW)malloc((width * 3));
      if (tmp_image == NULL) {
        *result = kJpegMemAllocError;
      } else {
        *result = EsfCodecJpegCopyRgbPacked(image, width, height, stride,
                                            target_line, false, tmp_image);
      }
      break;
    case kJpegInputBgrPacked_8:
      tmp_image = (JSAMPROW)malloc((width * 3));
      if (tmp_image == NULL) {
        *result = kJpegMemAllocError;
      } else {
#if defined(CONFIG_EXTERNAL_CODEC_JPEG_OSS_LIBJPEG)
        *result = EsfCodecJpegCopyRgbPacked(image, width, height, stride,
                                            target_line, true, tmp_image);
#elif defined(CONFIG_EXTERNAL_CODEC_JPEG_OSS_LIBJPEG_TURBO)
        *result = EsfCodecJpegCopyRgbPacked(image, width, height, stride,
                                            target_line, false, tmp_image);
#endif
      }
      break;
    case kJpegInputGray_8:
      tmp_image = (JSAMPROW)malloc((width));
      if (tmp_image == NULL) {
        *result = kJpegMemAllocError;
      } else {
        *result = EsfCodecJpegCopyGrayScale(image, width, height, stride,
                                            target_line, tmp_image);
      }
      break;
    case kJpegInputYuv_8:
      tmp_image = (JSAMPROW)malloc((width * 3));
      if (tmp_image == NULL) {
        *result = kJpegMemAllocError;
      } else {
        *result = EsfCodecJpegNv12ToYuvPacked(image, width, height, stride,
                                              target_line, tmp_image);
      }
      break;
    default:
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Parameter error. format=%d",
                       "jpeg_internal.c", __LINE__, format);
      *result = kJpegParamError;
      break;
  }

  if (*result != kJpegSuccess) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Error. *result=%d format=%d"
                     " image=%p width=%" PRId32 " height=%" PRId32
                     " stride=%" PRId32 " target_line=%" PRId32,
                     "jpeg_internal.c", __LINE__, *result, format, image, width,
                     height, stride, target_line);
    free(tmp_image);
    tmp_image = (JSAMPROW)NULL;
  }

  return tmp_image;
}

STATIC JSAMPROW EsfCodecJpegConvertLineFileIo(
    EsfCodecJpegInputFormat format, EsfMemoryManagerHandle input_file_handle,
    int32_t width, int32_t height, int32_t stride, int32_t target_line,
    EsfCodecJpegError *result) {
  if (result == (EsfCodecJpegError *)NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Parameter error. result=%p ",
                     "jpeg_internal.c", __LINE__, result);
    return (JSAMPROW)NULL;
  }

  JSAMPROW tmp_image = (JSAMPROW)NULL;

  switch (format) {
    case kJpegInputRgbPlanar_8:
      tmp_image = (JSAMPROW)malloc((width * 3));
      if (tmp_image == NULL) {
        *result = kJpegMemAllocError;
      } else {
        *result = EsfCodecJpegRgbPlanarToRgbPackedFileIo(
            input_file_handle, width, height, stride, target_line, tmp_image);
      }
      break;
    case kJpegInputRgbPacked_8:
      tmp_image = (JSAMPROW)malloc((width * 3));
      if (tmp_image == NULL) {
        *result = kJpegMemAllocError;
      } else {
        *result = EsfCodecJpegCopyRgbPackedFileIo(input_file_handle, width,
                                                  height, stride, target_line,
                                                  false, tmp_image);
      }
      break;
    case kJpegInputBgrPacked_8:
      tmp_image = (JSAMPROW)malloc((width * 3));
      if (tmp_image == NULL) {
        *result = kJpegMemAllocError;
      } else {
#if defined(CONFIG_EXTERNAL_CODEC_JPEG_OSS_LIBJPEG)
        *result = EsfCodecJpegCopyRgbPackedFileIo(input_file_handle, width,
                                                  height, stride, target_line,
                                                  true, tmp_image);
#elif defined(CONFIG_EXTERNAL_CODEC_JPEG_OSS_LIBJPEG_TURBO)
        *result = EsfCodecJpegCopyRgbPackedFileIo(input_file_handle, width,
                                                  height, stride, target_line,
                                                  false, tmp_image);
#endif
      }
      break;
    case kJpegInputGray_8:
      tmp_image = (JSAMPROW)malloc((width));
      if (tmp_image == NULL) {
        *result = kJpegMemAllocError;
      } else {
        *result = EsfCodecJpegCopyGrayScaleFileIo(
            input_file_handle, width, height, stride, target_line, tmp_image);
      }
      break;
    case kJpegInputYuv_8:
      tmp_image = (JSAMPROW)malloc((width * 3));
      if (tmp_image == NULL) {
        *result = kJpegMemAllocError;
      } else {
        *result = EsfCodecJpegNv12ToYuvPackedFileIo(
            input_file_handle, width, height, stride, target_line, tmp_image);
      }
      break;
    default:
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Parameter error. format=%d",
                       "jpeg_internal.c", __LINE__, format);
      *result = kJpegParamError;
      break;
  }

  if (*result != kJpegSuccess) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Error. *result=%d format=%d"
                     " width=%" PRId32 " height=%" PRId32 " stride=%" PRId32
                     " target_line=%" PRId32,
                     "jpeg_internal.c", __LINE__, *result, format, width,
                     height, stride, target_line);
    free(tmp_image);
    tmp_image = (JSAMPROW)NULL;
  }

  return tmp_image;
}

STATIC bool EsfCodecJpegIsFileHandle(EsfMemoryManagerHandle file_handle) {
  // Check Large Heap
  EsfMemoryManagerHandleInfo info = {0};
  EsfMemoryManagerResult result = EsfMemoryManagerGetHandleInfo(file_handle,
                                                                &info);
  if (result != kEsfMemoryManagerResultSuccess) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Failed to get handle info.",
                     "jpeg_internal.c", __LINE__);
    return false;
  }
  if (info.target_area != kEsfMemoryManagerTargetLargeHeap) {
    return false;
  }

  // Check FileIO
  EsfMemoryManagerMapSupport support = kEsfMemoryManagerMapIsSupport;
  result = EsfMemoryManagerIsMapSupport(file_handle, &support);
  if (result != kEsfMemoryManagerResultSuccess) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Failed to check map support.",
                     "jpeg_internal.c", __LINE__);
    return false;
  }
  if (support == kEsfMemoryManagerMapIsSupport) {
    return false;
  }

  return true;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
EsfCodecJpegCompressManager *EsfCodecJpegCreateManager(
    EsfCodecJpegAccessType access_type) {
  EsfCodecJpegCompressManager *compress_manager =
      (EsfCodecJpegCompressManager *)calloc(
          1, sizeof(EsfCodecJpegCompressManager));
  struct jpeg_compress_struct *jpeg_object =
      (struct jpeg_compress_struct *)calloc(
          1, sizeof(struct jpeg_compress_struct));
  EsfCodecJpegErrorManager *error_manager =
      (EsfCodecJpegErrorManager *)calloc(1, sizeof(EsfCodecJpegErrorManager));
  EsfCodecJpegDestManager *dest_manager =
      (EsfCodecJpegDestManager *)calloc(1, sizeof(EsfCodecJpegDestManager));

  if ((compress_manager == (EsfCodecJpegCompressManager *)NULL) ||
      (jpeg_object == (struct jpeg_compress_struct *)NULL) ||
      (error_manager == (EsfCodecJpegErrorManager *)NULL) ||
      (dest_manager == (EsfCodecJpegDestManager *)NULL)) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Parameter error. compress_manager=%p jpeg_object=%p "
        "error_manager=%p "
        "dest_manager=%p",
        "jpeg_internal.c", __LINE__, compress_manager, jpeg_object,
        error_manager, dest_manager);
    free(compress_manager);
    free(jpeg_object);
    free(error_manager);
    free(dest_manager);
    return (EsfCodecJpegCompressManager *)NULL;
  } else {
    jpeg_object->err = jpeg_std_error(&(error_manager->pub));
    error_manager->pub.error_exit = EsfCodecJpegErrorExitCallback;
    compress_manager->jpeg_object = jpeg_object;
    compress_manager->error_manager = error_manager;
    compress_manager->dest_manager = dest_manager;
    compress_manager->access_type = access_type;
    return compress_manager;
  }
}

EsfCodecJpegError EsfCodecJpegDestroyManager(
    EsfCodecJpegCompressManager *compress_manager) {
  if (compress_manager == (EsfCodecJpegCompressManager *)NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Parameter error. compress_manager=%p",
                     "jpeg_internal.c", __LINE__, compress_manager);
    return kJpegParamError;
  }

  jpeg_destroy_compress(compress_manager->jpeg_object);
  free(compress_manager->jpeg_object);
  compress_manager->jpeg_object = (struct jpeg_compress_struct *)NULL;

  free(compress_manager->dest_manager);
  compress_manager->dest_manager = (EsfCodecJpegDestManager *)NULL;

  free(compress_manager->error_manager);
  compress_manager->error_manager = (EsfCodecJpegErrorManager *)NULL;

  free(compress_manager);

  return kJpegSuccess;
}

EsfCodecJpegError EsfCodecJpegSetParam(
    const EsfCodecJpegEncParam *enc_param,
    EsfCodecJpegCompressManager *compress_manager) {
  if ((enc_param == (const EsfCodecJpegEncParam *)NULL) ||
      (compress_manager == (EsfCodecJpegCompressManager *)NULL) ||
      (EsfCodecJpegCheckParam(enc_param, compress_manager->access_type) !=
       kJpegSuccess)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Parameter error. enc_param=%p compress_manager=%p",
                     "jpeg_internal.c", __LINE__, enc_param, compress_manager);
    return kJpegParamError;
  }

  jpeg_create_compress(compress_manager->jpeg_object);
  switch (enc_param->input_fmt) {
    case kJpegInputRgbPlanar_8:
      compress_manager->jpeg_object->in_color_space = JCS_RGB;
      break;
    case kJpegInputRgbPacked_8:
      compress_manager->jpeg_object->in_color_space = JCS_RGB;
      break;
    case kJpegInputBgrPacked_8:
#if defined(CONFIG_EXTERNAL_CODEC_JPEG_OSS_LIBJPEG)
      compress_manager->jpeg_object->in_color_space = JCS_RGB;
#elif defined(CONFIG_EXTERNAL_CODEC_JPEG_OSS_LIBJPEG_TURBO)
      compress_manager->jpeg_object->in_color_space = JCS_EXT_BGR;
#endif
      break;
    case kJpegInputGray_8:
      compress_manager->jpeg_object->in_color_space = JCS_GRAYSCALE;
      break;
    case kJpegInputYuv_8:
      compress_manager->jpeg_object->in_color_space = JCS_YCbCr;
      break;
    default:
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Parameter error. input_fmt=%d",
                       "jpeg_internal.c", __LINE__, enc_param->input_fmt);
      return kJpegParamError;
  }

  jpeg_set_defaults(compress_manager->jpeg_object);

  compress_manager->jpeg_object->image_width = enc_param->width;
  compress_manager->jpeg_object->image_height = enc_param->height;
  compress_manager->jpeg_object->input_components =
      (enc_param->input_fmt == kJpegInputGray_8) ? 1 : 3;
  compress_manager->jpeg_object->dct_method = JDCT_IFAST;
#if JPEG_LIB_VERSION >= 70
  /*
   * Versions earlier than 7 lack the do_fancy_downsampling member of this
   * struct. The default on Raspberry Pi seems to be 6.2, and ideally we could
   * rely on the packaged version rather than having to supply our own.
   */
  compress_manager->jpeg_object->do_fancy_downsampling = (boolean) false;
#endif
  compress_manager->jpeg_object->dest =
      (struct jpeg_destination_mgr *)(compress_manager->dest_manager);

  jpeg_set_quality(compress_manager->jpeg_object, enc_param->quality,
                   (boolean) true);

  compress_manager->image = (uint8_t *)(uintptr_t)enc_param->input_adr_handle;
  compress_manager->format = enc_param->input_fmt;
  compress_manager->stride = enc_param->stride;

  return kJpegSuccess;
}

EsfCodecJpegError EsfCodecJpegSetDestManager(
    const uint8_t *buffer, size_t size,
    EsfCodecJpegCompressManager *compress_manager) {
  if ((buffer == NULL) || (compress_manager == NULL)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Parameter error. buffer=%p compress_manager=%p",
                     "jpeg_internal.c", __LINE__, buffer, compress_manager);
    return kJpegParamError;
  }

  compress_manager->dest_manager->pub.init_destination =
      EsfCodecJpegTermDestinationCallback;
  compress_manager->dest_manager->pub.empty_output_buffer =
      EsfCodecJpegEmptyOutputBufferCallback;
  compress_manager->dest_manager->pub.term_destination =
      EsfCodecJpegTermDestinationCallback;
  compress_manager->dest_manager->pub.next_output_byte = (JOCTET *)buffer;
  compress_manager->dest_manager->pub.free_in_buffer = size;
  compress_manager->dest_manager->succeed = kJpegSuccess;
  compress_manager->dest_manager->output_file_handle = 0;
  compress_manager->dest_manager->tmp_output_buffer = NULL;
  compress_manager->dest_manager->tmp_output_buffer_size = 0;
  compress_manager->dest_manager->jpeg_size = 0;
  compress_manager->dest_manager->jpeg_size_max = size;

  return kJpegSuccess;
}

EsfCodecJpegError EsfCodecJpegSetDestManagerFileIo(
    uint8_t *buffer, size_t size, EsfMemoryManagerHandle output_file_handle,
    EsfCodecJpegCompressManager *compress_manager) {
  if ((buffer == NULL) || (compress_manager == NULL)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Parameter error. buffer=%p compress_manager=%p",
                     "jpeg_internal.c", __LINE__, buffer, compress_manager);
    return kJpegParamError;
  }

  compress_manager->dest_manager->pub.init_destination =
      EsfCodecJpegTermDestinationCallback;
  compress_manager->dest_manager->pub.empty_output_buffer =
      EsfCodecJpegEmptyOutputBufferCallbackFileIo;
  compress_manager->dest_manager->pub.term_destination =
      EsfCodecJpegTermDestinationCallbackFileIo;
  compress_manager->dest_manager->pub.next_output_byte = (JOCTET *)buffer;
  compress_manager->dest_manager->pub.free_in_buffer = size;
  compress_manager->dest_manager->succeed = kJpegSuccess;
  compress_manager->dest_manager->output_file_handle = output_file_handle;
  compress_manager->dest_manager->tmp_output_buffer = buffer;
  compress_manager->dest_manager->tmp_output_buffer_size = size;
  compress_manager->dest_manager->jpeg_size = 0;

  {
    EsfMemoryManagerHandleInfo info = {0};
    EsfMemoryManagerResult result =
        EsfMemoryManagerGetHandleInfo(output_file_handle, &info);
    if (result != kEsfMemoryManagerResultSuccess) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Failed to get handle info.",
                       "jpeg_internal.c", __LINE__);
      return kJpegOtherError;
    }
    compress_manager->dest_manager->jpeg_size_max = info.allocate_size;
  }

  return kJpegSuccess;
}

EsfCodecJpegError EsfCodecJpegCompressImage(
    int32_t *output_size, EsfCodecJpegCompressManager *compress_manager) {
  if ((output_size == (int32_t *)NULL) ||
      (compress_manager == (EsfCodecJpegCompressManager *)NULL) ||
      (compress_manager->jpeg_object == (struct jpeg_compress_struct *)NULL)) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:Parameter error. output_size=%p compress_manager=%p "
        "jpeg_object=%p",
        "jpeg_internal.c", __LINE__, output_size, compress_manager,
        compress_manager ? compress_manager->jpeg_object : 0);
    return kJpegParamError;
  }

  struct jpeg_compress_struct *jpeg_object = compress_manager->jpeg_object;

  jpeg_start_compress(jpeg_object, (boolean) true);

  while (jpeg_object->next_scanline < jpeg_object->image_height) {
    EsfCodecJpegError result = kJpegSuccess;
    JSAMPROW scan_line = NULL;
    if (compress_manager->access_type == kEsfCodecJpegMemoryAccess) {
      scan_line = EsfCodecJpegConvertLine(
          compress_manager->format, compress_manager->image,
          jpeg_object->image_width, jpeg_object->image_height,
          compress_manager->stride, jpeg_object->next_scanline, &result);
    } else {
      scan_line = EsfCodecJpegConvertLineFileIo(
          compress_manager->format, compress_manager->input_file_handle,
          jpeg_object->image_width, jpeg_object->image_height,
          compress_manager->stride, jpeg_object->next_scanline, &result);
    }
    if (result != kJpegSuccess) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:Failed to covert line. result=%d",
                       "jpeg_internal.c", __LINE__, result);
      return result;
    }

    ((EsfCodecJpegErrorManager *)(jpeg_object->err))->scan_line_ptr = scan_line;
    JSAMPROW array[1] = {scan_line};
    jpeg_write_scanlines(jpeg_object, array, 1);
    free(scan_line);
    ((EsfCodecJpegErrorManager *)(jpeg_object->err))->scan_line_ptr =
        (JSAMPROW)NULL;

    if (((EsfCodecJpegDestManager *)(jpeg_object->dest))->succeed !=
        kJpegSuccess) {
      WRITE_DLOG_ERROR(
          MODULE_ID_SYSTEM, "%s-%d:Error during encoding. succeed=%d",
          "jpeg_internal.c", __LINE__,
          ((EsfCodecJpegDestManager *)(jpeg_object->dest))->succeed);
      jpeg_abort((j_common_ptr)jpeg_object);
      return ((EsfCodecJpegDestManager *)(jpeg_object->dest))->succeed;
    }
  }

  jpeg_finish_compress(jpeg_object);

  if (((EsfCodecJpegDestManager *)(jpeg_object->dest))->succeed !=
      kJpegSuccess) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Error during encoding. succeed=%d",
                     "jpeg_internal.c", __LINE__,
                     ((EsfCodecJpegDestManager *)(jpeg_object->dest))->succeed);
    return ((EsfCodecJpegDestManager *)(jpeg_object->dest))->succeed;
  }

  if (compress_manager->access_type == kEsfCodecJpegMemoryAccess) {
    *output_size =
        ((EsfCodecJpegDestManager *)(jpeg_object->dest))->jpeg_size_max -
        jpeg_object->dest->free_in_buffer;
  } else {
    *output_size = ((EsfCodecJpegDestManager *)(jpeg_object->dest))->jpeg_size;
  }

  return kJpegSuccess;
}

bool EsfCodecJpegIsFileHandleOpen(EsfMemoryManagerHandle file_handle) {
  if (!EsfCodecJpegIsFileHandle(file_handle)) {
    return false;
  }

  {
    off_t current_offset = 0;
    EsfMemoryManagerResult result =
        EsfMemoryManagerFseek(file_handle, 0, SEEK_CUR, &current_offset);
    if (result != kEsfMemoryManagerResultSuccess) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:FileIO seek failed. memory_manager_result=%d",
                       "jpeg_internal.c", __LINE__, result);
      return false;
    }
  }

  return true;
}

EsfCodecJpegError EsfCodecJpegEncodeHandleMap(
    EsfMemoryManagerHandle input_handle, EsfMemoryManagerHandle output_handle,
    const EsfCodecJpegInfo *info, int32_t *jpeg_size) {
  if ((input_handle == (EsfMemoryManagerHandle)0) ||
      (output_handle == (EsfMemoryManagerHandle)0) || (info == NULL) ||
      (jpeg_size == NULL)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Parameter error. input_handle=%" PRIu32
                     "output_handle=%" PRIu32 " info=%p jpeg_size=%p",
                     "jpeg_internal.c", __LINE__, input_handle, output_handle,
                     info, jpeg_size);
    return kJpegParamError;
  }

  EsfMemoryManagerResult memory_manager_result;
  EsfCodecJpegEncParam enc_param = {.input_fmt = info->input_fmt,
                                    .width = info->width,
                                    .height = info->height,
                                    .stride = info->stride,
                                    .quality = info->quality};
  EsfMemoryManagerHandleInfo handle_info = {kEsfMemoryManagerTargetLargeHeap,
                                            0};

  memory_manager_result = EsfMemoryManagerGetHandleInfo(input_handle,
                                                        &handle_info);
  if (memory_manager_result != kEsfMemoryManagerResultSuccess) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:EsfMemoryManagerGetHandleInfo failed. "
                     "memory_manager_result=%d",
                     "jpeg_internal.c", __LINE__, memory_manager_result);
    return kJpegOtherError;
  } else if (handle_info.target_area == kEsfMemoryManagerTargetOtherHeap) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Invalid input handle target area. target_area=%d",
                     "jpeg_internal.c", __LINE__, handle_info.target_area);
    return kJpegParamError;
  }
  int32_t input_buf_size = CalculateInputBufferSize(enc_param.stride, enc_param.height,
                                            enc_param.input_fmt);
  if (input_buf_size < 0) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:CalculateInputBufferSize failed. "
                     "input_buf_size=%" PRId32,
                     "jpeg_internal.c", __LINE__, input_buf_size);
    return kJpegParamError;
  }

  memory_manager_result = EsfMemoryManagerGetHandleInfo(output_handle,
                                                        &handle_info);
  if (memory_manager_result != kEsfMemoryManagerResultSuccess) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:EsfMemoryManagerGetHandleInfo failed. "
                     "memory_manager_result=%d",
                     "jpeg_internal.c", __LINE__, memory_manager_result);
    return kJpegOtherError;
  } else if (handle_info.target_area == kEsfMemoryManagerTargetOtherHeap) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Invalid input handle target area. target_area=%d",
                     "jpeg_internal.c", __LINE__, handle_info.target_area);
    return kJpegParamError;
  }
  enc_param.out_buf.output_buf_size = handle_info.allocate_size;

  uint8_t *input_data = NULL;
  memory_manager_result = EsfMemoryManagerMap(
      input_handle, NULL, input_buf_size, (void **)&input_data);
  if (memory_manager_result != kEsfMemoryManagerResultSuccess) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:EsfMemoryManagerMap failed. memory_manager_result=%d",
        "jpeg_internal.c", __LINE__, memory_manager_result);
    return kJpegOtherError;
  }
  enc_param.input_adr_handle = (uint64_t)(uintptr_t)input_data;

  void *output_data = NULL;
  memory_manager_result = EsfMemoryManagerMap(
      output_handle, NULL, enc_param.out_buf.output_buf_size, &output_data);
  if (memory_manager_result != kEsfMemoryManagerResultSuccess) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:EsfMemoryManagerMap failed. memory_manager_result=%d",
        "jpeg_internal.c", __LINE__, memory_manager_result);
    (void)EsfMemoryManagerUnmap(input_handle, (void **)&input_data);
    return kJpegOtherError;
  }
  enc_param.out_buf.output_adr_handle = (uint64_t)(uintptr_t)output_data;

  EsfCodecJpegError jpeg_result = EsfCodecJpegEncode(&enc_param, jpeg_size);
  if (jpeg_result != kJpegSuccess) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:EsfCodecJpegEncode failed. jpeg_result=%d",
                     "jpeg_internal.c", __LINE__, jpeg_result);
    (void)EsfMemoryManagerUnmap(output_handle, (void **)&output_data);
    (void)EsfMemoryManagerUnmap(input_handle, (void **)&input_data);
    return jpeg_result;
  }

  memory_manager_result = EsfMemoryManagerUnmap(output_handle,
                                                (void **)&output_data);
  if (memory_manager_result != kEsfMemoryManagerResultSuccess) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:EsfMemoryManagerUnmap failed. memory_manager_result=%d",
        "jpeg_internal.c", __LINE__, memory_manager_result);
    jpeg_result = kJpegOtherError;
  }

  memory_manager_result = EsfMemoryManagerUnmap(input_handle,
                                                (void **)&input_data);
  if (memory_manager_result != kEsfMemoryManagerResultSuccess) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:EsfMemoryManagerUnmap failed. memory_manager_result=%d",
        "jpeg_internal.c", __LINE__, memory_manager_result);
    jpeg_result = kJpegOtherError;
  }

  return jpeg_result;
}

EsfCodecJpegError EsfCodecJpegEncodeHandleFileIo(
    EsfMemoryManagerHandle input_handle, EsfMemoryManagerHandle output_handle,
    const EsfCodecJpegInfo *info, int32_t *jpeg_size) {
  if ((input_handle == (EsfMemoryManagerHandle)0) ||
      (output_handle == (EsfMemoryManagerHandle)0) || (info == NULL) ||
      (jpeg_size == NULL)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Parameter error. input_handle=%" PRIu32
                     " output_handle=%" PRIu32 " info=%p jpeg_size=%p",
                     "jpeg_internal.c", __LINE__, input_handle, output_handle,
                     info, jpeg_size);
    return kJpegParamError;
  }

  if (!EsfCodecJpegIsFileHandle(input_handle)) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d: EsfCodecJpegIsHandle check failed for input_handle=%" PRIu32,
        " jpeg_internal.c", __LINE__, input_handle);
    return kJpegParamError;
  }

  if (!EsfCodecJpegIsFileHandle(output_handle)) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d: EsfCodecJpegIsHandle check failed for output_handle=%" PRIu32,
        " jpeg_internal.c", __LINE__, output_handle);
    return kJpegParamError;
  }

  EsfMemoryManagerResult memory_manager_result =
      EsfMemoryManagerFopen(input_handle);
  if (memory_manager_result != kEsfMemoryManagerResultSuccess) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:EsfMemoryManagerFopen failed. memory_manager_result=%d",
        "jpeg_internal.c", __LINE__, memory_manager_result);
    return kJpegOtherError;
  }

  memory_manager_result = EsfMemoryManagerFopen(output_handle);
  if (memory_manager_result != kEsfMemoryManagerResultSuccess) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:EsfMemoryManagerFopen failed. memory_manager_result=%d",
        "jpeg_internal.c", __LINE__, memory_manager_result);
    (void)EsfMemoryManagerFclose(input_handle);
    return kJpegOtherError;
  }

  EsfCodecJpegError jpeg_result =
      EsfCodecJpegEncodeFileIo(input_handle, output_handle, info, jpeg_size);
  if (jpeg_result != kJpegSuccess) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:EsfCodecJpegEncodeFileIo failed. jpeg_result=%d",
                     "jpeg_internal.c", __LINE__, jpeg_result);
    (void)EsfMemoryManagerFclose(output_handle);
    (void)EsfMemoryManagerFclose(input_handle);
    return jpeg_result;
  }

  memory_manager_result = EsfMemoryManagerFclose(output_handle);
  if (memory_manager_result != kEsfMemoryManagerResultSuccess) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:EsfMemoryManagerFclose failed. memory_manager_result=%d",
        "jpeg_internal.c", __LINE__, memory_manager_result);
    jpeg_result = kJpegOtherError;
  }

  memory_manager_result = EsfMemoryManagerFclose(input_handle);
  if (memory_manager_result != kEsfMemoryManagerResultSuccess) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:EsfMemoryManagerFclose for input_handle "
                     "failed. memory_manager_result=%d",
                     "jpeg_internal.c", __LINE__, memory_manager_result);
    jpeg_result = kJpegOtherError;
  }

  return jpeg_result;
}

STATIC int32_t CalculateInputBufferSize(int32_t stride, int32_t height,
                                 EsfCodecJpegInputFormat input_fmt) {
  if (stride < (int32_t)0 || height <= (int32_t)0) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Parameter error. stride=%" PRId32 " height=%" PRId32,
                     "jpeg_internal.c", __LINE__, stride, height);
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
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:Parameter error. input_fmt=%d", "jpeg_internal.c",
                       __LINE__, input_fmt);
      return -1;
  }
}
