/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <bsd/sys/queue.h>

#include "pl_storage.h"
#include "pl_storage_id_table.h"
#include "pl.h"

#include "utility_log_module_id.h"
#include "utility_log.h"

// Macros ---------------------------------------------------------------------
#define EVENT_ID  0x9700
#define EVENT_ID_START 0x0000
#define LOG_ERR(event_id, format, ...) \
  WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, \
    "%s-%d:" format, __FILE__, __LINE__, ##__VA_ARGS__); \
  WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (EVENT_ID | \
                  (0x00FF & (EVENT_ID_START + event_id))));
// Variables ------------------------------------------------------------------

#define DEFAULT_SQLITE3_DATABASE_PATH "/var/lib/edge-device-core/db.sqlite3"

TAILQ_HEAD(PlStorageSqlite3HandleList, PlStorageSqlite3Handle);

struct PlStorageSqlite3Handle {
  sqlite3_blob *blob;
  sqlite3_int64 id;
  unsigned int offset;
  int oflags;
  TAILQ_ENTRY(PlStorageSqlite3Handle) list;
};

static pthread_mutex_t s_mutex = PTHREAD_MUTEX_INITIALIZER;
static struct PlStorageSqlite3HandleList s_handle_list;
static bool s_initialized = false;
static sqlite3 *s_db = NULL;

// ----------------------------------------------------------------------------
static struct PlStorageSqlite3Handle *toSqlite3Handle(PlStorageHandle handle) {
  struct PlStorageSqlite3Handle *hdl, *tmp;

  TAILQ_FOREACH_SAFE(hdl, &s_handle_list, list, tmp) {
    if (hdl == handle) {
      return hdl;
    }
  }
  return NULL;
}

//-----------------------------------------------------------------------------
static PlErrCode PlStorageSqliteIdHasRow(PlStorageDataId id, bool *hasRow) {
  const char *sql = "select * from psm_data where rowid = ?";
  sqlite3_stmt *stmt;
  int ret = UINT32_MAX;

  if (id >= PlStorageDataMax) {
    LOG_ERR(0x00, "%s(%d): invalid id\n", __func__, id);
    return kPlErrInvalidParam;
  }

  ret = sqlite3_prepare_v3(s_db, sql, -1, 0, &stmt, NULL);
  if (ret != SQLITE_OK) {
    LOG_ERR(0x01, "%s(%d): Failed to prepare statement: %s\n",
      __func__, id, sqlite3_errmsg(s_db));
    return kPlErrInternal;
  }

  ret = sqlite3_bind_int(stmt, 1, id);
  if (ret != SQLITE_OK) {
    LOG_ERR(0x02, "%s(%d): Failed to bind id parameter: %s\n",
      __func__, id, sqlite3_errmsg(s_db));
    ret = kPlErrInternal;
    goto out_finalize_statement;
  }

  ret = sqlite3_step(stmt);

  if (ret == SQLITE_ROW) {
    *hasRow = true;
    ret = kPlErrCodeOk;
    goto out_finalize_statement;
  }

  if (ret == SQLITE_DONE) {
    *hasRow = false;
    ret = kPlErrCodeOk;
    goto out_finalize_statement;
  }

  LOG_ERR(0x03, "%s(%d): Failed to execute statement: %s\n",
    __func__, id, sqlite3_errmsg(s_db));
  ret = kPlErrInternal;

out_finalize_statement:
  sqlite3_finalize(stmt);

  return ret;
}

