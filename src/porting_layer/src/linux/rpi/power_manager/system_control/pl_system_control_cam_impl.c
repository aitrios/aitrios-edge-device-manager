/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

// Includes --------------------------------------------------------------------
#include <sys/reboot.h>
#include <stddef.h>
#include <stdbool.h>

#include "pl.h"

// -----------------------------------------------------------------------------
PlErrCode PlSystemCtlRebootCpuCamImpl(void) {
  execl("/sbin/reboot", "reboot", (char *)NULL);

  // should not reach.
  return kPlErrCodeOk;
}

// -----------------------------------------------------------------------------
PlErrCode PlSystemCtlRebootEdgeDeviceCamImpl(void) {
  return kPlErrNoSupported;
}

// -----------------------------------------------------------------------------
