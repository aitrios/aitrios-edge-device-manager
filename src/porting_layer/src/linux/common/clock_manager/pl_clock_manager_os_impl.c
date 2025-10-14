/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#include "pl.h"
#include "pl_clock_manager_cam_impl.h"

// ----------------------------------------------------------------------------
PlClockManagerReturnValue PlClockManagerStartNtpClientDaemonOsImpl(const PlClockManagerParams *param) {
  return PlClockManagerStartNtpClientDaemonCamImpl(param);
}

// -----------------------------------------------------------------------------
PlClockManagerReturnValue PlClockManagerWaitForTerminatingDaemonOsImpl(void) {
  return PlClockManagerWaitForTerminatingDaemonCamImpl();
}

// -----------------------------------------------------------------------------
PlClockManagerNtpTimeSyncStatus PlClockManagerJudgeNtpTimeSyncCompleteOsImpl(
    PlClockManagerNtpStatus status) {
  return PlClockManagerJudgeNtpTimeSyncCompleteCamImpl(status);
}

// -----------------------------------------------------------------------------
PlClockManagerReturnValue PlClockManagerDeleteConfFilesOsImpl(void){
  return PlClockManagerDeleteConfFilesCamImpl();
}

// -----------------------------------------------------------------------------
PlClockManagerNtpStatus PlClockManagerGetNtpStatusOsImpl(void){
  return PlClockManagerGetNtpStatusCamImpl();
}

// -----------------------------------------------------------------------------
PlClockManagerReturnValue PlClockManagerRestartNtpClientDaemonOsImpl(void){
  return PlClockManagerRestartNtpClientDaemonCamImpl();
}

// -----------------------------------------------------------------------------
bool PlClockManagerIsNtpClientDaemonActiveOsImpl(void){
  return PlClockManagerIsNtpClientDaemonActiveCamImpl();
}
