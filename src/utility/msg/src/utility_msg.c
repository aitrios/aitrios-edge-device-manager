/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

// Includes --------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <semaphore.h>
#include <string.h>
#include <stdbool.h>
#include <sys/queue.h>

#include "utility_msg.h"
#include "utility_log.h"
#include "utility_log_module_id.h"

// Debug log -------------------------------------------------------------------
#define EVENT_ID  0xA100
#define EVENT_ID_START 0x00
#define LOG_E(event_id, format, ...) \
  WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:" \
                   format, __FILE__, __LINE__, ##__VA_ARGS__); \
  WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (EVENT_ID | (EVENT_ID_START + event_id)));

// Typedefs --------------------------------------------------------------------
#ifndef TAILQ_FOREACH_SAFE
#define TAILQ_FOREACH_SAFE(var, head, field, tvar) \
  for ((var) = TAILQ_FIRST((head)); \
      (var) && ((tvar) = TAILQ_NEXT((var), field), 1); \
      (var) = (tvar))
#endif
TAILQ_HEAD(MqMsgList, MqMsg);

struct MqMsg {
  TAILQ_ENTRY(MqMsg) head;
  uint8_t *msg;
  uint32_t msg_size;
  uint32_t prio;
};

// Semaphore sem_recv and sem_send are prepared for each handle.
// The count of sem_recv is the number of messages in the queue.
// The sem_send count is the number of unused messages in the queue.
// The following explains how the sem_recv and sem_send counts work.
// ex. queue_size = 5
// |pid| Execution     | sem_recv | sem_send |
// |   | Function      | count    | count    |
// |---|---------------|----------|----------|
// | 0 |UtilityMsgOpen |    0     |    5     |
// | 1 |UtilityMsgRecv |   Wait   |          | * Wait until UtilityMsgSend
//                                               is executed or times out.
// | 2 |UtilityMsgSend |    1     |    4     | * If UtilityMsgSend is executed
//                                               before UtilityMsgRecv(*1)
//                                               times out.
// | 1 |UtilityMsgRecv |    0     |    5     |
// | 3 |UtilityMsgSend |    1     |    4     |
// | 4 | - Execute UtilityMsgSend 3 times. - |
// | 5 |UtilityMsgSend |    5     |    0     |
// | 6 |UtilityMsgSend |          |   Wait   | * Wait until UtilityMsgRecv is
//                                               executed.
// | 7 |UtilityMsgRecv |    4     |    1     |
// | 6 |UtilityMsgSend |    5     |    0     |

TAILQ_HEAD(MqInfoList, MqInfo);

struct MqInfo {
  TAILQ_ENTRY(MqInfo) head;
  struct MqMsgList msgs;  // Message queue
  uint32_t max_msg_size;  // max message size
  sem_t sem_recv;         // count is cuurent msg num.
  sem_t sem_send;         // count is unused msg num.
  bool is_terminate;      // terminate is true;
  int32_t handle;         // Utility message queue handle
  int32_t sem_recv_wait;  // wait count
  int32_t sem_send_wait;  // wait count
};

// External functions ----------------------------------------------------------

// Local functions -------------------------------------------------------------
static UtilityMsgErrCode MsgClose(struct MqInfo *info);
static struct MqInfo *MsgSearchList(int32_t handle);
static UtilityMsgErrCode MsgSetMsg(struct MqInfo *info, const void *msg,
                                   uint32_t msg_size, int32_t msg_prio);
static UtilityMsgErrCode MsgCalcTimeout(int32_t timeout_ms,
                                        struct timespec *timeout);
static UtilityMsgErrCode MsgWaitRecvMsg(struct MqInfo *info,
                                        int32_t timeout_ms);
static UtilityMsgErrCode MsgGetMsg(struct MqInfo *info, void *buf,
                                   uint32_t size, int32_t timeout_ms,
                                   int32_t *recv_size);

// Global Variables ------------------------------------------------------------
static pthread_mutex_t s_api_mutex;
static pthread_mutex_t s_list_mutex;

static struct MqInfoList s_mq_info_list = {0};
static bool s_is_initialized = false;
static int32_t s_handle_cnt = 0;

