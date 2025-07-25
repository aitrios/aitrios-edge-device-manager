/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESF_MEMORY_MANAGER_INCLUDE_MEMORY_MANAGER_H_
#define ESF_MEMORY_MANAGER_INCLUDE_MEMORY_MANAGER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>  // for free
#include <string.h>  // for memcpy
#include <sys/types.h>
#include <wasm_export.h>  // for wasm_exec_env_t, wasm_runtim...

// This code defines an enumeration type for the result of executing an API.
typedef enum {
  kEsfMemoryManagerResultSuccess = 0,
  kEsfMemoryManagerResultParamError,
  kEsfMemoryManagerResultAllocationError,
  kEsfMemoryManagerResultMapError,
  kEsfMemoryManagerResultFileIoError,
  kEsfMemoryManagerResultNotSupport,
  kEsfMemoryManagerResultOperationError,
  kEsfMemoryManagerResultOtherError
} EsfMemoryManagerResult;

// This code defines an enumeration for specifying regions when allocating
// memory.
typedef enum {
  kEsfMemoryManagerTargetLargeHeap = 0,
  kEsfMemoryManagerTargetDma,
  kEsfMemoryManagerTargetWasmHeap,
  kEsfMemoryManagerTargetOtherHeap
} EsfMemoryManagerTargetArea;

// This code defines an enumeration for specifying whether or not Map features
// are supported.
typedef enum {
  kEsfMemoryManagerMapIsSupport = 0,
  kEsfMemoryManagerMapIsNotSupport
} EsfMemoryManagerMapSupport;

// This code defines an enumeration for specifying memory usage when allocating
// Wasm memory.
typedef enum {
  kEsfMemoryManagerWasmAllocForRuntime = 0,
  kEsfMemoryManagerWasmAllocForLinearMemory
} EsfMemoryManagerWasmMemoryUsage;

// Definition of memory operation handles for LargeHeap/DMA/WasmHeap area.
typedef uint32_t EsfMemoryManagerHandle;
// EsfMemoryManagerHandle details
//  |<---- 7bit ----->|<---------------------- 25bit ---------------------->|
//  +-----------------+-----------------+-----------------+-----------------+
//  |    handle_id    |                   address_offset                    |
//  +-----------------+-----------------+-----------------+-----------------+
//  handle_id      : 0,1-127
//  address_offset : 0x0000000-0x1FFFFFF(32MB)

// macro for getting handle_id.
#define MEMORY_MANAGER_HANDLE_ID_MASK(handle) (uint32_t)(handle & 0xfe000000)
#define MEMORY_MANAGER_HANDLE_ID(handle) \
  (uint8_t)((handle >> 25) & (0x0000007f))
// macro for getting address_offset.
#define MEMORY_MANAGER_OFFSET_MASK (uint32_t)(0x01ffffff)
#define MEMORY_MANAGER_HANDLE_OFFSET(handle) \
  (uint32_t)(handle & MEMORY_MANAGER_OFFSET_MASK)

// Definition of memory pointer to the AppMemory area.
typedef void *EsfMemoryManagerAppMemory;

// Definition of memory operation handle information (area, size)
typedef struct {
  EsfMemoryManagerTargetArea target_area;  // target memory area
  int32_t allocate_size;                   // allocated memory size
} EsfMemoryManagerHandleInfo;

// """ Memory allocation.
// This function is used to allocate memory in the specified target area
// (WasmHeap, LargeHeap or DMA) of the system. The allocated memory can be
// identified by a handle which will be returned after successful allocation.

// Args:
//    target_area: An enum value specifying the target area for memory
//      allocation. Can be one of kEsfMemoryManagerTargetWasmHeap,
//      kEsfMemoryManagerTargetLargeHeap or kEsfMemoryManagerTargetDMA.
//    exec_env: A pointer to a wasm execution environment structure
//      (wasm_exec_env_t). This is required for WasmHeap allocation and should
//      be NULL for other target areas.
//    size: The requested memory size in bytes. Must be greater than zero.
//    handle: A pointer to an EsfMemoryManagerHandle variable where the
//      allocated memory's handle will be stored upon successful allocation.

