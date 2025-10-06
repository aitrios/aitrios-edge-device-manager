/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pl_system_manager.h"

#include <stdbool.h>
#include <stddef.h>

#include "pl.h"
#include "pl_system_manager_os_impl.h"

size_t PlSystemManagerGetHwInfoSize(void) {
  return PlSystemManagerGetHwInfoSizeOsImpl();
}

PlErrCode PlSystemManagerParseHwInfo(char *hw_info,
                                     PlSystemManagerHwInfo *data) {
  return PlSystemManagerParseHwInfoOsImpl(hw_info, data);
}

bool PlSystemManagerIsHwInfoSupported(void) {
  return PlSystemManagerIsHwInfoSupportedOsImpl();
}

bool PlSystemManagerRequiresSerialNumberFromDeviceManifest(void) {
  return PlSystemManagerRequiresSerialNumberFromDeviceManifestOsImpl();
}

PlErrCode PlSystemManagerIsNeedReboot(PlSystemManagerResetCause reset_cause,
                                      bool *reset_flag) {
  return PlSystemManagerIsNeedRebootOsImpl(reset_cause, reset_flag);
}

PlErrCode PlSystemManagerGetMigrationData(PlSystemManagerMigrationDataId id,
                                          void *dst, size_t dst_size) {
  return PlSystemManagerGetMigrationDataOsImpl(id, dst, dst_size);
}

PlErrCode PlSystemManagerEraseMigrationData(PlSystemManagerMigrationDataId id) {
  return PlSystemManagerEraseMigrationDataOsImpl(id);
}
