/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef _PL_APPMEM_OS_IMPL_H_
#define _PL_APPMEM_OS_IMPL_H_

#include "pl.h"
#include "pl_appmem.h"

// -----------------------------------------------------------------------------
//  Function Prototypes
// -----------------------------------------------------------------------------
// AppmemOsImpl API
PlErrCode PlAppmemInitializeOsImpl(void);
PlErrCode PlAppmemFinalizeOsImpl(void);
PlErrCode PlAppmemSetBlockOsImpl(int32_t div_num);
PlAppMemory PlAppmemMallocOsImpl(PlAppmemUsage mem_usage, uint32_t size);
PlAppMemory PlAppmemReallocOsImpl(PlAppmemUsage mem_usage, PlAppMemory oldmem,
                              uint32_t size);
void PlAppmemFreeOsImpl(PlAppmemUsage mem_usage, PlAppMemory mem);
PlErrCode PlAppmemMapOsImpl(const void *native_addr, uint32_t size,
                          void **vaddr);
PlErrCode PlAppmemUnmapOsImpl(void *vaddr);
PlErrCode PlAppmemPwriteOsImpl(void *native_addr, const char *buf,
                         uint32_t size, uint32_t offset);
bool PlAppmemIsMapSupportOsImpl(const PlAppMemory mem);

#endif  /* _PL_APPMEM_OS_IMPL_H_ */
