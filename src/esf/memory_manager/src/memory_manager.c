/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define _GNU_SOURCE
// Define the external public API for memory manager.
#include "memory_manager.h"

#include <inttypes.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>       // for free
#include <string.h>       // for memcpy
#include <wasm_export.h>  // for wasm_exec_env_t, wasm_runtim...
#ifndef __NuttX__
#include <bsd/sys/queue.h>
#else
#include <sys/queue.h>
#endif  // __NuttX__

#include "memory_manager_internal.h"

// Global Variables ------------------------------------------------------------

// This variable is used by MemoryManager as a list management variable for
// memory information allocated from the LargeHeap area and DMA area.
STATIC struct EsfMemoryManagerHandleInfoList s_handle_info_list;

// This variable is used as a flag variable to indicate completion of
// initialization of MemoryManager.
//    true: Initialized
//    false: Not initialized
STATIC bool s_is_initialized = false;

// This variable is a mutex object and is used for exclusive control of the
// MemoryManager API.
STATIC pthread_mutex_t s_memory_manager_mutex = PTHREAD_MUTEX_INITIALIZER;

// This variable is used to manage the handle ID of memory allocated by
// MemoryManager. note:
//  128-bit bit positions are used as handle IDs/unused bits are managed
//  ("bit 0" is unused: handle ID=0 is reserved for WasmHeap)
STATIC uint32_t s_handle_id_map[4] = {0, 0, 0, 0};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/
// """ EsfMemoryManagerAllocateWasm
// This function allocates memory in the WASM module's linear memory space.
// The allocated memory is managed by the WebAssembly runtime and can be used
// for various purposes such as storing global variables or passing data between
// host functions and WASM functions.

// Args:
//      exec_env (wasm_exec_env_t*): A pointer to the execution environment of
//       the current WASM module. This is required for accessing the linear
//       memory space of the module.
//      size (int32_t): The number of bytes to allocate in the WASM module's
//       linear memory space.
//      user_handle (EsfMemoryManagerHandle*): A pointer to a handle that will
//       store the address of the allocated memory. This handle can be used
//       later to free or reallocate the memory.

// Returns:
//     EsfMemoryManagerResult: An enumeration value indicating the result of the
//     operation. It can have one of the following values:
//      - kEsfMemoryManagerResultSuccess: The allocation was successful and the
//      user_handle contains the address of the allocated memory.
//      - kEsfMemoryManagerResultParamError: One or more parameters are invalid,
//      such as a non-positive size value or a NULL user_handle pointer. In this
//      case, no memory is allocated and the user_handle remains unchanged.

// Raises:
//     kEsfMemoryManagerResultSuccess: Successful processing.
//     kEsfMemoryManagerResultParamError: Parameter error.
//     kEsfMemoryManagerResultAllocationError: Memory allocation error.
// Notes:
//     This function checks if the given execution environment is valid before
//     proceeding with the allocation. If it's not, an error result is returned
//     without further action.
// """
STATIC EsfMemoryManagerResult
EsfMemoryManagerAllocateWasm(const wasm_exec_env_t *exec_env, int32_t size,
                             EsfMemoryManagerHandle *memory_handle);

// """ EsfMemoryManagerGenerateMemoryInfo
//  Description: This function generates memory information for a specified
//  target area and size.

// Args:
//    target_area (EsfMemoryManagerTargetArea): The target area to allocate
//     memory from. Can be kEsfMemoryManagerTargetLargeHeap,
//     kEsfMemoryManagerTargetDma or kEsfMemoryManagerTargetWasmHeap.
//    size (int32_t): The requested allocation size in bytes.
//    user_handle (EsfMemoryManagerHandle *): A pointer to an integer that will
//     store the allocated memory operation handle ID on success.

// Returns:
//     EsfMemoryManagerResult: An enum value indicating the result of the
//     function. Can be kEsfMemoryManagerResultSuccess,
//     kEsfMemoryManagerResultAllocationError or
//     kEsfMemoryManagerResultParamError.
// """
STATIC EsfMemoryManagerResult EsfMemoryManagerGenerateMemoryInfo(
    EsfMemoryManagerTargetArea target_area, int32_t size,
    EsfMemoryManagerHandle *user_handle);

// """ EsfMemoryManagerFreeWasm
// This function is responsible for freeing memory allocated by the WebAssembly
// (wasm) module instance. It takes two parameters, a handle and an execution
// environment pointer. The handle contains information about the memory block
// to be freed, while the exec_env parameter points to the wasm execution
// environment that holds the module instance.

// Args:
//    handle (EsfMemoryManagerHandle): A handle containing information about the
//     memory block to be freed. It is composed of an ID and an offset.
//    exec_env (const wasm_exec_env_t*): Pointer to a wasm execution environment
//     that holds the module instance.

// Returns:
//    EsfMemoryManagerResult:
//      An enumeration value indicating the result of the operation.
//      It can be one of the following values:
//       - kEsfMemoryManagerResultSuccess: The memory block was successfully
//       freed.
//       - kEsfMemoryManagerResultParamError: One or more parameters are
//       invalid, such as a NULL pointer or an incorrect handle value.

// Note:
//    1. First, the function checks if the handle's ID is not equal to zero or
//    its offset is equal to zero. If either condition is true, it returns
//    kEsfMemoryManagerResultParamError and exits the function.
//    2. Next, the function validates the wasm module instance by calling
//    EsfMemoryManagerValidateWasmModuleInstance(). If the validation fails, the
//    function again returns kEsfMemoryManagerResultParamError and terminates.
//    3. After successful validation of the module instance, the function
//    retrieves the module instance from the execution environment using
//    wasm_runtime_get_module_inst().
//    4. Finally, it frees the memory block by calling
//    wasm_runtime_module_free() with the module instance and the handle's
//    offset as arguments.
// """
STATIC EsfMemoryManagerResult EsfMemoryManagerFreeWasm(
    EsfMemoryManagerHandle handle, const wasm_exec_env_t *exec_env);

// """ EsfMemoryManagerDiscardMemoryInfo
// This function discards memory information from the memory management system.
// It takes a handle as an argument and checks if it exists in the memory
// manager's list of handles. If not, it returns a parameter error result. Then,
// it iterates through the linked list of memory management information to find
// the matching entry based on the given handle ID. Once found, it verifies that
// the memory is unmapped and has no map count. It then frees the allocated
// memory depending on the target area (large heap or DMA). After freeing the
// memory, it cleans up the map management information list by removing all
// entries and deallocating their memory. Finally, it removes the found entry
// from the linked list of memory manager's handles and frees its memory before
// returning a success result. If any error occurs during this process, an
// appropriate error result is returned instead.

// Args:
//    handle - The handle to discard memory information for.

// Returns:
//    kEsfMemoryManagerResultSuccess:
//      - On successful completion of the operation.
//    kEsfMemoryManagerResultParamError:
//      - If the given handle does not exist in the memory manager's list or if
//      an invalid target area is encountered.
//    kEsfMemoryManagerResultOperationError:
//      - If the memory is still mapped, has a non-zero map count, or cannot be
//      freed properly.
//    kEsfMemoryManagerResultOtherError:
//      - For any other unspecified error during the operation.
// """
STATIC EsfMemoryManagerResult
EsfMemoryManagerDiscardMemoryInfo(EsfMemoryManagerHandle handle);

// """ EsfMemoryManagerMapWasm
// Maps a WebAssembly memory region to the native address space.
// This function maps a specified WebAssembly memory region to the native
// address space. The mapping is performed based on the handle and offset
// provided in the handle parameter, as well as the size of the memory region.
// If successful, it returns kEsfMemoryManagerResultSuccess and stores the
// mapped virtual address in the 'address' pointer.

// Args:
//    handle: Handle for the memory manager instance.
//    size: Size of the memory region in bytes.
//    exec_env: Pointer to the execution environment of the WebAssembly module.
//    address: Pointer to store the mapped virtual address.

// Return:
//    EsfMemoryManagerResult indicating the result of the operation
//    (kEsfMemoryManagerResultSuccess on success).
// """
STATIC EsfMemoryManagerResult
EsfMemoryManagerMapWasm(EsfMemoryManagerHandle handle, int32_t size,
                        const wasm_exec_env_t *exec_env, void **address);

// """ EsfMemoryManagerMapMemoryInfo
// Maps the memory information of a specified
// This function maps the memory information associated with the given handle.
// It first checks if the handle exists and then selects the appropriate memory
// management information from the list. If the area is not large enough for the
// requested size, it returns an error. The function then proceeds to map the
// virtual address using OSAL's MapAPI. After a successful mapping, it updates
// the map address for the entire area (the beginning of the mapped region). It
// also registers the offset values and increments the map count.

// Args:
//    handle: The handle to map memory for.
//    size: The size in bytes of the memory region to be mapped.
//    address: A pointer to store the resulting virtual address after mapping.

// Return:
//    kEsfMemoryManagerResultSuccess if the operation was successful,
//    kEsfMemoryManagerResultParamError if there were invalid parameters,
//    kEsfMemoryManagerResultMapError if there was an error during mapping.
// """
STATIC EsfMemoryManagerResult EsfMemoryManagerMapMemoryInfo(
    EsfMemoryManagerHandle handle, int32_t size, void **address);

// """ EsfMemoryManagerMapMemory
// Maps the memory of a specified area to the virtual address space.
// This function maps the memory of a specified area (large heap, DMA, or WASM
// heap) to the virtual address space. The mapping process is performed using
// OSAL's MapAPI. If it is the first time mapping, the map address for the
// entire area will be updated.

// Args:
//    entry : Pointer to an EsfMemoryManagerHandleInternal structure that
//    contains information about the memory area to be mapped.

// Return:
//    kEsfMemoryManagerResultSuccess if successful; otherwise, an error code
//    indicating the type of failure (e.g., parameter error or mapping error).
// """
STATIC EsfMemoryManagerResult
EsfMemoryManagerMapMemory(struct EsfMemoryManagerHandleInternal *entry);

// """ EsfMemoryManagerUnmapWasm
// This function is used to unmap the wasm memory from the
// application's address space.

// Args:
//    handle - A handle that represents a memory manager instance.
//    address - Pointer to the void * value containing the base address of the
//    mapped memory region.

// Return:
//    The function returns an EsfMemoryManagerResult enum indicating the result
//    of the operation. It can be one of the following values:
//      kEsfMemoryManagerResultSuccess - Indicates that the operation was
//      successful.
//      kEsfMemoryManagerResultParamError - Indicates that there was
//      a parameter error, such as an invalid handle or address pointer.
// """
STATIC EsfMemoryManagerResult
EsfMemoryManagerUnmapWasm(EsfMemoryManagerHandle handle, void **address);

// """ EsfMemoryManagerUnmapMemoryInfo
// Unmap memory from the specified handle.
// This function unmap memory from a given handle. It first checks if the
// handle exists and then proceeds to remove the corresponding map management
// information from the map management list in the memory management information
// list. The virtual address unmap processing is performed by deleting the
// corresponding map entry and clearing all map addresses of the entire area
// (setting them to "ff"). If the map registration count becomes 0, it executes
// Osal's UnAPI for unmapping.

// Args:
//    handle: Handle to unmap memory from.

// Return:
//    kEsfMemoryManagerResultSuccess if successful, an error code otherwise.
// """
STATIC EsfMemoryManagerResult
EsfMemoryManagerUnmapMemoryInfo(EsfMemoryManagerHandle handle);

// """ EsfMemoryManagerUnmapMemory
// This function is used to unmap memory that has been previously mapped using
// the EsfMemoryManagerMapMemory function. It takes a pointer to an
// EsfMemoryManagerHandleInternal structure as its argument, which contains
// information about the memory being unmapped. The function first checks if the
// entry parameter is NULL. If it is, the function returns
// kEsfMemoryManagerResultParamError indicating an error with the parameters
// passed in. Next, the function checks if the map_count of the entry is 0. If
// so, it proceeds to unmap the memory based on the target_area specified in the
// entry structure:
//   - For kEsfMemoryManagerTargetLargeHeap, it calls PlLheapUnmap with the
//   map_address as its argument.
//   - For kEsfMemoryManagerTargetDma, it calls PlDmaMemUnmap with the
//   map_address as its argument.
// If an error occurs during the unmapping process (indicated by a non-zero
// err_code), the function logs the error and returns
// kEsfMemoryManagerResultMapError. Finally, if no errors occur, the function
// sets the map_address field of the entry structure to MEMORY_MANAGER_UNMAP and
// returns kEsfMemoryManagerResultSuccess indicating successful unmapping of
// memory.

// Args:
//    handle - A handle that represents a memory manager instance.
//    address - Pointer to the void * value containing the base address of the
//    mapped memory region.

// Return:
//    The function returns an EsfMemoryManagerResult enum indicating the result
//    of the operation. It can be one of the following values:
//      kEsfMemoryManagerResultSuccess - Indicates that the operation was
//      successful.
//      kEsfMemoryManagerResultParamError - Indicates that there was
//      a parameter error, such as an invalid handle or address pointer.
// """
STATIC EsfMemoryManagerResult
EsfMemoryManagerUnmapMemory(struct EsfMemoryManagerHandleInternal *entry);

// """ EsfMemoryManagerClearHandleId
//  Description: This function is used to clear the s_handle_id_map array by
//  initializing all its elements to zero.

// Args: None

// Returns: void - The function does not return any value. It just clears the
// s_handle_id_map array.
// """
STATIC void EsfMemoryManagerClearHandleId(void);

// """ EsfMemoryManagerGetFreeHandleId
//  Description: Get a free handle ID from the memory manager.
//  This function searches for an available handle ID within the range of 1 to
//  MEMORY_MANAGER_MAX_MAPS. It uses a bit map (s_handle_id_map) to track which
//  IDs are in use or not. The first available ID is returned, and -1 is
//  returned if no free ID is found.

// Args: None

// Returns:
//  int8_t: Returns the first available handle ID as an 8-bit integer. If there
//  are no free IDs, it returns -1.
// """
STATIC int8_t EsfMemoryManagerGetFreeHandleId(void);

// """ EsfMemoryManagerReturnUsedHandleId
// Description: This function is responsible for clearing the bit in the
// s_handle_id_map array that corresponds to a given used_id.

// Args:
//    int8_t used_id - The handle ID of which its corresponding bit in the
//    s_handle_id_map will be cleared.

// Returns: void

// Details about Return Value: This function does not return any value. It only
// clears the bit in the s_handle_id_map array that corresponds to a given
// used_id.
// """
STATIC void EsfMemoryManagerReturnUsedHandleId(int8_t used_id);

// """ EsfMemoryManagerHandleExists
//  Description: This function checks if a given handle exists in the memory
//  management information list.

// Args:
//    handle (EsfMemoryManagerHandle): The handle to be checked for its
//    existence in the memory manager's internal data structure.

// Returns:
//   - bool: Returns true if the handle is found, otherwise returns false
//   indicating that the handle does not exist in the memory management
//   information list.
// """
STATIC bool EsfMemoryManagerHandleExists(EsfMemoryManagerHandle handle);

// """ EsfMemoryManagerIsHandleExist
//  Description: This function checks if a given handle exists in the memory
//  management information list. And if it exists, it returns the entry.

// Args:
//    handle (EsfMemoryManagerHandle): The handle to be checked for its
//    existence in the memory manager's internal data structure.
//    [out]entry: Pointer to an EsfMemoryManagerHandleInternal structure
//    representing the memory manager handle.

// Returns:
//   - bool: Returns true if the handle is found, otherwise returns false
//   indicating that the handle does not exist in the memory management
//   information list.
// """
STATIC bool EsfMemoryManagerIsHandleExist(
    EsfMemoryManagerHandle handle,
    struct EsfMemoryManagerHandleInternal **entry);

// """ EsfMemoryManagerMapInfoExists
//  Description: This function checks if a specific map information exists in
//  the memory manager's handle.

// Args:
//    handle (EsfMemoryManagerHandle): The handle to be checked for its
//     corresponding map information.
//    memory_handle (const EsfMemoryManagerHandleInternal*): A pointer to the
//     internal memory manager handle structure.

// Returns:
//     bool: Returns true if the specified map information exists, otherwise
//     returns false.
// """
STATIC bool EsfMemoryManagerMapInfoExists(
    EsfMemoryManagerHandle handle,
    const struct EsfMemoryManagerHandleInternal *memory_handle);

// """ EsfMemoryManagerFileIoGetHandleEntry
// Retrieves the handle entry and map entry for a given memory manager handle.
// This function attempts to retrieve the internal handle entry and the
// corresponding map entry associated with the specified memory manager handle.
// It checks if the provided pointers for handle entry and map entry are valid,
// and then proceeds to verify the existence of the handle and map information.
// If any of these checks fail, an appropriate error code is returned.

// Args:
//    handle - The memory manager handle for which the entries are to be
//              retrieved.
//    handle_entry - A pointer to a location where the handle entry
//                    will be stored.
//                     Must not be NULL.
//    map_entry - A pointer to a location where the map entry will be stored.
//                  Must not be NULL.

// Returns:
//    Returns a result code indicating the outcome of the operation:
//      - kEsfMemoryManagerResultSuccess: The operation was successful, and
//         both handle entry and map entry have been retrieved.
//      - kEsfMemoryManagerResultParamError: One or more parameters were
//         invalid, or the handle/map information does not exist.
// """
STATIC EsfMemoryManagerResult EsfMemoryManagerFileIoGetHandleEntry(
    EsfMemoryManagerHandle handle,
    struct EsfMemoryManagerHandleInternal **handle_entry,
    struct EsfMemoryManagerHandleAddressOffset **map_entry);

// """ EsfMemoryManagerIsMapInfoExist
// This function searches through the map management information list to find
// if there is an entry that corresponds to the provided memory manager handle.
// If such an entry is found, it sets the `map_info` pointer to point to this
// entry and returns true. If no matching entry is found, it sets `map_info`
// to NULL and returns false.

// Args:
//    handle (EsfMemoryManagerHandle): The handle to be checked for its
//     corresponding map information.
//    memory_handle (const EsfMemoryManagerHandleInternal*): A pointer to the
//     internal memory manager handle structure.
//    map_info (EsfMemoryManagerHandleAddressOffset): A double pointer to a
//     structure where the address of the found map information will be stored
//     if it exists.

// Returns:
//    Returns true if the map information exists and `map_info` is set to point
//    to the corresponding entry. Returns false if no map information is found
//    and `map_info` is set to NULL.
// """
STATIC bool EsfMemoryManagerIsMapInfoExist(
    EsfMemoryManagerHandle handle,
    const struct EsfMemoryManagerHandleInternal *memory_handle,
    struct EsfMemoryManagerHandleAddressOffset **map_info);

// """ EsfMemoryManagerCleanUpMemoryInfo
//  Description: This function is responsible for cleaning up the memory
//  information of an EsfMemoryManagerHandleInternal structure.

// Args:
//    memory_handle (EsfMemoryManagerHandleInternal*): A pointer to an
//     EsfMemoryManagerHandleInternal structure representing the memory
//     manager's internal handle. If this parameter is NULL, the function does
//     nothing and returns immediately.

// Returns: None

