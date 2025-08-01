/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 #include <stddef.h>

 #include "pl_log_manager.h"
 #include "pl.h"
 
 #define PL_LOG_MANAGER_TARGET_BLOCK_TYPE_NUM (1)
 static const PlLogManagerBlockType
     s_block_type[PL_LOG_MANAGER_TARGET_BLOCK_TYPE_NUM] = {
         kPlLogManagerBlockTypeMain};
 
 const PlLogManagerBlockType *PlLogManagerGetLoadableBlock(
     size_t *loadable_block_num) {
   if (loadable_block_num == NULL) {
     return NULL;
   }
 
   *loadable_block_num = PL_LOG_MANAGER_TARGET_BLOCK_TYPE_NUM;
 
   return s_block_type;
 }
 
 const PlLogManagerBlockType *PlLogManagerGetSaveableBlock(
     PlLogManagerBlockType target_block, size_t *saveable_block_num) {
    return NULL;
 }

bool PlLogManagerLocalUploadAvailabilityCheck(void) { return false; }
