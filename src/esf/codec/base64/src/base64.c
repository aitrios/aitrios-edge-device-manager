/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "base64_lib/include/base64.h"  // base64_lib

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>

#include "include/base64.h"
#include "include/base64_fileio.h"
#include "memory_manager.h"
#include "src/base64_log.h"
#include "utility_log.h"

// Mutex object for FileIO access state.
static pthread_mutex_t file_io_mutex = PTHREAD_MUTEX_INITIALIZER;

// """EsfCodecBase64CheckMaxSizeForEncode

// Check the maximum data size that can be processed for Base64 encoding.

// Args:
//     [IN] in_size (size_t): Data size (bytes) to be Base64 encoded.

// Returns:
//     One of the values of EsfCodecBase64ResultEnum is returned
//     depending on the execution result.

// Yields:
//     kEsfCodecBase64ResultSuccess: Success.
//     kEsfCodecBase64ResultOutOfRange: in_size is out of range.

// Note:

// """
static EsfCodecBase64ResultEnum EsfCodecBase64CheckMaxSizeForEncode(
    size_t in_size) {
  ESF_CODEC_BASE64_TRACE("func start");
  if ((((ESF_BASE64_MAX_SIZE - 1) / kBase64EncodeUnit) * kBase64EncodeUnit + 1 -
       1) *
          kBase64EncodeConvertDataUnit / kBase64EncodeUnit <
      in_size) {
    ESF_CODEC_BASE64_ERR("The maximum value is exceeded. in_size=%zu", in_size);
    ESF_CODEC_BASE64_TRACE("func end");
    return kEsfCodecBase64ResultOutOfRange;
  }
  ESF_CODEC_BASE64_TRACE("func end");
  return kEsfCodecBase64ResultSuccess;
}

// """EsfCodecBase64CheckMaxSizeForDecode

// Check the maximum data size that can be processed for Base64 decoding.

// Args:
//     [IN] in_size (size_t): Data size (bytes) for Base64 decoding.

// Returns:
//     One of the values of EsfCodecBase64ResultEnum is returned
//     depending on the execution result.

// Yields:
//     kEsfCodecBase64ResultSuccess: Success.
//     kEsfCodecBase64ResultOutOfRange: in_size is out of range.

// Note:

// """
static EsfCodecBase64ResultEnum EsfCodecBase64CheckMaxSizeForDecode(
    size_t in_size) {
  ESF_CODEC_BASE64_TRACE("func start");
  if ((ESF_BASE64_MAX_SIZE / kBase64EncodeUnit) * kBase64EncodeUnit < in_size) {
    ESF_CODEC_BASE64_ERR("The maximum value is exceeded. in_size=%zu", in_size);
    ESF_CODEC_BASE64_TRACE("func end");
    return kEsfCodecBase64ResultOutOfRange;
  }
  ESF_CODEC_BASE64_TRACE("func end");
  return kEsfCodecBase64ResultSuccess;
}

// """EsfCodecBase64CheckOutBufferOverFlowForEncode

// Check for overflow of the buffer that stores the result of the Base64
// encoding process.

// Args:
//     [IN] in_size (size_t): Data size (bytes) to be Base64 encoded.
//     [IN] out_size (size_t): Buffer size (in bytes) to store Base64 encoded
//                             string size results.

// Returns:
//     One of the values of EsfCodecBase64ResultEnum is returned
//     depending on the execution result.

// Yields:
//     kEsfCodecBase64ResultSuccess: Success.
//     kEsfCodecBase64ResultExceedsOutBuffer: Encode result size exceeds
//                                            out_size buffer.

// Note:

// """
static EsfCodecBase64ResultEnum EsfCodecBase64CheckOutBufferOverFlowForEncode(
    size_t in_size, size_t out_size) {
  ESF_CODEC_BASE64_TRACE("func start");
  if ((out_size - 1) * kBase64EncodeConvertDataUnit / kBase64EncodeUnit <
      in_size) {
    ESF_CODEC_BASE64_ERR(
        "The buffer area to store the encorded output is exceeded. "
        "in_size=%zu, out_size=%zu",
        in_size, out_size);
    ESF_CODEC_BASE64_TRACE("func end");
    return kEsfCodecBase64ResultExceedsOutBuffer;
  }
  ESF_CODEC_BASE64_TRACE("func end");
  return kEsfCodecBase64ResultSuccess;
}

// """EsfCodecBase64CheckOutBufferOverFlowForDecode

// Check for overflow of the buffer that stores the result of the Base64
// decoding process.

// Args:
//     [IN] in_size (size_t): Data size (bytes) for Base64 decoding.
//     [IN] out_size (size_t): Base64 decode data size buffer size (bytes) to
//                             store the result.

// Returns:
//     One of the values of EsfCodecBase64ResultEnum is returned
//     depending on the execution result.

// Yields:
//     kEsfCodecBase64ResultSuccess: Success.
//     kEsfCodecBase64ResultExceedsOutBuffer: Decode result size exceeds
//                                            out_size buffer.

// Note:

// """
static EsfCodecBase64ResultEnum EsfCodecBase64CheckOutBufferOverFlowForDecode(
    size_t in_size, size_t out_size) {
  ESF_CODEC_BASE64_TRACE("func start");
  if (out_size * kBase64EncodeUnit / kBase64EncodeConvertDataUnit < in_size) {
    ESF_CODEC_BASE64_ERR(
        "The buffer area to store the decoded output is exceeded. "
        "in_size=%zu, out_size=%zu",
        in_size, out_size);
    ESF_CODEC_BASE64_TRACE("func end");
    return kEsfCodecBase64ResultExceedsOutBuffer;
  }
  ESF_CODEC_BASE64_TRACE("func end");
  return kEsfCodecBase64ResultSuccess;
}

// """EsfCodecBase64ValidateCharacter

// Make sure it is Base64 characters.

// Args:
//     [IN] in (char*): Buffer to store data for Base64 decoding; must not be
//                      NULL.
//     [IN] in_size (size_t): Value of data size (bytes) for Base64 decoding.
//                            The terminating character is not included in the
//                            size.

// Returns:
//     One of the values of EsfCodecBase64ResultEnum is returned
//     depending on the execution result.

