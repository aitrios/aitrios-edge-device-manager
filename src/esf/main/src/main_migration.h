/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESF_MAIN_MAIN_MIGRATION_H_
#define ESF_MAIN_MAIN_MIGRATION_H_

#include <stdbool.h>

#include "main.h"

EsfMainError EsfMainIsNeedMigration(bool *ret);
EsfMainError EsfMainDisableMigrationFlag(void);
EsfMainError EsfMainExecMigration(void);

#endif  // ESF_MAIN_MAIN_MIGRATION_H_
