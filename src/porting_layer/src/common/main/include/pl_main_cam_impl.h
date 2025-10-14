/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PL_MAIN_CAM_IMPL_H__
#define PL_MAIN_CAM_IMPL_H__

#include <stdbool.h>

#include "pl.h"

bool PlMainIsMigrationSupportedCamImpl(void);
void PlMainEraseMigrationSrcDataCamImpl(void);
PlErrCode PlMainExecMigrationCamImpl(void);

#endif  // PL_MAIN_CAM_IMPL_H__
