/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "log_manager_internal.h"

#include <ctype.h>
#include <malloc.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>  // for clock_gettime, CLOCK_REALTIME
#include <unistd.h>

#ifdef __NuttX__
#include <sys/queue.h>
#else
#include <bsd/sys/queue.h>
#endif

#include "bytebuffer/bytebuffer.h"
#include "json.h"
#include "system_manager.h"

#ifdef LOG_MANAGER_EVP_ENABLE
#include "evp_agent/sdk_backdoor.h"
#else /* LOG_MANAGER_EVP_ENABLE */
#include <evp/sdk_sys.h>
#include <sdk_backdoor.h>
#include <sys/sys.h>

#include "log_manager_stub.h"
#endif /* LOG_MANAGER_EVP_ENABLE */

#ifdef LOG_MANAGER_ENCRYPT_ENABLE
#include "sec_util_public.h"
#endif  // LOG_MANAGER_ENCRYPT_ENABLE
#ifdef LOG_MANAGER_LOCAL_BUILD
#include "dummy_utility_msg.h"
#else
#include "utility_msg.h"
#endif

#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
#include "log_manager_list.h"
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

#include "log_manager_setting.h"
#include "pl_log_manager.h"

#ifndef LOG_MANAGER_EVP_ENABLE
#define SYS_process_event stub_SYS_process_event
#define SYS_put_blob stub_SYS_put_blob
#define SYS_put_blob_mstp stub_SYS_put_blob_mstp
#define EVP_Agent_register_sys_client stub_EVP_Agent_register_sys_client
#define EVP_Agent_unregister_sys_client stub_EVP_Agent_unregister_sys_client
#define SYS_send_telemetry stub_SYS_send_telemetry
#define EVP_getAgentStatus stub_EVP_getAgentStatus
#endif /* LOG_MANAGER_EVP_ENABLE */

/****************************************************************************
 * private Data
 ****************************************************************************/
// Dlog thread stack size.
#define LOG_MANAGER_DLOG_THREAD_STACK_SIZE \
  ((size_t)CONFIG_EXTERNAL_LOG_MANAGER_DLOG_THREAD_STACK_SIZE)
// Event log buffer size.
#define ELOG_SIZE_OF_RAM_BUFFER_PLANE \
  ((size_t)CONFIG_EXTERNAL_LOG_MANAGER_ELOG_SIZE_OF_BUF)
// Event log buffer num.
#define ELOG_NUM_OF_RAM_BUFFER_PLANES \
  ((size_t)CONFIG_EXTERNAL_LOG_MANAGER_ELOG_NUM_OF_BUF)
// Debug log buffer size.
#define DLOG_SIZE_OF_RAM_BUFFER_PLANE \
  ((size_t)CONFIG_EXTERNAL_LOG_MANAGER_DLOG_SIZE_OF_BUF)
// Debug log buffer num.
#define DLOG_NUM_OF_RAM_BUFFER_PLANES \
  ((size_t)CONFIG_EXTERNAL_LOG_MANAGER_DLOG_NUM_OF_BUF)
// Maximum capacity of local list
#define DLOG_NUM_OF_LOCAL_LIST_MAX_NUM \
  ((size_t)CONFIG_EXTERNAL_LOG_MANAGER_LOCAL_LIST_MAX_NUM)
// Maximum capacity of cloud list
#define DLOG_NUM_OF_CLOUD_LIST_MAX_NUM \
  ((size_t)CONFIG_EXTERNAL_LOG_MANAGER_CLOUD_LIST_MAX_NUM)

// Security AES block size
#define LOG_MANAGER_INTERNAL_SEC_AES_BLOCK_SIZE (16)
// Elog thread stack size
#define LOG_MANAGER_ELOG_THREAD_STACK_SIZE \
  ((size_t)CONFIG_EXTERNAL_LOG_MANAGER_ELOG_THREAD_STACK_SIZE)
// Blob thread stack size.
#define LOG_MANAGER_INTERNAL_BLOB_THREAD_STACK_SIZE ((size_t)4096)
// Utility msg receive time out (milliseconds)
#define LOG_MANAGER_INTERNAL_DLOG_MSG_TIMEOUT (1000)
// Utility msg receive time out (milliseconds)
#define LOG_MANAGER_INTERNAL_ELOG_MSG_TIMEOUT (-1)
// Allocate msg queue for Elog client register
#define LOG_MANAGER_INTERNAL_REGISTER_MSG_MAX (1)
// Allocate msg queue for Elog thread destroy
#define LOG_MANAGER_INTERNAL_DESTROY_MSG_MAX (1)
// Utility msg queue size
#define LOG_MANAGER_INTERNAL_MSG_QUEUE_SIZE     \
  (10 + LOG_MANAGER_INTERNAL_REGISTER_MSG_MAX + \
   LOG_MANAGER_INTERNAL_DESTROY_MSG_MAX)
// Log output buffer size
#define LOG_MANAGER_INTERNAL_STR_BUFF_SIZE 128
// Blob upload time out
#define LOG_MANAGER_INTERNAL_BLOB_TIMEOUT (1000)
// Blob upload time out
#define LOG_MANAGER_INTERNAL_BLOB_UPLOAD_SIZE 2048
// Elog telemetry topic
#define LOG_MANAGER_INTERNAL_ELOG_TOPIC "event_log"
// Elog max save message num
#define LOG_MANAGER_INTERNAL_ELOG_SAVE_NUM 5
// Elog max queuing message num
#define LOG_MANAGER_INTERNAL_ELOG_QUEUE_NUM 10
// Blob upload filename max length
#define LOG_MANAGER_INTERNAL_DEFAULT_FILENAME_SIZE 22
// Storage name min name size
#define LOG_MANAGER_INTERNAL_STORAGE_NAME_MIN 3
// Storage name max name size
#define LOG_MANAGER_INTERNAL_STORAGE_NAME_MAX 64
// Http protocol name
#define LOG_MANAGER_INTERNAL_HTTP_PROTOCOL_NAME "http://"
// Blob upload retry wait time
#define LOG_MANAGER_INTERNAL_RETRY_UPLOAD_SLEEP_TIME (1)
// Maximum retry count for upload failure
#define LOG_MANAGER_INTERNAL_MAX_RETRY_COUNT (3)

// Processing result value
typedef enum {
  kCmdIsNon,
  kCmdIsFinDlogCollector,
  kCmdIsRamBufferPlaneFull,
  kCmdIsSendBulkDlog,
  kCmdIsBlobUploadStart,
  kCmdIsBlobUploadContinue,
  kCmdIsBlobUploadFinished,
  kCmdIsFinBlobCollector
} LogCollectorCmdsT;

// Elog process value
typedef enum {
  kCmdIsElogNon,
  kCmdIsWait,
  kCmdIsSend,
  kCmdIsResend,
  kCmdIsRegister,
  kCmdIsDestroyElogThread
} ElogCmdsT;

// Log buffer status
typedef enum {
  kBufferStatusUse,
  kBufferStatusHalfUse,
} LogBufferStatus;

// Blob upload status
typedef enum { kUploadStart, kUploadContinue, kUploadFinish } BlobUploadStatusT;

// Upload type
typedef enum { kCloudUpload, kLocalUpload } UploadTypeT;

// Message control structure
struct MessagePassingT {
  bool m_is_handle_alive;
  int32_t m_handle;
  int32_t m_msg_prio;
  uint32_t m_max_msg_size;
  uint32_t m_queue_size;
  uint32_t *m_msg_buffer;
};

// Message object structure
struct MessagePassingObjT {
  LogCollectorCmdsT m_cmd;
  size_t m_len_of_data;
  uint8_t *m_data;
  size_t m_data_size;
  size_t m_buf_size;
  EsfLogManagerSettingBlockType m_block_type;
  EsfLogManagerBulkDlogCallback m_callback;
  void *m_user_data;
  bool m_is_critical;  // Critical log flag
};

// Elog Message Object structure
struct MessagePassingElogObjT {
  ElogCmdsT m_cmd;
  size_t m_len_of_data;
  char *message;
};

// Atomic values
struct AtomicUint32T {
  pthread_mutex_t m_mutex;
  uint32_t m_value;
};

// Byte buffer object structure
struct ByteBufferMainT {
  uint32_t *m_allocated_memory;
  ByteBuffer_Handle m_handle;
  bool is_critical;
};

// Log setting structure
struct LockForParameterValueT {
  pthread_mutex_t m_mutex;
  EsfLogManagerParameterValue m_value[ESF_LOG_MANAGER_BLOCK_TYPE_MAX_NUM];
};

// Blob update status
struct BlogUploadStateT {
  pthread_mutex_t m_mutex;
  BlobUploadStatusT m_state;
};

// Defining groups and id
struct GroupValueT {
  uint32_t m_module_id;
  uint32_t m_group_id;
};

// Configuration information required during upload
struct UploadNameObjT {
  char *storage_name;
  char *storage_path;
};

// Upload information structure for each group
struct UploadNameGroupObjT {
  pthread_mutex_t m_mutex;
  struct UploadNameObjT upload_group[ESF_LOG_MANAGER_BLOCK_TYPE_MAX_NUM];
};

typedef uint32_t BitValueT;

// Association of group and id
const struct GroupValueT s_grouping_lockup_table[] = {
    {0x00000001, kEsfLogManagerBlockTypeSysApp},  // MODULE_ID_SYSTEM_APP
    {0x00000002,
     kEsfLogManagerBlockTypeSysApp},  // MODULE_ID_INITIAL_SETTING_APP
    {0x00000004, kEsfLogManagerBlockTypeSysApp},  // MODULE_ID_EVP_RUNTIME
    {0x00000008,
     kEsfLogManagerBlockTypeSysApp},  // MODULE_ID_SENSOR_AI_LIB //
                                      // TODO: Temporarily changed
                                      // group. It true group is Sensor.
    {0x00000010,
     kEsfLogManagerBlockTypeSysApp},  // MODULE_ID_SENS_CORD // TODO:
                                      // Temporarily changed group. It true
                                      // group is Sensor.
    {0x00000020,
     kEsfLogManagerBlockTypeSysApp},  // MODULE_ID_AI_ISP_VIC_APP //
                                      // TODO: Temporarily changed
                                      // group. It true group is Sensor.
    {0x00000040,
     kEsfLogManagerBlockTypeSysApp},  // MODULE_ID_AI_ISP_FW // TODO:
                                      // Temporarily changed group. It true
                                      // group is Sensor.
    {0x00000080, kEsfLogManagerBlockTypeSysApp},  // MODULE_ID_ESF_MAIN
    {0x00000100, kEsfLogManagerBlockTypeSysApp},  // MODULE_ID_BUTTON_MANAGER
    {0x00000200, kEsfLogManagerBlockTypeSysApp},  // MODULE_ID_CLOCK_MANAGER
    {0x00000400, kEsfLogManagerBlockTypeSysApp},  // MODULE_ID_FIRMWARE_MANAGER
    {0x00000800, kEsfLogManagerBlockTypeSysApp},  // MODULE_ID_LED_MANAGER
    {0x00001000, kEsfLogManagerBlockTypeSysApp},  // MODULE_ID_LOG_MANAGER
    {0x00002000, kEsfLogManagerBlockTypeSysApp},  // MODULE_ID_MEMORY_MANAGER
    {0x00004000, kEsfLogManagerBlockTypeSysApp},  // MODULE_ID_NETWORK_MANAGER
    {0x00008000,
     kEsfLogManagerBlockTypeSysApp},  // MODULE_ID_PARAMETER_STORAGE_MANAGER
    {0x00010000, kEsfLogManagerBlockTypeSysApp},  // MODULE_ID_POWER_MANAGER
    {0x00020000, kEsfLogManagerBlockTypeSysApp},  // MODULE_ID_SYSTEM_MANAGER
    {0x00100000, kEsfLogManagerBlockTypeSysApp},  // MODULE_ID_SYSTEM_APP
    {0x00200000, kEsfLogManagerBlockTypeSensor},  // MODULE_ID_SENSOR
    {0x00400000, kEsfLogManagerBlockTypeAiisp},   // MODULE_ID_AIISP
    {0x00800000, kEsfLogManagerBlockTypeVicapp},  // MODULE_ID_VICAPP
    {0x80000000, kEsfLogManagerBlockTypeSysApp},  // MODULE_ID_OTHER
};

/****************************************************************************
 * Static variables
 ****************************************************************************/
// PSM variables
static struct LockForParameterValueT s_parameter = {
    .m_mutex = PTHREAD_MUTEX_INITIALIZER,
    .m_value = {{.dlog_dest = kEsfLogManagerDlogDestUart,
                 .dlog_level = kEsfLogManagerDlogLevelInfo,
                 .elog_level = kEsfLogManagerElogLevelInfo,
                 .dlog_filter = 0,
                 .storage_name = {'\0'},
                 .storage_path = {'\0'}},
                {.dlog_dest = kEsfLogManagerDlogDestUart,
                 .dlog_level = kEsfLogManagerDlogLevelInfo,
                 .elog_level = kEsfLogManagerElogLevelInfo,
                 .dlog_filter = 0,
                 .storage_name = {'\0'},
                 .storage_path = {'\0'}},
                {.dlog_dest = kEsfLogManagerDlogDestUart,
                 .dlog_level = kEsfLogManagerDlogLevelInfo,
                 .elog_level = kEsfLogManagerElogLevelInfo,
                 .dlog_filter = 0,
                 .storage_name = {'\0'},
                 .storage_path = {'\0'}},
                {.dlog_dest = kEsfLogManagerDlogDestUart,
                 .dlog_level = kEsfLogManagerDlogLevelInfo,
                 .elog_level = kEsfLogManagerElogLevelInfo,
                 .dlog_filter = 0,
                 .storage_name = {'\0'},
                 .storage_path = {'\0'}}}};

static EsfSystemManagerHwInfo s_hw_info = {0};

/****************************************************************************
 * Bytebuffer static variables
 ****************************************************************************/
#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
// Since it is updated by the Dlog thread and only referenced by the Blob
// thread, mutual exclusion is not necessary.
STATIC LogBufferStatus s_is_dlog_buffer_status = kBufferStatusUse;

STATIC pthread_mutex_t s_mutex_for_bytebuffer = PTHREAD_MUTEX_INITIALIZER;

/* --- Start of s_mutex_for_bytebuffer scope --- */
static uint8_t s_dlog_buff_idx = (uint8_t)0;
static uint8_t s_dlog_oldest_buff_idx = (uint8_t)0;

static size_t s_dlog_store_of_bytebuffer[DLOG_NUM_OF_RAM_BUFFER_PLANES] = {0};
static size_t s_dlog_store_of_temp_bytebuffer = 0;
static struct ByteBufferMainT
    s_dlog_buffer_handles[DLOG_NUM_OF_RAM_BUFFER_PLANES] = {
        {.m_allocated_memory = NULL,
         .m_handle = (ByteBuffer_Handle)NULL,
         .is_critical = false}};
/* --- End of s_mutex_for_bytebuffer scope --- */
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

/****************************************************************************
 * Dlog static variables
 ****************************************************************************/
#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
STATIC struct MessagePassingT s_dlog_msg_passing = {
    .m_is_handle_alive = false,
    .m_handle = (int32_t)-1,
    .m_msg_prio = (int32_t)0,
    .m_max_msg_size = (uint32_t)0,
    .m_queue_size = (uint32_t)0,
    .m_msg_buffer = NULL};

// thread
STATIC pthread_t s_dlog_thread_tid = ESF_LOG_MANAGER_INTERNAL_INVALID_THREAD_ID;
STATIC void *s_dlog_thread_stack = (void *)NULL;
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

/****************************************************************************
 * Blob static variables
 ****************************************************************************/
#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
// thread
static pthread_t s_blob_thread_tid = ESF_LOG_MANAGER_INTERNAL_INVALID_THREAD_ID;
STATIC void *s_blob_thread_stack = (void *)NULL;

STATIC struct SYS_client *s_dlog_sys_client = NULL;

// flag for determining the termination of a blob thread
STATIC bool s_blob_thread_fin_flag = false;

// Critical log management variables
STATIC pthread_mutex_t s_critical_log_mutex = PTHREAD_MUTEX_INITIALIZER;
STATIC struct timespec s_last_critical_log_time = {0, 0};
STATIC bool s_critical_log_pending = false;

// configuration information for local upload
STATIC struct UploadNameGroupObjT s_local_upload_info = {
    .m_mutex = PTHREAD_MUTEX_INITIALIZER,
    .upload_group = {{.storage_name = NULL, .storage_path = NULL},
                     {.storage_name = NULL, .storage_path = NULL},
                     {.storage_name = NULL, .storage_path = NULL},
                     {.storage_name = NULL, .storage_path = NULL}}};
// configuration information for cloud upload
STATIC struct UploadNameGroupObjT s_cloud_upload_info = {
    .m_mutex = PTHREAD_MUTEX_INITIALIZER,
    .upload_group = {{.storage_name = NULL, .storage_path = NULL},
                     {.storage_name = NULL, .storage_path = NULL},
                     {.storage_name = NULL, .storage_path = NULL},
                     {.storage_name = NULL, .storage_path = NULL}}};
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE
/****************************************************************************
 * Elog static variables
 ****************************************************************************/
STATIC struct MessagePassingT s_elog_msg_passing = {
    .m_is_handle_alive = false,
    .m_handle = (int32_t)-1,
    .m_max_msg_size = (uint32_t)0,
    .m_queue_size = (uint32_t)0,
    .m_msg_buffer = NULL};

// thread
static pthread_t s_elog_thread_tid = ESF_LOG_MANAGER_INTERNAL_INVALID_THREAD_ID;
STATIC void *s_elog_thread_stack = (void *)NULL;

STATIC struct SYS_client *s_elog_sys_client = NULL;

STATIC pthread_mutex_t sp_elog_mutex = PTHREAD_MUTEX_INITIALIZER;
/* --- Start of sp_elog_mutex scope --- */
STATIC int32_t s_elog_queue_cnt = 0;
/* --- End of sp_elog_mutex scope --- */

STATIC pthread_mutex_t s_elog_save_message_mutex = PTHREAD_MUTEX_INITIALIZER;

/* --- Start of s_elog_save_message_mutex scope --- */
static char *s_elog_save_message[LOG_MANAGER_INTERNAL_ELOG_SAVE_NUM];
STATIC int32_t s_elog_save_message_cnt = 0;
/* --- End of s_elog_save_message_mutex scope --- */

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
// """ Thread that processes Dlogs
// Args:
//    *p(void): Data to pass to the Dlog thread
// Returns:
//    no return
STATIC void *EsfLogManagerInternalWriteDlogThread(void *p);

// """ Thread that processes Dlogs
// Args:
//    *p(void): Data to pass to the Blob thread
// Returns:
//    no return
STATIC void *EsfLogManagerInternalSendBlobThread(void *p);

// """ Create thread for Dlog
// Args:
//    no arguments
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination.
STATIC EsfLogManagerStatus EsfLogManagerInternalCreateDlogCollectorThread(void);

// """ Create thread for Blob
// Args:
//    no arguments
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination.
STATIC EsfLogManagerStatus EsfLogManagerInternalCreateBlobCollectorThread(void);

// """ Send a message to the Dlog thread.
// Args:
//    *msg_obj(const struct MessagePassingObjT): Message structure to notify
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination.
STATIC EsfLogManagerStatus EsfLogManagerInternalSendCmdToDlogCollectorThread(
    const struct MessagePassingObjT *const msg_obj);

// """ Message processing to notify when the Dlog buffer is full
// Args:
//    no arguments
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination.
STATIC EsfLogManagerStatus EsfLogManagerInternalDlogFullCollector(void);

