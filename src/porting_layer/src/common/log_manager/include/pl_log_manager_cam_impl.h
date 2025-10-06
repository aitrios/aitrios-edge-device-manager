/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef __PL_LOG_MANAGER_CAM_IMPL_H
#define __PL_LOG_MANAGER_CAM_IMPL_H

/*******************************************************************************
 * Included Files
 ******************************************************************************/
#include <stdio.h>
#include "pl.h"
#include "pl_log_manager.h"

/*******************************************************************************
 * Pre-preprocessor Definitions
 ******************************************************************************/

/*******************************************************************************
 * Public Types
 ******************************************************************************/

/*******************************************************************************
 * Public Data
 ******************************************************************************/

/*******************************************************************************
 * Inline Functions
 ******************************************************************************/

/*******************************************************************************
 * Public Function Prototypes
 ******************************************************************************/

const PlLogManagerBlockType *PlLogManagerGetLoadableBlockCamImpl(size_t *loadable_block_num);
const PlLogManagerBlockType *PlLogManagerGetSaveableBlockCamImpl(PlLogManagerBlockType target_block, size_t *saveable_block_num);
bool PlLogManagerLocalUploadAvailabilityCheckCamImpl(void);

#endif /* __PL_LOG_MANAGER_CAM_IMPL_H */
