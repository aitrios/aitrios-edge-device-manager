/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef __HAL_DRIVER_IOEXP_H
#define __HAL_DRIVER_IOEXP_H

#include "hal.h"
#include "hal_driver.h"

// Macros ---------------------------------------------------------------------
typedef enum {
  kIoexpIoctlCmdRead,
  kIoexpIoctlCmdWrite,
  kIoexpIoctlCmdWriteMulti,
  kIoexpIoctlCmdGetDirection,
  kIoexpIoctlCmdSetDirection,
  kIoexpIoctlCmdRegIrqHandler,
  kIoexpIoctlCmdUnregIrqHandler,
  kIoexpIoctlCmdGetPinTotalNum,
  kIoexpIoctlCmdMax,
} IoexpCmd;

typedef enum {
  kIoexpDirectionInput = 0,
  kIoexpDirectionOutput,
  kIoexpDirectionMax
} IoexpDirection;

typedef enum {
  kIoexpValueHigh = 0,
  kIoexpValueLow,
  kIoexpValueMax
} IoexpValue;

typedef enum {
  kHalGpioIrqTypeInputRisingEdge = 0,
  kHalGpioIrqTypeInputFallingEdge,
  kHalGpioIrqTypeBothEdge,
  kHalGpioIrqTypeLowLevel,
  kHalGpioIrqTypeHighLevel,
  kHalGpioIrqTypeMax
} HalGpioIrqType;


typedef enum {
  kHalGpioValueLow = 0,
  kHalGpioValueHigh,
  kHalGpioValueMax
} HalGpioValue;

typedef void (*IoexpIrqHandler)(IoexpValue val, void *arg);

struct IoexpIrqInfo {
  HalGpioIrqType irq_type;
  uint32_t irq_num;
  IoexpIrqHandler irq_handler;
  void *private_data;
};

struct IoexpConfig {
  IoexpDirection direction;
};

struct IoexpIoctlArg {
  uint32_t device_id;
  uint32_t pin;
  uint32_t total_pin_num;
  IoexpValue value;
  struct IoexpIrqInfo irq_info;
  struct IoexpConfig config;
};

struct IoexpIoctlArgArray {
  uint32_t arg_num;
  struct IoexpIoctlArg *arg_array;
};

struct IoexpOpenArg {
  uint32_t device_id;
};
struct IoexpCloseArg {
  uint32_t device_id;
};

// Global Variables -----------------------------------------------------------

// Local functions ------------------------------------------------------------

// Functions ------------------------------------------------------------------
#endif
