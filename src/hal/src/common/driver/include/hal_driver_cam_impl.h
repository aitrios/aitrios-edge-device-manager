/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef __HAL_DRIVER_CAM_IMPL_H
#define __HAL_DRIVER_CAM_IMPL_H

#include "hal.h"
#include "hal_driver.h"
#include "hal_driver_os_impl.h"

// Functions ------------------------------------------------------------------
HalErrCode HalDriverInitializeCamImpl(void);
HalErrCode HalDriverGetInitInfoCamImpl(struct DriverInfoList **info);
HalErrCode HalDriverFinalizeCamImpl(void);

#endif
