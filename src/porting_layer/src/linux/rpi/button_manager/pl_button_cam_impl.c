/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

// Includes --------------------------------------------------------------------
#include <stdio.h>

#include "pl.h"
#include "pl_button.h"

// Macros ----------------------------------------------------------------------

// Typedefs --------------------------------------------------------------------

// Local functions -------------------------------------------------------------

// Global Variables ------------------------------------------------------------

// Functions -------------------------------------------------------------------
// -----------------------------------------------------------------------------
//  PlButtonInitializeCamImpl
// -----------------------------------------------------------------------------
PlErrCode PlButtonInitializeCamImpl(void) {
  return kPlErrNoSupported;
}

// -----------------------------------------------------------------------------
//  PlButtonFinalizeCamImpl
// -----------------------------------------------------------------------------
PlErrCode PlButtonFinalizeCamImpl(void) {
  return kPlErrNoSupported;
}

// -----------------------------------------------------------------------------
//  PlButtonOpenCamImpl
// -----------------------------------------------------------------------------
PlErrCode PlButtonOpenCamImpl(uint32_t pin) {
  (void)pin;  // Avoid compiler warning
  return kPlErrNoSupported;
}

// -----------------------------------------------------------------------------
//  PlButtonCloseCamImpl
// -----------------------------------------------------------------------------
PlErrCode PlButtonCloseCamImpl(uint32_t pin) {
  (void)pin;  // Avoid compiler warning
  return kPlErrNoSupported;
}

// -----------------------------------------------------------------------------
//  PlButtonWaitEventCamImpl
// -----------------------------------------------------------------------------
PlErrCode PlButtonWaitEventCamImpl(void) {
  return kPlErrNoSupported;
}

// -----------------------------------------------------------------------------
//  PlButtonGetValCamImpl
// -----------------------------------------------------------------------------
PlErrCode PlButtonGetValCamImpl(uint32_t *pin, PlButtonStatus *btn_status) {
  (void)pin;  // Avoid compiler warning
  (void)btn_status;  // Avoid compiler warning
  return kPlErrNoSupported;
}

// -----------------------------------------------------------------------------
//  PlButtonRegisterHandlerCamImpl
// -----------------------------------------------------------------------------
PlErrCode PlButtonRegisterHandlerCamImpl(uint32_t pin,
                                      PlButtonStatus *btn_status) {
  (void)pin;  // Avoid compiler warning
  (void)btn_status;  // Avoid compiler warning
  return kPlErrNoSupported;
}

// -----------------------------------------------------------------------------
//  PlButtonUnregisterHandlerCamImpl
// -----------------------------------------------------------------------------
PlErrCode PlButtonUnregisterHandlerCamImpl(uint32_t pin) {
  (void)pin;  // Avoid compiler warning
  return kPlErrNoSupported;
}
