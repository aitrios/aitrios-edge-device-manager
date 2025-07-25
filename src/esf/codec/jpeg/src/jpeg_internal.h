/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

// Define the internal API for Jpeg.

#ifndef ESF_CODEC_JPEG_SRC_JPEG_INTERNAL_H_
#define ESF_CODEC_JPEG_SRC_JPEG_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

// clang-format off
// stdio.h needs to be included before setjmp.h.
#include <stdio.h>
#include <setjmp.h>
// clang-format on
#include <stdbool.h>
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

#include "jpeg.h"

// JPEG destination manager structure of libjpeg/libjpeg-turbo.
typedef struct {
  // jpeg_destination_mgr structure of libjpeg.
  struct jpeg_destination_mgr pub;

  // The result of the Jpeg encoding in OSS will be stored.
  EsfCodecJpegError succeed;
  EsfMemoryManagerHandle
      output_file_handle;     // Output side MemoryManager's FileIO handle.
  JOCTET *tmp_output_buffer;  // Buffer for temporarily storing JPEG data.
  size_t
      tmp_output_buffer_size;  // Buffer size for temporarily storing JPEG data.
  size_t jpeg_size;            // Total size of the JPEG data
  size_t jpeg_size_max;        // Max size of the JPEG data
} EsfCodecJpegDestManager;

// JPEG error manager structure of libjpeg/libjpeg-turbo.
typedef struct {
  // jpeg_error_mgr structure of libjpeg.
  struct jpeg_error_mgr pub;

  // Buffer for restoring when returning from longjmp().
  jmp_buf setjmp_buffer;

  // Use to release the area allocated by EsfCodecJpegConvertLine() in case of
  // an error.
  JSAMPROW scan_line_ptr;
} EsfCodecJpegErrorManager;

// Methods of accessing input/output data.
typedef enum {
  kEsfCodecJpegMemoryAccess,  // Memory access
  kEsfCodecJpegFileIoAccess   // FileIO access
} EsfCodecJpegAccessType;

// JPEG compress manager structure of libjpeg/libjpeg-turbo.
typedef struct {
  // Pointer to a JPEG compression object.
  struct jpeg_compress_struct *jpeg_object;

  // Pointer to a JPEG destination manager.
  EsfCodecJpegDestManager *dest_manager;

  // Pointer to a JPEG error manager.
  EsfCodecJpegErrorManager *error_manager;

  // Pointer to the beginning of the input image.
  uint8_t *image;

  // Input image format.
  EsfCodecJpegInputFormat format;

  // Stride of the input image (in bytes).
  int32_t stride;
  EsfMemoryManagerHandle
      input_file_handle;  // Input side MemoryManager's FileIO handle.
  EsfCodecJpegAccessType access_type;  // Methods of accessing input/output
                                       // data.
} EsfCodecJpegCompressManager;

// """Create a new instance of EsfCodecJpegCompressManager.

// This function allocates memory for the EsfCodecJpegCompressManager structure,
// the jpeg_compress_struct structure, the EsfCodecJpegErrorManager structure,
// and the EsfCodecJpegDestManager structure. It then initializes these
// structures and sets the error_exit callback function for the
// jpeg_compress_struct structure.

// Args:
//     access_type (EsfCodecJpegAccessType): Methods of accessing input/output
//       data.

// Returns:
//     EsfCodecJpegCompressManager *: The function returns a pointer to the Jpeg
//       compression manager. If memory allocation fails, it returns NULL.
// """
EsfCodecJpegCompressManager *EsfCodecJpegCreateManager(
    EsfCodecJpegAccessType access_type);

// """Destroy the JPEG compress manager.

// This function destroys the given JPEG compress manager and frees all
// associated resources.

// Args:
//     compress_manager (EsfCodecJpegCompressManager *): Pointer to JPEG
//       compression manager cannot be NULL.

// Returns:
//     kJpegSuccess: On normal termination, it returns.
//     kJpegParamError: If the argument compress_manager is NULL, return.
// """
EsfCodecJpegError EsfCodecJpegDestroyManager(
    EsfCodecJpegCompressManager *compress_manager);

// """Set parameters for JPEG encoding.

// Checks if the provided encoding parameters are valid or not. It
// verifies that the input parameters are not NULL, the output buffer address
// handle is not zero, the input address handle is not zero, the width and
// height are greater than zero, and the quality is between 0 and 100
// (inclusive).

// Args:
//     enc_param (const EsfCodecJpegEncParam *): Encoding parameters. Null input
//       is not allowed.
//     compress_manager (EsfCodecJpegCompressManager *): Pointer to JPEG
//       compression manager. Null input is not allowed.

// Returns:
//     kJpegSuccess: On normal termination, it returns.
//     kJpegParamError: - If the arguments enc_param or compress_manager are
//                          NULL, it returns.
//                      - If the value of enc_param is invalid.
//     kJpegOssInternalError: If an error occurs in OSS, it will be returned.
// """
EsfCodecJpegError EsfCodecJpegSetParam(
    const EsfCodecJpegEncParam *enc_param,
    EsfCodecJpegCompressManager *compress_manager);

// """Sets memory access parameters in the JPEG output manager.
// Sets memory access parameters in the JPEG output manager.
// Args:
//     buffer (const uint8_t *): Starting address of the output area. NULL input
//       is not allowed.
//     size (size_t): Available size of the output area.
//     compress_manager (EsfCodecJpegCompressManager *): Pointer to JPEG
//       compression manager. Null input is not allowed.
// Returns:
//     kJpegSuccess: On normal termination, it returns.
//     kJpegParamError: - If the arguments buffer or compress_manager are
//                          NULL, it returns.
// """
EsfCodecJpegError EsfCodecJpegSetDestManager(
    const uint8_t *buffer, size_t size,
    EsfCodecJpegCompressManager *compress_manager);

