/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PL_NETWORK_MIGRATION_OS_IMPL_H_
#define PL_NETWORK_MIGRATION_OS_IMPL_H_

// Includes --------------------------------------------------------------------
#include <stdbool.h>

#include "pl.h"
#include "pl_network.h"

// Public functions ------------------------------------------------------------
PlErrCode PlNetworkInitMigrationOsImpl(PlNetworkMigrationHandle *handle);
PlErrCode PlNetworkFinMigrationOsImpl(PlNetworkMigrationHandle handle);
PlErrCode PlNetworkGetMigrationDataOsImpl(PlNetworkMigrationHandle handle,
                                          PlNetworkMigrationDataId id,
                                          void *dst, size_t dst_size);
PlErrCode PlNetworkIsNeedMigrationOsImpl(PlNetworkMigrationNeedParam *param,
                                         bool *need_migration);
void PlNetworkEraseMigrationSrcDataOsImpl(void);

#endif  // PL_NETWORK_MIGRATION_OS_IMPL_H_
