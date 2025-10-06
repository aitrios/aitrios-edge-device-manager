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

PlErrCode PlMainFileSystemFormatLittleFsImpl(
    const PlMainDeviceInformation* info);
PlErrCode PlMainFileSystemFormatOtherImpl(const PlMainDeviceInformation* info);
PlErrCode PlMainFileSystemFormatFat32Impl(const PlMainDeviceInformation* info);

#endif  // PL_MAIN_IMPL_H__