// Returns:
//    EsfMemoryManagerResult:
//    - kEsfMemoryManagerResultSuccess: Memory was successfully allocated and
//    its handle is stored in 'handle'.
//    - kEsfMemoryManagerResultParamError: Invalid parameters were passed (e.g.,
//    NULL pointer for 'handle' or non-positive 'size').
//    - kEsfMemoryManagerResultOtherError: An internal error occurred, such as a
//    mutex locking failure.
//    - Other EsfMemoryManagerResult values may be returned by the called
//    subfunctions (e.g., EsfMemoryManagerAllocateWasm or
//    EsfMemoryManagerGenerateMemoryInfo).

// Note:
// """
EsfMemoryManagerResult EsfMemoryManagerAllocate(
    EsfMemoryManagerTargetArea target_area, const wasm_exec_env_t *exec_env,
    int32_t size, EsfMemoryManagerHandle *handle);

// """ Free memory.
// This function is used to free memory allocated by the EsfMemoryManager. It
// takes two parameters, a handle and an execution environment. The function
// first checks if the provided handle is valid or not. If it's invalid, it
// returns kEsfMemoryManagerResultParamError. Then, it checks if the memory
// manager has been initialized or not. If not, it returns
// kEsfMemoryManagerResultOtherError. After that, it locks a mutex to ensure
// thread safety and unlocks it at the end of the function. The function then
// discards WasmHeap or LargeHeap/DMA memory based on the handle provided. If an
// error occurs during this process, it returns
// kEsfMemoryManagerResultOtherError. Finally, if no errors occur, it deletes
// the used handle ID and returns kEsfMemoryManagerResultSuccess.

// Args:
//    handle: A handle to the memory that needs to be freed. It is an input
//      parameter.
//    exec_env: The execution environment of WASM. It is an input parameter.

// Returns:
//    EsfMemoryManagerResult:
//      An enum value indicating whether the operation was successful or not.

// Raises:
//    kEsfMemoryManagerResultSuccess: Successful processing.
//    kEsfMemoryManagerResultParamError: Parameter error.
//                        (If the memory operation handle entered is invalid)
//    kEsfMemoryManagerResultOperationError: Operation error.(Not unmapped.)

// Note:
// """
EsfMemoryManagerResult EsfMemoryManagerFree(EsfMemoryManagerHandle handle,
                                            const wasm_exec_env_t *exec_env);

// """ Map memory.
// Maps the memory held by the memory operation handle.
// EsfMemoryManagerMap is a function that maps memory for the given handle,
// execution environment and size.

// Args:
//    handle (EsfMemoryManagerHandle): a memory operation handle
//      Please specify the memory operation handle obtained
//      with EsfMemoryManagerAllocate().
//    exec_env (const wasm_exec_env_t*):
//    size (int32_t):
//    address (void **):
//      When map successful : Set the start address (virtual memory address)
//        of the map destination memory.
//      When map fails : Set NULL.

// Returns:
//     EsfMemoryManagerResult:
//        The code returns one of the values EsfMemoryManagerResult depending
//        on the execution result.

// Raises:
//     kEsfMemoryManagerResultSuccess: Successful processing.
//     kEsfMemoryManagerResultParamError: Parameter error.
//                    (If the memory operation handle entered is invalid)
//     kEsfMemoryManagerResultMapError: Memory map error
//     kEsfMemoryManagerResultOperationError: Operation error.
//                    (Map operations on map states.)

// Note:
//  If the target area is "LargeHeap" or "DMA", specifying "MAP_ALL_AREA(1)"
//  will map the allocated size (it is recommended to obtain the size using
//  MemoryMangerGetHandleInfo()).
// """
#define MAP_ALL_AREA (1)
EsfMemoryManagerResult EsfMemoryManagerMap(EsfMemoryManagerHandle handle,
                                           const wasm_exec_env_t *exec_env,
                                           int32_t size, void **address);

// """ Unmap memory.
// Unmap memory address from the specified handle.
// This function unmap a memory address associated with the given
// EsfMemoryManagerHandle. It checks for valid parameters, initializes if
// necessary, and then proceeds to unmap the memory based on the type of heap
// (WasmHeap or LargeHeap/DMA). If an error occurs during this process, it
// returns an appropriate error code.

// Args:
//    handle (EsfMemoryManagerHandle): The EsfMemoryManagerHandle associated
//      with the memory address to be unmapped.
//    address (void **): A pointer to the void * value representing the memory
//    address to be unmapped. Must not be NULL and must contain a non-zero
//    value.