// """Sets FileIO access parameters in the JPEG output manager.
// Sets FileIO access parameters in the JPEG output manager.
// Args:
//     buffer (uint8_t *): Starting address of the output area. NULL input
//       is not allowed.
//     size (size_t): Available size of the output area.
//     output_file_handle (EsfMemoryManagerHandle): Output side MemoryManager's
//     FileIO handle.
//     compress_manager (EsfCodecJpegCompressManager *): Pointer
//     to JPEG
//       compression manager. Null input is not allowed.
// Returns:
//     kJpegSuccess: On normal termination, it returns.
//     kJpegParamError: - If the arguments buffer or compress_manager are
//                          NULL, it returns.
// """
EsfCodecJpegError EsfCodecJpegSetDestManagerFileIo(
    uint8_t *buffer, size_t size, EsfMemoryManagerHandle output_file_handle,
    EsfCodecJpegCompressManager *compress_manager);

// """Compresses an image using the JPEG codec.

// This function compresses an image using the JPEG codec. It takes as input
// the output size and a compress manager object. The output size is a pointer
// to an integer that will be updated with the size of the compressed image
// data. The compress manager object contains information about the image to be
// compressed, such as its format, dimensions, and stride. The function returns
// a EsfCodecJpegError value indicating the success or failure of the
// compression process.

// Args:
//     output_size (int32_t *): Store the size of the output Jpeg image. Null
//       input is not allowed.
//     compress_manager (EsfCodecJpegCompressManager *): Pointer to JPEG
//       compression manager. Please input a parameter that has been set using
//       EsfCodecJpegSetParam(). Null input is not allowed.

// Returns:
//     kJpegSuccess: On normal termination, it returns.
//     kJpegParamError: If the argument output_size, compress_manager, or
//       jpeg_object is NULL, or if EsfCodecJpegConvertLine() returns
//       kJpegParamError.
//     kJpegMemAllocError: Return if EsfCodecJpegConvertLine() returns
//       kJpegMemAllocError.
//     kJpegOutputBufferFullError: If the output buffer is insufficient during
//     JPEG
//       compression, return.
// """
EsfCodecJpegError EsfCodecJpegCompressImage(
    int32_t *output_size, EsfCodecJpegCompressManager *compress_manager);

// """Determines if the file_handle is a FileIO handle and confirms it is open.

// Utilizes EsfCodecJpegIsFileHandle() to check if the file_handle is valid
// for FileIO operations.
// Additionally, it tries a EsfMemoryManagerFseek() to ensure the handle is
// open. Returns true if all conditions are met, otherwise returns false.

// Args:
//     file_handle (EsfMemoryManagerHandle): Memory manager handle.

// Returns:
//     true:  If file_handle is a valid and open FileIO handle.
//     false: If file_handle is not a valid or open FileIO handle.
// """
bool EsfCodecJpegIsFileHandleOpen(EsfMemoryManagerHandle file_handle);

// """Executes JPEG encoding using memory mapping techniques to handle image
// data. This function performs parameter validation, calculates input buffer
// size, maps input data into memory, and executes the encoding process.

// Args:
//     input_handle (EsfMemoryManagerHandle):
//       Input side MemoryManager's handle.
//     output_handle (EsfMemoryManagerHandle):
//       Output side MemoryManager's handle.
//     info (const struct EsfCodecJpegInfo *):
//       Pointer to the JPEG encoding parameters. NULL assignment is not
//       allowed.
//     jpeg_size (int32_t *):
//       Pointer to store the size of the resulting JPEG image. NULL assignment
//       is not allowed.

// Returns:
//     kJpegSuccess: Normal termination.
//     kJpegParamError: When info is NULL.
//                      When the value of info is invalid.
//                      When jpeg_size is NULL.
//                      When input_handle or output_handle is not a
//                        LargeHeap handle.
//     kJpegOssInternalError: An error occurred internally in the OSS.
//     kJpegMemAllocError: If memory allocation fails.
//     kJpegOtherError: Other Errors.
//     kJpegOutputBufferFullError: If the output buffer is insufficient during
//       JPEG compression, return.
// """
EsfCodecJpegError EsfCodecJpegEncodeHandleMap(
    EsfMemoryManagerHandle input_handle, EsfMemoryManagerHandle output_handle,
    const EsfCodecJpegInfo *info, int32_t *jpeg_size);

// """Executes JPEG encoding using file I/O operations to handle image data.
// This function performs parameter validation, confirms file handle status,
// reads input data via file operations, and executes the encoding process.

// Args:
//     input_handle (EsfMemoryManagerHandle):
//       Input side MemoryManager's handle.
//     output_handle (EsfMemoryManagerHandle):
//       Output side MemoryManager's handle.
//     info (const struct EsfCodecJpegInfo *):
//       Pointer to the JPEG encoding parameters. NULL assignment is not
//       allowed.
//     jpeg_size (int32_t *):
//       Pointer to store the size of the resulting JPEG image. NULL assignment
//       is not allowed.

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
EsfCodecJpegError EsfCodecJpegEncodeHandleFileIo(
    EsfMemoryManagerHandle input_handle, EsfMemoryManagerHandle output_handle,
    const EsfCodecJpegInfo *info, int32_t *jpeg_size);

#ifdef __cplusplus
}
#endif

#endif  // ESF_CODEC_JPEG_SRC_JPEG_INTERNAL_H_
