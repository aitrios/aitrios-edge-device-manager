/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _BYTEBUFFER_H_
#define _BYTEBUFFER_H_

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>

/** Error Code */

typedef enum {
  ByteBuffer_EcOk = 0,          /** Success */
  ByteBuffer_EcIllegalHandle,   /** Given handle is not acceptable */
  ByteBuffer_EcHandleCreateErr, /** Handle create failed */
  ByteBuffer_EcLockAccessErr,   /** Failed to access mutex */

  ByteBuffer_EcNum,
} ByteBuffer_ErrCode;

/** Buffer type */

typedef enum {
  ByteBuffer_SimpleRing =
      0, /** Simple byte order ring buffer, allow data split. */
  ByteBuffer_NoSplitRing,  // Don't allow data split, but buffer valid size will
                           // reduce.

  BufferTypeNum,
} ByteBuffer_Type;

/** Buffer handle */

typedef void* ByteBuffer_Handle;

/** Interface */

ByteBuffer_ErrCode BYTEBUFFER_Init(ByteBuffer_Handle* handle,
                                   ByteBuffer_Type type, uint32_t* buf,
                                   uint32_t bufsize);
uint8_t* BYTEBUFFER_PushBack(ByteBuffer_Handle handle, uint8_t* data,
                             uint32_t datasize);
ByteBuffer_ErrCode BYTEBUFFER_Clear(ByteBuffer_Handle handle);
ByteBuffer_ErrCode BYTEBUFFER_Fin(ByteBuffer_Handle handle);

#endif /* _BYTEBUFFER_H_ */
