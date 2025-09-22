/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef __PL_BUTTON_OS_IMPL_H
#define __PL_BUTTON_OS_IMPL_H

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
PlErrCode PlButtonInitializeOsImpl(void);
PlErrCode PlButtonFinalizeOsImpl(void);
PlErrCode PlButtonGetInfoOsImpl(PlButtonInfo *info);
PlErrCode PlButtonRegisterHandlerOsImpl(uint32_t button_id,
                                        PlButtonHandler handler,
                                        void *private_data);
PlErrCode PlButtonUnregisterHandlerOsImpl(uint32_t button_id);

#endif /* __PL_BUTTON_OS_IMPL_H */
