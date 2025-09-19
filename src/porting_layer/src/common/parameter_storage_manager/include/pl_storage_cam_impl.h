/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef PORTING_LAYER_INCLUDE_PL_STORAGE_CAM_IMPL_H_
#define PORTING_LAYER_INCLUDE_PL_STORAGE_CAM_IMPL_H_

#include "pl.h"
#include "pl_storage.h"

// Functions ------------------------------------------------------------------
PlErrCode PlStorageOpenCamImpl(PlStorageDataId id, int oflags,
                              PlStorageHandle *handle);
PlErrCode PlStorageCloseCamImpl(const PlStorageHandle handle);
PlErrCode PlStorageSeekCamImpl(const PlStorageHandle handle, int32_t offset,
                              PlStorageSeekType type, int32_t *cur_pos);
PlErrCode PlStorageReadCamImpl(const PlStorageHandle handle, void *out_buf,
                              uint32_t read_size, uint32_t *out_size);
PlErrCode PlStorageWriteCamImpl(const PlStorageHandle handle,
                               const void *src_buf,
                               uint32_t write_size, uint32_t *out_size);
PlErrCode PlStorageEraseCamImpl(PlStorageDataId id);
PlErrCode PlStorageFactoryResetCamImpl(PlStorageDataId id);
PlErrCode PlStorageDReadCamImpl(PlStorageDataId id, int oflags, void *out_buf,
                               uint32_t read_size, uint32_t *out_size);
PlErrCode PlStorageDWriteCamImpl(PlStorageDataId id, int oflags,
                                const void *src_buf,
                                uint32_t write_size, uint32_t *out_size);
PlErrCode PlStorageGetDataInfoCamImpl(PlStorageDataId id,
                                     PlStorageDataInfo *info);
PlErrCode PlStorageSwitchDataCamImpl(PlStorageTmpDataId src_id,
                                    PlStorageDataId dst_id);
PlErrCode PlStorageGetTmpDataIdCamImpl(PlStorageDataId src_id,
                                      PlStorageTmpDataId *tmp_id);
PlErrCode PlStorageGetCapabilitiesCamImpl(PlStorageCapabilities *capabilities);
PlErrCode PlStorageGetIdCapabilitiesCamImpl(PlStorageDataId id,
                                     PlStorageIdCapabilities *id_capabilities);
PlErrCode PlStorageInitializeCamImpl(void);
PlErrCode PlStorageFinalizeCamImpl(void);
PlErrCode PlStorageCleanCamImpl(void);
PlErrCode PlStorageDowngradeCamImpl(void);

#endif  // PORTING_LAYER_INCLUDE_PL_STORAGE_CAM_IMPL_H_
