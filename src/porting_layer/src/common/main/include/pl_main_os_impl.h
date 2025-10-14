/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PL_MAIN_IMPL_H__
#define PL_MAIN_IMPL_H__

#include "pl.h"
#include "pl_main_table.h"

#define PL_MAIN_LITTLEFS_FILESYSTEM_TYPE "littlefs"

PlErrCode PlMainFileSystemFormatLittleFsOsImpl(
    const PlMainDeviceInformation* info);
PlErrCode PlMainFileSystemFormatOtherOsImpl(const PlMainDeviceInformation* info);
PlErrCode PlMainFileSystemFormatFat32OsImpl(const PlMainDeviceInformation* info);
bool PlMainIsMigrationSupportedOsImpl(void);
void PlMainEraseMigrationSrcDataOsImpl(void);
PlErrCode PlMainExecMigrationOsImpl(void);
#endif  // PL_MAIN_IMPL_H__
