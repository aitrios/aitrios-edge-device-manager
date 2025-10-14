/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef __PL_CLOCK_MANAGER_CAM_IMPL_H
#define __PL_CLOCK_MANAGER_CAM_IMPL_H

/*******************************************************************************
 * Included Files
 ******************************************************************************/
#include <stdio.h>
#include "pl.h"
#include "pl_clock_manager.h"

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

PlClockManagerReturnValue PlClockManagerStartNtpClientDaemonCamImpl(const PlClockManagerParams *param);
PlClockManagerReturnValue PlClockManagerWaitForTerminatingDaemonCamImpl(void);
PlClockManagerNtpTimeSyncStatus PlClockManagerJudgeNtpTimeSyncCompleteCamImpl(PlClockManagerNtpStatus status);
PlClockManagerReturnValue PlClockManagerDeleteConfFilesCamImpl(void);
PlClockManagerNtpStatus PlClockManagerGetNtpStatusCamImpl(void);
PlClockManagerReturnValue PlClockManagerRestartNtpClientDaemonCamImpl(void);
bool PlClockManagerIsNtpClientDaemonActiveCamImpl(void);

#endif /* __PL_CLOCK_MANAGER_CAM_IMPL_H */
