/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#include <stdio.h>
#include <stdbool.h>

#include "hal.h"
#include "hal_ioexp_os_impl.h"
#include "hal_ioexp_cam_impl.h"

// ----------------------------------------------------------------------------
HalErrCode HalIoexpGetDataOsImpl(uint32_t ioexp_id,
                            uint32_t *device_id,
                            uint32_t *pin,
                            uint32_t *irq_num,
                            bool *reverse) {
  return HalIoexpGetDataCamImpl(ioexp_id, device_id, pin, irq_num, reverse);
}
// ----------------------------------------------------------------------------
