/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef PORTING_LAYER_INCLUDE_PL_POWER_MANAGER_CAM_IMPL_H_
#define PORTING_LAYER_INCLUDE_PL_POWER_MANAGER_CAM_IMPL_H_

#include <stddef.h>
#include "pl.h"
#include "pl_power_manager.h"

PlErrCode PlPowerMgrInitializeCamImpl(void);
PlErrCode PlPowerMgrFinalizeCamImpl(void);
PlErrCode PlPowerMgrGetSupplyTypeCamImpl(PlPowerMgrSupplyType *type);
PlErrCode PlPowerMgrSetupUsbCamImpl(void);
PlErrCode PlPowerMgrGetMigrationDataCamImpl(PlPowerMgrMigrationDataId id,
                                     void *dst,
                                     size_t dst_size);
PlErrCode PlPowerMgrEraseMigrationDataCamImpl(PlPowerMgrMigrationDataId id);

#endif  // PORTING_LAYER_INCLUDE_PL_POWER_MANAGER_CAM_IMPL_H_
