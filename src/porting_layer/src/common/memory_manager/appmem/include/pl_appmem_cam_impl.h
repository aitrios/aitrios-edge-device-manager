/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef _PL_APPMEM_CAM_IMPL__
#define _PL_APPMEM_CAM_IMPL__

#include "pl.h"
#include "pl_appmem.h"

// -----------------------------------------------------------------------------
//  Function Prototypes
// -----------------------------------------------------------------------------
// AppmemCamImpl API
PlErrCode PlAppmemInitializeCamImpl(void);
PlErrCode PlAppmemFinalizeCamImpl(void);
PlErrCode PlAppmemSetBlockCamImpl(int32_t div_num);
PlAppMemory PlAppmemMallocCamImpl(PlAppmemUsage mem_usage, uint32_t size);
PlAppMemory PlAppmemReallocCamImpl(PlAppmemUsage mem_usage, PlAppMemory oldmem,
                                    uint32_t size);
void PlAppmemFreeCamImpl(PlAppmemUsage mem_usage, PlAppMemory mem);
PlErrCode PlAppmemMapCamImpl(const void *native_addr, uint32_t size,
                              void **vaddr);
PlErrCode PlAppmemUnmapCamImpl(void *vaddr);
PlErrCode PlAppmemPwriteCamImpl(void *native_addr, const char *buf,
                                uint32_t size, uint32_t offset);
bool PlAppmemIsMapSupportCamImpl(const PlAppMemory mem);

#endif  /* _PL_APPMEM_CAM_IMPL__ */
