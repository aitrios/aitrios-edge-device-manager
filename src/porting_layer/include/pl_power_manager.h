/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef __PL_POWER_MGR_H
#define __PL_POWER_MGR_H

#include "pl.h"

// Typedef ---------------------------------------------------------------------
typedef enum {
  kPlPowerMgrSupplyTypePoE = 0,
  kPlPowerMgrSupplyTypeBC12,
  kPlPowerMgrSupplyTypeCC15A,
  kPlPowerMgrSupplyTypeNotSupport,
  kPlPowerMgrSupplyTypeMax
} PlPowerMgrSupplyType;

// Public API-------------------------------------------------------------------
PlErrCode PlPowerMgrInitialize(void);
PlErrCode PlPowerMgrFinalize(void);
PlErrCode PlPowerMgrGetSupplyType(PlPowerMgrSupplyType *type);
PlErrCode PlPowerMgrSetupUsb(void);

#endif /* __PL_POWER_MGR_H */
