/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESF_CODEC_BASE64_BASE64_H_
#define ESF_CODEC_BASE64_BASE64_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>

#include "memory_manager.h"

// define
// There is a process in OSS that multiplies the input string length by 3. As a
// countermeasure against overflow, the maximum value of the type divided by 3
// is defined as the maximum value in Base64.
// If SIZE_MAX is greater than UINT_MAX, UINT_MAX is treated as the maximum
// value.
#if SIZE_MAX > UINT_MAX
#define ESF_BASE64_MAX_SIZE (UINT_MAX / 3)
#else
#define ESF_BASE64_MAX_SIZE (SIZE_MAX / 3)
#endif

// enum
// This code defines an enumeration type for the result of executing an API.
typedef enum EsfCodecBase64ResultEnum {
  kEsfCodecBase64ResultSuccess = 0,       // Execution is successful.
  kEsfCodecBase64ResultNullParam,         // Parameter is NULL .
  kEsfCodecBase64ResultOutOfRange,        // Parameter is out of range.
  kEsfCodecBase64ResultExceedsOutBuffer,  // Data size after processing exceeds
                                          // buffer area.
  kEsfCodecBase64ResultIllegalInSize,     // Parameter is illegal size.
  kEsfCodecBase64ResultIllegalInData,     // Parameter is illegal data.
  kEsfCodecBase64ResultInternalError,     // Internal processing error.
  kEsfCodecBase64ResultExternalError,     // External processing error.
  kEsfCodecBase64NotSupported  // This API is not supported on this device.
} EsfCodecBase64ResultEnum;

// This code defines an enumeration type for each parameter value to be used in
// Base64.
typedef enum Base64ParamNum {
  // Minimum size of data to be encoded in Base64.
  kBase64EncodeInDataMinSize = 1,
  // Minimum value of the buffer for storing Base64 encoding results.
  kBase64EncodeOutBufMinSize = 0,
  // Minimum data size for Base64 decoding.
  kBase64DecodeInDataMinSize = 4,
  // Minimum size of the buffer for storing Base64 decode results.
  kBase64DecodeOutBufMinSize = 0,
  // Minimum value for calculating the data size for Base64 decoding.
  kBase64DecodeSizeInDataMinSize = 2,
  // Unit of data to be converted for Base64 encoding.
  kBase64EncodeConvertDataUnit = 3,
  // Unit for processing Base64 encode.
  kBase64EncodeUnit = 4
} Base64ParamNum;

// function
// """Base64 Encoding

// Description: This encording from original data(ex.binary) to base64 string.

// Args:
//     [IN] in (const uint8_t*): Original data buffer.
//                               NULL assignment not allowed.
//     [IN] in_size (size_t): Original data size.
//     [OUT] out (char*): Base64 string buffer.
//                        NULL assignment not allowed.
//     [IN/OUT] out_size (size_t*): [IN] Base64 string buffer size.
//                                  [OUT] Base64 string size.
//

// Returns:
//     kEsfCodecBase64ResultSuccess: Success.
//     kEsfCodecBase64ResultNullParam: Args in or out is a NULL.
//     kEsfCodecBase64ResultOutOfRange: Args in or out is out of range.
//     kEsfCodecBase64ResultExceedsOutBuffer: Base64 string exceeds out buffer.
//
EsfCodecBase64ResultEnum EsfCodecBase64Encode(const uint8_t* in, size_t in_size,
                                              char* out, size_t* out_size);

// """Base64 Decoding

// Description: This decoding from base64 string to original data.

// Args:
//     [IN] in (const char*): Base64 string buffer.
//                            NULL assignment not allowed.
//     [IN] in_size (size_t): Base64 string buffer size.
//     [OUT] out (uint_8_t*): Original data buffer.
//                            NULL assignment not allowed.
//     [IN/OUT] out_size (size_t*): [IN] Original data buffer size.
//                                  [OUT] Base64 string size.
//

// Returns:
//     kEsfCodecBase64ResultSuccess: Success.
//     kEsfCodecBase64ResultNullParam: Arg in or out is a NULL.
//     kEsfCodecBase64ResultOutOfRange: Arg in or out is out of range.
//     kEsfCodecBase64ResultExceedsOutBuffer: Original data exceeds out buffer.
//     kEsfCodecBase64ResultIllegalInSize: Arg in_size is illegal.
//     kEsfCodecBase64ResultIllegalInData: Arg in is illegal because Arg in
//                        contains characters that do not correspond to Base64.
//
EsfCodecBase64ResultEnum EsfCodecBase64Decode(const char* in, size_t in_size,
                                              uint8_t* out, size_t* out_size);

// """Get Base64 Encoding Size

// Description: Get base64 encoded size from original data size.

// Args:
//     [IN] in_size (size_t): Base64 string buffer size.

// Returns:
//     return > 0 : "return" is size after base64 decoding
//     0          : Arg in_size is out of range.
//
size_t EsfCodecBase64GetEncodeSize(size_t in_size);

// """Get Base64 Encoding Size

// Description: Get original data size from base64 encoded size.

// Args:
//     [IN] in_size (size_t): Original data size.

// Returns:
//     return > 0 : "return" is size after base64 encoding
//     0          : Arg in_size is out of range.
//
size_t EsfCodecBase64GetDecodeSize(size_t in_size);

// """EsfCodecBase64EncodeHandle

// Encodes data using Base64, selecting the appropriate method based on memory
// handle support for mapping.

// Args:
//     [IN] in_handle (EsfMemoryManagerHandle): Handle for the input data.
//     [IN] in_size (size_t): Size of the input data.
//     [OUT] out_handle (EsfMemoryManagerHandle): Handle for the output buffer.
//     [IN, OUT] out_size (size_t*): Buffer size for output on input; contains
//     size of encoded data with null terminator on output.

// Returns:
//     One of the values of EsfCodecBase64ResultEnum is returned
//     depending on the execution result.

// Yields:
//     kEsfCodecBase64ResultSuccess: Success.
//     kEsfCodecBase64ResultNullParam: Args in or out is a NULL.
//     kEsfCodecBase64ResultOutOfRange: Args in or out is out of range.
//     kEsfCodecBase64ResultExceedsOutBuffer: Base64 string exceeds out buffer.
//     kEsfCodecBase64ResultExternalError: External processing error.
//     kEsfCodecBase64NotSupported: This API is not supported on this device.

// Note:

// """
EsfCodecBase64ResultEnum EsfCodecBase64EncodeHandle(
    EsfMemoryManagerHandle in_handle, size_t in_size,
    EsfMemoryManagerHandle out_handle, size_t* out_size);

#ifdef __cplusplus
}
#endif
#endif  // ESF_CODEC_BASE64_BASE64_H_
