/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

// Includes --------------------------------------------------------------------
#include <inttypes.h>
#include <errno.h>
#include <endian.h>
#include <pthread.h>
#include <unistd.h>

#include "hal.h"
#include "hal_i2c.h"
#include "hal_i2c_impl.h"
#include "utility_log.h"
#include "utility_log_module_id.h"

// Typedefs --------------------------------------------------------------------
typedef union {
  uint8_t     u8reg[8];
  uint16_t    u16reg;
  uint32_t    u32reg;
  uint64_t    u64reg;
} CommandRegister;

typedef struct {
  uint16_t    reg_offset;
  union {
    uint16_t  u16;
    uint32_t  u32;
    uint64_t  u64;
    uint8_t   u8bytes[8];
  } union_64bits;
} __attribute__((packed)) AddrAndData;

struct I2cDeviceInfo {
  char          name[32 + 1];
  uint32_t      device_id;
  uint32_t      port;
  uint32_t      addr;
  uint32_t      frequency;
};

#define EVENT_ID        (0x9B00)
#define EVENT_ID_START  (0x00)
#define EVENT_UID_START (0x00)

#define LOG_E(event_id, format, ...) \
  WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:" \
                   format, __FILE__, __LINE__, ##__VA_ARGS__); \
  WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (EVENT_ID | (EVENT_ID_START + event_id)))