// details:
//    This function performs the following tasks:
//      - Iterates through a linked list of map management information entries
//      associated with the given memory_handle and removes each entry from the
//      list, freeing its memory.
//      - If memory_handle->allocate_address is not 0, it frees the allocated
//      memory based on the value of memory_handle->target_area:
//        - kEsfMemoryManagerTargetLargeHeap: The memory is freed using
//        PlLheapFree.
//        - kEsfMemoryManagerTargetDma: The memory is freed using
//        PlDmaMemFree.
//        - Other values of memory_handle->target_area, including
//        kEsfMemoryManagerTargetWasmHeap: No action is taken.
//          - Finally, the memory allocated for memory_handle itself is freed
//          and memory_handle is set to NULL.
// """
STATIC void EsfMemoryManagerCleanUpMemoryInfo(
    struct EsfMemoryManagerHandleInternal *memory_handle);

// """ EsfMemoryManagerCleanUpAllMemoryInfo
//  Description: This function is responsible for cleaning up all memory
//  management information. It iterates through the linked list of memory
//  management handles and cleans up any remaining map entries, unmap memory if
//  necessary, frees allocated memory, and finally discards the memory
//  management information.

// Args: None

// Returns:
//    - kEsfMemoryManagerResultSuccess: If all operations are successful.
// """
STATIC EsfMemoryManagerResult EsfMemoryManagerCleanUpAllMemoryInfo(void);

// """  EsfMemoryManagerCleanUpRemainResource
// Clean up remaining resources in the memory manager.
// This function cleans up any remaining resources such as memory maps and file
// descriptors associated with a given EsfMemoryManagerHandleInternal structure.
// It is called when deinitialize or releasing resources in the memory manager.

// Args:  Pointer to an internal handle of the memory manager.

// Return: None, but logs errors if encountered during cleanup process.
// """
STATIC void EsfMemoryManagerCleanUpRemainResource(
    struct EsfMemoryManagerHandleInternal *entry);

// """ EsfMemoryManagerValidateWasmModuleInstance
// This function checks if the provided execution environment and its associated
// WASM module instance are valid or not. If either of them is NULL, it returns
// false indicating an invalid state. Otherwise, it returns true.
// EsfMemoryManagerValidateWasmModuleInstance - Validates the WASM module
// instance in the given execution environment.

// Args:
//    exec_env: Pointer to the execution environment containing the WASM module
//     instance.

// Returns:
//    True if both the execution environment and the WASM module instance are
//    valid, otherwise false.
// """
STATIC bool EsfMemoryManagerValidateWasmModuleInstance(
    const wasm_exec_env_t *exec_env);

// """ EsfMemoryManagerFopenAreaCheck
// This function determines whether a requested memory area, defined by its size
// and offset, can be accommodated within a larger memory area. It ensures that
// the offset is within bounds and that the requested size does not exceed the
// available space from the offset to the end of the area.

// Args:
//    area_size - The total size of the target memory area.
//    handle_offset - The offset within the target area where the requested area
//      begins.

// Returns:
//    Returns true if the requested area fits within the target area starting at
//    the given offset; otherwise, returns false if the offset is out of range
//    or if the requested size exceeds the available space.
// """
STATIC bool EsfMemoryManagerFopenAreaCheck(uint32_t area_size,
                                           uint32_t handle_offset);

// """ EsfMemoryManagerFileIoFopen
// This function attempts to open a file for the specified memory manager
// handle. It checks if the handle exists and whether it is already open. If the
// handle does not exist or the entry information is missing, an error is
// returned. If the maximum capacity is specified, the size is adjusted to match
// the entire size of the handle's memory area. The function also verifies if
// the map(open) information already exists and returns success if the handle is
// already open with the same offset and size.

// Args:
//    handle - The memory manager handle used to identify the file to open.
//    size - The size of the memory area to be opened. If set to
//    MEMORY_MANAGER_OPEN_MAX_SIZE, it will use the full size of the handle.

// Returns:
//    The return value is of type EsfMemoryManagerResult, which indicates the
//    result of the operation. It can be one of the following values:
//    - kEsfMemoryManagerResultSuccess: The file was successfully opened.
//    - kEsfMemoryManagerResultParamError: An invalid parameter was provided
//      or the handle does not exist.
//    - kEsfMemoryManagerResultOperationError: The file is already open or
//      the handle is currently being accessed.
//    - kEsfMemoryManagerResultFileIoError: The file could not be opened due
//      to an I/O error.
//    - kEsfMemoryManagerResultNotSupport: The operation is not supported for
//      this target area.
// """
STATIC EsfMemoryManagerResult
EsfMemoryManagerFileIoFopen(EsfMemoryManagerHandle handle, int32_t size);

// """ EsfMemoryManagerFileIoFclose
// Closes a file associated with the given memory handle.
// This function attempts to close a file that is managed by the specified
// EsfMemoryManagerHandle. It verifies the existence of the handle and checks
// the access state before proceeding to close the file. The function supports
// different target areas such as Large Heap, DMA, and Wasm Heap, handling them
// accordingly.

// Args:
//    handle - An EsfMemoryManagerHandle that uniquely identifies the memory
//    location or resource being managed.

// Returns:
//    EsfMemoryManagerResult: The result of the operation. Possible values are:
//    - kEsfMemoryManagerResultSuccess: File closed successfully.
//    - kEsfMemoryManagerResultParamError: Parameter error if the handle
//      doesn't exist or entry information is missing.
//    - kEsfMemoryManagerResultOperationError: Operation error due to
//      inappropriate access state or if already closed.
//    - kEsfMemoryManagerResultFileIoError: Error in closing the file.
//    - kEsfMemoryManagerResultNotSupport: Operation not supported for the
//      specific target area.
// """
STATIC EsfMemoryManagerResult
EsfMemoryManagerFileIoFclose(EsfMemoryManagerHandle handle);

// """ EsfMemoryManagerFileIoFseekPreCheck
// Pre-checks parameters for file seek operations in the ESF Memory Manager.
// This function performs preliminary checks on the parameters provided for a
// file seek operation, ensuring that the seek position is valid within the
// constraints of the allocated memory size. The validation is done based on the
// 'whence' parameter, which specifies the reference point for the offset:
// - SEEK_SET: Beginning of the file.
// - SEEK_CUR: Current position in the file.
// - SEEK_END: End of the file.
// The function checks if the calculated seek position lies within the
// permissible range (from 0 to allocate_size). If the parameters are valid, the
// function returns a success result; otherwise, it returns an error indicating
// a parameter issue.

// Args:
//    map_info - A pointer to a structure containing the handle address offset
//      information, including the current seek position and file size.
//    param - Pointer to the structure containing the seek parameters,
//      including the offset and whence value (SEEK_SET, SEEK_CUR, or SEEK_END).

// Returns:
//    An EsfMemoryManagerResult value indicating whether the function succeeded
//    or encountered an error. The possible return values are as follows:
//      - kEsfMemoryManagerResultSuccess - The parameters were valid, and the
//        file seek operation can proceed.
//      - kEsfMemoryManagerResultParamError - One of
//        the input parameters was invalid, such as a negative offset for
//        SEEK_SET or an out-of-bounds offset. In this case, the file seek
//        operation should not be attempted.
// """
STATIC EsfMemoryManagerResult EsfMemoryManagerFileIoFseekPreCheck(
    struct EsfMemoryManagerHandleAddressOffset *map_info,
    EsfMemoryManagerFileIoFseekParam *param);

// """ EsfMemoryManagerFileIoFseekExec
// Executes a file seek operation in the memory manager.
// This function attempts to perform a seek operation on a file using the
// specified parameters and updates the file position if successful. It is
// designed to handle specific error conditions and return appropriate result
// codes based on the success or failure of the operation.

// Args:
//    entry - A pointer to an EsfMemoryManagerHandleInternal structure,
//        which contains information relevant to the memory manager's file I/O
//        handling, including the allocated address and file descriptor.
//    map_info - A pointer to a structure containing the handle address offset
//        information, including the current seek position and file size.
//    param - A pointer to an EsfMemoryManagerFileIoFseekParam structure
//        that holds parameters necessary for the seek operation, such as the
//        offset and whence (similar to the standard 'fseek' parameters), and
//        a pointer to store the resulting offset after the seek.

// Returns:
//    An EsfMemoryManagerResult indicating the result of the seek operation:
//      - kEsfMemoryManagerResultSuccess: The seek operation was completed
//        successfully, and the current file position has been updated.
//      - kEsfMemoryManagerResultParamError: The seek operation failed due
//        to invalid parameters.
//      - kEsfMemoryManagerResultFileIoError: An error occurred during the
//        seek operation that is not related to parameter invalidity.
// """
STATIC EsfMemoryManagerResult EsfMemoryManagerFileIoFseekExec(
    uint64_t allocate_address, int file_descriptor, uint32_t address_offset,
    int32_t allocate_size, EsfMemoryManagerFileIoFseekParam *param,
    off_t *seek_position);

// """ EsfMemoryManagerFileIoPreCheck
// This function performs a pre-check before executing a FileIO operation based
// on the specified target area. It determines whether the FileIO operation is
// supported for the given memory allocation address and target area.

// Args:
//    target_area - The target area where the memory operation is intended. It
//    can be one of the following:
//                    - kEsfMemoryManagerTargetLargeHeap: Large heap area.
//                    - kEsfMemoryManagerTargetDma: DMA area.
//                    - kEsfMemoryManagerTargetWasmHeap: WASM heap area.
//    allocate_address - The address to be allocated or checked for the FileIO
//    operation.

// Returns:
//    EsfMemoryManagerResult Returns the result of the pre-check operation:
//         - kEsfMemoryManagerResultSuccess: The pre-check was successful, and
//         the seek operation should proceed.
//         - kEsfMemoryManagerResultNotSupport: The FileIO operation is not
//         supported for the specified target area.
//         - kEsfMemoryManagerResultParamError: An error occurred due to invalid
//         parameters.
// """
STATIC EsfMemoryManagerResult EsfMemoryManagerFileIoPreCheck(
    EsfMemoryManagerTargetArea target_area, uint64_t allocate_address);

// """ EsfMemoryManagerFileIoSeekParamCheck
// Checks the validity of the parameters for file I/O seek operations.
// This function verifies that the provided parameter structure is not NULL and
// that the result_offset within the structure is also not NULL. It ensures that
// the necessary parameters for a file seek operation are valid before
// proceeding.

// Args:
//    param A pointer to an EsfMemoryManagerFileIoFseekParam structure that
//    contains the parameters for the file seek operation.

// Returns:
//    Returns kEsfMemoryManagerResultParamError if the param or its
//    result_offset is NULL, indicating invalid parameters. Otherwise, returns
//    kEsfMemoryManagerResultSuccess, indicating that the parameters are valid.
// """
STATIC EsfMemoryManagerResult
EsfMemoryManagerFileIoSeekParamCheck(EsfMemoryManagerFileIoFseekParam *param);

// """ EsfMemoryManagerFileIoFseek
// Executes file seek operation for the memory manager.
// This function performs a seek operation on a file associated with
// the provided memory manager entry. It attempts to reposition the
// file pointer according to the specified offset and direction, and
// updates the result with the new file position if successful.

// Args:
//    entry - A pointer to the EsfMemoryManagerHandleInternal structure
//          representing the current memory manager's internal handle.
//    param - A pointer to the EsfMemoryManagerFileIoFseekParam structure
//          containing parameters needed for the seek operation, such as
//          offset, whence, and the result_offset for storing the seek result.

// Returns:
//    The function returns an 'EsfMemoryManagerResult', which indicates the
//    result of the operation. Possible return values are:
//      - kEsfMemoryManagerResultSuccess: If the seek operation is successful.
//      - kEsfMemoryManagerResultParamError: If the parameters are invalid.
//      - kEsfMemoryManagerResultFileIoError: If the seek operation fails.

// Note:
//    This function first checks if map function support is available for the
//    given memory manager handle. If it is, then it proceeds to check the
//    parameters and perform a seek pre-check before executing the seek
//    operation. If map function support is not available (e.g., for DMA or WASM
//    heap), the function returns kEsfMemoryManagerResultNotSupport.
// """
STATIC EsfMemoryManagerResult EsfMemoryManagerFileIoFseek(
    EsfMemoryManagerHandle handle, EsfMemoryManagerFileIoFseekParam *param);

// """ EsfMemoryManagerFileIoAccessFunc
// Handles file I/O operations using a memory manager.
// This function provides an interface for performing read or write operations
// on a file through memory management. It takes an allocated address, file
// descriptor, and operation type to determine the action to be executed.

// Args:
//    allocate_address - The starting address in memory where I/O will be
//    performed. file_descriptor - An integer representing the open file
//    descriptor on which
//                    operations are conducted.
//    operation - A value of type EsfMemoryManagerFileIoRequest indicating the
//              specific I/O operation (either read or write).
//    access_buff - A pointer to the buffer containing data to be written or
//                where data will be stored if reading.
//    access_size - The number of bytes to read from or write to the file.
//    result_size - A pointer to a size_t variable where the actual number of
//                bytes successfully processed will be stored.

// Returns:
//    Returns a PlErrCode indicating the success or failure status:
//    - kPlErrCodeOk: Operation completed successfully.
//    - Other values: Corresponding error codes in case of failure.
// """
STATIC PlErrCode EsfMemoryManagerFileIoAccessFunc(
    uint64_t allocate_address, int file_descriptor,
    EsfMemoryManagerFileIoRequest operation, void *access_buff,
    size_t access_size, size_t *result_size);

// """ EsfMemoryManagerFileIoFwriteFreadExec
// Executes File I/O operations using either fwrite or fread based on the
// specified operation. This function handles file input/output operations by
// dividing the total data size into chunks, allowing maximum access size as
// defined by MEMORY_MANAGER_ACCESS_MAX_SIZE. It performs the requested
// operation (either fwrite or fread) on each chunk until all the data is
// processed or an error occurs. The function also updates the file offset and
// retains information about processed size.

// Args:
//    allocate_address - A memory address where the operation will be performed.
//    Must not be zero. file_descriptor - A valid file descriptor for performing
//    file operations. operation - Specifies the type of operation:
//    kEsfMemoryManagerFileIoFwrite or kEsfMemoryManagerFileIoFread. param -
//    Pointer to the structure EsfMemoryManagerFileIoAccessParam, containing
//    buffer details and sizes. file_offset - Pointer to the file offset, which
//    will be updated during the operation.

// Returns:
//    Returns kEsfMemoryManagerResultSuccess if the operation was successful for
//    all chunks.
//      If parameters are invalid, returns kEsfMemoryManagerResultParamError.
//      If any file access error occurs, returns
//      kEsfMemoryManagerResultFileIoError.
// """
STATIC EsfMemoryManagerResult EsfMemoryManagerFileIoFwriteFreadExec(
    uint64_t allocate_address, int file_descriptor,
    EsfMemoryManagerFileIoRequest operation,
    EsfMemoryManagerFileIoAccessParam *param, off_t *file_offset);

// """ EsfMemoryManagerFileIoAccessParamCheck
// Checks the validity of parameters for file I/O operations in the
// EsfMemoryManager. This function verifies that the provided operation is
// either a read or write operation and checks if the parameters are valid. The
// function ensures that the `param` pointer, its buffer, and size are not null
// or zero, respectively.

// Args:
//    operation The file I/O operation to be performed. It should be either
//                  `kEsfMemoryManagerFileIoFwrite` or
//                  `kEsfMemoryManagerFileIoFread`.
//    param A pointer to an `EsfMemoryManagerFileIoAccessParam` structure
//    containing
//              the parameters for the file I/O operation. This includes a
//              buffer pointer, the size of the data, and a pointer to store the
//              result size.

// Returns:
//    Returns `kEsfMemoryManagerResultSuccess` if all parameters are valid.
//    Returns `kEsfMemoryManagerResultParamError` if any parameter is invalid.
// """
STATIC EsfMemoryManagerResult EsfMemoryManagerFileIoAccessParamCheck(
    EsfMemoryManagerFileIoRequest operation,
    EsfMemoryManagerFileIoAccessParam *param);

// """ EsfMemoryManagerFileIoFwriteFread
// Executes file I/O operations for reading or writing memory blocks.
// This function performs file input/output operations on memory blocks
// allocated at a given address. The operation can either be a read or write as
// specified by the `operation` parameter. It handles data in chunks, if
// necessary, based on predefined maximum access size limits and updates the
// file offset and the actual read/write sizes.

// Args:
//    handle - A valid EsfMemoryManagerHandle representing the memory management
//    operation - Defines the type of FileIO operation, either read or write.
//                It should be either kEsfMemoryManagerFileIoFwrite or
//                kEsfMemoryManagerFileIoFread.
//    param  - A pointer to a structure containing buffer information, size,
//                and other access parameters needed for the operation.

// Returns:
//    Returns an enumeration of type EsfMemoryManagerResult indicating the
//    result of the operation.
//      - `kEsfMemoryManagerResultParamError` if any input parameters are
//      invalid;
//      - `kEsfMemoryManagerResultFileIoError` if an error occurs during file
//      I/O operations.
// """
STATIC EsfMemoryManagerResult EsfMemoryManagerFileIoFwriteFread(
    EsfMemoryManagerHandle handle, EsfMemoryManagerFileIoRequest operation,
    EsfMemoryManagerFileIoAccessParam *param);

// """ EsfMemoryManagerFileIoIsMapSupport
// Check if map support is available for file I/O in the memory manager.
// This function determines whether the map function is supported in conjunction
// with FileIO operations for a given memory management handle. It checks
// various internal conditions and sets the parameter `param` to indicate the
// level of support.

// Args:
//    handle - A valid EsfMemoryManagerHandle representing the memory management
//    entity. param - Pointer to an EsfMemoryManagerMapSupport variable where
//    the support status will be stored. It must not be NULL.

// Returns:
//    EsfMemoryManagerResult
//     - Returns an enumeration value that indicates the success or failure of
//     the operation. If param is NULL, returns
//     kEsfMemoryManagerResultParamError.
// """
STATIC EsfMemoryManagerResult EsfMemoryManagerFileIoIsMapSupport(
    EsfMemoryManagerHandle handle, EsfMemoryManagerMapSupport *param);

// """ EsfMemoryManagerSetHandleInfo
// Sets the information for a given memory handle in the ESF Memory Manager.
// This function attempts to populate the EsfMemoryManagerHandleInfo structure
// with details associated with the specified handle. It checks if the handle
// exists and retrieves its memory management information when valid. If the
// handle does not exist or is invalid, corresponding error codes are returned.

// Args:
//    handle The EsfMemoryManagerHandle to set information for.
//    info Pointer to an EsfMemoryManagerHandleInfo struct where the information
//    will be stored.
//    handle - The memory manager handle whose information needs to be set.
//    info - A pointer to an EsfMemoryManagerHandleInfo structure where the
//    handle's information will be stored. Must not be NULL.

// Returns:
//    EsfMemoryManagerResult indicating the result of the operation. Can be one
//    of:
//     - kEsfMemoryManagerResultSuccess: Operation was successful.
//     - kEsfMemoryManagerResultParamError: Passed parameter is NULL.
//     - kEsfMemoryManagerResultOtherError: The handle entry does not exist or
//     other errors.

// The function first validates the input parameters. If the 'info' pointer is
// NULL, the function returns kEsfMemoryManagerResultParamError. It then
// initializes the info structure with default values and checks if the provided
// handle exists within the memory manager. If it doesn't, the function
// considers the handle as an "other area" and returns success. For handles that
// do exist, it further checks the target area and verifies the validity of the
// allocated address using specific checks depending on the type of memory
// (e.g., Large Heap or DMA). If validation passes and matches expected
// criteria, the relevant info details are populated and a success result is
// returned.
// """
STATIC EsfMemoryManagerResult EsfMemoryManagerSetHandleInfo(
    EsfMemoryManagerHandle handle, EsfMemoryManagerHandleInfo *info);

