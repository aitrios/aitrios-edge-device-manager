/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef ESF_CODEC_JSON_JSON_FILEIO_H_
#define ESF_CODEC_JSON_JSON_FILEIO_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "json.h"
#include "memory_manager.h"

// """Gets the number of characters when converting a JSON Value to a string.
//
// Returns the number of characters when the target value is stringified.
// The handle must be obtained with EsfJsonOpen before calling this API.
// If the target JSON Value and its child JSON Values include String Values
// created with EsfJsonStringInitHandle or EsfJsonStringSetHandle, it returns
// the size including the data in the Memory Manager handle.
//
// Args:
// handle (EsfJsonHandle): Handle for the JSON API
// value (EsfJsonValue): Target JSON Value
//
// Returns:
// size_t: Number of characters excluding the NULL terminator. Returns 0 on
// error.
//
// Notes:
// """
size_t EsfJsonSerializeSizeGet(EsfJsonHandle handle, EsfJsonValue value);

// """Converts a JSON Value to a string and writes it to a File I/O compatible
// Memory Manager handle.
//
// Stringifies the target value for serialization.
// The handle must be obtained with EsfJsonOpen before calling this API.
// The stringification result is returned in mem_handle.
// If the target JSON Value does not exist, this API returns
// kEsfJsonValueNotFound. In this case, mem_handle remains unchanged.
//
// Args:
// handle (EsfJsonHandle): Handle for the JSON API
// value (EsfJsonValue): Target JSON Value
// mem_handle (EsfMemoryManagerHandle): Memory Manager handle to store the
// converted string mem_size (size_t*): Number of characters after conversion
// (excluding NULL terminator)
//
// Returns:
// enum EsfJsonErrorCode: Error code indicating the processing result
//
// Notes:
// - Does not affect strings managed by EsfJsonSerialize and
// EsfJsonSerializeFree functions.
// - Pass mem_handle in a state already opened with EsfMemoryManagerFopen.
// - Set the seek position to the beginning of where you want to write the data.
// - After executing this API, the seek position will be at the end of the
// written JSON string.
// - Use a different mem_handle from other Memory Manager handles.
// - This API can be called multiple times but cannot be executed
// simultaneously.
// """
EsfJsonErrorCode EsfJsonSerializeFileIO(EsfJsonHandle handle,
                                        EsfJsonValue value,
                                        EsfMemoryManagerHandle mem_handle,
                                        size_t* mem_size);

// """Determines if the JSON Value includes any String Values linked to File I/O
// compatible Memory Manager handles when converting to a string.
//
// This function determines if the JSON Value includes any String Values created
// with EsfJsonStringInitFileIO or EsfJsonStringSetFileIO when converting to a
// string. The handle must be obtained with EsfJsonOpen before calling this API.
// The determination result is stored in is_included. If true, it includes
// String Values created with EsfJsonStringInitFileIO or EsfJsonStringSetFileIO.
// If the target JSON Value does not exist, this API returns
// kEsfJsonValueNotFound. In this case, is_included remains unchanged.
//
// Args:
// handle (EsfJsonHandle): Handle for the JSON API
// value (EsfJsonValue): Target JSON Value
// is_included (bool*): Pointer to store the determination result
//
// Returns:
// - kEsfJsonHandleError: If handle is invalid, obtain a handle and retry.
// - kEsfJsonInvalidArgument: If is_included is NULL, check arguments and retry.
// - kEsfJsonValueNotFound: If JSON Value doesn't exist, check arguments and
// retry.
// """
EsfJsonErrorCode EsfJsonSerializeIncludesFileIO(EsfJsonHandle handle,
                                                EsfJsonValue value,
                                                bool* is_included);

// """Creates a String Value linked to a File I/O compatible Memory Manager
// handle.
//
// This function generates a String Value with the string from mem_handle and
// returns it in value. The handle must be obtained with EsfJsonOpen before
// calling this API.
//
// Args:
// handle (EsfJsonHandle): Handle for the JSON API
// mem_handle (EsfMemoryManagerHandle): File I/O compatible Memory Manager
// handle containing the string to set mem_size (size_t): Number of characters
// to set value (EsfJsonValue*): Pointer to store the created String Value
//
// Returns:
// - kEsfJsonHandleError: If handle is invalid, obtain a handle and retry.
// - kEsfJsonInvalidArgument: If mem_handle is NULL or not File I/O compatible,
// or if value is NULL, check arguments and retry.
// - kEsfJsonOutOfMemory: If memory allocation fails, check available memory on
// the user side.
// - kEsfJsonValueLimit: If JSON Value limit for the handle is reached,
// re-obtain the handle and retry.
//
// Notes:
// - Pass mem_handle in a state already opened with EsfMemoryManagerFopen.
// - Set the seek position to the beginning of the data you want to write, and
// specify the size of the data you want to write with the mem_size argument.
// - The generated String Value is set with an alternative string.
// EsfJsonStringGet will return this alternative string.
// - The data in mem_handle replaces the alternative string when
// EsfJsonSerializeFileIO is called.
// - The data in mem_handle is not escaped when embedded in the JSON string.
// Ensure the data in mem_handle is already properly escaped.
// - It's possible to set the same mem_handle for different JSON Values as it
// seeks to the initial position before processing during stringification.
// """
EsfJsonErrorCode EsfJsonStringInitFileIO(EsfJsonHandle handle,
                                         EsfMemoryManagerHandle mem_handle,
                                         size_t mem_size, EsfJsonValue* value);

// """Sets a String Value linked to a File I/O compatible Memory Manager handle.
//
// This function sets the string from mem_handle to the JSON Value specified by
// value. The handle must be obtained with EsfJsonOpen before calling this API.
//
// Args:
// handle (EsfJsonHandle): Handle for the JSON API
// value (EsfJsonValue): JSON Value to be set
// mem_handle (EsfMemoryManagerHandle): File I/O compatible Memory Manager
// handle containing the string to set mem_size (size_t): Number of characters
// to set
//
// Returns:
// - kEsfJsonHandleError: If handle is invalid, obtain a handle and retry.
// - kEsfJsonInvalidArgument: If mem_handle is NULL or not File I/O compatible,
// or if value is NULL, check arguments and retry.
// - kEsfJsonOutOfMemory: If memory allocation fails, check available memory on
// the user side.
// - kEsfJsonValueNotFound: If the specified JSON Value is not found, check
// arguments and retry.
// - kEsfJsonInternalError: Internal error occurred, please contact support.
//
// Notes:
// - Pass mem_handle in a state already opened with EsfMemoryManagerFopen.
// - Set the seek position to the beginning of the data you want to write, and
// specify the size of the data you want to write with the mem_size argument.
// - The set String Value is given an alternative string. EsfJsonStringGet will
// return this alternative string.
// - The data in mem_handle replaces the alternative string when
// EsfJsonSerializeFileIO is called.
// - The data in mem_handle is not escaped when embedded in the JSON string.
// Ensure the data in mem_handle is already properly escaped.
// - It's possible to set the same mem_handle for different JSON Values as it
// seeks to the initial position before processing during stringification.
// """
EsfJsonErrorCode EsfJsonStringSetFileIO(EsfJsonHandle handle,
                                        EsfJsonValue value,
                                        EsfMemoryManagerHandle mem_handle,
                                        size_t mem_size);

#ifdef __cplusplus
}
#endif
#endif  // ESF_CODEC_JSON_JSON_FILEIO_H_