// Functions -------------------------------------------------------------------
UtilityMsgErrCode UtilityMsgOpen(int32_t *handle,
                                 uint32_t queue_size,
                                 uint32_t max_msg_size) {
  if (!s_is_initialized) {
    LOG_E(0x01, "State error.");
    pthread_mutex_unlock(&s_api_mutex);
    return kUtilityMsgErrState;
  }

  int lock_ret = pthread_mutex_lock(&s_api_mutex);
  if (lock_ret != 0) {
    LOG_E(0x00, "Lock error. errno=%d", errno);
    return kUtilityMsgErrLock;
  }

  if ((handle == NULL) || (queue_size == 0) || (max_msg_size == 0)) {
    LOG_E(0x02, "Parameter error.");
    pthread_mutex_unlock(&s_api_mutex);
    return kUtilityMsgErrParam;
  }

  // check used handle
  // For example, if s_handle_cnt=1 is in use and s_handle_cnt is counted up
  // again to s_handle_cnt=1, this is a check process that prevents processing
  // from being executed with s_handle_cnt=1.
  struct MqInfo *found = MsgSearchList(s_handle_cnt);
  if (found != NULL) {
    LOG_E(0x03, "handle is used.(handle = %d)", s_handle_cnt);
    // s_handle_cnt is currently in use,
    // so increment s_handle_cnt for the next UtilityMsgOpen.
    s_handle_cnt++;
    pthread_mutex_unlock(&s_api_mutex);
    return kUtilityMsgErrRetry;
  }

  struct MqInfo *item = (struct MqInfo *)malloc(sizeof(struct MqInfo));
  if (item == NULL) {
    LOG_E(0x04, "memory alloc error.");
    pthread_mutex_unlock(&s_api_mutex);
    return kUtilityMsgErrMemory;
  }

  struct MqMsgList *msgs = &(item->msgs);
  TAILQ_INIT(msgs);

  int ret = sem_init(&(item->sem_recv), 0, 0);
  if (ret != 0) {
    LOG_E(0x05, "sem_init failed. errno=%d", errno);
    free(item);
    pthread_mutex_unlock(&s_api_mutex);
    return kUtilityMsgErrInternal;
  }

  ret = sem_init(&(item->sem_send), 0, queue_size);
  if (ret != 0) {
    LOG_E(0x06, "sem_init failed. errno=%d", errno);
    free(item);
    pthread_mutex_unlock(&s_api_mutex);
    return kUtilityMsgErrInternal;
  }
  item->max_msg_size = max_msg_size;
  item->handle = s_handle_cnt++;
  item->is_terminate = false;
  item->sem_recv_wait = 0;
  item->sem_send_wait = 0;

  TAILQ_INSERT_TAIL(&s_mq_info_list, item, head);

  *handle = item->handle;

  lock_ret = pthread_mutex_unlock(&s_api_mutex);
  if (lock_ret != 0) {
    LOG_E(0x07, "Unlock error. errno=%d", errno);
    TAILQ_REMOVE(&s_mq_info_list, item, head);
    free(item);
    return kUtilityMsgErrUnlock;
  }

  return kUtilityMsgOk;
}

//------------------------------------------------------------------------------
//    UtilityMsgSend
//------------------------------------------------------------------------------
UtilityMsgErrCode UtilityMsgSend(int32_t handle, const void *msg,
                                 uint32_t msg_size, int32_t msg_prio,
                                 int32_t *sent_size) {
  if (!s_is_initialized) {
    LOG_E(0x08, "State error.");
    return kUtilityMsgErrState;
  }

  if ((msg == NULL) || (sent_size == NULL)) {
    LOG_E(0x09, "Parameter error.");
    return kUtilityMsgErrParam;
  }

  struct MqInfo *found = MsgSearchList(handle);
  if (found == NULL) {
    LOG_E(0x0A, "handle not found. handle=%d", handle);
    return kUtilityMsgErrNotFound;
  }

  if (msg_size > found->max_msg_size) {
    LOG_E(0x0B, "Parameter error. (msg_size = %u > max_size = %u)",
        msg_size, found->max_msg_size);
    return kUtilityMsgErrParam;
  }

  UtilityMsgErrCode ret_code = MsgSetMsg(found, msg, msg_size, msg_prio);
  if (ret_code != kUtilityMsgOk) {
    LOG_E(0x0C, "SetMsg error(%u). handle=%d", ret_code, handle);
    return ret_code;
  }

  *sent_size = msg_size;

  return kUtilityMsgOk;
}