/****************************************************************************
 * Private Functions
 ****************************************************************************/
/* ------------------------------------------------------------------------ */
STATIC EsfMemoryManagerResult
EsfMemoryManagerAllocateWasm(const wasm_exec_env_t *exec_env, int32_t size,
                             EsfMemoryManagerHandle *user_handle) {
  // check parameters.
  if ((size <= 0) || (user_handle == (EsfMemoryManagerHandle *)NULL)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:[%s] parameter error - size=%d, user_handle=%p",
                     __FILE__, __LINE__, __func__, size, user_handle);
    return kEsfMemoryManagerResultParamError;
  }
  *user_handle = (EsfMemoryManagerHandle)0;

  if (!EsfMemoryManagerValidateWasmModuleInstance(exec_env)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:[%s] invalid wasm module instance", __FILE__,
                     __LINE__, __func__);
    return kEsfMemoryManagerResultParamError;
  }
  wasm_module_inst_t module_inst =
      wasm_runtime_get_module_inst((wasm_exec_env_t)*exec_env);
  *user_handle = (EsfMemoryManagerHandle)wasm_runtime_module_malloc(
                     module_inst, size, NULL) &
                 MEMORY_MANAGER_OFFSET_MASK;
  if (*user_handle == (EsfMemoryManagerHandle)0) {
    // Elog (telemetry) output
    ESF_MEMORY_MANAGER_ELOG_ERROR(ESF_ERROR_MEMORY_ALLOCATE);
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] failed(%d)", __FILE__,
                     __LINE__, __func__,
                     kEsfMemoryManagerResultAllocationError);
    return kEsfMemoryManagerResultAllocationError;
  }

  return kEsfMemoryManagerResultSuccess;
}
/* ------------------------------------------------------------------------ */
STATIC EsfMemoryManagerResult EsfMemoryManagerGenerateMemoryInfo(
    EsfMemoryManagerTargetArea target_area, int32_t size,
    EsfMemoryManagerHandle *user_handle) {
  EsfMemoryManagerResult ret = kEsfMemoryManagerResultSuccess;

  // check parameters.
  if ((size <= 0) || (user_handle == (EsfMemoryManagerHandle *)NULL)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:[%s] parameter error - size=%d, user_handle=%p",
                     __FILE__, __LINE__, __func__, size, user_handle);
    return kEsfMemoryManagerResultParamError;
  }
  *user_handle = (EsfMemoryManagerHandle)0;

  struct EsfMemoryManagerHandleInternal *memory_handle =
      (struct EsfMemoryManagerHandleInternal *)NULL;
  // allocate memory management information
  memory_handle = (struct EsfMemoryManagerHandleInternal *)malloc(
      sizeof(struct EsfMemoryManagerHandleInternal));
  if (memory_handle == (struct EsfMemoryManagerHandleInternal *)NULL) {
    ret = kEsfMemoryManagerResultAllocationError;
    // Elog (telemetry) output
    ESF_MEMORY_MANAGER_ELOG_ERROR(ESF_ERROR_MEMORY_ALLOCATE);
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] failed(%d)", __FILE__,
                     __LINE__, __func__, ret);
    goto error_exit;
  }
  // initial setting of memory management information
  memory_handle->link_info = 0;
  memory_handle->target_area = target_area;
  memory_handle->allocate_address = (uint64_t)(uintptr_t)NULL;
  memory_handle->allocate_size = size;
  memory_handle->map_address = MEMORY_MANAGER_UNMAP;
  memory_handle->map_count = 0;
  memory_handle->file_descriptor = -1;
  // initialize map management information list
  SLIST_INIT(&(memory_handle->map_info_list));
  // memory allocation
  switch (target_area) {
    case kEsfMemoryManagerTargetLargeHeap:
      memory_handle->allocate_address =
          (uint64_t)(uintptr_t)PlLheapAlloc((uint32_t)size);
      break;
    case kEsfMemoryManagerTargetDma:
      memory_handle->allocate_address =
          (uint64_t)(uintptr_t)PlDmaMemAlloc((uint32_t)size);
      break;
    case kEsfMemoryManagerTargetWasmHeap:
    default:
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:[%s] parameter error - unsupported target area, "
                       "target_area=%d",
                       __FILE__, __LINE__, __func__, target_area);
      ret = kEsfMemoryManagerResultParamError;
      goto error_exit;
  }
  if (memory_handle->allocate_address == (uint64_t)0) {
    ret = kEsfMemoryManagerResultAllocationError;
    // Elog (telemetry) output
    ESF_MEMORY_MANAGER_ELOG_ERROR(ESF_ERROR_MEMORY_ALLOCATE);
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] failed(%d)", __FILE__,
                     __LINE__, __func__, ret);
    goto error_exit;
  }
  // generate a memory operation handle ID for new allocated memory
  int8_t handle_id = EsfMemoryManagerGetFreeHandleId();
  if (handle_id < 0) {
    ret = kEsfMemoryManagerResultAllocationError;
    // Elog (telemetry) output
    ESF_MEMORY_MANAGER_ELOG_ERROR(ESF_ERROR_MEMORY_ALLOCATE);
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] failed(%d)", __FILE__,
                     __LINE__, __func__, ret);
    goto error_exit;
  }
  *user_handle = ((uint32_t)handle_id << 25);
  // linking memory operation handles to memory management information
  memory_handle->link_info = *user_handle;
  // register memory management information in the memory information
  // management list
  SLIST_INSERT_HEAD(&s_handle_info_list, memory_handle, next);
  return ret;

error_exit:
  // clean up memory management information
  EsfMemoryManagerCleanUpMemoryInfo(memory_handle);
  return ret;
}
/* ------------------------------------------------------------------------ */
STATIC EsfMemoryManagerResult EsfMemoryManagerFreeWasm(
    EsfMemoryManagerHandle handle, const wasm_exec_env_t *exec_env) {
  // check parameters.
  if ((MEMORY_MANAGER_HANDLE_ID(handle) != 0) ||
      (MEMORY_MANAGER_HANDLE_OFFSET(handle) == 0)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:[%s] parameter error - handle=0x%08x, "
                     "handle_id=%d, offset=0x%08x",
                     __FILE__, __LINE__, __func__, handle,
                     MEMORY_MANAGER_HANDLE_ID(handle),
                     MEMORY_MANAGER_HANDLE_OFFSET(handle));
    return kEsfMemoryManagerResultParamError;
  }
  if (!EsfMemoryManagerValidateWasmModuleInstance(exec_env)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:[%s] parameter error - invalid wasm module "
                     "instance, exec_env=%p",
                     __FILE__, __LINE__, __func__, exec_env);
    return kEsfMemoryManagerResultParamError;
  }

  wasm_module_inst_t module_inst =
      wasm_runtime_get_module_inst((wasm_exec_env_t)*exec_env);
  wasm_runtime_module_free(module_inst,
                           (uint32_t)MEMORY_MANAGER_HANDLE_OFFSET(handle));

  return kEsfMemoryManagerResultSuccess;
}
/* ------------------------------------------------------------------------- */
STATIC EsfMemoryManagerResult
EsfMemoryManagerDiscardMemoryInfo(EsfMemoryManagerHandle handle) {
  EsfMemoryManagerResult ret = kEsfMemoryManagerResultSuccess;
  // memory information existence check
  if (!EsfMemoryManagerHandleExists(handle)) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:[%s] parameter error - handle does not exist, handle=0x%08x",
        __FILE__, __LINE__, __func__, handle);
    ret = kEsfMemoryManagerResultParamError;
    goto error_exit;
  }
  // selecting memory information from the memory management information list
  struct EsfMemoryManagerHandleInternal *entry =
      (struct EsfMemoryManagerHandleInternal *)NULL;
  struct EsfMemoryManagerHandleInternal *temp =
      (struct EsfMemoryManagerHandleInternal *)NULL;
  ret = kEsfMemoryManagerResultParamError;
  SLIST_FOREACH_SAFE(entry, &s_handle_info_list, next, temp) {
    if ((entry != (struct EsfMemoryManagerHandleInternal *)NULL) &&
        (MEMORY_MANAGER_HANDLE_ID(entry->link_info) ==
         MEMORY_MANAGER_HANDLE_ID(handle))) {
      // unmap error,close error
      if ((entry->map_address != MEMORY_MANAGER_UNMAP) ||
          (entry->file_descriptor >= 0)) {
        WRITE_DLOG_ERROR(
            MODULE_ID_SYSTEM,
            "%s-%d:[%s] operation error - memory still mapped or file open, "
            "handle=0x%08x, map_address=0x%016" PRIx64 ", fd=%d",
            __FILE__, __LINE__, __func__, handle, entry->map_address,
            entry->file_descriptor);
        ret = kEsfMemoryManagerResultOperationError;
        goto error_exit;
      }
      // memory free
      switch (entry->target_area) {
        case kEsfMemoryManagerTargetLargeHeap:
          if (PlLheapFree((PlLheapHandle)(uintptr_t)entry->allocate_address)) {
            WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                             "%s-%d:[%s] other error - large heap free failed, "
                             "handle=0x%08x, address=0x%lx",
                             __FILE__, __LINE__, __func__, handle,
                             (unsigned long)entry->allocate_address);
            ret = kEsfMemoryManagerResultOtherError;
            goto error_exit;
          }
          break;
        case kEsfMemoryManagerTargetDma:
          if (PlDmaMemFree(
                  (PlDmaMemHandle)(uintptr_t)entry->allocate_address)) {
            WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                             "%s-%d:[%s] other error - DMA memory free failed, "
                             "handle=0x%08x, address=0x%lx",
                             __FILE__, __LINE__, __func__, handle,
                             (unsigned long)entry->allocate_address);
            ret = kEsfMemoryManagerResultOtherError;
            goto error_exit;
          }
          break;
        case kEsfMemoryManagerTargetWasmHeap:
        default:
          WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                           "%s-%d:[%s] parameter error - unsupported target "
                           "area, target_area=%d",
                           __FILE__, __LINE__, __func__, entry->target_area);
          ret = kEsfMemoryManagerResultParamError;
          goto error_exit;
      }
      // discarding memory management information
      SLIST_REMOVE(&s_handle_info_list, entry, EsfMemoryManagerHandleInternal,
                   next);
      free(entry);
      entry = (struct EsfMemoryManagerHandleInternal *)NULL;
      // delete the memory operation handle ID.
      EsfMemoryManagerReturnUsedHandleId(MEMORY_MANAGER_HANDLE_ID(handle));
      ret = kEsfMemoryManagerResultSuccess;
      break;
    }
  }

