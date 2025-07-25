/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dummy_utility_msg.h"

#include <inttypes.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>

static int32_t s_send_msg_id = 0;
static int32_t s_rcv_msg_id = 0;
static int32_t s_handle = 0;

typedef enum {
  kCmdIsNon,
  kCmdIsFinDlogCollector,
  kCmdIsQueueDlog,
  kCmdIsWrite,
  kCmdIsRamBufferPlaneFull,
  kCmdIsSaveToFlash
} DlogCollectorCmdsT;

struct MessagePassingObjT {
  DlogCollectorCmdsT m_cmd;
  size_t m_len_of_data;
  uint8_t *m_data;
};

UtilityMsgErrCode UtilityMsgOpen(int32_t *handle, uint32_t queue_size,
                                 uint32_t *max_msg_size) {
  *handle = 123;
  s_handle = *handle;

  return kUtilityMsgOk;
}

UtilityMsgErrCode UtilityMsgSend(int32_t handle, const void *msg,
                                 uint32_t msg_size, int32_t msg_prio,
                                 int32_t *sent_size) {
  int ret = 0;

  if (handle != s_handle) {
    printf("MsgSend Err!!(%d) %x\n", __LINE__, ret);
    return kUtilityMsgErrParam;
  }

  if (s_send_msg_id == 0) {
    key_t key;
    key = ftok("msgq.key", 'X');
    s_send_msg_id = msgget(1234L, 0666 | IPC_CREAT);
  }
  ret = msgsnd(s_send_msg_id, msg, 256, 0);
  if (ret < 0) {
    printf("MsgSend Err!!(%d) %x\n", __LINE__, ret);
    return kUtilityMsgErrInternal;
  }
  //  struct MessagePassingObjT *msg_obj = (struct MessagePassingObjT *)msg;
  //  printf("MsgSend %d::cmd=%d ret=%d\n",__LINE__,msg_obj->m_cmd, ret );

  return kUtilityMsgOk;
}

UtilityMsgErrCode UtilityMsgRecv(int32_t handle, void *buf, uint32_t size,
                                 int32_t timeout_ms, int32_t *recv_size) {
  int ret = 0;

  if (handle != s_handle) {
    return kUtilityMsgErrParam;
  }

  int32_t tmp = timeout_ms;
  if (s_rcv_msg_id == 0) {
    s_rcv_msg_id = msgget(1234L, 0);
  }
  ret = msgrcv(s_rcv_msg_id, buf, 256, 0, IPC_NOWAIT);
  //  struct MessagePassingObjT *msg_obj = (struct MessagePassingObjT *)buf;
  //  printf("MsgRecv %d::cmd=%d ret=%d\n",__LINE__,msg_obj->m_cmd, ret );
  if (ret < 0) {
    struct MessagePassingObjT *msg_obj = (struct MessagePassingObjT *)buf;
    if (msg_obj->m_cmd > 0) {
      printf("MsgRcv Err!!(%d) %x\n", __LINE__, ret);
      return kUtilityMsgErrInternal;
    }
  }

  return kUtilityMsgOk;
}

UtilityMsgErrCode UtilityMsgClose(int32_t handle) {
  if (handle != s_handle) {
    return kUtilityMsgErrParam;
  }
  handle = 0;
  s_send_msg_id = 0;
  s_rcv_msg_id = 0;

  return kUtilityMsgOk;
}
