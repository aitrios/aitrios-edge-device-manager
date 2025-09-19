/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef PORTING_LAYER_INCLUDE_PL_POWER_MANAGER_OS_IMPL_H_
#define PORTING_LAYER_INCLUDE_PL_POWER_MANAGER_OS_IMPL_H_

#include <stddef.h>
#include "pl.h"

PlErrCode PlPowerMgrInitializeOsImpl(void);
PlErrCode PlPowerMgrFinalizeOsImpl(void);
PlErrCode PlPowerMgrGetSupplyTypeOsImpl(PlPowerMgrSupplyType *type);
PlErrCode PlPowerMgrSetupUsbOsImpl(void);
PlErrCode PlPowerMgrGetMigrationDataOsImpl(PlPowerMgrMigrationDataId id,
                                     void *dst,
                                     size_t dst_size);
PlErrCode PlPowerMgrEraseMigrationDataOsImpl(PlPowerMgrMigrationDataId id);

#endif /* PORTING_LAYER_INCLUDE_PL_POWER_MANAGER_OS_IMPL_H_ */