// Returns:
//    EsfMemoryManagerResult:
//      An EsfMemoryManagerResult indicating the success or failure of the
//      operation, as well as any specific error codes that may have occurred
//      during execution.

// Raises:
//     kEsfMemoryManagerResultSuccess: Successful processing.
//     kEsfMemoryManagerResultParamError: Parameter error.
//                    (If the memory operation handle entered is invalid)
//     kEsfMemoryManagerResultOperationError: Operation error.
//                    (Unmap operations on unmap states.)

// Note:
// """
EsfMemoryManagerResult EsfMemoryManagerUnmap(EsfMemoryManagerHandle handle,
                                             void **address);

// """ Wasm Memory allocation.
// Reallocates memory in the application's memory space based on the specified
// usage and size.

// Args:
//    usage (EsfMemoryManagerWasmMemoryUsage): The intended usage of the
//      reallocated memory. Can be either kEsfMemoryManagerWasmAllocForRuntime
//      or kEsfMemoryManagerWasmAllocForLinearMemory.
//    size  : signed 32-bit integer 'size' indicating the size of the requested
//      memory.

// Returns:
//    The function returns a pointer of type 'EsfMemoryManagerAppMemory'
//    pointing to the allocated memory. If the size is less than or equal to
//    zero, or if an invalid usage value was provided, the function will return
//    NULL.

// Note:
//  This function is used to allocate memory based on the provided usage and
//  size. The function takes two parameters.
//  The function first checks if the provided size is greater than zero. If not,
//  it returns NULL without allocating any memory. Then, based on the usage
//  value, it sets an enumeration variable 'osal_usage' to either kOsalAppHeap
//  or kOsalAppLinearMemory. Finally, it calls OsalAppmemMalloc with
//  'osal_usage' and the provided size (converted to unsigned 32-bit integer) as
//  arguments and returns the allocated memory address casted to
//  EsfMemoryManagerAppMemory type. If the allocation fails, NULL is returned.
// """
EsfMemoryManagerAppMemory EsfMemoryManagerWasmAllocate(
    EsfMemoryManagerWasmMemoryUsage usage, int32_t size);

// """ Wasm Memory reallocation.
// Allocates memory in the application's memory space based on the specified
// usage and size.

// Args:
//    usage (EsfMemoryManagerWasmMemoryUsage): The intended usage of the
//      reallocated memory. Can be either kEsfMemoryManagerWasmAllocForRuntime
//      or kEsfMemoryManagerWasmAllocForLinearMemory.
//    old_memory (EsfMemoryManagerAppMemory) : Pointer to the existing memory
//      block that needs to be resized.
//    size (int32_t) : The new size of the memory block, in bytes. Must be
//      greater than 0.

// Returns:
//    EsfMemoryManagerAppMemory: A pointer to the newly allocated or resized
//    memory block. If reallocation fails, returns NULL.

// Note:
//  This function checks if the input parameters are valid and then attempts to
//  reallocate the specified memory block based on the given usage and size. The
//  new memory block is returned as a result of this operation. In case of any
//  errors or invalid inputs, the function returns NULL.
// """
EsfMemoryManagerAppMemory EsfMemoryManagerWasmReallocate(
    EsfMemoryManagerWasmMemoryUsage usage,
    const EsfMemoryManagerAppMemory old_memory, int32_t size);

// """ Frees the wasm memory.
// Args:
//    usage (EsfMemoryManagerWasmMemoryUsage): The type of memory allocation.
//      It can be either kEsfMemoryManagerWasmAllocForRuntime or
//      kEsfMemoryManagerWasmAllocForLinearMemory.
//    memory (EsfMemoryManagerAppMemory): Pointer to the allocated memory that
//      needs to be freed.

// Returns:
//    None

// Note:
//  This function checks if the input parameters are valid. If not, it returns
//  without performing any operation. It also converts the usage parameter into
//  an equivalent OsalAppmemUsage value and calls the OsalAppmemFree() function
//  with this converted value and the memory pointer to free the allocated
//  memory.
// """
void EsfMemoryManagerWasmFree(EsfMemoryManagerWasmMemoryUsage usage,
                              EsfMemoryManagerAppMemory memory);

