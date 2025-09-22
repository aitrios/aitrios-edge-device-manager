/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

// Includes --------------------------------------------------------------------
#include <sys/types.h>

#include "pl.h"
#include "pl_lheap.h"
#include "pl_lheap_os_impl.h"

// Macros ----------------------------------------------------------------------

// Global Variables ------------------------------------------------------------

// External functions ----------------------------------------------------------
PlErrCode PlLheapInitialize(void) {
  return PlLheapInitializeOsImpl();
}

// -----------------------------------------------------------------------------
PlErrCode PlLheapFinalize(void) {
  return PlLheapFinalizeOsImpl();
}

// -----------------------------------------------------------------------------
PlLheapHandle PlLheapAlloc(uint32_t size) {
  return PlLheapAllocOsImpl(size);
}

// -----------------------------------------------------------------------------
PlErrCode PlLheapFree(PlLheapHandle handle) {
  return PlLheapFreeOsImpl(handle);
}

// -----------------------------------------------------------------------------
PlErrCode PlLheapMap(const PlLheapHandle handle, void **vaddr) {
  return PlLheapMapOsImpl(handle, vaddr);
}

// -----------------------------------------------------------------------------
PlErrCode PlLheapUnmap(void *vaddr) {
  return PlLheapUnmapOsImpl(vaddr);
}

// -----------------------------------------------------------------------------
PlErrCode PlLheapGetMeminfo(PlLheapMeminfo *info) {
  return PlLheapGetMeminfoOsImpl(info);
}
// -----------------------------------------------------------------------------
bool PlLheapIsValid(const PlLheapHandle handle) {
  return PlLheapIsValidOsImpl(handle);
}

// -----------------------------------------------------------------------------
PlErrCode PlLheapPwrite(const PlLheapHandle handle, const char *buf,
                            uint32_t count, uint32_t offset) {
  return PlLheapPwriteOsImpl(handle, buf, count, offset);
}

// -----------------------------------------------------------------------------
PlErrCode PlLheapPread(const PlLheapHandle handle, char *buf,
                        uint32_t count, uint32_t offset) {
  return PlLheapPreadOsImpl(handle, buf, count, offset);
}

// -----------------------------------------------------------------------------
PlErrCode PlLheapFopen(const PlLheapHandle handle, int *pfd) {
  return PlLheapFopenOsImpl(handle, pfd);
}
// -----------------------------------------------------------------------------
PlErrCode PlLheapFclose(const PlLheapHandle handle, int fd) {
  return PlLheapFcloseOsImpl(handle, fd);
}
// -----------------------------------------------------------------------------
PlErrCode PlLheapFseek(const PlLheapHandle handle, int fd,
                        off_t offset, int whence, off_t *roffset) {
  return PlLheapFseekOsImpl(handle, fd, offset, whence, roffset);
}
// -----------------------------------------------------------------------------
PlErrCode PlLheapFwrite(const PlLheapHandle handle, int fd,
                        const void *buf, size_t size, size_t *rsize) {
  return PlLheapFwriteOsImpl(handle, fd, buf, size, rsize);
}
// -----------------------------------------------------------------------------
PlErrCode PlLheapFread(const PlLheapHandle handle, int fd, void *buf,
                        size_t size, size_t *rsize) {
  return PlLheapFreadOsImpl(handle, fd, buf, size, rsize);
}
// -----------------------------------------------------------------------------
bool PlLheapIsMapSupport(const PlLheapHandle handle) {
  return PlLheapIsMapSupportOsImpl(handle);
}
// -----------------------------------------------------------------------------