error_exit:
  return ret;
}
/* ------------------------------------------------------------------------ */
STATIC EsfMemoryManagerResult
EsfMemoryManagerMapWasm(EsfMemoryManagerHandle handle, int32_t size,
                        const wasm_exec_env_t *exec_env, void **address) {
  void *vaddr = (void *)NULL;
  PlErrCode err_code = kPlErrCodeOk;
  // check parameters.
  if ((address == (void **)NULL) || (size <= 0)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:[%s] parameter error - address=%p, size=%d",
                     __FILE__, __LINE__, __func__, address, size);
    return kEsfMemoryManagerResultParamError;
  }
  // map address initialize
  *address = (void *)NULL;

  // user handle(handle_id = 0): when specified
  //  if the handle ID is 0, only the physical mapping process for the
  //  address specified by the handle offset is performed and the virtual
  //  address is returned as the mapping address.
  if ((MEMORY_MANAGER_HANDLE_ID(handle) != 0) ||
      (MEMORY_MANAGER_HANDLE_OFFSET(handle) == 0)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:[%s] parameter error - handle=0x%08x, "
                     "handle_id=%d, offset=0x%08x",
                     __FILE__, __LINE__, __func__, handle,
                     MEMORY_MANAGER_HANDLE_ID(handle),
                     MEMORY_MANAGER_HANDLE_OFFSET(handle));
    return kEsfMemoryManagerResultParamError;
  }
  if (!EsfMemoryManagerValidateWasmModuleInstance(exec_env)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:[%s] parameter error - invalid wasm module "
                     "instance, exec_env=%p",
                     __FILE__, __LINE__, __func__, exec_env);
    return kEsfMemoryManagerResultParamError;
  }
  wasm_module_inst_t module_inst =
      wasm_runtime_get_module_inst((wasm_exec_env_t)*exec_env);
  if (!wasm_runtime_validate_app_addr(
          module_inst, MEMORY_MANAGER_HANDLE_OFFSET(handle), (uint32_t)size)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:[%s] parameter error - invalid wasm app address, "
                     "handle=0x%08x, offset=0x%08x, size=%d",
                     __FILE__, __LINE__, __func__, handle,
                     MEMORY_MANAGER_HANDLE_OFFSET(handle), size);
    return kEsfMemoryManagerResultParamError;
  }
  void *native_address = wasm_runtime_addr_app_to_native(
      module_inst, MEMORY_MANAGER_HANDLE_OFFSET(handle));
  err_code = PlAppmemMap(native_address, (uint32_t)size, (void **)&vaddr);
  if ((err_code != kPlErrCodeOk) || (vaddr == (void *)NULL)) {
    // Elog (telemetry) output
    ESF_MEMORY_MANAGER_ELOG_ERROR(ESF_ERROR_MEMORY_MAP);
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] failed(%d)", __FILE__,
                     __LINE__, __func__, kEsfMemoryManagerResultMapError);
    return kEsfMemoryManagerResultMapError;
  }
  // map address
  *address = vaddr;

  return kEsfMemoryManagerResultSuccess;
}
/* ------------------------------------------------------------------------ */
STATIC EsfMemoryManagerResult EsfMemoryManagerMapMemoryInfo(
    EsfMemoryManagerHandle handle, int32_t size, void **address) {
  EsfMemoryManagerResult ret = kEsfMemoryManagerResultSuccess;
  // check parameters.
  if ((address == (void **)NULL) || (size <= 0)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:[%s] parameter error - address=%p, size=%d",
                     __FILE__, __LINE__, __func__, address, size);
    return kEsfMemoryManagerResultParamError;
  }
  // map address initialize
  *address = (void *)NULL;

  // memory operation handle (handle_id = 1-127)
  if (!EsfMemoryManagerHandleExists(handle)) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:[%s] parameter error - handle does not exist, handle=0x%08x",
        __FILE__, __LINE__, __func__, handle);
    return kEsfMemoryManagerResultParamError;
  }
  // selecting memory information from the memory management information list
  struct EsfMemoryManagerHandleInternal *entry =
      (struct EsfMemoryManagerHandleInternal *)NULL;
  struct EsfMemoryManagerHandleInternal *temp =
      (struct EsfMemoryManagerHandleInternal *)NULL;
  SLIST_FOREACH_SAFE(entry, &s_handle_info_list, next, temp) {
    if ((entry != (struct EsfMemoryManagerHandleInternal *)NULL) &&
        (MEMORY_MANAGER_HANDLE_ID(entry->link_info) ==
         MEMORY_MANAGER_HANDLE_ID(handle))) {
      // map area check
      uint32_t area_size = 0;
      if (entry->target_area == kEsfMemoryManagerTargetWasmHeap) {
        WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                         "%s-%d:[%s] parameter error - wasm heap not supported "
                         "for mapping, handle=0x%08x",
                         __FILE__, __LINE__, __func__, handle);
        return kEsfMemoryManagerResultParamError;
      }
      if ((uint32_t)entry->allocate_size <
          MEMORY_MANAGER_HANDLE_OFFSET(handle)) {
        WRITE_DLOG_ERROR(
            MODULE_ID_SYSTEM,
            "%s-%d:[%s] parameter error - offset exceeds allocated size, "
            "handle=0x%08x, allocate_size=%d, offset=0x%08x",
            __FILE__, __LINE__, __func__, handle, entry->allocate_size,
            MEMORY_MANAGER_HANDLE_OFFSET(handle));
        return kEsfMemoryManagerResultParamError;
      }
      area_size = (uint32_t)entry->allocate_size -
                  MEMORY_MANAGER_HANDLE_OFFSET(handle);
      if (area_size < (uint32_t)size) {
        WRITE_DLOG_ERROR(
            MODULE_ID_SYSTEM,
            "%s-%d:[%s] parameter error - requested size exceeds available "
            "area, handle=0x%08x, area_size=%u, requested_size=%d",
            __FILE__, __LINE__, __func__, handle, area_size, size);
        return kEsfMemoryManagerResultParamError;
      }
      // virtual Address Map processing
      //  when mapping for the first time, execute Osal's MapAPI. *Updates
      //  the map address for the entire area (virtual address of the
      //  beginning of the area).
      ret = EsfMemoryManagerMapMemory(entry);
      if (ret != kEsfMemoryManagerResultSuccess) {
        WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                         "%s-%d:[%s] map memory failed, handle=0x%08x, ret=%d",
                         __FILE__, __LINE__, __func__, handle, ret);
        return ret;
      }
      // map management information list unregistered check
      if (!EsfMemoryManagerMapInfoExists(handle, entry)) {
        // allocate map management information for new registrations
        struct EsfMemoryManagerHandleAddressOffset *new_list = NULL;
        new_list = malloc(sizeof(struct EsfMemoryManagerHandleAddressOffset));
        if (new_list == NULL) {
          // Elog (telemetry) output
          ESF_MEMORY_MANAGER_ELOG_ERROR(ESF_ERROR_MEMORY_MAP);
          WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] failed(%d)", __FILE__,
                           __LINE__, __func__, kEsfMemoryManagerResultMapError);
          return kEsfMemoryManagerResultMapError;
        }
        new_list->address_offset = MEMORY_MANAGER_HANDLE_OFFSET(handle);
        new_list->file_descriptor = -1;
        // register map management information in the map management
        // information list
        SLIST_INSERT_HEAD(&(entry->map_info_list), new_list, next);
        // map registration count up
        entry->map_count++;
      }
      // map address: thew offset value specified by the memory operation
      // handle + virtual address
      *address = (void *)(uintptr_t)(entry->map_address +
                                     MEMORY_MANAGER_HANDLE_OFFSET(handle));
      // break;
      return kEsfMemoryManagerResultSuccess;
    }
  }
  WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                   "%s-%d:[%s] parameter error - handle not found in memory "
                   "info list, handle=0x%08x",
                   __FILE__, __LINE__, __func__, handle);
  return kEsfMemoryManagerResultParamError;
}
/* ------------------------------------------------------------------------ */
STATIC EsfMemoryManagerResult
EsfMemoryManagerMapMemory(struct EsfMemoryManagerHandleInternal *entry) {
  uint64_t *vaddr = (void *)NULL;
  PlErrCode err_code = kPlErrCodeOk;
  // check parameters.
  if (entry == (struct EsfMemoryManagerHandleInternal *)NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:[%s] parameter error - entry is NULL", __FILE__,
                     __LINE__, __func__);
    return kEsfMemoryManagerResultParamError;
  }
  // virtual Address Map processing
  //  when mapping for the first time, execute Osal's MapAPI. *Updates
  //  the map address for the entire area (virtual address of the
  //  beginning of the area).
  if (entry->map_address == MEMORY_MANAGER_UNMAP) {
    // memory map
    switch (entry->target_area) {
      case kEsfMemoryManagerTargetLargeHeap:
        if (!PlLheapIsValid(
                (PlLheapHandle)(uintptr_t)entry->allocate_address)) {
          WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                           "%s-%d:[%s] parameter error - invalid large heap "
                           "handle, handle=0x%lx",
                           __FILE__, __LINE__, __func__,
                           (unsigned long)entry->allocate_address);
          return kEsfMemoryManagerResultParamError;
        }
        err_code = PlLheapMap((PlLheapHandle)(uintptr_t)entry->allocate_address,
                              (void **)&vaddr);
        break;
      case kEsfMemoryManagerTargetDma:
        if (!PlDmaMemIsValid(
                (PlDmaMemHandle)(uintptr_t)entry->allocate_address)) {
          WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                           "%s-%d:[%s] parameter error - invalid DMA memory "
                           "handle, handle=0x%lx",
                           __FILE__, __LINE__, __func__,
                           (unsigned long)entry->allocate_address);
          return kEsfMemoryManagerResultParamError;
        }
        err_code =
            PlDmaMemMap((PlDmaMemHandle)(uintptr_t)entry->allocate_address,
                        (void **)&vaddr);
        break;
      case kEsfMemoryManagerTargetWasmHeap:
      default:
        WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                         "%s-%d:[%s] parameter error - unsupported target "
                         "area, target_area=%d",
                         __FILE__, __LINE__, __func__, entry->target_area);
        return kEsfMemoryManagerResultParamError;
    }
    if ((err_code != kPlErrCodeOk) || (vaddr == (void *)NULL)) {
      // Elog (telemetry) output
      ESF_MEMORY_MANAGER_ELOG_ERROR(ESF_ERROR_MEMORY_MAP);
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] failed(%d)", __FILE__,
                       __LINE__, __func__, kEsfMemoryManagerResultMapError);
      return kEsfMemoryManagerResultMapError;
    }
    // registering a map address (virtual address)
    entry->map_address = (uint64_t)(uintptr_t)vaddr;
  }
  return kEsfMemoryManagerResultSuccess;
}
/* ------------------------------------------------------------------------ */
STATIC EsfMemoryManagerResult
EsfMemoryManagerUnmapWasm(EsfMemoryManagerHandle handle, void **address) {
  PlErrCode err_code = kPlErrCodeOk;
  // check parameters.
  if ((address == (void **)NULL) || (*address == (void *)NULL)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:[%s] parameter error - address=%p, *address=%p",
                     __FILE__, __LINE__, __func__, address,
                     address ? *address : NULL);
    return kEsfMemoryManagerResultParamError;
  }
  // user handle(handle_id = 0): when specified
  if ((MEMORY_MANAGER_HANDLE_ID(handle) != 0) ||
      (MEMORY_MANAGER_HANDLE_OFFSET(handle) == 0)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:[%s] parameter error - invalid handle, "
                     "handle=0x%08x, handle_id=%d, offset=0x%08x",
                     __FILE__, __LINE__, __func__, handle,
                     MEMORY_MANAGER_HANDLE_ID(handle),
                     MEMORY_MANAGER_HANDLE_OFFSET(handle));
    return kEsfMemoryManagerResultParamError;
  }
  err_code = PlAppmemUnmap(*address);
  if (err_code != kPlErrCodeOk) {
    // No error is returned to the higher level
  }
  return kEsfMemoryManagerResultSuccess;
}
/* ------------------------------------------------------------------------ */
STATIC EsfMemoryManagerResult
EsfMemoryManagerUnmapMemoryInfo(EsfMemoryManagerHandle handle) {
  EsfMemoryManagerResult ret = kEsfMemoryManagerResultSuccess;
  // memory operation handle (handle_id = 1-127)
  if (!EsfMemoryManagerHandleExists(handle)) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:[%s] parameter error - handle does not exist, handle=0x%08x",
        __FILE__, __LINE__, __func__, handle);
    return kEsfMemoryManagerResultParamError;
  }
  // selecting memory information from the memory management information list
  struct EsfMemoryManagerHandleInternal *entry =
      (struct EsfMemoryManagerHandleInternal *)NULL;
  struct EsfMemoryManagerHandleInternal *temp =
      (struct EsfMemoryManagerHandleInternal *)NULL;
  SLIST_FOREACH_SAFE(entry, &s_handle_info_list, next, temp) {
    if ((entry != (struct EsfMemoryManagerHandleInternal *)NULL) &&
        (MEMORY_MANAGER_HANDLE_ID(entry->link_info) ==
         MEMORY_MANAGER_HANDLE_ID(handle))) {
      if (entry->map_address == MEMORY_MANAGER_UNMAP) {
        WRITE_DLOG_ERROR(
            MODULE_ID_SYSTEM,
            "%s-%d:[%s] operation error - memory not mapped, handle=0x%08x",
            __FILE__, __LINE__, __func__, handle);
        return kEsfMemoryManagerResultOperationError;
      }
      if (!EsfMemoryManagerMapInfoExists(handle, entry)) {
        WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                         "%s-%d:[%s] parameter error - map info does not "
                         "exist, handle=0x%08x",
                         __FILE__, __LINE__, __func__, handle);
        return kEsfMemoryManagerResultParamError;
      }
      // virtual Address Unmap processing
      //  delete the corresponding map management information from the map
      //  management information list in the memory management information
      struct EsfMemoryManagerHandleAddressOffset *map_entry =
          (struct EsfMemoryManagerHandleAddressOffset *)NULL;
      struct EsfMemoryManagerHandleAddressOffset *map_temp =
          (struct EsfMemoryManagerHandleAddressOffset *)NULL;
      SLIST_FOREACH_SAFE(map_entry, &(entry->map_info_list), next, map_temp) {
        if (map_entry->address_offset == MEMORY_MANAGER_HANDLE_OFFSET(handle)) {
          SLIST_REMOVE(&(entry->map_info_list), map_entry,
                       EsfMemoryManagerHandleAddressOffset, next);
          free(map_entry);
          break;
        }
      }
      // map registration count down
      if (entry->map_count > 0) {
        entry->map_count--;
      }
      // virtual Address unmap processing
      //  when map registration count is 0, execute Osal's UnAPI. *Clear the
      //  map addresses of the entire area (All "ff")
      ret = EsfMemoryManagerUnmapMemory(entry);
      if (ret != kEsfMemoryManagerResultSuccess) {
        // No error is returned to the higher level
      }
      return kEsfMemoryManagerResultSuccess;
    }
  }
  WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                   "%s-%d:[%s] parameter error - handle not found in memory "
                   "info list, handle=0x%08x",
                   __FILE__, __LINE__, __func__, handle);
  return kEsfMemoryManagerResultParamError;
}
/* ------------------------------------------------------------------------ */
STATIC EsfMemoryManagerResult
EsfMemoryManagerUnmapMemory(struct EsfMemoryManagerHandleInternal *entry) {
  PlErrCode err_code = kPlErrCodeOk;
  // check parameters.
  if (entry == (struct EsfMemoryManagerHandleInternal *)NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:[%s] parameter error - entry is NULL", __FILE__,
                     __LINE__, __func__);
    return kEsfMemoryManagerResultParamError;
  }
  // virtual Address unmap processing
  //  when map registration count is 0, execute Osal's UnAPI. *Clear the
  //  map addresses of the entire area (All "ff")
  if (entry->map_count == 0) {
    // memory unmap
    switch (entry->target_area) {
      case kEsfMemoryManagerTargetLargeHeap:
        err_code = PlLheapUnmap((void *)(uintptr_t)entry->map_address);
        break;
      case kEsfMemoryManagerTargetDma:
        err_code = PlDmaMemUnmap((void *)(uintptr_t)entry->map_address);
        break;
      case kEsfMemoryManagerTargetWasmHeap:
      default:
        WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                         "%s-%d:[%s] parameter error - unsupported target "
                         "area, target_area=%d",
                         __FILE__, __LINE__, __func__, entry->target_area);
        return kEsfMemoryManagerResultParamError;
    }
    if (err_code != kPlErrCodeOk) {
      // No error is returned to the higher level
    }
    entry->map_address = MEMORY_MANAGER_UNMAP;
  }
  return kEsfMemoryManagerResultSuccess;
}
/* ------------------------------------------------------------------------ */
STATIC void EsfMemoryManagerClearHandleId(void) {
  // s_handle_id_map initialize(clear)
  s_handle_id_map[0] = 0;
  s_handle_id_map[1] = 0;
  s_handle_id_map[2] = 0;
  s_handle_id_map[3] = 0;
  return;
}
/* ------------------------------------------------------------------------ */
STATIC int8_t EsfMemoryManagerGetFreeHandleId(void) {
  uint32_t i = 0;
  uint32_t index = 0;
  uint32_t bit_pos = 0;
  int8_t free_id = -1;  // id initialize(No free ID)

  // search for available IDs
  for (i = 1; i <= MEMORY_MANAGER_MAX_MAPS; i++) {
    index = i / 32;    // s_handle_id_map index calculation
    bit_pos = i % 32;  // s_handle_id_map bit position set
    if ((s_handle_id_map[index] & ((uint32_t)(1 << bit_pos))) == 0) {
      // calculate ID from s_handle_id_map bit position
      s_handle_id_map[index] |= ((uint32_t)(1 << bit_pos));
      free_id = (int8_t)(32 * index + bit_pos);
      break;
    }
  }
  return free_id;
}
/* ------------------------------------------------------------------------ */
STATIC void EsfMemoryManagerReturnUsedHandleId(int8_t used_id) {
  int8_t index = 0;
  int8_t bit_pos = 0;
  uint32_t mask_value = 0;
  index = used_id / 32;    // s_handle_id_map index calculation
  bit_pos = used_id % 32;  // s_handle_id_map bit position set
  mask_value = ~((uint32_t)(1 << bit_pos));
  s_handle_id_map[index] &= mask_value;  // s_handle_id_map used clear
  return;
}
/* ------------------------------------------------------------------------ */
STATIC bool EsfMemoryManagerHandleExists(EsfMemoryManagerHandle handle) {
  // Search for the corresponding handle ID from the memory management
  // information list
  struct EsfMemoryManagerHandleInternal *entry =
      (struct EsfMemoryManagerHandleInternal *)NULL;
  struct EsfMemoryManagerHandleInternal *temp =
      (struct EsfMemoryManagerHandleInternal *)NULL;
  SLIST_FOREACH_SAFE(entry, &s_handle_info_list, next, temp) {
    if (MEMORY_MANAGER_HANDLE_ID(entry->link_info) ==
        MEMORY_MANAGER_HANDLE_ID(handle)) {
      // Handle ID detection
      return true;
    }
  }
  // Handle ID not found
  return false;
}
/* ------------------------------------------------------------------------ */
STATIC bool EsfMemoryManagerIsHandleExist(
    EsfMemoryManagerHandle handle,
    struct EsfMemoryManagerHandleInternal **entry) {
  if (entry == (struct EsfMemoryManagerHandleInternal **)NULL) return false;
  // Search for the corresponding handle ID from the memory management
  // information list
  struct EsfMemoryManagerHandleInternal *entry_temp =
      (struct EsfMemoryManagerHandleInternal *)NULL;
  struct EsfMemoryManagerHandleInternal *temp =
      (struct EsfMemoryManagerHandleInternal *)NULL;
  SLIST_FOREACH_SAFE(entry_temp, &s_handle_info_list, next, temp) {
    if (MEMORY_MANAGER_HANDLE_ID(entry_temp->link_info) ==
        MEMORY_MANAGER_HANDLE_ID(handle)) {
      // Handle ID detection
      *entry = entry_temp;
      return true;
    }
  }
  // Handle ID not found
  *entry = entry_temp;
  return false;
}
/* ------------------------------------------------------------------------ */
STATIC bool EsfMemoryManagerMapInfoExists(
    EsfMemoryManagerHandle handle,
    const struct EsfMemoryManagerHandleInternal *memory_handle) {
  // Search for the corresponding map information from the map management
  // information list.
  struct EsfMemoryManagerHandleAddressOffset *entry =
      (struct EsfMemoryManagerHandleAddressOffset *)NULL;
  struct EsfMemoryManagerHandleAddressOffset *temp =
      (struct EsfMemoryManagerHandleAddressOffset *)NULL;
  SLIST_FOREACH_SAFE(entry, &(memory_handle->map_info_list), next, temp) {
    if (entry->address_offset == MEMORY_MANAGER_HANDLE_OFFSET(handle)) {
      // Map information detection
      return true;
    }
  }
  // Map information not found
  return false;
}
/* ------------------------------------------------------------------------ */
STATIC bool EsfMemoryManagerIsMapInfoExist(
    EsfMemoryManagerHandle handle,
    const struct EsfMemoryManagerHandleInternal *memory_handle,
    struct EsfMemoryManagerHandleAddressOffset **map_info) {
  if (map_info == (struct EsfMemoryManagerHandleAddressOffset **)NULL)
    return false;
  // Search for the corresponding map information from the map management
  // information list.
  struct EsfMemoryManagerHandleAddressOffset *entry =
      (struct EsfMemoryManagerHandleAddressOffset *)NULL;
  struct EsfMemoryManagerHandleAddressOffset *temp =
      (struct EsfMemoryManagerHandleAddressOffset *)NULL;
  SLIST_FOREACH_SAFE(entry, &(memory_handle->map_info_list), next, temp) {
    if (entry->address_offset == MEMORY_MANAGER_HANDLE_OFFSET(handle)) {
      // Map information detection
      *map_info = entry;
      return true;
    }
  }
  // Map information not found
  *map_info = entry;
  return false;
}
/* ------------------------------------------------------------------------ */
STATIC void EsfMemoryManagerCleanUpMemoryInfo(
    struct EsfMemoryManagerHandleInternal *memory_handle) {
  PlErrCode err_code = kPlErrCodeOk;
  if (memory_handle != (struct EsfMemoryManagerHandleInternal *)NULL) {
    if (memory_handle->allocate_address != (uint64_t)0) {
      switch (memory_handle->target_area) {
        case kEsfMemoryManagerTargetLargeHeap:
          err_code = PlLheapFree(
              (PlLheapHandle)(uintptr_t)memory_handle->allocate_address);
          break;
        case kEsfMemoryManagerTargetDma:
          err_code = PlDmaMemFree(
              (PlDmaMemHandle)(uintptr_t)memory_handle->allocate_address);
          break;
        case kEsfMemoryManagerTargetWasmHeap:
        default:
          break;
      }
      if (err_code != kPlErrCodeOk) {
        // No error is returned to the higher level
      }
    }
    free(memory_handle);
    memory_handle = (struct EsfMemoryManagerHandleInternal *)NULL;
  }
  return;
}
/* ------------------------------------------------------------------------ */
STATIC void EsfMemoryManagerCleanUpRemainResource(
    struct EsfMemoryManagerHandleInternal *entry) {
  PlErrCode err_code = kPlErrCodeOk;
  // clean up memory map
  if (entry->map_address != MEMORY_MANAGER_UNMAP) {
    // memory unmap
    switch (entry->target_area) {
      case kEsfMemoryManagerTargetLargeHeap:
        err_code = PlLheapUnmap((void *)(uintptr_t)entry->map_address);
        break;
      case kEsfMemoryManagerTargetDma:
        err_code = PlDmaMemUnmap((void *)(uintptr_t)entry->map_address);
        break;
      case kEsfMemoryManagerTargetWasmHeap:
      default:
        break;
    }
    if (err_code != kPlErrCodeOk) {
      // No error is returned to the higher level
    }
    entry->map_address = MEMORY_MANAGER_UNMAP;
  } else if (entry->file_descriptor >= 0) {
    // clean up file descriptors
    if (entry->target_area == kEsfMemoryManagerTargetLargeHeap) {
      // file descriptor close
      err_code =
          PlLheapFclose((PlLheapHandle)(uintptr_t)entry->allocate_address,
                        entry->file_descriptor);
      if (err_code != kPlErrCodeOk) {
        // No error is returned to the higher level
      }
    }
  }
  return;
}
/* ------------------------------------------------------------------------ */
STATIC EsfMemoryManagerResult EsfMemoryManagerCleanUpAllMemoryInfo(void) {
  // finalizing the linked list of memory management information.
  struct EsfMemoryManagerHandleInternal *entry =
      (struct EsfMemoryManagerHandleInternal *)NULL;
  struct EsfMemoryManagerHandleInternal *temp =
      (struct EsfMemoryManagerHandleInternal *)NULL;
  SLIST_FOREACH_SAFE(entry, &s_handle_info_list, next, temp) {
    if (entry != (struct EsfMemoryManagerHandleInternal *)NULL) {
      // Cleaning up remaining resources (memory map & file descriptors )
      EsfMemoryManagerCleanUpRemainResource(entry);
      // cleaning up the map management information list
      //  Check the handle link list and clean up if there are any remaining
      //  handles Does not unmap or free remaining memory
      struct EsfMemoryManagerHandleAddressOffset *map_entry =
          (struct EsfMemoryManagerHandleAddressOffset *)NULL;
      struct EsfMemoryManagerHandleAddressOffset *map_temp =
          (struct EsfMemoryManagerHandleAddressOffset *)NULL;
      SLIST_FOREACH_SAFE(map_entry, &(entry->map_info_list), next, map_temp) {
        // discarding memory management information
        if (map_entry->file_descriptor >= 0) {
          pthread_mutex_destroy(&(map_entry->handle_mutex));
        }
        SLIST_REMOVE(&(entry->map_info_list), map_entry,
                     EsfMemoryManagerHandleAddressOffset, next);
        free(map_entry);
      }
      SLIST_REMOVE(&s_handle_info_list, entry, EsfMemoryManagerHandleInternal,
                   next);
      // memory free
      EsfMemoryManagerCleanUpMemoryInfo(entry);
    }
  }
  return kEsfMemoryManagerResultSuccess;
}
/* ------------------------------------------------------------------------ */
STATIC bool EsfMemoryManagerValidateWasmModuleInstance(
    const wasm_exec_env_t *exec_env) {
  if (exec_env == (const wasm_exec_env_t *)NULL) {
    return false;
  }
  if ((wasm_exec_env_t)*exec_env == (wasm_exec_env_t)NULL) {
    return false;
  }
  return true;
}
/* ------------------------------------------------------------------------- */
STATIC EsfMemoryManagerResult EsfMemoryManagerFileIoPreCheck(
    EsfMemoryManagerTargetArea target_area, uint64_t allocate_address) {
  EsfMemoryManagerResult ret = kEsfMemoryManagerResultSuccess;
  // Pre-check before FileIO operation
  switch (target_area) {
    case kEsfMemoryManagerTargetLargeHeap:
      // Map function support check
      if (!PlLheapIsMapSupport((PlLheapHandle)(uintptr_t)allocate_address)) {
        // Performing FileIO operations
        ret = kEsfMemoryManagerResultSuccess;
      } else {
        // Map function support (FileIO function not supported)
        ret = kEsfMemoryManagerResultNotSupport;
      }
      break;
    case kEsfMemoryManagerTargetDma:
    case kEsfMemoryManagerTargetWasmHeap:
      // FileIO function not supported
      ret = kEsfMemoryManagerResultNotSupport;
      break;
    default:
      ret = kEsfMemoryManagerResultParamError;
      break;
  }
  return ret;
}
/* ------------------------------------------------------------------------- */
STATIC bool EsfMemoryManagerFopenAreaCheck(uint32_t area_size,
                                           uint32_t handle_offset) {
  if (area_size < handle_offset) {
    // Offset is out of range
    return false;
  }
  // The specified area is within the target area.
  return true;
}
/* ------------------------------------------------------------------------- */
STATIC EsfMemoryManagerResult
EsfMemoryManagerFileIoFopen(EsfMemoryManagerHandle handle, int32_t size) {
  EsfMemoryManagerResult ret = kEsfMemoryManagerResultSuccess;
  PlErrCode err_code = kPlErrCodeOk;
  int file_descriptor = -1;

  // selecting memory information from the memory management information list
  struct EsfMemoryManagerHandleInternal *entry =
      (struct EsfMemoryManagerHandleInternal *)NULL;
  bool handle_exist = EsfMemoryManagerIsHandleExist(handle, &entry);
  // If "handle does not exist" or "handle entry info not exist", return error.
  if ((!handle_exist) ||
      (entry == (struct EsfMemoryManagerHandleInternal *)NULL)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:[%s] parameter error - handle does not exist or "
                     "invalid entry, handle=0x%08x, handle_exist=%d, entry=%p",
                     __FILE__, __LINE__, __func__, handle, handle_exist, entry);
    return kEsfMemoryManagerResultParamError;
  }
  // If the maximum capacity is specified, the size specification is replaced
  // with the entire size of the handle (memory area).
  if (size == MEMORY_MANAGER_OPEN_MAX_SIZE) size = entry->allocate_size;
  // Search for the corresponding map(open) information from the map(open)
  // management information list.
  struct EsfMemoryManagerHandleAddressOffset *map_info_entry =
      (struct EsfMemoryManagerHandleAddressOffset *)NULL;
  bool map_info_exist = EsfMemoryManagerIsMapInfoExist(handle, entry,
                                                       &map_info_entry);
  // If the corresponding handle (partial specification) is already open, the
  // return value is success.
  if (map_info_exist) {
    if ((map_info_entry !=
         (struct EsfMemoryManagerHandleAddressOffset *)NULL) &&
        (map_info_entry->file_descriptor >= 0)) {
      // If the offset and size are the same and fd is a valid value, it is
      // considered to be the same handle and returns success.
      // map(open) registration count up
      map_info_entry->open_count++;
      return kEsfMemoryManagerResultSuccess;
    }
    return kEsfMemoryManagerResultOtherError;
  }
  // Parameter pre-check
  ret = EsfMemoryManagerFileIoPreCheck(entry->target_area,
                                       entry->allocate_address);
  if (ret == kEsfMemoryManagerResultSuccess) {
    // handle open
    // Map function not supported (FileIO function supported)
    // Open the file using the handle

    if (!EsfMemoryManagerFopenAreaCheck((uint32_t)entry->allocate_size,
                                        MEMORY_MANAGER_HANDLE_OFFSET(handle)))
      return kEsfMemoryManagerResultParamError;
    if (entry->file_descriptor < 0) {
      // Map function not supported (FileIO function supported)
      // Open the file using the handle
      err_code = PlLheapFopen((PlLheapHandle)(uintptr_t)entry->allocate_address,
                              &file_descriptor);
      if ((err_code == kPlErrCodeOk) && (file_descriptor >= 0)) {
        entry->file_descriptor = file_descriptor;
        // If the file is opened successfully, save the file descriptor.
        ret = kEsfMemoryManagerResultSuccess;
      } else {
        // If the file cannot be opened, an error is returned.
        ret = kEsfMemoryManagerResultFileIoError;
        // Elog (telemetry) output
        ESF_MEMORY_MANAGER_ELOG_ERROR(ESF_ERROR_MEMORY_FILEIO);
        WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] failed(%d)", __FILE__,
                         __LINE__, __func__, ret);
      }
    } else {
      file_descriptor = entry->file_descriptor;
    }
    if (ret == kEsfMemoryManagerResultSuccess) {
      // allocate map(open) management information for new registrations
      struct EsfMemoryManagerHandleAddressOffset *new_list = NULL;
      new_list = malloc(sizeof(struct EsfMemoryManagerHandleAddressOffset));
      if (new_list == NULL) {
        WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                         "%s-%d:[%s] other error - malloc failed for map info, "
                         "handle=0x%08x",
                         __FILE__, __LINE__, __func__, handle);
        return kEsfMemoryManagerResultOtherError;
      }
      new_list->address_offset = MEMORY_MANAGER_HANDLE_OFFSET(handle);
      new_list->size = size - MEMORY_MANAGER_HANDLE_OFFSET(handle);
      new_list->file_descriptor = file_descriptor;
      new_list->seek_position = 0;
      new_list->open_count = 1;
      // mutex initialize
      int32_t err = pthread_mutex_init(&(new_list->handle_mutex), NULL);
      if (err != 0) {
        WRITE_DLOG_ERROR(
            MODULE_ID_SYSTEM,
            "%s-%d:[%s] other error - mutex init failed, handle=0x%08x, err=%d",
            __FILE__, __LINE__, __func__, handle, err);
        free(new_list);
        return kEsfMemoryManagerResultOtherError;
      }
      // register map(open) management information in the map management
      // information list
      SLIST_INSERT_HEAD(&(entry->map_info_list), new_list, next);
      // map registration count up
      entry->map_count++;
    }
  }
  return ret;
}
/* ------------------------------------------------------------------------- */
STATIC EsfMemoryManagerResult EsfMemoryManagerFileIoGetHandleEntry(
    EsfMemoryManagerHandle handle,
    struct EsfMemoryManagerHandleInternal **handle_entry,
    struct EsfMemoryManagerHandleAddressOffset **map_entry) {
  EsfMemoryManagerResult ret = kEsfMemoryManagerResultSuccess;
  // check parameters.
  if ((handle_entry == (struct EsfMemoryManagerHandleInternal **)NULL) ||
      (map_entry == (struct EsfMemoryManagerHandleAddressOffset **)NULL)) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:[%s] parameter error - handle_entry=%p, map_entry=%p", __FILE__,
        __LINE__, __func__, handle_entry, map_entry);
    return kEsfMemoryManagerResultParamError;
  }
  // selecting memory information from the memory management
  // information list
  struct EsfMemoryManagerHandleInternal *entry =
      (struct EsfMemoryManagerHandleInternal *)NULL;
  bool handle_exist = EsfMemoryManagerIsHandleExist(handle, &entry);
  *handle_entry = entry;
  // If "handle does not exist" or "handle entry info not exist", return error.
  if ((!handle_exist) ||
      (entry == (struct EsfMemoryManagerHandleInternal *)NULL)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:[%s] parameter error - handle does not exist or "
                     "invalid entry, handle=0x%08x, handle_exist=%d, entry=%p",
                     __FILE__, __LINE__, __func__, handle, handle_exist, entry);
    ret = kEsfMemoryManagerResultParamError;
    goto exit;
  }
  // Search for the corresponding map(open) information from the map(open)
  // management information list.
  struct EsfMemoryManagerHandleAddressOffset *map_info_entry =
      (struct EsfMemoryManagerHandleAddressOffset *)NULL;
  bool map_info_exist = EsfMemoryManagerIsMapInfoExist(handle, entry,
                                                       &map_info_entry);
  *map_entry = map_info_entry;
  // If "it is closed", return an error.
  if ((!map_info_exist) ||
      (map_info_entry == (struct EsfMemoryManagerHandleAddressOffset *)NULL)) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:[%s] parameter error - map info does not exist or closed, "
        "handle=0x%08x, map_info_exist=%d, map_info_entry=%p",
        __FILE__, __LINE__, __func__, handle, map_info_exist, map_info_entry);
    ret = kEsfMemoryManagerResultParamError;
    goto exit;
  }
