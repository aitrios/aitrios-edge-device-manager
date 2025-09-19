/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

// Includes --------------------------------------------------------------------
#include <stdio.h>

#include "pl.h"
#include "pl_button.h"
#include "pl_button_os_impl.h"

// Macros ----------------------------------------------------------------------

// Typedefs --------------------------------------------------------------------

// External functions ----------------------------------------------------------

// Local functions -------------------------------------------------------------

// Global Variables ------------------------------------------------------------

// Functions -------------------------------------------------------------------
// -----------------------------------------------------------------------------
//  PlButtonInitialize
// -----------------------------------------------------------------------------
PlErrCode PlButtonInitialize(void) {
  return PlButtonInitializeOsImpl();
}

// -----------------------------------------------------------------------------
//  PlButtonFinalize
// -----------------------------------------------------------------------------
PlErrCode PlButtonFinalize(void) {
  return PlButtonFinalizeOsImpl();
}

// -----------------------------------------------------------------------------
//  PlButtonGetInfo
// -----------------------------------------------------------------------------
PlErrCode PlButtonGetInfo(PlButtonInfo *info) {
  return PlButtonGetInfoOsImpl(info);
}

// -----------------------------------------------------------------------------
//  PlButtonRegisterHandler
// -----------------------------------------------------------------------------
PlErrCode PlButtonRegisterHandler(uint32_t button_id,
                                    PlButtonHandler handler,
                                    void *private_data) {
  return PlButtonRegisterHandlerOsImpl(button_id, handler, private_data);
}

// -----------------------------------------------------------------------------
//  PlButtonUnregisterHandler
// -----------------------------------------------------------------------------
PlErrCode PlButtonUnregisterHandler(uint32_t button_id) {
  return PlButtonUnregisterHandlerOsImpl(button_id);
}
