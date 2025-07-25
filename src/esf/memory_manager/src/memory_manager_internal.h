/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Define the external internal API for memory manager.

#ifndef ESF_MEMORY_MANAGER_SRC_MEMORY_MANAGER_INTERNAL_H_
#define ESF_MEMORY_MANAGER_SRC_MEMORY_MANAGER_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <sys/types.h>
#include <wasm_export.h>  // for wasm_exec_env_t, wasm_runtim...
#ifndef __NuttX__
#include <bsd/sys/queue.h>
#else
#include <sys/queue.h>
#endif  // __NuttX__

#include "memory_manager.h"
#include "pl.h"
#include "pl_appmem.h"
#include "pl_dmamem.h"
#include "pl_lheap.h"
#include "utility_log.h"
#include "utility_log_module_id.h"

#ifndef MEMORY_MANAGER_REMOVE_STATIC
#ifndef CONFIG_EXTERNAL_TEST_ESF_MEMORY_MANAGER
#define STATIC static
#else  // CONFIG_EXTERNAL_TEST_ESF_MEMORY_MANAGER
#define STATIC
#endif  // CONFIG_EXTERNAL_TEST_ESF_MEMORY_MANAGER
#else   // MEMORY_MANAGER_REMOVE_STATIC
#define STATIC
#endif  // MEMORY_MANAGER_REMOVE_STATIC

// Definition of Elog (telemetry) output for MemoryManager.
// ID Level1
#define ESF_ERROR_MEMORY_MANAGER_ID (0x8900)
// ID Level2
// LargeHeap area initialization failure
#define ESF_ERROR_LARGE_HEAP_INITIALIZE (ESF_ERROR_MEMORY_MANAGER_ID + 0x0000)
// AppMem area initialization failure
#define ESF_ERROR_APP_MEMORY_INITIALIZE (ESF_ERROR_MEMORY_MANAGER_ID + 0x0001)
// AppMem area division number setting failure
#define ESF_ERROR_APP_MEMORY_DIVIDE (ESF_ERROR_MEMORY_MANAGER_ID + 0x0002)
// DMA area initialization failure
#define ESF_ERROR_DMA_INITIALIZE (ESF_ERROR_MEMORY_MANAGER_ID + 0x0003)
// LargeHeap area terminating failure
#define ESF_ERROR_LARGE_HEAP_FINALIZE (ESF_ERROR_MEMORY_MANAGER_ID + 0x0004)
// AppMem area terminating failure
#define ESF_ERROR_APP_MEMORY_FINALIZE (ESF_ERROR_MEMORY_MANAGER_ID + 0x0005)
// DMA area terminating failure
#define ESF_ERROR_DMA_FINALIZE (ESF_ERROR_MEMORY_MANAGER_ID + 0x0006)
// Memory allocation failure
#define ESF_ERROR_MEMORY_ALLOCATE (ESF_ERROR_MEMORY_MANAGER_ID + 0x0007)
// Failed to map memory
#define ESF_ERROR_MEMORY_MAP (ESF_ERROR_MEMORY_MANAGER_ID + 0x0008)
// File IO failure
#define ESF_ERROR_MEMORY_FILEIO (ESF_ERROR_MEMORY_MANAGER_ID + 0x0009)

// Elog (telemetry) output macro
#define ESF_MEMORY_MANAGER_ELOG_ERROR(id) \
  WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (uint16_t)id)

// Definition for unmap(-1)
#define MEMORY_MANAGER_UNMAP (uint64_t)(-1)

// Definition for maximum number of maps.
#define MEMORY_MANAGER_MAX_MAPS (127)

// Definition for Amount of access (read/write) per time. (Unit:byte)
#define MEMORY_MANAGER_ACCESS_MAX_SIZE (4096)

// Definition for Open with the maximum size.  (Unit:byte)
#define MEMORY_MANAGER_OPEN_MAX_SIZE (0)

// This code defines the FileIO operation request.
typedef enum {
  kEsfMemoryManagerFileIoFopen = 0,
  kEsfMemoryManagerFileIoFclose,
  kEsfMemoryManagerFileIoFseek,
  kEsfMemoryManagerFileIoFwrite,
  kEsfMemoryManagerFileIoFread,
  kEsfMemoryManagerFileIoIsMapSupport
} EsfMemoryManagerFileIoRequest;

// A structure that defines the parameter elements of Fseek.
typedef struct EsfMemoryManagerFileIoFseekParam {
  off_t offset;          // seek movement amount
  int whence;            // seek start position
  off_t *result_offset;  // seek position after movement
} EsfMemoryManagerFileIoFseekParam;

// A structure that defines the parameter elements of Fwrite/Fread.
typedef struct EsfMemoryManagerFileIoAccessParam {
  void *buff;     // Write/read Buffer
  size_t size;    // Write/read Size
  size_t *rsize;  // Written/read size
} EsfMemoryManagerFileIoAccessParam;

// Link list definition for map management information.
struct EsfMemoryManagerHandleAddressOffset {
  uint32_t address_offset;       // Area part specified offset
  int32_t size;                  // Area part specified size (for FileIO)
  int file_descriptor;           // File Descriptors (for FileIO)
  int32_t open_count;            // File open count (for FileIO)
  off_t seek_position;           // File seek position (for FileIO)
  pthread_mutex_t handle_mutex;  // FileIO handle(fd) mutex
  SLIST_ENTRY(EsfMemoryManagerHandleAddressOffset) next;
};
SLIST_HEAD(EsfMemoryManagerMapInfoList, EsfMemoryManagerHandleAddressOffset);

// Definition of memory information for LargeHeap/DMA/WasmHeap area.
// Linked list definition of memory management information.
struct EsfMemoryManagerHandleInternal {
  uint32_t link_info;  // Link information with the handle (handle ID)
  EsfMemoryManagerTargetArea target_area;  // target memory area
  uint64_t allocate_address;               // allocated memory start address
  int32_t allocate_size;                   // allocated memory size
  uint64_t map_address;                    // mapped address
  int32_t map_count;                       // mapped count
  int file_descriptor;                     // File Descriptors (for FileIO)
  struct EsfMemoryManagerMapInfoList map_info_list;
  SLIST_ENTRY(EsfMemoryManagerHandleInternal) next;
};
SLIST_HEAD(EsfMemoryManagerHandleInfoList, EsfMemoryManagerHandleInternal);

#ifdef __cplusplus
}
#endif

#endif  // ESF_MEMORY_MANAGER_SRC_MEMORY_MANAGER_INTERNAL_H_