// ----------------------------------------------------------------------------
static PlErrCode PlStorageSqliteResizeBlob(PlStorageDataId id, size_t size) {
  const char *sql = "insert or replace into psm_data select ?, ?";
  sqlite3_stmt *stmt;
  int ret;

  ret = sqlite3_prepare_v3(s_db, sql, -1, 0, &stmt, NULL);
  if (ret != SQLITE_OK) {
    LOG_ERR(0x04, "%s(%d): Failed to prepare statement: %s\n",
      __func__, id, sqlite3_errmsg(s_db));
    return kPlErrInternal;
  }

  ret = sqlite3_bind_int(stmt, 1, id);
  if (ret != SQLITE_OK) {
    LOG_ERR(0x05, "%s(%d): Failed to bind id parameter: %s\n",
      __func__, id, sqlite3_errmsg(s_db));
    ret = kPlErrInternal;
    goto out_finalize_statement;
  }

  ret = sqlite3_bind_zeroblob(stmt, 2, size);
  if (ret != SQLITE_OK) {
    LOG_ERR(0x06, "%s(%d): Failed to bind blob parameter: %s\n",
      __func__, id, sqlite3_errmsg(s_db));
    ret = kPlErrInternal;
    goto out_finalize_statement;
  }

  ret = sqlite3_step(stmt);
  if (ret != SQLITE_DONE) {
    LOG_ERR(0x07, "%s(%d): Failed to execute statement: %s\n",
      __func__, id, sqlite3_errmsg(s_db));
    ret = kPlErrInternal;
    goto out_finalize_statement;
  }

  ret = kPlErrCodeOk;

out_finalize_statement:
  sqlite3_finalize(stmt);

  return ret;
}

// ----------------------------------------------------------------------------
static PlErrCode PlStorageSqliteIdDeleteRow(PlStorageDataId id) {
  const char *sql = "delete from psm_data where rowid = ?";
  sqlite3_stmt *stmt;
  int ret = UINT32_MAX;

  if (s_id_tbl[id].access == O_RDONLY) {
    LOG_ERR(0x08, "%s(%d): Cant delete readonly data\n", __func__, id);
    return kPlErrInvalidOperation;
  }

  ret = sqlite3_prepare_v3(s_db, sql, -1, 0, &stmt, NULL);
  if (ret != SQLITE_OK) {
    LOG_ERR(0x09, "%s(%d): Failed to prepare statement: %s\n",
      __func__, id, sqlite3_errmsg(s_db));
    return kPlErrInternal;
  }

  ret = sqlite3_bind_int(stmt, 1, id);
  if (ret != SQLITE_OK) {
    LOG_ERR(0x0A, "%s(%d): Failed to bind id parameter: %s\n",
      __func__, id, sqlite3_errmsg(s_db));
    ret = kPlErrInternal;
    goto out_finalize_statement;
  }

  ret = sqlite3_step(stmt);
  if (ret == SQLITE_DONE) {
    ret = kPlErrCodeOk;
    goto out_finalize_statement;
  }

  LOG_ERR(0x0B, "%s(%d): Failed to delete statement: %s\n",
    __func__, id, sqlite3_errmsg(s_db));
  ret = kPlErrInternal;

out_finalize_statement:
  sqlite3_finalize(stmt);
  return ret;
}