// Yields:
//     kEsfCodecBase64ResultSuccess: Success.
//     kEsfCodecBase64ResultIllegalInData: "in" that do not correspond to
//                                         Base64 characters.

// Note:

// """
static EsfCodecBase64ResultEnum EsfCodecBase64ValidateCharacter(
    const char* in, size_t in_size) {
  ESF_CODEC_BASE64_TRACE("func start");
  if (in == NULL) {
    ESF_CODEC_BASE64_ERR("Input parameter is NULL.");
    ESF_CODEC_BASE64_TRACE("func end");
    return kEsfCodecBase64ResultNullParam;
  }
  if (in_size < kBase64EncodeInDataMinSize || ESF_BASE64_MAX_SIZE < in_size) {
    ESF_CODEC_BASE64_ERR("Input parameter is out of range. %zu", in_size);
    ESF_CODEC_BASE64_TRACE("func end");
    return kEsfCodecBase64ResultIllegalInSize;
  }
  for (int32_t i = 0; i < (int32_t)in_size; ++i) {
    char target_char = *(in + i);
    if (0 != b64_int((unsigned int)target_char)) {
      continue;
    }
    // Normal if b64_int return value is 0 but target character is A
    if (target_char == 'A') {
      continue;
    } else {
      ESF_CODEC_BASE64_ERR(
          "The %d character of the input string is not a Base64 defined "
          "character. [%c]",
          i + 1, target_char);
      ESF_CODEC_BASE64_TRACE("func end");
      return kEsfCodecBase64ResultIllegalInData;
    }
  }
  ESF_CODEC_BASE64_TRACE("func end");
  return kEsfCodecBase64ResultSuccess;
}

// """EsfCodecBase64CheckParamForEncode

// Performs input parameter checks for Base64 encoding process.

// Args:
//     [IN] in (uint8_t*): Buffer to store data to be Base64 encoded, must not
//                         be NULL.
//     [IN] in_size (size_t): Data size (bytes) value for Base64 encoding
//                            target.
//     [IN] out (char*): Buffer to store the result of Base64 encoding, must
//                       not be NULL.
//     [IN] out_size (size_t*): The buffer size (in bytes) value of out.

// Returns:
//     One of the values of EsfCodecBase64ResultEnum is returned
//     depending on the execution result.

// Yields:
//     kEsfCodecBase64ResultSuccess: Success.
//     kEsfCodecBase64ResultNullParam: Setting "in" or "out" or "out_size" is a
//                                     NULL.
//     kEsfCodecBase64ResultOutOfRange: in_size or out_size is out of range.
//     kEsfCodecBase64ResultExceedsOutBuffer: Encode result size exceeds
//                                            out_size buffer.

// Note:

// """
static EsfCodecBase64ResultEnum EsfCodecBase64CheckParamForEncode(
    const uint8_t* in, size_t in_size, const char* out,
    const size_t* out_size) {
  ESF_CODEC_BASE64_TRACE("func start");
  EsfCodecBase64ResultEnum ret = kEsfCodecBase64ResultSuccess;
  if (in == NULL || out == NULL || out_size == NULL) {
    ESF_CODEC_BASE64_ERR("Input parameter is NULL.");
    ESF_CODEC_BASE64_TRACE("func end");
    return kEsfCodecBase64ResultNullParam;
  }
  if (in_size < kBase64EncodeInDataMinSize) {
    ESF_CODEC_BASE64_ERR("in_size is out of range. in_size=%zu", in_size);
    ESF_CODEC_BASE64_TRACE("func end");
    return kEsfCodecBase64ResultOutOfRange;
  }
  ret = EsfCodecBase64CheckMaxSizeForEncode(in_size);
  if (ret != kEsfCodecBase64ResultSuccess) {
    ESF_CODEC_BASE64_ERR("EsfCodecBase64CheckMaxSizeForEncode=%d", ret);
    ESF_CODEC_BASE64_TRACE("func end");
    return ret;
  }
  if (*out_size <= kBase64EncodeOutBufMinSize ||
      ESF_BASE64_MAX_SIZE < *out_size) {
    ESF_CODEC_BASE64_ERR("out_size is out of range. out_size=%zu", *out_size);
    ESF_CODEC_BASE64_TRACE("func end");
    return kEsfCodecBase64ResultOutOfRange;
  }
  ret = EsfCodecBase64CheckOutBufferOverFlowForEncode(in_size, *out_size);
  if (ret != kEsfCodecBase64ResultSuccess) {
    ESF_CODEC_BASE64_ERR("EsfCodecBase64CheckOutBufferOverFlowForEncode=%d",
                         ret);
    ESF_CODEC_BASE64_TRACE("func end");
    return ret;
  }
  ESF_CODEC_BASE64_TRACE("func end");
  return kEsfCodecBase64ResultSuccess;
}

// """EsfCodecBase64CheckParamForDecode

// Performs input parameter checks for Base64 decoding process.

// Args:
//     [IN] in (uint8_t*): Buffer to store data for Base64 decoding; must not
//                         be NULL.
//     [IN] in_size (size_t): Data size (in bytes) value for Base64 decoding.
//                            The terminating character is not included in the
//                            size.
//     [IN] out (char*): Buffer to store the result of Base64 decoding.
//     [IN] out_size (size_t*): The size (in bytes) of the buffer for out.

// Returns:
//     One of the values of EsfCodecBase64ResultEnum is returned
//     depending on the execution result.

// Yields:
//     kEsfCodecBase64ResultSuccess: Success.
//     kEsfCodecBase64ResultNullParam: Setting "in" or "out" or "out_size" is a
//                                     NULL.
//     kEsfCodecBase64ResultOutOfRange: in_size or out_size is out of range.
//     kEsfCodecBase64ResultExceedsOutBuffer: Decode result size exceeds
//                                            out_size buffer.
//     kEsfCodecBase64ResultIllegalInSize: "in" is not in 4-character units.
//     kEsfCodecBase64ResultIllegalInData: "in" contains non-base64 characters.

// Note:

