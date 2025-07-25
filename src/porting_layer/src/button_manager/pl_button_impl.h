/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef __PL_BUTTON_IMPL_H
#define __PL_BUTTON_IMPL_H

/*******************************************************************************
 * Included Files
 ******************************************************************************/
#include <stdio.h>
#include "pl.h"
#include "pl_button.h"

/*******************************************************************************
 * Pre-preprocessor Definitions
 ******************************************************************************/

/*******************************************************************************
 * Public Types
 ******************************************************************************/

/*******************************************************************************
 * Public Data
 ******************************************************************************/

/*******************************************************************************
 * Inline Functions
 ******************************************************************************/

/*******************************************************************************
 * Public Function Prototypes
 ******************************************************************************/
PlErrCode PlButtonInitializeImpl(void);
PlErrCode PlButtonFinalizeImpl(void);
PlErrCode PlButtonOpenImpl(uint32_t pin);
PlErrCode PlButtonCloseImpl(uint32_t pin);
PlErrCode PlButtonWaitEventImpl(void);
PlErrCode PlButtonGetValImpl(uint32_t *pin, PlButtonStatus *btn_status);
PlErrCode PlButtonRegisterHandlerImpl(uint32_t pin, PlButtonStatus *btn_status);
PlErrCode PlButtonUnregisterHandlerImpl(uint32_t pin);

#endif /* __PL_BUTTON_IMPL_H */
