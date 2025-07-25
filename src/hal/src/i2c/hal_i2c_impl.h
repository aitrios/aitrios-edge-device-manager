/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef _HAL_I2C_IMPL_H_
#define _HAL_I2C_IMPL_H_

// Includes --------------------------------------------------------------------
#include <nuttx/i2c/i2c_master.h>

// Typedefs --------------------------------------------------------------------

// External functions ----------------------------------------------------------
struct i2c_master_s *HalI2cbusInitializeImpl(int port);
int HalI2cbusUninitializeImpl(int port, struct i2c_master_s *dev);
#endif  // _HAL_I2C_IMPL_H_