// """
static EsfCodecBase64ResultEnum EsfCodecBase64CheckParamForDecode(
    const char* in, size_t in_size, const uint8_t* out,
    const size_t* out_size) {
  ESF_CODEC_BASE64_TRACE("func start");
  EsfCodecBase64ResultEnum ret = kEsfCodecBase64ResultSuccess;
  if (in == NULL || out == NULL || out_size == NULL) {
    ESF_CODEC_BASE64_ERR("Input parameter is NULL.");
    ESF_CODEC_BASE64_TRACE("func end");
    return kEsfCodecBase64ResultNullParam;
  }
  if (in_size < kBase64DecodeInDataMinSize) {
    ESF_CODEC_BASE64_ERR("in_size is out of range. in_size=%zu", in_size);
    ESF_CODEC_BASE64_TRACE("func end");
    return kEsfCodecBase64ResultOutOfRange;
  }
  ret = EsfCodecBase64CheckMaxSizeForDecode(in_size);
  if (ret != kEsfCodecBase64ResultSuccess) {
    ESF_CODEC_BASE64_ERR("EsfCodecBase64CheckMaxSizeForDecode=%d", ret);
    ESF_CODEC_BASE64_TRACE("func end");
    return ret;
  }
  if (0 != (in_size % kBase64EncodeUnit)) {
    ESF_CODEC_BASE64_ERR("in_size is not in increments of 4 characters.");
    ESF_CODEC_BASE64_TRACE("func end");
    return kEsfCodecBase64ResultIllegalInSize;
  }
  if (*out_size <= kBase64DecodeOutBufMinSize ||
      ESF_BASE64_MAX_SIZE < *out_size) {
    ESF_CODEC_BASE64_ERR("out_size is out of range. out_size=%zu", *out_size);
    ESF_CODEC_BASE64_TRACE("func end");
    return kEsfCodecBase64ResultOutOfRange;
  }
  ret = EsfCodecBase64CheckOutBufferOverFlowForDecode(in_size, *out_size);
  if (ret != kEsfCodecBase64ResultSuccess) {
    ESF_CODEC_BASE64_ERR("EsfCodecBase64CheckOutBufferOverFlowForDecode=%d",
                         ret);
    ESF_CODEC_BASE64_TRACE("func end");
    return ret;
  }
  ret = EsfCodecBase64ValidateCharacter(in, in_size);
  if (ret != kEsfCodecBase64ResultSuccess) {
    ESF_CODEC_BASE64_ERR("EsfCodecBase64ValidateCharacter=%d", ret);
    ESF_CODEC_BASE64_TRACE("func end");
    return ret;
  }
  ESF_CODEC_BASE64_TRACE("func end");
  return kEsfCodecBase64ResultSuccess;
}

// """EsfCodecBase64CheckParamForGetEncodeSize

// Performs input parameter checking for Base64-encoded string size get.

// Args:
//     [IN] in_size (size_t): data size (bytes) to be encoded in Base64.

// Returns:
//     One of the values of EsfCodecBase64ResultEnum is returned
//     depending on the execution result.

// Yields:
//     kEsfCodecBase64ResultSuccess: Success.
//     kEsfCodecBase64ResultOutOfRange: in_size is out of range.

// Note:

// """
static EsfCodecBase64ResultEnum EsfCodecBase64CheckParamForGetEncodeSize(
    size_t in_size) {
  ESF_CODEC_BASE64_TRACE("func start");
  if (in_size < kBase64EncodeInDataMinSize) {
    ESF_CODEC_BASE64_ERR("in_size is out of range. in_size=%zu", in_size);
    return kEsfCodecBase64ResultOutOfRange;
    ESF_CODEC_BASE64_TRACE("func end");
  }
  EsfCodecBase64ResultEnum ret = EsfCodecBase64CheckMaxSizeForEncode(in_size);
  if (ret != kEsfCodecBase64ResultSuccess) {
    ESF_CODEC_BASE64_ERR("EsfCodecBase64CheckMaxSizeForEncode=%d", ret);
    ESF_CODEC_BASE64_TRACE("func end");
    return ret;
  }
  ESF_CODEC_BASE64_TRACE("func end");
  return kEsfCodecBase64ResultSuccess;
}

// """EsfCodecBase64CheckParamForGetDecodeSize

// Performs input parameter check for Base64 decode data size get.

// Args:
//     [IN] in_size (size_t): Data size (bytes) for Base64 decoding. Terminating
//                            characters are not included in the size.

// Returns:
//     One of the values of EsfCodecBase64ResultEnum is returned
//     depending on the execution result.

// Yields:
//     kEsfCodecBase64ResultSuccess: Success.
//     kEsfCodecBase64ResultOutOfRange: in_size is out of range.

// Note:

// """
static EsfCodecBase64ResultEnum EsfCodecBase64CheckParamForGetDecodeSize(
    size_t in_size) {
  ESF_CODEC_BASE64_TRACE("func start");
  if (in_size < kBase64DecodeSizeInDataMinSize ||
      ESF_BASE64_MAX_SIZE < in_size) {
    ESF_CODEC_BASE64_ERR("in_size is out of range. in_size=%zu", in_size);
    ESF_CODEC_BASE64_TRACE("func end");
    return kEsfCodecBase64ResultOutOfRange;
  }
  ESF_CODEC_BASE64_TRACE("func end");
  return kEsfCodecBase64ResultSuccess;
}

// """Determines the FileIO handle.

// If the result of EsfCodecBase64IsFileIoAccess() is not
// kEsfMemoryManagerTargetLargeHeap, return false. Additionally, if the
// execution result is not kEsfMemoryManagerResultSuccess, return false. If the
// result of EsfMemoryManagerIsMapSupport() is kEsfMemoryManagerMapIsSupport,
// return false. Additionally, if the execution result is not
// kEsfMemoryManagerResultSuccess, return false. If neither of the above
// conditions are met, it is considered a FileIO handle, so return true.

// Args:
//     [IN] file_handle (EsfMemoryManagerHandle): Memory manager handle.

// Returns:
//     true:  If file_handle is a FileIO handle.
//     false: If file_handle is not a FileIO handle: false

// Note:

// """
static bool EsfCodecBase64IsFileIoAccess(EsfMemoryManagerHandle file_handle) {
  {
    // Check large heap.
    EsfMemoryManagerHandleInfo info;
    memset(&info, 0x0, sizeof(EsfMemoryManagerHandleInfo));
    EsfMemoryManagerResult result =
        EsfMemoryManagerGetHandleInfo((uint32_t)file_handle, &info);
    if (result != kEsfMemoryManagerResultSuccess) {
      ESF_CODEC_BASE64_ERR("Failed to get handle info.");
      return false;
    }

    if (info.target_area != kEsfMemoryManagerTargetLargeHeap) {
      ESF_CODEC_BASE64_ERR("Target area is not LargeHeap.");
      return false;
    }
  }

  {
    // Check FileIO.
    EsfMemoryManagerMapSupport support = kEsfMemoryManagerMapIsSupport;
    EsfMemoryManagerResult result = EsfMemoryManagerIsMapSupport(file_handle,
                                                                 &support);
    if (result != kEsfMemoryManagerResultSuccess) {
      ESF_CODEC_BASE64_ERR("Failed to check map support.");
      return false;
    }

    if (support == kEsfMemoryManagerMapIsSupport) {
      ESF_CODEC_BASE64_ERR("Map is supported.");
      return false;
    }
  }

  return true;
}

// """EsfCodecBase64MutexLock

// Starting exclusive control of the Base64 function using a mutex.

// Args:
//     [IN] mutex (pthread_mutex_t*): It is a mutex structure.

// Returns:
//     One of the values of EsfCodecBase64ResultEnum is returned
//     depending on the execution result.

// Yields:
//     kEsfCodecBase64ResultSuccess: Success.
//     kEsfCodecBase64ResultExternalError: External processing error.

// Note:

// """
static EsfCodecBase64ResultEnum EsfCodecBase64MutexLock(
    pthread_mutex_t* mutex) {
  ESF_CODEC_BASE64_TRACE("func start");

  if (mutex == NULL) {
    ESF_CODEC_BASE64_ERR("mutex is NULL.");
    ESF_CODEC_BASE64_TRACE("func end");
    return kEsfCodecBase64ResultInternalError;
  }

  // Try to acquire the mutex
  int ret = pthread_mutex_lock(mutex);
  if (ret != 0) {
    ESF_CODEC_BASE64_ERR("pthread_mutex_lock ret=%d", ret);
    ESF_CODEC_BASE64_TRACE("func end");
    return kEsfCodecBase64ResultExternalError;
  }
  ESF_CODEC_BASE64_DBG("mutex lock.");

  ESF_CODEC_BASE64_TRACE("func end");
  return kEsfCodecBase64ResultSuccess;
}

// """EsfCodecBase64MutexUnlock

// Releasing exclusive control of the Base64 using a mutex.

// Args:
//     [IN] mutex (pthread_mutex_t*): It is a mutex structure.

// Returns:
//     One of the values of EsfCodecBase64ResultEnum is returned
//     depending on the execution result.

// Yields:
//     kEsfCodecBase64ResultSuccess: Success.
//     kEsfCodecBase64ResultInternalError: Internal processing error.
//     kEsfCodecBase64ResultExternalError: External processing error.

// Note:

// """
static EsfCodecBase64ResultEnum EsfCodecBase64MutexUnlock(
    pthread_mutex_t* mutex) {
  ESF_CODEC_BASE64_TRACE("func start");

  if (mutex == NULL) {
    ESF_CODEC_BASE64_ERR("mutex is NULL.");
    ESF_CODEC_BASE64_TRACE("func end");
    return kEsfCodecBase64ResultInternalError;
  }

  int ret = pthread_mutex_unlock(mutex);
  if (ret != 0) {
    ESF_CODEC_BASE64_ERR("pthread_mutex_unlock ret=%d", ret);
    ESF_CODEC_BASE64_TRACE("func end");
    return kEsfCodecBase64ResultExternalError;
  }
  ESF_CODEC_BASE64_DBG("mutex unlock.");

  ESF_CODEC_BASE64_TRACE("func end");
  return kEsfCodecBase64ResultSuccess;
}

// """EsfCodecBase64SplitWorkArea

// The work area size is divided into the size for the source data to be encoded
// and the size for the encoded data, and the values are returned. Set in_size
// to a multiple of 3.

// Args:
//     [OUT] in_size (size_t*): the size for the source data to be encoded
//     [OUT] out_size (size_t*): the size for the encoded data

// Returns:
//     One of the values of EsfCodecBase64ResultEnum is returned
//     depending on the execution result.

// Yields:
//     kEsfCodecBase64ResultSuccess: Success.
//     kEsfCodecBase64ResultInternalError: Internal processing error.

// Note:
//     The work area size is specified in
//     CONFIG_EXTERNAL_CODEC_BASE64_FILEIO_WORK_SIZE.
// """
static EsfCodecBase64ResultEnum EsfCodecBase64SplitWorkArea(size_t* in_size,
                                                            size_t* out_size) {
  ESF_CODEC_BASE64_TRACE("func start");

  if (in_size == NULL || out_size == NULL) {
    ESF_CODEC_BASE64_ERR("parameter is NULL.");
    ESF_CODEC_BASE64_TRACE("func end");
    return kEsfCodecBase64ResultInternalError;
  }

  size_t work_size = CONFIG_EXTERNAL_CODEC_BASE64_FILEIO_WORK_SIZE;

  *in_size = (work_size * 3) / 7;
  *in_size -= *in_size % 3;
  *out_size = EsfCodecBase64GetEncodeSize(*in_size);
  ESF_CODEC_BASE64_DBG("work_size=%zu, in_size=%zu, out_size=%zu", work_size,
                       *in_size, *out_size);

  ESF_CODEC_BASE64_TRACE("func end");
  return kEsfCodecBase64ResultSuccess;
}

// """EsfCodecBase64EncodeHandleMap

// Encodes data using Base64, handling memory mapping for input and output.

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
//     kEsfCodecBase64ResultExternalError: External processing error.
//     kEsfCodecBase64ResultNullParam: Args in or out is a NULL.
//     kEsfCodecBase64ResultOutOfRange: Args in or out is out of range.
//     kEsfCodecBase64ResultExceedsOutBuffer: Base64 string exceeds out buffer.

// Note:

// """
static EsfCodecBase64ResultEnum EsfCodecBase64EncodeHandleMap(
    EsfMemoryManagerHandle in_handle, size_t in_size,
    EsfMemoryManagerHandle out_handle, size_t* out_size) {
  ESF_CODEC_BASE64_TRACE("func start");
  EsfCodecBase64ResultEnum result = kEsfCodecBase64ResultSuccess;

  // 1. validate parameters
  if ((in_handle == (EsfMemoryManagerHandle)0) ||
      (out_handle == (EsfMemoryManagerHandle)0)) {
    ESF_CODEC_BASE64_ERR("NULL handle error");
    ESF_CODEC_BASE64_ELOG_WARN(ESF_CODEC_BASE64_ELOG_INVALID_PARAM);
    ESF_CODEC_BASE64_TRACE("func end");
    return kEsfCodecBase64ResultNullParam;
  }

  {
    const uint8_t in_dummy = 0;
    const char out_dummy = 0;
    result = EsfCodecBase64CheckParamForEncode(&in_dummy, in_size, &out_dummy,
                                               out_size);
    if (result != kEsfCodecBase64ResultSuccess) {
      ESF_CODEC_BASE64_ERR("EsfCodecBase64CheckParamForEncode error. ret=%d",
                           result);
      ESF_CODEC_BASE64_ELOG_WARN(ESF_CODEC_BASE64_ELOG_INVALID_PARAM);
      ESF_CODEC_BASE64_TRACE("func end");
      return result;
    }
  }

  // 2. map input data to memory
  const uint8_t* input_data = NULL;
  EsfMemoryManagerResult mm_ret = EsfMemoryManagerMap(
      in_handle, NULL, (int32_t)in_size, (void**)&input_data);
  if (mm_ret != kEsfMemoryManagerResultSuccess) {
    ESF_CODEC_BASE64_ERR(
        "EsfMemoryManagerMap for input handle failed. ret = %u", mm_ret);
    ESF_CODEC_BASE64_TRACE("func end");
    return kEsfCodecBase64ResultExternalError;
  }

  // 3. map output data to memory
  char* output_data = NULL;
  mm_ret = EsfMemoryManagerMap(out_handle, NULL, (int32_t)(*out_size),
                               (void**)&output_data);
  if (mm_ret != kEsfMemoryManagerResultSuccess) {
    ESF_CODEC_BASE64_ERR(
        "EsfMemoryManagerMap for output handle failed. ret = %u", mm_ret);
    (void)EsfMemoryManagerUnmap(in_handle, (void**)&input_data);
    ESF_CODEC_BASE64_TRACE("func end");
    return kEsfCodecBase64ResultExternalError;
  }

  // 4. Base64 encoding
  result = EsfCodecBase64Encode(input_data, in_size, output_data, out_size);
  if (result != kEsfCodecBase64ResultSuccess) {
    ESF_CODEC_BASE64_ERR("EsfCodecBase64Encode failed. ret=%d", result);
    (void)EsfMemoryManagerUnmap(in_handle, (void**)&input_data);
    (void)EsfMemoryManagerUnmap(out_handle, (void**)&output_data);
    ESF_CODEC_BASE64_TRACE("func end");
    return result;
  } else {
    ESF_CODEC_BASE64_TRACE("EsfCodecBase64Encode successful, out_size=%zu",
                           *out_size);
  }

  // 5. unmap input data from memory
  mm_ret = EsfMemoryManagerUnmap(in_handle, (void**)&input_data);
  if (mm_ret != kEsfMemoryManagerResultSuccess) {
    ESF_CODEC_BASE64_ERR(
        "EsfMemoryManagerMap for input handle failed. ret = %u", mm_ret);
    result = kEsfCodecBase64ResultExternalError;
  }

  mm_ret = EsfMemoryManagerUnmap(out_handle, (void**)&output_data);
  if (mm_ret != kEsfMemoryManagerResultSuccess) {
    ESF_CODEC_BASE64_ERR(
        "EsfMemoryManagerMap for output handle failed. ret = %u", mm_ret);
    result = kEsfCodecBase64ResultExternalError;
  }

  ESF_CODEC_BASE64_TRACE("func end");
  return result;
}

// """EsfCodecBase64EncodeHandleFileIO

// Encodes data using Base64 with file I/O operations for memory handles.

// Args:
//     [IN] in_handle (EsfMemoryManagerHandle): Handle for the input data file.
//     [IN] in_size (size_t): Size of the input data.
//     [OUT] out_handle (EsfMemoryManagerHandle): Handle for the output data
//     file.
//     [IN, OUT] out_size (size_t*): Buffer size for output on input; contains
//     size of encoded data with null terminator on output.

// Returns:
//     kEsfCodecBase64ResultSuccess: Success.
//     kEsfCodecBase64ResultNullParam: Args in or out is a NULL.
//     kEsfCodecBase64ResultOutOfRange: Args in or out is out of range.
//     kEsfCodecBase64ResultExceedsOutBuffer: Base64 string exceeds out buffer.
//     kEsfCodecBase64ResultInternalError: Internal processing error.
//     kEsfCodecBase64ResultExternalError: External processing error.
//     kEsfCodecBase64NotSupported: This API is not supported on this device.

// Note:

// """
static EsfCodecBase64ResultEnum EsfCodecBase64EncodeHandleFileIO(
    EsfMemoryManagerHandle in_handle, size_t in_size,
    EsfMemoryManagerHandle out_handle, size_t* out_size) {
  ESF_CODEC_BASE64_TRACE("func start");
  EsfCodecBase64ResultEnum result = kEsfCodecBase64ResultSuccess;

  // 1. validate parameters
  if (!EsfCodecBase64IsFileIoAccess(in_handle) ||
      !EsfCodecBase64IsFileIoAccess(out_handle)) {
    ESF_CODEC_BASE64_ERR("Can not FileI/O Access.");
    ESF_CODEC_BASE64_ELOG_WARN(ESF_CODEC_BASE64_ELOG_INVALID_PARAM);
    ESF_CODEC_BASE64_TRACE("func end");
    return kEsfCodecBase64NotSupported;
  }

  {
    const uint8_t in_dummy = 0;
    const char out_dummy = 0;
    result = EsfCodecBase64CheckParamForEncode(&in_dummy, in_size, &out_dummy,
                                               out_size);
    if (result != kEsfCodecBase64ResultSuccess) {
      ESF_CODEC_BASE64_ERR("EsfCodecBase64CheckParamForEncode error. ret=%d",
                           result);
      ESF_CODEC_BASE64_ELOG_WARN(ESF_CODEC_BASE64_ELOG_INVALID_PARAM);
      ESF_CODEC_BASE64_TRACE("func end");
      return result;
    }
  }

  // 2. open handle
  EsfMemoryManagerResult mm_ret = EsfMemoryManagerFopen(in_handle);
  if (mm_ret != kEsfMemoryManagerResultSuccess) {
    ESF_CODEC_BASE64_ERR(
        "EsfMemoryManagerFopen for input handle failed. ret = %u", mm_ret);
    ESF_CODEC_BASE64_TRACE("func end");
    return kEsfCodecBase64ResultExternalError;
  }

  mm_ret = EsfMemoryManagerFopen(out_handle);
  if (mm_ret != kEsfMemoryManagerResultSuccess) {
    ESF_CODEC_BASE64_ERR(
        "EsfMemoryManagerFopen for output handle failed. ret = %u", mm_ret);
    (void)EsfMemoryManagerFclose(in_handle);
    ESF_CODEC_BASE64_TRACE("func end");
    return kEsfCodecBase64ResultExternalError;
  }

  // 3. Base64 encoding
  result = EsfCodecBase64EncodeFileIO(in_handle, in_size, out_handle, out_size);
  if (result != kEsfCodecBase64ResultSuccess) {
    ESF_CODEC_BASE64_ERR("EsfCodecBase64EncodeFileIO failed. ret = %d", result);
    (void)EsfMemoryManagerFclose(in_handle);
    (void)EsfMemoryManagerFclose(out_handle);
    ESF_CODEC_BASE64_TRACE("func end");
    return result;
  } else {
    ESF_CODEC_BASE64_TRACE("output_file_handle=%" PRIu32, out_handle);
    ESF_CODEC_BASE64_TRACE("out_size=%zu", *out_size);
  }

  // 4. close handle
  mm_ret = EsfMemoryManagerFclose(in_handle);
  if (mm_ret != kEsfMemoryManagerResultSuccess) {
    ESF_CODEC_BASE64_ERR(
        "EsfMemoryManagerFclose for input handle failed. ret = %u", mm_ret);
    result = kEsfCodecBase64ResultExternalError;
  }

  mm_ret = EsfMemoryManagerFclose(out_handle);
  if (mm_ret != kEsfMemoryManagerResultSuccess) {
    ESF_CODEC_BASE64_ERR(
        "EsfMemoryManagerFclose for output handle failed. ret = %u", mm_ret);
    result = kEsfCodecBase64ResultExternalError;
  }

  ESF_CODEC_BASE64_TRACE("func end");
  return result;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
EsfCodecBase64ResultEnum EsfCodecBase64Encode(const uint8_t* in, size_t in_size,
                                              char* out, size_t* out_size) {
  ESF_CODEC_BASE64_TRACE("func start");
  EsfCodecBase64ResultEnum ret =
      EsfCodecBase64CheckParamForEncode(in, in_size, out, out_size);
  if (ret != kEsfCodecBase64ResultSuccess) {
    ESF_CODEC_BASE64_ERR("EsfCodecBase64CheckParamForEncode=%d", ret);
    ESF_CODEC_BASE64_ELOG_WARN(ESF_CODEC_BASE64_ELOG_INVALID_PARAM);
    return ret;
  }
  // For b64_encode, "out" sets the string including the terminating character.
  // The return value sets the size without the terminating character, so add 1
  // to out_size.
  *out_size = (size_t)(b64_encode(in, in_size, (unsigned char*)out) + 1U);
  ESF_CODEC_BASE64_TRACE("func end");
  return kEsfCodecBase64ResultSuccess;
}

EsfCodecBase64ResultEnum EsfCodecBase64Decode(const char* in, size_t in_size,
                                              uint8_t* out, size_t* out_size) {
  ESF_CODEC_BASE64_TRACE("func start");
  EsfCodecBase64ResultEnum ret =
      EsfCodecBase64CheckParamForDecode(in, in_size, out, out_size);
  if (ret != kEsfCodecBase64ResultSuccess) {
    ESF_CODEC_BASE64_ERR("EsfCodecBase64CheckParamForDecode=%d", ret);
    ESF_CODEC_BASE64_ELOG_WARN(ESF_CODEC_BASE64_ELOG_INVALID_PARAM);
    return ret;
  }
  *out_size = (size_t)b64_decode((const unsigned char*)in, in_size, out);
  ESF_CODEC_BASE64_TRACE("func end");
  return kEsfCodecBase64ResultSuccess;
}

size_t EsfCodecBase64GetEncodeSize(size_t in_size) {
  ESF_CODEC_BASE64_TRACE("func start");
  EsfCodecBase64ResultEnum ret =
      EsfCodecBase64CheckParamForGetEncodeSize(in_size);
  if (ret != kEsfCodecBase64ResultSuccess) {
    ESF_CODEC_BASE64_ERR("EsfCodecBase64CheckParamForGetEncodeSize=%d", ret);
    ESF_CODEC_BASE64_ELOG_WARN(ESF_CODEC_BASE64_ELOG_INVALID_PARAM);
    return 0;
  }
  // The return value of b64e_size does not include the terminating character,
  // so set the value +1
  ESF_CODEC_BASE64_TRACE("func end");
  return (size_t)(b64e_size(in_size) + 1U);
}

size_t EsfCodecBase64GetDecodeSize(size_t in_size) {
  ESF_CODEC_BASE64_TRACE("func start");
  EsfCodecBase64ResultEnum ret =
      EsfCodecBase64CheckParamForGetDecodeSize(in_size);
  if (ret != kEsfCodecBase64ResultSuccess) {
    ESF_CODEC_BASE64_ERR("EsfCodecBase64CheckParamForGetDecodeSize=%d", ret);
    ESF_CODEC_BASE64_ELOG_WARN(ESF_CODEC_BASE64_ELOG_INVALID_PARAM);
    return 0;
  }
  ESF_CODEC_BASE64_TRACE("func end");
  return (size_t)b64d_size(in_size);
}

EsfCodecBase64ResultEnum EsfCodecBase64EncodeFileIO(
    EsfMemoryManagerHandle in_handle, size_t in_size,
    EsfMemoryManagerHandle out_handle, size_t* out_size) {
  ESF_CODEC_BASE64_TRACE("func start");

  EsfMemoryManagerResult ret = kEsfMemoryManagerResultSuccess;
  EsfCodecBase64ResultEnum result = kEsfCodecBase64ResultSuccess;

  if (!EsfCodecBase64IsFileIoAccess(in_handle) ||
      !EsfCodecBase64IsFileIoAccess(out_handle)) {
    ESF_CODEC_BASE64_ERR("Can not FileI/O Access.");
    ESF_CODEC_BASE64_ELOG_WARN(ESF_CODEC_BASE64_ELOG_INVALID_PARAM);
    ESF_CODEC_BASE64_TRACE("func end");
    return kEsfCodecBase64NotSupported;
  }

  const uint8_t in_dummy = 0;
  const char out_dummy = 0;
  result = EsfCodecBase64CheckParamForEncode(&in_dummy, in_size, &out_dummy,
                                             out_size);
  if (result != kEsfCodecBase64ResultSuccess) {
    ESF_CODEC_BASE64_ERR("EsfCodecBase64CheckParamForEncode error. ret=%d",
                         result);
    ESF_CODEC_BASE64_ELOG_WARN(ESF_CODEC_BASE64_ELOG_INVALID_PARAM);
    ESF_CODEC_BASE64_TRACE("func end");
    return result;
  }

  size_t in_buf_size = 0;
  size_t enc_buf_size = 0;
  result = EsfCodecBase64SplitWorkArea(&in_buf_size, &enc_buf_size);
  if (result != kEsfCodecBase64ResultSuccess) {
    ESF_CODEC_BASE64_ERR("EsfCodecBase64Encode error. ret=%d", result);
    ESF_CODEC_BASE64_ELOG_WARN(ESF_CODEC_BASE64_ELOG_ENCODE_FAILURE);
    ESF_CODEC_BASE64_TRACE("func end");
    return result;
  }

  result = EsfCodecBase64MutexLock(&file_io_mutex);
  if (result != kEsfCodecBase64ResultSuccess) {
    ESF_CODEC_BASE64_ERR("EsfCodecBase64MutexLock error. ret=%d", result);
    ESF_CODEC_BASE64_ELOG_WARN(ESF_CODEC_BASE64_ELOG_ENCODE_FAILURE);
    ESF_CODEC_BASE64_TRACE("func end");
    return result;
  }

  // Allocate and offset in batches to prevent memory fragmentation
  void* buf = malloc(in_buf_size + enc_buf_size);
  if (buf == NULL) {
    ESF_CODEC_BASE64_ERR("malloc error. errno=%d", errno);
    result = EsfCodecBase64MutexUnlock(&file_io_mutex);
    if (result != kEsfCodecBase64ResultSuccess) {
      ESF_CODEC_BASE64_ERR("EsfCodecBase64MutexUnlock error. ret=%d", result);
    }
    ESF_CODEC_BASE64_ELOG_WARN(ESF_CODEC_BASE64_ELOG_ENCODE_FAILURE);
    ESF_CODEC_BASE64_TRACE("func end");
    return kEsfCodecBase64ResultExternalError;
  }
  char* enc_buf = (char*)((char*)buf + in_buf_size);

  size_t unread_size = in_size;
  size_t read_size = in_size;

  // If read size exceeds buffer size, replace with buffer size
  if (read_size > in_buf_size) {
    read_size = in_buf_size;
  }

  EsfCodecBase64ResultEnum ret_result = kEsfCodecBase64ResultSuccess;
  size_t encoded_size = 0;
  do {
    memset(buf, 0x0, in_buf_size);
    memset(enc_buf, 0x0, enc_buf_size);

    ESF_CODEC_BASE64_DBG("read_size=%zu", read_size);
    size_t rsize = 0;
    ret = EsfMemoryManagerFread(in_handle, buf, read_size, &rsize);
    if (ret != kEsfMemoryManagerResultSuccess) {
      ESF_CODEC_BASE64_ERR("EsfMemoryManagerFread error. ret=%d", ret);
      ESF_CODEC_BASE64_ELOG_WARN(ESF_CODEC_BASE64_ELOG_ENCODE_FAILURE);
      ret_result = kEsfCodecBase64ResultExternalError;
      break;
    }
    ESF_CODEC_BASE64_DBG("rsize=%zu", rsize);
    if (read_size != rsize) {
      ESF_CODEC_BASE64_ERR(
          "Read size mismatch error. write_size=%zu, enc_size=%zu", read_size,
          rsize);
      ESF_CODEC_BASE64_ELOG_WARN(ESF_CODEC_BASE64_ELOG_ENCODE_FAILURE);
      ret_result = kEsfCodecBase64ResultExternalError;
      break;
    }

    size_t enc_size = (size_t)(b64_encode(
        (unsigned char*)buf, (unsigned int)rsize, (unsigned char*)enc_buf));
    if (enc_size == 0) {
      ESF_CODEC_BASE64_ERR("b64_encode error. ret=%zu", enc_size);
      ESF_CODEC_BASE64_ELOG_WARN(ESF_CODEC_BASE64_ELOG_ENCODE_FAILURE);
      ret_result = kEsfCodecBase64ResultExceedsOutBuffer;
      break;
    }
    ESF_CODEC_BASE64_DBG("enc_size=%zu", enc_size);

    // If it is the final encoding, add the length for terminal character
    if (unread_size <= read_size) {
      ++enc_size;
    }

    // Add encoded size
    encoded_size += enc_size;
    ESF_CODEC_BASE64_DBG("encoded_size=%zu", encoded_size);

    size_t write_size = 0;
    ret = EsfMemoryManagerFwrite(out_handle, enc_buf, enc_size, &write_size);
    if (ret != kEsfMemoryManagerResultSuccess) {
      ESF_CODEC_BASE64_ERR("EsfMemoryManagerFwrite error. ret=%d", ret);
      ESF_CODEC_BASE64_ELOG_WARN(ESF_CODEC_BASE64_ELOG_ENCODE_FAILURE);
      ret_result = kEsfCodecBase64ResultExternalError;
      break;
    }
    ESF_CODEC_BASE64_DBG("write_size=%zu", write_size);
    if (write_size != enc_size) {
      ESF_CODEC_BASE64_ERR(
          "Write size mismatch error. write_size=%zu, enc_size=%zu", write_size,
          enc_size);
      ESF_CODEC_BASE64_ELOG_WARN(ESF_CODEC_BASE64_ELOG_ENCODE_FAILURE);
      ret_result = kEsfCodecBase64ResultExternalError;
      break;
    }

    // Next readout size setting
    unread_size -= read_size;
    if (read_size > unread_size) {
      read_size = unread_size;
    }

    ESF_CODEC_BASE64_DBG("unread_size=%zu", unread_size);
  } while (read_size > 0);

  // If encoding process is successful, set out_size
  if (ret_result == kEsfCodecBase64ResultSuccess) {
    *out_size = encoded_size;
    ESF_CODEC_BASE64_DBG("out_size=%zu", *out_size);
  }

  free(buf);

  result = EsfCodecBase64MutexUnlock(&file_io_mutex);
  if (result != kEsfCodecBase64ResultSuccess) {
    ESF_CODEC_BASE64_ERR("EsfCodecBase64MutexUnlock error. ret=%d", result);
    ESF_CODEC_BASE64_ELOG_WARN(ESF_CODEC_BASE64_ELOG_ENCODE_FAILURE);
    ESF_CODEC_BASE64_TRACE("func end");
    ret_result = result;
    return ret_result;
  }

  ESF_CODEC_BASE64_TRACE("func end");
  return ret_result;
}

EsfCodecBase64ResultEnum EsfCodecBase64EncodeHandle(
    EsfMemoryManagerHandle in_handle, size_t in_size,
    EsfMemoryManagerHandle out_handle, size_t* out_size) {
  ESF_CODEC_BASE64_TRACE("func start");
  EsfCodecBase64ResultEnum result = kEsfCodecBase64ResultSuccess;

  // 1. validate parameters
  {
    const uint8_t in_dummy = 0;
    const char out_dummy = 0;
    result = EsfCodecBase64CheckParamForEncode(&in_dummy, in_size, &out_dummy,
                                               out_size);
    if (result != kEsfCodecBase64ResultSuccess) {
      ESF_CODEC_BASE64_ERR("EsfCodecBase64CheckParamForEncode error. ret=%d",
                           result);
      ESF_CODEC_BASE64_ELOG_WARN(ESF_CODEC_BASE64_ELOG_INVALID_PARAM);
      ESF_CODEC_BASE64_TRACE("func end");
      return result;
    }
  }

  // 2. check handle info
  EsfMemoryManagerHandleInfo in_handleinfo = {kEsfMemoryManagerTargetLargeHeap,
                                              0};
  EsfMemoryManagerResult mm_ret =
      EsfMemoryManagerGetHandleInfo((uint32_t)in_handle, &in_handleinfo);
  if (mm_ret != kEsfMemoryManagerResultSuccess) {
    ESF_CODEC_BASE64_ERR("Failed to get handle info for input handle.");
    ESF_CODEC_BASE64_TRACE("func end");
    return kEsfCodecBase64ResultExternalError;
  } else if (in_handleinfo.target_area != kEsfMemoryManagerTargetLargeHeap) {
    ESF_CODEC_BASE64_ERR("Target area is not LargeHeap.");
    ESF_CODEC_BASE64_TRACE("func end");
    return kEsfCodecBase64NotSupported;
  }

  EsfMemoryManagerHandleInfo out_handleinfo = {kEsfMemoryManagerTargetLargeHeap,
                                               0};
  mm_ret = EsfMemoryManagerGetHandleInfo((uint32_t)out_handle, &out_handleinfo);
  if (mm_ret != kEsfMemoryManagerResultSuccess) {
    ESF_CODEC_BASE64_ERR("Failed to get handle info for output handle.");
    ESF_CODEC_BASE64_TRACE("func end");
    return kEsfCodecBase64ResultExternalError;
  } else if (out_handleinfo.target_area != kEsfMemoryManagerTargetLargeHeap) {
    ESF_CODEC_BASE64_ERR("Target area is not LargeHeap.");
    ESF_CODEC_BASE64_TRACE("func end");
    return kEsfCodecBase64NotSupported;
  }

  // 3. check MAP support
  EsfMemoryManagerMapSupport in_support = kEsfMemoryManagerMapIsSupport;
  mm_ret = EsfMemoryManagerIsMapSupport(in_handle, &in_support);
  if (mm_ret != kEsfMemoryManagerResultSuccess) {
    ESF_CODEC_BASE64_ERR("Failed to check map support for in_handle.");
    ESF_CODEC_BASE64_TRACE("func end");
    return kEsfCodecBase64ResultExternalError;
  }

  EsfMemoryManagerMapSupport out_support = kEsfMemoryManagerMapIsSupport;
  mm_ret = EsfMemoryManagerIsMapSupport(out_handle, &out_support);
  if (mm_ret != kEsfMemoryManagerResultSuccess) {
    ESF_CODEC_BASE64_ERR("Failed to check map support for out_handle.");
    ESF_CODEC_BASE64_TRACE("func end");
    return kEsfCodecBase64ResultExternalError;
  }

  // 4. select encoding method
  if ((in_support == kEsfMemoryManagerMapIsSupport) &&
      (out_support == kEsfMemoryManagerMapIsSupport)) {
    ESF_CODEC_BASE64_TRACE(
        "EsfMemoryManagerMap is supported. Call "
        "EsfCodecBase64EncodeHandleMap()");
    result = EsfCodecBase64EncodeHandleMap(in_handle, in_size, out_handle,
                                           out_size);
  } else if ((in_support == kEsfMemoryManagerMapIsNotSupport) &&
             (out_support == kEsfMemoryManagerMapIsNotSupport)) {
    ESF_CODEC_BASE64_TRACE(
        "EsfMemoryManagerMap is not supported. Call "
        "EsfCodecBase64EncodeHandleFileIO()");
    result = EsfCodecBase64EncodeHandleFileIO(in_handle, in_size, out_handle,
                                              out_size);
  } else {
    ESF_CODEC_BASE64_ERR("Invalid EsfMemoryManagerMapSupport");
    ESF_CODEC_BASE64_TRACE("func end");
    return kEsfCodecBase64NotSupported;
  }

  ESF_CODEC_BASE64_TRACE("func end");
  return result;
}