// """ EsfMemoryManagerInitialize
// Initialize the EsfMemoryManager.
// This function initializes the EsfMemoryManager, which is responsible for
// managing memory resources in the system. It performs various tasks such as
// initializing OSAL LargeHeap, OSAL AppMemory, and setting the division number
// of OSAL AppMemory based on input parameters. If any error occurs during
// initialization, it returns an appropriate error code.

// Args:
//    app_mem_div_num:  The division number for OSAL AppMemory. This value
//      should be greater than 0.

// Returns:
//    EsfMemoryManagerResult An enumeration that indicates the result of the
//    operation. If successful, kEsfMemoryManagerResultSuccess is returned;
//    otherwise, an appropriate error code is returned.
// """
EsfMemoryManagerResult EsfMemoryManagerInitialize(int32_t app_mem_blocks);

// """ EsfMemoryManagerFinalize
// Finalize the EsfMemoryManager.
// This function finalizes the EsfMemoryManager, releasing all resources and
// making it unusable until initialized again. It is safe to call this function
// even if the memory manager has not been initialized or has already been
// finalized.

// Args:
//    none

// Returns:
//    EsfMemoryManagerResult An enumeration that indicates the result of the
//    operation. If successful, kEsfMemoryManagerResultSuccess is returned;
//    otherwise, an appropriate error code is returned.
// """
EsfMemoryManagerResult EsfMemoryManagerFinalize(void);

// """ EsfMemoryManagerFopen
// Opens a file associated with the specified memory manager handle.
// This function attempts to open a file that is associated with the given
// `EsfMemoryManagerHandle`. It ensures thread safety by locking a mutex before
// performing any operations. The function first checks if the memory manager
// has been initialized. If it hasn't been initialized, the function returns
// `kEsfMemoryManagerResultOtherError`.
//
// The function then checks the type of the handle. For handles associated with
// WasmHeap, FileIO functions are not supported and the function returns
// `kEsfMemoryManagerResultNotSupport`. For handles associated with LargeHeap
// or DMA, it delegates the file opening operation to
// `EsfMemoryManagerFileIoFopen(handle)`.
//
// After performing the necessary operations, the mutex is unlocked before the
// function returns the result.

// Args:
//    handle(EsfMemoryManagerHandle) - The handle associated with the memory
//    manager that is used for the file operation.

// Returns:
//    EsfMemoryManagerResult: An enumeration value indicating the result of the
//    operation. It can be one of the following values:
//      - `kEsfMemoryManagerResultSuccess` on successful operation.
//      - `kEsfMemoryManagerResultOtherError` if there was an error
//        (e.g., initialization check failed).
//      - `kEsfMemoryManagerResultNotSupport` if the handle is associated
//        with WasmHeap which does not support FileIO functions.
// """
EsfMemoryManagerResult EsfMemoryManagerFopen(EsfMemoryManagerHandle handle);

// """ EsfMemoryManagerFclose
// Closes a memory manager handle.
// This function attempts to close the given memory manager handle. It first
// locks a mutex to ensure thread safety, then checks if the memory manager is
// initialized. If not, it returns an error. It differentiates between different
// types of handles and calls appropriate functions to close them. For example,
// WasmHeap does not support file I/O functions, so it returns a specific error
// for that case.

// Args:
//    handle - The memory manager handle associated with the file to be closed.

// Returns:
//    EsfMemoryManagerResult - An enum value indicating the result of the
//    operation. It can be one of:
//      - kEsfMemoryManagerResultSuccess: The handle was successfully closed.
//      - kEsfMemoryManagerResultOtherError: An error occurred (e.g., memory
//      manager not initialized, mutex lock failure).
//      - kEsfMemoryManagerResultNotSupport: The handle type does not support
//      file I/O functions.
// """
EsfMemoryManagerResult EsfMemoryManagerFclose(EsfMemoryManagerHandle handle);

// """ EsfMemoryManagerFseek
// Seeks to a specified position in the memory manager.
// This function moves the file pointer associated with a given memory manager
// handle to a new location, determined by an offset and a reference point
// (whence).

// Args:
//    handle - The memory manager handle used to identify the memory block or
//    file.
//    offset - The number of bytes to move the file pointer from the
//    reference point specified by whence.
//    whence - The reference point from which the offset is applied. It can be
//    one of the following:
//      - SEEK_SET: Set the pointer to the offset.
//      - SEEK_CUR: Move the pointer from the current position by the offset.
//      - SEEK_END: Move the pointer from the end by the offset.
//    result_offset - Pointer to store the resultant offset after seeking. Must
//    not be NULL.

