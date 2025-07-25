/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef __HAL_IOEXP_H__
#define __HAL_IOEXP_H__

#include "hal.h"

// Macros ---------------------------------------------------------------------
// Global Variables -----------------------------------------------------------
typedef enum {
  kHalIoexpDirectionInput = 0,
  kHalIoexpDirectionOutput,
  kHalIoexpDirectionMax
} HalIoexpDirection;

struct HalIoexpConfig{
  HalIoexpDirection direction;
};

typedef enum {
  kHalIoexpIrqTypeFallingEdge,
  kHalIoexpIrqTypeRisingEdge,
  kHalIoexpIrqTypeBothEdge,
  kHalIoexpIrqTypeHighLevel,
  kHalIoexpIrqTypeLowLevel,
  kHalIoexpIrqTypeMax,
} HalIoexpIrqType;

typedef enum {
  kHalIoexpValueHigh,
  kHalIoexpValueLow,
  kHalIoexpValueMax,
} HalIoexpValue;

typedef void* HalIoexpHandle;
typedef void (*HalIoexpIrqHandler)(HalIoexpValue val, void *private_data);

// Local functions ------------------------------------------------------------
// Functions ------------------------------------------------------------------
HalErrCode HalIoexpInitialize(void);
HalErrCode HalIoexpFinalize(void);
HalErrCode HalIoexpOpen(uint32_t ioexp_id, HalIoexpHandle *handle);
HalErrCode HalIoexpClose(const HalIoexpHandle handle);
HalErrCode HalIoexpSetConfigure(const HalIoexpHandle handle,
                                   const struct HalIoexpConfig *config);
HalErrCode HalIoexpGetConfigure(const HalIoexpHandle handle,
                                   struct HalIoexpConfig *config);
HalErrCode HalIoexpWrite(const HalIoexpHandle handle,
                            HalIoexpValue value);
HalErrCode HalIoexpWriteMulti(const HalIoexpHandle *handle_array,
                              uint32_t handle_num,
                              const HalIoexpValue *value_array,
                              uint32_t value_num);
HalErrCode HalIoexpRead(const HalIoexpHandle handle,
                           HalIoexpValue *value);
HalErrCode HalIoexpRegisterIrqHandler(const HalIoexpHandle handle,
                                         HalIoexpIrqHandler handler,
                                         void *private_data,
                                         HalIoexpIrqType type);
HalErrCode HalIoexpUnregisterIrqHandler(const HalIoexpHandle handle);

#endif /* __HAL_IOEXP_H__ */
