/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

// Includes --------------------------------------------------------------------
#include <stdio.h>

#include "pl.h"
#include "pl_clock_manager.h"
#include "pl_clock_manager_os_impl.h"

// Macros ----------------------------------------------------------------------

// Typedefs --------------------------------------------------------------------

// External functions ----------------------------------------------------------

// Local functions -------------------------------------------------------------

// Global Variables ------------------------------------------------------------

// Functions -------------------------------------------------------------------
// -----------------------------------------------------------------------------
//  PlClockManagerStartNtpClientDaemon
// -----------------------------------------------------------------------------
PlClockManagerReturnValue PlClockManagerStartNtpClientDaemon(
    const PlClockManagerParams *param) {
  return PlClockManagerStartNtpClientDaemonOsImpl(param);
}
// -----------------------------------------------------------------------------
//  PlClockManagerWaitForTerminatingDaemon
// -----------------------------------------------------------------------------
PlClockManagerReturnValue PlClockManagerWaitForTerminatingDaemon(void) {
  return PlClockManagerWaitForTerminatingDaemonOsImpl();
}

// -----------------------------------------------------------------------------
//  PlClockManagerJudgeNtpTimeSyncComplete
// -----------------------------------------------------------------------------
PlClockManagerNtpTimeSyncStatus PlClockManagerJudgeNtpTimeSyncComplete(
    PlClockManagerNtpStatus status) {
  return PlClockManagerJudgeNtpTimeSyncCompleteOsImpl(status);
}

// -----------------------------------------------------------------------------
//  PlClockManagerDeleteConfFiles
// -----------------------------------------------------------------------------
PlClockManagerReturnValue PlClockManagerDeleteConfFiles(void){
  return PlClockManagerDeleteConfFilesOsImpl();
}

// -----------------------------------------------------------------------------
//  PlClockManagerGetNtpStatus
// -----------------------------------------------------------------------------
PlClockManagerNtpStatus PlClockManagerGetNtpStatus(void){
  return PlClockManagerGetNtpStatusOsImpl();
}

// -----------------------------------------------------------------------------
//  PlClockManagerRestartNtpClientDaemon
// -----------------------------------------------------------------------------
PlClockManagerReturnValue PlClockManagerRestartNtpClientDaemon(void){
  return PlClockManagerRestartNtpClientDaemonOsImpl();
}

// -----------------------------------------------------------------------------
//  PlClockManagerIsNtpClientDaemonActive
// -----------------------------------------------------------------------------
bool PlClockManagerIsNtpClientDaemonActive(void){
  return PlClockManagerIsNtpClientDaemonActiveOsImpl();
}
