/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

// Includes --------------------------------------------------------------------
#include <stdio.h>

#include "pl.h"
#include "pl_log_manager.h"
#include "pl_log_manager_os_impl.h"

// Macros ----------------------------------------------------------------------

// Typedefs --------------------------------------------------------------------

// External functions ----------------------------------------------------------

// Local functions -------------------------------------------------------------

// Global Variables ------------------------------------------------------------

// Functions -------------------------------------------------------------------
// -----------------------------------------------------------------------------
//  PlLogManagerGetLoadableBlock
// -----------------------------------------------------------------------------
const PlLogManagerBlockType *PlLogManagerGetLoadableBlock(
    size_t *loadable_block_num) {
  return PlLogManagerGetLoadableBlockOsImpl(loadable_block_num);
}

// -----------------------------------------------------------------------------
//  PlLogManagerGetSaveableBlock
// -----------------------------------------------------------------------------
const PlLogManagerBlockType *PlLogManagerGetSaveableBlock(
    PlLogManagerBlockType target_block, size_t *saveable_block_num) {
  return PlLogManagerGetSaveableBlockOsImpl(target_block, saveable_block_num);
}

// -----------------------------------------------------------------------------
//  PlLogManagerLocalUploadAvailabilityCheck
// -----------------------------------------------------------------------------
bool PlLogManagerLocalUploadAvailabilityCheck(void){
  return PlLogManagerLocalUploadAvailabilityCheckOsImpl();
}
