/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef _PL_DMAMEM_OS_IMPL_H_
#define _PL_DMAMEM_OS_IMPL_H_

#include "pl.h"
#include "pl_dmamem.h"

// -----------------------------------------------------------------------------
//  Function Prototypes
// -----------------------------------------------------------------------------
// DmaMemOsImpl API
PlErrCode PlDmaMemInitializeOsImpl(void);
PlErrCode PlDmaMemFinalizeOsImpl(void);
PlDmaMemHandle PlDmaMemAllocOsImpl(uint32_t size);
PlErrCode PlDmaMemFreeOsImpl(const PlDmaMemHandle handle);
PlErrCode PlDmaMemMapOsImpl(const PlDmaMemHandle handle, void **vaddr);
PlErrCode PlDmaMemUnmapOsImpl(const void *vaddr);
bool PlDmaMemIsValidOsImpl(const PlDmaMemHandle handle);
PlErrCode PlDmaMemGetMemInfoOsImpl(PlDmaMemInfo *info);

#endif /* _PL_DMAMEM_OS_IMPL_H_ */
