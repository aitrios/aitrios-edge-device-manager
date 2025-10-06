/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#include "pl.h"
#include "pl_log_manager_cam_impl.h"

// ----------------------------------------------------------------------------
const PlLogManagerBlockType *PlLogManagerGetLoadableBlockOsImpl(
    size_t *loadable_block_num) {
  return PlLogManagerGetLoadableBlockCamImpl(loadable_block_num);
}
// ----------------------------------------------------------------------------
const PlLogManagerBlockType *PlLogManagerGetSaveableBlockOsImpl(
    PlLogManagerBlockType target_block, size_t *saveable_block_num) {
  return PlLogManagerGetSaveableBlockCamImpl(target_block, saveable_block_num);
}
// ----------------------------------------------------------------------------
bool PlLogManagerLocalUploadAvailabilityCheckOsImpl(void){
  return PlLogManagerLocalUploadAvailabilityCheckCamImpl();
}
// ----------------------------------------------------------------------------
