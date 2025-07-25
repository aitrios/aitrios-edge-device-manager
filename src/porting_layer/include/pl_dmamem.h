/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef _PL_DMAMEM_H_
#define _PL_DMAMEM_H_

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
typedef void* PlDmaMemHandle;

typedef struct {
  uint32_t total_bytes;
  uint32_t used_bytes;
  uint32_t free_bytes;
  uint32_t free_linear_bytes;
} PlDmaMemInfo;

// -----------------------------------------------------------------------------
//  Public Function Prototypes
// -----------------------------------------------------------------------------
// Appmem API
PlErrCode       PlDmaMemInitialize(void);
PlErrCode       PlDmaMemFinalize(void);
PlDmaMemHandle  PlDmaMemAlloc(uint32_t size);
PlErrCode       PlDmaMemFree(const PlDmaMemHandle handle);
PlErrCode       PlDmaMemMap(const PlDmaMemHandle handle, void **vaddr);
PlErrCode       PlDmaMemUnmap(const void *vaddr);
bool              PlDmaMemIsValid(const PlDmaMemHandle handle);
PlErrCode       PlDmaMemGetMemInfo(PlDmaMemInfo *info);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _PL_DMAMEM_H_ */
