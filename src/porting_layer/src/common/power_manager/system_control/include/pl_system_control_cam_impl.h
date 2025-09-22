/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

// Includes --------------------------------------------------------------------
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <pthread.h>

#include "pl_system_control.h"
#include "pl.h"

// Macros ----------------------------------------------------------------------

// Typedefs --------------------------------------------------------------------

// External functions ----------------------------------------------------------

// Local functions -------------------------------------------------------------

// Global Variables ------------------------------------------------------------

// Functions -------------------------------------------------------------------
// Use only Nuttx
PlErrCode PlSystemCtlGetResetCauseCamImpl(PlSystemCtlResetCause *cause);
char* PlSystemCtlGetRtcAddrCamImpl(void);

// Use only Linux
bool PlSystemCtlIsEnableCamImpl(void);

// Use other than raspi
PlErrCode PlSystemCtlRebootCpuCamImpl(void);
PlErrCode PlSystemCtlRebootEdgeDeviceCamImpl(void);