// """ Dlog thread termination process
// Args:
//    no arguments
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination.
STATIC EsfLogManagerStatus EsfLogManagerInternalDestroyDlogCollector(void);

// """ Blob thread termination process
// Args:
//    no arguments
// Returns:
//    no return
STATIC void EsfLogManagerInternalDestroyBlobCollector(void);

// """ Dlog buffer index switching process
// Args:
//    no arguments
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination.
STATIC EsfLogManagerStatus EsfLogManagerInternalChangeDlogByteBuffer(void);

// """ The process of notifying DeviceControlService with Dlog data
// Args:
//    data_size(size_t): data size
//    buf_size(size_t): buf size
//    *data(uint8_t):data address
//    *out_size(size_t):encrypt after size
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination.
STATIC EsfLogManagerStatus EsfLogManagerInternalCreateEncryptData(
    size_t data_size, size_t buf_size, uint8_t *data, size_t *out_size);

// """ Create a temporary buffer and copy the data that becomes full.
// Args:
//    size(size_t):real data size
//    size(buf_size):encrypted data size
//    *data(uint8_t):real data
//    **out_data(uint8_t):encryption processing buffer
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination.
STATIC EsfLogManagerStatus EsfLogManagerInternalBackupBuffer(
    size_t size, size_t buf_size, uint8_t *data, uint8_t **out_data);

// """ Handle critical log upload timing
// Args:
//    no arguments
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination.
STATIC EsfLogManagerStatus EsfLogManagerInternalHandleCriticalLogTiming(void);

#ifdef LOG_MANAGER_ENCRYPT_ENABLE
// """ Perform the encryption process on the specified buffer.
// Args:
//    data_size(size_t): Encryption data size
//    buf_size(size_t): Encryption buffer size
//    *data(uint8_t): Encryption data
//    *out_size(uint32_t): Size of Data After Encryption
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination.
STATIC EsfLogManagerStatus EsfLogManagerInternalEncryptLog(size_t data_size,
                                                           size_t buf_size,
                                                           uint8_t *data,
                                                           uint32_t *out_size);
#endif /* LOG_MANAGER_ENCRYPT_ENABLE */

// """ Checks whether the configuration parameters are OK.
// Args:
//    block_type(EsfLogManagerSettingBlockType const): block type
//    value(sfLogManagerParameterValue const): check value
//    mask(EsfLogManagerParameterMask const): check mask
// Returns:
//    true: success
//    false: ng
STATIC bool EsfLogManagerCheckParameters(
    EsfLogManagerSettingBlockType const block_type,
    EsfLogManagerParameterValue const *value,
    EsfLogManagerParameterMask const *mask);
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

// """ Check if the upload setting is specified as Local.
// Args:
//    *name(const char): the storage_name specified in SetParameter
// Returns:
//    true: local upload
//    false: cloud upload
STATIC bool EsfLogManagerInternalJudgeLocalUpload(const char *name);

#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
// """ Allocate memory for the globally declared s_local_upload_info and the
// members within s_local_upload_info, based on the block type and mask
// specified by the arguments. Args:
//    block_type(EsfLogManagerSettingBlockType const): block type to allocate
//    *mask(EsfLogManagerParameterMask const): mask type to allocate
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination.
STATIC EsfLogManagerStatus EsfLogManagerInternalAllocateGlobalUploadData(void);

// """ Release the memory allocated for upload information.
// Args:
//    no arguments
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination.
STATIC EsfLogManagerStatus
EsfLogManagerInternalDeallocateGlobalUploadData(void);

// """ Set the storage_name for the block type specified by the arguments.
// Args:
//    upload_type(UploadTypeT) : upload type
//    block_type(EsfLogManagerSettingBlockType const): block type to set values
//    *str(char const): set string
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination.
STATIC EsfLogManagerStatus
EsfLogManagerInternalSetGlobalUploadDataForStorageName(
    UploadTypeT upload_type, EsfLogManagerSettingBlockType const block_type,
    char const *str);

// """ Set the storage_path for the block type specified by the arguments.
// Args:
//    block_type(EsfLogManagerSettingBlockType const): block type to set values
//    *str(char const): set string
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination.
STATIC EsfLogManagerStatus
EsfLogManagerInternalSetGlobalUploadDataForStoragePath(
    EsfLogManagerSettingBlockType const block_type, char const *str);

#endif  // CONFIG_EXTERNAL_DLOG_DISABLE
// """ Thread that processes Elog
// Args:
//    no arguments
// Returns:
//    no return
STATIC void *EsfLogWriteElogThread(void *p __attribute__((unused)));

// """ Create thread for Elog
// Args:
//    no arguments
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination.
STATIC EsfLogManagerStatus EsfLogManagerInternalCreateElogThread(void);

// """ Send a message to the Elog thread.
// Args:
//    *msg_obj(const struct MessagePassingElogObjT): Message structure
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination.
STATIC EsfLogManagerStatus EsfLogManagerInternalSendCmdToElogThread(
    const struct MessagePassingElogObjT *const msg_obj);

// """ Send a message to the Elog thread.
//     This function can use allocated queue space.
// Args:
//    *msg_obj(const struct MessagePassingElogObjT): Message structure
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination.
STATIC EsfLogManagerStatus EsfLogManagerInternalSendCmdForceToElogThread(
    const struct MessagePassingElogObjT *const msg_obj);

// """ Elog thread termination process
// Args:
//    no arguments
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination.
static EsfLogManagerStatus EsfLogManagerInternalDestroyElogThread(void);

// """ Callback of send_telemetry()
// Args:
//    c(struct SYS_client*): EVP Agent Client
//    reason(enum SYS_callback_reason): Telemetry result
//    user(void *): User data
// Returns:
//    no return
STATIC void EsfLogManagerInternalElogCallback(struct SYS_client *client,
                                              enum SYS_callback_reason reason,
                                              void *user);

// """ Convert Elog message to json string
// Args :
//    *msg(struct MessagePassingElogObjT*): Elog message from UtilityLog
//    **pstr (const char**): Json String
// Returns:
//    kEsfLogManagerStatusOk: success.
//    kEsfLogManagerStatusFailed: abnormal termination.
STATIC EsfLogManagerStatus EsfLogManagerInternalSerializeElogJson(
    const EsfLogManagerElogMessage *message, char **pstr);

// """ Save Elog Message
// Args:
//    *pstr(const char): Elog Json Message
// Returns:
//    kEsfLogManagerStatusOk: success.
//    kEsfLogManagerStatusFailed: abnormal termination.
STATIC EsfLogManagerStatus EsfLogManagerInternalSaveElog(const char *pstr);

// """ Load Elog Message
// Args:
//    **pstr(char): Elog Json Message
// Returns:
//    kEsfLogManagerStatusOk: success.
//    kEsfLogManagerStatusFailed: abnormal termination.
STATIC EsfLogManagerStatus EsfLogManagerInternalLoadElog(char **pstr);

// """ Clears the internally held value.
// Args:
//    no arguments
// Returns:
//    no return
STATIC void EsfLogManagerInternalSetupCleaning(void);
#ifndef CONFIG_EXTERNAL_DLOG_DISABLE

// """ Create the file path to be created in the blob destination.
// Args:
//    upload_type(UploadTypeT):cloud or local
//    block_type(EsfLogManagerSettingBlockType):use block type
//    ts(const struct timespec): date and time of file creation
//    **file_name(char): create file name
//    *name_len(uint32_t): create file name length
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination.
STATIC EsfLogManagerStatus EsfLogManagerInternalCreateFileName(
    UploadTypeT upload_type, EsfLogManagerSettingBlockType block_type,
    const struct timespec ts, char **file_name, uint32_t *name_len);
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

// """ Naming rule check for storage path.
// Args:
//    str(const char*):storage path
// Returns:
//    true: no issues
//    false: naming violation for container name.
static bool EsfLogManagerCheckStoragePath(const char *str);

// """ Naming rule check for storage name.
// Args:
//    block_type(EsfLogManagerSettingBlockType): block type
//    str(const char*):storage name
// Returns:
//    true: no issues
//    false: naming violation for folder name.
STATIC bool EsfLogManagerCheckStorageName(
    EsfLogManagerSettingBlockType block_type, const char *str);

#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
// """ Determine the need for encryption.
// Args:
//    block_type(EsfLogManagerSettingBlockType):block type
//    local_upload(bool):false(cloud) true(local)
// Returns:
//    true: Encryption necessary
//    false: No encryption necessary
STATIC bool EsfLogManagerInternalJudgeEncrypt(
    EsfLogManagerSettingBlockType block_type, bool local_upload);

// """ callback processing for local upload.
// Args:
//    *client(struct SYS_client):sys client
//    *blob(struct SYS_blob_data):sys blob data struct
// Returns:
//    SYS_RESULT_OK: success
STATIC enum SYS_result EsfLogManagerInternalLocalUploadCallback(
    struct SYS_client *client, struct SYS_blob_data *blob,
    enum SYS_callback_reason reason, void *user);

// """ callback processing for cloud upload.
// Args:
//    *client(struct SYS_client):sys client
//    *blob(struct SYS_blob_data):sys blob data struct
// Returns:
//    SYS_RESULT_OK: success
STATIC enum SYS_result EsfLogManagerInternalCloudUploadCallback(
    struct SYS_client *client, struct SYS_blob_data *blob,
    enum SYS_callback_reason reason, void *user);

// """ Handle upload retry logic for failed uploads.
// Args:
//    *data(void): Upload data structure (LocalUploadDlogDataT or
//    CloudUploadDlogDataT) is_local(bool): true for local upload, false for
//    cloud upload
// Returns:
//    no return
STATIC void EsfLogManagerInternalHandleUploadRetry(void *data, bool is_local);

// """ Create filenames for upload and perform local upload processing.
// Args:
//    no arguments
// Returns:
//    kEsfLogManagerStatusOk: success.
//    kEsfLogManagerStatusFailed: abnormal termination.
STATIC EsfLogManagerStatus EsfLogManagerInternalLocalUpload(void);

// """ Create filenames for upload and perform cloud upload processing.
// Args:
//    no arguments
// Returns:
//    kEsfLogManagerStatusOk: success.
//    kEsfLogManagerStatusFailed: abnormal termination.
STATIC EsfLogManagerStatus EsfLogManagerInternalCloudUpload(void);

// """ Checks the validity of arguments passed to the blob upload callback.
// Args:
//    client(struct SYS_client*): Dlog client
//    blob(struct SYS_blob_data*): Blob data
//    user(void*): User data
// Returns:
//    true: All arguments are valid
//    false: Any argument is invalid or an error state
STATIC bool EsfLogManagerInternalCheckBlobCallbackArguments(
    struct SYS_client *client, struct SYS_blob_data *blob, void *user);
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
STATIC PlLogManagerBlockType EsfLogManagerInternalCovertBlockTypeToPl(
    EsfLogManagerSettingBlockType esf_block_type) {
  PlLogManagerBlockType pl_block_type = kPlLogManagerBlockTypeMain;

  switch (esf_block_type) {
    case kEsfLogManagerBlockTypeSysApp:
      pl_block_type = kPlLogManagerBlockTypeMain;
      break;

    case kEsfLogManagerBlockTypeSensor:
      pl_block_type = kPlLogManagerBlockTypeSensor;
      break;

    case kEsfLogManagerBlockTypeAiisp:
      pl_block_type = kPlLogManagerBlockTypeCompanionFw;
      break;

    case kEsfLogManagerBlockTypeVicapp:
      pl_block_type = kPlLogManagerBlockTypeCompanionApp;
      break;

    case kEsfLogManagerBlockTypeAll:
      pl_block_type = kPlLogManagerBlockTypeAll;
      break;

    case kEsfLogManagerBlockTypeNum:
      pl_block_type = kPlLogManagerBlockTypeNum;
      break;

    default:
      pl_block_type = kPlLogManagerBlockTypeMain;
      break;
  }

  return pl_block_type;
}
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

STATIC EsfLogManagerSettingBlockType
EsfLogManagerInternalCovertBlockTypeToEsf(PlLogManagerBlockType pl_block_type) {
  EsfLogManagerSettingBlockType esf_block_type = kEsfLogManagerBlockTypeSysApp;

  switch (pl_block_type) {
    case kPlLogManagerBlockTypeMain:
      esf_block_type = kEsfLogManagerBlockTypeSysApp;
      break;

    case kPlLogManagerBlockTypeSensor:
      esf_block_type = kEsfLogManagerBlockTypeSensor;
      break;

    case kPlLogManagerBlockTypeCompanionFw:
      esf_block_type = kEsfLogManagerBlockTypeAiisp;
      break;

    case kPlLogManagerBlockTypeCompanionApp:
      esf_block_type = kEsfLogManagerBlockTypeVicapp;
      break;

    case kPlLogManagerBlockTypeAll:
      esf_block_type = kEsfLogManagerBlockTypeAll;
      break;

    case kPlLogManagerBlockTypeNum:
      esf_block_type = kEsfLogManagerBlockTypeNum;
      break;

    default:
      esf_block_type = kEsfLogManagerBlockTypeSysApp;
      break;
  }

  return esf_block_type;
}
#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
STATIC bool EsfLogManagerInternalCheckBlobCallbackArguments(
    struct SYS_client *client, struct SYS_blob_data *blob, void *user) {
  if (client != s_dlog_sys_client) {
    ESF_LOG_MANAGER_ERROR("Blob callback client is not dlog client.\n");
    return false;
  }

  if (user == NULL) {
    ESF_LOG_MANAGER_ERROR("Blob callback user is NULL.\n");
    return false;
  }

  if (blob == NULL) {
    ESF_LOG_MANAGER_ERROR("Blob callback blob is NULL.\n");
    return false;
  }

  if (blob->error != 0) {
    ESF_LOG_MANAGER_ERROR("Blob callback error. blob=%p error=%d\n", blob,
                          blob->error);
    return false;
  }

  if ((blob->status_code != 0) &&
      ((blob->status_code < 200) || (blob->status_code >= 300))) {
    ESF_LOG_MANAGER_ERROR("Blob callback error. blob=%p status_code=%d\n", blob,
                          blob->status_code);
    return false;
  }

  return true;
}
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
STATIC void EsfLogManagerInternalHandleUploadRetry(void *data, bool is_local) {
  const char *log_type = is_local ? "Local" : "Cloud";

  if (data != NULL) {
    // Both LocalUploadDlogDataT and CloudUploadDlogDataT have the same
    // structure (UploadDlogData) so we can safely cast to access m_retry_count
    CloudUploadDlogDataT *upload_data = (CloudUploadDlogDataT *)data;
    upload_data->m_retry_count++;

    if (upload_data->m_retry_count < LOG_MANAGER_INTERNAL_MAX_RETRY_COUNT) {
      ESF_LOG_MANAGER_ERROR("%s upload failed, retrying (%d/%d)\n", log_type,
                            upload_data->m_retry_count,
                            LOG_MANAGER_INTERNAL_MAX_RETRY_COUNT);
      if (is_local) {
        EsfLogManagerSetLocalUploadStatus(kUploadStatusRequest);
      } else {
        EsfLogManagerSetCloudUploadStatus(kUploadStatusRequest);
      }
    } else {
      ESF_LOG_MANAGER_ERROR(
          "%s upload failed after %d retries, discarding data\n", log_type,
          LOG_MANAGER_INTERNAL_MAX_RETRY_COUNT);
      if (is_local) {
        EsfLogManagerSetLocalUploadStatus(kUploadStatusFinished);
      } else {
        EsfLogManagerSetCloudUploadStatus(kUploadStatusFinished);
      }
    }
  } else {
    if (is_local) {
      EsfLogManagerSetLocalUploadStatus(kUploadStatusRequest);
    } else {
      EsfLogManagerSetCloudUploadStatus(kUploadStatusRequest);
    }
  }
}
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

STATIC void EsfLogManagerInternalSetupCleaning(void) {
  UtilityMsgErrCode utility_ret = kUtilityMsgOk;
#ifndef CONFIG_EXTERNAL_DLOG_DISABLE

  if (s_dlog_thread_stack != NULL) {
    EsfLogManagerInternalDestroyDlogCollector();
    free(s_dlog_thread_stack);
    s_dlog_thread_stack = NULL;
  }

  if (s_blob_thread_stack != NULL) {
    EsfLogManagerInternalDestroyBlobCollector();
    free(s_blob_thread_stack);
    s_blob_thread_stack = NULL;
  }
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

  if (s_elog_thread_stack != NULL) {
    EsfLogManagerInternalDestroyElogThread();
    free(s_elog_thread_stack);
    s_elog_thread_stack = NULL;
  }

#ifndef CONFIG_EXTERNAL_DLOG_DISABLE

  if (s_dlog_msg_passing.m_handle != -1) {
    utility_ret = UtilityMsgClose(s_dlog_msg_passing.m_handle);
    if (utility_ret != kUtilityMsgOk) {
      ESF_LOG_MANAGER_ERROR(
          "Failed to UtilityMsgClose. Handle is s_dlog_msg_passing.m_handle. "
          "ret=%d\n",
          utility_ret);
    }
    s_dlog_msg_passing.m_handle = -1;
  }

#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

  if (s_elog_msg_passing.m_handle != -1) {
    utility_ret = UtilityMsgClose(s_elog_msg_passing.m_handle);
    if (utility_ret != kUtilityMsgOk) {
      ESF_LOG_MANAGER_ERROR(
          "Failed to UtilityMsgClose. Handle is s_elog_msg_passing.m_handle. "
          "ret=%d\n",
          utility_ret);
    }
    s_elog_msg_passing.m_handle = -1;

    {
      if (pthread_mutex_lock(&sp_elog_mutex) != 0) {
        ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_lock.\n");
      }

      s_elog_queue_cnt = 0;

      if (pthread_mutex_unlock(&sp_elog_mutex) != 0) {
        ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_unlock.\n");
      }
    }
  }
}

#ifndef CONFIG_EXTERNAL_DLOG_DISABLE

STATIC bool EsfLogManagerInternalJudgeEncrypt(
    EsfLogManagerSettingBlockType block_type, bool local_upload) {
  if (block_type == kEsfLogManagerBlockTypeVicapp) {
    return false;
  }

  if (local_upload == true) {
    return false;
  }

  return true;
}

