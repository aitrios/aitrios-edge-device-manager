/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pl.h"

#include <stdbool.h>
#include <stddef.h>

#include "pl_system_manager.h"
#include "pl_system_manager_cam_impl.h"

size_t PlSystemManagerGetHwInfoSizeOsImpl(void) {
  return PlSystemManagerGetHwInfoSizeCamImpl();
}

PlErrCode PlSystemManagerParseHwInfoOsImpl(char *hw_info,
                                           PlSystemManagerHwInfo *data) {
  return PlSystemManagerParseHwInfoCamImpl(hw_info, data);
}

bool PlSystemManagerIsHwInfoSupportedOsImpl(void) {
  return PlSystemManagerIsHwInfoSupportedCamImpl();
}

bool PlSystemManagerRequiresSerialNumberFromDeviceManifestOsImpl(void) {
  return PlSystemManagerRequiresSerialNumberFromDeviceManifestCamImpl();
}

PlErrCode PlSystemManagerIsNeedRebootOsImpl(
    PlSystemManagerResetCause reset_cause, bool *reset_flag) {
  return PlSystemManagerIsNeedRebootCamImpl(reset_cause, reset_flag);
}

PlErrCode PlSystemManagerGetMigrationDataOsImpl(
    PlSystemManagerMigrationDataId id, void *dst, size_t dst_size) {
  return PlSystemManagerGetMigrationDataCamImpl(id, dst, dst_size);
}

PlErrCode PlSystemManagerEraseMigrationDataOsImpl(
    PlSystemManagerMigrationDataId id) {
  return PlSystemManagerEraseMigrationDataCamImpl(id);
}
