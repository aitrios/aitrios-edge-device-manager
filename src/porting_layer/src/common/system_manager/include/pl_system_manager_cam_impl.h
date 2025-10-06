/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PORTING_LAYER_INCLUDE_PL_SYSTEM_MANAGER_CAM_IMPL_H_
#define PORTING_LAYER_INCLUDE_PL_SYSTEM_MANAGER_CAM_IMPL_H_

#include <stdbool.h>
#include <stddef.h>

#include "pl.h"
#include "pl_system_manager.h"

size_t PlSystemManagerGetHwInfoSizeCamImpl(void);
PlErrCode PlSystemManagerParseHwInfoCamImpl(char *hw_info,
                                            PlSystemManagerHwInfo *data);
bool PlSystemManagerIsHwInfoSupportedCamImpl(void);
bool PlSystemManagerRequiresSerialNumberFromDeviceManifestCamImpl(void);
PlErrCode PlSystemManagerIsNeedRebootCamImpl(
    PlSystemManagerResetCause reset_cause, bool *reset_flag);
PlErrCode PlSystemManagerGetMigrationDataCamImpl(
    PlSystemManagerMigrationDataId id, void *dst, size_t dst_size);
PlErrCode PlSystemManagerEraseMigrationDataCamImpl(
    PlSystemManagerMigrationDataId id);

#endif  // PORTING_LAYER_INCLUDE_PL_SYSTEM_MANAGER_CAM_IMPL_H_
