/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

// Includes --------------------------------------------------------------------
#include <stdio.h>

#include "pl_wdt.h"
#include "pl_wdt_lib.h"
#include "pl_wdt_os_impl.h"
#include "pl.h"

// Macros ----------------------------------------------------------------------

// Typedefs --------------------------------------------------------------------

// External functions ----------------------------------------------------------

// Local functions -------------------------------------------------------------

// Global Variables ------------------------------------------------------------

// Functions -------------------------------------------------------------------
void WdtCallbackOsImpl(int irq, WdtHandleInfo *info, int info_count) {
  (void)irq;  // Avoid unused parameter warning
  (void)info;  // Avoid unused parameter warning
  (void)info_count;  // Avoid unused parameter warning
  return kPlErrNoSupported;
}

// -----------------------------------------------------------------------------
PlErrCode PlWdtOpenOsImpl(uint32_t wdt_num,
                            WdtHandleInfo *info, int info_count) {
  (void)wdt_num;  // Avoid unused parameter warning
  (void)info;  // Avoid unused parameter warning
  (void)info_count;  // Avoid unused parameter warning
  return kPlErrNoSupported;
}

// -----------------------------------------------------------------------------
PlErrCode PlWdtKeepAliveIrqContextOsImpl(uint32_t wdt_num) {
  (void)wdt_num;  // Avoid unused parameter warning
  return kPlErrNoSupported;
}

// -----------------------------------------------------------------------------
