/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "log_manager_list.h"

#include "log_manager.h"
#include "log_manager_internal.h"

#define LOG_MANAGER_LIST_INVALID_VALUE (-1)
/****************************************************************************
 * Dlog callback static variables
 ****************************************************************************/
// Mutex for dlog call list
static pthread_mutex_t s_dlog_callback_list_mutex = PTHREAD_MUTEX_INITIALIZER;
/* --- Start of s_dlog_callback_list_mutex scope --- */
// Callback list for notification upon parameter setting change
static struct LogManagerSettingCallbackList s_change_dlog_callback_list;
/* --- End of s_dlog_callback_list_mutex scope --- */

/****************************************************************************
 * Local upload static variables
 ****************************************************************************/
// Mutex for local upload list
static pthread_mutex_t s_local_upload_dlog_data_list_mutex =
    PTHREAD_MUTEX_INITIALIZER;
/* --- Start of s_local_upload_dlog_data_list_mutex scope --- */
// List for managing local upload requests
static struct LogManagerLocalUploadDlogDataList s_local_upload_dlog_data_list;
/* --- End of s_local_upload_dlog_data_list_mutex scope --- */

/****************************************************************************
 * Cloud upload static variables
 ****************************************************************************/
// Mutex for cloud upload list
static pthread_mutex_t s_cloud_upload_dlog_data_list_mutex =
    PTHREAD_MUTEX_INITIALIZER;
/* --- Start of s_cloud_upload_dlog_data_list_mutex scope --- */
// List for managing cloud upload requests
static struct LogManagerCloudUploadDlogDataList s_cloud_upload_dlog_data_list;
/* --- End of s_cloud_upload_dlog_data_list_mutex scope --- */