exit:
  return ret;
}
/* ------------------------------------------------------------------------- */
STATIC EsfMemoryManagerResult
EsfMemoryManagerFileIoFclose(EsfMemoryManagerHandle handle) {
  EsfMemoryManagerResult ret = kEsfMemoryManagerResultSuccess;
  PlErrCode err_code = kPlErrCodeOk;
  int32_t err = pthread_mutex_lock(&s_memory_manager_mutex);
  if (err != 0) {
    // Elog (telemetry) output
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] mutex_lock failed", __FILE__,
                     __LINE__, __func__);
    return kEsfMemoryManagerResultOtherError;
  }

  struct EsfMemoryManagerHandleInternal *entry =
      (struct EsfMemoryManagerHandleInternal *)NULL;
  struct EsfMemoryManagerHandleAddressOffset *map_info_entry =
      (struct EsfMemoryManagerHandleAddressOffset *)NULL;
  // selecting memory information from the memory management information list
  // Search for the corresponding map(open) information from the map(open)
  // management information list.
  ret = EsfMemoryManagerFileIoGetHandleEntry(handle, &entry, &map_info_entry);
  if (ret != kEsfMemoryManagerResultSuccess) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:[%s] get handle entry failed, handle=0x%08x, ret=%d", __FILE__,
        __LINE__, __func__, handle, ret);
    goto exit;
  }
  // If "it is closed", return an error.
  if (map_info_entry->file_descriptor < 0) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:[%s] operation error - file already closed, "
                     "handle=0x%08x, fd=%d",
                     __FILE__, __LINE__, __func__, handle,
                     map_info_entry->file_descriptor);
    ret = kEsfMemoryManagerResultOperationError;
    goto exit;
  }
  // Parameter pre-check
  ret = EsfMemoryManagerFileIoPreCheck(entry->target_area,
                                       entry->allocate_address);
  if (ret == kEsfMemoryManagerResultSuccess) {
    // handle close
    // map(open) registration count down
    if (map_info_entry->open_count > 0) {
      map_info_entry->open_count--;
    }
    // If there are open files, they will not be closed.
    if (map_info_entry->open_count != 0) goto exit;
    // map registration count down
    if (entry->map_count > 0) {
      entry->map_count--;
    }
    uint64_t allocate_address =
        entry->allocate_address;  // allocated memory start address
    int file_descriptor =
        entry->file_descriptor;  // File Descriptors (for FileIO)
    pthread_mutex_t *handle_mutex =
        &(map_info_entry->handle_mutex);  // FileIO handle(fd) mutex
    int32_t map_count = entry->map_count;
    // FileIO access processing (mutex lock)
    err = pthread_mutex_lock(handle_mutex);
    if (err != 0) {
      ret = kEsfMemoryManagerResultOtherError;
      // Elog (telemetry) output
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] mutex_lock failed",
                       __FILE__, __LINE__, __func__);
      goto exit;
    }
    // Close when the number of open areas becomes "0"
    if (map_count == 0) {
      // Close the file using "handle" & "file descriptor"
      err_code = PlLheapFclose((PlLheapHandle)(uintptr_t)allocate_address,
                               file_descriptor);
    }
    // FileIO access end (mutex unlock)
    pthread_mutex_unlock(handle_mutex);
    // selecting memory information from the memory management information list
    // Search for the corresponding map(open) information from the map(open)
    // management information list.
    ret = EsfMemoryManagerFileIoGetHandleEntry(handle, &entry, &map_info_entry);
    if (ret != kEsfMemoryManagerResultSuccess) {
      ret = kEsfMemoryManagerResultOtherError;
      goto exit;
    }
    // If the file is closed successfully, save the file descriptor.
    SLIST_REMOVE(&(entry->map_info_list), map_info_entry,
                 EsfMemoryManagerHandleAddressOffset, next);
    pthread_mutex_destroy(&(map_info_entry->handle_mutex));
    free(map_info_entry);
    map_info_entry = (struct EsfMemoryManagerHandleAddressOffset *)
        NULL;  // FileIO access end (mutex unlock)
    if (entry->map_count == 0) {
      if (err_code == kPlErrCodeOk) {
        entry->file_descriptor = -1;
        ret = kEsfMemoryManagerResultSuccess;
      } else {
        // If the file cannot be closed, an error is returned.
        ret = kEsfMemoryManagerResultFileIoError;
        // Elog (telemetry) output
        ESF_MEMORY_MANAGER_ELOG_ERROR(ESF_ERROR_MEMORY_FILEIO);
        WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] failed(%d)", __FILE__,
                         __LINE__, __func__, ret);
      }
    }
  }
exit:
  pthread_mutex_unlock(&s_memory_manager_mutex);
  return ret;
}
/* ------------------------------------------------------------------------- */
STATIC EsfMemoryManagerResult EsfMemoryManagerFileIoFseekPreCheck(
    struct EsfMemoryManagerHandleAddressOffset *map_info,
    EsfMemoryManagerFileIoFseekParam *param) {
  // Map function not supported (FileIO function supported)
  // Parameter check for each seek position.
  switch (param->whence) {
    case SEEK_SET:
      // Set Seek position as the position from beginning of file
      if ((param->offset < 0) || (param->offset > (off_t)map_info->size)) {
        WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                         "%s-%d:[%s] parameter error - SEEK_SET offset out of "
                         "range, offset=%ld, size=%u",
                         __FILE__, __LINE__, __func__, param->offset,
                         map_info->size);
        return kEsfMemoryManagerResultParamError;
      }
      break;
    case SEEK_CUR:
      // Set Seek position as position to move from current file position
      if (param->offset >= 0) {
        if ((map_info->seek_position + param->offset) > (off_t)map_info->size) {
          WRITE_DLOG_ERROR(
              MODULE_ID_SYSTEM,
              "%s-%d:[%s] parameter error - SEEK_CUR forward offset out of "
              "range, seek_pos=%ld, offset=%ld, size=%u",
              __FILE__, __LINE__, __func__, map_info->seek_position,
              param->offset, map_info->size);
          return kEsfMemoryManagerResultParamError;
        }
      } else {
        if (map_info->seek_position < (off_t)(llabs(param->offset))) {
          WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                           "%s-%d:[%s] parameter error - SEEK_CUR backward "
                           "offset out of range, seek_pos=%ld, offset=%ld",
                           __FILE__, __LINE__, __func__,
                           map_info->seek_position, param->offset);
          return kEsfMemoryManagerResultParamError;
        }
      }
      break;
    case SEEK_END:
      // Set Seek position as position to move from end of file
      if ((param->offset > 0) ||
          ((off_t)map_info->size < (off_t)(llabs(param->offset)))) {
        WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                         "%s-%d:[%s] parameter error - SEEK_END offset out of "
                         "range, offset=%ld, size=%u",
                         __FILE__, __LINE__, __func__, param->offset,
                         map_info->size);
        return kEsfMemoryManagerResultParamError;
      }
      break;
    default:
      // Undefined
      WRITE_DLOG_ERROR(
          MODULE_ID_SYSTEM,
          "%s-%d:[%s] parameter error - invalid whence value, whence=%d",
          __FILE__, __LINE__, __func__, param->whence);
      return kEsfMemoryManagerResultParamError;
  }
  return kEsfMemoryManagerResultSuccess;
}
/* ------------------------------------------------------------------------- */
STATIC EsfMemoryManagerResult EsfMemoryManagerFileIoFseekExec(
    uint64_t allocate_address, int file_descriptor, uint32_t address_offset,
    int32_t allocate_size, EsfMemoryManagerFileIoFseekParam *param,
    off_t *seek_position) {
  EsfMemoryManagerResult ret = kEsfMemoryManagerResultSuccess;
  PlErrCode err_code = kPlErrCodeOk;
  off_t request_position = 0;
  off_t result_position = 0;
  switch (param->whence) {
    case SEEK_SET:
      request_position = (off_t)address_offset + param->offset;
      break;
    case SEEK_CUR:
      request_position = (off_t)address_offset + *seek_position + param->offset;
      break;
    case SEEK_END:
      request_position = (off_t)address_offset + (off_t)allocate_size +
                         param->offset;
      break;
    default:
      // If the parameters are invalid, error is returned.
      WRITE_DLOG_ERROR(
          MODULE_ID_SYSTEM,
          "%s-%d:[%s] parameter error - invalid whence value, whence=%d",
          __FILE__, __LINE__, __func__, param->whence);
      return kEsfMemoryManagerResultParamError;
  }
  err_code = PlLheapFseek((PlLheapHandle)(uintptr_t)allocate_address,
                          file_descriptor, request_position, SEEK_SET,
                          &result_position);
  if (err_code == kPlErrCodeOk) {
    // If file is seek successfully, save file position.
    *seek_position = result_position - (off_t)address_offset;
    *param->result_offset = *seek_position;
    ret = kEsfMemoryManagerResultSuccess;
  } else if (err_code == kPlErrInvalidParam) {
    // If the parameters are invalid, error is returned.
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:[%s] parameter error - PLheap seek failed with "
                     "invalid param, fd=%d, pos=%ld",
                     __FILE__, __LINE__, __func__, file_descriptor,
                     request_position);
    ret = kEsfMemoryManagerResultParamError;
  } else {
    // If file cannot be seek, error is returned.
    ret = kEsfMemoryManagerResultFileIoError;
    // Elog (telemetry) output
    ESF_MEMORY_MANAGER_ELOG_ERROR(ESF_ERROR_MEMORY_FILEIO);
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] failed(%d)", __FILE__,
                     __LINE__, __func__, ret);
  }
  return ret;
}  // NOLINT
/* ------------------------------------------------------------------------- */
STATIC EsfMemoryManagerResult
EsfMemoryManagerFileIoSeekParamCheck(EsfMemoryManagerFileIoFseekParam *param) {
  if ((param == (EsfMemoryManagerFileIoFseekParam *)NULL) ||
      (param->result_offset == (off_t *)NULL)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:[%s] parameter error - param=%p, result_offset=%p",
                     __FILE__, __LINE__, __func__, param,
                     param ? param->result_offset : NULL);
    return kEsfMemoryManagerResultParamError;
  }
  return kEsfMemoryManagerResultSuccess;
}
/* ------------------------------------------------------------------------- */
STATIC EsfMemoryManagerResult EsfMemoryManagerFileIoFseek(
    EsfMemoryManagerHandle handle, EsfMemoryManagerFileIoFseekParam *param) {
  EsfMemoryManagerResult ret = kEsfMemoryManagerResultSuccess;
  // check parameters.
  ret = EsfMemoryManagerFileIoSeekParamCheck(param);
  if (ret != kEsfMemoryManagerResultSuccess) return ret;

  int32_t err = pthread_mutex_lock(&s_memory_manager_mutex);
  if (err != 0) {
    // Elog (telemetry) output
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] mutex_lock failed", __FILE__,
                     __LINE__, __func__);
    return kEsfMemoryManagerResultOtherError;
  }
  struct EsfMemoryManagerHandleInternal *entry =
      (struct EsfMemoryManagerHandleInternal *)NULL;
  struct EsfMemoryManagerHandleAddressOffset *map_info_entry =
      (struct EsfMemoryManagerHandleAddressOffset *)NULL;
  // selecting memory information from the memory management information list
  // Search for the corresponding map(open) information from the map(open)
  // management information list.
  ret = EsfMemoryManagerFileIoGetHandleEntry(handle, &entry, &map_info_entry);
  if (ret != kEsfMemoryManagerResultSuccess) goto exit;
  // If "it is closed", return an error.
  if (map_info_entry->file_descriptor < 0) {
    ret = kEsfMemoryManagerResultOperationError;
    goto exit;
  }

  // Parameter pre-check
  ret = EsfMemoryManagerFileIoPreCheck(entry->target_area,
                                       entry->allocate_address);
  if (ret == kEsfMemoryManagerResultSuccess) {
    // Seek Pre-Check
    ret = EsfMemoryManagerFileIoFseekPreCheck(map_info_entry, param);
    if (ret != kEsfMemoryManagerResultSuccess) goto exit;

    uint64_t allocate_address =
        entry->allocate_address;  // allocated memory start address
    uint32_t address_offset =
        map_info_entry->address_offset;            // Area part specified offset
    int32_t allocate_size = map_info_entry->size;  // allocated memory size
    int file_descriptor =
        map_info_entry->file_descriptor;  // File Descriptors (for FileIO)
    off_t seek_position =
        map_info_entry->seek_position;  // File seek position (for FileIO)
    pthread_mutex_t *handle_mutex =
        &(map_info_entry->handle_mutex);  // FileIO handle(fd) mutex

    pthread_mutex_unlock(&s_memory_manager_mutex);
    // FileIO access processing (mutex lock)
    err = pthread_mutex_lock(handle_mutex);
    if (err != 0) {
      // Elog (telemetry) output
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] mutex_lock failed",
                       __FILE__, __LINE__, __func__);
      return kEsfMemoryManagerResultOtherError;
    }
    //  Seek execution
    ret = EsfMemoryManagerFileIoFseekExec(allocate_address, file_descriptor,
                                          address_offset, allocate_size, param,
                                          &seek_position);
    // FileIO access end (mutex unlock)
    pthread_mutex_unlock(handle_mutex);
    err = pthread_mutex_lock(&s_memory_manager_mutex);
    if (err != 0) {
      // Elog (telemetry) output
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] mutex_lock failed",
                       __FILE__, __LINE__, __func__);
      return kEsfMemoryManagerResultOtherError;
    }
    // selecting memory information from the memory management information list
    // Search for the corresponding map(open) information from the map(open)
    // management information list.
    EsfMemoryManagerResult ret2 =
        EsfMemoryManagerFileIoGetHandleEntry(handle, &entry, &map_info_entry);
    if (ret2 != kEsfMemoryManagerResultSuccess) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:[%s] get handle entry failed after seek, "
                       "handle=0x%08x, ret2=%d",
                       __FILE__, __LINE__, __func__, handle, ret2);
      ret = kEsfMemoryManagerResultOtherError;
      goto exit;
    }
    if (ret == kEsfMemoryManagerResultSuccess)
      map_info_entry->seek_position = seek_position;
  }
