/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef __PL_BUTTON_CAM_IMPL_H
#define __PL_BUTTON_CAM_IMPL_H

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
PlErrCode PlButtonInitializeCamImpl(void);
PlErrCode PlButtonFinalizeCamImpl(void);
PlErrCode PlButtonOpenCamImpl(uint32_t pin);
PlErrCode PlButtonCloseCamImpl(uint32_t pin);
PlErrCode PlButtonWaitEventCamImpl(void);
PlErrCode PlButtonGetValCamImpl(uint32_t *pin, PlButtonStatus *btn_status);
PlErrCode PlButtonRegisterHandlerCamImpl(uint32_t pin,
                                          PlButtonStatus *btn_status);
PlErrCode PlButtonUnregisterHandlerCamImpl(uint32_t pin);

#endif /* __PL_BUTTON_CAM_IMPL_H */
