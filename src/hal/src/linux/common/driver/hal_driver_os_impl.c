/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#include "hal.h"
#include "hal_driver.h"
#include "hal_driver_os_impl.h"
#include "hal_driver_cam_impl.h"

// Functions ------------------------------------------------------------------
HalErrCode HalDriverInitializeOsImpl(void) {
  return HalDriverInitializeCamImpl();
}
// ----------------------------------------------------------------------------
HalErrCode HalDriverGetInitInfoOsImpl(struct DriverInfoList **info) {
  return HalDriverGetInitInfoCamImpl(info);
}
// ----------------------------------------------------------------------------
HalErrCode HalDriverFinalizeOsImpl(void) {
  return HalDriverFinalizeCamImpl();
}