exit:
  pthread_mutex_unlock(&s_memory_manager_mutex);
  return ret;
}
/* ------------------------------------------------------------------------ */
STATIC PlErrCode EsfMemoryManagerFileIoAccessFunc(
    uint64_t allocate_address, int file_descriptor,
    EsfMemoryManagerFileIoRequest operation, void *access_buff,
    size_t access_size, size_t *result_size) {
  PlErrCode err_code = kPlErrCodeOk;
  if (operation == kEsfMemoryManagerFileIoFwrite) {
    err_code = PlLheapFwrite((PlLheapHandle)(uintptr_t)allocate_address,
                             file_descriptor, access_buff, access_size,
                             result_size);
  } else {
    err_code = PlLheapFread((PlLheapHandle)(uintptr_t)allocate_address,
                            file_descriptor, access_buff, access_size,
                            result_size);
  }
  return err_code;
}
/* ------------------------------------------------------------------------- */
STATIC EsfMemoryManagerResult EsfMemoryManagerFileIoFwriteFreadExec(
    uint64_t allocate_address, int file_descriptor,
    EsfMemoryManagerFileIoRequest operation,
    EsfMemoryManagerFileIoAccessParam *param, off_t *seek_position) {
  PlErrCode err_code = kPlErrCodeOk;
  EsfMemoryManagerResult ret = kEsfMemoryManagerResultSuccess;
  size_t remain_size = 0;
  void *access_buff = (void *)NULL;
  size_t access_size = 0;
  size_t result_size = 0;
  uint32_t split_count = 0;
  // check parameters.
  if ((allocate_address == (uint64_t)0) || (file_descriptor < 0) ||
      ((operation != kEsfMemoryManagerFileIoFwrite) &&
       (operation != kEsfMemoryManagerFileIoFread)) ||
      (param == (EsfMemoryManagerFileIoAccessParam *)NULL) ||
      (seek_position == (off_t *)NULL)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:[%s] parameter error - address=0x%016" PRIx64
                     ", fd=%d, "
                     "operation=%d, param=%p, seek_position=%p",
                     __FILE__, __LINE__, __func__, allocate_address,
                     file_descriptor, operation, param, seek_position);
    return kEsfMemoryManagerResultParamError;
  }
  // FileIO access preparation
  remain_size = param->size;
  access_buff = param->buff;
  split_count = (uint32_t)(remain_size / MEMORY_MANAGER_ACCESS_MAX_SIZE);
  if (split_count > 0) access_size = MEMORY_MANAGER_ACCESS_MAX_SIZE;

  // FileIO access by maximum access size.
  // (max:MEMORY_MANAGER_ACCESS_MAX_SIZE(4096))
  for (uint32_t i = 0; i < split_count; i++) {
    // Thread switching entrance
    // pthread_yield() is deprecated, changed to sched_yield()
    sched_yield();
    // FileIO Access API call
    err_code = EsfMemoryManagerFileIoAccessFunc(
        allocate_address, file_descriptor, operation, access_buff, access_size,
        &result_size);
    if ((err_code == kPlErrCodeOk) && (access_size == result_size)) {
      // If file is seek successfully, save file position.
      remain_size = remain_size - access_size;
      access_buff = access_buff + access_size;
      *seek_position = *seek_position + access_size;
      *param->rsize = *param->rsize + result_size;
      ret = kEsfMemoryManagerResultSuccess;
    } else {
      // If file cannot be seek, error is returned.
      ret = kEsfMemoryManagerResultFileIoError;
      // Elog (telemetry) output
      ESF_MEMORY_MANAGER_ELOG_ERROR(ESF_ERROR_MEMORY_FILEIO);
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] failed(%d)", __FILE__,
                       __LINE__, __func__, ret);
      break;
    }
  }
  // If an access error occurs, return here.
  if (ret != kEsfMemoryManagerResultSuccess) return ret;

  // FileIO access by remaining size (less than max size)
  if (remain_size > 0) {
    // Thread switching entrance
    // pthread_yield() is deprecated, changed to sched_yield()
    sched_yield();
    // FileIO Access API call
    access_size = remain_size;
    err_code = EsfMemoryManagerFileIoAccessFunc(
        allocate_address, file_descriptor, operation, access_buff, access_size,
        &result_size);
    if ((err_code == kPlErrCodeOk) && (access_size == result_size)) {
      // If file is seek successfully, save file position.
      remain_size = remain_size - access_size;
      access_buff = access_buff + access_size;
      *seek_position = *seek_position + access_size;
      *param->rsize = *param->rsize + result_size;
      ret = kEsfMemoryManagerResultSuccess;
    } else {
      // If file cannot be seek, error is returned.
      ret = kEsfMemoryManagerResultFileIoError;
      // Elog (telemetry) output
      ESF_MEMORY_MANAGER_ELOG_ERROR(ESF_ERROR_MEMORY_FILEIO);
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] failed(%d)", __FILE__,
                       __LINE__, __func__, ret);
    }
  }
  return ret;
}
/* ------------------------------------------------------------------------- */
STATIC EsfMemoryManagerResult EsfMemoryManagerFileIoAccessParamCheck(
    EsfMemoryManagerFileIoRequest operation,
    EsfMemoryManagerFileIoAccessParam *param) {
  if ((operation != kEsfMemoryManagerFileIoFwrite) &&
      (operation != kEsfMemoryManagerFileIoFread)) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:[%s] parameter error - invalid operation, operation=%d",
        __FILE__, __LINE__, __func__, operation);
    return kEsfMemoryManagerResultParamError;
  }
  if ((param == (EsfMemoryManagerFileIoAccessParam *)NULL) ||
      (param->buff == (void *)NULL) || (param->size == 0) ||
      (param->rsize == (size_t *)NULL)) {
    WRITE_DLOG_ERROR(
        MODULE_ID_SYSTEM,
        "%s-%d:[%s] parameter error - param=%p, buff=%p, size=%zu, rsize=%p",
        __FILE__, __LINE__, __func__, param, param ? param->buff : NULL,
        param ? param->size : 0, param ? param->rsize : NULL);
    return kEsfMemoryManagerResultParamError;
  }
  return kEsfMemoryManagerResultSuccess;
}
/* ------------------------------------------------------------------------- */
STATIC EsfMemoryManagerResult EsfMemoryManagerFileIoFwriteFread(
    EsfMemoryManagerHandle handle, EsfMemoryManagerFileIoRequest operation,
    EsfMemoryManagerFileIoAccessParam *param) {
  EsfMemoryManagerResult ret = kEsfMemoryManagerResultSuccess;

  // check parameters.
  ret = EsfMemoryManagerFileIoAccessParamCheck(operation, param);
  if (ret != kEsfMemoryManagerResultSuccess) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:[%s] access param check failed, handle=0x%08x, "
                     "operation=%d, ret=%d",
                     __FILE__, __LINE__, __func__, handle, operation, ret);
    return ret;
  }
  int32_t err = pthread_mutex_lock(&s_memory_manager_mutex);
  if (err != 0) {
    // Elog (telemetry) output
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] mutex_lock failed", __FILE__,
                     __LINE__, __func__);
    return kEsfMemoryManagerResultOtherError;
  }
  struct EsfMemoryManagerHandleInternal *entry =
      (struct EsfMemoryManagerHandleInternal *)NULL;
  struct EsfMemoryManagerHandleAddressOffset *map_info_entry =
      (struct EsfMemoryManagerHandleAddressOffset *)NULL;
  // selecting memory information from the memory management information list
  // Search for the corresponding map(open) information from the map(open)
  // management information list.
  ret = EsfMemoryManagerFileIoGetHandleEntry(handle, &entry, &map_info_entry);
  if (ret != kEsfMemoryManagerResultSuccess) goto exit;
  // If "it is closed", return an error.
  if (map_info_entry->file_descriptor < 0) {
    ret = kEsfMemoryManagerResultOperationError;
    goto exit;
  }

  // Parameter pre-check
  ret = EsfMemoryManagerFileIoPreCheck(entry->target_area,
                                       entry->allocate_address);
  if (ret == kEsfMemoryManagerResultSuccess) {
    // Get handle info
    uint64_t allocate_address =
        entry->allocate_address;  // allocated memory start address
    int32_t allocate_size = map_info_entry->size;  // allocated memory size
    int file_descriptor =
        map_info_entry->file_descriptor;  // File Descriptors (for FileIO)
    off_t seek_position =
        map_info_entry->seek_position;  // File seek position (for FileIO)
    pthread_mutex_t *handle_mutex =
        &(map_info_entry->handle_mutex);  // FileIO handle(fd) mutex
    // check parameters.
    if ((int32_t)(seek_position + param->size) > allocate_size) {
      ret = kEsfMemoryManagerResultParamError;
      goto exit;
    }
    pthread_mutex_unlock(&s_memory_manager_mutex);
    // FileIO access processing (mutex lock)
    err = pthread_mutex_lock(handle_mutex);
    if (err != 0) {
      // Elog (telemetry) output
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] mutex_lock failed",
                       __FILE__, __LINE__, __func__);
      ret = kEsfMemoryManagerResultOtherError;
      goto exit;
    }
    ret = EsfMemoryManagerFileIoFwriteFreadExec(
        allocate_address, file_descriptor, operation, param, &seek_position);
    // FileIO access end (mutex unlock)
    pthread_mutex_unlock(handle_mutex);
    err = pthread_mutex_lock(&s_memory_manager_mutex);
    if (err != 0) {
      // Elog (telemetry) output
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] mutex_lock failed",
                       __FILE__, __LINE__, __func__);
      return kEsfMemoryManagerResultOtherError;
    }
    // selecting memory information from the memory management information list
    // Search for the corresponding map(open) information from the map(open)
    // management information list.
    EsfMemoryManagerResult ret2 =
        EsfMemoryManagerFileIoGetHandleEntry(handle, &entry, &map_info_entry);
    if (ret2 != kEsfMemoryManagerResultSuccess) {
      ret = kEsfMemoryManagerResultOtherError;
      goto exit;
    }
    // FileIO access end
    if (ret == kEsfMemoryManagerResultSuccess)
      map_info_entry->seek_position = seek_position;
  }
exit:
  pthread_mutex_unlock(&s_memory_manager_mutex);
  return ret;
}
/* ------------------------------------------------------------------------- */
STATIC EsfMemoryManagerResult EsfMemoryManagerFileIoPwritePread(
    EsfMemoryManagerHandle handle, EsfMemoryManagerFileIoRequest operation,
    EsfMemoryManagerFileIoFseekParam *seek_param,
    EsfMemoryManagerFileIoAccessParam *access_param) {
  EsfMemoryManagerResult ret = kEsfMemoryManagerResultSuccess;

  // check parameters.
  ret = EsfMemoryManagerFileIoSeekParamCheck(seek_param);
  if (ret != kEsfMemoryManagerResultSuccess) return ret;
  ret = EsfMemoryManagerFileIoAccessParamCheck(operation, access_param);
  if (ret != kEsfMemoryManagerResultSuccess) return ret;

  int32_t err = pthread_mutex_lock(&s_memory_manager_mutex);
  if (err != 0) {
    // Elog (telemetry) output
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] mutex_lock failed", __FILE__,
                     __LINE__, __func__);
    return kEsfMemoryManagerResultOtherError;
  }
  struct EsfMemoryManagerHandleInternal *entry =
      (struct EsfMemoryManagerHandleInternal *)NULL;
  struct EsfMemoryManagerHandleAddressOffset *map_info_entry =
      (struct EsfMemoryManagerHandleAddressOffset *)NULL;
  // selecting memory information from the memory management information list
  // Search for the corresponding map(open) information from the map(open)
  // management information list.
  ret = EsfMemoryManagerFileIoGetHandleEntry(handle, &entry, &map_info_entry);
  if (ret != kEsfMemoryManagerResultSuccess) goto exit;
  // If "it is closed", return an error.
  if (map_info_entry->file_descriptor < 0) {
    ret = kEsfMemoryManagerResultOperationError;
    goto exit;
  }

  // Parameter pre-check
  ret = EsfMemoryManagerFileIoPreCheck(entry->target_area,
                                       entry->allocate_address);
  if (ret == kEsfMemoryManagerResultSuccess) {
    // Get handle info
    uint64_t allocate_address =
        entry->allocate_address;  // allocated memory start address
    uint32_t address_offset =
        map_info_entry->address_offset;            // Area part specified offset
    int32_t allocate_size = map_info_entry->size;  // allocated memory size
    int file_descriptor =
        map_info_entry->file_descriptor;  // File Descriptors (for FileIO)
    off_t seek_position =
        map_info_entry->seek_position;  // File seek position (for FileIO)
    pthread_mutex_t *handle_mutex =
        &(map_info_entry->handle_mutex);  // FileIO handle(fd) mutex
    // check parameters.
    if ((int32_t)(seek_position + access_param->size) > allocate_size) {
      WRITE_DLOG_ERROR(
          MODULE_ID_SYSTEM,
          "%s-%d:[%s] Parameter size check failed - seek_position=%ld, "
          "access_param->size=%zu, allocate_size=%d",
          __FILE__, __LINE__, __func__, seek_position, access_param->size,
          allocate_size);
      ret = kEsfMemoryManagerResultParamError;
      goto exit;
    }
    ret = EsfMemoryManagerFileIoFseekPreCheck(map_info_entry, seek_param);
    if (ret != kEsfMemoryManagerResultSuccess) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:[%s] Fseek precheck failed - ret=%d, "
                       "map_info_entry=%p, seek_param=%p",
                       __FILE__, __LINE__, __func__, ret, map_info_entry,
                       seek_param);
      goto exit;
    }

    pthread_mutex_unlock(&s_memory_manager_mutex);
    // FileIO access processing (mutex lock)
    err = pthread_mutex_lock(handle_mutex);
    if (err != 0) {
      // Elog (telemetry) output
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] mutex_lock failed",
                       __FILE__, __LINE__, __func__);
      ret = kEsfMemoryManagerResultOtherError;
      goto exit;
    }
    //  Seek execution
    ret = EsfMemoryManagerFileIoFseekExec(allocate_address, file_descriptor,
                                          address_offset, allocate_size,
                                          seek_param, &seek_position);
    if (ret != kEsfMemoryManagerResultSuccess) {
      // FileIO access end (mutex unlock)
      pthread_mutex_unlock(handle_mutex);
      goto exit;
    }
    ret = EsfMemoryManagerFileIoFwriteFreadExec(allocate_address,
                                                file_descriptor, operation,
                                                access_param, &seek_position);
    // FileIO access end (mutex unlock)
    pthread_mutex_unlock(handle_mutex);
    err = pthread_mutex_lock(&s_memory_manager_mutex);
    if (err != 0) {
      // Elog (telemetry) output
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] mutex_lock failed",
                       __FILE__, __LINE__, __func__);
      return kEsfMemoryManagerResultOtherError;
    }
    // selecting memory information from the memory management information list
    // Search for the corresponding map(open) information from the map(open)
    // management information list.
    EsfMemoryManagerResult ret2 =
        EsfMemoryManagerFileIoGetHandleEntry(handle, &entry, &map_info_entry);
    if (ret2 != kEsfMemoryManagerResultSuccess) {
      ret = kEsfMemoryManagerResultOtherError;
      goto exit;
    }
    // FileIO access end
    if (ret == kEsfMemoryManagerResultSuccess)
      map_info_entry->seek_position = seek_position;
  }