// Returns:
//    EsfMemoryManagerResult enum which can have the following values:
//      - kEsfMemoryManagerResultSuccess: The operation was successful.
//      - kEsfMemoryManagerResultParamError: A parameter error occurred (e.g.,
//      result_offset is NULL).
//      - kEsfMemoryManagerResultOtherError: An unspecified error occurred
//      (e.g., mutex lock/unlock failed).
//      - kEsfMemoryManagerResultNotSupport: The operation is not supported for
//      the given handle.
// """
EsfMemoryManagerResult EsfMemoryManagerFseek(EsfMemoryManagerHandle handle,
                                             off_t offset, int whence,
                                             off_t *result_offset);

// """ EsfMemoryManagerFwrite
// Writes data from a buffer to a memory location managed by the ESF Memory
// Manager. This function attempts to write "size" bytes of data from the buffer
// pointed to by "buff" to the memory location associated with the specified
// "handle". The actual number of bytes written is stored in the location
// pointed to by "rsize".

// Args:
//    handle - The handle to the memory location where data should be written.
//           Must be a valid ESF Memory Manager handle.
//    buff -  A pointer to the buffer containing the data to be written.
//         This parameter must not be NULL.
//    size - The number of bytes of data to write from "buff". This parameter
//    must be greater than zero. rsize - A pointer to a size_t variable where
//    the number of bytes actually written will be stored.
//          This parameter must not be NULL.

// Returns:
//    EsfMemoryManagerResult: The return value indicates whether the operation
//    was successful or an error occurred. Possible values are:
//      - kEsfMemoryManagerResultSuccess: The data was written successfully.
//      - kEsfMemoryManagerResultParamError: One or more parameters were invalid
//      (e.g., NULL pointers or size is zero).
//      - kEsfMemoryManagerResultOtherError: An error occurred during execution
//      (e.g., mutex lock failure or uninitialized state).
//      - kEsfMemoryManagerResultNotSupport: The specified handle does not
//      support file I/O operations.
// """
EsfMemoryManagerResult EsfMemoryManagerFwrite(EsfMemoryManagerHandle handle,
                                              const void *buff, size_t size,
                                              size_t *rsize);

// """ EsfMemoryManagerFpread
// Reads data from a memory-managed source into a buffer.
// This function reads data of the specified size from a given memory
// handle into the provided buffer. It also returns the actual
// number of bytes read through the rsize parameter.

// Args:
//    handle - The memory manager handle from which to read the data.
//    buff - The buffer into which the data will be read.
//    size - The size of the data to be read in bytes.
//    rsize - A pointer to a size_t variable where the actual number of bytes
//          read will be stored.

// Returns:
//    EsfMemoryManagerResult: An enumeration value indicating the result of the
//    operation. Possible values are:
//      - kEsfMemoryManagerResultSuccess: The read operation was successful.
//      - kEsfMemoryManagerResultParamError: An invalid parameter was passed.
//      - kEsfMemoryManagerResultOtherError: An error occurred during the mutex
//        lock/unlock operation or the memory manager is not initialized.
//      - kEsfMemoryManagerResultNotSupport: The operation is not supported by
//        the specified memory manager handle.
// """
EsfMemoryManagerResult EsfMemoryManagerFread(EsfMemoryManagerHandle handle,
                                             void *buff, size_t size,
                                             size_t *rsize);

// """ EsfMemoryManagerFpwrite
// Writes data from a buffer to a memory location managed by the ESF Memory
// Manager. This function attempts to write "size" bytes of data from the buffer
// pointed to by "buff" to the memory location associated with the specified
// "handle". The actual number of bytes written is stored in the location
// pointed to by "rsize".

// Args:
//    handle - The handle to the memory location where data should be written.
//           Must be a valid ESF Memory Manager handle.
//    buff -  A pointer to the buffer containing the data to be written.
//         This parameter must not be NULL.
//    size - The number of bytes of data to write from "buff". This parameter
//      must be greater than zero.
//    offset - The number of bytes to move the file pointer from the reference
//      point specified at the beginning of the region.
//    rsize - A pointer to a size_t variable where the number of bytes actually
//      written will be stored. This parameter must not be NULL.