#define LOG_W(event_id, format, ...) \
  WRITE_DLOG_WARN(MODULE_ID_SYSTEM, "%s-%d:" \
                  format, __FILE__, __LINE__, ##__VA_ARGS__); \
  WRITE_ELOG_WARN(MODULE_ID_SYSTEM, (EVENT_ID | (EVENT_ID_START + event_id)))

#define LOG_I(format, ...) \
  WRITE_DLOG_INFO(MODULE_ID_SYSTEM, "%s-%d:" \
                  format, __FILE__, __LINE__, ##__VA_ARGS__)

#define LOG_D(format, ...) \
  WRITE_DLOG_DEBUG(MODULE_ID_SYSTEM, "%s-%d:" \
                   format, __FILE__, __LINE__, ##__VA_ARGS__)

// External functions ----------------------------------------------------------

// Local variables -------------------------------------------------------------
                // Mutex for exclusive processing
static pthread_mutex_t           s_mutex = PTHREAD_MUTEX_INITIALIZER;
static struct HalI2cDeviceInfo  *s_i2c_device_info = NULL;
static uint32_t                  s_i2c_device_num  = 0;
static bool                      s_i2c_initialized = false;

static struct I2cDeviceInfo  s_all_device_info[CONFIG_HAL_I2C_DEVICE_NUM] = {0};
static uint32_t              s_all_device_num = CONFIG_HAL_I2C_DEVICE_NUM;
static bool                  s_used_port[CONFIG_HAL_I2C_DEVICE_NUM] = {false};
static struct i2c_master_s  *s_handle[CONFIG_HAL_I2C_DEVICE_NUM] = {NULL};
// Local functions -------------------------------------------------------------
static HalErrCode InitializeI2cPort(uint32_t i2c_port);
static HalErrCode UninitializeI2cPort(uint32_t i2c_port);
static HalErrCode HalI2cTransfer(uint32_t device_id,
                                const uint8_t *transfer_data[],
                                const uint8_t  transfer_size[],
                                const bool     is_read[],
                                uint32_t trans_num,
                                HalI2cEndian dev_endian);
static int  ResetAndRetransmit(uint32_t i2c_port,
                               struct i2c_master_s port_handle[],
                               struct i2c_msg_s i2c_msg[],
                               uint32_t trans_num);
static void SwapDataByEndian(uint8_t *data, int data_len);
static bool IsValidParameter(uint32_t device_id, const void *buf_ptr);
static bool IsValidDeviceId(uint32_t device_id);
static HalErrCode RegisterWriteAndRead(uint32_t device_id,
                                       uint64_t read_addr,
                                       uint16_t addr_bytes,
                                       uint8_t *read_buff,
                                       uint16_t read_bytes,
                                       HalI2cEndian dev_endian);
static HalErrCode RegisterWriteAndWrite(uint32_t device_id,
                                        uint64_t write_addr,
                                        uint16_t addr_bytes,
                                        uint8_t *write_buff,
                                        uint16_t write_bytes,
                                        HalI2cEndian dev_endian);
static HalErrCode  GetDevInfoByDeviceId(uint32_t device_id,
                                        struct I2cDeviceInfo *dev_info[]);
static HalErrCode CollectDeviceInfoForI2c(void);
static void RegisterDeviceInfo(uint32_t idx, const char *name,
                               uint32_t dev_id, uint32_t port,
                               uint32_t frequency, uint32_t addr);

// -----------------------------------------------------------------------------
HalErrCode HalI2cInitialize(void) {
  HalErrCode err_code = kHalErrCodeOk;

  int ret = pthread_mutex_lock(&s_mutex);
  if (ret) {
    LOG_E(0x00, "pthread_mutex_lock() fail:%d, errno=%d", ret, errno);
    return kHalErrLock;
  }

  if (s_i2c_initialized) {
    LOG_E(0x01, "I2c is already initialized.");
    err_code = kHalErrInvalidState;
    goto exit_to_release_mutex;
  }
  err_code = CollectDeviceInfoForI2c();
  if (kHalErrCodeOk != err_code) {
    LOG_W(0x02, "CollectDeviceInfoForI2c fail:%u", err_code);
    goto exit_to_release_mutex;
  }
  // Initialize I2C ports
  for (uint32_t i_port_num = 0;
       i_port_num < s_all_device_num; i_port_num++) {
    if (s_used_port[i_port_num]) {
      err_code = InitializeI2cPort(i_port_num);
      if (kHalErrCodeOk != err_code) {
        LOG_W(0x03, "InitializeI2cPort fail:%u, i_port_num=%u",
              err_code, i_port_num);
        goto exit_to_release_mutex;
      }
    }
  }
  s_i2c_initialized = true;

exit_to_release_mutex:
  ret = pthread_mutex_unlock(&s_mutex);
  if (ret) {
    LOG_E(0x04, "pthread_mutex_unlock() fail:%d, errno=%d", ret, errno);
  }
  if (err_code != kHalErrCodeOk) {
    if (s_i2c_device_info) {
      free(s_i2c_device_info);
      s_i2c_device_info = NULL;
    }
    s_i2c_initialized = false;
    for (uint32_t i_port_num = 0; i_port_num < CONFIG_HAL_I2C_DEVICE_NUM;
         i_port_num++) {
      s_handle[i_port_num] = NULL;
    }
  }

  LOG_D("[OUT] err_code=%u", err_code);
  return err_code;
}

static HalErrCode InitializeI2cPort(uint32_t i2c_port) {
  // i2c_port is verified in caller function.
  if (s_handle[i2c_port]) {
    LOG_W(0x06, "port %u was already opened.", i2c_port);
    return kHalErrInvalidState;
  }
  s_handle[i2c_port] = HalI2cbusInitializeImpl(i2c_port);
  if (NULL == s_handle[i2c_port]) {
    LOG_E(0x07, "HalI2cbusInitializeImpl(%u) error", i2c_port);
    return kHalErrOpen;
  } else {
    LOG_D("s_handle[%u] : %p", i2c_port, s_handle[i2c_port]);
  }
  return kHalErrCodeOk;
}

// -----------------------------------------------------------------------------
HalErrCode HalI2cFinalize(void) {
  HalErrCode err_code = kHalErrCodeOk;

  int ret = pthread_mutex_lock(&s_mutex);
  if (ret) {
    LOG_E(0x08, "pthread_mutex_lock() fail:%d, errno=%d", ret, errno);
    return kHalErrLock;
  }

  if (false == s_i2c_initialized) {
    LOG_E(0x09, "I2c is already finalized.");
    err_code = kHalErrInvalidState;
    goto exit_to_release_mutex;
  }
  // Release memory, handle, etc.
  if (s_i2c_device_info) {
    free(s_i2c_device_info);
    s_i2c_device_info = NULL;
  }
  for (uint32_t i_port_num = 0;
       i_port_num < s_all_device_num; i_port_num++) {
    if (s_used_port[i_port_num]) {
      err_code = UninitializeI2cPort(i_port_num);
      if (kHalErrCodeOk != err_code) {
        LOG_W(0x0A, "UninitializeI2cPort(%u) fail:%u", i_port_num, err_code);
        goto exit_to_release_mutex;
      }
    }
  }
  s_i2c_initialized = false;

exit_to_release_mutex:
  ret = pthread_mutex_unlock(&s_mutex);
  if (ret) {
    LOG_E(0x0B, "pthread_mutex_unlock() fail:%d, errno=%d", ret, errno);
  }

  return err_code;
}

static HalErrCode UninitializeI2cPort(uint32_t i2c_port) {
  if ((0 != i2c_port) && (1 != i2c_port)) {
    LOG_E(0x0C, "param error. i2c_port=%u", i2c_port);
    return kHalErrInvalidParam;
  }
  if (NULL == s_handle[i2c_port]) {
    LOG_W(0x0D, "port %u was already closed.", i2c_port);
    return kHalErrInvalidState;
  }
  int ret_code = 0;
  ret_code = HalI2cbusUninitializeImpl(i2c_port, s_handle[i2c_port]);
  if (0 != ret_code) {
    LOG_E(0x0E, "HalI2cbusUninitializeImpl(%u) error=%d", i2c_port, ret_code);
    return kHalErrOpen;
  }
  s_handle[i2c_port] = NULL;
  LOG_D("port %u finalized.", i2c_port);
  return kHalErrCodeOk;
}

// -----------------------------------------------------------------------------
static HalErrCode HalI2cTransfer(uint32_t device_id,
                          const uint8_t *transfer_data[],
                          const uint8_t  transfer_size[],
                          const bool     is_read[],
                          uint32_t trans_num,
                          HalI2cEndian dev_endian) {
  LOG_D("device_id=%u, trans_num=%u, dev_endian=%u",
        device_id, trans_num, dev_endian);

  struct I2cDeviceInfo *dev_info = NULL;
  // Get addr from device_info.
  HalErrCode err_code = GetDevInfoByDeviceId(device_id, &dev_info);
  if (err_code != kHalErrCodeOk) {
    LOG_E(0x0F, "GetDevInfoByDeviceId(dev_id:%u) fail:%u", device_id, err_code);
    goto exit_this_func;
  }

  struct i2c_msg_s i2c_msg[8] = {0};
  if (trans_num >= 8) {
    LOG_E(0x10, "** Coding : i2c_msg[8] is short. trans_num=%u", trans_num);
    err_code = kHalErrMemory;
    goto exit_this_func;
  }
  for (uint32_t i_trans = 0; i_trans < trans_num; i_trans++) {
    i2c_msg[i_trans].frequency = dev_info->frequency;
    i2c_msg[i_trans].addr  = dev_info->addr;
    i2c_msg[i_trans].flags = is_read[i_trans] ? kHalI2cMsgTypeRead
                                              : kHalI2cMsgTypeWrite;
    // Write buffer was converted to target endian.
    i2c_msg[i_trans].buffer = (uint8_t *)transfer_data[i_trans];
    i2c_msg[i_trans].length = (ssize_t)transfer_size[i_trans];
  }

  uint32_t port = dev_info->port;
  if (s_handle[port] != NULL) {
    // ---- Call I2C_TRANSFER() ----
    int i2c_ret = I2C_TRANSFER(s_handle[port], i2c_msg, trans_num);
    if (i2c_ret < 0) {
      i2c_ret = ResetAndRetransmit(port, s_handle[port], i2c_msg, trans_num);
      if (i2c_ret < 0) {
        LOG_E(0x11, "I2C_TRANSFER() failed: %d.", i2c_ret);
        err_code = (i2c_ret == -ETIMEDOUT ? kHalErrTimedout : kHalErrTransfer);
      }
    }
  } else {
    LOG_E(0x12, "s_handle[%u] is invalid param.", port);
    err_code = kHalErrCodeError;
  }
  if (err_code != kHalErrCodeOk) {
    goto exit_this_func;
  }

  if (((BYTE_ORDER == BIG_ENDIAN) && (dev_endian != kHalI2cBigEndian)) ||
      ((BYTE_ORDER != BIG_ENDIAN) && (dev_endian == kHalI2cBigEndian))) {
    for (uint32_t i_trans = 0; i_trans < trans_num; i_trans++) {
      if (i2c_msg[i_trans].flags == kHalI2cMsgTypeRead) {
        if (i2c_msg[i_trans].length >= 2) {
          SwapDataByEndian(i2c_msg[i_trans].buffer,
                           i2c_msg[i_trans].length);
        }
        memcpy((void *)transfer_data[i_trans],
               i2c_msg[i_trans].buffer, i2c_msg[i_trans].length);
      }
    }
  }

exit_this_func:
  LOG_D("err_code=%u", err_code);
  return err_code;
}

static void SwapDataByEndian(uint8_t* data, int data_len) {
  if ((data_len < 2) || (data_len & 1)) {
    LOG_E(0x13, "data_len(%d) is not even value.", data_len);
    return;
  }
  int i_tail = data_len - 1;
  for (int i_buf = 0; i_buf < data_len; i_buf++) {
    uint8_t swap_byte = data[i_tail - i_buf];
    data[i_tail - i_buf] = data[i_buf];
    data[i_buf] = swap_byte;
  }
}

static int ResetAndRetransmit(uint32_t i2c_port,
                              struct i2c_master_s port_handle[],
                              struct i2c_msg_s i2c_msg[],
                              uint32_t trans_num) {
  LOG_D("i2c_port=%u, port_handle=%p, trans_num=%u",
        i2c_port, port_handle, trans_num);
  int i2c_ret = -1;
#ifdef CONFIG_I2C_RESET
  int retry_cnt = 0;
  while (retry_cnt < CONFIG_HAL_I2C_ERROR_RETRY) {
    LOG_E(0x14, "Retry I2C TRANSFER : %d/%d",
          retry_cnt + 1, CONFIG_HAL_I2C_ERROR_RETRY);
    i2c_ret = I2C_RESET(s_handle[i2c_port]);
    if (i2c_ret < 0) {
      LOG_E(0x15, "I2C_RESET() failed: %d", i2c_ret);
      return i2c_ret;
    }
    for (uint32_t i_loop = 0; i_loop < trans_num; i_loop++) {
      usleep(5000);
      i2c_ret = I2C_TRANSFER(port_handle, &i2c_msg[i_loop], 1);
      if (i2c_ret != 0) {
        LOG_E(0x16, "I2C_TRANSFER(i2c_msg[%u]) error=%d", i_loop, i2c_ret);
        goto do_retry;
      }
    }
    if (i2c_ret == 0) {
      LOG_D("Retransmission OK.");
      return 0;
    }

do_retry:
    retry_cnt++;
  }
  for (uint32_t i_msg = 0; i_msg < trans_num; i_msg++) {
    LOG_E(0x17, "i2c_msg[%u].frequency:%u", i_msg, i2c_msg[i_msg].frequency);
    LOG_E(0x17, "i2c_msg[%u].addr     :0x%04X", i_msg, i2c_msg[i_msg].addr);
    LOG_E(0x17, "i2c_msg[%u].flags    :%d", i_msg, i2c_msg[i_msg].flags);
    LOG_E(0x17, "i2c_msg[%u].buffer   :%p", i_msg, i2c_msg[i_msg].buffer);
    LOG_E(0x17, "i2c_msg[%u].length   :%u", i_msg,
          (uint32_t)i2c_msg[i_msg].length);
  }
#else
  (void)i2c_msg;  // Avoid compiler warning
  LOG_E(0x18, "Not support I2C Bus reset.");
#endif
  return i2c_ret;
}

// -----------------------------------------------------------------------------
HalErrCode HalI2cReadRegister8(uint32_t device_id,
                               uint8_t read_addr,
                               uint8_t *read_buf) {
  int ret = pthread_mutex_lock(&s_mutex);
  if (ret) {
    LOG_E(0x19, "pthread_mutex_lock() fail:%d", ret);
    return kHalErrLock;
  }

  HalErrCode err_code = kHalErrCodeOk;
  if (s_i2c_initialized == false) {
    LOG_E(0x1A, "I2c is not initialized.");
    err_code = kHalErrInvalidState;
    goto exit_to_release_mutex;
  }
  if (IsValidParameter(device_id, (const void *)read_buf) == false) {
    err_code = kHalErrInvalidParam;
    goto exit_to_release_mutex;
  }
  LOG_D("[IN] device_id=%u, read_addr=0x%04X, read_buf=%p",
        device_id, read_addr, read_buf);
  err_code = RegisterWriteAndRead(device_id, (uint64_t)read_addr, 1,
                                  read_buf, 1, kHalI2cLittleEndian);

exit_to_release_mutex:
  ret = pthread_mutex_unlock(&s_mutex);
  if (ret) {
    LOG_E(0x1B, "pthread_mutex_unlock() fail:%d", ret);
  }
  LOG_D("[OUT] err_code=%un", err_code);
  return err_code;
}

// -----------------------------------------------------------------------------
HalErrCode HalI2cReadRegister16(uint32_t device_id,
                                uint16_t read_addr,
                                uint16_t *read_buf,
                                HalI2cEndian dev_endian) {
  int ret = pthread_mutex_lock(&s_mutex);
  if (ret) {
    LOG_E(0x1C, "pthread_mutex_lock() fail:%d", ret);
    return kHalErrLock;
  }

  HalErrCode err_code = kHalErrCodeOk;
  if (s_i2c_initialized == false) {
    LOG_E(0x1D, "I2c is not initialized.");
    err_code = kHalErrInvalidState;
    goto exit_to_release_mutex;
  }
  if (IsValidParameter(device_id, (const void *)read_buf) == false) {
    err_code = kHalErrInvalidParam;
    goto exit_to_release_mutex;
  }
  LOG_D("[IN] device_id=%u, read_addr=0x%04X, read_buf=%p, dev_endian=%u",
        device_id, read_addr, read_buf, dev_endian);

  union {
    uint16_t  u16;
    uint8_t   u8_2[2];
  } __attribute__((packed)) union_16bits = {0};
  err_code = RegisterWriteAndRead(device_id, (uint64_t)read_addr, 2,
                                  (uint8_t *)&union_16bits, 2, dev_endian);
  if (err_code == kHalErrCodeOk) {
    *read_buf = union_16bits.u16;
    LOG_D("buff:0x%02X, 0x%02X", union_16bits.u8_2[0], union_16bits.u8_2[1]);
  }
exit_to_release_mutex:
  ret = pthread_mutex_unlock(&s_mutex);
  if (ret) {
    LOG_E(0x1E, "pthread_mutex_unlock() fail:%d", ret);
  }
  LOG_D("[OUT] err_code=%u", err_code);
  return err_code;
}

// -----------------------------------------------------------------------------
HalErrCode HalI2cReadRegister32(uint32_t device_id,
                                uint32_t read_addr,
                                uint32_t *read_buf,
                                HalI2cEndian dev_endian) {
  int ret = pthread_mutex_lock(&s_mutex);
  if (ret) {
    LOG_E(0x1F, "pthread_mutex_lock() fail:%d", ret);
    return kHalErrLock;
  }

  HalErrCode err_code = kHalErrCodeOk;
  if (s_i2c_initialized == false) {
    LOG_E(0x20, "I2c is not initialized.");
    err_code = kHalErrInvalidState;
    goto exit_to_release_mutex;
  }
  if (IsValidParameter(device_id, (const void *)read_buf) == false) {
    err_code = kHalErrInvalidParam;
    goto exit_to_release_mutex;
  }
  LOG_D("[IN] device_id=%u, read_addr=0x%04X, read_buf=%p, dev_endian=%u",
        device_id, read_addr, read_buf, dev_endian);
  union {
    uint32_t  u32;
    uint8_t   u8_4[4];
  } __attribute__((packed)) union_32bits = {0};
  err_code = RegisterWriteAndRead(device_id, (uint64_t)read_addr, 4,
                                  (uint8_t *)&union_32bits, 4, dev_endian);
  if (err_code == kHalErrCodeOk) {
    *read_buf = union_32bits.u32;
    LOG_D("buff:0x%02X, 0x%02X, 0x%02X, 0x%02X",
          union_32bits.u8_4[0], union_32bits.u8_4[1],
          union_32bits.u8_4[2], union_32bits.u8_4[3]);
  }
exit_to_release_mutex:
  ret = pthread_mutex_unlock(&s_mutex);
  if (ret) {
    LOG_E(0x21, "pthread_mutex_unlock() fail:%d", ret);
  }
  LOG_D("[OUT] err_code=%u", err_code);
  return err_code;
}

// -----------------------------------------------------------------------------
HalErrCode HalI2cReadRegister64(uint32_t device_id,
                                uint64_t read_addr,
                                uint64_t *read_buf,
                                HalI2cEndian dev_endian) {
  int ret = pthread_mutex_lock(&s_mutex);
  if (ret) {
    LOG_E(0x22, "pthread_mutex_lock() fail:%d", ret);
    return kHalErrLock;
  }

  HalErrCode err_code = kHalErrCodeOk;
  if (s_i2c_initialized == false) {
    LOG_E(0x23, "I2c is not initialized.");
    err_code = kHalErrInvalidState;
    goto exit_to_release_mutex;
  }
  if (IsValidParameter(device_id, (const void *)read_buf) == false) {
    err_code = kHalErrInvalidParam;
    goto exit_to_release_mutex;
  }
  LOG_D("[IN] device_id=%u, read_addr=0x%"PRIx64", read_buf=%p, dev_endian=%u",
        device_id, read_addr, read_buf, dev_endian);

  union {
    uint64_t  u64;
    uint8_t   u8_8[8];
  } __attribute__((packed))union_64bits = {0};
  err_code = RegisterWriteAndRead(device_id, read_addr, 8,
                                  (uint8_t *)&union_64bits, 8, dev_endian);
  if (err_code == kHalErrCodeOk) {
    *read_buf = union_64bits.u64;
    LOG_D("buff:0x%02X, 0x%02X, 0x%02X, 0x%02X," \
          " 0x%02X, 0x%02X, 0x%02X, 0x%02X",
          union_64bits.u8_8[0], union_64bits.u8_8[1],
          union_64bits.u8_8[2], union_64bits.u8_8[3],
          union_64bits.u8_8[4], union_64bits.u8_8[5],
          union_64bits.u8_8[6], union_64bits.u8_8[7]);
  }
exit_to_release_mutex:
  ret = pthread_mutex_unlock(&s_mutex);
  if (ret) {
    LOG_E(0x24, "pthread_mutex_unlock() fail:%d", ret);
  }
  LOG_D("[OUT] err_code=%u", err_code);
  return err_code;
}

// -----------------------------------------------------------------------------
HalErrCode HalI2cWriteRegister8(uint32_t device_id,
                                uint8_t write_addr,
                                const uint8_t *write_buf) {
  int ret = pthread_mutex_lock(&s_mutex);
  if (ret) {
    LOG_E(0x25, "pthread_mutex_lock() fail:%d", ret);
    return kHalErrLock;
  }

  HalErrCode err_code = kHalErrCodeOk;
  if (s_i2c_initialized == false) {
    LOG_E(0x26, "I2c is not initialized.");
    err_code = kHalErrInvalidState;
    goto exit_to_release_mutex;
  }
  if (IsValidParameter(device_id, (const void *)write_buf) == false) {
    err_code = kHalErrInvalidParam;
    goto exit_to_release_mutex;
  }
  LOG_D("[IN] device_id=%u, write_addr=0x%04X", device_id, write_addr);
  uint8_t  addr_and_data[2] = {0};
  addr_and_data[0] =  write_addr;
  addr_and_data[1] = *write_buf;
  uint8_t *transfer_data[1] = {NULL};
  transfer_data[0] = addr_and_data;
  uint8_t  transfer_size[1] = {2};
  bool     is_read[1] = {false};
  err_code = HalI2cTransfer(device_id, (const uint8_t **)transfer_data,
                            transfer_size, is_read, 1, kHalI2cLittleEndian);
exit_to_release_mutex:
  ret = pthread_mutex_unlock(&s_mutex);
  if (ret) {
    LOG_E(0x27, "pthread_mutex_unlock() fail:%d", ret);
  }
  LOG_D("[OUT] err_code=%u", err_code);
  return err_code;
}

// -----------------------------------------------------------------------------
HalErrCode HalI2cWriteRegister16(uint32_t device_id,
                                 uint16_t write_addr,
                                 const uint16_t *write_buf,
                                 HalI2cEndian dev_endian) {
  int ret = pthread_mutex_lock(&s_mutex);
  if (ret) {
    LOG_E(0x28, "pthread_mutex_lock() fail:%d", ret);
    return kHalErrLock;
  }

  HalErrCode err_code = kHalErrCodeOk;
  if (s_i2c_initialized == false) {
    LOG_E(0x29, "I2c is not initialized.");
    err_code = kHalErrInvalidState;
    goto exit_to_release_mutex;
  }
  if (IsValidParameter(device_id, (const void *)write_buf) == false) {
    err_code = kHalErrInvalidParam;
    goto exit_to_release_mutex;
  }
  LOG_D("[IN] device_id=%u, write_addr=0x%04X, " \
        "*write_buf=0x%04X, dev_endian=%u",
        device_id, write_addr, *write_buf, dev_endian);
  uint16_t write_data = *write_buf;
  err_code = RegisterWriteAndWrite(device_id, (uint64_t)write_addr, 2,
                                   (uint8_t *)&write_data, 2, dev_endian);
exit_to_release_mutex:
  ret = pthread_mutex_unlock(&s_mutex);
  if (ret) {
    LOG_E(0x2A, "pthread_mutex_unlock() fail:%d", ret);
  }
  LOG_D("[OUT] err_code=%u", err_code);
  return err_code;
}

// -----------------------------------------------------------------------------
HalErrCode HalI2cWriteRegister32(uint32_t device_id,
                                 uint32_t write_addr,
                                 const uint32_t * write_buf,
                                 HalI2cEndian dev_endian) {
  int ret = pthread_mutex_lock(&s_mutex);
  if (ret) {
    LOG_E(0x2B, "pthread_mutex_lock() fail:%d", ret);
    return kHalErrLock;
  }

  HalErrCode err_code = kHalErrCodeOk;
  if (s_i2c_initialized == false) {
    LOG_E(0x2C, "I2c is not initialized.");
    err_code = kHalErrInvalidState;
    goto exit_to_release_mutex;
  }
  if (IsValidParameter(device_id, (const void *)write_buf) == false) {
    err_code = kHalErrInvalidParam;
    goto exit_to_release_mutex;
  }
  LOG_D("[IN] device_id=%u, write_addr=0x%04X, dev_endian=%u",
        device_id, write_addr, dev_endian);
  err_code = RegisterWriteAndWrite(device_id, (uint64_t)write_addr, 4,
                                   (uint8_t *)write_buf, 4, dev_endian);
exit_to_release_mutex:
  ret = pthread_mutex_unlock(&s_mutex);
  if (ret) {
    LOG_E(0x2D, "pthread_mutex_unlock() fail:%d", ret);
  }
  LOG_D("[OUT] err_code=%u", err_code);
  return err_code;
}

// -----------------------------------------------------------------------------
HalErrCode HalI2cWriteRegister64(uint32_t device_id,
                                 uint64_t write_addr,
                                 const uint64_t *write_buf,
                                 HalI2cEndian dev_endian) {
  int ret = pthread_mutex_lock(&s_mutex);
  if (ret) {
    LOG_E(0x2E, "pthread_mutex_lock() fail:%d", ret);
    return kHalErrLock;
  }

  HalErrCode err_code = kHalErrCodeOk;
  if (s_i2c_initialized == false) {
    LOG_E(0x2F, "I2c is not initialized.");
    err_code = kHalErrInvalidState;
    goto exit_to_release_mutex;
  }
  if (IsValidParameter(device_id, (const void *)write_buf) == false) {
    err_code = kHalErrInvalidParam;
    goto exit_to_release_mutex;
  }
  LOG_D("[IN] device_id=%u, write_addr=0x%"PRIx64", *write_buf=0x%"PRIx64", " \
        "dev_endian=%u\n", device_id, write_addr, *write_buf, dev_endian);
  err_code = RegisterWriteAndWrite(device_id, (uint64_t)write_addr, 8,
                                   (uint8_t *)write_buf, 8, dev_endian);
exit_to_release_mutex:
  ret = pthread_mutex_unlock(&s_mutex);
  if (ret) {
    LOG_E(0x30, "pthread_mutex_unlock() fail:%d", ret);
  }
  LOG_D("[OUT] err_code=%u", err_code);
  return err_code;
}

// -----------------------------------------------------------------------------
HalErrCode HalI2cGetDeviceInfo(struct HalI2cDeviceInfo *device_info[],
                               uint32_t *count) {
  HalErrCode err_code = kHalErrCodeOk;
  int ret = pthread_mutex_lock(&s_mutex);
  if (ret) {
    LOG_E(0x31, "pthread_mutex_lock() fail:%d, errno=%d", ret, errno);
    return kHalErrLock;
  }
  if (s_i2c_initialized == false) {
    LOG_E(0x32, "I2c is not initialized.");
    err_code = kHalErrInvalidState;
    goto exit_to_release_mutex;
  }
  if ((NULL == device_info) || (NULL == count)) {
    LOG_E(0x33, "NULL parameter(s).");
    err_code = kHalErrInvalidParam;
    goto exit_to_release_mutex;
  }
  if ((0 == s_all_device_num) || (0 == s_i2c_device_num)) {
    LOG_E(0x34, "No i2c device.(s_all_device_num=%u, s_i2c_device_num=%u)",
          s_all_device_num, s_i2c_device_num);
    err_code = kHalErrCodeError;
    goto exit_to_release_mutex;
  }
  if (s_i2c_device_info) {
    // data is ready.
    *count = s_i2c_device_num;
    *device_info = s_i2c_device_info;
  } else {
    LOG_E(0x35, "No s_i2c_device_info available.");
    err_code = kHalErrCodeError;
  }
exit_to_release_mutex:
  ret = pthread_mutex_unlock(&s_mutex);
  if (ret) {
    LOG_E(0x36, "pthread_mutex_unlock() fail:%d, errno=%d", ret, errno);
  }

  return err_code;
}

// -----------------------------------------------------------------------------
HalErrCode HalI2cReset(uint32_t device_id) {
  HalErrCode err_code = kHalErrCodeOk;
  int ret = pthread_mutex_lock(&s_mutex);
  if (ret) {
    LOG_E(0x37, "pthread_mutex_lock() fail:%d, errno=%d", ret, errno);
    return kHalErrLock;
  }

  if (s_i2c_initialized == false) {
    LOG_E(0x38, "I2c is not initialized.");
    err_code = kHalErrInvalidState;
    goto exit_to_release_mutex;
  }
  if (false == IsValidDeviceId(device_id)) {
    LOG_E(0x39, "Invalid device_id(%u)", device_id);
    err_code = kHalErrInvalidParam;
    goto exit_to_release_mutex;
  }

  struct I2cDeviceInfo *dev_info = NULL;
  // Get addr from device_info.
  err_code = GetDevInfoByDeviceId(device_id, &dev_info);
  if (kHalErrCodeOk != err_code) {
    LOG_W(0x3A, "GetDevInfoByDeviceId(dev_id:%u) fail:%u", device_id, err_code);
    goto exit_to_release_mutex;
  }

  if (NULL == s_handle[dev_info->port]) {
    goto exit_to_release_mutex;
  }
#ifdef CONFIG_I2C_RESET
  usleep(1000);
  int i2c_ret = I2C_RESET(s_handle[dev_info->port]);
  usleep(1000);
  if (0 > i2c_ret) {
    LOG_E(0x3B, "I2C_RESET() failed: %d", i2c_ret);
    err_code = kHalErrCodeError;
  } else {
    LOG_D("I2C_RESET(%p) : i2c_ret=%d", s_handle[dev_info->port], i2c_ret);
  }
#else
  LOG_W(0x3C, "Not support I2C Bus reset.");
  err_code = kHalErrCodeError;
#endif

exit_to_release_mutex:
  ret = pthread_mutex_unlock(&s_mutex);
  if (ret) {
    LOG_E(0x3D, "pthread_mutex_unlock() fail:%d, errno=%d", ret, errno);
  }

  return err_code;
}

// -----------------------------------------------------------------------------
HalErrCode HalI2cLock(void) {
  if (s_i2c_initialized == false) {
    LOG_E(0x3E, "I2c is not initialized.");
    return kHalErrInvalidState;
  }
  int ret = pthread_mutex_lock(&s_mutex);
  if (0 != ret) {
    LOG_E(0x3F, "Mutex lock error() fail:%d, errno=%d", ret, errno);
    return kHalErrLock;
  }
  return kHalErrCodeOk;
}

// -----------------------------------------------------------------------------
HalErrCode HalI2cUnlock(void) {
  if (s_i2c_initialized == false) {
    LOG_E(0x40, "I2c is not initialized.");
    return kHalErrInvalidState;
  }
  int ret = pthread_mutex_unlock(&s_mutex);
  if (0 != ret) {
    LOG_E(0x41, "Mutex unlock error fail:%d, errno=%d", ret, errno);
  }
  return kHalErrCodeOk;
}

// -----------------------------------------------------------------------------
// common parameters are vaild?
static bool IsValidParameter(uint32_t device_id, const void *buf_ptr) {
  if (IsValidDeviceId(device_id) && (buf_ptr != NULL)) {
    return true;
  }
  return false;
}

// -----------------------------------------------------------------------------
// Determine if a device ID is valid
static bool IsValidDeviceId(uint32_t device_id) {
  for (uint32_t idx = 0; idx < s_all_device_num; idx++) {
    if (device_id == s_all_device_info[idx].device_id) {
      return true;
    }
  }
  LOG_E(0x42, "Invalid device_id(%u) : s_all_device_num(%u)",
        device_id, s_all_device_num);
  return false;
}

// -----------------------------------------------------------------------------
// To read data, split msg into writing command register and reading data.
static HalErrCode RegisterWriteAndRead(uint32_t device_id,
                                       uint64_t read_addr,
                                       uint16_t addr_bytes,
                                       uint8_t *read_buff,
                                       uint16_t read_bytes,
                                       HalI2cEndian dev_endian) {
  CommandRegister cmd_reg = {0};
  cmd_reg.u64reg = read_addr;
  if (((BYTE_ORDER == BIG_ENDIAN) && (dev_endian != kHalI2cBigEndian)) ||
      ((BYTE_ORDER != BIG_ENDIAN) && (dev_endian == kHalI2cBigEndian))) {
    SwapDataByEndian(&cmd_reg.u8reg[0], addr_bytes);
  }
  uint8_t *transfer_data[2] = {NULL};
  transfer_data[0] = (uint8_t *)&cmd_reg;  // to write
  transfer_data[1] = read_buff;            // to read
  uint8_t  transfer_size[2] = {addr_bytes, (uint8_t)read_bytes};
  bool     is_read[2] = {false, true};     // Write and Read
  return HalI2cTransfer(device_id, (const uint8_t **)transfer_data,
                        transfer_size, is_read, 2, dev_endian);
}

// -----------------------------------------------------------------------------
// To write data, combine the command register and writing data into one msg.
static HalErrCode RegisterWriteAndWrite(uint32_t device_id,
                                        uint64_t write_addr,
                                        uint16_t addr_bytes,
                                        uint8_t *write_buff,
                                        uint16_t write_bytes,
                                        HalI2cEndian dev_endian) {
  LOG_D("write_addr=0x%"PRIx64", addr_bytes=%d, " \
        "write_buff=%p, write_bytes=%d",
        write_addr, addr_bytes, write_buff, write_bytes);
  uint8_t *reg_and_data = (uint8_t *)malloc(addr_bytes + write_bytes);
  if (reg_and_data == NULL) {
    return kHalErrMemory;
  }
  CommandRegister cmd_reg = {0};
  cmd_reg.u64reg = write_addr;
  memcpy(&reg_and_data[0], &cmd_reg.u8reg[0], addr_bytes);
  memcpy(&reg_and_data[addr_bytes], write_buff, write_bytes);
  if (((BYTE_ORDER == BIG_ENDIAN) && (dev_endian != kHalI2cBigEndian)) ||
      ((BYTE_ORDER != BIG_ENDIAN) && (dev_endian == kHalI2cBigEndian))) {
    SwapDataByEndian(&reg_and_data[0], addr_bytes);
    if ((write_bytes == 2) || (write_bytes == 4) || (write_bytes == 8)) {
      SwapDataByEndian(&reg_and_data[addr_bytes], write_bytes);
    }
  }
  uint8_t *transfer_data[1] = {NULL};
  transfer_data[0] = reg_and_data;
  uint8_t  transfer_size = addr_bytes + write_bytes;
  bool     is_read = false;
  HalErrCode err_code = HalI2cTransfer(device_id,
                                       (const uint8_t **)transfer_data,
                                       &transfer_size, &is_read, 1,
                                       dev_endian);
  if (reg_and_data) {
    free(reg_and_data);
  }
  return err_code;
}

// -----------------------------------------------------------------------------
// Get the corresponding port given the device ID.
static HalErrCode GetDevInfoByDeviceId(uint32_t device_id,
                                       struct I2cDeviceInfo *dev_info[]) {
  for (uint32_t idx = 0; idx < s_all_device_num; idx++) {
    if (s_all_device_info[idx].device_id == device_id) {
      *dev_info = &s_all_device_info[idx];
      return kHalErrCodeOk;
    }
  }
  return kHalErrInvalidParam;
}

// -----------------------------------------------------------------------------
static HalErrCode CollectDeviceInfoForI2c(void) {
  HalErrCode err_code = kHalErrCodeOk;
  s_all_device_num = CONFIG_HAL_I2C_DEVICE_NUM;
  if (0 == s_all_device_num) {
    LOG_E(0x43, "Illegal definition of CONFIG_HAL_I2C_DEVICE_NUM:%d",
          CONFIG_HAL_I2C_DEVICE_NUM);
    err_code = kHalErrMemory;
    goto exit_to_relase_memory;
  }
  memset(s_all_device_info, 0,
         sizeof(struct I2cDeviceInfo) * s_all_device_num);
  //
  s_i2c_device_num = 0;
  uint32_t i_devinfo = 0;
  if (s_all_device_num >= 1) {
#ifdef CONFIG_HAL_I2C_DEVICE0_DEV_ID
    RegisterDeviceInfo(i_devinfo++,
                       (const char *)CONFIG_HAL_I2C_DEVICE0_DEV_NAME,
                       CONFIG_HAL_I2C_DEVICE0_DEV_ID,
                       CONFIG_HAL_I2C_DEVICE0_PORT,
                       CONFIG_HAL_I2C_DEVICE0_FREQ,
                       CONFIG_HAL_I2C_DEVICE0_ADDR);
#else
    LOG_E(0x44, "Illegal definition of CONFIG_HAL_I2C_DEVICE0_DEV_ID");
#endif  // CONFIG_HAL_I2C_DEVICE0_DEV_ID
  }
  if (s_all_device_num >= 2) {
#ifdef CONFIG_HAL_I2C_DEVICE1_DEV_ID
    RegisterDeviceInfo(i_devinfo++,
                       (const char *)CONFIG_HAL_I2C_DEVICE1_DEV_NAME,
                       CONFIG_HAL_I2C_DEVICE1_DEV_ID,
                       CONFIG_HAL_I2C_DEVICE1_PORT,
                       CONFIG_HAL_I2C_DEVICE1_FREQ,
                       CONFIG_HAL_I2C_DEVICE1_ADDR);
#else
    LOG_E(0x45, "Illegal definition of CONFIG_HAL_I2C_DEVICE1_DEV_ID");
#endif  // CONFIG_HAL_I2C_DEVICE1_DEV_ID
  }
  if (s_all_device_num >= 3) {
#ifdef CONFIG_HAL_I2C_DEVICE2_DEV_ID
    RegisterDeviceInfo(i_devinfo++,
                       (const char *)CONFIG_HAL_I2C_DEVICE2_DEV_NAME,
                       CONFIG_HAL_I2C_DEVICE2_DEV_ID,
                       CONFIG_HAL_I2C_DEVICE2_PORT,
                       CONFIG_HAL_I2C_DEVICE2_FREQ,
                       CONFIG_HAL_I2C_DEVICE2_ADDR);
#else
    LOG_E(0x46, "Illegal definition of CONFIG_HAL_I2C_DEVICE2_DEV_ID");
#endif  // CONFIG_HAL_I2C_DEVICE2_DEV_ID
  }
  if (s_all_device_num >= 4) {
#ifdef CONFIG_HAL_I2C_DEVICE3_DEV_ID
    RegisterDeviceInfo(i_devinfo++,
                       (const char *)CONFIG_HAL_I2C_DEVICE3_DEV_NAME,
                       CONFIG_HAL_I2C_DEVICE3_DEV_ID,
                       CONFIG_HAL_I2C_DEVICE3_PORT,
                       CONFIG_HAL_I2C_DEVICE3_FREQ,
                       CONFIG_HAL_I2C_DEVICE3_ADDR);
#else
    LOG_E(0x47, "Illegal definition of CONFIG_HAL_I2C_DEVICE3_DEV_ID");
#endif  // CONFIG_HAL_I2C_DEVICE3_DEV_ID
  }
  for (i_devinfo = 0; i_devinfo < s_all_device_num; i_devinfo++) {
    // List s_all_device_info
    LOG_D("s_device_info[%u]", i_devinfo);
    LOG_D("  name      : %s",     s_all_device_info[i_devinfo].name);
    LOG_D("  device_id : %u",     s_all_device_info[i_devinfo].device_id);
    LOG_D("  port      : 0x%04X", s_all_device_info[i_devinfo].port);
    LOG_D("  frequency : %u",     s_all_device_info[i_devinfo].frequency);
    LOG_D("  addr: 0x%04X",       s_all_device_info[i_devinfo].addr);
  }
  s_i2c_device_num = s_all_device_num;
  // Initialize s_i2c_device_info[]
  s_i2c_device_info = (struct HalI2cDeviceInfo *)
                    malloc(sizeof(struct HalI2cDeviceInfo) * s_i2c_device_num);
  if (NULL == s_i2c_device_info) {
    LOG_E(0x48, "malloc() error");
    s_i2c_device_num = 0;
    err_code = kHalErrMemory;
    goto exit_to_relase_memory;
  }
  int i_devices = 0;
  for (uint32_t idx1 = 0; idx1 < s_i2c_device_num; idx1++) {
    size_t size = strlen(s_all_device_info[idx1].name);
    memcpy(s_i2c_device_info[i_devices].name,
           s_all_device_info[idx1].name, size + 1);
    s_i2c_device_info[i_devices].device_id =
        s_all_device_info[idx1].device_id;
    s_i2c_device_info[i_devices].port =
        s_all_device_info[idx1].port;
    s_i2c_device_info[i_devices].addr =
        s_all_device_info[idx1].addr;
    i_devices++;
  }

exit_to_relase_memory:
  LOG_D("[OUT] err_code=%u", err_code);
  return err_code;
}
static void RegisterDeviceInfo(uint32_t idx, const char *name,
                               uint32_t dev_id, uint32_t port,
                               uint32_t frequency, uint32_t addr) {
  snprintf(s_all_device_info[idx].name, sizeof(s_all_device_info[idx].name),
           "%s", name);
  s_all_device_info[idx].device_id  = dev_id;
  s_all_device_info[idx].port       = port;
  s_all_device_info[idx].frequency  = frequency;
  s_all_device_info[idx].addr = addr;
  s_used_port[s_all_device_info[idx].port] = true;
}
