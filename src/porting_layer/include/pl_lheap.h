/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef _PL_LHEAP_H_
#define _PL_LHEAP_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "pl.h"

// -----------------------------------------------------------------------------
//  Pre-processor Definitions
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//  Public Types
// -----------------------------------------------------------------------------
typedef void *PlLheapHandle;

typedef struct {
  uint32_t total;
  uint32_t used;
  uint32_t free;
  uint32_t linear_free;
} PlLheapMeminfo;

// -----------------------------------------------------------------------------
//  Public Function Prototypes
// -----------------------------------------------------------------------------
// Lheap API
PlErrCode PlLheapInitialize(void);
PlErrCode PlLheapFinalize(void);

PlLheapHandle PlLheapAlloc(uint32_t size);
PlErrCode PlLheapFree(PlLheapHandle handle);
PlErrCode PlLheapMap(const PlLheapHandle handle, void **vaddr);
PlErrCode PlLheapUnmap(void *vaddr);
PlErrCode PlLheapGetMeminfo(PlLheapMeminfo *info);
bool PlLheapIsValid(const PlLheapHandle handle);
PlErrCode PlLheapPwrite(const PlLheapHandle handle, const char *buf,
                            uint32_t size, uint32_t offset);
PlErrCode PlLheapFopen(const PlLheapHandle handle, int *pfd);
PlErrCode PlLheapFclose(const PlLheapHandle handle, int fd);
PlErrCode PlLheapFseek(const PlLheapHandle handle, int fd,
                        off_t offset, int whence, off_t *roffset);
PlErrCode PlLheapFwrite(const PlLheapHandle handle, int fd,
                        const void *buf, size_t size, size_t *rsize);
PlErrCode PlLheapFread(const PlLheapHandle handle, int fd, void *buf,
                        size_t size, size_t *rsize);
bool PlLheapIsMapSupport(const PlLheapHandle handle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _PL_LEHAP_H_ */
