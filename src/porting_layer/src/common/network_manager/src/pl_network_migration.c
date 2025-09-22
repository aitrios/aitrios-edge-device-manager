/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Includes --------------------------------------------------------------------
#include "pl_network_migration.h"

#include <stdbool.h>
#include <stddef.h>

#include "pl.h"
#include "pl_network.h"
#include "pl_network_log.h"
#include "pl_network_migration_os_impl.h"

// Public functions ------------------------------------------------------------
PlErrCode PlNetworkInitMigration(PlNetworkMigrationHandle *handle) {
  PlErrCode ret = PlNetworkInitMigrationOsImpl(handle);
  return ret;
}

// -----------------------------------------------------------------------------
PlErrCode PlNetworkFinMigration(PlNetworkMigrationHandle handle) {
  PlErrCode ret = PlNetworkFinMigrationOsImpl(handle);
  return ret;
}

// -----------------------------------------------------------------------------
PlErrCode PlNetworkGetMigrationData(PlNetworkMigrationHandle handle,
                                    PlNetworkMigrationDataId id, void *dst,
                                    size_t dst_size) {
  PlErrCode ret = PlNetworkGetMigrationDataOsImpl(handle, id, dst, dst_size);
  return ret;
}

// -----------------------------------------------------------------------------
PlErrCode PlNetworkIsNeedMigration(PlNetworkMigrationNeedParam *param,
                                   bool *need_migration) {
  if (param == NULL || need_migration == NULL) {
    DLOGE("%s:%p:%p", __func__, param, need_migration);
    ELOGE(kElog_PlNetworkIsNeedMigration);
    return kPlErrInvalidParam;
  }
  PlErrCode ret = PlNetworkIsNeedMigrationOsImpl(param, need_migration);
  return ret;
}

// -----------------------------------------------------------------------------
void PlNetworkEraseMigrationSrcData(void) {
  return PlNetworkEraseMigrationSrcDataOsImpl();
}
