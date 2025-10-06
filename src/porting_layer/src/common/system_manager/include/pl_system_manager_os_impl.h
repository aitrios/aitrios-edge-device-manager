/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PORTING_LAYER_INCLUDE_PL_SYSTEM_MANAGER_OS_IMPL_H_
#define PORTING_LAYER_INCLUDE_PL_SYSTEM_MANAGER_OS_IMPL_H_

#include <stdbool.h>
#include <stddef.h>

#include "pl.h"
#include "pl_system_manager.h"

size_t PlSystemManagerGetHwInfoSizeOsImpl(void);
PlErrCode PlSystemManagerParseHwInfoOsImpl(char *hw_info,
                                           PlSystemManagerHwInfo *data);
bool PlSystemManagerIsHwInfoSupportedOsImpl(void);
bool PlSystemManagerRequiresSerialNumberFromDeviceManifestOsImpl(void);
PlErrCode PlSystemManagerIsNeedRebootOsImpl(
    PlSystemManagerResetCause reset_cause, bool *reset_flag);
PlErrCode PlSystemManagerGetMigrationDataOsImpl(
    PlSystemManagerMigrationDataId id, void *dst, size_t dst_size);
PlErrCode PlSystemManagerEraseMigrationDataOsImpl(
    PlSystemManagerMigrationDataId id);

#endif  // PORTING_LAYER_INCLUDE_PL_SYSTEM_MANAGER_OS_IMPL_H_
