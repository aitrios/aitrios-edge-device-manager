/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#include "hal.h"
#include "hal_i2c.h"
#include "hal_i2c_os_impl.h"
// -----------------------------------------------------------------------------
HalErrCode HalI2cInitialize(void) {
  return HalI2cInitializeOsImpl();
}
// -----------------------------------------------------------------------------
HalErrCode HalI2cFinalize(void) {
  return HalI2cFinalizeOsImpl();
}
// -----------------------------------------------------------------------------
HalErrCode HalI2cReadRegister8(uint32_t device_id,
                               uint8_t read_addr,
                               uint8_t* read_buf) {
  return HalI2cReadRegister8OsImpl(device_id, read_addr, read_buf);
}
// -----------------------------------------------------------------------------
HalErrCode HalI2cReadRegister16(uint32_t device_id,
                                uint16_t read_addr,
                                uint16_t *read_buf,
                                HalI2cEndian dev_endian) {
  return HalI2cReadRegister16OsImpl(device_id, read_addr, read_buf, dev_endian);
}
// -----------------------------------------------------------------------------
HalErrCode HalI2cReadRegister32(uint32_t device_id,
                                uint32_t read_addr,
                                uint32_t *read_buf,
                                HalI2cEndian dev_endian) {
  return HalI2cReadRegister32OsImpl(device_id, read_addr, read_buf, dev_endian);
}
// -----------------------------------------------------------------------------
HalErrCode HalI2cReadRegister64(uint32_t device_id,
                                uint64_t read_addr,
                                uint64_t *read_buf,
                                HalI2cEndian dev_endian) {
  return HalI2cReadRegister64OsImpl(device_id, read_addr, read_buf, dev_endian);
}
// -----------------------------------------------------------------------------
HalErrCode HalI2cWriteRegister8(uint32_t device_id,
                                uint8_t  write_addr,
                                const uint8_t *write_buf) {
  return HalI2cWriteRegister8OsImpl(device_id, write_addr, write_buf);
}
// -----------------------------------------------------------------------------
HalErrCode HalI2cWriteRegister16(uint32_t device_id,
                                 uint16_t write_addr,
                                 const uint16_t *write_buf,
                                 HalI2cEndian dev_endian) {
  return HalI2cWriteRegister16OsImpl(device_id, write_addr,
                                     write_buf, dev_endian);
}
// -----------------------------------------------------------------------------
HalErrCode HalI2cWriteRegister32(uint32_t device_id,
                                 uint32_t write_addr,
                                 const uint32_t * write_buf,
                                 HalI2cEndian dev_endian) {
  return HalI2cWriteRegister32OsImpl(device_id, write_addr,
                                     write_buf, dev_endian);
}
// -----------------------------------------------------------------------------
HalErrCode HalI2cWriteRegister64(uint32_t device_id,
                                 uint64_t write_addr,
                                 const uint64_t *write_buf,
                                 HalI2cEndian dev_endian) {
  return HalI2cWriteRegister64OsImpl(device_id, write_addr,
                                     write_buf, dev_endian);
}
// -----------------------------------------------------------------------------
HalErrCode HalI2cGetDeviceInfo(struct HalI2cDeviceInfo *device_info[],
                               uint32_t *count) {
  return HalI2cGetDeviceInfoOsImpl(device_info, count);
}
// -----------------------------------------------------------------------------
HalErrCode HalI2cReset(uint32_t device_id) {
  return HalI2cResetOsImpl(device_id);
}
// -----------------------------------------------------------------------------
HalErrCode HalI2cLock(void) {
  return HalI2cLockOsImpl();
}
// -----------------------------------------------------------------------------
HalErrCode HalI2cUnlock(void) {
  return HalI2cUnlockOsImpl();
}
// -----------------------------------------------------------------------------
