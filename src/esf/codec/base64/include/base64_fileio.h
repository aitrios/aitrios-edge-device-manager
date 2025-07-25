/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESF_CODEC_BASE64_BASE64_FILEIO_H_
#define ESF_CODEC_BASE64_BASE64_FILEIO_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>

#include "base64.h"
#include "memory_manager.h"

// function
// """Base64 Encoding (FileIO)

// Description: This encoding converts the original data on Lheap to a Base64
//              string and stores it in Lheap.

// Args:
//     [IN] in_handle (EsfMemoryManagerHandle): MemoryManager's FileIO handle
//          where the data to be Base64 encoded is stored.
//     [IN] in_size (size_t): Size of the data to be Base64 encoded (in bytes).
//     [IN] out_handle (EsfMemoryManagerHandle): MemoryManager's FileIO handle
//          where the Base64 encoded result will be stored.
//     [IN/OUT] out_size (size_t*):
//              [IN]  Size of the data (in bytes) that
//                    can be stored in out_handle.
//              [OUT] Size of the Base64 encoded string
//                    including the null terminator (in bytes).

// Returns:
//     kEsfCodecBase64ResultSuccess: Success.
//     kEsfCodecBase64ResultNullParam: Args in or out is a NULL.
//     kEsfCodecBase64ResultOutOfRange: Args in or out is out of range.
//     kEsfCodecBase64ResultExceedsOutBuffer: Base64 string exceeds out buffer.
//     kEsfCodecBase64ResultInternalError: Internal processing error.
//     kEsfCodecBase64ResultExternalError: External processing error.
//     kEsfCodecBase64NotSupported: This API is not supported on this device.
//
EsfCodecBase64ResultEnum EsfCodecBase64EncodeFileIO(
    EsfMemoryManagerHandle in_handle, size_t in_size,
    EsfMemoryManagerHandle out_handle, size_t* out_size);

#ifdef __cplusplus
}
#endif
#endif  // ESF_CODEC_BASE64_BASE64_FILEIO_H_
