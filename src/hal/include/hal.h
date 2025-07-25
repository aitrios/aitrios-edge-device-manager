/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#include <stdint.h>
#ifndef __HAL_H__
#define __HAL_H__

// Global Variables -----------------------------------------------------------
typedef enum {
  kHalErrCodeOk,
  kHalErrCodeError,

  kHalErrInvalidParam,
  kHalErrInvalidState,
  kHalErrInvalidOperation,
  kHalErrLock,
  kHalErrUnlock,
  kHalErrAlready,
  kHalErrNotFound,
  kHalErrNoSupported,
  kHalErrMemory,
  kHalErrInternal,
  kHalErrConfig,
  kHalErrInvalidValue,
  kHalErrHandler,
  kHalErrIrq,
  kHalErrCallback,
  kHalThreadError,
  kHalErrOpen,
  kHalErrInputDirection,
  kHalErrTimedout,
  kHalErrTransfer,
  kHalErrCodeMax,
} HalErrCode;  // T.B.D.

#endif /* __HAL_H__ */

