/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef _PL_DMAMEM_CAM_IMPL_H_
#define _PL_DMAMEM_CAM_IMPL_H_

#include "pl.h"
#include "pl_dmamem.h"

// -----------------------------------------------------------------------------
//  Function Prototypes
// -----------------------------------------------------------------------------
// DmaMemCamImpl API
PlErrCode PlDmaMemInitializeCamImpl(void);
PlErrCode PlDmaMemFinalizeCamImpl(void);
PlDmaMemHandle PlDmaMemAllocCamImpl(uint32_t size);
PlErrCode PlDmaMemFreeCamImpl(PlDmaMemHandle handle);
PlErrCode PlDmaMemMapCamImpl(const PlDmaMemHandle handle, void **vaddr);
PlErrCode PlDmaMemUnmapCamImpl(const void *addr);
PlErrCode PlDmaMemGetMemInfoCamImpl(PlDmaMemInfo *info);
bool PlDmaMemIsValidCamImpl(const PlDmaMemHandle handle);

#endif  /* _PL_DMAMEM_CAM_IMPL_H_ */
