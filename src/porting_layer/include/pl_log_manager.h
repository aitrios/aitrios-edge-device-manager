/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

// This header file defines structures and functions related to the HW info
// data management in the parameter storage manager.

#ifndef PL_LOG_MANAGER_H_
#define PL_LOG_MANAGER_H_

#include <stdbool.h>
#include <stddef.h>

#include "pl.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  kPlLogManagerBlockTypeMain,
  kPlLogManagerBlockTypeSensor,
  kPlLogManagerBlockTypeCompanionFw,
  kPlLogManagerBlockTypeCompanionApp,
  kPlLogManagerBlockTypeAll,  // All(Main/Snsor/CompanionFw/CompanionApp)
  kPlLogManagerBlockTypeNum   // PlLogManagerBlockType element count
} PlLogManagerBlockType;

const PlLogManagerBlockType *PlLogManagerGetLoadableBlock(
    size_t *loadable_block_num);

const PlLogManagerBlockType *PlLogManagerGetSaveableBlock(
    PlLogManagerBlockType target_block, size_t *saveable_block_num);

bool PlLogManagerLocalUploadAvailabilityCheck(void);

#ifdef __cplusplus
}
#endif

#endif  // PL_LOG_MANAGER_H_
