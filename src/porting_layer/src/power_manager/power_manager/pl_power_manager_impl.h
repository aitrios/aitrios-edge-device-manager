/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef __PL_POWER_MGR_IMPL_H
#define __PL_POWER_MGR_IMPL_H

#include "pl.h"
#include "pl_power_manager.h"

// Functions ------------------------------------------------------------------
PlErrCode PlPowerMgrInitializeImpl(void);
PlErrCode PlPowerMgrFinalizeImpl(void);
PlErrCode PlPowerMgrGetSupplyTypeImpl(PlPowerMgrSupplyType *type);
PlErrCode PlPowerMgrSetupUsbImpl(void);

#endif /* __PL_POWER_MGR_IMPL_H */
