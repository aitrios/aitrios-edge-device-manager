/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

// Includes --------------------------------------------------------------------
#include "pl.h"
#include "pl_appmem.h"
#include "pl_appmem_os_impl.h"

// -----------------------------------------------------------------------------
PlErrCode PlAppmemInitialize(void) {
  return PlAppmemInitializeOsImpl();
}
// -----------------------------------------------------------------------------
PlErrCode PlAppmemFinalize(void) {
  return PlAppmemFinalizeOsImpl();
}
// -----------------------------------------------------------------------------
PlErrCode PlAppmemSetBlock(int32_t div_num) {
  return PlAppmemSetBlockOsImpl(div_num);
}
// -----------------------------------------------------------------------------
PlAppMemory PlAppmemMalloc(PlAppmemUsage mem_usage, uint32_t size) {
  return PlAppmemMallocOsImpl(mem_usage, size);
}
// -----------------------------------------------------------------------------
PlAppMemory PlAppmemRealloc(PlAppmemUsage mem_usage, PlAppMemory oldmem,
                              uint32_t size) {
  return PlAppmemReallocOsImpl(mem_usage, oldmem, size);
}
// -----------------------------------------------------------------------------
void PlAppmemFree(PlAppmemUsage mem_usage, PlAppMemory mem) {
  PlAppmemFreeOsImpl(mem_usage, mem);
}
// -----------------------------------------------------------------------------
PlErrCode PlAppmemMap(const void *native_addr, uint32_t size, void **vaddr) {
  return PlAppmemMapOsImpl(native_addr, size, vaddr);
}
// -----------------------------------------------------------------------------
PlErrCode PlAppmemUnmap(void *vaddr) {
  return PlAppmemUnmapOsImpl(vaddr);
}
// -----------------------------------------------------------------------------
PlErrCode PlAppmemPwrite(void *native_addr, const char *buf, uint32_t size,
                         uint32_t offset) {
  return PlAppmemPwriteOsImpl(native_addr, buf, size, offset);
}
// -----------------------------------------------------------------------------
bool PlAppmemIsMapSupport(const PlAppMemory mem) {
  return PlAppmemIsMapSupportOsImpl(mem);
}
// -----------------------------------------------------------------------------