//------------------------------------------------------------------------------
//    UtilityMsgRecv
//------------------------------------------------------------------------------
UtilityMsgErrCode UtilityMsgRecv(int32_t handle, void *buf, uint32_t size,
                        int32_t timeout_ms, int32_t *recv_size) {
  if (!s_is_initialized) {
    LOG_E(0x0D, "State error.");
    return kUtilityMsgErrState;
  }

  if ((buf == NULL) || (timeout_ms < -1) || (recv_size == NULL)) {
    LOG_E(0x0E, "Parameter error.");
    return kUtilityMsgErrParam;
  }

  struct MqInfo *found = MsgSearchList(handle);
  if (found == NULL) {
    LOG_E(0x0F, "handle not found. handle=%d", handle);
    return kUtilityMsgErrNotFound;
  }

  if (size < found->max_msg_size) {
    LOG_E(0x10, "Parameter error. (size = %u < max_size = %u)",
        size, found->max_msg_size);
    return kUtilityMsgErrParam;
  }

  UtilityMsgErrCode ret_code = MsgGetMsg(
                                 found, buf, size, timeout_ms, recv_size);
  if (ret_code != kUtilityMsgOk) {
    if (ret_code != kUtilityMsgErrTimedout) {
      LOG_E(0x11, "GetMsg error. handle=%d", handle);
    }
    return ret_code;
  }

  return kUtilityMsgOk;
}

//------------------------------------------------------------------------------
//    MsgClose
//------------------------------------------------------------------------------
static UtilityMsgErrCode MsgClose(struct MqInfo *info) {
  // Argument info is guaranteed to be non-NULL.
  int lock_ret = pthread_mutex_lock(&s_list_mutex);
  if (lock_ret != 0) {
    LOG_E(0x12, "Lock error. errno=%d", errno);
    return kUtilityMsgErrLock;
  }

  UtilityMsgErrCode err_code = kUtilityMsgOk;
  struct MqMsg *entry = NULL, *temp = NULL;
  struct MqMsgList *msgs = &(info->msgs);
  TAILQ_FOREACH_SAFE(entry, msgs, head, temp) {
    TAILQ_REMOVE(msgs, entry, head);
    if (entry->msg) {
      free(entry->msg);
    }
    free(entry);
    entry = NULL;
  }

  info->is_terminate = true;
  int sem_recv_wait = info->sem_recv_wait;
  for (int i = 0; i < sem_recv_wait; i++) {
    sem_post(&(info->sem_recv));
  }
  int ret = sem_destroy(&(info->sem_recv));
  if (ret != 0) {
    LOG_E(0x13, "sem_destroy failed. errno=%d", errno);
    err_code = kUtilityMsgErrInternal;
  }
  int sem_send_wait = info->sem_send_wait;
  for (int i = 0; i < sem_send_wait; i++) {
    sem_post(&(info->sem_send));
  }
  ret = sem_destroy(&(info->sem_send));
  if (ret != 0) {
    LOG_E(0x14, "sem_destroy failed. errno=%d", errno);
    err_code = kUtilityMsgErrInternal;
  }

  TAILQ_REMOVE(&s_mq_info_list, info, head);
  free(info);
  info = NULL;

  lock_ret = pthread_mutex_unlock(&s_list_mutex);
  if (lock_ret != 0) {
    LOG_E(0x15, "Unlock error. errno=%d", errno);
    return kUtilityMsgErrUnlock;
  }
  return err_code;
}

//------------------------------------------------------------------------------
//    UtilityMsgClose
//------------------------------------------------------------------------------
UtilityMsgErrCode UtilityMsgClose(int32_t handle) {
  if (!s_is_initialized) {
    LOG_E(0x17, "State error.");
    pthread_mutex_unlock(&s_api_mutex);
    return kUtilityMsgErrState;
  }

  int lock_ret = pthread_mutex_lock(&s_api_mutex);
  if (lock_ret != 0) {
    LOG_E(0x16, "Lock error. errno=%d", errno);
    return kUtilityMsgErrLock;
  }

  struct MqInfo *found = MsgSearchList(handle);
  if (found == NULL) {
    LOG_E(0x18, "handle not found. handle=%d",
        handle);
    pthread_mutex_unlock(&s_api_mutex);
    return kUtilityMsgErrNotFound;
  }

  UtilityMsgErrCode ret_code = MsgClose(found);
  if (ret_code != kUtilityMsgOk) {
    LOG_E(0x19, "[ERROR] %s %d:Close error. err=%u handle=%d\n",
        __func__, __LINE__, ret_code, handle);
    pthread_mutex_unlock(&s_api_mutex);
    return ret_code;
  }

  lock_ret = pthread_mutex_unlock(&s_api_mutex);
  if (lock_ret != 0) {
    LOG_E(0x1A, "Unlock error. errno=%d", errno);
    return kUtilityMsgErrUnlock;
  }

  return kUtilityMsgOk;
}

