/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#include <stddef.h>
#include "pl.h"
#include "pl_power_manager.h"

// ----------------------------------------------------------------------------
PlErrCode PlPowerMgrInitializeCamImpl(void) {
  return kPlErrNoSupported;
}
// ----------------------------------------------------------------------------
PlErrCode PlPowerMgrFinalizeCamImpl(void) {
  return kPlErrNoSupported;
}
// ----------------------------------------------------------------------------
PlErrCode PlPowerMgrGetSupplyTypeCamImpl(PlPowerMgrSupplyType *type) {
  return kPlErrNoSupported;
}
// ----------------------------------------------------------------------------
PlErrCode PlPowerMgrSetupUsbCamImpl(void) {
  return kPlErrNoSupported;
}
// ----------------------------------------------------------------------------
PlErrCode PlPowerMgrGetMigrationDataCamImpl(PlPowerMgrMigrationDataId id,
                                     void *dst,
                                     size_t dst_size) {
  return kPlErrNoSupported;
}
// ----------------------------------------------------------------------------
PlErrCode PlPowerMgrEraseMigrationDataCamImpl(PlPowerMgrMigrationDataId id) {
  return kPlErrNoSupported;
}
// ----------------------------------------------------------------------------
