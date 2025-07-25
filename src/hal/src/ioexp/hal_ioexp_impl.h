/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef __HAL_IOEXP_IMPL_H
#define __HAL_IOEXP_IMPL_H

#include "hal.h"
#include "hal_driver.h"

// Macros ---------------------------------------------------------------------

// Global Variables -----------------------------------------------------------

// Local functions ------------------------------------------------------------

// Functions ------------------------------------------------------------------
HalErrCode HalIoexpGetDataImpl(uint32_t ioexp_id,
                            uint32_t *device_id,
                            uint32_t *pin,
                            uint32_t *irq_num,
                            bool *reverse);
#endif
