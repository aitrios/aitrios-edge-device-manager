/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef __UTILITY_MSG_H
#define __UTILITY_MSG_H

/*******************************************************************************
 * Included Files
 ******************************************************************************/
#include <stdint.h>

/*******************************************************************************
 * Pre-preprocessor Definitions
 ******************************************************************************/

/*******************************************************************************
 * Public Types
 ******************************************************************************/
typedef enum {
  kUtilityMsgOk = 0,
  kUtilityMsgError,
  kUtilityMsgErrState,
  kUtilityMsgErrParam,
  kUtilityMsgErrLock,
  kUtilityMsgErrUnlock,
  kUtilityMsgErrTimedout,
  kUtilityMsgErrRetry,
  kUtilityMsgErrMemory,
  kUtilityMsgErrNotFound,
  kUtilityMsgErrInternal,
  kUtilityMsgErrTerminate,
} UtilityMsgErrCode;
/*******************************************************************************
 * Public Data
 ******************************************************************************/

/*******************************************************************************
 * Inline Functions
 ******************************************************************************/

/*******************************************************************************
 * Public Function Prototypes
 ******************************************************************************/
UtilityMsgErrCode UtilityMsgInitialize(void);
UtilityMsgErrCode UtilityMsgFinalize(void);
UtilityMsgErrCode UtilityMsgOpen(int32_t *handle, uint32_t queue_size,
                                 uint32_t max_msg_size);
UtilityMsgErrCode UtilityMsgSend(int32_t handle, const void *msg,
                                 uint32_t msg_size, int32_t msg_prio,
                                 int32_t *sent_size);
UtilityMsgErrCode UtilityMsgRecv(int32_t handle, void *buf, uint32_t size,
                                 int32_t timeout_ms, int32_t *recv_size);
UtilityMsgErrCode UtilityMsgClose(int32_t handle);

#endif  // __UTILITY_MSG_H
