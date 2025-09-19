/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#include "pl_storage.h"
#include "pl_storage_os_impl.h"

// ----------------------------------------------------------------------------
PlErrCode PlStorageOpen(PlStorageDataId id, int oflags,
                        PlStorageHandle *handle) {
  return PlStorageOpenOsImpl(id, oflags, handle);
}
// ----------------------------------------------------------------------------
PlErrCode PlStorageClose(const PlStorageHandle handle) {
  return PlStorageCloseOsImpl(handle);
}
// ----------------------------------------------------------------------------
PlErrCode PlStorageSeek(const PlStorageHandle handle, int32_t offset,
                        PlStorageSeekType type, int32_t *cur_pos) {
  return PlStorageSeekOsImpl(handle, offset, type, cur_pos);
}
// ----------------------------------------------------------------------------
PlErrCode PlStorageRead(const PlStorageHandle handle, void *out_buf,
                        uint32_t read_size, uint32_t *out_size) {
  return PlStorageReadOsImpl(handle, out_buf, read_size, out_size);
}
// ----------------------------------------------------------------------------
PlErrCode PlStorageWrite(const PlStorageHandle handle, const void *src_buf,
                         uint32_t write_size, uint32_t *out_size) {
  return PlStorageWriteOsImpl(handle, src_buf, write_size, out_size);
}
// ----------------------------------------------------------------------------
PlErrCode PlStorageErase(PlStorageDataId id) {
  return PlStorageEraseOsImpl(id);
}
// ----------------------------------------------------------------------------
PlErrCode PlStorageFactoryReset(PlStorageDataId id) {
  return PlStorageFactoryResetOsImpl(id);
}
// ----------------------------------------------------------------------------
PlErrCode PlStorageDRead(PlStorageDataId id, int oflags, void *out_buf,
                         uint32_t read_size, uint32_t *out_size) {
  return PlStorageDReadOsImpl(id, oflags, out_buf, read_size, out_size);
}
// ----------------------------------------------------------------------------
PlErrCode PlStorageDWrite(PlStorageDataId id, int oflags, const void *src_buf,
                          uint32_t write_size, uint32_t *out_size) {
  return PlStorageDWriteOsImpl(id, oflags, src_buf, write_size, out_size);
}
// ----------------------------------------------------------------------------
PlErrCode PlStorageGetDataInfo(PlStorageDataId id, PlStorageDataInfo *info) {
  return PlStorageGetDataInfoOsImpl(id, info);
}
// ----------------------------------------------------------------------------
PlErrCode PlStorageSwitchData(PlStorageTmpDataId src_id,
                              PlStorageDataId dst_id) {
  return PlStorageSwitchDataOsImpl(src_id, dst_id);
}
// ----------------------------------------------------------------------------
PlErrCode PlStorageGetTmpDataId(PlStorageDataId src_id,
                                PlStorageTmpDataId *tmp_id) {
  return PlStorageGetTmpDataIdOsImpl(src_id, tmp_id);
}
// ----------------------------------------------------------------------------
PlErrCode PlStorageGetCapabilities(PlStorageCapabilities *capabilities) {
  return PlStorageGetCapabilitiesOsImpl(capabilities);
}
// ----------------------------------------------------------------------------
PlErrCode PlStorageGetIdCapabilities(PlStorageDataId id,
                                     PlStorageIdCapabilities *id_capabilities) {
  return PlStorageGetIdCapabilitiesOsImpl(id, id_capabilities);
}
// ----------------------------------------------------------------------------
PlErrCode PlStorageInitialize(void) {
  return PlStorageInitializeOsImpl();
}
// ----------------------------------------------------------------------------
PlErrCode PlStorageFinalize(void) {
  return PlStorageFinalizeOsImpl();
}
// ----------------------------------------------------------------------------
PlErrCode PlStorageClean(void) {
  return PlStorageCleanOsImpl();
}
// ----------------------------------------------------------------------------
PlErrCode PlStorageDowngrade(void) {
  return PlStorageDowngradeOsImpl();
}
// ----------------------------------------------------------------------------
