/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef ESF_CODEC_JSON_JSON_INTERNAL_H_
#define ESF_CODEC_JSON_JSON_INTERNAL_H_

#include <pthread.h>

#include "json.h"
#include "json_fileio.h"
#include "json_handle.h"
#include "memory_manager.h"
#include "parson/lib/parson.h"
#include "utility_log.h"
#include "utility_log_module_id.h"

#ifdef UNIT_TEST_JSON
#define JSON_STATIC
#else
#define JSON_STATIC static
#endif

// JSON log.
#ifdef CONFIG_EXTERNAL_JSON_UTILITY_LOG_ENABLE
#define ESF_JSON_ERR(fmt, ...)                                          \
  (WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:" fmt, __FILE__, __LINE__, \
                    ##__VA_ARGS__))
#define ESF_JSON_WARN(fmt, ...)                                        \
  (WRITE_DLOG_WARN(MODULE_ID_SYSTEM, "%s-%d:" fmt, __FILE__, __LINE__, \
                   ##__VA_ARGS__))
#define ESF_JSON_INFO(fmt, ...)                                        \
  (WRITE_DLOG_INFO(MODULE_ID_SYSTEM, "%s-%d:" fmt, __FILE__, __LINE__, \
                   ##__VA_ARGS__))
#define ESF_JSON_DEBUG(fmt, ...)                                        \
  (WRITE_DLOG_DEBUG(MODULE_ID_SYSTEM, "%s-%d:" fmt, __FILE__, __LINE__, \
                    ##__VA_ARGS__))
#define ESF_JSON_TRACE(fmt, ...)
#else
#define ESF_JSON_PRINT(fmt, ...)                                  \
  (printf("[%s][%s][%d] " fmt "\n", __FILE__, __func__, __LINE__, \
          ##__VA_ARGS__))
#define ESF_JSON_ERR(fmt, ...)                                         \
  (printf("[ERR][%s][%s][%d] " fmt "\n", __FILE__, __func__, __LINE__, \
          ##__VA_ARGS__))
#define ESF_JSON_WARN(fmt, ...)                                        \
  (printf("[WRN][%s][%s][%d] " fmt "\n", __FILE__, __func__, __LINE__, \
          ##__VA_ARGS__))
#define ESF_JSON_INFO(fmt, ...)                                        \
  (printf("[INF][%s][%s][%d] " fmt "\n", __FILE__, __func__, __LINE__, \
          ##__VA_ARGS__))
#define ESF_JSON_DEBUG(fmt, ...)                                       \
  (printf("[DBG][%s][%s][%d] " fmt "\n", __FILE__, __func__, __LINE__, \
          ##__VA_ARGS__))
#define ESF_JSON_TRACE(fmt, ...)
#endif

// Used for string replacement.
#define REPLACEMENT_STRING_MAX_SIZE 29
#define REPLACEMENT_STRING "JSON_PLACEHOLDER_"
#define JSON_VALUE_ID_STRING_MAX_SIZE 5

typedef struct EsfJsonValueData EsfJsonValueData;

// The structure defines an JSON data
struct EsfJsonValueData {
  // ID associated with JSON Value.
  EsfJsonValue id;
  // JSON Value
  JSON_Value* data;
};

typedef struct EsfJsonMemoryInfo EsfJsonMemoryInfo;
// Information used in Memory Manager FileIO
struct EsfJsonMemoryInfo {
  EsfJsonValue id;              // JSon Value ID
  EsfMemoryManagerHandle data;  // Memory Manager handle
  size_t size;                  // mem size
  off_t offset;                 // seek position
};

typedef struct EsfJsonValueContainer EsfJsonValueContainer;
// The structure that manages JSON data
struct EsfJsonValueContainer {
  // JSON Data
  EsfJsonValueData* data;
  // The maximum number of elements in the array.
  int32_t capacity;
  // The number of array elements.
  int32_t size;
  // Last JSON Value ID.
  EsfJsonValue last_id;
  // memory manager info.
  EsfJsonMemoryInfo mem_info[CONFIG_EXTERNAL_CODEC_JSON_MEM_HANDLE_MAX];
};

// Operating handle for the JSON module.
struct EsfJsonHandleImpl {
  // JSON data array.
  EsfJsonValueContainer* json_value;
  // Serialization string.
  char* serialized_str;
};

// internal func
// """Search for JSON value ID matches.

// Searches for JSON value ID matches and returns the result JSON value.

// Args:
//    json_value (EsfJsonValueContainer*): JSON value array info.
//    value (EsfJsonValue): JSON Value ID.
//    data (JSON_Value**): JSON Value.
//      information. NULL is not acceptable.

// Returns:
//    kEsfJsonSuccess: Normal termination.
//    kEsfJsonInvalidArgument: Arg parameter error.
//    kEsfJsonValueNotFound: JSON Value not found.
EsfJsonErrorCode EsfJsonValueFind(EsfJsonValueContainer* json_value,
                                  EsfJsonValue value, JSON_Value** data);

// """Search for JSON value matches.

// Searches for JSON value matches and returns the result JSON value ID.

// Args:
//    json_value (EsfJsonValueContainer*): JSON value array info.
//    data (const JSON_Value*): JSON Value.
//      information. NULL is not acceptable.
//    value (EsfJsonValue*): JSON Value ID.
//      information. NULL is not acceptable.

// Returns:
//    kEsfJsonSuccess: Normal termination.
//    kEsfJsonInvalidArgument: Arg parameter error.
//    kEsfJsonValueNotFound: JSON Value not found.
EsfJsonErrorCode EsfJsonValueLookup(EsfJsonValueContainer* json_value,
                                    const JSON_Value* data,
                                    EsfJsonValue* value);

// """Delete JSON value matches.

// Args:
//    json_value (EsfJsonValueContainer*): JSON value array info.
//    data (const JSON_Value*): JSON Value.
//      information. NULL is not acceptable.

// Returns:
//    kEsfJsonSuccess: Normal termination.
//    kEsfJsonInvalidArgument: Arg parameter error.
EsfJsonErrorCode EsfJsonValueLookupRemove(EsfJsonValueContainer* json_value,
                                          const JSON_Value* data);

// """Specify JSON value and add.

// JSON value is specified and added, a new JSON value ID is issued and the
// result is returned.

// Args:
//    json_value (EsfJsonValueContainer*): JSON value array info.
//    data (const JSON_Value*): JSON Value.
//      information. NULL is not acceptable.
//    value (EsfJsonValue*): JSON Value ID.
//      information. NULL is not acceptable.

// Returns:
//    kEsfJsonSuccess: Normal termination.
//    kEsfJsonInvalidArgument: Arg parameter error.
//    kEsfJsonValueLimit: JSON Value ID excess error.
//    kEsfJsonOutOfMemory: Memory allocation failure.
EsfJsonErrorCode EsfJsonValueAdd(EsfJsonValueContainer* json_value,
                                 JSON_Value* data, EsfJsonValue* value);

// """Replace JSON value with JSON value ID.

// Args:
//    json_value (EsfJsonValueContainer*): JSON value array info.
//    value (EsfJsonValue): JSON Value ID.
//    data (const JSON_Value*): JSON Value.
//      information. NULL is not acceptable.

// Returns:
//    kEsfJsonSuccess: Normal termination.
//    kEsfJsonInvalidArgument: Arg parameter error.
//    kEsfJsonValueNotFound: JSON Value not found.
EsfJsonErrorCode EsfJsonValueReplace(EsfJsonValueContainer* json_value,
                                     EsfJsonValue value, JSON_Value* data);

// """Allocate and initialize memory for EsfJsonValueContainer.

// Args:
//    json_value (EsfJsonValueContainer*): JSON value array info.
//      information. NULL is not acceptable.

// Returns:
//    kEsfJsonSuccess: Normal termination.
//    kEsfJsonInvalidArgument: Arg parameter error.
//    kEsfJsonOutOfMemory: Memory allocation failure.
EsfJsonErrorCode EsfJsonValueContainerInit(EsfJsonValueContainer** json_value);

// """Release EsfJsonValueContainer.

// Args:
//    json_value (EsfJsonValueContainer*): JSON value array info.
//      information. NULL is not acceptable.

// Returns:
//    kEsfJsonSuccess: Normal termination.
//    kEsfJsonInvalidArgument: Arg parameter error.
//    kEsfJsonInternalError:  Internal error.
EsfJsonErrorCode EsfJsonValueContainerFree(EsfJsonValueContainer* json_value);

// """Convert JSON_Value_Type to EsfJsonValueType.

// Args:
//    json_type (JSON_Value_Type): Parson JSON value type.

// Returns:
//    kEsfJsonValueTypeError: Arg parameter error.
//    other: Any EsfJsonValueType.
EsfJsonValueType EsfJsonValueTypeConvert(JSON_Value_Type json_type);

// """Delete JSON value recursively.

// Args:
//    json_value (EsfJsonValueContainer*): JSON value array info.
//    data (const JSON_Value*): JSON Value.
//      information. NULL is not acceptable.

// Returns:
//    kEsfJsonSuccess: Normal termination.
//    kEsfJsonInvalidArgument: Arg parameter error.
EsfJsonErrorCode EsfJsonValueRecursiveRemove(EsfJsonValueContainer* json_value,
                                             JSON_Value* data);

// """Set the specified JSON value to JSON object or JSON array.

// Args:
//    data (JSON_Value*): JSON value.
//      information. NULL is not acceptable.
//    set data (const JSON_Value*): JSON value to set.
//      information. NULL is not acceptable.

// Returns:
//    kEsfJsonSuccess: Normal termination.
//    kEsfJsonInvalidArgument: Arg parameter error.
//    kEsfJsonInternalError:  Internal error.
EsfJsonErrorCode EsfJsonValueDataSet(JSON_Value* data, JSON_Value* set_data);

// """Get JSON Value that has no parent.

// Args:
//    json_value (EsfJsonValueContainer*): JSON value array info.
//      information. NULL is not acceptable.
//    value (EsfJsonValue): JSON Value ID.
//    data (const JSON_Value**): JSON Value.
//      information. NULL is not acceptable.

// Returns:
//    kEsfJsonSuccess: Normal termination.
//    kEsfJsonInvalidArgument: Arg parameter error.
//    kEsfJsonParentAlreadyExists: Parent already exists error.
EsfJsonErrorCode EsfJsonValueNotHaveParentGet(EsfJsonValueContainer* json_value,
                                              EsfJsonValue value,
                                              JSON_Value** data);

// """Add an unmanaged JSON Value.

// Args:
//    json_value (EsfJsonValueContainer*): JSON value array info.
//      information. NULL is not acceptable.
//    data (const JSON_Value**): JSON Value.
//      information. NULL is not acceptable.
//    value (EsfJsonValue*): JSON Value ID.
//      information. NULL is not acceptable.

// Returns:
//    kEsfJsonSuccess: Normal termination.
//    kEsfJsonInvalidArgument: Arg parameter error.
//    kEsfJsonValueLimit: JSON Value ID excess error.
//    kEsfJsonOutOfMemory: Memory allocation failure.
//    kEsfJsonValueNotFound: JSON Value not found.
EsfJsonErrorCode EsfJsonValueNotManagedAdd(EsfJsonValueContainer* json_value,
                                           JSON_Value* data,
                                           EsfJsonValue* value);

// """Can Parent JSON_Value be added to the parent of JSON_Value

// Args:
//    json_value (EsfJsonValueContainer*): JSON value array info.
//      information. NULL is not acceptable.
//    data (const JSON_Value**): JSON Value.
//      information. NULL is not acceptable.
//    parent_data (const JSON_Value**): Parent JSON Value.
//      information. NULL is not acceptable.

// Returns:
//    true: possible
//    false: impossible
bool EsfJsonCanJsonAddToJson(EsfJsonValueContainer* json_value,
                             JSON_Value* data, JSON_Value* parent_data);

// """Get the number of characters in the substitution string when the
// substitution string is converted to the replaced string.

// Args:
//    json_value (EsfJsonValueContainer*): JSON value array info.
//      information. NULL is not acceptable.
//    data (JSON_Value**): JSON Value.
//      information. NULL is not acceptable.
//    str_size (size_t*): string size.
//      information. NULL is not acceptable.

// Returns:
//    kEsfJsonSuccess: Normal termination.
//    kEsfJsonInvalidArgument: Arg parameter error.
//    kEsfJsonInternalError: Internal error.
EsfJsonErrorCode EsfJsonConvertedStringSizeGet(
    EsfJsonValueContainer* json_value, JSON_Value* data, size_t* str_size);

// """Determines if there is a String Value associated with the Memory Manager
// handle that supports File I/O.

// Args:
//    json_value (EsfJsonValueContainer*): JSON value array info.
//      information. NULL is not acceptable.
//    data (JSON_Value**): JSON Value.
//      information. NULL is not acceptable.
//    is_included (bool*): Judgment Result.
//      information. NULL is not acceptable.

// Returns:
//    kEsfJsonSuccess: Normal termination.
//    kEsfJsonInvalidArgument: Arg parameter error.
//    kEsfJsonInternalError: Internal error.
EsfJsonErrorCode EsfJsonCheckStringReplace(EsfJsonValueContainer* json_value,
                                           JSON_Value* data, bool* is_included);

// """Serializes a JSON value using a memory manager and writes it to a file.
//
// This function takes a JSON value container and serializes it into a string,
// utilizing a memory manager for handling the memory operations. The serialized
// string is then written to a file. The function can optionally include
// additional data during serialization based on the `is_included` flag.
//
// Args:
// json_value A pointer to the EsfJsonValueContainer that holds the JSON data to
//            be serialized. Must not be NULL.
// mem_handle A handle to the memory manager used for managing memory
//            operations.
// mem_handle_open memory manager handle open request (true:open request)
// serialized_str A pointer to a character array where the serialized JSON
//                string will be stored. Must not be NULL.
// is_included A boolean flag indicating whether additional data should be
//             included in the serialization.
// erialized_size A pointer to a size_t variable where the size of the
//                serialized data will be stored. Must not be NULL.
//
// Returns:
// Returns an EsfJsonErrorCode indicating the success or failure of the
// operation.
//  - kEsfJsonSuccess: Serialization and writing were successful.
//  - kEsfJsonInvalidArgument: One or more input parameters are NULL.
//  - kEsfJsonInternalError: An internal error occurred during serialization or
//  writing.
//
// Notes:
// """
EsfJsonErrorCode EsfJsonSerializeUsingMemoryManagerFileIO(
    EsfJsonValueContainer* json_value, EsfMemoryManagerHandle mem_handle,
    bool mem_handle_open, char* serialized_str, bool is_included,
    size_t* serialized_size);

// """Serializes a JSON value using a memory manager and memory map.
//
// This function serializes a given JSON value into a string format, utilizing
// a specified memory manager. The serialized string is stored in the provided
// memory data buffer. If the `is_included` flag is set to true, the function
// will perform additional conversion steps using the memory map.
//
// Args:
// json_value Pointer to the JSON value container that needs to be serialized.
// mem_handle Handle to the memory manager used for managing memory allocations.
// mem_data Pointer to the memory buffer where the serialized string will be
//          stored.
// serialized_str Pointer to a character array where the serialized JSON
//                string is initially stored.
// is_included Boolean flag indicating whether additional conversion steps
//             should be included.
// serialized_size Pointer to a size_t variable where the size of the serialized
//                 string will be stored.
//
// Returns:
// Returns an EsfJsonErrorCode indicating the success or failure of the
// serialization process.
//  - kEsfJsonSuccess: Serialization was successful.
//  - kEsfJsonInvalidArgument: One or more input parameters are NULL.
//  - kEsfJsonInternalError: An internal error occurred during serialization.
//
// Notes:
// """
EsfJsonErrorCode EsfJsonSerializeUsingMemoryManagerMemMap(
    EsfJsonValueContainer* json_value, EsfMemoryManagerHandle mem_handle,
    void* mem_data, char* serialized_str, bool is_included,
    size_t* serialized_size);

// """Starts exclusive control.

// Args:
//    mutex (pthread_mutex_t*): Exclusive control identifier.
//      information. NULL is not acceptable.

// Returns:
//    kEsfJsonSuccess: Normal termination.
//    kEsfJsonInvalidArgument: Arg parameter error.
//    kEsfJsonInternalError: Internal error.
EsfJsonErrorCode EsfJsonMutexLock(pthread_mutex_t* mutex);

// """Ends exclusive control.

// Args:
//    mutex (pthread_mutex_t*): Exclusive control identifier.
//      information. NULL is not acceptable.

// Returns:
//    kEsfJsonSuccess: Normal termination.
//    kEsfJsonInvalidArgument: Arg parameter error.
//    kEsfJsonInternalError: Internal error.
EsfJsonErrorCode EsfJsonMutexUnlock(pthread_mutex_t* mutex);

// """Sets Json Value ID to the specified Json Value.

// Args:
//    json_value (EsfJsonValueContainer*): JSON value array info.
//      information. NULL is not acceptable.
//    value (EsfJsonValue): JSON value ID.
//    data (JSON_Value*): JSON Value.
//      information. NULL is not acceptable.

// Returns:
// EsfJsonErrorCode indicating the success or failure of the operation:
// - kEsfJsonSuccess: The operation was successful.
// - kEsfJsonInvalidArgument: One or more arguments are invalid (e.g.,
//   null pointers).
// - kEsfJsonInternalError: An internal error occurred during processing.
// - kEsfJsonOutOfMemory: Memory allocation failed while creating a new
//   JSON string.
EsfJsonErrorCode EsfJsonStringSetHandleInternal(
    EsfJsonValueContainer* json_value, EsfJsonValue value, JSON_Value* data);

// """Converts a JSON Value to a string and writes it to a File I/O compatible
// Memory Manager handle. (for FileIO)
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
//                                      converted string
// serialized_size (size_t*): Number of characters after conversion
//                     (excluding NULL terminator)
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
EsfJsonErrorCode EsfJsonSerializeHandleFileIO(EsfJsonHandle handle,
                                              EsfJsonValue value,
                                              EsfMemoryManagerHandle mem_handle,
                                              size_t* serialized_size);

// """Converts a JSON Value to a string and writes it to a File I/O compatible
// Memory Manager handle. (for MemMap)
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
//                                      converted string
// serialized_size (size_t*): Number of characters after conversion
//                     (excluding NULL terminator)
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
EsfJsonErrorCode EsfJsonSerializeHandleMemMap(EsfJsonHandle handle,
                                              EsfJsonValue value,
                                              EsfMemoryManagerHandle mem_handle,
                                              size_t* serialized_size);

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
// to set
// value (EsfJsonValue*): Pointer to store the created String Value
//
// Returns:
// EsfJsonErrorCode indicating the success or failure of the operation:
// - kEsfJsonSuccess: Initialization was successful.
// - kEsfJsonHandleError: The provided handle is NULL or uninitialized.
// - kEsfJsonInvalidArgument: The provided value pointer is NULL.
// - kEsfJsonValueLimit: No free space available in the File I/O retention area.
// - kEsfJsonOutOfMemory: Memory allocation failed during JSON value
// initialization.
// - kEsfJsonInternalError: An internal error occurred during processing.
//
// Notes:
// - Pass mem_handle in a state already opened with EsfMemoryManagerFopen.
// - Set the seek position to the beginning of the data you want to write, and
// specify the size of the data you want to write with the mem_size argument.
// - The generated String Value is set with an alternative string.
// EsfJsonStringGet will return this alternative string.
// - The data in mem_handle replaces the alternative string when
// EsfJsonSerializeHandle is called.
// - The data in mem_handle is not escaped when embedded in the JSON string.
// Ensure the data in mem_handle is already properly escaped.
// - It's possible to set the same mem_handle for different JSON Values as it
// seeks to the initial position before processing during stringification.
// """
EsfJsonErrorCode EsfJsonStringInitHandleProcess(
    EsfJsonHandle handle, EsfMemoryManagerHandle mem_handle, size_t mem_size,
    EsfJsonValue* value);

// """Sets a String Value linked to a File I/O compatible Memory Manager handle.
//
// This function sets the string from mem_handle to the JSON Value specified by
// value. The handle must be obtained with EsfJsonOpen before calling this API.
//
// Args:
// handle (EsfJsonHandle): Handle for the JSON API
// value (EsfJsonValue): JSON Value to be set
// mem_handle (EsfMemoryManagerHandle): File I/O compatible Memory Manager
//                                      handle containing the string to set
// mem_size (size_t): Number of characters to set
//
// Returns:
// EsfJsonErrorCode indicating the success or failure of the operation:
// - kEsfJsonSuccess: The operation was successful.
// - kEsfJsonHandleError: The provided handle is invalid.
// - kEsfJsonValueLimit: No free space is available in the File I/O
//   retention area.
// - kEsfJsonInternalError: An internal error occurred during processing.
//
// Notes:
// - Pass mem_handle in a state already opened with EsfMemoryManagerFopen.
// - Set the seek position to the beginning of the data you want to write, and
// specify the size of the data you want to write with the mem_size argument.
// - The set String Value is given an alternative string. EsfJsonStringGet will
// return this alternative string.
// - The data in mem_handle replaces the alternative string when
// EsfJsonSerializeHandle is called.
// - The data in mem_handle is not escaped when embedded in the JSON string.
// Ensure the data in mem_handle is already properly escaped.
// - It's possible to set the same mem_handle for different JSON Values as it
// seeks to the initial position before processing during stringification.
// """
EsfJsonErrorCode EsfJsonStringSetHandleProcess(
    EsfJsonHandle handle, EsfJsonValue value, EsfMemoryManagerHandle mem_handle,
    size_t mem_size);
#endif  // ESF_CODEC_JSON_JSON_INTERNAL_H_