// ----------------------------------------------------------------------------
static PlErrCode PlStorageCreateHandle(PlStorageDataId id, int oflags,
                                       PlStorageHandle *handle) {
  struct PlStorageSqlite3Handle *hdl;
  sqlite3_blob *blob;
  int flag;
  int ret;
  bool hasRow;

  if (!s_initialized) {
    LOG_ERR(0x0C, "%s(%d): Uninitialized\n", __func__, id);
    return kPlErrInvalidState;
  }

  hdl = malloc(sizeof(struct PlStorageSqlite3Handle));
  if (!hdl) {
    LOG_ERR(0x0D, "Failed to allocate handle\n");
    return kPlErrMemory;
  }

  ret = PlStorageSqliteIdHasRow(id, &hasRow);
  if (ret) {
    goto err_free;
  }

  if (s_id_tbl[id].access == O_RDONLY && oflags != PL_STORAGE_OPEN_RDONLY) {
    ret = kPlErrInternal;
    goto err_free;
  }

  if (!hasRow && oflags == PL_STORAGE_OPEN_RDONLY) {
    ret = kPlErrNotFound;
    goto err_free;
  }

  if (!hasRow || oflags == PL_STORAGE_OPEN_WRONLY) {
    /*
     * The resize function performs an upsert, and a size of -1 is
     * used by SQLite to create a zero-sized blob.
     */
    ret = PlStorageSqliteResizeBlob(id, -1);
    if (ret) {
      goto err_free;
    }
  }

  /*
   * SQLite doesn't support a write only open. If the caller asks for
   * RDWR or WRONLY we open in read/write mode.
   */
  flag = oflags & (PL_STORAGE_OPEN_RDWR | PL_STORAGE_OPEN_WRONLY) ? 1 : 0;

  ret = sqlite3_blob_open(s_db, "main", "psm_data", "value", id, flag, &blob);
  if (ret != SQLITE_OK) {
    ret = kPlErrInternal;
    goto err_free;
  }

  memset(hdl, 0, sizeof(*hdl));
  hdl->blob = blob;
  hdl->id = id;
  hdl->oflags = oflags;

  TAILQ_INSERT_TAIL(&s_handle_list, hdl, list);

  *handle = hdl;

  return kPlErrCodeOk;

err_free:
  free(hdl);
  return ret;
}

// ----------------------------------------------------------------------------
PlErrCode PlStorageOpen(PlStorageDataId id, int oflags,
                        PlStorageHandle *handle) {
  PlErrCode ret;

  if (handle == NULL) {
    LOG_ERR(0x0E, "%s(%d): handle is NULL\n", __func__, id);
    return kPlErrInvalidParam;
  }

  if (oflags != PL_STORAGE_OPEN_RDONLY &&
      oflags != PL_STORAGE_OPEN_WRONLY &&
      oflags != PL_STORAGE_OPEN_RDWR) {
    LOG_ERR(0x0F, "%s(%d): Invalid oflags\n", __func__, id);
    return kPlErrInvalidParam;
  }

  ret = pthread_mutex_lock(&s_mutex);
  if (ret) {
    return kPlErrLock;
  }

  ret = PlStorageCreateHandle(id, oflags, handle);
  pthread_mutex_unlock(&s_mutex);

  return ret;
}

// ----------------------------------------------------------------------------
static void PlStorageDestroyHandle(struct PlStorageSqlite3Handle *hdl) {
  TAILQ_REMOVE(&s_handle_list, hdl, list);
  sqlite3_blob_close(hdl->blob);
  free(hdl);
}

// ----------------------------------------------------------------------------
PlErrCode PlStorageClose(const PlStorageHandle handle) {
  struct PlStorageSqlite3Handle *hdl = NULL;
  int ret;

  ret = pthread_mutex_lock(&s_mutex);
  if (ret) {
    return kPlErrLock;
  }

  if (!s_initialized) {
    LOG_ERR(0x10, "%s(): Uninitialized\n", __func__);
    ret = kPlErrInvalidState;
    goto out_unlock_mutex;
  }

  hdl = toSqlite3Handle(handle);
  if (hdl == NULL) {
    LOG_ERR(0x11, "%s(): Invalid handle\n", __func__);
    ret = kPlErrInvalidParam;
    goto out_unlock_mutex;
  }

  PlStorageDestroyHandle(hdl);
  ret = kPlErrCodeOk;
out_unlock_mutex:
  pthread_mutex_unlock(&s_mutex);
  return ret;
}

