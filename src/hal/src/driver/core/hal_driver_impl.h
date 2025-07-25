/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef __HAL_DRIVER_IMPL_H
#define __HAL_DRIVER_IMPL_H

#ifdef __linux__
#include <bsd/sys/queue.h>
#endif
#ifdef __NuttX__
#include <sys/queue.h>
#endif
#include "hal.h"
#include "hal_driver.h"

// Macros ---------------------------------------------------------------------
#define DRIVER_NAME_LEN (32)

struct HalDriverInfo {
  TAILQ_ENTRY(HalDriverInfo) head;
  uint32_t device_id;
  char name[DRIVER_NAME_LEN + 1];
  struct HalDriverOps *ops;
};
TAILQ_HEAD(DriverInfoList, HalDriverInfo);

// Global Variables -----------------------------------------------------------

// Local functions ------------------------------------------------------------

// Functions ------------------------------------------------------------------
HalErrCode HalDriverInitializeImpl(void);
HalErrCode HalDriverGetInitInfoImpl(struct DriverInfoList **info);
HalErrCode HalDriverFinalizeImpl(void);

#endif