exit:
  pthread_mutex_unlock(&s_memory_manager_mutex);
  return ret;
}
/* ------------------------------------------------------------------------- */
STATIC EsfMemoryManagerResult EsfMemoryManagerFileIoIsMapSupport(
    EsfMemoryManagerHandle handle, EsfMemoryManagerMapSupport *param) {
  if (param == (EsfMemoryManagerMapSupport *)NULL) {
    // Map function support (FileIO function not supported)
    return kEsfMemoryManagerResultParamError;
  }
  // selecting memory information from the memory management information list
  struct EsfMemoryManagerHandleInternal *entry =
      (struct EsfMemoryManagerHandleInternal *)NULL;
  bool handle_exist = EsfMemoryManagerIsHandleExist(handle, &entry);
  // If "handle does not exist" or "handle entry info not exist", return error.
  if ((!handle_exist) ||
      (entry == (struct EsfMemoryManagerHandleInternal *)NULL))
    return kEsfMemoryManagerResultParamError;

  // memory allocation
  switch (entry->target_area) {
    case kEsfMemoryManagerTargetLargeHeap:
      // Map function support check
      if (!PlLheapIsMapSupport(
              (PlLheapHandle)(uintptr_t)entry->allocate_address)) {
        // Map function not support (FileIO function supported)
        *param = kEsfMemoryManagerMapIsNotSupport;
      } else {
        // Map function support (FileIO function not supported)
        *param = kEsfMemoryManagerMapIsSupport;
      }
      break;
    case kEsfMemoryManagerTargetDma:
    case kEsfMemoryManagerTargetWasmHeap:
    default:
      // Map function support (FileIO function not supported)
      *param = kEsfMemoryManagerMapIsSupport;
      break;
  }
  return kEsfMemoryManagerResultSuccess;
}
/* ------------------------------------------------------------------------- */
STATIC EsfMemoryManagerResult EsfMemoryManagerSetHandleInfo(
    EsfMemoryManagerHandle handle, EsfMemoryManagerHandleInfo *info) {
  EsfMemoryManagerResult ret = kEsfMemoryManagerResultSuccess;
  if (info == (EsfMemoryManagerHandleInfo *)NULL) {
    // TODO: Error log.
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] Parameter error", __FILE__,
                     __LINE__, __func__);
    return kEsfMemoryManagerResultParamError;
  }
  info->target_area = kEsfMemoryManagerTargetOtherHeap;
  info->allocate_size = 0;
  // selecting memory information from the memory management information list
  struct EsfMemoryManagerHandleInternal *entry =
      (struct EsfMemoryManagerHandleInternal *)NULL;
  bool handle_exist = EsfMemoryManagerIsHandleExist(handle, &entry);
  WRITE_DLOG_DEBUG(MODULE_ID_SYSTEM, "%s-%d:[%s] handle=0x%08x, handle_exist=%s",
                  __FILE__, __LINE__, __func__, handle,
                  handle_exist ? "true" : "false");
  // If "handle does not exist", return an error.
  if (!handle_exist) {
    // Anything not under the handle management of the memory manager is
    // considered to be "other area."
    WRITE_DLOG_DEBUG(MODULE_ID_SYSTEM,
                    "%s-%d:[%s] handle does not exist, staying with OtherHeap",
                    __FILE__, __LINE__, __func__);
    ret = kEsfMemoryManagerResultSuccess;
    goto exit;
  }
  // If "handle entry info not exist", return an error.
  if (entry == (struct EsfMemoryManagerHandleInternal *)NULL) {
    // Anything not under the handle management of the memory manager is
    // considered to be "other area."
    ret = kEsfMemoryManagerResultOtherError;
    goto exit;
  }
  // Get handle information
  ret = kEsfMemoryManagerResultOtherError;
  switch (entry->target_area) {
    case kEsfMemoryManagerTargetLargeHeap:
      if (PlLheapIsValid((PlDmaMemHandle)(uintptr_t)entry->allocate_address)) {
        // If the memory manager management information and the PL layer
        // management information match, set them as handle information.
        info->target_area = entry->target_area;
        info->allocate_size = entry->allocate_size;
        ret = kEsfMemoryManagerResultSuccess;
      }
      break;
    case kEsfMemoryManagerTargetDma:
      if (PlDmaMemIsValid((PlDmaMemHandle)(uintptr_t)entry->allocate_address)) {
        // If the memory manager management information and the PL layer
        // management information match, set them as handle information.
        info->target_area = entry->target_area;
        info->allocate_size = entry->allocate_size;
        ret = kEsfMemoryManagerResultSuccess;
      }
      break;
    case kEsfMemoryManagerTargetWasmHeap:
    case kEsfMemoryManagerTargetOtherHeap:
    default:
      break;
  }
exit:
  return ret;
}
/* ------------------------------------------------------------------------- */

/****************************************************************************
 * Public Functions
 ****************************************************************************/