// ----------------------------------------------------------------------------
PlErrCode PlStorageSeek(const PlStorageHandle handle, int32_t offset,
                        PlStorageSeekType type, int32_t *cur_pos) {
  struct PlStorageSqlite3Handle *hdl = NULL;
  int position;
  int ret;

  ret = pthread_mutex_lock(&s_mutex);
  if (ret) {
    return kPlErrLock;
  }

  if (!s_initialized) {
    LOG_ERR(0x12, "%s(): Uninitialized\n", __func__);
    ret = kPlErrInvalidState;
    goto out_unlock_mutex;
  }

  if (!cur_pos) {
    LOG_ERR(0x13, "%s(): cur_pos parameter is null\n", __func__);
    ret = kPlErrInvalidParam;
    goto out_unlock_mutex;
  }

  if (type >= PlStorageSeekMax) {
    LOG_ERR(0x14, "%s(,,%d): Invalid type parameter\n", __func__, type);
    ret = kPlErrInvalidParam;
    goto out_unlock_mutex;
  }

  hdl = toSqlite3Handle(handle);
  if (hdl == NULL) {
    LOG_ERR(0x15, "%s(): Invalid handle\n", __func__);
    ret = kPlErrInvalidParam;
    goto out_unlock_mutex;
  }
  if (s_id_tbl[hdl->id].is_list_type == true) {
    if (offset != 0 || type != PlStorageSeekSet) {
      LOG_ERR(0x16, "%s(): ID Type=File seek error offset=%d type=%u\n",
              __func__, offset, type);
      ret = kPlErrNoSupported;
      goto out_unlock_mutex;
    }
  }

  if (type == PlStorageSeekEnd) {
    position = sqlite3_blob_bytes(hdl->blob) + offset;
  } else {
    position = type == PlStorageSeekSet ? offset : hdl->offset + offset;
  }
  if (position < 0) {
    LOG_ERR(0x17, "%s(,%d,%d): Offset results in negative position\n",
      __func__, offset, type);
    ret = kPlErrInvalidParam;
    goto out_unlock_mutex;
  }

  /*
   * The SQLite blob handle has no concept of a position marker, so we
   * just track it internally.
   */
  hdl->offset = position;
  *cur_pos = hdl->offset;
  ret = kPlErrCodeOk;

out_unlock_mutex:
  pthread_mutex_unlock(&s_mutex);
  return ret;
}

// ----------------------------------------------------------------------------
static PlErrCode PlStorageReadLocked(struct PlStorageSqlite3Handle *hdl,
             void *out_buf, uint32_t read_size,
             uint32_t *out_size) {
  size_t blob_size;
  int ret;

  /*
   * The specification for this function mandates that it not fail if the
   * read_size and current offset exceed the size of the stored data. On
   * the other hand SQLite _will_ fail when attempting to read past the
   * blob, so we need to check that.
   */
  blob_size = sqlite3_blob_bytes(hdl->blob);
  if (s_id_tbl[hdl->id].is_list_type == true) {
    if (read_size < blob_size) {
      LOG_ERR(0x18, "%s(%zu>%" PRIu32 "): too short buffer for list type\n",
        __func__, blob_size, read_size);
      return kPlErrBufferOverflow;
    }
  }

  if ((hdl->offset + read_size) >= blob_size) {
    read_size = (uint32_t)blob_size - hdl->offset;
  }

  ret = sqlite3_blob_read(hdl->blob, out_buf, read_size, hdl->offset);
  if (ret != SQLITE_OK) {
    LOG_ERR(0x19, "%s(,,%" PRIu32 ",): Failed to read blob: %s\n",
      __func__, read_size, sqlite3_errmsg(s_db));
    return kPlErrInternal;
  }

  hdl->offset += read_size;
  *out_size = read_size;

  return kPlErrCodeOk;
}