STATIC void *EsfLogManagerInternalWriteDlogThread(void *p
                                                  __attribute__((unused))) {
  struct MessagePassingObjT msg = {
      .m_cmd = kCmdIsNon,
      .m_len_of_data = (size_t)(sizeof(struct MessagePassingObjT)),
      .m_data = NULL,
      .m_data_size = 0,
      .m_buf_size = 0,
      .m_block_type = kEsfLogManagerBlockTypeNum,
      .m_callback = NULL,
      .m_user_data = NULL};
  int32_t recv_size = 0;
  int ret = 0;

  if (NULL != p) {
    LOG_MANAGER_TRACE_PRINT(":%p\n", p);
  }

  for (;;) {
    UtilityMsgErrCode utility_ret = UtilityMsgRecv(
        s_dlog_msg_passing.m_handle, (void *)&msg,
        sizeof(struct MessagePassingObjT),
        LOG_MANAGER_INTERNAL_DLOG_MSG_TIMEOUT, (int32_t *)&recv_size);
    if (utility_ret != kUtilityMsgOk) {
      if (utility_ret == kUtilityMsgErrTimedout) {
        // Check critical log timing for automatic upload
        EsfLogManagerStatus timing_ret =
            EsfLogManagerInternalHandleCriticalLogTiming();
        if (timing_ret != kEsfLogManagerStatusOk) {
          ESF_LOG_MANAGER_ERROR("Failed to handle critical log timing ret=%d\n",
                                timing_ret);
        }
      } else {
        ESF_LOG_MANAGER_ERROR(
            "Failed to UtilityMsgRecv. Handle is s_dlog_msg_passing.m_handle. "
            "ret=%d\n",
            utility_ret);
      }
      continue;
    }

    if (msg.m_cmd == kCmdIsFinDlogCollector) {
      ESF_LOG_MANAGER_DEBUG("EsfLogManagerInternalWriteDlogThread Fin\n");
      break;
    } else if ((msg.m_cmd == kCmdIsRamBufferPlaneFull) ||
               (msg.m_cmd == kCmdIsSendBulkDlog)) {
      uint8_t *dlog_data = NULL;

      if (msg.m_cmd == kCmdIsSendBulkDlog) {
        dlog_data = msg.m_data;
      } else {
        ret = EsfLogManagerInternalBackupBuffer(msg.m_data_size, msg.m_buf_size,
                                                msg.m_data, &dlog_data);
        if (ret != kEsfLogManagerStatusOk) {
          ESF_LOG_MANAGER_ERROR("Failed to backup DLOG buffer. ret=%d\n", ret);
          continue;
        }
      }

      bool local_upload = false;
      if (pthread_mutex_lock(&s_parameter.m_mutex) != 0) {
        ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_lock.\n");
        free(dlog_data);
        continue;
      }

      if (EsfLogManagerInternalJudgeLocalUpload(
              s_parameter.m_value[msg.m_block_type].storage_name) == true) {
        local_upload = true;
      }

      if (pthread_mutex_unlock(&s_parameter.m_mutex) != 0) {
        ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_unlock.\n");
        free(dlog_data);
        continue;
      }

      size_t upload_size = msg.m_data_size;
      if (EsfLogManagerInternalJudgeEncrypt(msg.m_block_type, local_upload) ==
          true) {
        ret = EsfLogManagerInternalCreateEncryptData(
            msg.m_data_size, msg.m_buf_size, (uint8_t *)dlog_data,
            &upload_size);
        if (ret != kEsfLogManagerStatusOk) {
          ESF_LOG_MANAGER_ERROR("Bad Encrypt. ret=%d\n", ret);
          free(dlog_data);
          continue;
        }
      }

      int32_t count = 0;
      if (local_upload == true) {
        /* Check the number of items in the list, and if it has reached the
         * limit, discard it */
        count = EsfLogManagerCheckLocalListNum();
        if ((count < 0) ||
            ((uint32_t)count >= DLOG_NUM_OF_LOCAL_LIST_MAX_NUM)) {
          ESF_LOG_MANAGER_DEBUG("local list num failed num=%d max=%lu\n", count,
                                DLOG_NUM_OF_LOCAL_LIST_MAX_NUM);
          free(dlog_data);
          msg.m_data = NULL;
          continue;
        } else {
          ret = EsfLogManagerRegisterLocalList(msg.m_block_type, msg.m_callback,
                                               msg.m_user_data, upload_size,
                                               dlog_data, msg.m_is_critical);
          if (ret != kEsfLogManagerStatusOk) {
            ESF_LOG_MANAGER_ERROR(
                "Add local list failed block_type=%d upload_size=%lu ret=%d\n",
                msg.m_block_type, upload_size, ret);
            free(dlog_data);
          }
        }
      } else {
        /* Check the number of items in the list, and if it has reached the
         * limit, discard it */
        count = EsfLogManagerCheckCloudListNum();
        if ((count < 0) ||
            ((uint32_t)count >= DLOG_NUM_OF_CLOUD_LIST_MAX_NUM)) {
          ESF_LOG_MANAGER_DEBUG("cloud list num failed num=%d max=%lu\n", count,
                                DLOG_NUM_OF_CLOUD_LIST_MAX_NUM);
          free(dlog_data);
          msg.m_data = NULL;
          continue;
        } else {
          ret = EsfLogManagerRegisterCloudList(msg.m_block_type, msg.m_callback,
                                               msg.m_user_data, upload_size,
                                               dlog_data, msg.m_is_critical);
          if (ret != kEsfLogManagerStatusOk) {
            ESF_LOG_MANAGER_ERROR(
                "Add cloud list failed block_type=%d upload_size=%lu ret=%d\n",
                msg.m_block_type, upload_size, ret);
            free(dlog_data);
          }
        }
      }
    } else {
      ESF_LOG_MANAGER_ERROR("Failed Invalid message command. cmd=%d\n",
                            msg.m_cmd);
    }
    msg.m_cmd = kCmdIsNon;
  }
  LOG_MANAGER_TRACE_PRINT(":Thread exit\n");
#ifndef LOG_MANAGER_LOCAL_BUILD
  pthread_exit((void *)NULL);
#endif
  LOG_MANAGER_TRACE_PRINT(":Thread exit\n");
  return (void *)NULL;
}

STATIC void *EsfLogManagerInternalSendBlobThread(void *p
                                                 __attribute__((unused))) {
  for (;;) {
    if (s_blob_thread_fin_flag) {
      ESF_LOG_MANAGER_DEBUG("EsfLogManagerInternalSendBlobThread Fin\n");
      break;
    }

    if (s_dlog_sys_client == NULL) {
      s_dlog_sys_client = EVP_Agent_register_sys_client();
      if (s_dlog_sys_client == NULL) {
        ESF_LOG_MANAGER_ERROR("Failed to register sys client.\n");
        sleep(LOG_MANAGER_INTERNAL_RETRY_UPLOAD_SLEEP_TIME);
        continue;
      }
    } else {
      EsfLogManagerStatus ret = EsfLogManagerInternalLocalUpload();
      if (ret != kEsfLogManagerStatusOk) {
        ESF_LOG_MANAGER_ERROR("Failed to local upload ret=%d\n", ret);
      }

      ret = EsfLogManagerInternalCloudUpload();
      if (ret != kEsfLogManagerStatusOk) {
        ESF_LOG_MANAGER_ERROR("Failed to cloud upload ret=%d\n", ret);
      }

      LOG_MANAGER_TRACE_PRINT(":Call SYS_process_event\n");
      enum SYS_result result = SYS_process_event(
          s_dlog_sys_client, LOG_MANAGER_INTERNAL_BLOB_TIMEOUT);
      if ((result != SYS_RESULT_OK) && (result != SYS_RESULT_TIMEDOUT)) {
        ESF_LOG_MANAGER_ERROR("SYS_process_event failed %d\n", result);
        if (result == SYS_RESULT_SHOULD_EXIT) {
          EVP_Agent_unregister_sys_client(s_dlog_sys_client);
          s_dlog_sys_client = NULL;
        }
        /* upload restart */
        EsfLogManagerSetLocalUploadStatus(kUploadStatusRequest);
        EsfLogManagerSetCloudUploadStatus(kUploadStatusRequest);
      }
    }
  }
  LOG_MANAGER_TRACE_PRINT(":Thread exit\n");
#ifndef LOG_MANAGER_LOCAL_BUILD
  pthread_exit((void *)NULL);
#endif
  LOG_MANAGER_TRACE_PRINT(":Thread exit\n");

  return (void *)NULL;
}

STATIC EsfLogManagerStatus
EsfLogManagerInternalCreateDlogCollectorThread(void) {
  pthread_attr_t dlog_attr;
  int ret = 0;

  ret = pthread_attr_init(&dlog_attr);
  if (ret != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_attr_init. ret=%d\n", ret);
    return kEsfLogManagerStatusFailed;
  }

#ifndef LOG_MANAGER_LOCAL_BUILD
#ifdef __NuttX__
  s_dlog_thread_stack = malloc(LOG_MANAGER_DLOG_THREAD_STACK_SIZE);
  if (s_dlog_thread_stack == NULL) {
    ESF_LOG_MANAGER_ERROR("allocate memory failed \n");
    return kEsfLogManagerStatusFailed;
  }

  ret = pthread_attr_setstack(&dlog_attr, s_dlog_thread_stack,
                              LOG_MANAGER_DLOG_THREAD_STACK_SIZE);
  if (ret != 0) {
    free(s_dlog_thread_stack);
    s_dlog_thread_stack = NULL;
    ESF_LOG_MANAGER_ERROR("Failed to pthread_attr_setstack. ret=%d\n", ret);
    return kEsfLogManagerStatusFailed;
  }
#endif  // __NuttX__
#endif  // LOG_MANAGER_LOCAL_BUILD

  ret = pthread_create(&s_dlog_thread_tid, &dlog_attr,
                       EsfLogManagerInternalWriteDlogThread, NULL);
  if (ret != 0) {
    free(s_dlog_thread_stack);
    s_dlog_thread_stack = NULL;
    ESF_LOG_MANAGER_ERROR("Failed to pthread_create. ret=%d\n", ret);
    return kEsfLogManagerStatusFailed;
  }

#ifndef LOG_MANAGER_LOCAL_BUILD
  ret = pthread_setname_np(s_dlog_thread_tid, "EsfWriteDlog");
  if (ret != 0) {
    free(s_dlog_thread_stack);
    s_dlog_thread_stack = NULL;
    ESF_LOG_MANAGER_ERROR("Failed to pthread_setname_np. ret=%d\n", ret);
    return kEsfLogManagerStatusFailed;
  }
#endif

  ret = pthread_attr_destroy(&dlog_attr);
  if (ret != 0) {
    free(s_dlog_thread_stack);
    s_dlog_thread_stack = NULL;
    ESF_LOG_MANAGER_ERROR("Failed to pthread_attr_destroy. ret=%d\n", ret);
    return kEsfLogManagerStatusFailed;
  }

  return kEsfLogManagerStatusOk;
}

STATIC EsfLogManagerStatus
EsfLogManagerInternalCreateBlobCollectorThread(void) {
  pthread_attr_t dlog_attr;
  int ret = 0;

  ret = pthread_attr_init(&dlog_attr);
  if (ret != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_attr_init. ret=%d\n", ret);
    return kEsfLogManagerStatusFailed;
  }

#ifndef LOG_MANAGER_LOCAL_BUILD
#ifdef __NuttX__
  s_blob_thread_stack = malloc(LOG_MANAGER_INTERNAL_BLOB_THREAD_STACK_SIZE);
  if (s_blob_thread_stack == NULL) {
    ESF_LOG_MANAGER_ERROR("allocate memory failed \n");
    return kEsfLogManagerStatusFailed;
  }

  ret = pthread_attr_setstack(&dlog_attr, s_blob_thread_stack,
                              LOG_MANAGER_INTERNAL_BLOB_THREAD_STACK_SIZE);
  if (ret != 0) {
    free(s_blob_thread_stack);
    s_blob_thread_stack = NULL;
    ESF_LOG_MANAGER_ERROR("Failed to pthread_attr_setstack. ret=%d\n", ret);
    return kEsfLogManagerStatusFailed;
  }
#endif  // __NuttX__
#endif  // LOG_MANAGER_LOCAL_BUILD

  ret = pthread_create(&s_blob_thread_tid, &dlog_attr,
                       EsfLogManagerInternalSendBlobThread, NULL);
  if (ret != 0) {
    free(s_blob_thread_stack);
    s_blob_thread_stack = NULL;
    ESF_LOG_MANAGER_ERROR("Failed to pthread_create. ret=%d\n", ret);
    return kEsfLogManagerStatusFailed;
  }

#ifndef LOG_MANAGER_LOCAL_BUILD
  ret = pthread_setname_np(s_blob_thread_tid, "EsfSendBlob");
  if (ret != 0) {
    free(s_blob_thread_stack);
    s_blob_thread_stack = NULL;
    ESF_LOG_MANAGER_ERROR("Failed to pthread_setname_np. ret=%d\n", ret);
    return kEsfLogManagerStatusFailed;
  }
#endif

  ret = pthread_attr_destroy(&dlog_attr);
  if (ret != 0) {
    free(s_blob_thread_stack);
    s_blob_thread_stack = NULL;
    ESF_LOG_MANAGER_ERROR("Failed to pthread_attr_destroy. ret=%d\n", ret);
    return kEsfLogManagerStatusFailed;
  }

  return kEsfLogManagerStatusOk;
}

STATIC EsfLogManagerStatus EsfLogManagerInternalSendCmdToDlogCollectorThread(
    const struct MessagePassingObjT *const msg_obj) {
  UtilityMsgErrCode ret = kUtilityMsgOk;
  int32_t send_size = 0;

  ret = UtilityMsgSend(s_dlog_msg_passing.m_handle, (void *)msg_obj,
                       (uint32_t)msg_obj->m_len_of_data,
                       s_dlog_msg_passing.m_msg_prio, (int32_t *)&send_size);
  if (ret != kUtilityMsgOk) {
    ESF_LOG_MANAGER_ERROR(
        "Failed to send message. ret=%d m_cmd=%d m_data=%p m_len_of_data=%lu "
        "m_data_size=%lu m_buf_size=%lu m_block_type=%d \n",
        ret, msg_obj->m_cmd, msg_obj->m_data, msg_obj->m_len_of_data,
        msg_obj->m_data_size, msg_obj->m_buf_size, msg_obj->m_block_type);
    return kEsfLogManagerStatusFailed;
  }

  return kEsfLogManagerStatusOk;
}

STATIC EsfLogManagerStatus EsfLogManagerInternalCreateFileName(
    UploadTypeT upload_type, EsfLogManagerSettingBlockType block_type,
    const struct timespec ts, char **file_name, uint32_t *name_len) {
  struct tm time;
  gmtime_r(&ts.tv_sec, &time);  // Get UTC time.

  char *storage_path = NULL;
  {
    if (upload_type == kCloudUpload) {
      if (pthread_mutex_lock(&s_cloud_upload_info.m_mutex) != 0) {
        ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_lock.\n");
        return kEsfLogManagerStatusFailed;
      }

      storage_path =
          strdup(s_cloud_upload_info.upload_group[block_type].storage_path);

      if (storage_path == NULL) {
        ESF_LOG_MANAGER_ERROR("Failed to strdup.\n");
        (void)pthread_mutex_unlock(&s_cloud_upload_info.m_mutex);
        return kEsfLogManagerStatusFailed;
      }

      if (pthread_mutex_unlock(&s_cloud_upload_info.m_mutex) != 0) {
        ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_unlock.\n");
        free(storage_path);
        return kEsfLogManagerStatusFailed;
      }

    } else {
      if (pthread_mutex_lock(&s_local_upload_info.m_mutex) != 0) {
        ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_lock.\n");
        return kEsfLogManagerStatusFailed;
      }

      storage_path =
          strdup(s_local_upload_info.upload_group[block_type].storage_path);

      if (storage_path == NULL) {
        ESF_LOG_MANAGER_ERROR("Failed to strdup.\n");
        (void)pthread_mutex_unlock(&s_local_upload_info.m_mutex);
        return kEsfLogManagerStatusFailed;
      }

      if (pthread_mutex_unlock(&s_local_upload_info.m_mutex) != 0) {
        ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_unlock.\n");
        free(storage_path);
        return kEsfLogManagerStatusFailed;
      }
    }
  }

  uint32_t len = LOG_MANAGER_INTERNAL_DEFAULT_FILENAME_SIZE;
  if (storage_path[0] == '\0') {
    *file_name = (char *)calloc(len, sizeof(char));
    if (*file_name == NULL) {
      free(storage_path);
      ESF_LOG_MANAGER_ERROR("Failed to calloc.\n");
      return kEsfLogManagerStatusFailed;
    }
    snprintf(*file_name, len, "%.4d%.2d%.2d%.2d%.2d%.2d%.3d.log",
             abs((time.tm_year + 1900)) % 10000, abs((time.tm_mon + 1)) % 100,
             abs(time.tm_mday) % 100, abs(time.tm_hour) % 100,
             abs(time.tm_min) % 100, abs(time.tm_sec) % 100,
             abs((ts.tv_nsec / 1000000)) % 1000);
  } else {
    /* The first +1 accounts for the size of '/', and the second +1 provides a
     * margin (to fix a T3P build error). */
    /* NULL size is included in LOG_MANAGER_INTERNAL_DEFAULT_FILENAME_SIZE */
    len += strlen(storage_path) + 1 + 1;

    *file_name = (char *)calloc(len, sizeof(char));
    if (*file_name == NULL) {
      free(storage_path);
      ESF_LOG_MANAGER_ERROR("Failed to calloc.\n");
      return kEsfLogManagerStatusFailed;
    }
    snprintf(*file_name, len, "%s/%.4d%.2d%.2d%.2d%.2d%.2d%.3d.log",
             storage_path, abs((time.tm_year + 1900)) % 10000,
             abs((time.tm_mon + 1)) % 100, abs(time.tm_mday) % 100,
             abs(time.tm_hour) % 100, abs(time.tm_min) % 100,
             abs(time.tm_sec) % 100, abs((ts.tv_nsec / 1000000)) % 1000);
  }
  LOG_MANAGER_TRACE_PRINT(":Cloud create file name %s\n", *file_name);
  *name_len = len;

  free(storage_path);

  return kEsfLogManagerStatusOk;
}

STATIC enum SYS_result EsfLogManagerInternalLocalUploadCallback(
    struct SYS_client *client, struct SYS_blob_data *blob,
    enum SYS_callback_reason reason, void *user) {
  if (!EsfLogManagerInternalCheckBlobCallbackArguments(client, blob, user)) {
    ESF_LOG_MANAGER_ERROR("%s Invalid Parameter.\n", __func__);
    EsfLogManagerInternalHandleUploadRetry(user, true);
    return SYS_RESULT_OK;
  }

  LOG_MANAGER_TRACE_PRINT(":BlobCallback blob_buffer(%p) reason(%d) len(%zu)\n",
                          blob->blob_buffer, reason, blob->len);

  LocalUploadDlogDataT *data = NULL;
  switch (reason) {
    case SYS_REASON_MORE_DATA:
      data = (LocalUploadDlogDataT *)user;
      if ((blob->blob_buffer != NULL) && (data->m_addr != NULL)) {
        /* Copy the data to the blob buffer */
        char *tmp_buffer = (char *)data->m_addr;
        tmp_buffer += data->m_upload_complete_size;
        memcpy(blob->blob_buffer, tmp_buffer, blob->len);
        data->m_upload_complete_size += blob->len;
      } else {
        ESF_LOG_MANAGER_ERROR("blob_buffer=%p data->m_addr=%p\n",
                              blob->blob_buffer, data->m_addr);
        EsfLogManagerSetLocalUploadStatus(kUploadStatusRequest);
      }
      break;

    case SYS_REASON_FINISHED:
      ESF_LOG_MANAGER_DEBUG("Blob upload finished\n");
      EsfLogManagerSetLocalUploadStatus(kUploadStatusFinished);
      break;

    case SYS_REASON_TIMEOUT:
      /* Do nothing */
      break;

    case SYS_REASON_ERROR:
    default:
      ESF_LOG_MANAGER_ERROR("Local upload failed\n");
      EsfLogManagerInternalHandleUploadRetry(user, true);
      break;
  }

  return SYS_RESULT_OK;
}

