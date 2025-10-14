/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pl.h"

#include <errno.h>

#include "pl_main_cam_impl.h"
#include "pl_main_internal.h"
#include "pl_main_table.h"
#include "utility_log.h"
#include "utility_log_module_id.h"

PlErrCode PlMainFileSystemFormatLittleFsOsImpl(
    const PlMainDeviceInformation* info) {
  LOG_W(0x3, "No supported.");
  return kPlErrNoSupported;
}

PlErrCode PlMainFileSystemFormatOtherOsImpl(
    const PlMainDeviceInformation* info) {
  LOG_W(0x3, "No supported.");
  return kPlErrNoSupported;
}

PlErrCode PlMainFileSystemFormatFat32OsImpl(
    const PlMainDeviceInformation* info) {
  LOG_E(0x00, "Unexpected operation. device:%s type:%d", info->source,
        (int)info->device_type);
  return kPlErrInternal;
}

bool PlMainIsMigrationSupportedOsImpl(void) {
  return PlMainIsMigrationSupportedCamImpl();
}

void PlMainEraseMigrationSrcDataOsImpl(void) {
  return PlMainEraseMigrationSrcDataCamImpl();
}

PlErrCode PlMainExecMigrationOsImpl(void) {
  return PlMainExecMigrationCamImpl();
}