// ----------------------------------------------------------------------------
PlErrCode PlStorageRead(const PlStorageHandle handle, void *out_buf,
                        uint32_t read_size, uint32_t *out_size) {
  int ret;
  struct PlStorageSqlite3Handle *hdl = NULL;

  if (out_buf == NULL) {
    LOG_ERR(0x1A, "%s(): out_buf is NULL\n", __func__);
    return kPlErrInvalidParam;
  }

  if (out_size == NULL) {
    LOG_ERR(0x1B, "%s(): out_size is NULL\n", __func__);
    return kPlErrInvalidParam;
  }

  ret = pthread_mutex_lock(&s_mutex);
  if (ret) {
    return kPlErrLock;
  }

  if (!s_initialized) {
    LOG_ERR(0x1C, "%s(): Uninitialized\n", __func__);
    ret = kPlErrInvalidState;
    goto out_unlock_mutex;
  }

  hdl = toSqlite3Handle(handle);
  if (hdl == NULL) {
    LOG_ERR(0x1D, "%s(): Invalid handle\n", __func__);
    ret = kPlErrInvalidParam;
    goto out_unlock_mutex;
  }

  ret = PlStorageReadLocked(hdl, out_buf, read_size, out_size);

out_unlock_mutex:
  pthread_mutex_unlock(&s_mutex);
  return ret;
}

// ----------------------------------------------------------------------------
  /*
   * The blob API cannot change the blob size. If the blob is large enough
   * to accomodate the new write then all is well; otherwise we'll need to
   * grow it. Note that we do not bother to shrink a blob that is larger
   * than required.
   */

static PlErrCode PlStorageResizeKeepData(struct PlStorageSqlite3Handle *hdl,
              uint32_t keep_size, uint32_t new_size) {
  void *buf;
  int ret;

  buf = malloc(keep_size);
  if (!buf) {
    LOG_ERR(0x1E, "%s(): Failed to allocate memory(%" PRIu32 ")\n",
            __func__, keep_size);
    return kPlErrMemory;
  }

  ret = sqlite3_blob_read(hdl->blob, buf, keep_size, 0);
  if (ret != SQLITE_OK) {
    LOG_ERR(0x1F, "%s(): Failed to read blob: %s\n",
      __func__, sqlite3_errmsg(s_db));
    ret = kPlErrInternal;
    goto err_free_buf;
  }

  ret = PlStorageSqliteResizeBlob(hdl->id, new_size);
  if (ret) {
    ret = kPlErrInternal;
    goto err_free_buf;
  }

  /*
    * Having resized an existing blob, we will also need to re-open
    * it, as it will now have been marked expired.
    */
  ret = sqlite3_blob_reopen(hdl->blob, hdl->id);
  if (ret != SQLITE_OK) {
    LOG_ERR(0x20, "%s(): Failed to reopen blob: %s\n",
      __func__, sqlite3_errmsg(s_db));
    ret = kPlErrInternal;
    goto err_free_buf;
  }

  ret = sqlite3_blob_write(hdl->blob, buf, keep_size, 0);
  if (ret != SQLITE_OK) {
    LOG_ERR(0x21, "%s(): Failed to restore blob: %s\n",
      __func__, sqlite3_errmsg(s_db));
    ret = kPlErrInternal;
    goto err_free_buf;
  }

  ret = kPlErrCodeOk;

err_free_buf:
  free(buf);
  return ret;
}
// ----------------------------------------------------------------------------
static PlErrCode PlStorageWriteLocked(struct PlStorageSqlite3Handle *hdl,
              const void *src_buf, uint32_t write_size,
              uint32_t *out_size) {
  size_t blob_size;
  int ret;

  blob_size = sqlite3_blob_bytes(hdl->blob);
  if (s_id_tbl[hdl->id].is_list_type == true) {
    if (write_size > UINT16_MAX) {
      LOG_ERR(0x22, "%s(): Over write_size=%u", __func__, write_size);
      return kPlErrBufferOverflow;
    }
  }
  if (blob_size == 0 || s_id_tbl[hdl->id].is_list_type == true) {
    hdl->offset = 0;
    ret = PlStorageSqliteResizeBlob(hdl->id, write_size);
    if (ret) {
      LOG_ERR(0x23, "%s(,,%d,): Failed to resize blob:%u",
        __func__, write_size, ret);
      return kPlErrInternal;
    }
    ret = sqlite3_blob_reopen(hdl->blob, hdl->id);
    if (ret != SQLITE_OK) {
      LOG_ERR(0x24, "%s(): Failed to reopen blob: %s\n",
        __func__, sqlite3_errmsg(s_db));
      return kPlErrInternal;
    }
    goto append;
  }

  if (hdl->offset + write_size > blob_size) {
    ret = PlStorageResizeKeepData(hdl, blob_size, hdl->offset + write_size);
    if (ret != SQLITE_OK) {
      return ret;
    }
  }

append:
  ret = sqlite3_blob_write(hdl->blob, src_buf, write_size, hdl->offset);
  if (ret != SQLITE_OK) {
    LOG_ERR(0x25, "%s()Failed to write to blob: %s\n", __func__,
      sqlite3_errmsg(s_db));
    return kPlErrInternal;
  }

  hdl->offset += write_size;
  *out_size = write_size;

  return kPlErrCodeOk;
}