STATIC enum SYS_result EsfLogManagerInternalCloudUploadCallback(
    struct SYS_client *client, struct SYS_blob_data *blob,
    enum SYS_callback_reason reason, void *user) {
  EsfLogManagerStatus ret = kEsfLogManagerStatusOk;

  if (!EsfLogManagerInternalCheckBlobCallbackArguments(client, blob, user)) {
    ESF_LOG_MANAGER_ERROR("%s Invalid Parameter.\n", __func__);
    EsfLogManagerInternalHandleUploadRetry(user, false);
    return SYS_RESULT_OK;
  }

  LOG_MANAGER_TRACE_PRINT(":BlobCallback blob_buffer(%p) reason(%d) len(%zu)\n",
                          blob->blob_buffer, reason, blob->len);

  CloudUploadDlogDataT *data = NULL;
  switch (reason) {
    case SYS_REASON_MORE_DATA:
      data = (CloudUploadDlogDataT *)user;
      if ((blob->blob_buffer != NULL) && (data->m_addr != NULL)) {
        /* Copy the data to the blob buffer */
        char *tmp_buffer = (char *)data->m_addr;
        tmp_buffer += data->m_upload_complete_size;
        memcpy(blob->blob_buffer, tmp_buffer, blob->len);
        data->m_upload_complete_size += blob->len;
      } else {
        ESF_LOG_MANAGER_ERROR("blob_buffer=%p data->m_addr=%p\n",
                              blob->blob_buffer, data->m_addr);
        EsfLogManagerSetCloudUploadStatus(kUploadStatusRequest);
      }
      break;

    case SYS_REASON_FINISHED:
      ESF_LOG_MANAGER_DEBUG("Blob upload finished\n");
      EsfLogManagerSetCloudUploadStatus(kUploadStatusFinished);
      break;

    case SYS_REASON_TIMEOUT:
      /* Do nothing */
      break;

    case SYS_REASON_ERROR:
    default:
      ESF_LOG_MANAGER_ERROR("Cloud upload failed\n");
      EsfLogManagerInternalHandleUploadRetry(user, false);
      break;
  }

  return SYS_RESULT_OK;
}

STATIC EsfLogManagerStatus EsfLogManagerInternalLocalUpload(void) {
  LocalUploadDlogDataT *data = EsfLogManagerGetLocalPriorityUploadData();
  // If there is no data in the list, do not perform any processing and return
  // 'Success' to end the process.
  if ((LocalUploadDlogDataT *)data == NULL) {
    return kEsfLogManagerStatusOk;
  }

  char *file_name = NULL;
  uint32_t name_len = 0;
  switch (data->m_status) {
    case kUploadStatusRequest:
      if (EsfLogManagerInternalCreateFileName(
              kLocalUpload, data->m_block_type, data->m_time_stamp, &file_name,
              &name_len) != kEsfLogManagerStatusOk) {
        ESF_LOG_MANAGER_ERROR("Failed to create file name for local use.\n");
        return kEsfLogManagerStatusFailed;
      }

      /* The size is the size of 'storage_name', 'file path', '/' and null
       * terminator. */
      // Calculate total_len as follows:
      // - (ESF_LOG_MANAGER_STORAGE_NAME_MAX_SIZE - 1): storage_name length
      // without NULL terminator
      // - (name_len - 1): file name length without NULL terminator
      // - +1: for the '/' separator
      // - +1: for the final NULL terminator
      uint32_t total_len = (ESF_LOG_MANAGER_STORAGE_NAME_MAX_SIZE - 1) +
                           (name_len - 1) + 1 + 1;
      char *local_url = (char *)calloc(total_len, sizeof(char));
      if (local_url == NULL) {
        ESF_LOG_MANAGER_ERROR("Failed to calloc.\n");
        free(file_name);
        file_name = NULL;
        return kEsfLogManagerStatusFailed;
      }

      if (pthread_mutex_lock(&s_local_upload_info.m_mutex) != 0) {
        ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_lock.\n");
        free(file_name);
        file_name = NULL;
        free(local_url);
        return kEsfLogManagerStatusFailed;
      }

      snprintf(
          local_url, total_len, "%s/%s",
          s_local_upload_info.upload_group[data->m_block_type].storage_name,
          file_name);
      ESF_LOG_MANAGER_DEBUG("Put blob local_url=%s upload_size=%lu\n",
                            local_url, data->m_upload_size);

      if (pthread_mutex_unlock(&s_local_upload_info.m_mutex) != 0) {
        ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_unlock.\n");
        free(file_name);
        file_name = NULL;
        free(local_url);
        return kEsfLogManagerStatusFailed;
      }

      free(file_name);
      file_name = NULL;

      const struct SYS_http_header headers[] = {
          {.key = NULL, .value = NULL}
      };
      enum SYS_result result =
          SYS_put_blob(s_dlog_sys_client, local_url, headers,
                       (unsigned long long)data->m_upload_size,  // NOLINT
                       EsfLogManagerInternalLocalUploadCallback, (void *)data);
      free(local_url);
      local_url = NULL;

      if (result != SYS_RESULT_OK) {
        ESF_LOG_MANAGER_ERROR("SYS_put_blob failed = %d\n", result);

        /* upload restart */
        EsfLogManagerSetLocalUploadStatus(kUploadStatusRequest);
        return kEsfLogManagerStatusFailed;
      }

      EsfLogManagerSetLocalUploadStatus(kUploadStatusUploading);
      break;

    case kUploadStatusUploading:
      /* Do nothing */
      break;

    case kUploadStatusFinished:
      EsfLogManagerUnregisterLocalListDataPriority();
      break;

    default:
      ESF_LOG_MANAGER_ERROR("status error %d\n", data->m_status);
      EsfLogManagerSetLocalUploadStatus(kUploadStatusRequest);
      break;
  }

  return kEsfLogManagerStatusOk;
}

STATIC EsfLogManagerStatus EsfLogManagerInternalCloudUpload(void) {
  CloudUploadDlogDataT *data = EsfLogManagerGetCloudPriorityUploadData();
  // If there is no data in the list, do not perform any processing and return
  // 'Success' to end the process.
  if ((CloudUploadDlogDataT *)data == NULL) {
    return kEsfLogManagerStatusOk;
  }

  char *file_name = NULL;
  uint32_t name_len = 0;
  switch (data->m_status) {
    case kUploadStatusRequest:
      if (EsfLogManagerInternalCreateFileName(
              kCloudUpload, data->m_block_type, data->m_time_stamp, &file_name,
              &name_len) != kEsfLogManagerStatusOk) {
        ESF_LOG_MANAGER_ERROR("Failed to create file name for cloud use.\n");
        return kEsfLogManagerStatusFailed;
      }

      char *storage_name = NULL;

      if (pthread_mutex_lock(&s_cloud_upload_info.m_mutex) != 0) {
        ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_lock.\n");
        free(file_name);
        file_name = NULL;
        return kEsfLogManagerStatusFailed;
      }

      storage_name = strdup(
          s_cloud_upload_info.upload_group[data->m_block_type].storage_name);

      if (storage_name == NULL) {
        ESF_LOG_MANAGER_ERROR("Failed to strdup.\n");
        free(file_name);
        (void)pthread_mutex_unlock(&s_cloud_upload_info.m_mutex);
        return kEsfLogManagerStatusFailed;
      }

      if (pthread_mutex_unlock(&s_cloud_upload_info.m_mutex) != 0) {
        ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_unlock.\n");
        free(file_name);
        file_name = NULL;
        free(storage_name);
        return kEsfLogManagerStatusFailed;
      }

      ESF_LOG_MANAGER_DEBUG(
          "Put blob mstp. storage_name=%s file_name=%s "
          "upload_size=%lu\n",
          storage_name, file_name, data->m_upload_size);
      enum SYS_result result = SYS_put_blob_mstp(
          s_dlog_sys_client, storage_name, file_name,
          (unsigned long long)data->m_upload_size,  // NOLINT
          EsfLogManagerInternalCloudUploadCallback, (void *)data);

      free(file_name);
      file_name = NULL;
      free(storage_name);
      if (result != SYS_RESULT_OK) {
        ESF_LOG_MANAGER_ERROR("SYS_put_blob_mstp failed = %d\n", result);

        /* upload restart */
        EsfLogManagerSetCloudUploadStatus(kUploadStatusRequest);
        return kEsfLogManagerStatusFailed;
      }

      EsfLogManagerSetCloudUploadStatus(kUploadStatusUploading);
      break;

    case kUploadStatusUploading:
      /* Do nothing */
      break;

    case kUploadStatusFinished:
      EsfLogManagerUnregisterCloudListDataPriority();
      break;

    default:
      ESF_LOG_MANAGER_ERROR("status error %d\n", data->m_status);
      EsfLogManagerSetLocalUploadStatus(kUploadStatusRequest);
      break;
  }

  return kEsfLogManagerStatusOk;
}

STATIC EsfLogManagerStatus EsfLogManagerInternalDlogFullCollector(void) {
  struct MessagePassingObjT msg_obj = {
      .m_cmd = kCmdIsRamBufferPlaneFull,
      .m_len_of_data = (size_t)(sizeof(struct MessagePassingObjT)),
      .m_data = (uint8_t *)s_dlog_buffer_handles[s_dlog_oldest_buff_idx]
                    .m_allocated_memory,
      .m_data_size = s_dlog_store_of_temp_bytebuffer,
      .m_buf_size = DLOG_SIZE_OF_RAM_BUFFER_PLANE,
      .m_block_type = kEsfLogManagerBlockTypeSysApp,
      .m_callback = NULL,
      .m_user_data = NULL,
      .m_is_critical =
          s_dlog_buffer_handles[s_dlog_oldest_buff_idx].is_critical};
  int ret = 0;

  ret = EsfLogManagerInternalSendCmdToDlogCollectorThread(&msg_obj);
  if (ret != kEsfLogManagerStatusOk) {
    ESF_LOG_MANAGER_ERROR(
        "Failed to send cmd to Dlog collector thread. ret=%d\n", ret);
    return kEsfLogManagerStatusFailed;
  }

  return kEsfLogManagerStatusOk;
}

STATIC EsfLogManagerStatus EsfLogManagerInternalDestroyDlogCollector(void) {
  EsfLogManagerStatus ret = 0;
  int ret_thread = 0;
  struct MessagePassingObjT msg_obj = {
      .m_cmd = kCmdIsFinDlogCollector,
      .m_len_of_data = (size_t)(sizeof(struct MessagePassingObjT)),
      .m_data = NULL,
      .m_data_size = 0,
      .m_is_critical = false};

  ret = EsfLogManagerInternalSendCmdToDlogCollectorThread(&msg_obj);
  if (ret != kEsfLogManagerStatusOk) {
    ESF_LOG_MANAGER_ERROR(
        "Failed to send cmd to Dlog collector thread. ret=%d\n", ret);
    return kEsfLogManagerStatusFailed;
  }

  /** Destroy Dlog thread. */
  if (pthread_equal(s_dlog_thread_tid,
                    ESF_LOG_MANAGER_INTERNAL_INVALID_THREAD_ID) == 0) {
    // pthread end wait...
#ifndef LOG_MANAGER_LOCAL_BUILD
    LOG_MANAGER_TRACE_PRINT("\n");
    ret_thread = pthread_join(s_dlog_thread_tid, (void **)NULL);
    LOG_MANAGER_TRACE_PRINT("\n");
    if (ret_thread != 0) {
      ESF_LOG_MANAGER_ERROR("pthread_join error %d\n", ret_thread);
    }
    s_dlog_thread_tid = ESF_LOG_MANAGER_INTERNAL_INVALID_THREAD_ID;
#endif /* #ifndef LOG_MANAGER_LOCAL_BUILD */
  }
  LOG_MANAGER_TRACE_PRINT("\n");

  return kEsfLogManagerStatusOk;
}

STATIC void EsfLogManagerInternalDestroyBlobCollector(void) {
  EsfLogManagerStatus ret = 0;
  int ret_thread = 0;

  s_blob_thread_fin_flag = true;

  /** Destroy Dlog thread. */
  if (pthread_equal(s_blob_thread_tid,
                    ESF_LOG_MANAGER_INTERNAL_INVALID_THREAD_ID) == 0) {
    // pthread end wait...
#ifndef LOG_MANAGER_LOCAL_BUILD
    LOG_MANAGER_TRACE_PRINT("\n");
    ret_thread = pthread_join(s_blob_thread_tid, (void **)NULL);
    LOG_MANAGER_TRACE_PRINT("\n");
    if (ret_thread != 0) {
      ESF_LOG_MANAGER_ERROR("pthread_join error %d\n", ret_thread);
    }
    s_blob_thread_tid = ESF_LOG_MANAGER_INTERNAL_INVALID_THREAD_ID;
#endif /* #ifndef LOG_MANAGER_LOCAL_BUILD */
  }
  LOG_MANAGER_TRACE_PRINT("\n");

  s_blob_thread_fin_flag = false;
}

STATIC EsfLogManagerStatus EsfLogManagerInternalCreateEncryptData(
    size_t data_size, size_t buf_size, uint8_t *data, size_t *out_size) {
#ifdef LOG_MANAGER_ENCRYPT_ENABLE
  EsfLogManagerStatus ret = kEsfLogManagerStatusOk;
  ret = EsfLogManagerInternalEncryptLog(data_size, buf_size, data, out_size);
  if (ret != kEsfLogManagerStatusOk) {
    ESF_LOG_MANAGER_ERROR(
        "Failed to encrypt log. ret=%d data_size=%lu buf_size=%lu\n", ret,
        data_size, buf_size);
    return ret;
  }
#endif /* LOG_MANAGER_ENCRYPT_ENABLE */

  return kEsfLogManagerStatusOk;
}

STATIC EsfLogManagerStatus EsfLogManagerInternalBackupBuffer(
    size_t data_size, size_t buf_size, uint8_t *data, uint8_t **out_data) {
  if ((data == NULL) || (data_size == 0) || (buf_size == 0) ||
      (out_data == NULL)) {
    ESF_LOG_MANAGER_ERROR(
        "ERR:data NULL or size 0. data=%p, data_size=%lu buf_size=%lu "
        "out_data=%p\n",
        data, data_size, buf_size, out_data);
    return kEsfLogManagerStatusFailed;
  }

  *out_data = calloc(buf_size, sizeof(char));
  if (*out_data == NULL) {
    ESF_LOG_MANAGER_ERROR("Failed to calloc.\n");
    return kEsfLogManagerStatusFailed;
  }

  LOG_MANAGER_TRACE_PRINT(":Create tmp buffer\n");
  LOG_MANAGER_TRACE_PRINT(":Copy Size %d\n", data_size);
  memcpy(*out_data, data, data_size);

  if (pthread_mutex_lock(&s_mutex_for_bytebuffer) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_lock.\n");
    free(*out_data);
    *out_data = NULL;
    return kEsfLogManagerStatusFailed;
  }

  ByteBuffer_ErrCode ret =
      BYTEBUFFER_Clear(s_dlog_buffer_handles[s_dlog_oldest_buff_idx].m_handle);
  if (ret != ByteBuffer_EcOk) {
    ESF_LOG_MANAGER_ERROR("Failed to BYTEBUFFER_Clear. idx=%d\n",
                          s_dlog_oldest_buff_idx);
    (void)pthread_mutex_unlock(&s_mutex_for_bytebuffer);
    free(*out_data);
    *out_data = NULL;
    return kEsfLogManagerStatusFailed;
  }

  s_is_dlog_buffer_status = kBufferStatusUse;
  s_dlog_store_of_bytebuffer[s_dlog_oldest_buff_idx] = 0;
  s_dlog_buffer_handles[s_dlog_oldest_buff_idx].is_critical = false;

  if (pthread_mutex_unlock(&s_mutex_for_bytebuffer) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_unlock.\n");
    free(*out_data);
    *out_data = NULL;
    return kEsfLogManagerStatusFailed;
  }

  return kEsfLogManagerStatusOk;
}

STATIC EsfLogManagerStatus EsfLogManagerInternalChangeDlogByteBuffer(void) {
  uint8_t *byte_buffer_ret = NULL;
  s_dlog_oldest_buff_idx = s_dlog_buff_idx;

  if (s_is_dlog_buffer_status == kBufferStatusHalfUse) {
    LOG_MANAGER_TRACE_PRINT(":All BufferFull!!\n");
    s_dlog_store_of_temp_bytebuffer =
        s_dlog_store_of_bytebuffer[s_dlog_buff_idx];
    s_dlog_buff_idx = (s_dlog_buff_idx == 0) ? 1 : 0;
    ByteBuffer_ErrCode ret =
        BYTEBUFFER_Clear(s_dlog_buffer_handles[s_dlog_buff_idx].m_handle);
    if (ret != ByteBuffer_EcOk) {
      ESF_LOG_MANAGER_ERROR("Failed to BYTEBUFFER_Clear. idx=%d\n",
                            s_dlog_buff_idx);
      return kEsfLogManagerStatusFailed;
    }
    s_dlog_store_of_bytebuffer[s_dlog_buff_idx] = 0;
    s_dlog_buffer_handles[s_dlog_buff_idx].is_critical = false;
  } else {
    s_is_dlog_buffer_status = kBufferStatusHalfUse;
    LOG_MANAGER_TRACE_PRINT(":%d %d\n", s_dlog_store_of_temp_bytebuffer,
                            s_dlog_store_of_bytebuffer[s_dlog_buff_idx]);
    s_dlog_store_of_temp_bytebuffer =
        s_dlog_store_of_bytebuffer[s_dlog_buff_idx];
    s_dlog_buff_idx = (s_dlog_buff_idx == 0) ? 1 : 0;
  }

  LOG_MANAGER_TRACE_PRINT(":Change Buff %d\n", s_dlog_buff_idx);

  return kEsfLogManagerStatusOk;
}
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

EsfLogManagerStatus EsfLogManagerInternalInitializeByteBuffer(void) {
// The init ByteBuffer func is nothing processing if the device type
// is Raspberry Pi.
#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
  ByteBuffer_ErrCode ret = ByteBuffer_EcOk;

  if (pthread_mutex_lock(&s_mutex_for_bytebuffer) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_lock.\n");
    return kEsfLogManagerStatusFailed;
  }

  for (uint8_t idx = 0; idx < DLOG_NUM_OF_RAM_BUFFER_PLANES; idx++) {
    s_dlog_buffer_handles[idx].m_allocated_memory =
        malloc(DLOG_SIZE_OF_RAM_BUFFER_PLANE);
    if (s_dlog_buffer_handles[idx].m_allocated_memory == NULL) {
      (void)pthread_mutex_unlock(&s_mutex_for_bytebuffer);
      ESF_LOG_MANAGER_ERROR("allocate memory failed\n");
      return kEsfLogManagerStatusFailed;
    }
    memset(s_dlog_buffer_handles[idx].m_allocated_memory, 0,
           DLOG_SIZE_OF_RAM_BUFFER_PLANE);

    ret = BYTEBUFFER_Init(
        &s_dlog_buffer_handles[idx].m_handle, ByteBuffer_SimpleRing,
        (uint32_t *)s_dlog_buffer_handles[idx].m_allocated_memory,
        DLOG_SIZE_OF_RAM_BUFFER_PLANE);
    if (ret != ByteBuffer_EcOk) {
      free(s_dlog_buffer_handles[idx].m_allocated_memory);
      s_dlog_buffer_handles[idx].m_allocated_memory = NULL;
      (void)pthread_mutex_unlock(&s_mutex_for_bytebuffer);
      ESF_LOG_MANAGER_ERROR("Failed to BYTEBUFFER_Init. ret=%d\n", ret);
      return kEsfLogManagerStatusFailed;
    }
    s_dlog_buffer_handles[idx].is_critical = false;
  }
  if (pthread_mutex_unlock(&s_mutex_for_bytebuffer) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_unlock.\n");
    return kEsfLogManagerStatusFailed;
  }
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

  return kEsfLogManagerStatusOk;
}

void EsfLogManagerInternalInitializeList(void) {
#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
  EsfLogManagerListInit();
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE
}

