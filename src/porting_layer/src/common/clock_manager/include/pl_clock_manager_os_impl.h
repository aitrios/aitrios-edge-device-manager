/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef __PL_CLOCK_MANAGER_OS_IMPL_H
#define __PL_CLOCK_MANAGER_OS_IMPL_H

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

PlClockManagerReturnValue PlClockManagerStartNtpClientDaemonOsImpl(const PlClockManagerParams *param);
PlClockManagerReturnValue PlClockManagerWaitForTerminatingDaemonOsImpl(void);
PlClockManagerNtpTimeSyncStatus PlClockManagerJudgeNtpTimeSyncCompleteOsImpl(PlClockManagerNtpStatus status);
PlClockManagerReturnValue PlClockManagerDeleteConfFilesOsImpl(void);
PlClockManagerNtpStatus PlClockManagerGetNtpStatusOsImpl(void);
PlClockManagerReturnValue PlClockManagerRestartNtpClientDaemonOsImpl(void);
bool PlClockManagerIsNtpClientDaemonActiveOsImpl(void);

#endif /* __PL_CLOCK_MANAGER_OS_IMPL_H */
