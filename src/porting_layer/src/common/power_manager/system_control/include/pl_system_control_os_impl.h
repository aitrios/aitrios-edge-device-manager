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
PlErrCode PlSystemCtlGetResetCauseOsImpl(PlSystemCtlResetCause *cause);
PlErrCode PlSystemCtlRebootCpuOsImpl(void);
PlErrCode PlSystemCtlRebootEdgeDeviceOsImpl(void);

// Use only Nuttx
PlErrCode PlSystemCtlGetExceptionInfoOsImpl(
            struct PlSystemCtlExceptionInfo *info);
PlErrCode PlSystemCtlSetExceptionInfoOsImpl(void);
PlErrCode PlSystemCtlConvExceptionInfoOsImpl(
            struct PlSystemCtlExceptionInfo *info,
            char *dst, uint32_t dst_size);
PlErrCode PlSystemCtlClearExceptionInfoOsImpl(void);
PlErrCode PlSystemCtlDumpAllStackOsImpl(void);