EsfLogManagerStatus EsfLogManagerInternalSetup(void) {
  UtilityMsgErrCode msg_ret = 0;
  EsfLogManagerStatus ret = kEsfLogManagerStatusOk;

// The dlog thread params is not setup if the device type is Raspberry Pi.
#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
  s_dlog_msg_passing.m_queue_size = LOG_MANAGER_INTERNAL_MSG_QUEUE_SIZE;
  s_dlog_msg_passing.m_max_msg_size = sizeof(struct MessagePassingObjT);
  msg_ret = UtilityMsgOpen(&s_dlog_msg_passing.m_handle,
                           s_dlog_msg_passing.m_queue_size,
                           s_dlog_msg_passing.m_max_msg_size);
  if (msg_ret != kUtilityMsgOk) {
    EsfLogManagerInternalSetupCleaning();
    ESF_LOG_MANAGER_ERROR(
        "Failed to UtilityMsgOpen. Handle is s_dlog_msg_passing.m_handle. "
        "ret=%d\n",
        msg_ret);
    return kEsfLogManagerStatusFailed;
  }
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

  s_elog_msg_passing.m_queue_size = LOG_MANAGER_INTERNAL_MSG_QUEUE_SIZE;
  s_elog_msg_passing.m_max_msg_size = sizeof(struct MessagePassingElogObjT);
  msg_ret = UtilityMsgOpen(&s_elog_msg_passing.m_handle,
                           s_elog_msg_passing.m_queue_size,
                           s_elog_msg_passing.m_max_msg_size);
  if (msg_ret != kUtilityMsgOk) {
    EsfLogManagerInternalSetupCleaning();
    ESF_LOG_MANAGER_ERROR(
        "Failed to UtilityMsgOpen. Handle is s_elog_msg_passing.m_handle. "
        "ret=%d\n",
        msg_ret);
    return kEsfLogManagerStatusFailed;
  }

// The blob thread params is not setup if the device type is Raspberry Pi.
#ifndef CONFIG_EXTERNAL_DLOG_DISABLE

#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

// The dlog thread is not created if the device type is Raspberry Pi.
#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
  ret = EsfLogManagerInternalCreateDlogCollectorThread();
  if (ret != kEsfLogManagerStatusOk) {
    EsfLogManagerInternalSetupCleaning();
    ESF_LOG_MANAGER_ERROR("Failed to create Dlog collector thread. ret=%d\n",
                          ret);
    return kEsfLogManagerStatusFailed;
  }
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

  ret = EsfLogManagerInternalCreateElogThread();
  if (ret != kEsfLogManagerStatusOk) {
    EsfLogManagerInternalSetupCleaning();
    ESF_LOG_MANAGER_ERROR("Failed to create Elog thread. ret=%d\n", ret);
    return kEsfLogManagerStatusFailed;
  }

// The blob thread is not created if the device type is Raspberry Pi.
#ifndef CONFIG_EXTERNAL_DLOG_DISABLE

  ret = EsfLogManagerInternalCreateBlobCollectorThread();
  if (ret != kEsfLogManagerStatusOk) {
    EsfLogManagerInternalSetupCleaning();
    ESF_LOG_MANAGER_ERROR("Failed to create blob collector thread. ret=%d\n",
                          ret);
    return kEsfLogManagerStatusFailed;
  }
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

#ifndef CONFIG_EXTERNAL_DLOG_DISABLE

  EsfSystemManagerResult hw_ret = EsfSystemManagerGetHwInfo(&s_hw_info);
  if (hw_ret != kEsfSystemManagerResultOk) {
    ESF_LOG_MANAGER_ERROR("Get hw info Failed:%d\n", hw_ret);
    memset(s_hw_info.serial_number, 0, sizeof(s_hw_info.serial_number));
  }

  s_dlog_sys_client = EVP_Agent_register_sys_client();
  if (s_dlog_sys_client == NULL) {
    ESF_LOG_MANAGER_ERROR("dlog client NULL\n");
  }
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

  s_elog_sys_client = EVP_Agent_register_sys_client();
  if (s_elog_sys_client == NULL) {
    ESF_LOG_MANAGER_ERROR("elog client NULL\n");
  }

  return kEsfLogManagerStatusOk;
}

EsfLogManagerStatus EsfLogManagerInternalLoadParameter(void) {
  EsfLogManagerStatus ret = kEsfLogManagerStatusOk;

  EsfLogManagerParameterMask mask = {.dlog_dest = 1,
                                     .dlog_level = 1,
                                     .elog_level = 1,
                                     .dlog_filter = 1,
                                     .storage_name = 1,
                                     .storage_path = 1};

  EsfLogManagerParameterValue *value =
      calloc(1, sizeof(EsfLogManagerParameterValue));
  if (value == NULL) {
    ESF_LOG_MANAGER_ERROR("Failed to allocate memory.\n");
    return kEsfLogManagerStatusFailed;
  }

#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
  ret = EsfLogManagerInternalAllocateGlobalUploadData();
  if (ret != kEsfLogManagerStatusOk) {
    ESF_LOG_MANAGER_ERROR("Failed memory allocate for upload data memory.\n");
    free(value);
    value = NULL;
    return kEsfLogManagerStatusFailed;
  }
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

  size_t loadable_block_num = 0;
  const PlLogManagerBlockType *pl_block_type_list =
      PlLogManagerGetLoadableBlock(&loadable_block_num);

  if (pl_block_type_list != NULL) {
    for (size_t i = 0; i < loadable_block_num; i++) {
      EsfLogManagerSettingBlockType esf_block_type =
          EsfLogManagerInternalCovertBlockTypeToEsf(pl_block_type_list[i]);

      if (esf_block_type >= ESF_LOG_MANAGER_BLOCK_TYPE_MAX_NUM) {
        ESF_LOG_MANAGER_ERROR("Invalid block type. esf_block_type=%d\n",
                              esf_block_type);
        ret = kEsfLogManagerStatusFailed;
        break;
      }

      ret = EsfLogManagerLoadParamsForPsm(esf_block_type, &mask, value);
      if (ret != kEsfLogManagerStatusOk) {
        ESF_LOG_MANAGER_ERROR(
            "Failed to load parameter from PSM. ret=%d esf_block_type=%d\n",
            ret, esf_block_type);
        break;
      }

      if (pthread_mutex_lock(&s_parameter.m_mutex) != 0) {
        ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_lock.\n");
        ret = kEsfLogManagerStatusFailed;
        break;
      }

      memcpy(&s_parameter.m_value[esf_block_type], value,
             sizeof(EsfLogManagerParameterValue));

      if (pthread_mutex_unlock(&s_parameter.m_mutex) != 0) {
        ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_unlock.\n");
        ret = kEsfLogManagerStatusFailed;
        break;
      }

#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
      UploadTypeT upload_type = kCloudUpload;
      if (strncmp(value->storage_name, LOG_MANAGER_INTERNAL_HTTP_PROTOCOL_NAME,
                  strlen(LOG_MANAGER_INTERNAL_HTTP_PROTOCOL_NAME)) == 0) {
        upload_type = kLocalUpload;
      }

      if (value->storage_name[0] != '\0') {
        ret = EsfLogManagerInternalSetGlobalUploadDataForStorageName(
            upload_type, esf_block_type, value->storage_name);
        if (ret != kEsfLogManagerStatusOk) {
          ESF_LOG_MANAGER_ERROR(
              "Failed to synchronize the global variable for upload parameter "
              "use.\n");
          ret = kEsfLogManagerStatusFailed;
          break;
        }
      }
      if (value->storage_path[0] != '\0') {
        ret = EsfLogManagerInternalSetGlobalUploadDataForStoragePath(
            esf_block_type, value->storage_path);
        if (ret != kEsfLogManagerStatusOk) {
          ESF_LOG_MANAGER_ERROR(
              "Failed to synchronize the global variable for upload parameter "
              "use.\n");
          ret = kEsfLogManagerStatusFailed;
          break;
        }
      }
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE
    }
  } else {
    ESF_LOG_MANAGER_ERROR("Failed to get loadable block type.\n");
    ret = kEsfLogManagerStatusFailed;
  }

  free(value);
  value = NULL;

  return ret;
}

EsfLogManagerStatus EsfLogManagerInternalDeinitByteBuffer(void) {
// The deinit ByteBuffer func is nothing processing if the device type
// is Raspberry Pi.
#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
  ByteBuffer_ErrCode ret = ByteBuffer_EcOk;

  if (pthread_mutex_lock(&s_mutex_for_bytebuffer) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_lock.\n");
    return kEsfLogManagerStatusFailed;
  }

  for (uint8_t idx = 0; idx < DLOG_NUM_OF_RAM_BUFFER_PLANES; idx++) {
    ret = BYTEBUFFER_Fin(s_dlog_buffer_handles[idx].m_handle);
    if (ret != ByteBuffer_EcOk) {
      (void)pthread_mutex_unlock(&s_mutex_for_bytebuffer);
      ESF_LOG_MANAGER_ERROR("Failed to BYTEBUFFER_Fin. ret=%d idx=%d\n", ret,
                            idx);
      return kEsfLogManagerStatusFailed;
    }
    free(s_dlog_buffer_handles[idx].m_allocated_memory);
    s_dlog_buffer_handles[idx].m_allocated_memory = NULL;
    s_dlog_buffer_handles[idx].is_critical = false;
  }

  if (pthread_mutex_unlock(&s_mutex_for_bytebuffer) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_unlock.\n");
    return kEsfLogManagerStatusFailed;
  }
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

  return kEsfLogManagerStatusOk;
}

#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
EsfLogManagerStatus EsfLogManagerInternalWriteDlog(const uint8_t *str,
                                                   uint32_t size,
                                                   bool is_critical) {
  // The write dlog func is nothing processing if the device type
  // is Raspberry Pi.
  uint8_t *byte_buffer_ret = NULL;
  EsfLogManagerStatus ret = kEsfLogManagerStatusOk;
  bool is_full = false;
  //  LOG_MANAGER_TRACE_PRINT(":%s %ul\n", str,size);
  if (pthread_mutex_lock(&s_mutex_for_bytebuffer) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_lock.\n");
    return kEsfLogManagerStatusFailed;
  }

  // after locking the byte buffer mutex
  if (pthread_mutex_lock(&s_critical_log_mutex) != 0) {
    (void)pthread_mutex_unlock(&s_mutex_for_bytebuffer);
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_lock.\n");
    return kEsfLogManagerStatusFailed;
  }

  //  LOG_MANAGER_TRACE_PRINT(":%d
  //  %d\n",s_dlog_store_of_bytebuffer[s_dlog_buff_idx],
  //                           s_dlog_buff_idx);
  if ((s_dlog_store_of_bytebuffer[s_dlog_buff_idx] + size +
       LOG_MANAGER_INTERNAL_SEC_AES_BLOCK_SIZE +
       (LOG_MANAGER_INTERNAL_SEC_AES_BLOCK_SIZE -
        ((s_dlog_store_of_bytebuffer[s_dlog_buff_idx] + size) %
         LOG_MANAGER_INTERNAL_SEC_AES_BLOCK_SIZE))) <
      DLOG_SIZE_OF_RAM_BUFFER_PLANE) {
    byte_buffer_ret = BYTEBUFFER_PushBack(
        s_dlog_buffer_handles[s_dlog_buff_idx].m_handle, (uint8_t *)str, size);
    if (byte_buffer_ret == NULL) {
      (void)pthread_mutex_unlock(&s_critical_log_mutex);
      (void)pthread_mutex_unlock(&s_mutex_for_bytebuffer);
      ESF_LOG_MANAGER_ERROR(
          "Failed to BYTEBUFFER_PushBack. s_dlog_buff_idx=%d stored_size=%lu "
          "size=%u sec_aes_block_size=%d max_size=%lu\n",
          s_dlog_buff_idx, s_dlog_store_of_bytebuffer[s_dlog_buff_idx], size,
          LOG_MANAGER_INTERNAL_SEC_AES_BLOCK_SIZE,
          DLOG_SIZE_OF_RAM_BUFFER_PLANE);
      return kEsfLogManagerStatusFailed;
    }
    //    LOG_MANAGER_TRACE_PRINT(":%d
    //    %d\n",s_dlog_store_of_bytebuffer[s_dlog_buff_idx],
    //                             size);
    s_dlog_store_of_bytebuffer[s_dlog_buff_idx] += size;
  } else {
    is_full = true;

    ret = EsfLogManagerInternalChangeDlogByteBuffer();
    if (ret != kEsfLogManagerStatusOk) {
      (void)pthread_mutex_unlock(&s_critical_log_mutex);
      (void)pthread_mutex_unlock(&s_mutex_for_bytebuffer);
      ESF_LOG_MANAGER_ERROR("Failed to change Dlog ByteBuffer. ret=%d\n", ret);
      return kEsfLogManagerStatusFailed;
    }

    s_critical_log_pending = false;

    byte_buffer_ret = BYTEBUFFER_PushBack(
        s_dlog_buffer_handles[s_dlog_buff_idx].m_handle, (uint8_t *)str, size);
    if (byte_buffer_ret == NULL) {
      ESF_LOG_MANAGER_ERROR(
          "Failed to BYTEBUFFER_PushBack. s_dlog_buff_idx=%d stored_size=%lu "
          "data_size=%u\n",
          s_dlog_buff_idx, s_dlog_store_of_bytebuffer[s_dlog_buff_idx], size);
      (void)pthread_mutex_unlock(&s_critical_log_mutex);
      (void)pthread_mutex_unlock(&s_mutex_for_bytebuffer);
      return kEsfLogManagerStatusFailed;
    }

    LOG_MANAGER_TRACE_PRINT("%d %d\n",
                            s_dlog_store_of_bytebuffer[s_dlog_buff_idx], size);
    s_dlog_store_of_bytebuffer[s_dlog_buff_idx] += size;

    LOG_MANAGER_TRACE_PRINT("\n");
  }

  if (is_critical) {
    s_dlog_buffer_handles[s_dlog_buff_idx].is_critical = is_critical;

    if (s_critical_log_pending == false) {
      clock_gettime(CLOCK_REALTIME, &s_last_critical_log_time);
      s_critical_log_pending = true;
      ESF_LOG_MANAGER_INFO(
          "Critical log detected, scheduling urgent upload.\n");
    }
  }

  if (pthread_mutex_unlock(&s_critical_log_mutex) != 0) {
    (void)pthread_mutex_unlock(&s_mutex_for_bytebuffer);
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_unlock.\n");
    return kEsfLogManagerStatusFailed;
  }

  if (pthread_mutex_unlock(&s_mutex_for_bytebuffer) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_unlock.\n");
    return kEsfLogManagerStatusFailed;
  }

  if (is_full) {
    ret = EsfLogManagerInternalDlogFullCollector();
    if (ret != kEsfLogManagerStatusOk) {
      ESF_LOG_MANAGER_ERROR("Failed to process about Dlog full. ret=%d\n", ret);
      return kEsfLogManagerStatusFailed;
    }
  }
  //  LOG_MANAGER_TRACE_PRINT("\n");

  return kEsfLogManagerStatusOk;
}
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

STATIC void EsfLogManagerInternalClearAllElogMessage(void) {
  struct MessagePassingElogObjT msg = {};
  int32_t recv_size = 0;
  int32_t timeout_ms = 500;

  while (1) {
    UtilityMsgErrCode utility_ret =
        UtilityMsgRecv(s_elog_msg_passing.m_handle, (void *)&msg, sizeof(msg),
                       timeout_ms, (int32_t *)&recv_size);
    if (utility_ret == kUtilityMsgErrTimedout) {
      // No message received.
      break;

    } else if (utility_ret == kUtilityMsgOk) {
      switch (msg.m_cmd) {
        case kCmdIsSend:
        // follow through
        case kCmdIsResend:
          free(msg.message);
          msg.message = NULL;
          break;

        default:
          // nothing to do
          break;
      }

    } else {
      ESF_LOG_MANAGER_ERROR(
          "Failed to UtilityMsgRecv. Handle is s_elog_msg_passing.m_handle. "
          "ret=%d\n",
          utility_ret);
      break;
    }
  }
}

EsfLogManagerStatus EsfLogManagerInternalDeinit(void) {
  EsfLogManagerStatus ret = kEsfLogManagerStatusOk;
  int byte_buffer_ret = 0;
  UtilityMsgErrCode utility_ret = kUtilityMsgOk;
// The dlog thread has not been created if the device type is Raspberry Pi.
#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
  ret = EsfLogManagerInternalDestroyDlogCollector();
  if (ret != kEsfLogManagerStatusOk) {
    ESF_LOG_MANAGER_ERROR("Failed to destroy Dlog collector thread. ret=%d\n",
                          ret);
    return kEsfLogManagerStatusFailed;
  }

  if (s_dlog_thread_stack != NULL) {
    free(s_dlog_thread_stack);
    s_dlog_thread_stack = (void *)NULL;
  }
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

  ret = EsfLogManagerInternalDestroyElogThread();
  if (ret != kEsfLogManagerStatusOk) {
    ESF_LOG_MANAGER_ERROR("Failed to destroy Elog thread. ret=%d\n", ret);
    return kEsfLogManagerStatusFailed;
  }

  if (s_elog_thread_stack != NULL) {
    free(s_elog_thread_stack);
    s_elog_thread_stack = (void *)NULL;
  }

  ret = EsfLogManagerInternalClearElog();
  if (ret != kEsfLogManagerStatusOk) {
    ESF_LOG_MANAGER_ERROR("Failed to clear Elog. ret=%d\n", ret);
    return kEsfLogManagerStatusFailed;
  }

// the blob thread has not been created if the device type is Raspberry Pi.
#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
  (void)EsfLogManagerInternalDestroyBlobCollector();

  if (s_blob_thread_stack != NULL) {
    free(s_blob_thread_stack);
    s_blob_thread_stack = (void *)NULL;
  }
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

  byte_buffer_ret = EsfLogManagerInternalDeinitByteBuffer();
  if (byte_buffer_ret != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to deinit ByteBuffer. ret=%d\n",
                          byte_buffer_ret);
    return kEsfLogManagerStatusFailed;
  }

#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
  // Unnecessary to receive all messages in the Dlog queue.
  // Because Dlog data is in the list.
  // List will be cleared in the next step.
  utility_ret = UtilityMsgClose(s_dlog_msg_passing.m_handle);
  if (utility_ret != kUtilityMsgOk) {
    ESF_LOG_MANAGER_ERROR(
        "Failed to UtilityMsgClose. Handle is s_dlog_msg_passing.m_handle. "
        "ret=%d\n",
        utility_ret);
    return kEsfLogManagerStatusFailed;
  }
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

  // Receive all messages in the Elog queue.
  // This is to prevent memory leak.
  EsfLogManagerInternalClearAllElogMessage();

  utility_ret = UtilityMsgClose(s_elog_msg_passing.m_handle);
  if (utility_ret != kUtilityMsgOk) {
    ESF_LOG_MANAGER_ERROR(
        "Failed to UtilityMsgClose. Handle is s_elog_msg_passing.m_handle. "
        "ret=%d\n",
        utility_ret);
    return kEsfLogManagerStatusFailed;
  }

  {
    if (pthread_mutex_lock(&sp_elog_mutex) != 0) {
      ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_lock.\n");
      return kEsfLogManagerStatusFailed;
    }

    s_elog_queue_cnt = 0;

    if (pthread_mutex_unlock(&sp_elog_mutex) != 0) {
      ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_unlock.\n");
      return kEsfLogManagerStatusFailed;
    }
  }
