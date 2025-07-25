/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESF_LOG_MANAGER_LOG_MANAGER_LIST_H_
#define ESF_LOG_MANAGER_LOG_MANAGER_LIST_H_

#ifdef __NuttX__
#include <sys/queue.h>
#else
#include <bsd/sys/queue.h>
#endif

#include <time.h>

#include "log_manager.h"

// Log update status
typedef enum {
  kUploadStatusRequest,
  kUploadStatusUploading,
  kUploadStatusFinished
} LogUploadStatusT;

// Unregister position
typedef enum {
  kUnregisterHead,
  kUnregisterTail,
} UnregisterPositionT;

// Notification of Dlog setting change
struct ChangeDlogCallbackData {
  uint32_t m_module_id;
  EsfLogManagerChangeDlogCallback m_callback;
  SLIST_ENTRY(ChangeDlogCallbackData)
  m_next;
};

// Contents to be transferred
typedef struct UploadDlogData {
  uint8_t *m_addr;
  size_t m_upload_size;
  size_t m_upload_complete_size;
  struct timespec m_time_stamp;
  EsfLogManagerSettingBlockType m_block_type;
  EsfLogManagerBulkDlogCallback m_callback;
  void *m_user_data;
  LogUploadStatusT m_status;
  char *m_file_name;
  SLIST_ENTRY(UploadDlogData)
  m_next;
} UploadDlogData;

// Callback list when parameters are changed with SetParameter.
typedef struct ChangeDlogCallbackData ChangeDlogCallbackDataT;
SLIST_HEAD(LogManagerSettingCallbackList, ChangeDlogCallbackData);

// Local upload request list.
typedef struct UploadDlogData LocalUploadDlogDataT;
SLIST_HEAD(LogManagerLocalUploadDlogDataList, UploadDlogData);

// Cloud upload request list.
typedef struct UploadDlogData CloudUploadDlogDataT;
SLIST_HEAD(LogManagerCloudUploadDlogDataList, UploadDlogData);

// """Initialization process of list data.
// Args:
//    no arguments
// Returns:
//    no return
void EsfLogManagerListInit(void);

// """Retrieve the callback data registered in the list for the specified block
// type Args:
//    block_type(EsfLogManagerSettingBlockType): Specify the block you want to
//    retrieve
// Returns:
//    ChangeDlogCallbackDataT*: On success, it returns a pointer to
//    ChangeDlogCallbackDataT. NULL: Failed to retrieve.
ChangeDlogCallbackDataT *EsfLogManagerSearchCallbackList(
    EsfLogManagerSettingBlockType const block_type);

// """Add to the callback list to be notified when a setting is changed with
// SetParameter Args:
//    module_id(uint32_t): Module ID for Callback Registration
//    callback(EsfLogManagerChangeDlogCallback): Callback Destination
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination.
EsfLogManagerStatus EsfLogManagerRegisterCallbackList(
    uint32_t module_id, const EsfLogManagerChangeDlogCallback *const callback);

// """I will remove the callback registration that notifies when changes are
// made to settings via SetParameter. Args:
//    module_id(uint32_t): Module ID for Callback Unregistration
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination.
EsfLogManagerStatus EsfLogManagerUnregisterCallbackList(uint32_t module_id);

// """I will delete the callback list that notifies when SetParameter is
// changed. Args:
//    no arguments
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination.
EsfLogManagerStatus EsfLogManagerDeleteCallbackList(void);

// """Change the status of the last data entry in the upload list to the
// specified status. Args:
//    status(LogUploadStatusT): Status to change
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination.
EsfLogManagerStatus EsfLogManagerSetLocalUploadStatus(LogUploadStatusT status);

// """Change the status of the last data entry in the upload list to the
// specified status. Args:
//    status(LogUploadStatusT): Status to change
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination.
EsfLogManagerStatus EsfLogManagerSetCloudUploadStatus(LogUploadStatusT status);

// """Retrieve the number of data entries registered in the Local list.
// Args:
//    no arguments
// Returns:
//    zero or positive value(s) : success
//    a value of -1  : error
int32_t EsfLogManagerCheckLocalListNum(void);

// """Retrieve the number of data entries registered in the Cloud list.
// Args:
//    no arguments
// Returns:
//    zero or positive value(s) : success
//    a value of -1  : error
int32_t EsfLogManagerCheckCloudListNum(void);

// """Register information in the local upload list.
// Args:
//    block_type(EsfLogManagerSettingBlockType): Block type for making upload
//    requests callback(EsfLogManagerBulkDlogCallback): Callback function for
//    completion notification *user_data(void): User data
//    data_size(size_t):Upload data size
//    data(uint8_t): Upload data
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination.
EsfLogManagerStatus EsfLogManagerRegisterLocalList(
    EsfLogManagerSettingBlockType block_type,
    EsfLogManagerBulkDlogCallback callback, void *user_data, size_t data_size,
    uint8_t *data);

// """Register information in the cloud upload list.
// Args:
//    block_type(EsfLogManagerSettingBlockType): Block type for making upload
//    requests callback(EsfLogManagerBulkDlogCallback): Callback function for
//    completion notification *user_data(void): User data
//    data_size(size_t):Upload data size
//    data(uint8_t): Upload data
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination.
EsfLogManagerStatus EsfLogManagerRegisterCloudList(
    EsfLogManagerSettingBlockType block_type,
    EsfLogManagerBulkDlogCallback callback, void *user_data, size_t data_size,
    uint8_t *data);

// """Retrieve the last data entry registered in the local upload list.
// Args:
//    no arguments
// Returns:
//    LocalUploadDlogDataT: On success, it returns a pointer to
//    LocalUploadDlogDataT. NULL: Failed to retriev.
LocalUploadDlogDataT *EsfLogManagerGetLocalTailListData(void);

// """Retrieve the last data entry registered in the cloud upload list.
// Args:
//    no arguments
// Returns:
//    CloudUploadDlogDataT: On success, it returns a pointer to
//    CloudUploadDlogDataT. NULL: Failed to retriev.
CloudUploadDlogDataT *EsfLogManagerGetCloudTailListData(void);

// """Delete the last data entry registered in the local upload list.
// Args:
//    position(UnregisterPositionT): Unregister Position
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination.
EsfLogManagerStatus EsfLogManagerUnregisterLocalListData(
    UnregisterPositionT position);

// """Delete the last data entry registered in the cloud upload list.
// Args:
//    position(UnregisterPositionT): Unregister Position
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination.
EsfLogManagerStatus EsfLogManagerUnregisterCloudListData(
    UnregisterPositionT position);

// """Delete all data entries registered in the local upload list.
// Args:
//    no arguments
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination.
EsfLogManagerStatus EsfLogManagerDeleteLocalUploadList(void);

// """Delete all data entries registered in the cloud upload list.
// Args:
//    no arguments
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination.
EsfLogManagerStatus EsfLogManagerDeleteCloudUploadList(void);

#endif  // ESF_LOG_MANAGER_LOG_MANAGER_LIST_H_