//------------------------------------------------------------------------------
//    UtilityMsgInitialize
//------------------------------------------------------------------------------
UtilityMsgErrCode UtilityMsgInitialize(void) {
  int ret = kUtilityMsgOk;

  if (s_is_initialized) {
    LOG_E(0x1C, "State error.");
    return kUtilityMsgErrState;
  }

  int init_ret;
  init_ret = pthread_mutex_init(&s_api_mutex, NULL);
  if (init_ret != 0) {
    LOG_E(0x31, "Mutex init error. ret=%d errno=%d", init_ret, errno);
    return kUtilityMsgErrInternal;
  }

  init_ret = pthread_mutex_init(&s_list_mutex, NULL);
  if (init_ret != 0) {
    LOG_E(0x32, "Mutex init error. ret=%d errno=%d", init_ret, errno);
    ret = kUtilityMsgErrInternal;
    goto api_mutex_destroy;
  }

  int lock_ret = pthread_mutex_lock(&s_api_mutex);
  if (lock_ret != 0) {
    LOG_E(0x1B, "Lock error. errno=%d", errno);
    ret = kUtilityMsgErrLock;
    goto list_mutex_destroy;
  }

  TAILQ_INIT(&s_mq_info_list);

  lock_ret = pthread_mutex_unlock(&s_api_mutex);
  if (lock_ret != 0) {
    LOG_E(0x1D, "Unlock error. errno=%d", errno);
    ret = kUtilityMsgErrUnlock;
    goto list_mutex_destroy;
  }

  s_is_initialized = true;

  return ret;

list_mutex_destroy:
  pthread_mutex_destroy(&s_list_mutex);
api_mutex_destroy:
  pthread_mutex_destroy(&s_api_mutex);

  return ret;
}

//------------------------------------------------------------------------------
//    UtilityMsgFinalize
//------------------------------------------------------------------------------
UtilityMsgErrCode UtilityMsgFinalize(void) {
  if (!s_is_initialized) {
    LOG_E(0x1F, "State error.");
    return kUtilityMsgErrState;
  }

  int lock_ret = pthread_mutex_lock(&s_api_mutex);
  if (lock_ret != 0) {
    LOG_E(0x1E, "Lock error. errno=%d", errno);
    return kUtilityMsgErrLock;
  }

  UtilityMsgErrCode err_code = kUtilityMsgOk;
  struct MqInfo *entry = NULL, *temp = NULL;
  TAILQ_FOREACH_SAFE(entry, &s_mq_info_list, head, temp) {
    int32_t handle = entry->handle;
    UtilityMsgErrCode ret_code = MsgClose(entry);
    if (ret_code != kUtilityMsgOk) {
      LOG_E(0x20, "[ERROR] %s %d:Close error. err=%u handle=%d\n",
        __func__, __LINE__, ret_code, handle);
      err_code = ret_code;
    }
  }

  lock_ret = pthread_mutex_unlock(&s_api_mutex);
  if (lock_ret != 0) {
    LOG_E(0x21, "Unlock error. errno=%d", errno);
    return kUtilityMsgErrUnlock;
  }

  pthread_mutex_destroy(&s_list_mutex);
  pthread_mutex_destroy(&s_api_mutex);

  if (err_code == kUtilityMsgOk) {
    s_is_initialized = false;
  }

  return err_code;
}

//------------------------------------------------------------------------------
//    MsgSearchList
//------------------------------------------------------------------------------
static struct MqInfo *MsgSearchList(int32_t handle) {
  int lock_ret = pthread_mutex_lock(&s_list_mutex);
  if (lock_ret != 0) {
    LOG_E(0x22, "Lock error. errno=%d", errno);
    return NULL;
  }

