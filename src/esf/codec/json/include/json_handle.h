/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef ESF_CODEC_JSON_JSON_HANDLE_H_
#define ESF_CODEC_JSON_JSON_HANDLE_H_
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

// """Serializes a JSON handle into a specified memory area.
//
// This function attempts to serialize the given JSON handle and value into a
// memory region managed by the provided memory manager handle. It first
// validates the input parameters, checks the memory handle information, and
// determines if memory mapping is supported. Based on the support for memory
// mapping, it either uses memory mapping or file I/O methods to perform the
// serialization.
//
// Args:
// handle The JSON handle to be serialized. Must not be NULL.
// value The JSON value associated with the handle.
// mem_handle The memory manager handle that specifies where the serialized data
//            should be stored. Must be valid and target the LargeHeap area.
// serialized_size A pointer to a size_t variable where the size of the
//                 serialized data will be stored. Must not be NULL.
//
// Returns:
// Returns an EsfJsonErrorCode indicating the result of the operation:
//  - kEsfJsonInternalError: An internal error occurred during serialization.
//      The provided handle was invalid or not properly initialized.
//  - kEsfJsonInvalidArgument: One or more arguments were invalid.
//  - Other specific error codes based on the serialization method used.
//
// Notes:
// """
EsfJsonErrorCode EsfJsonSerializeHandle(EsfJsonHandle handle,
                                        EsfJsonValue value,
                                        EsfMemoryManagerHandle mem_handle,
                                        size_t* serialized_size);

// """Determines if the JSON Value includes any String Values linked to File I/O
// compatible Memory Manager handles when converting to a string.
//
// This function determines if the JSON Value includes any String Values created
// with EsfJsonStringInitHandle or EsfJsonStringSetHandle when converting to a
// string. The handle must be obtained with EsfJsonOpen before calling this API.
// The determination result is stored in is_included. If true, it includes
// String Values created with EsfJsonStringInitHandle or EsfJsonStringSetHandle.
// If the target JSON Value does not exist, this API returns
// kEsfJsonValueNotFound. In this case, is_included remains unchanged.
//
// Args:
// handle (EsfJsonHandle): Handle for the JSON API
// value (EsfJsonValue): Target JSON Value
// is_included (bool*): Pointer to store the determination result
//
// Returns:
// EsfJsonErrorCode indicating the result of the operation:
// - kEsfJsonSuccess: The operation was successful, and the value was serialized
// and checked.
// - kEsfJsonHandleError: The provided handle is invalid or uninitialized.
// - kEsfJsonInvalidArgument: The is_included parameter is NULL.
// - kEsfJsonInternalError: An internal error occurred during processing.
// - Other error codes may be returned by EsfJsonValueFind or
// EsfJsonCheckStringReplace functions.
// """
EsfJsonErrorCode EsfJsonSerializeIncludesHandle(EsfJsonHandle handle,
                                                EsfJsonValue value,
                                                bool* is_included);

// """Initializes a JSON string handle with the specified memory manager and
// size.
//
// This function sets up a JSON string handle using the provided memory manager
// handle and memory size. It performs several validation checks on the input
// parameters and ensures that the memory manager is configured correctly for
// large heap operations. The function also determines if the memory manager
// supports mapping and calls the appropriate processing function based on this
// capability.
//
// Args:
// handle The JSON handle to be initialized. Must not be equal to
//        ESF_JSON_HANDLE_INITIALIZER.
// mem_handle The memory manager handle used for allocating memory.
//            Must be valid and support large heap.
// mem_size The size of the memory to allocate. Must be greater than zero.
// value A pointer to an EsfJsonValue structure where the result will be stored.
//       Must not be NULL.
//
// Returns:
// Returns an EsfJsonErrorCode indicating the success or failure of the
// operation:
//  - kEsfJsonInternalError: An internal error occurred during initialization.
//      The provided handle or memory manager handle was invalid.
//  - kEsfJsonInvalidArgument: One or more arguments were invalid.
//  - Other codes as returned by EsfJsonStringInitHandleProcess().
//
// Notes:
// """
EsfJsonErrorCode EsfJsonStringInitHandle(EsfJsonHandle handle,
                                         EsfMemoryManagerHandle mem_handle,
                                         size_t mem_size, EsfJsonValue* value);

// """Sets a JSON string handle with the specified parameters.
//
// This function is responsible for setting a JSON string handle using the
// provided memory manager handle and size. It performs several checks to ensure
// that the parameters are valid and that the memory manager supports the
// required operations.
//
// Args:
// handle The JSON handle to be set. Must not be equal to
// ESF_JSON_HANDLE_INITIALIZER. value The JSON value to be associated with the
// handle. mem_handle The memory manager handle used for managing memory
// operations. mem_size The size of the memory to be allocated. Must be greater
// than zero.
//
// Returns:
// Returns an EsfJsonErrorCode indicating the result of the operation:
//  - kEsfJsonInternalError: An internal error occurred during processing.
//      The provided handle or memory manager handle was invalid.
//  - kEsfJsonInvalidArgument: The provided memory size was zero.
//  - Other values may be returned by EsfJsonStringSetHandleProcess() based on
//  its execution.
//
// Notes:
// """
EsfJsonErrorCode EsfJsonStringSetHandle(EsfJsonHandle handle,
                                        EsfJsonValue value,
                                        EsfMemoryManagerHandle mem_handle,
                                        size_t mem_size);

#ifdef __cplusplus
}
#endif
#endif  // ESF_CODEC_JSON_JSON_HANDLE_H_