// ----------------------------------------------------------------------------
PlErrCode PlStorageWrite(const PlStorageHandle handle, const void *src_buf,
                         uint32_t write_size, uint32_t *out_size) {
  PlErrCode ret;
  struct PlStorageSqlite3Handle *hdl = NULL;

  if (src_buf == NULL) {
    LOG_ERR(0x26, "%s(): src_buf is NULL\n", __func__);
    return kPlErrInvalidParam;
  }

  if (out_size == NULL) {
    LOG_ERR(0x27, "%s(): out_size is NULL\n", __func__);
    return kPlErrInvalidParam;
  }

  ret = pthread_mutex_lock(&s_mutex);
  if (ret) {
    return kPlErrLock;
  }

  if (!s_initialized) {
    LOG_ERR(0x28, "%s(): Uninitialized\n", __func__);
    ret = kPlErrInvalidState;
    goto out_unlock_mutex;
  }

  hdl = toSqlite3Handle(handle);
  if (hdl == NULL) {
    LOG_ERR(0x29, "%s(): Invalid handle\n", __func__);
    ret = kPlErrInvalidParam;
    goto out_unlock_mutex;
  }

  if (hdl->oflags == PL_STORAGE_OPEN_RDONLY ||
      s_id_tbl[hdl->id].access == O_RDONLY) {
    LOG_ERR(0x2A, "%s(): Invalid Operation oflags:%d, access:%d\n",
      __func__, hdl->oflags, s_id_tbl[hdl->id].access);
    ret = kPlErrInvalidOperation;
    goto out_unlock_mutex;
  }

  uint32_t size = write_size +
                  (s_id_tbl[hdl->id].is_list_type ? 0 : hdl->offset);
  if ((s_id_tbl[hdl->id].max_size != 0) &&
      (s_id_tbl[hdl->id].max_size < size)) {
    LOG_ERR(0x2B, "%s(,,%d,): Limit over max size(%d)\n", __func__, write_size,
            s_id_tbl[hdl->id].max_size);
    ret = kPlErrInvalidValue;
    goto out_unlock_mutex;
  }

  ret = PlStorageWriteLocked(hdl, src_buf, write_size, out_size);

out_unlock_mutex:
  pthread_mutex_unlock(&s_mutex);
  return ret;
}

// ----------------------------------------------------------------------------
PlErrCode PlStorageErase(PlStorageDataId id) {
  PlErrCode ret;
  bool hasRow;

  if (id >= PlStorageDataMax) {
    LOG_ERR(0x2C, "%s(%d): invalid id\n", __func__, id);
    return kPlErrInvalidParam;
  }

  ret = pthread_mutex_lock(&s_mutex);
  if (ret) {
    return kPlErrLock;
  }

  if (!s_initialized) {
    LOG_ERR(0x2D, "%s(%d): Uninitialized\n", __func__, id);
    ret = kPlErrInvalidState;
    goto out_unlock_mutex;
  }

  ret = PlStorageSqliteIdHasRow(id, &hasRow);
  if (ret) {
    goto out_unlock_mutex;
  }

  if (!hasRow) {
    ret = kPlErrNotFound;
    goto out_unlock_mutex;
  }

  ret = PlStorageSqliteIdDeleteRow(id);
  if (ret) {
    goto out_unlock_mutex;
  }
  ret = kPlErrCodeOk;

out_unlock_mutex:
  pthread_mutex_unlock(&s_mutex);
  return ret;
}

