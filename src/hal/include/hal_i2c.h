/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef _HAL_I2C_H_
#define _HAL_I2C_H_

// Includes --------------------------------------------------------------------
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "hal.h"

// Typedefs --------------------------------------------------------------------
typedef enum {
  kHalI2cLittleEndian = 1,
  kHalI2cBigEndian,

  kHalI2cEndianMax
} HalI2cEndian;

typedef enum {
  kHalI2cMsgTypeWrite = 0,
  kHalI2cMsgTypeRead,

  kHalI2cMsgTypeMax
} HalI2cMsgType;

struct HalI2cDeviceInfo {
  char          name[32 + 1];
  uint32_t      device_id;
  uint32_t      port;
  uint32_t      addr;
};
// External functions ----------------------------------------------------------
HalErrCode HalI2cInitialize(void);
HalErrCode HalI2cFinalize(void);
HalErrCode HalI2cReadRegister8(uint32_t device_id,
                               uint8_t read_addr,
                               uint8_t* read_buf);
HalErrCode HalI2cReadRegister16(uint32_t device_id,
                                uint16_t read_addr,
                                uint16_t *read_buf,
                                HalI2cEndian dev_endian);
HalErrCode HalI2cReadRegister32(uint32_t device_id,
                                uint32_t read_addr,
                                uint32_t *read_buf,
                                HalI2cEndian dev_endian);
HalErrCode HalI2cReadRegister64(uint32_t device_id,
                                uint64_t read_addr,
                                uint64_t *read_buf,
                                HalI2cEndian dev_endian);
HalErrCode HalI2cWriteRegister8(uint32_t device_id,
                                uint8_t  write_addr,
                                const uint8_t *write_buf);
HalErrCode HalI2cWriteRegister16(uint32_t device_id,
                                 uint16_t write_addr,
                                 const uint16_t *write_buf,
                                 HalI2cEndian dev_endian);
HalErrCode HalI2cWriteRegister32(uint32_t device_id,
                                 uint32_t write_addr,
                                 const uint32_t * write_buf,
                                 HalI2cEndian dev_endian);
HalErrCode HalI2cWriteRegister64(uint32_t device_id,
                                 uint64_t write_addr,
                                 const uint64_t *write_buf,
                                 HalI2cEndian dev_endian);
HalErrCode HalI2cGetDeviceInfo(struct HalI2cDeviceInfo *device_info[],
                               uint32_t *count);
HalErrCode HalI2cReset(uint32_t device_id);
HalErrCode HalI2cLock(void);
HalErrCode HalI2cUnlock(void);
#endif  // _HAL_I2C_H_
