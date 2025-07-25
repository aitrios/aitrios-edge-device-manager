/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef __HAL_DRIVER_H
#define __HAL_DRIVER_H

#include "hal.h"

// Macros ---------------------------------------------------------------------
typedef void* HalDriverHandle;

struct HalDriverOps {
  HalErrCode (*open)(uint32_t device_id);
  HalErrCode (*read)(void *buf, uint32_t size, uint32_t *read_size);
  HalErrCode (*write)(const void *buf, uint32_t size, uint32_t *written_size);
  HalErrCode (*close)(uint32_t device_id);
  HalErrCode (*ioctl)(void *arg, uint32_t cmd);
};
// Global Variables -----------------------------------------------------------

// Local functions ------------------------------------------------------------

// Functions ------------------------------------------------------------------
HalErrCode HalDriverOpen(uint32_t device_id, void *arg,
                        HalDriverHandle *handle);

HalErrCode HalDriverClose(HalDriverHandle handle);

HalErrCode HalDriverRead(HalDriverHandle handle, void *buf, uint32_t size,
                         uint32_t *read_size);

HalErrCode HalDriverWrite(HalDriverHandle handle, const void *buf,
                          uint32_t size, uint32_t *written_size);

HalErrCode HalDriverIoctl(HalDriverHandle handle, void *arg, uint32_t cmd);
HalErrCode HalDriverInitialize(void);
HalErrCode HalDriverFinalize(void);
HalErrCode HalDriverAddDriver(uint32_t device_id, const char *name,
                            const struct HalDriverOps *ops);
#endif
