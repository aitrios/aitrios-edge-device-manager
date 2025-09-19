/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#include <stdio.h>
#include <stdbool.h>

#include "hal.h"
#include "hal_ioexp_cam_impl.h"

// ----------------------------------------------------------------------------
HalErrCode HalIoexpGetDataCamImpl(uint32_t ioexp_id,
                               uint32_t *device_id,
                               uint32_t *pin,
                               uint32_t *irq_num,
                               bool *reverse) {
  return kHalErrNoSupported;
}
// ----------------------------------------------------------------------------
