/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef _HAL_I2C_CAM_IMPL_H_
#define _HAL_I2C_CAM_IMPL_H_

void* HalI2cbusInitializeCamImpl(int port);
int   HalI2cbusFinalizeCamImpl(int port, void *handle);

#endif  // _HAL_I2C_CAM_IMPL_H_
