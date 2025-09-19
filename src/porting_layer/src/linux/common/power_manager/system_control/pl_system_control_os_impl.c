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
#include "pl_system_control_os_impl.h"
#include "pl_system_control_cam_impl.h"
#include "pl.h"

// Macros ----------------------------------------------------------------------

// Typedefs --------------------------------------------------------------------

// External functions ----------------------------------------------------------

// Local functions -------------------------------------------------------------

// Global Variables ------------------------------------------------------------

// Functions -------------------------------------------------------------------
PlErrCode PlSystemCtlGetResetCauseOsImpl(PlSystemCtlResetCause *cause) {
  return kPlErrNoSupported;
}

// -----------------------------------------------------------------------------
PlErrCode PlSystemCtlRebootCpuOsImpl(void) {
  return PlSystemCtlRebootCpuCamImpl();
}

// -----------------------------------------------------------------------------
PlErrCode PlSystemCtlRebootEdgeDeviceOsImpl(void) {
  return PlSystemCtlRebootEdgeDeviceCamImpl();
}

// -----------------------------------------------------------------------------
PlErrCode PlSystemCtlGetExceptionInfoOsImpl(
            struct PlSystemCtlExceptionInfo *info) {
  (void)info;  // Avoid compiler warning
  return kPlErrNoSupported;
}

// -----------------------------------------------------------------------------
PlErrCode PlSystemCtlSetExceptionInfoOsImpl(void) {
  return kPlErrNoSupported;
}

// -----------------------------------------------------------------------------
PlErrCode PlSystemCtlConvExceptionInfoOsImpl(
            struct PlSystemCtlExceptionInfo *info,
            char *dst, uint32_t dst_size) {
  (void)info;      // Avoid compiler warning
  (void)dst;       // Avoid compiler warning
  (void)dst_size;  // Avoid compiler warning
  return kPlErrNoSupported;
}

// -----------------------------------------------------------------------------
PlErrCode PlSystemCtlClearExceptionInfoOsImpl(void) {
  return kPlErrNoSupported;
}

// -----------------------------------------------------------------------------
PlErrCode PlSystemCtlDumpAllStackOsImpl(void) {
  return kPlErrNoSupported;
}

// -----------------------------------------------------------------------------