  struct MqInfo *found = NULL;
  struct MqInfo *entry = NULL, *temp = NULL;
  TAILQ_FOREACH_SAFE(entry, &s_mq_info_list, head, temp) {
    if (entry->handle == handle) {
      found = entry;
      break;
    }
  }

  lock_ret = pthread_mutex_unlock(&s_list_mutex);
  if (lock_ret != 0) {
    LOG_E(0x23, "Unlock error. errno=%d", errno);
  }

  return found;
}

//------------------------------------------------------------------------------
//    MsgSetMsg
//------------------------------------------------------------------------------
static UtilityMsgErrCode MsgSetMsg(struct MqInfo *info, const void *msg,
                                   uint32_t msg_size, int32_t msg_prio) {
  // Argument info and msg are guaranteed to be non-NULL.

  // Wait if Message queue is full.
  // count down unused msg num
  int ret_os = 0;
  do {
    info->sem_send_wait++;
    ret_os = sem_wait(&(info->sem_send));
    info->sem_send_wait--;
    if ((ret_os == 0) && (info->is_terminate)) {
      // if closed.
      return kUtilityMsgErrTerminate;
    }
    // Restart if interrupted by handler
  } while ((ret_os == -1) && (errno == EINTR));
  if (ret_os != 0) {
    LOG_E(0x24, "sem_wait failed. errno=%d", errno);
    return kUtilityMsgErrInternal;
  }

  struct MqMsg *msgs = (struct MqMsg *)malloc(sizeof(struct MqMsg));
  if (msgs == NULL) {
    LOG_E(0x25, "memory alloc error.");
    return kUtilityMsgErrMemory;
  }
  if (msg_size > 0) {
    msgs->msg = (uint8_t *)malloc(msg_size);
    if (msgs->msg == NULL) {
      LOG_E(0x26, "memory alloc error. allocsize=%u", msg_size);
      free(msgs);
      return kUtilityMsgErrMemory;
    }
    memcpy(msgs->msg, msg, msg_size);
  } else {
    msgs->msg = NULL;
  }
  msgs->msg_size = msg_size;
  msgs->prio = msg_prio;

  UtilityMsgErrCode ret_code = kUtilityMsgOk;
  ret_os = pthread_mutex_lock(&s_list_mutex);
  if (ret_os != 0) {
    LOG_E(0x27, "Lock error. errno=%d", errno);
    ret_code = kUtilityMsgErrLock;
    goto release_memory;
  }

  TAILQ_INSERT_TAIL(&(info->msgs), msgs, head);

  // count up current msg num
  ret_os = sem_post(&(info->sem_recv));
  if (ret_os != 0) {
    LOG_E(0x28, "sem_post failed. errno=%d", errno);
    ret_code = kUtilityMsgErrInternal;
  }

  ret_os = pthread_mutex_unlock(&s_list_mutex);
  if (ret_os != 0) {
    LOG_E(0x29, "Unlock error. errno=%d", errno);
    ret_code = kUtilityMsgErrUnlock;
    goto remove_list;
  }
  if (ret_code == kUtilityMsgOk) {
    goto exit_func;
  }

remove_list:
  TAILQ_REMOVE(&(info->msgs), msgs, head);
release_memory:
  free(msgs->msg);
  free(msgs);
exit_func:
  return ret_code;
}

//------------------------------------------------------------------------------
//    MsgCalcTimeout
//------------------------------------------------------------------------------
static UtilityMsgErrCode MsgCalcTimeout(int32_t timeout_ms,
                                      struct timespec *timeout) {
  // Argument timeout is guaranteed to be non-NULL.
  int ret = clock_gettime(CLOCK_REALTIME, timeout);
  if (ret != 0) {
    LOG_E(0x2A, "Time get failed. errno=%d", errno);
    return kUtilityMsgErrInternal;
  }

  timeout->tv_sec += (timeout_ms / 1000);
  int32_t ns = timeout->tv_nsec
          + ((timeout_ms % 1000) * 1000000);
  timeout->tv_sec += (ns / 1000000000);
  timeout->tv_nsec = (ns % 1000000000);

  return kUtilityMsgOk;
}