EsfMemoryManagerResult EsfMemoryManagerInitialize(int32_t app_mem_div_num) {
  EsfMemoryManagerResult ret = kEsfMemoryManagerResultSuccess;
  PlErrCode err_code = kPlErrCodeOk;

  // Parameter check
  if (app_mem_div_num <= 0) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:[%s] parameter error - app_mem_div_num=%d",
                     __FILE__, __LINE__, __func__, app_mem_div_num);
    return kEsfMemoryManagerResultParamError;
  }
  int32_t err = pthread_mutex_lock(&s_memory_manager_mutex);
  if (err != 0) {
    // Elog (telemetry) output
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] mutex_lock failed", __FILE__,
                     __LINE__, __func__);
    return kEsfMemoryManagerResultOtherError;
  }
  // Initialized Check.
  if (s_is_initialized) {
    pthread_mutex_unlock(&s_memory_manager_mutex);
    return kEsfMemoryManagerResultSuccess;
  }
  // OSAL LargeHeap initialize.
  err_code = PlLheapInitialize();
  if (err_code != kPlErrCodeOk) {
    pthread_mutex_unlock(&s_memory_manager_mutex);
    // TODO: Error log.
    ESF_MEMORY_MANAGER_ELOG_ERROR(ESF_ERROR_LARGE_HEAP_INITIALIZE);
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] Lheap failed", __FILE__,
                     __LINE__, __func__);
    return kEsfMemoryManagerResultOtherError;
  }
  // OSAL AppMemory initialize.
  err_code = PlAppmemInitialize();
  if (err_code != kPlErrCodeOk) {
    pthread_mutex_unlock(&s_memory_manager_mutex);
    // TODO: Error log.
    ESF_MEMORY_MANAGER_ELOG_ERROR(ESF_ERROR_APP_MEMORY_INITIALIZE);
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] AppMem failed", __FILE__,
                     __LINE__, __func__);
    return kEsfMemoryManagerResultOtherError;
  }
  // OSAL AppMemory division number setting
  err_code = PlAppmemSetBlock(app_mem_div_num);
  if (err_code != kPlErrCodeOk) {
    pthread_mutex_unlock(&s_memory_manager_mutex);
    // TODO: Error log.
    ESF_MEMORY_MANAGER_ELOG_ERROR(ESF_ERROR_APP_MEMORY_DIVIDE);
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] AppMem divide failed",
                     __FILE__, __LINE__, __func__);
    return kEsfMemoryManagerResultOtherError;
  }
  // OSAL DmaMemory initialize.
  err_code = PlDmaMemInitialize();
  if (err_code != kPlErrCodeOk) {
    pthread_mutex_unlock(&s_memory_manager_mutex);
    // TODO: Error log.
    ESF_MEMORY_MANAGER_ELOG_ERROR(ESF_ERROR_DMA_INITIALIZE);
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] DMA failed", __FILE__,
                     __LINE__, __func__);
    return kEsfMemoryManagerResultOtherError;
  }
  // Linked list initialize for memory management information.
  SLIST_INIT(&s_handle_info_list);

  // s_handle_id_map initialize(clear)
  EsfMemoryManagerClearHandleId();
  // Memory manager initialized
  s_is_initialized = true;

  pthread_mutex_unlock(&s_memory_manager_mutex);
  return ret;
}
/* ------------------------------------------------------------------------- */
EsfMemoryManagerResult EsfMemoryManagerFinalize(void) {
  EsfMemoryManagerResult ret = kEsfMemoryManagerResultSuccess;
  PlErrCode err_code = kPlErrCodeOk;
  int32_t err = pthread_mutex_lock(&s_memory_manager_mutex);
  if (err != 0) {
    // Elog (telemetry) output
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] mutex_lock failed", __FILE__,
                     __LINE__, __func__);
    return kEsfMemoryManagerResultOtherError;
  }
  // Finalized Check.
  if (!s_is_initialized) {
    pthread_mutex_unlock(&s_memory_manager_mutex);
    return kEsfMemoryManagerResultSuccess;
  }
  // finalizing the linked list of memory management information.
  ret = EsfMemoryManagerCleanUpAllMemoryInfo();
  if (ret != kEsfMemoryManagerResultSuccess) {
    // TODO: Error log. (This process does not go through)
  }
  // OSAL LargeHeap finalize.
  err_code = PlLheapFinalize();
  if (err_code != kPlErrCodeOk) {
    ret = kEsfMemoryManagerResultOtherError;
    // TODO: Error log.
    ESF_MEMORY_MANAGER_ELOG_ERROR(ESF_ERROR_LARGE_HEAP_FINALIZE);
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] Lheap failed", __FILE__,
                     __LINE__, __func__);
  }
  // OSAL AppMemory finalize.
  err_code = PlAppmemFinalize();
  if (err_code != kPlErrCodeOk) {
    ret = kEsfMemoryManagerResultOtherError;
    // TODO: Error log.
    ESF_MEMORY_MANAGER_ELOG_ERROR(ESF_ERROR_APP_MEMORY_FINALIZE);
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] AppMem failed", __FILE__,
                     __LINE__, __func__);
  }
  // OSAL DmaMemory finalize.
  err_code = PlDmaMemFinalize();
  if (err_code != kPlErrCodeOk) {
    ret = kEsfMemoryManagerResultOtherError;
    // TODO: Error log.
    ESF_MEMORY_MANAGER_ELOG_ERROR(ESF_ERROR_DMA_FINALIZE);
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] DMA failed", __FILE__,
                     __LINE__, __func__);
  }
  // Memory manager Finalized
  s_is_initialized = false;

  pthread_mutex_unlock(&s_memory_manager_mutex);
  return ret;
}
/* ------------------------------------------------------------------------- */
EsfMemoryManagerResult EsfMemoryManagerAllocate(
    EsfMemoryManagerTargetArea target_area, const wasm_exec_env_t *exec_env,
    int32_t size, EsfMemoryManagerHandle *handle) {
  EsfMemoryManagerResult ret = kEsfMemoryManagerResultSuccess;
  EsfMemoryManagerHandle user_handle = (EsfMemoryManagerHandle)0;
  // check parameters.
  if ((handle == (EsfMemoryManagerHandle *)NULL) || (size <= 0)) {
    // TODO: Error log.
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] failed(%d)", __FILE__,
                     __LINE__, __func__, kEsfMemoryManagerResultParamError);
    return kEsfMemoryManagerResultParamError;
  }
  int32_t err = pthread_mutex_lock(&s_memory_manager_mutex);
  if (err != 0) {
    // Elog (telemetry) output
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] mutex_lock failed", __FILE__,
                     __LINE__, __func__);
    return kEsfMemoryManagerResultOtherError;
  }
  // initialized Check.
  if (!s_is_initialized) {
    *handle = 0;
    pthread_mutex_unlock(&s_memory_manager_mutex);
    // TODO: Error log.
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] not initialized", __FILE__,
                     __LINE__, __func__);
    return kEsfMemoryManagerResultOtherError;
  }
  // WasmHeap Allocation
  if (target_area == kEsfMemoryManagerTargetWasmHeap) {
    ret = EsfMemoryManagerAllocateWasm(exec_env, size, &user_handle);
  } else {  // LargeHeap,DMA Allocation
    // generate memory management information
    ret = EsfMemoryManagerGenerateMemoryInfo(target_area, size, &user_handle);
  }
  if (ret != kEsfMemoryManagerResultSuccess) {
    // TODO: Error log.
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] failed(%d)", __FILE__,
                     __LINE__, __func__, ret);
  }
  // set the memory operation handle for the new allocate memory
  *handle = user_handle;

  pthread_mutex_unlock(&s_memory_manager_mutex);
  return ret;
}
/* ------------------------------------------------------------------------- */
EsfMemoryManagerResult EsfMemoryManagerFree(EsfMemoryManagerHandle handle,
                                            const wasm_exec_env_t *exec_env) {
  EsfMemoryManagerResult ret = kEsfMemoryManagerResultSuccess;

  int32_t err = pthread_mutex_lock(&s_memory_manager_mutex);
  if (err != 0) {
    // Elog (telemetry) output
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] mutex_lock failed", __FILE__,
                     __LINE__, __func__);
    return kEsfMemoryManagerResultOtherError;
  }
  // initialized Check.
  if (!s_is_initialized) {
    pthread_mutex_unlock(&s_memory_manager_mutex);
    // TODO: Error log.
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] not initialized", __FILE__,
                     __LINE__, __func__);
    return kEsfMemoryManagerResultOtherError;
  }
  // discarding WasmHeap
  if (MEMORY_MANAGER_HANDLE_ID(handle) == 0) {
    ret = EsfMemoryManagerFreeWasm(handle, exec_env);
  } else {  // discarding LargeHeap,DMA
    // discarding memory management information
    ret = EsfMemoryManagerDiscardMemoryInfo(handle);
  }
  if (ret != kEsfMemoryManagerResultSuccess) {
    // TODO: Error log.
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] failed(%d)", __FILE__,
                     __LINE__, __func__, ret);
  }

  pthread_mutex_unlock(&s_memory_manager_mutex);
  return ret;
}
/* ------------------------------------------------------------------------- */
EsfMemoryManagerResult EsfMemoryManagerMap(EsfMemoryManagerHandle handle,
                                           const wasm_exec_env_t *exec_env,
                                           int32_t size, void **address) {
  EsfMemoryManagerResult ret = kEsfMemoryManagerResultSuccess;

  // check parameters.
  if ((address == (void **)NULL) || (size <= 0)) {
    // TODO: Error log.
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] failed(%d)", __FILE__,
                     __LINE__, __func__, kEsfMemoryManagerResultParamError);
    return kEsfMemoryManagerResultParamError;
  }
  *address = (void *)NULL;

  int32_t err = pthread_mutex_lock(&s_memory_manager_mutex);
  if (err != 0) {
    // Elog (telemetry) output
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] mutex_lock failed", __FILE__,
                     __LINE__, __func__);
    return kEsfMemoryManagerResultOtherError;
  }
  // initialized Check.
  if (!s_is_initialized) {
    pthread_mutex_unlock(&s_memory_manager_mutex);
    // TODO: Error log.
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] not initialized", __FILE__,
                     __LINE__, __func__);
    return kEsfMemoryManagerResultOtherError;
  }

  // Map WasmHeap
  if (MEMORY_MANAGER_HANDLE_ID(handle) == 0) {
    ret = EsfMemoryManagerMapWasm(handle, size, exec_env, address);
  } else {  // Map LargeHeap,DMA
    ret = EsfMemoryManagerMapMemoryInfo(handle, size, address);
  }
  if (ret != kEsfMemoryManagerResultSuccess) {
    // TODO: Error log.
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] failed(%d)", __FILE__,
                     __LINE__, __func__, ret);
  }

  pthread_mutex_unlock(&s_memory_manager_mutex);
  return ret;
}
/* ------------------------------------------------------------------------- */
EsfMemoryManagerResult EsfMemoryManagerUnmap(EsfMemoryManagerHandle handle,
                                             void **address) {
  EsfMemoryManagerResult ret = kEsfMemoryManagerResultSuccess;

  // check parameters.
  // initialized Check.
  int32_t err = pthread_mutex_lock(&s_memory_manager_mutex);
  if (err != 0) {
    // Elog (telemetry) output
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] mutex_lock failed", __FILE__,
                     __LINE__, __func__);
    return kEsfMemoryManagerResultOtherError;
  }
  if (!s_is_initialized) {
    pthread_mutex_unlock(&s_memory_manager_mutex);
    // TODO: Error log.
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] not initialized", __FILE__,
                     __LINE__, __func__);
    return kEsfMemoryManagerResultOtherError;
  }
  // Unmap WasmHeap
  if (MEMORY_MANAGER_HANDLE_ID(handle) == 0) {
    ret = EsfMemoryManagerUnmapWasm(handle, address);
  } else {  // Unmap LargeHeap,DMA
    ret = EsfMemoryManagerUnmapMemoryInfo(handle);
  }
  if (ret != kEsfMemoryManagerResultSuccess) {
    // TODO: Error log.
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] failed(%d)", __FILE__,
                     __LINE__, __func__, ret);
  } else {
    if (address != (void **)NULL) {
      *address = (void *)NULL;
    }
  }
  pthread_mutex_unlock(&s_memory_manager_mutex);
  return ret;
}
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
EsfMemoryManagerResult EsfMemoryManagerFopen(EsfMemoryManagerHandle handle) {
  EsfMemoryManagerResult ret = kEsfMemoryManagerResultSuccess;
  EsfMemoryManagerFileIoFseekParam seek_param = {0, SEEK_SET, (off_t *)NULL};
  off_t result_offset = 0;

  int32_t err = pthread_mutex_lock(&s_memory_manager_mutex);
  if (err != 0) {
    // Elog (telemetry) output
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] mutex_lock failed", __FILE__,
                     __LINE__, __func__);
    return kEsfMemoryManagerResultOtherError;
  }
  // initialized Check.
  if (!s_is_initialized) {
    pthread_mutex_unlock(&s_memory_manager_mutex);
    // TODO: Error log.
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] not initialized", __FILE__,
                     __LINE__, __func__);

    return kEsfMemoryManagerResultOtherError;
  }
  // Handle check : WasmHeap?
  if (MEMORY_MANAGER_HANDLE_ID(handle) == 0) {
    // WasmHeap does not support FileIO functions
    ret = kEsfMemoryManagerResultNotSupport;
  } else {  // Handle check : LargeHeap,DMA?
    ret = EsfMemoryManagerFileIoFopen(handle, MEMORY_MANAGER_OPEN_MAX_SIZE);
  }
  pthread_mutex_unlock(&s_memory_manager_mutex);

  // Seek position initialization
  // If Fopen() is successful, set the seek position to the beginning of the
  // handle.
  if (ret == kEsfMemoryManagerResultSuccess) {
    // Seek parameter settings
    seek_param.offset = 0;
    seek_param.whence = SEEK_SET;
    seek_param.result_offset = &result_offset;
    // WasmHeap does not support FileIO functions
    ret = EsfMemoryManagerFileIoFseek(handle, &seek_param);
  }

  if (ret != kEsfMemoryManagerResultSuccess) {
    // TODO: Error log.
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] failed(%d)", __FILE__,
                     __LINE__, __func__, ret);
  }
  return ret;
}
/* ------------------------------------------------------------------------- */
EsfMemoryManagerResult EsfMemoryManagerFclose(EsfMemoryManagerHandle handle) {
  EsfMemoryManagerResult ret = kEsfMemoryManagerResultSuccess;
  int32_t err = pthread_mutex_lock(&s_memory_manager_mutex);
  if (err != 0) {
    // Elog (telemetry) output
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] mutex_lock failed", __FILE__,
                     __LINE__, __func__);
    return kEsfMemoryManagerResultOtherError;
  }
  // initialized Check.
  if (!s_is_initialized) {
    pthread_mutex_unlock(&s_memory_manager_mutex);
    // TODO: Error log.
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] not initialized", __FILE__,
                     __LINE__, __func__);
    return kEsfMemoryManagerResultOtherError;
  }
  pthread_mutex_unlock(&s_memory_manager_mutex);
  // Handle check : WasmHeap?
  if (MEMORY_MANAGER_HANDLE_ID(handle) == 0) {
    // WasmHeap does not support FileIO functions
    ret = kEsfMemoryManagerResultNotSupport;
  } else {  // Handle check : LargeHeap,DMA?
    ret = EsfMemoryManagerFileIoFclose(handle);
  }
  if (ret != kEsfMemoryManagerResultSuccess) {
    // TODO: Error log.
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] failed(%d)", __FILE__,
                     __LINE__, __func__, ret);
  }
  return ret;
}
/* ------------------------------------------------------------------------- */
EsfMemoryManagerResult EsfMemoryManagerFseek(EsfMemoryManagerHandle handle,
                                             off_t offset, int whence,
                                             off_t *result_offset) {
  EsfMemoryManagerResult ret = kEsfMemoryManagerResultSuccess;
  EsfMemoryManagerFileIoFseekParam seek_param = {0, SEEK_SET, (off_t *)NULL};
  // check parameters.
  if (result_offset == (off_t *)NULL) {
    // TODO: Error log.
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] failed(%d)", __FILE__,
                     __LINE__, __func__, kEsfMemoryManagerResultParamError);
    return kEsfMemoryManagerResultParamError;
  }
  *result_offset = 0;

  int32_t err = pthread_mutex_lock(&s_memory_manager_mutex);
  if (err != 0) {
    // Elog (telemetry) output
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] mutex_lock failed", __FILE__,
                     __LINE__, __func__);
    return kEsfMemoryManagerResultOtherError;
  }
  // initialized Check.
  if (!s_is_initialized) {
    pthread_mutex_unlock(&s_memory_manager_mutex);
    // TODO: Error log.
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] not initialized", __FILE__,
                     __LINE__, __func__);
    return kEsfMemoryManagerResultOtherError;
  }
  pthread_mutex_unlock(&s_memory_manager_mutex);

  // Seek parameter settings
  seek_param.offset = offset;
  seek_param.whence = whence;
  seek_param.result_offset = result_offset;

  // Handle check : WasmHeap?
  if (MEMORY_MANAGER_HANDLE_ID(handle) == 0) {
    // WasmHeap does not support FileIO functions
    ret = kEsfMemoryManagerResultNotSupport;
  } else {  // Handle check : LargeHeap,DMA?
    ret = EsfMemoryManagerFileIoFseek(handle, &seek_param);
  }
  if (ret != kEsfMemoryManagerResultSuccess) {
    // TODO: Error log.
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] failed(%d)", __FILE__,
                     __LINE__, __func__, ret);
  }
  return ret;
}
/* ------------------------------------------------------------------------- */
EsfMemoryManagerResult EsfMemoryManagerFwrite(EsfMemoryManagerHandle handle,
                                              const void *buff, size_t size,
                                              size_t *rsize) {
  EsfMemoryManagerResult ret = kEsfMemoryManagerResultSuccess;
  EsfMemoryManagerFileIoAccessParam write_param = {(void *)NULL, 0,
                                                   (size_t *)NULL};
  // check parameters.
  if ((buff == (void *)NULL) || (size == 0) || (rsize == (size_t *)NULL)) {
    // TODO: Error log.
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] failed(%d)", __FILE__,
                     __LINE__, __func__, kEsfMemoryManagerResultParamError);
    return kEsfMemoryManagerResultParamError;
  }
  *rsize = 0;

  int32_t err = pthread_mutex_lock(&s_memory_manager_mutex);
  if (err != 0) {
    // Elog (telemetry) output
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] mutex_lock failed", __FILE__,
                     __LINE__, __func__);
    return kEsfMemoryManagerResultOtherError;
  }
  // initialized Check.
  if (!s_is_initialized) {
    pthread_mutex_unlock(&s_memory_manager_mutex);
    // TODO: Error log.
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] not initialized", __FILE__,
                     __LINE__, __func__);
    return kEsfMemoryManagerResultOtherError;
  }
  pthread_mutex_unlock(&s_memory_manager_mutex);

  // Write parameter settings
  write_param.buff = (void *)buff;
  write_param.size = size;
  write_param.rsize = rsize;

  // Handle check : WasmHeap?
  if (MEMORY_MANAGER_HANDLE_ID(handle) == 0) {
    // WasmHeap does not support FileIO functions
    ret = kEsfMemoryManagerResultNotSupport;
  } else {  // Handle check : LargeHeap,DMA?
    ret = EsfMemoryManagerFileIoFwriteFread(
        handle, kEsfMemoryManagerFileIoFwrite, &write_param);
  }
  if (ret != kEsfMemoryManagerResultSuccess) {
    // TODO: Error log.
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] failed(%d)", __FILE__,
                     __LINE__, __func__, ret);
  }
  return ret;
}
/* ------------------------------------------------------------------------- */
EsfMemoryManagerResult EsfMemoryManagerFpwrite(EsfMemoryManagerHandle handle,
                                               const void *buff, size_t size,
                                               off_t offset, size_t *rsize) {
  EsfMemoryManagerResult ret = kEsfMemoryManagerResultSuccess;
  EsfMemoryManagerFileIoFseekParam seek_param = {0, SEEK_SET, (off_t *)NULL};
  EsfMemoryManagerFileIoAccessParam write_param = {(void *)NULL, 0,
                                                   (size_t *)NULL};
  off_t result_offset = 0;
  // check parameters.
  if ((buff == (void *)NULL) || (size == 0) || (rsize == (size_t *)NULL) ||
      (offset < 0)) {
    // TODO: Error log.
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] failed(%d)", __FILE__,
                     __LINE__, __func__, kEsfMemoryManagerResultParamError);
    return kEsfMemoryManagerResultParamError;
  }
  *rsize = 0;

  int32_t err = pthread_mutex_lock(&s_memory_manager_mutex);
  if (err != 0) {
    // Elog (telemetry) output
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] mutex_lock failed", __FILE__,
                     __LINE__, __func__);
    return kEsfMemoryManagerResultOtherError;
  }
  // initialized Check.
  if (!s_is_initialized) {
    pthread_mutex_unlock(&s_memory_manager_mutex);
    // TODO: Error log.
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] not initialized", __FILE__,
                     __LINE__, __func__);
    return kEsfMemoryManagerResultOtherError;
  }
  pthread_mutex_unlock(&s_memory_manager_mutex);

  // Seek parameter settings
  seek_param.offset = offset;
  seek_param.whence = SEEK_SET;
  seek_param.result_offset = &result_offset;

  // Write parameter settings
  write_param.buff = (void *)buff;
  write_param.size = size;
  write_param.rsize = rsize;

  // Handle check : WasmHeap?
  if (MEMORY_MANAGER_HANDLE_ID(handle) == 0) {
    // WasmHeap does not support FileIO functions
    ret = kEsfMemoryManagerResultNotSupport;
  } else {  // Handle check : LargeHeap,DMA?
    ret = EsfMemoryManagerFileIoPwritePread(
        handle, kEsfMemoryManagerFileIoFwrite, &seek_param, &write_param);
  }
  if (ret != kEsfMemoryManagerResultSuccess) {
    // TODO: Error log.
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] failed(%d)", __FILE__,
                     __LINE__, __func__, ret);
  }
  return ret;
}
/* ------------------------------------------------------------------------- */
EsfMemoryManagerResult EsfMemoryManagerFread(EsfMemoryManagerHandle handle,
                                             void *buff, size_t size,
                                             size_t *rsize) {
  EsfMemoryManagerResult ret = kEsfMemoryManagerResultSuccess;
  EsfMemoryManagerFileIoAccessParam read_param = {(void *)NULL, 0,
                                                  (size_t *)NULL};
  // check parameters.
  if ((buff == (void *)NULL) || (size == 0) || (rsize == (size_t *)NULL)) {
    // TODO: Error log.
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] failed(%d)", __FILE__,
                     __LINE__, __func__, kEsfMemoryManagerResultParamError);
    return kEsfMemoryManagerResultParamError;
  }
  *rsize = 0;

  int32_t err = pthread_mutex_lock(&s_memory_manager_mutex);
  if (err != 0) {
    // Elog (telemetry) output
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] mutex_lock failed", __FILE__,
                     __LINE__, __func__);
    return kEsfMemoryManagerResultOtherError;
  }
  // initialized Check.
  if (!s_is_initialized) {
    pthread_mutex_unlock(&s_memory_manager_mutex);
    // TODO: Error log.
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] not initialized", __FILE__,
                     __LINE__, __func__);
    return kEsfMemoryManagerResultOtherError;
  }
  pthread_mutex_unlock(&s_memory_manager_mutex);

  // Write parameter settings
  read_param.buff = (void *)buff;
  read_param.size = size;
  read_param.rsize = rsize;

  // Handle check : WasmHeap?
  if (MEMORY_MANAGER_HANDLE_ID(handle) == 0) {
    // WasmHeap does not support FileIO functions
    ret = kEsfMemoryManagerResultNotSupport;
  } else {  // Handle check : LargeHeap,DMA?
    ret = EsfMemoryManagerFileIoFwriteFread(
        handle, kEsfMemoryManagerFileIoFread, &read_param);
  }
  if (ret != kEsfMemoryManagerResultSuccess) {
    // TODO: Error log.
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] failed(%d)", __FILE__,
                     __LINE__, __func__, ret);
  }
  return ret;
}
/* ------------------------------------------------------------------------- */
EsfMemoryManagerResult EsfMemoryManagerFpread(EsfMemoryManagerHandle handle,
                                              void *buff, size_t size,
                                              off_t offset, size_t *rsize) {
  EsfMemoryManagerResult ret = kEsfMemoryManagerResultSuccess;
  EsfMemoryManagerFileIoFseekParam seek_param = {0, SEEK_SET, (off_t *)NULL};
  EsfMemoryManagerFileIoAccessParam read_param = {(void *)NULL, 0,
                                                  (size_t *)NULL};
  off_t result_offset = 0;
  // check parameters.
  if ((buff == (void *)NULL) || (size == 0) || (rsize == (size_t *)NULL) ||
      (offset < 0)) {
    // TODO: Error log.
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] failed(%d)", __FILE__,
                     __LINE__, __func__, kEsfMemoryManagerResultParamError);
    return kEsfMemoryManagerResultParamError;
  }
  *rsize = 0;

  int32_t err = pthread_mutex_lock(&s_memory_manager_mutex);
  if (err != 0) {
    // Elog (telemetry) output
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] mutex_lock failed", __FILE__,
                     __LINE__, __func__);
    return kEsfMemoryManagerResultOtherError;
  }
  // initialized Check.
  if (!s_is_initialized) {
    pthread_mutex_unlock(&s_memory_manager_mutex);
    // TODO: Error log.
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] not initialized", __FILE__,
                     __LINE__, __func__);
    return kEsfMemoryManagerResultOtherError;
  }
  pthread_mutex_unlock(&s_memory_manager_mutex);

  // Seek parameter settings
  seek_param.offset = offset;
  seek_param.whence = SEEK_SET;
  seek_param.result_offset = &result_offset;

  // Write parameter settings
  read_param.buff = (void *)buff;
  read_param.size = size;
  read_param.rsize = rsize;

  // Handle check : WasmHeap?
  if (MEMORY_MANAGER_HANDLE_ID(handle) == 0) {
    // WasmHeap does not support FileIO functions
    ret = kEsfMemoryManagerResultNotSupport;
  } else {  // Handle check : LargeHeap,DMA?
    ret = EsfMemoryManagerFileIoPwritePread(
        handle, kEsfMemoryManagerFileIoFread, &seek_param, &read_param);
  }
  if (ret != kEsfMemoryManagerResultSuccess) {
    // TODO: Error log.
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] failed(%d)", __FILE__,
                     __LINE__, __func__, ret);
  }
  return ret;
}
/* ------------------------------------------------------------------------- */
EsfMemoryManagerResult EsfMemoryManagerIsMapSupport(
    EsfMemoryManagerHandle handle, EsfMemoryManagerMapSupport *support) {
  EsfMemoryManagerResult ret = kEsfMemoryManagerResultSuccess;
  // check parameters.
  if (support == (EsfMemoryManagerMapSupport *)NULL) {
    // TODO: Error log.
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] failed(%d)", __FILE__,
                     __LINE__, __func__, kEsfMemoryManagerResultParamError);
    return kEsfMemoryManagerResultParamError;
  }
  *support = kEsfMemoryManagerMapIsSupport;

  int32_t err = pthread_mutex_lock(&s_memory_manager_mutex);
  if (err != 0) {
    // Elog (telemetry) output
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] mutex_lock failed", __FILE__,
                     __LINE__, __func__);
    return kEsfMemoryManagerResultOtherError;
  }
  // initialized Check.
  if (!s_is_initialized) {
    pthread_mutex_unlock(&s_memory_manager_mutex);
    // TODO: Error log.
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] not initialized", __FILE__,
                     __LINE__, __func__);
    return kEsfMemoryManagerResultOtherError;
  }
  // Handle check : WasmHeap?
  if (MEMORY_MANAGER_HANDLE_ID(handle) == 0) {
    // WasmHeap does not support FileIO functions
    *support = kEsfMemoryManagerMapIsSupport;
  } else {  // Handle check : LargeHeap,DMA?
    ret = EsfMemoryManagerFileIoIsMapSupport(handle, support);
  }
  if (ret != kEsfMemoryManagerResultSuccess) {
    // TODO: Error log.
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] failed(%d)", __FILE__,
                     __LINE__, __func__, ret);
  }
  pthread_mutex_unlock(&s_memory_manager_mutex);
  return ret;
}
/* ------------------------------------------------------------------------- */
EsfMemoryManagerResult EsfMemoryManagerGetHandleInfo(
    uint32_t handle, EsfMemoryManagerHandleInfo *info) {
  EsfMemoryManagerResult ret = kEsfMemoryManagerResultSuccess;
  // check parameters.
  if (info == (EsfMemoryManagerHandleInfo *)NULL) {
    // TODO: Error log.
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] failed(%d)", __FILE__,
                     __LINE__, __func__, kEsfMemoryManagerResultParamError);
    return kEsfMemoryManagerResultParamError;
  }
  // Handle Info initialize
  info->target_area = kEsfMemoryManagerTargetOtherHeap;
  info->allocate_size = 0;

  // Log handle information for debugging
  WRITE_DLOG_DEBUG(
      MODULE_ID_SYSTEM,
      "%s-%d:[%s] handle=0x%08x, handle_id=%d, handle_offset=0x%08x", __FILE__,
      __LINE__, __func__, handle,
      MEMORY_MANAGER_HANDLE_ID((EsfMemoryManagerHandle)handle),
      MEMORY_MANAGER_HANDLE_OFFSET((EsfMemoryManagerHandle)handle));

  int32_t err = pthread_mutex_lock(&s_memory_manager_mutex);
  if (err != 0) {
    // Elog (telemetry) output
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] mutex_lock failed", __FILE__,
                     __LINE__, __func__);
    return kEsfMemoryManagerResultOtherError;
  }
  // initialized Check.
  if (!s_is_initialized) {
    pthread_mutex_unlock(&s_memory_manager_mutex);
    // TODO: Error log.
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] not initialized", __FILE__,
                     __LINE__, __func__);
    return kEsfMemoryManagerResultOtherError;
  }
  ret = kEsfMemoryManagerResultOtherError;
  // Handle check : WasmHeap?
  if (MEMORY_MANAGER_HANDLE_ID((EsfMemoryManagerHandle)handle) == 0) {
    if (MEMORY_MANAGER_HANDLE_OFFSET((EsfMemoryManagerHandle)handle) != 0) {
      // If handle_id is 0 and handle_offset is non-zero, it is the WasmHeap
      // area.
      info->target_area = kEsfMemoryManagerTargetWasmHeap;
      info->allocate_size = 0;
      ret = kEsfMemoryManagerResultSuccess;
    }
  } else {  // Handle check : LargeHeap,DMA or OtherArea ?
    ret = EsfMemoryManagerSetHandleInfo((EsfMemoryManagerHandle)handle, info);
  }

  if (ret != kEsfMemoryManagerResultSuccess) {
    // TODO: Error log.
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] failed(%d)", __FILE__,
                     __LINE__, __func__, ret);
  }
  pthread_mutex_unlock(&s_memory_manager_mutex);
  return ret;
}
/* ------------------------------------------------------------------------- */
EsfMemoryManagerAppMemory EsfMemoryManagerWasmAllocate(
    EsfMemoryManagerWasmMemoryUsage usage, int32_t size) {
  EsfMemoryManagerAppMemory app_memory = (EsfMemoryManagerAppMemory)NULL;
  PlAppmemUsage pl_usage = kPlAppHeap;
  // check parameters.
  if (size <= 0) {
    // TODO: Error log.
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] failed(%d)", __FILE__,
                     __LINE__, __func__, kEsfMemoryManagerResultParamError);
    return app_memory;
  }
  int32_t err = pthread_mutex_lock(&s_memory_manager_mutex);
  if (err != 0) {
    // Elog (telemetry) output
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] mutex_lock failed", __FILE__,
                     __LINE__, __func__);
    return app_memory;
  }
  switch (usage) {
    case kEsfMemoryManagerWasmAllocForRuntime:
      pl_usage = kPlAppHeap;
      break;
    case kEsfMemoryManagerWasmAllocForLinearMemory:
      pl_usage = kPlAppLinearMemory;
      break;
    default:
      // TODO: Error log.
      pthread_mutex_unlock(&s_memory_manager_mutex);
      return app_memory;
  }
  // memory allocate
  app_memory = (EsfMemoryManagerAppMemory)PlAppmemMalloc(pl_usage,
                                                         (uint32_t)size);
  if (app_memory == (EsfMemoryManagerAppMemory)NULL) {
    // TODO: Error log.
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] failed(%d)", __FILE__,
                     __LINE__, __func__,
                     kEsfMemoryManagerResultAllocationError);
  }
  pthread_mutex_unlock(&s_memory_manager_mutex);
  return app_memory;
}
/* ------------------------------------------------------------------------- */
EsfMemoryManagerAppMemory EsfMemoryManagerWasmReallocate(
    EsfMemoryManagerWasmMemoryUsage usage, EsfMemoryManagerAppMemory old_memory,
    int32_t size) {
  EsfMemoryManagerAppMemory app_memory = (EsfMemoryManagerAppMemory)NULL;
  PlAppmemUsage pl_usage = kPlAppHeap;
  // check parameters.
  if ((size <= 0) || (old_memory == (EsfMemoryManagerAppMemory)NULL)) {
    // TODO: Error log.
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] failed(%d)", __FILE__,
                     __LINE__, __func__, kEsfMemoryManagerResultParamError);
    return app_memory;
  }
  int32_t err = pthread_mutex_lock(&s_memory_manager_mutex);
  if (err != 0) {
    // Elog (telemetry) output
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] mutex_lock failed", __FILE__,
                     __LINE__, __func__);
    return app_memory;
  }
  switch (usage) {
    case kEsfMemoryManagerWasmAllocForRuntime:
      pl_usage = kPlAppHeap;
      break;
    case kEsfMemoryManagerWasmAllocForLinearMemory:
      pl_usage = kPlAppLinearMemory;
      break;
    default:
      // TODO: Error log.
      pthread_mutex_unlock(&s_memory_manager_mutex);
      return app_memory;
  }
  // memory reallocate.
  app_memory = (EsfMemoryManagerAppMemory)PlAppmemRealloc(
      pl_usage, (PlAppMemory)old_memory, (uint32_t)size);
  if (app_memory == (EsfMemoryManagerAppMemory)NULL) {
    // TODO: Error log.
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] failed(%d)", __FILE__,
                     __LINE__, __func__,
                     kEsfMemoryManagerResultAllocationError);
  }
  pthread_mutex_unlock(&s_memory_manager_mutex);
  return app_memory;
}
/* ------------------------------------------------------------------------- */
void EsfMemoryManagerWasmFree(EsfMemoryManagerWasmMemoryUsage usage,
                              EsfMemoryManagerAppMemory memory) {
  PlAppmemUsage pl_usage = kPlAppHeap;
  // check parameters.
  if (memory == (EsfMemoryManagerAppMemory)NULL) {
    // TODO: Error log.
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] failed(%d)", __FILE__,
                     __LINE__, __func__, kEsfMemoryManagerResultParamError);
    return;
  }
  int32_t err = pthread_mutex_lock(&s_memory_manager_mutex);
  if (err != 0) {
    // Elog (telemetry) output
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] mutex_lock failed", __FILE__,
                     __LINE__, __func__);
    return;
  }
  switch (usage) {
    case kEsfMemoryManagerWasmAllocForRuntime:
      pl_usage = kPlAppHeap;
      break;
    case kEsfMemoryManagerWasmAllocForLinearMemory:
      pl_usage = kPlAppLinearMemory;
      break;
    default:
      // TODO: Error log.
      pthread_mutex_unlock(&s_memory_manager_mutex);
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:[%s] failed(%d)", __FILE__,
                       __LINE__, __func__, kEsfMemoryManagerResultParamError);
      return;
  }
  // memory free.
  PlAppmemFree(pl_usage, (PlAppMemory)memory);
  pthread_mutex_unlock(&s_memory_manager_mutex);
  return;
}
/* ------------------------------------------------------------------------- */
