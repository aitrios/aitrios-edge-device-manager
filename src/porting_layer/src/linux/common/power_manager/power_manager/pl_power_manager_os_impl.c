/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#include "pl.h"
#include "pl_power_manager_cam_impl.h"

// ----------------------------------------------------------------------------
PlErrCode PlPowerMgrInitializeOsImpl(void) {
  return PlPowerMgrInitializeCamImpl();
}
// ----------------------------------------------------------------------------
PlErrCode PlPowerMgrFinalizeOsImpl(void) {
  return PlPowerMgrFinalizeCamImpl();
}
// ----------------------------------------------------------------------------
PlErrCode PlPowerMgrGetSupplyTypeOsImpl(PlPowerMgrSupplyType *type) {
  return PlPowerMgrGetSupplyTypeCamImpl(type);
}
// ----------------------------------------------------------------------------
PlErrCode PlPowerMgrSetupUsbOsImpl(void) {
  return kPlErrNoSupported;
}
// ----------------------------------------------------------------------------
PlErrCode PlPowerMgrGetMigrationDataOsImpl(PlPowerMgrMigrationDataId id,
                                     void *dst,
                                     size_t dst_size) {
  return PlPowerMgrGetMigrationDataCamImpl(id, dst, dst_size);
}
// ----------------------------------------------------------------------------
PlErrCode PlPowerMgrEraseMigrationDataOsImpl(PlPowerMgrMigrationDataId id) {
  return PlPowerMgrEraseMigrationDataCamImpl(id);
}
// ----------------------------------------------------------------------------
