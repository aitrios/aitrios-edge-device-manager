/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pl.h"

#include <stdbool.h>

bool PlMainIsMigrationSupportedCamImpl(void) { return false; }

void PlMainEraseMigrationSrcDataCamImpl(void) { return; }

PlErrCode PlMainExecMigrationCamImpl(void) { return kPlErrCodeOk; }