// Returns:
//    EsfMemoryManagerResult: The return value indicates whether the operation
//    was successful or an error occurred. Possible values are:
//      - kEsfMemoryManagerResultSuccess: The data was written successfully.
//      - kEsfMemoryManagerResultParamError: One or more parameters were invalid
//      (e.g., NULL pointers or size is zero).
//      - kEsfMemoryManagerResultOtherError: An error occurred during execution
//      (e.g., mutex lock failure or uninitialized state).
//      - kEsfMemoryManagerResultNotSupport: The specified handle does not
//      support file I/O operations.
// """
EsfMemoryManagerResult EsfMemoryManagerFpwrite(EsfMemoryManagerHandle handle,
                                               const void *buff, size_t size,
                                               off_t offset, size_t *rsize);

// """ EsfMemoryManagerFread
// Reads data from a memory-managed source into a buffer.
// This function reads data of the specified size from a given memory
// handle into the provided buffer. It also returns the actual
// number of bytes read through the rsize parameter.

// Args:
//    handle - The memory manager handle from which to read the data.
//    buff - The buffer into which the data will be read.
//    size - The size of the data to be read in bytes.
//    offset - The number of bytes to move the file pointer from the reference
//      point specified at the beginning of the region.
//    rsize - A pointer to a size_t variable where the number of bytes actually
//      written will be stored. This parameter must not be NULL.

// Returns:
//    EsfMemoryManagerResult: An enumeration value indicating the result of the
//    operation. Possible values are:
//      - kEsfMemoryManagerResultSuccess: The read operation was successful.
//      - kEsfMemoryManagerResultParamError: An invalid parameter was passed.
//      - kEsfMemoryManagerResultOtherError: An error occurred during the mutex
//        lock/unlock operation or the memory manager is not initialized.
//      - kEsfMemoryManagerResultNotSupport: The operation is not supported by
//        the specified memory manager handle.
// """
EsfMemoryManagerResult EsfMemoryManagerFpread(EsfMemoryManagerHandle handle,
                                              void *buff, size_t size,
                                              off_t offset, size_t *rsize);

// """ EsfMemoryManagerIsMapSupport
// Check if map support is available for the given memory manager handle.
// This function checks whether the memory manager referenced by the provided
// handle supports memory mapping operations. It first validates the parameters,
// and then it checks the initialization and the type of memory handle to
// determine the map support capability.

// Args:
//    handle - The handle to the memory manager that needs to be checked.
//    support - A pointer to an EsfMemoryManagerMapSupport variable where the
//    result will be stored.

// Returns:
//    EsfMemoryManagerResult: An enumeration value indicating the result of the
//    operation. Possible values are:
//      - kEsfMemoryManagerResultSuccess: Operation was successful.
//      - kEsfMemoryManagerResultParamError: An invalid parameter was provided
//      (e.g., support is NULL).
//      - kEsfMemoryManagerResultOtherError: Another error occurred (e.g., mutex
//      lock failed or not initialized).
// """
EsfMemoryManagerResult EsfMemoryManagerIsMapSupport(
    EsfMemoryManagerHandle handle, EsfMemoryManagerMapSupport *support);

// """ EsfMemoryManagerGetHandleInfo
// Retrieves information about a specific memory handle.
// This function takes a memory handle, retrieves its associated information,
// and populates the provided EsfMemoryManagerHandleInfo structure with this
// data. The function checks if the given handle is related to WasmHeap,
// LargeHeap, DMA, or another memory area and sets the target_area and
// allocate_size fields accordingly.

// Args:
//    handle - The memory handle for which information is requested.
//    info - A pointer to an EsfMemoryManagerHandleInfo structure where the
//    handle information will be stored. Must not be NULL.

// Returns:
//    EsfMemoryManagerResult Result of the operation,
//    - kEsfMemoryManagerResultSuccess: The operation was successful.
//      The 'info' structure contains valid data.
//    - kEsfMemoryManagerResultParamError: An error occurred due to invalid
//    parameters
//      (e.g., 'info' is NULL).
//    - kEsfMemoryManagerResultOtherError: An unspecified error occurred during
//    the operation.
// """
EsfMemoryManagerResult EsfMemoryManagerGetHandleInfo(
    uint32_t handle, EsfMemoryManagerHandleInfo *info);

#ifdef __cplusplus
}
#endif

#endif  // ESF_MEMORY_MANAGER_INCLUDE_MEMORY_MANAGER_H_
