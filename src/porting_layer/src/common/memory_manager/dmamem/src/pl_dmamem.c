/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

// Includes --------------------------------------------------------------------
#include "pl.h"
#include "pl_dmamem.h"
#include "pl_dmamem_os_impl.h"

// -----------------------------------------------------------------------------
PlErrCode PlDmaMemInitialize(void) {
  return PlDmaMemInitializeOsImpl();
}
// -----------------------------------------------------------------------------
PlErrCode PlDmaMemFinalize(void) {
  return PlDmaMemFinalizeOsImpl();
}
// -----------------------------------------------------------------------------
PlDmaMemHandle PlDmaMemAlloc(uint32_t size) {
  return PlDmaMemAllocOsImpl(size);
}
// -----------------------------------------------------------------------------
PlErrCode PlDmaMemFree(const PlDmaMemHandle handle) {
  return PlDmaMemFreeOsImpl(handle);
}
// -----------------------------------------------------------------------------
PlErrCode PlDmaMemMap(const PlDmaMemHandle handle, void **vaddr) {
  return PlDmaMemMapOsImpl(handle, vaddr);
}
// -----------------------------------------------------------------------------
PlErrCode PlDmaMemUnmap(const void *vaddr) {
  return PlDmaMemUnmapOsImpl(vaddr);
}
// -----------------------------------------------------------------------------
bool PlDmaMemIsValid(const PlDmaMemHandle handle) {
  return PlDmaMemIsValidOsImpl(handle);
}
// -----------------------------------------------------------------------------
PlErrCode PlDmaMemGetMemInfo(PlDmaMemInfo *info) {
  return PlDmaMemGetMemInfoOsImpl(info);
}
// -----------------------------------------------------------------------------