#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
  ret = EsfLogManagerInternalDeallocateGlobalUploadData();
  if (ret != kEsfLogManagerStatusOk) {
    ESF_LOG_MANAGER_ERROR("Failed to deallocate global upload data ret=%d\n",
                          ret);
    return kEsfLogManagerStatusFailed;
  }

  ret = EsfLogManagerDeleteCallbackList();
  if (ret != kEsfLogManagerStatusOk) {
    ESF_LOG_MANAGER_ERROR("Failed to clean Dlog callback list. ret=%d\n", ret);
    return kEsfLogManagerStatusFailed;
  }

  ret = EsfLogManagerDeleteLocalUploadList();
  if (ret != kEsfLogManagerStatusOk) {
    ESF_LOG_MANAGER_ERROR("Failed to upload local list delete. ret=%d\n", ret);
    return kEsfLogManagerStatusFailed;
  }
  ret = EsfLogManagerDeleteCloudUploadList();
  if (ret != kEsfLogManagerStatusOk) {
    ESF_LOG_MANAGER_ERROR("Failed to upload cloud list delete. ret=%d\n", ret);
    return kEsfLogManagerStatusFailed;
  }

  if (s_dlog_sys_client != NULL) {
    //    EVP_Agent_unregister_sys_client(s_dlog_sys_client);
    s_dlog_sys_client = NULL;
  }
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

  if (s_elog_sys_client != NULL) {
    EVP_Agent_unregister_sys_client(s_elog_sys_client);
    s_elog_sys_client = NULL;
  }

  return kEsfLogManagerStatusOk;
}

bool EsfLogManagerInternalGetLogInfo(EsfLogManagerLogInfo *const log_info) {
  if (log_info == NULL) {
    return false;
  }

  log_info->dlog_ram.size = (uint32_t)DLOG_SIZE_OF_RAM_BUFFER_PLANE;
  log_info->dlog_ram.num = (uint32_t)DLOG_NUM_OF_RAM_BUFFER_PLANES;
  log_info->elog_ram.size = (uint32_t)ELOG_SIZE_OF_RAM_BUFFER_PLANE;
  log_info->elog_ram.num = (uint32_t)ELOG_NUM_OF_RAM_BUFFER_PLANES;
  log_info->dlog_flash.size = 0;
  log_info->dlog_flash.num = 0;
  log_info->elog_flash.size = 0;
  log_info->elog_flash.num = 0;

  return true;
}

static bool EsfLogManagerCheckStoragePath(const char *str) {
  size_t len = strnlen(str, ESF_LOG_MANAGER_STORAGE_SUB_DIR_PATH_MAX_SIZE);
  /* Allow empty strings for clearing text */
  if (len == 0) {
    return true;
  }
  if (len == ESF_LOG_MANAGER_STORAGE_SUB_DIR_PATH_MAX_SIZE) {
    ESF_LOG_MANAGER_ERROR("Invalid parameter. storage path length = %zu\n",
                          len);
    return false;
  }

  /* check of the last character */
  if ((str[len - 1] == '.') || (str[len - 1] == '/') ||
      (str[len - 1] == '\\')) {
    ESF_LOG_MANAGER_ERROR("Invalid parameter. storage_path=%s\n", str);
    return false;
  }

  /* check of the whitespace */
  for (uint32_t i = 0; i < len; i++) {
    if (isspace(str[i])) {
      ESF_LOG_MANAGER_ERROR("Invalid parameter. storage path whitespace = %s\n",
                            str);
      return false;
    }
  }

  return true;
}

STATIC bool EsfLogManagerCheckStorageName(
    EsfLogManagerSettingBlockType block_type, const char *str) {
  size_t len = strnlen(str, ESF_LOG_MANAGER_STORAGE_NAME_MAX_SIZE);
  /* Allow empty strings for clearing text */
  if (len == 0) {
    return true;
  }

  if (len == ESF_LOG_MANAGER_STORAGE_NAME_MAX_SIZE) {
    ESF_LOG_MANAGER_ERROR("Invalid parameter. storage_name length = %zu\n",
                          len);
    return false;
  }

  if (EsfLogManagerInternalJudgeLocalUpload(str) == true) {
    if ((PlLogManagerLocalUploadAvailabilityCheck() == false) ||
        (block_type != kEsfLogManagerBlockTypeVicapp)) {
      ESF_LOG_MANAGER_ERROR(
          "Invalid parameter(You cannot set the specified value. storage_name "
          "= %s \n",
          str);
      return false;
    }
  }

  return true;
}

STATIC bool EsfLogManagerCheckParameters(
    EsfLogManagerSettingBlockType const block_type,
    EsfLogManagerParameterValue const *value,
    EsfLogManagerParameterMask const *mask) {
  if (mask->dlog_dest) {
    if ((value->dlog_dest < kEsfLogManagerDlogDestUart) ||
        (value->dlog_dest >= kEsfLogManagerDlogDestNum)) {
      ESF_LOG_MANAGER_ERROR("Invalid parameter. dlog_dest=%d\n",
                            value->dlog_dest);
      return false;
    }
  }

  if (mask->dlog_level) {
    if ((value->dlog_level < kEsfLogManagerDlogLevelCritical) ||
        (value->dlog_level >= kEsfLogManagerDlogLevelNum)) {
      ESF_LOG_MANAGER_ERROR("Invalid parameter. dlog_level=%d\n",
                            value->dlog_level);
      return false;
    }
  }

  if (mask->elog_level) {
    if ((value->elog_level < kEsfLogManagerElogLevelCritical) ||
        (value->elog_level >= kEsfLogManagerElogLevelNum)) {
      ESF_LOG_MANAGER_ERROR("Invalid parameter. elog_level=%d\n",
                            value->elog_level);
      return false;
    }
  }

  if (mask->storage_path) {
    if (EsfLogManagerCheckStoragePath(value->storage_path) == false) {
      return false;
    }
  }

  if (mask->storage_name) {
    if (EsfLogManagerCheckStorageName(block_type, value->storage_name) ==
        false) {
      return false;
    }
  }

  return true;
}

