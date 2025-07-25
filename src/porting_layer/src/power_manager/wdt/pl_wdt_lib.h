/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef __PL_WDT_LIB_H
#define __PL_WDT_LIB_H

#include "pl.h"

// Macros ---------------------------------------------------------------------

// Global Variables -----------------------------------------------------------
typedef void* PlWdtLibHandle;
typedef void (*PlWdtLibIrqHandler)(void *private_data);

typedef enum {
  kWdtLibStateClose = 0,
  kWdtLibStateStop,
  kWdtLibStateStart
} WdtLibState;

typedef struct {
  int wdt_no;
  int wdt_handle;
  WdtLibState state;
  PlWdtLibIrqHandler irq_handler;
  void *private_data;
} WdtHandleInfo;
// Local functions ------------------------------------------------------------

// Functions ------------------------------------------------------------------
PlErrCode PlWdtLibInitialize(void);
PlErrCode PlWdtLibFinalize(void);
PlErrCode PlWdtLibOpen(PlWdtLibHandle *handle, uint32_t wdt_num);
PlErrCode PlWdtLibClose(const PlWdtLibHandle handle);
PlErrCode PlWdtLibStart(const PlWdtLibHandle handle);
PlErrCode PlWdtLibStop(const PlWdtLibHandle handle);
PlErrCode PlWdtLibSetTimeout(const PlWdtLibHandle handle,
                                uint32_t timeout_sec);
PlErrCode PlWdtLibRegisterIrqHandler(const PlWdtLibHandle handle,
                              PlWdtLibIrqHandler handler, void *private_data);
PlErrCode PlWdtLibUnregisterIrqHandler(const PlWdtLibHandle handle);
PlErrCode PlWdtLibKeepAlive(const PlWdtLibHandle handle);
PlErrCode PlWdtLibKeepAliveIrqContext(uint32_t wdt_num);

#endif /* __PL_WDT_LIB_H */
