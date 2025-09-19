/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef _PL_LHEAP_OS_IMPL_H_
#define _PL_LHEAP_OS_IMPL_H_

#include <sys/types.h>

#include "pl.h"
#include "pl_lheap.h"

// -----------------------------------------------------------------------------
//  Function Prototypes
// -----------------------------------------------------------------------------
// LHeapOsImpl API
PlErrCode PlLheapInitializeOsImpl(void);
PlErrCode PlLheapFinalizeOsImpl(void);
PlLheapHandle PlLheapAllocOsImpl(uint32_t size);
PlErrCode PlLheapFreeOsImpl(PlLheapHandle handle);
PlErrCode PlLheapMapOsImpl(const PlLheapHandle handle, void **vaddr);
PlErrCode PlLheapUnmapOsImpl(void *vaddr);
PlErrCode PlLheapGetMeminfoOsImpl(PlLheapMeminfo *info);
bool PlLheapIsValidOsImpl(const PlLheapHandle handle);
PlErrCode PlLheapPwriteOsImpl(const PlLheapHandle handle, const char *buf,
                            uint32_t count, uint32_t offset);
PlErrCode PlLheapPreadOsImpl(const PlLheapHandle handle, char *buf,
                              uint32_t count, uint32_t offset);
PlErrCode PlLheapFopenOsImpl(const PlLheapHandle handle, int *pfd);
PlErrCode PlLheapFcloseOsImpl(const PlLheapHandle handle, int fd);
PlErrCode PlLheapFseekOsImpl(const PlLheapHandle handle, int fd,
                        off_t offset, int whence, off_t *roffset);
PlErrCode PlLheapFwriteOsImpl(const PlLheapHandle handle, int fd,
                        const void *buf, size_t size, size_t *rsize);
PlErrCode PlLheapFreadOsImpl(const PlLheapHandle handle, int fd, void *buf,
                        size_t size, size_t *rsize);
bool PlLheapIsMapSupportOsImpl(const PlLheapHandle handle);

#endif  /* _PL_LHEAP_OS_IMPL_H_ */
