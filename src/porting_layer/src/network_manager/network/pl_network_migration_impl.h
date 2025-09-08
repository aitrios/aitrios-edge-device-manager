/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PORTING_LAYER_SRC_NETWORK_MANAGER_NETWORK_MIGRATION_IMPL_H_
#define PORTING_LAYER_SRC_NETWORK_MANAGER_NETWORK_MIGRATION_IMPL_H_

#include <stdbool.h>

#include "pl.h"
#include "pl_network.h"

PlErrCode PlNetworkInitMigrationImpl(PlNetworkMigrationHandle *handle);
PlErrCode PlNetworkFinMigrationImpl(PlNetworkMigrationHandle handle);
PlErrCode PlNetworkGetMigrationDataImpl(PlNetworkMigrationHandle handle,
                                        PlNetworkMigrationDataId id, void *dst,
                                        size_t dst_size);
PlErrCode PlNetworkIsNeedMigrationImpl(PlNetworkMigrationNeedParam *param,
                                       bool *need_migration);
void PlNetworkEraseMigrationSrcDataImpl(void);

#endif  // PORTING_LAYER_SRC_NETWORK_MANAGER_NETWORK_MIGRATION_IMPL_H_
