/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#ifndef __PL_H__
#define __PL_H__

// Global Variables -----------------------------------------------------------
typedef enum {
  kPlErrCodeOk,
  kPlErrCodeError,

  kPlErrInvalidParam,
  kPlErrInvalidState,
  kPlErrInvalidOperation,
  kPlErrLock,
  kPlErrUnlock,
  kPlErrAlready,
  kPlErrNotFound,
  kPlErrNoSupported,
  kPlErrMemory,
  kPlErrInternal,
  kPlErrConfig,
  kPlErrInvalidValue,
  kPlErrHandler,
  kPlErrIrq,
  kPlErrCallback,
  kPlThreadError,
  kPlErrOpen,
  kPlErrClose,
  kPlErrDevice,
  kPlErrMagicCode,
  kPlErrBufferOverflow,
  kPlErrWrite,
  kPlErrCodeMax,
} PlErrCode;

#endif /* __PL_H__ */

