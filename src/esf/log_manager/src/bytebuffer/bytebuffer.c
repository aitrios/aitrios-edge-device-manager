/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bytebuffer.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BYB_ERR_PRINTF(fmt, ...) printf(fmt, ##__VA_ARGS__);

#define ACTIVE_HANDLE_TABLE_NUM 30

/*
 * Constant value definition
 */

/*
 * Structure definition
 */

/** Buffer area manage handle */

typedef struct {
  uint32_t wp;       /** Write pointer */
  uint32_t rp;       /** Read pointer */
  uint32_t rem;      /** Buffer remain */
  uint8_t* body;     /** Buffer data area pointer */
  uint32_t bodysize; /** Buffer data area size */

  ByteBuffer_Type type; /** Buffer type */

  int32_t lastwp; /** Wp which storead last */

  pthread_mutex_t lock_mutex; /** Mutex for exclusive access of buffer */

  bool is_init; /** Is initialized ? */
} BufferMngHandle;

/*
 * Internal static variable
 */

static const uint8_t sc_header_size = sizeof(uint32_t);
static const uint32_t sc_dummy_header = 0xFFFFFFFF;

static ByteBuffer_Handle active_handle_list[ACTIVE_HANDLE_TABLE_NUM] = {NULL};

/*
 * Private functions
 */

/*----------------------------------------------------------------------*/
static inline bool check_active_handle(ByteBuffer_Handle handle) {
  bool enable = false;

  for (int i = 0; i < ACTIVE_HANDLE_TABLE_NUM; i++) {
    if (active_handle_list[i] == handle) {
      enable = true;
      break;
    }
  }

  return enable;
}

/*----------------------------------------------------------------------*/
static inline uint32_t rup4(uint32_t val) { return ((val + 3) & 0xFFFFFFFC); }

/*----------------------------------------------------------------------*/
static uint8_t* PushBackImpl(BufferMngHandle* hdl, uint8_t* data,
                             uint32_t datasize) {
  /** Null check. */

  if ((hdl == NULL) || (data == NULL)) {
    return NULL;
  }

  /** Lock mutex. */

  if (pthread_mutex_lock(&hdl->lock_mutex) != 0) {
    return NULL;
  }

  /** Set parameter. */

  uint8_t* ret = &hdl->body[hdl->wp];

  /** Push data. */

  if (datasize <= hdl->rem) {
    if ((hdl->wp + datasize) <= hdl->bodysize) {
      /* Write one plane. */

      memcpy(&hdl->body[hdl->wp], data, datasize);
      hdl->wp = (hdl->wp + datasize) % hdl->bodysize;
    } else {
      /* Write two plane. */

      uint32_t tailsz = hdl->bodysize - hdl->wp;

      memcpy(&hdl->body[hdl->wp], data, tailsz);

      uint32_t headsz = datasize - tailsz;

      memcpy(&hdl->body[0], data + tailsz, headsz);

      hdl->wp = headsz;
    }

    hdl->rem -= datasize;
  } else {
    ret = NULL;
  }

  /** Unlock mutex. */

  pthread_mutex_unlock(&hdl->lock_mutex);

  return ret;
}

/*----------------------------------------------------------------------*/
static uint8_t* PushBackNosplitImpl(BufferMngHandle* hdl, uint8_t* data,
                                    uint32_t offset, uint32_t datasize) {
  /** Null check. */

  if ((hdl == NULL) || (data == NULL)) {
    return NULL;
  }

  /** Set parameter. */

  uint8_t* ret = NULL;

  /** Lock mutex. */

  if (pthread_mutex_lock(&hdl->lock_mutex) != 0) {
    return NULL;
  }

  /** Calculate occupy size. */

  uint32_t occupysize = sc_header_size + rup4(datasize + offset);

  /** Push data. */

  if (occupysize <= hdl->rem) {
    if ((hdl->wp + occupysize) <= hdl->bodysize) {
      /** Write datasize */

      *(uint32_t*)&hdl->body[hdl->wp] = datasize + offset;

      /** Write to wp. */

      memcpy(&hdl->body[hdl->wp + sc_header_size + offset], data, datasize);

      ret = &hdl->body[hdl->wp + sc_header_size];

      hdl->lastwp = hdl->wp;
      hdl->wp = (hdl->wp + occupysize) % hdl->bodysize;
      hdl->rem -= occupysize;
    } else {
      /** Update remain. */

      uint32_t tail_deadsize = hdl->bodysize - hdl->wp;

      /** Check size and remain again. */

      if (occupysize <= (hdl->rem - tail_deadsize)) {
        /** Write dummy data size */

        *(uint32_t*)&hdl->body[hdl->wp] = sc_dummy_header;

        /** Write datasize */

        *(uint32_t*)&hdl->body[0] = datasize + offset;

        /** Write to top. */

        memcpy(&hdl->body[0 + sc_header_size + offset], data, datasize);

        ret = &hdl->body[0 + sc_header_size];

        hdl->lastwp = 0;
        hdl->wp = occupysize;
        hdl->rem -= (occupysize + tail_deadsize);
      }
    }
  } else {
    ret = NULL;
  }

  /** Unlock mutex. */

  pthread_mutex_unlock(&hdl->lock_mutex);

  return ret;
}

/*
 * Public functions
 */

/*----------------------------------------------------------------------*/
/* Initialize                                                           */
/*----------------------------------------------------------------------*/
ByteBuffer_ErrCode BYTEBUFFER_Init(ByteBuffer_Handle* handle,
                                   ByteBuffer_Type type, uint32_t* buf,
                                   uint32_t bufsize) {
  ByteBuffer_ErrCode ret = ByteBuffer_EcHandleCreateErr;

  /** Null check. */

  if ((handle == NULL) || (buf == NULL)) {
    return ByteBuffer_EcHandleCreateErr;
  }

  /** Parameter check. */

  if ((((uint32_t)(uintptr_t)buf & 0x00000003) != 0) || ((bufsize % 4) != 0)) {
    return ByteBuffer_EcHandleCreateErr;
  }

  /** Allocate buffer manage area. */

  BufferMngHandle* hdl = (BufferMngHandle*)malloc(sizeof(BufferMngHandle));

  if (hdl == NULL) {
    return ByteBuffer_EcHandleCreateErr;
  }

  memset(hdl, 0, sizeof(BufferMngHandle));

  hdl->wp = 0;
  hdl->rp = 0;
  hdl->rem = bufsize;
  hdl->body = (uint8_t*)buf;
  hdl->bodysize = bufsize;
  // hdl->bodysize = (type == ByteBuffer_SimpleRing) ? bufsize : (bufsize -
  // sc_header_size);

  hdl->type = type;

  if (pthread_mutex_init(&hdl->lock_mutex, NULL) != 0) {
    free(hdl);
    return ByteBuffer_EcHandleCreateErr;
  }

  hdl->is_init = true;

  *handle = (ByteBuffer_Handle*)hdl;

  /** Register to active handle list. */

  for (int i = 0; i < ACTIVE_HANDLE_TABLE_NUM; i++) {
    if (active_handle_list[i] == NULL) {
      active_handle_list[i] = *handle;
      ret = ByteBuffer_EcOk;
      break;
    }
  }

  if (ret != ByteBuffer_EcOk) {
    free(hdl);

    /* CodeSonar: 503523.58764361 */

    *handle = NULL;
  }

  return ret;
}

/*----------------------------------------------------------------------*/
/* Push byte data to back of bytebuffer.                                */
/*----------------------------------------------------------------------*/
uint8_t* BYTEBUFFER_PushBack(ByteBuffer_Handle handle, uint8_t* data,
                             uint32_t datasize) {
  uint8_t* ret = NULL;
  BufferMngHandle* hdl = (BufferMngHandle*)handle;

  if (hdl != NULL) {
    /** Active handle check */

    if (!check_active_handle(hdl)) {
      return NULL;
    }

    /** Push the data to buffer. */

    if (hdl->type == ByteBuffer_SimpleRing) {
      ret = PushBackImpl(hdl, data, datasize);
    } else if (hdl->type == ByteBuffer_NoSplitRing) {
      ret = PushBackNosplitImpl(hdl, data, 0, datasize);
    } else {
      ret = NULL;
    }
  }

  return ret;
}

/*----------------------------------------------------------------------*/
/* Clean bytebuffer.                                                    */
/*----------------------------------------------------------------------*/
ByteBuffer_ErrCode BYTEBUFFER_Clear(ByteBuffer_Handle handle) {
  ByteBuffer_ErrCode ret = ByteBuffer_EcOk;

  /** Null check. */

  if (handle == NULL) {
    return ByteBuffer_EcIllegalHandle;
  }

  /** Active handle check */

  if (!check_active_handle(handle)) {
    return ByteBuffer_EcIllegalHandle;
  }

  /** Clear buffer. */

  BufferMngHandle* hdl = (BufferMngHandle*)handle;

  /** Lock mutex. */

  if (pthread_mutex_lock(&hdl->lock_mutex) != 0) {
    return ByteBuffer_EcLockAccessErr;
  }

  /** Clear data. */

  hdl->rp = 0;
  hdl->wp = 0;
  hdl->lastwp = hdl->wp;
  hdl->rem = hdl->bodysize;

  /** Unlock mutex. */

  pthread_mutex_unlock(&hdl->lock_mutex);

  return ret;
}

/*----------------------------------------------------------------------*/
/* Finalize bytebuffer.                                                 */
/*----------------------------------------------------------------------*/
ByteBuffer_ErrCode BYTEBUFFER_Fin(ByteBuffer_Handle handle) {
  ByteBuffer_ErrCode ret = ByteBuffer_EcOk;

  /** Null check. */

  if (handle == NULL) {
    return ByteBuffer_EcIllegalHandle;
  }

  /** Active handle check */

  if (!check_active_handle(handle)) {
    return ByteBuffer_EcIllegalHandle;
  }

  /** Finalize bytebuffer. */

  BufferMngHandle* hdl = (BufferMngHandle*)handle;

  if (pthread_mutex_destroy(&hdl->lock_mutex) != 0) {
    BYB_ERR_PRINTF("pthread_mutex_destroy(lock_mutex) failed.\n");
  }

  /** Release handle. */

  free(handle);

  for (int i = 0; i < ACTIVE_HANDLE_TABLE_NUM; i++) {
    if (active_handle_list[i] == handle) {
      active_handle_list[i] = NULL;
      break;
    }
  }

  return ret;
}
