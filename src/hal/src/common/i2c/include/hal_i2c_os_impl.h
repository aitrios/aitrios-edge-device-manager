/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef _HAL_I2C_OS_IMPL_H_
#define _HAL_I2C_OS_IMPL_H_

#include "hal.h"
#include "hal_i2c.h"

HalErrCode HalI2cInitializeOsImpl(void);
HalErrCode HalI2cFinalizeOsImpl(void);
HalErrCode HalI2cReadRegister8OsImpl(uint32_t device_id,
                                     uint8_t read_addr,
                                     uint8_t* read_buf);
HalErrCode HalI2cReadRegister16OsImpl(uint32_t device_id,
                                      uint16_t read_addr,
                                      uint16_t *read_buf,
                                      HalI2cEndian dev_endian);
HalErrCode HalI2cReadRegister32OsImpl(uint32_t device_id,
                                      uint32_t read_addr,
                                      uint32_t *read_buf,
                                      HalI2cEndian dev_endian);
HalErrCode HalI2cReadRegister64OsImpl(uint32_t device_id,
                                      uint64_t read_addr,
                                      uint64_t *read_buf,
                                      HalI2cEndian dev_endian);
HalErrCode HalI2cWriteRegister8OsImpl(uint32_t device_id,
                                      uint8_t  write_addr,
                                      const uint8_t *write_buf);
HalErrCode HalI2cWriteRegister16OsImpl(uint32_t device_id,
                                       uint16_t write_addr,
                                       const uint16_t *write_buf,
                                       HalI2cEndian dev_endian);
HalErrCode HalI2cWriteRegister32OsImpl(uint32_t device_id,
                                       uint32_t write_addr,
                                       const uint32_t * write_buf,
                                       HalI2cEndian dev_endian);
HalErrCode HalI2cWriteRegister64OsImpl(uint32_t device_id,
                                       uint64_t write_addr,
                                       const uint64_t *write_buf,
                                       HalI2cEndian dev_endian);
HalErrCode HalI2cGetDeviceInfoOsImpl(struct HalI2cDeviceInfo *device_info[],
                                     uint32_t *count);
HalErrCode HalI2cResetOsImpl(uint32_t device_id);
HalErrCode HalI2cLockOsImpl(void);
HalErrCode HalI2cUnlockOsImpl(void);
#endif  // _HAL_I2C_OS_IMPL_H_