EsfLogManagerStatus EsfLogManagerInternalSetParameter(
    EsfLogManagerSettingBlockType const block_type,
    EsfLogManagerParameterValue const *value,
    EsfLogManagerParameterMask const *mask) {
#if !defined(CONFIG_EXTERNAL_DLOG_DISABLE) || \
    !defined(CONFIG_EXTERNAL_LOG_MANAGER_PSM_DISABLE)
  EsfLogManagerStatus ret = kEsfLogManagerStatusOk;
#endif  // !defined(CONFIG_EXTERNAL_DLOG_DISABLE) ||
        // !defined(CONFIG_EXTERNAL_LOG_MANAGER_PSM_DISABLE)

#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
  UploadTypeT upload_type[kEsfLogManagerBlockTypeAll] = {};
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

  if (block_type > kEsfLogManagerBlockTypeAll) {
    ESF_LOG_MANAGER_ERROR("Not support block type %d\n", block_type);
    return kEsfLogManagerStatusParamError;
  }

  if (EsfLogManagerCheckParameters(block_type, value, mask) == false) {
    ESF_LOG_MANAGER_ERROR("Failed to check parameter.\n");
    return kEsfLogManagerStatusParamError;
  }

  if (pthread_mutex_lock(&s_parameter.m_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_lock.\n");
    return kEsfLogManagerStatusFailed;
  }

  if (block_type == kEsfLogManagerBlockTypeAll) {
    for (uint32_t i = 0; i < kEsfLogManagerBlockTypeAll; i++) {
      if (mask->dlog_dest) {
        s_parameter.m_value[i].dlog_dest = value->dlog_dest;
      }
      if (mask->dlog_level) {
        s_parameter.m_value[i].dlog_level = value->dlog_level;
      }
      if (mask->elog_level) {
        s_parameter.m_value[i].elog_level = value->elog_level;
      }
      if (mask->dlog_filter) {
        s_parameter.m_value[i].dlog_filter = value->dlog_filter;
      }
      if (mask->storage_name) {
        strncpy(s_parameter.m_value[i].storage_name, value->storage_name,
                sizeof(s_parameter.m_value[i].storage_name));
      }
      if (mask->storage_path) {
        strncpy(s_parameter.m_value[i].storage_path, value->storage_path,
                sizeof(s_parameter.m_value[i].storage_path));
      }
    }
  } else {
    if (mask->dlog_dest) {
      s_parameter.m_value[block_type].dlog_dest = value->dlog_dest;
    }
    if (mask->dlog_level) {
      s_parameter.m_value[block_type].dlog_level = value->dlog_level;
    }
    if (mask->elog_level) {
      s_parameter.m_value[block_type].elog_level = value->elog_level;
    }
    if (mask->dlog_filter) {
      s_parameter.m_value[block_type].dlog_filter = value->dlog_filter;
    }
    if (mask->storage_name) {
      strncpy(s_parameter.m_value[block_type].storage_name, value->storage_name,
              sizeof(s_parameter.m_value[block_type].storage_name));
    }
    if (mask->storage_path) {
      strncpy(s_parameter.m_value[block_type].storage_path, value->storage_path,
              sizeof(s_parameter.m_value[block_type].storage_path));
    }
#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
    if (strncmp(s_parameter.m_value[block_type].storage_name,
                LOG_MANAGER_INTERNAL_HTTP_PROTOCOL_NAME,
                strlen(LOG_MANAGER_INTERNAL_HTTP_PROTOCOL_NAME)) == 0) {
      upload_type[block_type] = kLocalUpload;
    }
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE
  }

  if (pthread_mutex_unlock(&s_parameter.m_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_unlock.\n");
    return kEsfLogManagerStatusFailed;
  }

#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
  if (block_type == kEsfLogManagerBlockTypeAll) {
    for (uint32_t i = 0; i < kEsfLogManagerBlockTypeAll; i++) {
      if (mask->storage_name) {
        ret = EsfLogManagerInternalSetGlobalUploadDataForStorageName(
            upload_type[i], i, value->storage_name);
        if (ret != kEsfLogManagerStatusOk) {
          ESF_LOG_MANAGER_ERROR(
              "Failed to synchronize the global variable for upload parameter "
              "use.\n");
          return kEsfLogManagerStatusFailed;
        }
      }
      if (mask->storage_path) {
        ret = EsfLogManagerInternalSetGlobalUploadDataForStoragePath(
            i, value->storage_path);
        if (ret != kEsfLogManagerStatusOk) {
          ESF_LOG_MANAGER_ERROR(
              "Failed to synchronize the global variable for upload parameter "
              "use.\n");
          return kEsfLogManagerStatusFailed;
        }
      }
    }
  } else {
    if (mask->storage_name) {
      ret = EsfLogManagerInternalSetGlobalUploadDataForStorageName(
          upload_type[block_type], block_type, value->storage_name);
      if (ret != kEsfLogManagerStatusOk) {
        ESF_LOG_MANAGER_ERROR(
            "Failed to synchronize the global variable for upload parameter "
            "use.\n");
        return kEsfLogManagerStatusFailed;
      }
    }
    if (mask->storage_path) {
      ret = EsfLogManagerInternalSetGlobalUploadDataForStoragePath(
          block_type, value->storage_path);
      if (ret != kEsfLogManagerStatusOk) {
        ESF_LOG_MANAGER_ERROR(
            "Failed to synchronize the global variable for upload parameter "
            "use.\n");
        return kEsfLogManagerStatusFailed;
      }
    }
  }
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE
// Disable the process of setting parameters to the PSM.
#ifndef CONFIG_EXTERNAL_LOG_MANAGER_PSM_DISABLE
  {
    size_t saveable_block_num = 0;
    const PlLogManagerBlockType *pl_block_type_list =
        PlLogManagerGetSaveableBlock(
            EsfLogManagerInternalCovertBlockTypeToPl(block_type),
            &saveable_block_num);

    if (pl_block_type_list != NULL) {
      for (size_t i = 0; i < saveable_block_num; i++) {
        ret = EsfLogManagerSaveParamsForPsm(
            EsfLogManagerInternalCovertBlockTypeToEsf(pl_block_type_list[i]),
            mask, value);
        if (ret != kEsfLogManagerStatusOk) {
          ESF_LOG_MANAGER_ERROR("Failed to save parameter for PSM. ret=%d\n",
                                ret);
          return kEsfLogManagerStatusFailed;
        }
      }
    }
  }
#endif  // CONFIG_EXTERNAL_LOG_MANAGER_PSM_DISABLE

  return kEsfLogManagerStatusOk;
}

EsfLogManagerStatus EsfLogManagerInternalGetParameter(
    EsfLogManagerSettingBlockType const block_type,
    EsfLogManagerParameterValue *value) {
  if (block_type >= kEsfLogManagerBlockTypeAll) {
    ESF_LOG_MANAGER_ERROR("Invalid parameter. block_type=%d\n", block_type);
    return kEsfLogManagerStatusFailed;
  }

  if (value == NULL) {
    ESF_LOG_MANAGER_ERROR("value is NULL\n");
    return kEsfLogManagerStatusFailed;
  }

  if (pthread_mutex_lock(&s_parameter.m_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_lock.\n");
    return kEsfLogManagerStatusFailed;
  }

  value->dlog_dest = s_parameter.m_value[block_type].dlog_dest;
  value->dlog_level = s_parameter.m_value[block_type].dlog_level;
  value->elog_level = s_parameter.m_value[block_type].elog_level;
  value->dlog_filter = s_parameter.m_value[block_type].dlog_filter;
  strncpy(value->storage_name, s_parameter.m_value[block_type].storage_name,
          strlen(s_parameter.m_value[block_type].storage_name) + 1);
  strncpy(value->storage_path, s_parameter.m_value[block_type].storage_path,
          strlen(s_parameter.m_value[block_type].storage_path) + 1);

  if (pthread_mutex_unlock(&s_parameter.m_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_unlock.\n");
    return kEsfLogManagerStatusFailed;
  }

  return kEsfLogManagerStatusOk;
}

EsfLogManagerSettingBlockType EsfLogManagerInternalGetGroupID(
    uint32_t module_id) {
  for (uint32_t i = 0;
       i < sizeof(s_grouping_lockup_table) / sizeof(s_grouping_lockup_table[0]);
       i++) {
    if (module_id == s_grouping_lockup_table[i].m_module_id) {
      return s_grouping_lockup_table[i].m_group_id;
    }
  }

  return kEsfLogManagerBlockTypeNum;
}

void EsfLogManagerInternalChangeDlogCallback(
    EsfLogManagerSettingBlockType const block_type) {
// The change dlog callback func is nothing processing if the device type
// is Raspberry Pi.
#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
  EsfLogManagerDlogChangeInfo info = {
      .value.dlog_dest = kEsfLogManagerDlogDestUart,
      .value.dlog_level = kEsfLogManagerDlogLevelInfo,
      .value.elog_level = kEsfLogManagerElogLevelInfo,
      .value.dlog_filter = 0,
      .value.storage_name = {'\0'},
      .value.storage_path = {'\0'},
      .module_id = 0};
  EsfLogManagerChangeDlogCallback callback = NULL;

  ChangeDlogCallbackDataT *entry = EsfLogManagerSearchCallbackList(block_type);
  if (entry != NULL) {
    if (entry->m_callback != NULL) {
      if (pthread_mutex_lock(&s_parameter.m_mutex) != 0) {
        ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_lock.\n");
        return;
      }

      info.module_id = entry->m_module_id;
      info.value.dlog_dest = s_parameter.m_value[block_type].dlog_dest;
      info.value.dlog_level = s_parameter.m_value[block_type].dlog_level;
      info.value.elog_level = s_parameter.m_value[block_type].elog_level;
      info.value.dlog_filter = s_parameter.m_value[block_type].dlog_filter;
      strncpy(info.value.storage_name,
              s_parameter.m_value[block_type].storage_name,
              sizeof(info.value.storage_name));
      strncpy(info.value.storage_path,
              s_parameter.m_value[block_type].storage_path,
              sizeof(info.value.storage_path));
      callback = entry->m_callback;

      if (pthread_mutex_unlock(&s_parameter.m_mutex) != 0) {
        ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_unlock.\n");
        return;
      }
    }
  }
  if (callback != NULL) {
    callback(&info);
  }
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE
}

// The register callback func is nothing processing if the device type
// is Raspberry Pi.
#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
EsfLogManagerStatus EsfLogManagerInternalRegisterChangeDlogCallback(
    uint32_t module_id, const EsfLogManagerChangeDlogCallback *const callback) {
  if ((module_id == 0) || (*callback == NULL)) {
    return kEsfLogManagerStatusParamError;
  }
  EsfLogManagerStatus ret = EsfLogManagerRegisterCallbackList(module_id,
                                                              callback);
  if (ret != kEsfLogManagerStatusOk) {
    ESF_LOG_MANAGER_ERROR("Failed to register callback id=%d\n", module_id);
    return kEsfLogManagerStatusFailed;
  }

  return kEsfLogManagerStatusOk;
}
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

// The register callback func is nothing processing if the device type
// is Raspberry Pi.
#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
EsfLogManagerStatus EsfLogManagerInternalUnregisterChangeDlogCallback(
    uint32_t module_id) {
  bool found = false;
  if (module_id == 0) {
    ESF_LOG_MANAGER_ERROR("Invalid parameter. module_id=%d\n", module_id);
    return kEsfLogManagerStatusParamError;
  }

  EsfLogManagerStatus ret = EsfLogManagerUnregisterCallbackList(module_id);
  if (ret != kEsfLogManagerStatusOk) {
    ESF_LOG_MANAGER_ERROR("Failed to unregister callback id=%d\n", module_id);
    return kEsfLogManagerStatusFailed;
  }

  return kEsfLogManagerStatusOk;
}
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

// The register callback func is nothing processing if the device type
// is Raspberry Pi.
#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
EsfLogManagerStatus EsfLogManagerInternalSendBulkDlog(
    uint32_t module_id, size_t size, uint8_t *bulk_log,
    EsfLogManagerBulkDlogCallback callback, void *user_data) {
  int ret = 0;

  if (size <= 0) {
    ESF_LOG_MANAGER_ERROR("Invalid parameter. size=%zu \n", size);
    return kEsfLogManagerStatusParamError;
  }
  if (bulk_log == NULL) {
    ESF_LOG_MANAGER_ERROR("Invalid parameter. bulk_log=%p\n", bulk_log);
    return kEsfLogManagerStatusParamError;
  }

  size_t buf_size = size +
                    (LOG_MANAGER_INTERNAL_SEC_AES_BLOCK_SIZE -
                     (size % LOG_MANAGER_INTERNAL_SEC_AES_BLOCK_SIZE)) +
                    LOG_MANAGER_INTERNAL_SEC_AES_BLOCK_SIZE;

  struct MessagePassingObjT msg_obj = {
      .m_cmd = kCmdIsSendBulkDlog,
      .m_len_of_data = (size_t)(sizeof(struct MessagePassingObjT)),
      .m_data = NULL,
      .m_data_size = size,
      .m_buf_size = buf_size,
      .m_callback = *callback,
      .m_user_data = user_data,
      .m_is_critical = false};

  msg_obj.m_block_type = EsfLogManagerInternalGetGroupID(module_id);
  if (msg_obj.m_block_type >= kEsfLogManagerBlockTypeNum) {
    ESF_LOG_MANAGER_ERROR("invalid module id = %d\n", module_id);
    return kEsfLogManagerStatusParamError;
  }

  uint8_t *tmp_bulk_log = (uint8_t *)calloc(buf_size, sizeof(uint8_t));
  if (tmp_bulk_log == NULL) {
    ESF_LOG_MANAGER_ERROR("malloc failed is NULL\n");
    return kEsfLogManagerStatusFailed;
  }
  msg_obj.m_data = tmp_bulk_log;
  memcpy(tmp_bulk_log, bulk_log, size);

  ret = EsfLogManagerInternalSendCmdToDlogCollectorThread(&msg_obj);
  if (ret != kEsfLogManagerStatusOk) {
    ESF_LOG_MANAGER_ERROR(
        "Failed to send cmd to Dlog collector thread. ret=%d\n", ret);
    free(tmp_bulk_log);
    return kEsfLogManagerStatusFailed;
  }

  return kEsfLogManagerStatusOk;
}
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

void EsfLogManagerInternalErrorOutput(uint16_t level, const char *func,
                                      int line, const char *format, ...) {
  char str[LOG_MANAGER_INTERNAL_STR_BUFF_SIZE] = {0};
  const char level_str[kEsfLogManagerDlogLevelNum] = {'C', 'E', 'W',
                                                      'I', 'D', 'T'};
  int32_t idx = 0;
  va_list list;
  va_start(list, format);
  snprintf(str, sizeof(str), "%c:%s-%d:", level_str[level], func, line);
  idx = strnlen(str, sizeof(str));
  vsnprintf(&str[idx], (sizeof(str) - idx), format, list);
#ifdef CONFIG_EXTERNAL_LOG_MANAGER_ENABLE_SYSLOG
  syslog(level, "%s", str);
#else   // CONFIG_EXTERNAL_LOG_MANAGER_ENABLE_SYSLOG
  printf("%s", str);
#endif  // CONFIG_EXTERNAL_LOG_MANAGER_ENABLE_SYSLOG
  va_end(list);
}

void EsfLogManagerInternalDebugOutput(uint16_t level, const char *func,
                                      int line, const char *format, ...) {
  char str[LOG_MANAGER_INTERNAL_STR_BUFF_SIZE] = {0};
  const char level_str[kEsfLogManagerDlogLevelNum] = {'C', 'E', 'W',
                                                      'I', 'D', 'T'};
  int32_t idx = 0;
  va_list list;
  va_start(list, format);
  snprintf(str, sizeof(str), "%s(%d):%c:", func, line, level_str[level]);
  idx = strnlen(str, sizeof(str));
  vsnprintf(&str[idx], (sizeof(str) - idx), format, list);
#ifdef CONFIG_EXTERNAL_LOG_MANAGER_ENABLE_SYSLOG
  syslog(level, "%s", str);
#else   // CONFIG_EXTERNAL_LOG_MANAGER_ENABLE_SYSLOG
  printf("%s", str);
#endif  // CONFIG_EXTERNAL_LOG_MANAGER_ENABLE_SYSLOG
  va_end(list);
}

STATIC bool EsfLogManagerInternalJudgeLocalUpload(const char *name) {
  if (name == NULL) {
    return false;
  }

  if (strncmp(name, LOG_MANAGER_INTERNAL_HTTP_PROTOCOL_NAME,
              strlen(LOG_MANAGER_INTERNAL_HTTP_PROTOCOL_NAME)) == 0) {
    return true;
  } else {
    return false;
  }
}

#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
STATIC EsfLogManagerStatus EsfLogManagerInternalAllocateGlobalUploadData(void) {
  for (uint32_t i = 0; i < kEsfLogManagerBlockTypeAll; i++) {
    s_local_upload_info.upload_group[i].storage_name =
        (char *)calloc(ESF_LOG_MANAGER_STORAGE_NAME_MAX_SIZE, sizeof(char));
    if (s_local_upload_info.upload_group[i].storage_name == NULL) {
      ESF_LOG_MANAGER_ERROR("calloc failed is NULL\n");
      (void)EsfLogManagerInternalDeallocateGlobalUploadData();
      return kEsfLogManagerStatusFailed;
    }

    s_cloud_upload_info.upload_group[i].storage_name =
        (char *)calloc(ESF_LOG_MANAGER_STORAGE_NAME_MAX_SIZE, sizeof(char));
    if (s_cloud_upload_info.upload_group[i].storage_name == NULL) {
      ESF_LOG_MANAGER_ERROR("calloc failed is NULL\n");
      (void)EsfLogManagerInternalDeallocateGlobalUploadData();
      return kEsfLogManagerStatusFailed;
    }

    s_local_upload_info.upload_group[i].storage_path = (char *)calloc(
        ESF_LOG_MANAGER_STORAGE_SUB_DIR_PATH_MAX_SIZE, sizeof(char));
    if (s_local_upload_info.upload_group[i].storage_path == NULL) {
      ESF_LOG_MANAGER_ERROR("calloc failed is NULL\n");
      (void)EsfLogManagerInternalDeallocateGlobalUploadData();
      return kEsfLogManagerStatusFailed;
    }

    s_cloud_upload_info.upload_group[i].storage_path = (char *)calloc(
        ESF_LOG_MANAGER_STORAGE_SUB_DIR_PATH_MAX_SIZE, sizeof(char));
    if (s_cloud_upload_info.upload_group[i].storage_path == NULL) {
      ESF_LOG_MANAGER_ERROR("calloc failed is NULL\n");
      (void)EsfLogManagerInternalDeallocateGlobalUploadData();
      return kEsfLogManagerStatusFailed;
    }
  }

  return kEsfLogManagerStatusOk;
}

STATIC EsfLogManagerStatus
EsfLogManagerInternalDeallocateGlobalUploadData(void) {
  for (uint32_t i = 0; i < kEsfLogManagerBlockTypeAll; i++) {
    if (pthread_mutex_lock(&s_local_upload_info.m_mutex) != 0) {
      ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_lock.\n");
      return kEsfLogManagerStatusFailed;
    }
    if (s_local_upload_info.upload_group[i].storage_name != NULL) {
      free(s_local_upload_info.upload_group[i].storage_name);
      s_local_upload_info.upload_group[i].storage_name = NULL;
    }
    if (s_local_upload_info.upload_group[i].storage_path != NULL) {
      free(s_local_upload_info.upload_group[i].storage_path);
      s_local_upload_info.upload_group[i].storage_path = NULL;
    }
    if (pthread_mutex_unlock(&s_local_upload_info.m_mutex) != 0) {
      ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_unlock.\n");
      return kEsfLogManagerStatusFailed;
    }

    if (pthread_mutex_lock(&s_cloud_upload_info.m_mutex) != 0) {
      ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_lock.\n");
      return kEsfLogManagerStatusFailed;
    }
    if (s_cloud_upload_info.upload_group[i].storage_name != NULL) {
      free(s_cloud_upload_info.upload_group[i].storage_name);
      s_cloud_upload_info.upload_group[i].storage_name = NULL;
    }
    if (s_cloud_upload_info.upload_group[i].storage_path != NULL) {
      free(s_cloud_upload_info.upload_group[i].storage_path);
      s_cloud_upload_info.upload_group[i].storage_path = NULL;
    }
    if (pthread_mutex_unlock(&s_cloud_upload_info.m_mutex) != 0) {
      ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_unlock.\n");
      return kEsfLogManagerStatusFailed;
    }
  }

  return kEsfLogManagerStatusOk;
}

STATIC EsfLogManagerStatus
EsfLogManagerInternalSetGlobalUploadDataForStorageName(
    UploadTypeT upload_type, EsfLogManagerSettingBlockType const block_type,
    char const *str) {
  if (upload_type == kLocalUpload) {
    if (pthread_mutex_lock(&s_local_upload_info.m_mutex) != 0) {
      ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_lock.\n");
      return kEsfLogManagerStatusFailed;
    }

    strncpy(s_local_upload_info.upload_group[block_type].storage_name, str,
            ESF_LOG_MANAGER_STORAGE_NAME_MAX_SIZE);

    if (pthread_mutex_unlock(&s_local_upload_info.m_mutex) != 0) {
      ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_unlock.\n");
      return kEsfLogManagerStatusFailed;
    }

  } else {
    if (pthread_mutex_lock(&s_cloud_upload_info.m_mutex) != 0) {
      ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_lock.\n");
      return kEsfLogManagerStatusFailed;
    }

    strncpy(s_cloud_upload_info.upload_group[block_type].storage_name, str,
            ESF_LOG_MANAGER_STORAGE_NAME_MAX_SIZE);

    if (pthread_mutex_unlock(&s_cloud_upload_info.m_mutex) != 0) {
      ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_unlock.\n");
      return kEsfLogManagerStatusFailed;
    }
  }

  return kEsfLogManagerStatusOk;
}

STATIC EsfLogManagerStatus
EsfLogManagerInternalSetGlobalUploadDataForStoragePath(
    EsfLogManagerSettingBlockType const block_type, char const *str) {
  if (pthread_mutex_lock(&s_local_upload_info.m_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_lock.\n");
    return kEsfLogManagerStatusFailed;
  }

  strncpy(s_local_upload_info.upload_group[block_type].storage_path, str,
          ESF_LOG_MANAGER_STORAGE_SUB_DIR_PATH_MAX_SIZE);

  if (pthread_mutex_unlock(&s_local_upload_info.m_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_unlock.\n");
    return kEsfLogManagerStatusFailed;
  }

  if (pthread_mutex_lock(&s_cloud_upload_info.m_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_lock.\n");
    return kEsfLogManagerStatusFailed;
  }

  strncpy(s_cloud_upload_info.upload_group[block_type].storage_path, str,
          ESF_LOG_MANAGER_STORAGE_SUB_DIR_PATH_MAX_SIZE);

  if (pthread_mutex_unlock(&s_cloud_upload_info.m_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_unlock.\n");
    return kEsfLogManagerStatusFailed;
  }

  return kEsfLogManagerStatusOk;
}
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE

#ifdef LOG_MANAGER_ENCRYPT_ENABLE
STATIC EsfLogManagerStatus EsfLogManagerInternalEncryptLog(size_t data_size,
                                                           size_t buf_size,
                                                           uint8_t *data,
                                                           uint32_t *out_size) {
  EsfSecUtilStatus ret = EsfSecUtilInit();
  if (kEsfSecUtilSuccess != ret) {
    ESF_LOG_MANAGER_ERROR("Failed to init sec util. ret=0x%x\n", ret);
    return kEsfLogManagerStatusFailed;
  }
  ret = EsfSecUtilEncryptDetailLog((uint8_t *)data, (uint32_t)data_size,
                                   buf_size, out_size);
  if (kEsfSecUtilSuccess != ret) {
    ESF_LOG_MANAGER_ERROR("Failed to encrypt detail log. ret=0x%x\n", ret);
    return kEsfLogManagerStatusFailed;
  }

  ret = EsfSecUtilDeinit();
  if (kEsfSecUtilSuccess != ret) {
    ESF_LOG_MANAGER_ERROR("Failed to deinit sec util. ret=0x%x\n", ret);
    return kEsfLogManagerStatusFailed;
  }

  return kEsfLogManagerStatusOk;
}
#endif  // LOG_MANAGER_ENCRYPT_ENABLE

STATIC void *EsfLogWriteElogThread(void *p __attribute__((unused))) {
  bool client_reg_req_flag = false;
  struct MessagePassingElogObjT msg_obj = {
      .m_cmd = kCmdIsElogNon,
      .m_len_of_data = (size_t)(sizeof(struct MessagePassingElogObjT))};
  int32_t recv_size = 0;
  EsfLogManagerStatus ret = kEsfLogManagerStatusOk;
  for (;;) {
    UtilityMsgErrCode utility_ret = UtilityMsgRecv(
        s_elog_msg_passing.m_handle, (void *)&msg_obj,
        sizeof(struct MessagePassingElogObjT),
        LOG_MANAGER_INTERNAL_ELOG_MSG_TIMEOUT, (int32_t *)&recv_size);
    if (utility_ret != kUtilityMsgOk) {
      ESF_LOG_MANAGER_ERROR(
          "Failed to UtilityMsgRecv. Handle is s_elog_msg_passing.m_handle. "
          "ret=%d\n",
          utility_ret);
      continue;
    }

    if (pthread_mutex_lock(&sp_elog_mutex) != 0) {
      ESF_LOG_MANAGER_ERROR("Mutex lock failed\n");
    }
    s_elog_queue_cnt--;
    if (pthread_mutex_unlock(&sp_elog_mutex) != 0) {
      ESF_LOG_MANAGER_ERROR("Mutex unlock failed\n");
    }

    if (msg_obj.m_cmd == kCmdIsRegister) {
      if (s_elog_sys_client == NULL) {
        s_elog_sys_client = EVP_Agent_register_sys_client();
        if (s_elog_sys_client == NULL) {
          // Send register command to register client again.
          sleep(1);
          struct MessagePassingElogObjT register_obj = {
              .m_cmd = kCmdIsRegister,
              .m_len_of_data = (size_t)(sizeof(struct MessagePassingElogObjT))};
          ret = EsfLogManagerInternalSendCmdForceToElogThread(&register_obj);
          if (ret != kEsfLogManagerStatusOk) {
            ESF_LOG_MANAGER_ERROR(
                "Failed to send cmd force to Elog thread. ret=%d\n", ret);
          }
        } else {
          ESF_LOG_MANAGER_INFO("Waiting connect to Hub\n");
          struct MessagePassingElogObjT wait_obj = {
              .m_cmd = kCmdIsWait,
              .m_len_of_data = (size_t)(sizeof(struct MessagePassingElogObjT))};
          ret = EsfLogManagerInternalSendCmdForceToElogThread(&wait_obj);
          if (ret != kEsfLogManagerStatusOk) {
            ESF_LOG_MANAGER_ERROR("Failed to send cmd to Elog thread. ret=%d\n",
                                  ret);
          }
        }
      }
    }

    if (msg_obj.m_cmd == kCmdIsWait) {
      if (EVP_getAgentStatus() != EVP_AGENT_STATUS_CONNECTED) {
        sleep(1);
        struct MessagePassingElogObjT wait_obj = {
            .m_cmd = kCmdIsWait,
            .m_len_of_data = (size_t)(sizeof(struct MessagePassingElogObjT))};
        ret = EsfLogManagerInternalSendCmdForceToElogThread(&wait_obj);
        if (ret != kEsfLogManagerStatusOk) {
          ESF_LOG_MANAGER_ERROR("Failed to send cmd to Elog thread. ret=%d\n",
                                ret);
        }
      } else {
        client_reg_req_flag = false;

        char *pstr = NULL;
        ret = EsfLogManagerInternalLoadElog(&pstr);
        if (ret != kEsfLogManagerStatusOk) {
          ESF_LOG_MANAGER_ERROR("Failed to load Elog message. ret=%d\n", ret);
          continue;
        }

        if (pstr != NULL) {
          struct MessagePassingElogObjT resend_obj = {
              .m_cmd = kCmdIsResend,
              .m_len_of_data = (size_t)(sizeof(struct MessagePassingElogObjT)),
              .message = pstr};
          ret = EsfLogManagerInternalSendCmdForceToElogThread(&resend_obj);
          if (ret != kEsfLogManagerStatusOk) {
            ESF_LOG_MANAGER_ERROR("Failed to send cmd to Elog thread. ret=%d\n",
                                  ret);
          }
        }
      }
    }

    if (msg_obj.m_cmd == kCmdIsResend) {
      if (s_elog_sys_client != NULL &&
          EVP_getAgentStatus() == EVP_AGENT_STATUS_CONNECTED) {
        enum SYS_result result = SYS_send_telemetry(
            s_elog_sys_client, LOG_MANAGER_INTERNAL_ELOG_TOPIC, msg_obj.message,
            EsfLogManagerInternalElogCallback, (void *)msg_obj.message);
        if (result != SYS_RESULT_OK) {
          ESF_LOG_MANAGER_ERROR("Failed to send telemetry. ret=%d\n", result);
          free(msg_obj.message);
        }
        result = SYS_process_event(s_elog_sys_client, -1);
        if (result != SYS_RESULT_OK) {
          ESF_LOG_MANAGER_ERROR("SYS_process_event ret=%d\n", result);
        }
      } else {
        ESF_LOG_MANAGER_ERROR("Elog resend failed\n");
        if (msg_obj.message != NULL) {
          ret = EsfLogManagerInternalSaveElog(msg_obj.message);
          free(msg_obj.message);
          if (ret != kEsfLogManagerStatusOk) {
            ESF_LOG_MANAGER_ERROR("Save Elog message failed. ret=%d\n", ret);
          }
        }

        if (!client_reg_req_flag) {
          struct MessagePassingElogObjT register_obj = {
              .m_cmd = kCmdIsRegister,
              .m_len_of_data = (size_t)(sizeof(struct MessagePassingElogObjT))};
          ret = EsfLogManagerInternalSendCmdForceToElogThread(&register_obj);
          if (ret != kEsfLogManagerStatusOk) {
            ESF_LOG_MANAGER_ERROR("Failed to send cmd to Elog thread. ret=%d\n",
                                  ret);
          }
          client_reg_req_flag = true;
        }
      }
    }

    if (msg_obj.m_cmd == kCmdIsSend) {
      if (s_elog_sys_client != NULL &&
          EVP_getAgentStatus() == EVP_AGENT_STATUS_CONNECTED) {
        enum SYS_result result = SYS_send_telemetry(
            s_elog_sys_client, LOG_MANAGER_INTERNAL_ELOG_TOPIC, msg_obj.message,
            EsfLogManagerInternalElogCallback, (void *)msg_obj.message);
        if (result != SYS_RESULT_OK) {
          ESF_LOG_MANAGER_ERROR("Failed to send telemetry. ret=%d\n", result);
          free(msg_obj.message);
        }
        result = SYS_process_event(s_elog_sys_client, -1);
        if (result != SYS_RESULT_OK) {
          ESF_LOG_MANAGER_ERROR("SYS_process_event ret=%d\n", result);
        }
      } else {
        // Temporary: Buffer is used to save message.
        // Todo: Save message to Flash.
        if (msg_obj.message != NULL) {
          ret = EsfLogManagerInternalSaveElog(msg_obj.message);
          free(msg_obj.message);
          if (ret != kEsfLogManagerStatusOk) {
            ESF_LOG_MANAGER_ERROR("Save Elog message failed. ret=%d\n", ret);
          }
        }
        if (!client_reg_req_flag) {
          struct MessagePassingElogObjT register_obj = {
              .m_cmd = kCmdIsRegister,
              .m_len_of_data = (size_t)(sizeof(struct MessagePassingElogObjT))};
          ret = EsfLogManagerInternalSendCmdForceToElogThread(&register_obj);
          if (ret != kEsfLogManagerStatusOk) {
            ESF_LOG_MANAGER_ERROR("Failed to send cmd to Elog thread. ret=%d\n",
                                  ret);
          }
          client_reg_req_flag = true;
        }
      }
    }

    if (msg_obj.m_cmd == kCmdIsDestroyElogThread) {
      LOG_MANAGER_TRACE_PRINT(":Thread Fin\n");
      break;
    }
    msg_obj.m_cmd = kCmdIsElogNon;
  }
  LOG_MANAGER_TRACE_PRINT(":Thread exit\n");
#ifndef LOG_MANAGER_LOCAL_BUILD
  pthread_exit((void *)NULL);
#endif
  LOG_MANAGER_TRACE_PRINT(":Thread exit\n");

  return (void *)NULL;
}

STATIC void EsfLogManagerInternalElogCallback(struct SYS_client *client,
                                              enum SYS_callback_reason reason,
                                              void *user) {
  EsfLogManagerStatus ret = kEsfLogManagerStatusOk;
  switch (reason) {
    case SYS_REASON_FINISHED:
      LOG_MANAGER_TRACE_PRINT("Send Telemetry is done, Sent Message is %s\n",
                              (char *)user);
      free(user);

      char *pstr = NULL;
      ret = EsfLogManagerInternalLoadElog(&pstr);
      if (ret != kEsfLogManagerStatusOk) {
        ESF_LOG_MANAGER_ERROR("Failed to load Elog message. ret=%d\n", ret);
        break;
      }

      if (pstr != NULL) {
        struct MessagePassingElogObjT resend_obj = {
            .m_cmd = kCmdIsResend,
            .m_len_of_data = (size_t)(sizeof(struct MessagePassingElogObjT)),
            .message = pstr};
        ret = EsfLogManagerInternalSendCmdToElogThread(&resend_obj);
        if (ret != kEsfLogManagerStatusOk) {
          ESF_LOG_MANAGER_ERROR("Failed to send Resend cmd. ret=%d\n", ret);
          free(pstr);
          break;
        }
      }
      break;

    case SYS_REASON_ERROR:
      ESF_LOG_MANAGER_ERROR("Send Telemetry is Error.\n");
      if (user != NULL) {
        ret = EsfLogManagerInternalSaveElog((char *)user);
        free(user);
        if (ret != kEsfLogManagerStatusOk) {
          ESF_LOG_MANAGER_ERROR("Failed to save Elog. ret=%d\n", ret);
        }
      }
      break;

    default:
      ESF_LOG_MANAGER_ERROR("Unexpected telemetry error occurred.\n");
      free(user);
      break;
  }
}

STATIC EsfLogManagerStatus EsfLogManagerInternalCreateElogThread(void) {
  pthread_attr_t elog_attr;
  int ret = 0;
  ret = pthread_attr_init(&elog_attr);
  if (ret != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_attr_init. ret=%d\n", ret);
    return kEsfLogManagerStatusFailed;
  }

#ifndef LOG_MANAGER_LOCAL_BUILD
#ifdef __NuttX__
  size_t elog_stack_size = LOG_MANAGER_ELOG_THREAD_STACK_SIZE;
  s_elog_thread_stack = malloc(elog_stack_size);
  if (s_elog_thread_stack == NULL) {
    ESF_LOG_MANAGER_ERROR("Elog thread allocate memory failed \n");
    return kEsfLogManagerStatusFailed;
  }

  ret = pthread_attr_setstack(&elog_attr, s_elog_thread_stack, elog_stack_size);
  if (ret != 0) {
    free(s_elog_thread_stack);
    s_elog_thread_stack = NULL;

    ESF_LOG_MANAGER_ERROR("Failed to pthread_attr_setstack. ret=%d\n", ret);
    return kEsfLogManagerStatusFailed;
  }
#endif  // __NuttX__
#endif  // LOG_MANAGER_LOCAL_BUILD
  ret = pthread_create(&s_elog_thread_tid, &elog_attr, EsfLogWriteElogThread,
                       NULL);
  if (ret != 0) {
    free(s_elog_thread_stack);
    s_elog_thread_stack = NULL;
    ESF_LOG_MANAGER_ERROR("Failed to pthread_create. ret=%d\n", ret);
    return kEsfLogManagerStatusFailed;
  }

#ifndef LOG_MANAGER_LOCAL_BUILD

  ret = pthread_setname_np(s_elog_thread_tid, "ElogThread");
  if (ret != 0) {
    free(s_elog_thread_stack);
    s_elog_thread_stack = NULL;
    ESF_LOG_MANAGER_ERROR("Failed to pthread_setname_np. ret=%d\n", ret);
    return kEsfLogManagerStatusFailed;
  }
#endif

  ret = pthread_attr_destroy(&elog_attr);
  if (ret != 0) {
    free(s_elog_thread_stack);
    s_elog_thread_stack = NULL;
    ESF_LOG_MANAGER_ERROR("Failed to pthread_attr_destroy. ret=%d\n", ret);
    return kEsfLogManagerStatusFailed;
  }

  return kEsfLogManagerStatusOk;
}

STATIC EsfLogManagerStatus EsfLogManagerInternalSendCmdToElogThread(
    const struct MessagePassingElogObjT *const msg_obj) {
  UtilityMsgErrCode ret = kUtilityMsgOk;
  int32_t sent_size = 0;

  if (pthread_mutex_lock(&sp_elog_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_lock.\n");
    return kEsfLogManagerStatusFailed;
  }

  // Allocate a queue space to destroy thread.
  if (s_elog_queue_cnt < LOG_MANAGER_INTERNAL_ELOG_QUEUE_NUM) {
    s_elog_queue_cnt++;
  } else {
    ESF_LOG_MANAGER_WARN("Elog Message queue is full.\n");
    (void)pthread_mutex_unlock(&sp_elog_mutex);
    return kEsfLogManagerStatusFailed;
  }

  if (pthread_mutex_unlock(&sp_elog_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_unlock.\n");
    s_elog_queue_cnt--;
    return kEsfLogManagerStatusFailed;
  }

  ret = UtilityMsgSend(s_elog_msg_passing.m_handle, (void *)msg_obj,
                       (uint32_t)msg_obj->m_len_of_data, msg_obj->m_cmd,
                       (int32_t *)&sent_size);
  if (ret != kUtilityMsgOk) {
    ESF_LOG_MANAGER_ERROR("Failed send msg:%d\n", ret);

    (void)pthread_mutex_lock(&sp_elog_mutex);
    s_elog_queue_cnt--;
    (void)pthread_mutex_unlock(&sp_elog_mutex);

    return kEsfLogManagerStatusFailed;
  }

  return kEsfLogManagerStatusOk;
}

STATIC EsfLogManagerStatus EsfLogManagerInternalSendCmdForceToElogThread(
    const struct MessagePassingElogObjT *const msg_obj) {
  UtilityMsgErrCode ret = kUtilityMsgOk;
  int32_t sent_size = 0;

  if (pthread_mutex_lock(&sp_elog_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_lock.\n");
    return kEsfLogManagerStatusFailed;
  }

  bool send_flag = false;
  if (s_elog_queue_cnt < LOG_MANAGER_INTERNAL_MSG_QUEUE_SIZE) {
    s_elog_queue_cnt++;
    send_flag = true;
  }

  if (pthread_mutex_unlock(&sp_elog_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_unlock.\n");
    s_elog_queue_cnt--;
    return kEsfLogManagerStatusFailed;
  }

  if (send_flag) {
    ret = UtilityMsgSend(s_elog_msg_passing.m_handle, (void *)msg_obj,
                         (uint32_t)msg_obj->m_len_of_data, msg_obj->m_cmd,
                         (int32_t *)&sent_size);
    if (ret != kUtilityMsgOk) {
      ESF_LOG_MANAGER_ERROR("Failed force send msg:%d\n", ret);

      (void)pthread_mutex_lock(&sp_elog_mutex);
      s_elog_queue_cnt--;
      (void)pthread_mutex_unlock(&sp_elog_mutex);

      return kEsfLogManagerStatusFailed;
    }
  }

  return kEsfLogManagerStatusOk;
}

static EsfLogManagerStatus EsfLogManagerInternalDestroyElogThread(void) {
  EsfLogManagerStatus ret = kEsfLogManagerStatusOk;
  int ret_thread = 0;
  struct MessagePassingElogObjT msg_obj = {
      .m_cmd = kCmdIsDestroyElogThread,
      .m_len_of_data = (size_t)(sizeof(struct MessagePassingElogObjT))};
  ret = EsfLogManagerInternalSendCmdForceToElogThread(&msg_obj);
  if (ret != kEsfLogManagerStatusOk) {
    ESF_LOG_MANAGER_ERROR("Failed send thread destroy:%d\n", ret);
    return kEsfLogManagerStatusFailed;
  }
#if 1
  /** Destroy Elog thread. */
  if (pthread_equal(s_elog_thread_tid,
                    ESF_LOG_MANAGER_INTERNAL_INVALID_THREAD_ID) == 0) {
    // pthread end wait...
#ifndef LOG_MANAGER_LOCAL_BUILD
    LOG_MANAGER_TRACE_PRINT("\n");
    ret_thread = pthread_join(s_elog_thread_tid, (void **)NULL);
    LOG_MANAGER_TRACE_PRINT("\n");
    if (ret_thread != 0) {
      ESF_LOG_MANAGER_ERROR("pthread_join error %d\n", ret_thread);
    }
    s_elog_thread_tid = ESF_LOG_MANAGER_INTERNAL_INVALID_THREAD_ID;
#endif /* #ifndef LOG_MANAGER_LOCAL_BUILD */
  }
#endif
  LOG_MANAGER_TRACE_PRINT("\n");

  return kEsfLogManagerStatusOk;
}

EsfLogManagerStatus EsfLogManagerInternalSendElog(
    const EsfLogManagerElogMessage *message) {
  EsfLogManagerStatus ret = kEsfLogManagerStatusOk;

  if (message->elog_level >
      s_parameter.m_value[kEsfLogManagerBlockTypeSysApp].elog_level) {
    return kEsfLogManagerStatusOk;
  }

  char *pstr = NULL;
  ret = EsfLogManagerInternalSerializeElogJson(message, &pstr);
  if (ret != kEsfLogManagerStatusOk) {
    ESF_LOG_MANAGER_ERROR("serialize json failed:%d\n", ret);
    return kEsfLogManagerStatusFailed;
  }

  struct MessagePassingElogObjT msg_obj = {
      .m_cmd = kCmdIsSend,
      .m_len_of_data = (size_t)(sizeof(struct MessagePassingElogObjT)),
      .message = pstr};

  ret = EsfLogManagerInternalSendCmdToElogThread(&msg_obj);
  if (ret != kEsfLogManagerStatusOk) {
    ESF_LOG_MANAGER_WARN("InternalSendElog:%d\n", ret);
    free(pstr);
    return kEsfLogManagerStatusFailed;
  }

  return kEsfLogManagerStatusOk;
}

STATIC EsfLogManagerStatus EsfLogManagerInternalSerializeElogJson(
    const EsfLogManagerElogMessage *message, char **pstr) {
  EsfJsonHandle handle;
  EsfJsonErrorCode json_result = kEsfJsonInternalError;
  const char *str;
  json_result = EsfJsonOpen(&handle);
  if (json_result != kEsfJsonSuccess) {
    ESF_LOG_MANAGER_ERROR("Failed to EsfJsonOpen. ret=%d\n", json_result);
    return kEsfLogManagerStatusFailed;
  }

  EsfJsonValue elog_object_id = -1;
  json_result = EsfJsonObjectInit(handle, &elog_object_id);
  if (json_result != kEsfJsonSuccess) {
    EsfJsonClose(handle);
    ESF_LOG_MANAGER_ERROR("Failed to EsfJsonObjectInit. ret=%d\n", json_result);
    return kEsfLogManagerStatusFailed;
  }

  EsfJsonValue serial_object_id = -1;
  if ((json_result = EsfJsonStringInit(handle, s_hw_info.serial_number,
                                       &serial_object_id)) != kEsfJsonSuccess ||
      ((json_result = EsfJsonObjectSet(handle, elog_object_id, "serial",
                                       serial_object_id)) != kEsfJsonSuccess)) {
    EsfJsonClose(handle);
    ESF_LOG_MANAGER_ERROR("Failed to set Json object for serial. ret=%d\n",
                          json_result);
    return kEsfLogManagerStatusFailed;
  }
  EsfJsonValue level_object_id = -1;
  if ((json_result = EsfJsonIntegerInit(handle, (int32_t)message->elog_level,
                                        &level_object_id)) != kEsfJsonSuccess ||
      ((json_result = EsfJsonObjectSet(handle, elog_object_id, "level",
                                       level_object_id)) != kEsfJsonSuccess)) {
    EsfJsonClose(handle);
    ESF_LOG_MANAGER_ERROR("Failed to set Json object for level. ret=%d\n",
                          json_result);
    return kEsfLogManagerStatusFailed;
  }

  EsfJsonValue time_object_id = -1;
  if ((json_result = EsfJsonStringInit(handle, message->time,
                                       &time_object_id)) != kEsfJsonSuccess ||
      ((json_result = EsfJsonObjectSet(handle, elog_object_id, "timestamp",
                                       time_object_id)) != kEsfJsonSuccess)) {
    EsfJsonClose(handle);
    ESF_LOG_MANAGER_ERROR("Failed to set Json object for timestamp. ret=%d\n",
                          json_result);
    return kEsfLogManagerStatusFailed;
  }

  EsfJsonValue component_object_id = -1;
  if ((json_result = EsfJsonIntegerInit(handle, (int32_t)message->component_id,
                                        &component_object_id)) !=
          kEsfJsonSuccess ||
      ((json_result = EsfJsonObjectSet(handle, elog_object_id, "component_id",
                                       component_object_id)) !=
       kEsfJsonSuccess)) {
    EsfJsonClose(handle);
    ESF_LOG_MANAGER_ERROR(
        "Failed to set Json object for component_id. ret=%d\n", json_result);
    return kEsfLogManagerStatusFailed;
  }

  EsfJsonValue event_object_id = -1;
  if ((json_result = EsfJsonIntegerInit(handle, (int32_t)message->event_id,
                                        &event_object_id)) != kEsfJsonSuccess ||
      ((json_result = EsfJsonObjectSet(handle, elog_object_id, "event_id",
                                       event_object_id)) != kEsfJsonSuccess)) {
    EsfJsonClose(handle);
    ESF_LOG_MANAGER_ERROR("Failed to set Json object for event_id. ret=%d\n",
                          json_result);
    return kEsfLogManagerStatusFailed;
  }

  json_result = EsfJsonSerialize(handle, elog_object_id, &str);
  if (json_result != kEsfJsonSuccess) {
    EsfJsonClose(handle);
    ESF_LOG_MANAGER_ERROR("Json serialize failed. ret=%d\n", json_result);
    return kEsfLogManagerStatusFailed;
  }

  *pstr = (char *)malloc((strlen(str) + 1) * sizeof(char));
  if (*pstr == NULL) {
    EsfJsonClose(handle);
    ESF_LOG_MANAGER_ERROR("Memory allocation failed\n");
    return kEsfLogManagerStatusFailed;
  }

  strncpy(*pstr, str, strlen(str));
  (*pstr)[strlen(str)] = '\0';

  EsfJsonSerializeFree(handle);
  EsfJsonClose(handle);

  return kEsfLogManagerStatusOk;
}

STATIC EsfLogManagerStatus EsfLogManagerInternalSaveElog(const char *pstr) {
  EsfLogManagerStatus result = kEsfLogManagerStatusOk;

  if (pthread_mutex_lock(&s_elog_save_message_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_lock.\n");
    result = kEsfLogManagerStatusFailed;
    goto exit;
  }

  if (s_elog_save_message_cnt >= LOG_MANAGER_INTERNAL_ELOG_SAVE_NUM) {
    LOG_MANAGER_TRACE_PRINT("Elog saved message is full\n");
    result = kEsfLogManagerStatusOk;
    goto unlock;
  }

  s_elog_save_message[s_elog_save_message_cnt] =
      (char *)malloc((strlen(pstr) + 1) * sizeof(char));
  if (s_elog_save_message[s_elog_save_message_cnt] == NULL) {
    ESF_LOG_MANAGER_ERROR("Memory allocation failed\n");
    result = kEsfLogManagerStatusFailed;
    goto unlock;
  }

  strncpy(s_elog_save_message[s_elog_save_message_cnt], pstr, strlen(pstr));
  s_elog_save_message[s_elog_save_message_cnt][strlen(pstr)] = '\0';
  s_elog_save_message_cnt++;

  LOG_MANAGER_TRACE_PRINT("Elog save message Done\n");

unlock:
  if (pthread_mutex_unlock(&s_elog_save_message_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_unlock.\n");
    result = kEsfLogManagerStatusFailed;
  }

exit:
  return result;
}

STATIC EsfLogManagerStatus EsfLogManagerInternalLoadElog(char **pstr) {
  EsfLogManagerStatus result = kEsfLogManagerStatusOk;

  if (pthread_mutex_lock(&s_elog_save_message_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_lock.\n");
    result = kEsfLogManagerStatusFailed;
    goto exit;
  }

  if (s_elog_save_message_cnt > 0) {
    *pstr = (char *)malloc((strlen(s_elog_save_message[0]) + 1) * sizeof(char));
    if (*pstr == NULL) {
      ESF_LOG_MANAGER_ERROR("Memory allocation failed\n");
      result = kEsfLogManagerStatusFailed;
      goto unlock;
    }

    strncpy(*pstr, s_elog_save_message[0], strlen(s_elog_save_message[0]));
    (*pstr)[strlen(s_elog_save_message[0])] = '\0';

    free(s_elog_save_message[0]);

    for (int i = 1; i < s_elog_save_message_cnt; i++) {
      s_elog_save_message[i - 1] = s_elog_save_message[i];
    }
    s_elog_save_message_cnt--;
  }

unlock:
  if (pthread_mutex_unlock(&s_elog_save_message_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_unlock.\n");
    free(*pstr);
    *pstr = NULL;
    result = kEsfLogManagerStatusFailed;
  }

exit:
  return result;
}

EsfLogManagerStatus EsfLogManagerInternalClearElog(void) {
  if (pthread_mutex_lock(&s_elog_save_message_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_lock.\n");
    return kEsfLogManagerStatusFailed;
  }

  if (s_elog_save_message_cnt > 0) {
    for (int i = 0; i < s_elog_save_message_cnt; i++) {
      free(s_elog_save_message[i]);
      s_elog_save_message[i] = NULL;
    }
    s_elog_save_message_cnt = 0;
  }

  if (pthread_mutex_unlock(&s_elog_save_message_mutex) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_unlock.\n");
    return kEsfLogManagerStatusFailed;
  }

  return kEsfLogManagerStatusOk;
}

#ifndef CONFIG_EXTERNAL_DLOG_DISABLE
STATIC EsfLogManagerStatus EsfLogManagerInternalHandleCriticalLogTiming(void) {
  bool should_upload = false;

  if (pthread_mutex_lock(&s_mutex_for_bytebuffer) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_lock.\n");
    return kEsfLogManagerStatusFailed;
  }

  // after locking the byte buffer mutex
  if (pthread_mutex_lock(&s_critical_log_mutex) != 0) {
    (void)pthread_mutex_unlock(&s_mutex_for_bytebuffer);
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_lock.\n");
    return kEsfLogManagerStatusFailed;
  }

  if (s_critical_log_pending) {
    struct timespec current_time;
    clock_gettime(CLOCK_REALTIME, &current_time);

    // Calculate time difference in seconds
    time_t time_diff = current_time.tv_sec - s_last_critical_log_time.tv_sec;

    if (time_diff >= CONFIG_EXTERNAL_LOG_MANAGER_CRITICAL_UPLOAD_TIMEOUT) {
      should_upload = true;
    }
  }

  if (should_upload) {
    EsfLogManagerStatus ret = EsfLogManagerInternalChangeDlogByteBuffer();
    if (ret != kEsfLogManagerStatusOk) {
      (void)pthread_mutex_unlock(&s_critical_log_mutex);
      (void)pthread_mutex_unlock(&s_mutex_for_bytebuffer);
      ESF_LOG_MANAGER_ERROR("Failed to change Dlog ByteBuffer. ret=%d\n", ret);
      return kEsfLogManagerStatusFailed;
    }

    s_critical_log_pending = false;
  }

  if (pthread_mutex_unlock(&s_critical_log_mutex) != 0) {
    (void)pthread_mutex_unlock(&s_mutex_for_bytebuffer);
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_unlock.\n");
    return kEsfLogManagerStatusFailed;
  }

  if (pthread_mutex_unlock(&s_mutex_for_bytebuffer) != 0) {
    ESF_LOG_MANAGER_ERROR("Failed to pthread_mutex_unlock.\n");
    return kEsfLogManagerStatusFailed;
  }

  if (should_upload) {
    // Trigger immediate upload for critical logs
    ESF_LOG_MANAGER_INFO(
        "Critical log upload timeout reached, triggering upload.\n");

    EsfLogManagerStatus ret = EsfLogManagerInternalDlogFullCollector();
    if (ret != kEsfLogManagerStatusOk) {
      ESF_LOG_MANAGER_ERROR("Failed to process about Dlog full. ret=%d\n", ret);
      return kEsfLogManagerStatusFailed;
    }
  }

  return kEsfLogManagerStatusOk;
}
#endif  // CONFIG_EXTERNAL_DLOG_DISABLE
