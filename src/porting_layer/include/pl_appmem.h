/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef _PL_APPMEM_H_
#define _PL_APPMEM_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include <stdbool.h>

#include "pl.h"
// -----------------------------------------------------------------------------
//  Pre-processor Definitions
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//  Public Types
// -----------------------------------------------------------------------------
typedef void *PlAppMemory;

typedef enum {
  kPlAppHeap = 0,
  kPlAppLinearMemory
} PlAppmemUsage;

// -----------------------------------------------------------------------------
//  Public Function Prototypes
// -----------------------------------------------------------------------------
// Appmem API
PlErrCode PlAppmemInitialize(void);
PlErrCode PlAppmemFinalize(void);

PlErrCode PlAppmemSetBlock(int32_t div_num);
PlAppMemory PlAppmemMalloc(PlAppmemUsage mem_usage, uint32_t size);
PlAppMemory PlAppmemRealloc(PlAppmemUsage mem_usage, PlAppMemory oldmem,
                              uint32_t size);
void PlAppmemFree(PlAppmemUsage mem_usage, PlAppMemory mem);
PlErrCode PlAppmemMap(const void *native_addr, uint32_t size, void **vaddr);
PlErrCode PlAppmemUnmap(void *vaddr);
PlErrCode PlAppmemPwrite(void *native_addr, const char *buf, uint32_t size,
                         uint32_t offset);
bool PlAppmemIsMapSupport(const PlAppMemory mem);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _PL_APPMEM_H_ */