// ----------------------------------------------------------------------------
PlErrCode PlStorageFactoryReset(PlStorageDataId id) {
  return PlStorageErase(id);
}

// ----------------------------------------------------------------------------
PlErrCode PlStorageDRead(PlStorageDataId id, int oflags, void *out_buf,
                         uint32_t read_size, uint32_t *out_size) {
  return kPlErrNoSupported;
}

// ----------------------------------------------------------------------------
PlErrCode PlStorageDWrite(PlStorageDataId id, int oflags, const void *src_buf,
                          uint32_t write_size, uint32_t *out_size) {
  return kPlErrNoSupported;
}

// ----------------------------------------------------------------------------
PlErrCode PlStorageGetDataInfo(PlStorageDataId id, PlStorageDataInfo *info) {
  sqlite3_blob *blob;
  int ret;
  bool hasRow;

  if (info == NULL) {
    LOG_ERR(0x2E, "%s(%d): info is NULL\n", __func__, id);
    return kPlErrInvalidParam;
  }

  ret = pthread_mutex_lock(&s_mutex);
  if (ret) {
    return kPlErrLock;
  }

  if (!s_initialized) {
    LOG_ERR(0x2F, "%s(%d): Uninitialized\n", __func__, id);
    ret = kPlErrInvalidState;
    goto out_unlock_mutex;
  }

  ret = PlStorageSqliteIdHasRow(id, &hasRow);
  if (ret) {
    goto out_unlock_mutex;
  }

  if (!hasRow) {
    ret = kPlErrNotFound;
    info->written_size = 0;
    goto out_unlock_mutex;
  }

  ret = sqlite3_blob_open(s_db, "main", "psm_data", "value", id, 0, &blob);
  if (ret != SQLITE_OK) {
    LOG_ERR(0x30, "%s()): Failed to open blob for id %d: %s\n",
      __func__, id, sqlite3_errmsg(s_db));
    ret = kPlErrInternal;
    goto out_unlock_mutex;
  }

  info->written_size = sqlite3_blob_bytes(blob);

  sqlite3_blob_close(blob);
  ret = kPlErrCodeOk;

out_unlock_mutex:
  pthread_mutex_unlock(&s_mutex);
  return ret;
}

// ----------------------------------------------------------------------------
PlErrCode PlStorageSwitchData(PlStorageTmpDataId src_id,
                              PlStorageDataId dst_id) {
  return kPlErrNoSupported;
}

// ----------------------------------------------------------------------------
PlErrCode PlStorageGetTmpDataId(PlStorageDataId src_id,
                                PlStorageTmpDataId *tmp_id) {
  return kPlErrNoSupported;
}

// ----------------------------------------------------------------------------
PlErrCode PlStorageGetCapabilities(PlStorageCapabilities *capabilities) {
  if (capabilities == NULL) {
    LOG_ERR(0x31, "%s(): capabilities is NULL\n", __func__);
    return kPlErrInvalidParam;
  }
  capabilities->enable_tmp_id = false;
  return kPlErrCodeOk;
}

