/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef PORTING_LAYER_INCLUDE_PL_STORAGE_OS_IMPL_H_
#define PORTING_LAYER_INCLUDE_PL_STORAGE_OS_IMPL_H_

#include "pl.h"
#include "pl_storage.h"

// Functions ------------------------------------------------------------------
PlErrCode PlStorageOpenOsImpl(PlStorageDataId id, int oflags,
                              PlStorageHandle *handle);
PlErrCode PlStorageCloseOsImpl(const PlStorageHandle handle);
PlErrCode PlStorageSeekOsImpl(const PlStorageHandle handle, int32_t offset,
                              PlStorageSeekType type, int32_t *cur_pos);
PlErrCode PlStorageReadOsImpl(const PlStorageHandle handle, void *out_buf,
                              uint32_t read_size, uint32_t *out_size);
PlErrCode PlStorageWriteOsImpl(const PlStorageHandle handle,
                               const void *src_buf,
                               uint32_t write_size, uint32_t *out_size);
PlErrCode PlStorageEraseOsImpl(PlStorageDataId id);
PlErrCode PlStorageFactoryResetOsImpl(PlStorageDataId id);
PlErrCode PlStorageDReadOsImpl(PlStorageDataId id, int oflags, void *out_buf,
                               uint32_t read_size, uint32_t *out_size);
PlErrCode PlStorageDWriteOsImpl(PlStorageDataId id, int oflags,
                                const void *src_buf,
                                uint32_t write_size, uint32_t *out_size);
PlErrCode PlStorageGetDataInfoOsImpl(PlStorageDataId id,
                                     PlStorageDataInfo *info);
PlErrCode PlStorageSwitchDataOsImpl(PlStorageTmpDataId src_id,
                                    PlStorageDataId dst_id);
PlErrCode PlStorageGetTmpDataIdOsImpl(PlStorageDataId src_id,
                                      PlStorageTmpDataId *tmp_id);
PlErrCode PlStorageGetCapabilitiesOsImpl(PlStorageCapabilities *capabilities);
PlErrCode PlStorageGetIdCapabilitiesOsImpl(PlStorageDataId id,
                                     PlStorageIdCapabilities *id_capabilities);
PlErrCode PlStorageInitializeOsImpl(void);
PlErrCode PlStorageFinalizeOsImpl(void);
PlErrCode PlStorageCleanOsImpl(void);
PlErrCode PlStorageDowngradeOsImpl(void);

#endif  // PORTING_LAYER_INCLUDE_PL_STORAGE_OS_IMPL_H_