//------------------------------------------------------------------------------
//    MsgWaitRecvMsg
//------------------------------------------------------------------------------
static UtilityMsgErrCode MsgWaitRecvMsg(struct MqInfo *info,
                                        int32_t timeout_ms) {
  // Wait if Message queue is none.
  // Message qeueu is none == current msg num is 0
  int ret = 0;
  if (timeout_ms < 0) {
    // count down current msg num
    do {
      info->sem_recv_wait++;
      ret = sem_wait(&(info->sem_recv));
      info->sem_recv_wait--;
      if ((ret == 0) && (info->is_terminate)) {
        // if closed.
        return kUtilityMsgErrTerminate;
      }
      // Restart if interrupted by handler
    } while ((ret == -1) && (errno == EINTR));
  } else {
    struct timespec timeout = {0};
    UtilityMsgErrCode ret_code = MsgCalcTimeout(timeout_ms, &timeout);
    if (ret_code != kUtilityMsgOk) {
      LOG_E(0x2B, "Calc time error. err=%d", ret_code);
      return ret_code;
    }
    // count down current msg num
    do {
      info->sem_recv_wait++;
      ret = sem_timedwait(&(info->sem_recv), &timeout);
      info->sem_recv_wait--;
      if ((ret == 0) && (info->is_terminate)) {
        // if closed.
        return kUtilityMsgErrTerminate;
      }
      // Restart if interrupted by handler
    } while ((ret == -1) && (errno == EINTR));
  }
  if (ret < 0) {
    int err = errno;
    if (err == ETIMEDOUT) {
      return kUtilityMsgErrTimedout;
    }
    LOG_E(0x2C, "%s failed. errno=%d handle=%d",
        (timeout_ms > -1) ? "sem_timedwait" : "sem_wait", err, info->handle);
    return kUtilityMsgErrInternal;
  }

  return kUtilityMsgOk;
}

//------------------------------------------------------------------------------
//    MsgGetMsg
//------------------------------------------------------------------------------
static UtilityMsgErrCode MsgGetMsg(struct MqInfo *info, void *buf,
                                   uint32_t size, int32_t timeout_ms,
                                   int32_t *recv_size) {
  // info, buf and recv_size were verified in caller function.
  // No need to check null parameters.
  UtilityMsgErrCode err_code = MsgWaitRecvMsg(info, timeout_ms);
  if (err_code != kUtilityMsgOk) {
    if (err_code != kUtilityMsgErrTimedout) {
      LOG_E(0x2D, "MsgWaitRecvMsg error(%d).", err_code);
    }
    return err_code;
  }

  int lock_ret = pthread_mutex_lock(&s_list_mutex);
  if (lock_ret != 0) {
    LOG_E(0x2E, "Lock error. errno=%d", errno);
    return kUtilityMsgErrLock;
  }

  struct MqMsg *found = NULL;
  struct MqMsg *entry = NULL, *temp = NULL;
  struct MqMsgList *msgs = &(info->msgs);
  TAILQ_FOREACH_SAFE(entry, msgs, head, temp) {
    if (found == NULL) {
      found = entry;
    } else if (found->prio < entry->prio) {
      found = entry;
    }
  }

  if (found == NULL) {
    LOG_E(0x2F, "message not found.");
    sem_post(&(info->sem_recv));
    pthread_mutex_unlock(&s_list_mutex);
    return kUtilityMsgErrNotFound;
  }
  // Argument 'buf' and size' are guaranteed by caller function
  // to be size >= found->max_msg_size and
  // found->msg_size <= found->max_msg_size.
  if (found->msg_size > 0) {
    memcpy(buf, found->msg, found->msg_size);
  }

  *recv_size = (int32_t)(found->msg_size);

  free(found->msg);
  found->msg = NULL;
  TAILQ_REMOVE(&(info->msgs), found, head);
  free(found);
  found = NULL;

  // count up unused msg num
  int ret = sem_post(&(info->sem_send));
  if (ret != 0) {
    LOG_E(0x30, "sem_post failed. errno=%d", errno);
    pthread_mutex_unlock(&s_list_mutex);
    return kUtilityMsgErrInternal;
  }

  lock_ret = pthread_mutex_unlock(&s_list_mutex);
  if (lock_ret != 0) {
    LOG_E(0x31, "Unlock error. errno=%d", errno);
  }

  return err_code;
}
