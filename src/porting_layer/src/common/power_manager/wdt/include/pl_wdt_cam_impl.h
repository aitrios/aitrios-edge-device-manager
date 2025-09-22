/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef __PL_WDT_CAM_IMPL_H
#define __PL_WDT_CAM_IMPL_H

#include "pl.h"
#include "pl_wdt_lib.h"
// Macros ---------------------------------------------------------------------

// Global Variables -----------------------------------------------------------

// Local functions ------------------------------------------------------------

// Functions ------------------------------------------------------------------
void WdtCallbackCamImpl(int irq, WdtHandleInfo *info, int info_count);
PlErrCode PlWdtOpenCamImpl(uint32_t wdt_num,
                            WdtHandleInfo *info, int info_count);
PlErrCode PlWdtKeepAliveIrqContextCamImpl(uint32_t wdt_num);

#endif /* __PL_WDT_CAM_IMPL_H */
