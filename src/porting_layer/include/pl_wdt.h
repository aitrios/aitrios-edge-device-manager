/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef __PL_WDT_H
#define __PL_WDT_H

#include "pl.h"

// Macros ---------------------------------------------------------------------

// Global Variables -----------------------------------------------------------
typedef void (*PlWdtIrqHandler)(void *private_data);

// Local functions ------------------------------------------------------------

// Functions ------------------------------------------------------------------
PlErrCode PlWdtInitialize(void);
PlErrCode PlWdtFinalize(void);
PlErrCode PlWdtStart(void);
PlErrCode PlWdtStop(void);
PlErrCode PlWdtRegisterIrqHandler(PlWdtIrqHandler handler,
                                  void *private_data);
PlErrCode PlWdtUnregisterIrqHandler(void);
PlErrCode PlWdtTerminate(void);
PlErrCode PlWdtKeepAlive(void);

#endif /* __PL_WDT_H */