void EsfLogManagerListInit(void) {
  SLIST_INIT(&s_change_dlog_callback_list);
  SLIST_INIT(&s_local_upload_dlog_data_list);
  SLIST_INIT(&s_cloud_upload_dlog_data_list);
}
ChangeDlogCallbackDataT *EsfLogManagerSearchCallbackList(
    EsfLogManagerSettingBlockType const block_type) {
  ChangeDlogCallbackDataT *entry = (ChangeDlogCallbackDataT *)NULL;
  ChangeDlogCallbackDataT *temp = (ChangeDlogCallbackDataT *)NULL;

  if (pthread_mutex_lock(&s_dlog_callback_list_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_lock.\n");
    return NULL;
  }

  SLIST_FOREACH_SAFE(entry, &s_change_dlog_callback_list, m_next, temp) {
    if (entry != NULL) {
      EsfLogManagerSettingBlockType group_id =
          EsfLogManagerInternalGetGroupID(entry->m_module_id);
      if (group_id == block_type) {
        break;
      }
    }
  }

  if (pthread_mutex_unlock(&s_dlog_callback_list_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_unlock.\n");
    return NULL;
  }

  return entry;
}

EsfLogManagerStatus EsfLogManagerRegisterCallbackList(
    uint32_t module_id, const EsfLogManagerChangeDlogCallback *const callback) {
  ChangeDlogCallbackDataT *data =
      (ChangeDlogCallbackDataT *)malloc(sizeof(ChangeDlogCallbackDataT));
  if (data == (ChangeDlogCallbackDataT *)NULL) {
    ESF_LOG_MANAGER_ERROR("Failed to malloc.\n");
    return kEsfLogManagerStatusFailed;
  }

  data->m_module_id = module_id;
  data->m_callback = *callback;

  if (pthread_mutex_lock(&s_dlog_callback_list_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_lock.\n");
    free(data);
    return kEsfLogManagerStatusFailed;
  }

  SLIST_INSERT_HEAD(&s_change_dlog_callback_list, data, m_next);

  if (pthread_mutex_unlock(&s_dlog_callback_list_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_unlock.\n");
    return kEsfLogManagerStatusFailed;
  }

  return kEsfLogManagerStatusOk;
}

EsfLogManagerStatus EsfLogManagerUnregisterCallbackList(uint32_t module_id) {
  ChangeDlogCallbackDataT *entry = (ChangeDlogCallbackDataT *)NULL;
  ChangeDlogCallbackDataT *temp = (ChangeDlogCallbackDataT *)NULL;
  bool found = false;

  if (pthread_mutex_lock(&s_dlog_callback_list_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_lock.\n");
    return kEsfLogManagerStatusFailed;
  }

  SLIST_FOREACH_SAFE(entry, &s_change_dlog_callback_list, m_next, temp) {
    if (entry->m_module_id == module_id) {
      found = true;
      break;
    }
  }
  if (found == true) {
    SLIST_REMOVE(&s_change_dlog_callback_list, entry, ChangeDlogCallbackData,
                 m_next);
    free(entry);
  } else {
    ESF_LOG_MANAGER_ERROR("Change Dlog callback not found.\n");
    (void)pthread_mutex_unlock(&s_dlog_callback_list_mutex);
    return kEsfLogManagerStatusParamError;
  }

  if (pthread_mutex_unlock(&s_dlog_callback_list_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_unlock\n");
    return kEsfLogManagerStatusFailed;
  }

  return kEsfLogManagerStatusOk;
}

EsfLogManagerStatus EsfLogManagerDeleteCallbackList(void) {
  ChangeDlogCallbackDataT *cb_entry = (ChangeDlogCallbackDataT *)NULL;
  ChangeDlogCallbackDataT *cb_temp = (ChangeDlogCallbackDataT *)NULL;

  if (pthread_mutex_lock(&s_dlog_callback_list_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_lock.\n");
    return kEsfLogManagerStatusFailed;
  }

  SLIST_FOREACH_SAFE(cb_entry, &s_change_dlog_callback_list, m_next, cb_temp) {
    if (cb_entry != NULL) {
      SLIST_REMOVE(&s_change_dlog_callback_list, cb_entry,
                   ChangeDlogCallbackData, m_next);
      free(cb_entry);
    }
  }

  if (pthread_mutex_unlock(&s_dlog_callback_list_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_unlock\n");
    return kEsfLogManagerStatusFailed;
  }

  return kEsfLogManagerStatusOk;
}

EsfLogManagerStatus EsfLogManagerSetLocalUploadStatus(LogUploadStatusT status) {
  LocalUploadDlogDataT *entry = (LocalUploadDlogDataT *)NULL;
  LocalUploadDlogDataT *temp = (LocalUploadDlogDataT *)NULL;
  LocalUploadDlogDataT *keep = (LocalUploadDlogDataT *)NULL;

  if (pthread_mutex_lock(&s_local_upload_dlog_data_list_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_lock.\n");
    return kEsfLogManagerStatusFailed;
  }

  if (SLIST_EMPTY(&s_local_upload_dlog_data_list) == false) {
    SLIST_FOREACH_SAFE(entry, &s_local_upload_dlog_data_list, m_next, temp) {
      keep = entry;
    }
  }

  if (keep != NULL) {
    keep->m_status = status;
    if (status == kUploadStatusRequest) {
      keep->m_upload_complete_size = 0;
    }
  }

  if (pthread_mutex_unlock(&s_local_upload_dlog_data_list_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_unlock.\n");
    return kEsfLogManagerStatusFailed;
  }

  return kEsfLogManagerStatusOk;
}

EsfLogManagerStatus EsfLogManagerSetCloudUploadStatus(LogUploadStatusT status) {
  CloudUploadDlogDataT *entry = (CloudUploadDlogDataT *)NULL;
  CloudUploadDlogDataT *temp = (CloudUploadDlogDataT *)NULL;
  CloudUploadDlogDataT *keep = (CloudUploadDlogDataT *)NULL;

  if (pthread_mutex_lock(&s_cloud_upload_dlog_data_list_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_lock.\n");
    return kEsfLogManagerStatusFailed;
  }

  if (SLIST_EMPTY(&s_cloud_upload_dlog_data_list) == false) {
    SLIST_FOREACH_SAFE(entry, &s_cloud_upload_dlog_data_list, m_next, temp) {
      keep = entry;
    }
  }

  if (keep != NULL) {
    keep->m_status = status;
    if (status == kUploadStatusRequest) {
      keep->m_upload_complete_size = 0;
    }
  }

  if (pthread_mutex_unlock(&s_cloud_upload_dlog_data_list_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_unlock.\n");
    return kEsfLogManagerStatusFailed;
  }

  return kEsfLogManagerStatusOk;
}

int32_t EsfLogManagerCheckLocalListNum(void) {
  uint32_t count = 0;
  LocalUploadDlogDataT *entry = (LocalUploadDlogDataT *)NULL;
  LocalUploadDlogDataT *temp = (LocalUploadDlogDataT *)NULL;
  LocalUploadDlogDataT *keep = (LocalUploadDlogDataT *)NULL;

  if (pthread_mutex_lock(&s_local_upload_dlog_data_list_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_lock.\n");
    return LOG_MANAGER_LIST_INVALID_VALUE;
  }

  if (SLIST_EMPTY(&s_local_upload_dlog_data_list) == false) {
    SLIST_FOREACH_SAFE(entry, &s_local_upload_dlog_data_list, m_next, temp) {
      count++;
    }
  }

  if (pthread_mutex_unlock(&s_local_upload_dlog_data_list_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_unlock.\n");
    return LOG_MANAGER_LIST_INVALID_VALUE;
  }

  return count;
}

int32_t EsfLogManagerCheckCloudListNum(void) {
  uint32_t count = 0;
  CloudUploadDlogDataT *entry = (CloudUploadDlogDataT *)NULL;
  CloudUploadDlogDataT *temp = (CloudUploadDlogDataT *)NULL;
  CloudUploadDlogDataT *keep = (CloudUploadDlogDataT *)NULL;

  if (pthread_mutex_lock(&s_cloud_upload_dlog_data_list_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_lock.\n");
    return LOG_MANAGER_LIST_INVALID_VALUE;
  }

  if (SLIST_EMPTY(&s_cloud_upload_dlog_data_list) == false) {
    SLIST_FOREACH_SAFE(entry, &s_cloud_upload_dlog_data_list, m_next, temp) {
      count++;
    }
  }

  if (pthread_mutex_unlock(&s_cloud_upload_dlog_data_list_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_unlock.\n");
    return LOG_MANAGER_LIST_INVALID_VALUE;
  }

  return count;
}

LocalUploadDlogDataT *EsfLogManagerGetLocalTailListData(void) {
  LocalUploadDlogDataT *entry = (LocalUploadDlogDataT *)NULL;
  LocalUploadDlogDataT *temp = (LocalUploadDlogDataT *)NULL;
  LocalUploadDlogDataT *keep = (LocalUploadDlogDataT *)NULL;

  if (pthread_mutex_lock(&s_local_upload_dlog_data_list_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_lock.\n");
    return NULL;
  }

  if (SLIST_EMPTY(&s_local_upload_dlog_data_list) == false) {
    SLIST_FOREACH_SAFE(entry, &s_local_upload_dlog_data_list, m_next, temp) {
      keep = entry;
    }
  }

  if (pthread_mutex_unlock(&s_local_upload_dlog_data_list_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_unlock.\n");
    return NULL;
  }

  return keep;
}

CloudUploadDlogDataT *EsfLogManagerGetCloudTailListData(void) {
  CloudUploadDlogDataT *entry = (CloudUploadDlogDataT *)NULL;
  CloudUploadDlogDataT *temp = (CloudUploadDlogDataT *)NULL;
  CloudUploadDlogDataT *keep = (CloudUploadDlogDataT *)NULL;

  if (pthread_mutex_lock(&s_cloud_upload_dlog_data_list_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_lock.\n");
    return NULL;
  }

  if (SLIST_EMPTY(&s_cloud_upload_dlog_data_list) == false) {
    SLIST_FOREACH_SAFE(entry, &s_cloud_upload_dlog_data_list, m_next, temp) {
      keep = entry;
    }
  }

  if (pthread_mutex_unlock(&s_cloud_upload_dlog_data_list_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_unlock.\n");
    return NULL;
  }

  return keep;
}

EsfLogManagerStatus EsfLogManagerRegisterLocalList(
    EsfLogManagerSettingBlockType block_type,
    EsfLogManagerBulkDlogCallback callback, void *user_data, size_t data_size,
    uint8_t *data) {
  LocalUploadDlogDataT *add_data =
      (LocalUploadDlogDataT *)malloc(sizeof(LocalUploadDlogDataT));
  if (add_data == (LocalUploadDlogDataT *)NULL) {
    ESF_LOG_MANAGER_ERROR("Failed to malloc.\n");
    return kEsfLogManagerStatusFailed;
  }

  add_data->m_addr = data;
  add_data->m_upload_size = data_size;
  add_data->m_upload_complete_size = 0;
  add_data->m_block_type = block_type;
  add_data->m_callback = callback;
  add_data->m_user_data = user_data;
  add_data->m_status = kUploadStatusRequest;
  add_data->m_file_name = NULL;
  clock_gettime(CLOCK_REALTIME, &add_data->m_time_stamp);

  if (pthread_mutex_lock(&s_local_upload_dlog_data_list_mutex) != 0) {
    free(add_data);
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_lock.\n");
    return kEsfLogManagerStatusFailed;
  }

  SLIST_INSERT_HEAD(&s_local_upload_dlog_data_list, add_data, m_next);

  if (pthread_mutex_unlock(&s_local_upload_dlog_data_list_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_unlock.\n");
    (void)EsfLogManagerUnregisterLocalListData(kUnregisterHead);
    return kEsfLogManagerStatusFailed;
  }

  return kEsfLogManagerStatusOk;
}
EsfLogManagerStatus EsfLogManagerRegisterCloudList(
    EsfLogManagerSettingBlockType block_type,
    EsfLogManagerBulkDlogCallback callback, void *user_data, size_t data_size,
    uint8_t *data) {
  CloudUploadDlogDataT *add_data =
      (CloudUploadDlogDataT *)malloc(sizeof(CloudUploadDlogDataT));
  if (add_data == (CloudUploadDlogDataT *)NULL) {
    ESF_LOG_MANAGER_ERROR("Failed to malloc.\n");
    return kEsfLogManagerStatusFailed;
  }

  add_data->m_addr = data;
  add_data->m_upload_size = data_size;
  add_data->m_upload_complete_size = 0;
  add_data->m_block_type = block_type;
  add_data->m_callback = callback;
  add_data->m_user_data = user_data;
  add_data->m_status = kUploadStatusRequest;
  add_data->m_file_name = NULL;
  clock_gettime(CLOCK_REALTIME, &add_data->m_time_stamp);

  if (pthread_mutex_lock(&s_cloud_upload_dlog_data_list_mutex) != 0) {
    free(add_data);
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_lock.\n");
    return kEsfLogManagerStatusFailed;
  }

  SLIST_INSERT_HEAD(&s_cloud_upload_dlog_data_list, add_data, m_next);

  if (pthread_mutex_unlock(&s_cloud_upload_dlog_data_list_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_unlock.\n");
    (void)EsfLogManagerUnregisterCloudListData(kUnregisterHead);
    return kEsfLogManagerStatusFailed;
  }

  return kEsfLogManagerStatusOk;
}

EsfLogManagerStatus EsfLogManagerUnregisterLocalListData(
    UnregisterPositionT position) {
  LocalUploadDlogDataT *entry = (LocalUploadDlogDataT *)NULL;
  LocalUploadDlogDataT *temp = (LocalUploadDlogDataT *)NULL;
  LocalUploadDlogDataT *keep = (LocalUploadDlogDataT *)NULL;

  EsfLogManagerBulkDlogCallback callback = NULL;
  size_t upload_size = 0;
  void *user_data = NULL;

  if (pthread_mutex_lock(&s_local_upload_dlog_data_list_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_lock.\n");
    return kEsfLogManagerStatusFailed;
  }

  if (SLIST_EMPTY(&s_local_upload_dlog_data_list) == false) {
    SLIST_FOREACH_SAFE(entry, &s_local_upload_dlog_data_list, m_next, temp) {
      keep = entry;
      if (position == kUnregisterHead) {
        break;
      }
    }
    if (keep != NULL) {
      if (keep->m_callback != NULL) {
        callback = keep->m_callback;
        upload_size = keep->m_upload_size;
        user_data = keep->m_user_data;
      } else {
        // Release the allocated buffer if there is no notification
        // callback.
        free(keep->m_addr);
        keep->m_addr = NULL;
      }
      SLIST_REMOVE(&s_local_upload_dlog_data_list, keep, UploadDlogData,
                   m_next);
      free(keep);
      keep = NULL;
    }
  }

  if (pthread_mutex_unlock(&s_local_upload_dlog_data_list_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_unlock.\n");
    return kEsfLogManagerStatusFailed;
  }

  if (callback != NULL) {
    callback(upload_size, user_data);
  }

  return kEsfLogManagerStatusOk;
}

EsfLogManagerStatus EsfLogManagerUnregisterCloudListData(
    UnregisterPositionT position) {
  CloudUploadDlogDataT *entry = (CloudUploadDlogDataT *)NULL;
  CloudUploadDlogDataT *temp = (CloudUploadDlogDataT *)NULL;
  CloudUploadDlogDataT *keep = (CloudUploadDlogDataT *)NULL;

  EsfLogManagerBulkDlogCallback callback = NULL;
  size_t upload_size = 0;
  void *user_data = NULL;

  if (pthread_mutex_lock(&s_cloud_upload_dlog_data_list_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_lock.\n");
    return kEsfLogManagerStatusFailed;
  }

  if (SLIST_EMPTY(&s_cloud_upload_dlog_data_list) == false) {
    SLIST_FOREACH_SAFE(entry, &s_cloud_upload_dlog_data_list, m_next, temp) {
      keep = entry;
      if (position == kUnregisterHead) {
        break;
      }
    }
    if (keep != NULL) {
      if (keep->m_callback != NULL) {
        callback = keep->m_callback;
        upload_size = keep->m_upload_size;
        user_data = keep->m_user_data;
      } else {
        // Release the allocated buffer if there is no notification
        // callback.
        free(keep->m_addr);
        keep->m_addr = NULL;
      }
      SLIST_REMOVE(&s_cloud_upload_dlog_data_list, keep, UploadDlogData,
                   m_next);
      free(keep);
      keep = NULL;
    }
  }
  if (pthread_mutex_unlock(&s_cloud_upload_dlog_data_list_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_unlock.\n");
    return kEsfLogManagerStatusFailed;
  }

  if (callback != NULL) {
    callback(upload_size, user_data);
  }

  return kEsfLogManagerStatusOk;
}

EsfLogManagerStatus EsfLogManagerDeleteLocalUploadList(void) {
  LocalUploadDlogDataT *local_entry = (LocalUploadDlogDataT *)NULL;
  LocalUploadDlogDataT *local_temp = (LocalUploadDlogDataT *)NULL;

  if (pthread_mutex_lock(&s_local_upload_dlog_data_list_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_lock.\n");
    return kEsfLogManagerStatusFailed;
  }

  if (SLIST_EMPTY(&s_local_upload_dlog_data_list) == false) {
    SLIST_FOREACH_SAFE(local_entry, &s_local_upload_dlog_data_list, m_next,
                       local_temp) {
      if (local_entry != NULL) {
        SLIST_REMOVE(&s_local_upload_dlog_data_list, local_entry,
                     UploadDlogData, m_next);
        // Release the allocated buffer if there is no notification callback.
        if (local_entry->m_callback == NULL) {
          free(local_entry->m_addr);
          local_entry->m_addr = NULL;
        }
        free(local_entry);
        local_entry = NULL;
      }
    }
  }

  if (pthread_mutex_unlock(&s_local_upload_dlog_data_list_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_unlock\n");
    return kEsfLogManagerStatusFailed;
  }

  return kEsfLogManagerStatusOk;
}
EsfLogManagerStatus EsfLogManagerDeleteCloudUploadList(void) {
  CloudUploadDlogDataT *cloud_entry = (CloudUploadDlogDataT *)NULL;
  CloudUploadDlogDataT *cloud_temp = (CloudUploadDlogDataT *)NULL;

  if (pthread_mutex_lock(&s_cloud_upload_dlog_data_list_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_lock.\n");
    return kEsfLogManagerStatusFailed;
  }

  if (SLIST_EMPTY(&s_cloud_upload_dlog_data_list) == false) {
    SLIST_FOREACH_SAFE(cloud_entry, &s_cloud_upload_dlog_data_list, m_next,
                       cloud_temp) {
      if (cloud_entry != NULL) {
        SLIST_REMOVE(&s_cloud_upload_dlog_data_list, cloud_entry,
                     UploadDlogData, m_next);
        // Release the allocated buffer if there is no notification callback.
        if (cloud_entry->m_callback == NULL) {
          free(cloud_entry->m_addr);
          cloud_entry->m_addr = NULL;
        }
        free(cloud_entry);
        cloud_entry = NULL;
      }
    }
  }

  if (pthread_mutex_unlock(&s_cloud_upload_dlog_data_list_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_unlock\n");
    return kEsfLogManagerStatusFailed;
  }

  return kEsfLogManagerStatusOk;
}
