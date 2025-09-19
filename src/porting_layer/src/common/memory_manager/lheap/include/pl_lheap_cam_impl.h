/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef _PL_LHEAP_CAM_IMPL_H_
#define _PL_LHEAP_CAM_IMPL_H_

#include "pl.h"
#include "pl_lheap.h"

// -----------------------------------------------------------------------------
//  Function Prototypes
// -----------------------------------------------------------------------------
// LHeapCamImpl API
PlErrCode PlLheapInitializeCamImpl(void);
PlErrCode PlLheapFinalizeCamImpl(void);
PlLheapHandle PlLheapAllocCamImpl(uint32_t size);
PlErrCode PlLheapFreeCamImpl(PlLheapHandle handle_);
PlErrCode PlLheapMapCamImpl(const PlLheapHandle handle_, void **vaddr);
PlErrCode PlLheapUnmapCamImpl(void *vaddr);
PlErrCode PlLheapGetMeminfoCamImpl(PlLheapMeminfo *info);
bool PlLheapIsValidCamImpl(const PlLheapHandle handle_);
PlErrCode PlLheapPwriteCamImpl(const PlLheapHandle handle_, const char *buf,
                                uint32_t count, uint32_t offset);
PlErrCode PlLheapPreadCamImpl(const PlLheapHandle handle_, char *buf,
                              uint32_t count, uint32_t offset);
PlErrCode PlLheapFopenCamImpl(const PlLheapHandle handle, int *pfd);
PlErrCode PlLheapFcloseCamImpl(const PlLheapHandle handle, int fd);
PlErrCode PlLheapFseekCamImpl(const PlLheapHandle handle, int fd,
                              off_t offset, int whence, off_t *roffset);
PlErrCode PlLheapFwriteCamImpl(const PlLheapHandle handle, int fd,
                                const void *buf, size_t size, size_t *rsize);
PlErrCode PlLheapFreadCamImpl(const PlLheapHandle handle, int fd, void *buf,
                              size_t size, size_t *rsize);
bool PlLheapIsMapSupportCamImpl(const PlLheapHandle handle);

#endif  /* _PL_LHEAP_CAM_IMPL_H_ */