// ----------------------------------------------------------------------------
PlErrCode PlStorageGetIdCapabilities(PlStorageDataId id,
                                    PlStorageIdCapabilities *id_capabilities) {
  if (!s_initialized) {
    LOG_ERR(0x32, "%s(%d): Uninitialized\n", __func__, id);
    return kPlErrInvalidState;
  }

  if (id >= PlStorageDataMax) {
    LOG_ERR(0x33, "%s(%d): invalid id\n", __func__, id);
    return kPlErrInvalidParam;
  }

  if (id_capabilities == NULL) {
    LOG_ERR(0x34, "%s(%d): id_capabilities is NULL\n", __func__, id);
    return kPlErrInvalidParam;
  }

  id_capabilities->is_read_only = (s_id_tbl[id].access == O_RDONLY);
  id_capabilities->enable_seek = !s_id_tbl[id].is_list_type;
  return kPlErrCodeOk;
}

// ----------------------------------------------------------------------------
PlErrCode PlStorageInitialize(void) {
  char *db_path;
  char *err;
  int ret;

  ret = pthread_mutex_lock(&s_mutex);
  if (ret) {
    return kPlErrLock;
  }

  db_path = getenv("EDGE_DEVICE_CORE_DB_PATH");
  if (!db_path) {
    db_path = DEFAULT_SQLITE3_DATABASE_PATH;
  }

  ret = sqlite3_open_v2(db_path, &s_db,
            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
  if (ret != SQLITE_OK) {
    LOG_ERR(0x35, "%s(): Failed to open database:%d, %s\n",
      __func__, ret, sqlite3_errmsg(s_db));
    sqlite3_close_v2(s_db);
    s_db = NULL;
    ret = kPlErrInternal;
    goto out_unlock_mutex;
  }

  const char *tblSql =
    "create table if not exists psm_data ("
      "key integer primary key unique,"
      "value"
    ")";

  ret = sqlite3_exec(s_db, tblSql, NULL, NULL, &err);
  if (ret != SQLITE_OK) {
    LOG_ERR(0x36, "Failed to create psm_data table: %s\n", err);
    sqlite3_free(err);
    ret = kPlErrInternal;
    goto err_close_db;
  }

  TAILQ_INIT(&s_handle_list);
  s_initialized = true;
  ret = kPlErrCodeOk;

out_unlock_mutex:
  pthread_mutex_unlock(&s_mutex);
  return ret;

err_close_db:
  sqlite3_close_v2(s_db);
  pthread_mutex_unlock(&s_mutex);
  return ret;
}

// ----------------------------------------------------------------------------
PlErrCode PlStorageFinalize(void) {
  struct PlStorageSqlite3Handle *hdl, *tmp;
  int ret;

  ret = pthread_mutex_lock(&s_mutex);
  if (ret) {
    return kPlErrLock;
  }

  s_initialized = false;

  TAILQ_FOREACH_SAFE(hdl, &s_handle_list, list, tmp)
    PlStorageDestroyHandle(hdl);

  sqlite3_close_v2(s_db);
  s_db = NULL;
  pthread_mutex_unlock(&s_mutex);

  return kPlErrCodeOk;
}

// ----------------------------------------------------------------------------
PlErrCode PlStorageClean(void) {
  const char *deleteSql = "vacuum;";
  char *err;
  int ret;

  ret = pthread_mutex_lock(&s_mutex);
  if (ret) {
    return kPlErrLock;
  }

  if (!s_initialized) {
    LOG_ERR(0x37, "%s(): Uninitialized\n", __func__);
    ret = kPlErrInvalidState;
    goto out_unlock_mutex;
  }

  ret = sqlite3_exec(s_db, deleteSql, NULL, NULL, &err);
  if (ret != SQLITE_OK) {
    LOG_ERR(0x38, "Failed to create data table: %s\n", err);
    sqlite3_free(err);
    ret = kPlErrInternal;
    goto out_unlock_mutex;
  }

  ret = kPlErrCodeOk;

out_unlock_mutex:
  pthread_mutex_unlock(&s_mutex);
  return ret;
}

// ----------------------------------------------------------------------------
PlErrCode PlStorageDowngrade(void) {
  return kPlErrNoSupported;
}
