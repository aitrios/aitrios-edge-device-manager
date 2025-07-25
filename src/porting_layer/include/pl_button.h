/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef __PL_BUTTON_H
#define __PL_BUTTON_H

/*******************************************************************************
 * Included Files
 ******************************************************************************/

#include "pl.h"

/*******************************************************************************
 * Pre-preprocessor Definitions
 ******************************************************************************/

/*******************************************************************************
 * Public Types
 ******************************************************************************/

typedef enum {
  kPlButtonStatusReleased = 0,
  kPlButtonStatusPressed,
  kPlButtonStatusMax
} PlButtonStatus;

typedef struct {
  uint32_t button_ids[CONFIG_EXTERNAL_PL_BUTTON_NUM];
  uint32_t button_total_num;
} PlButtonInfo;

typedef void (*PlButtonHandler)(PlButtonStatus status, void *private_data);

/*******************************************************************************
 * Public Data
 ******************************************************************************/

/*******************************************************************************
 * Inline Functions
 ******************************************************************************/

/*******************************************************************************
 * Public Function Prototypes
 ******************************************************************************/
PlErrCode PlButtonInitialize(void);
PlErrCode PlButtonFinalize(void);
PlErrCode PlButtonGetInfo(PlButtonInfo *info);
PlErrCode PlButtonRegisterHandler(uint32_t button_id,
                                    PlButtonHandler handler,
                                    void *private_data);
PlErrCode PlButtonUnregisterHandler(uint32_t button_id);

#endif /* __PL_BUTTON_H */
