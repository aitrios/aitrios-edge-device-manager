/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "log_manager_metrics.h"

#include <ctype.h>
#include <dirent.h>
#include <evp/sdk_sys.h>
#include <limits.h>
#ifdef __NuttX__
#include <nuttx/kmalloc.h>
#else
#include <malloc.h>
#include <stdlib.h>
#endif  // __NuttX__
#include <pthread.h>
#include <sched.h>
#include <sdk_backdoor.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "json.h"
#include "pl.h"
#include "pl_dmamem.h"
#include "pl_lheap.h"
#include "utility_msg.h"
#include "wasm_export.h"

#define LOG_MANAGER_METRICS_DATE_SIZE 15
#define LOG_MANAGER_METRICS_PROC_VALUE_SIZE 128
#define LOG_MANAGER_METRICS_PERCENT_SIZE 7

#define LOG_MANAGER_METRICS_VERSION "v0.0.2"
#define LOG_MANAGER_METRICS_TOPIC "metrics"

#define LOG_MANAGER_METRICS_MEM_KEY_TOTAL "total"
#define LOG_MANAGER_METRICS_MEM_KEY_TOTAL_USED "total_used"
#define LOG_MANAGER_METRICS_MEM_KEY_TOTAL_FREE "total_free"
#define LOG_MANAGER_METRICS_MEM_KEY_LINER_MAXFREE "linear_maxfree"

#define LOG_MANAGER_METRICS_STACK_SIZE (20)

struct meminfo {
  int32_t total;
  int32_t total_used;
  int32_t total_free;
  int32_t linear_maxfree;
};

typedef struct {
  char stack_alloc[LOG_MANAGER_METRICS_STACK_SIZE];
  char stack_base[LOG_MANAGER_METRICS_STACK_SIZE];
  int32_t stack_size;
  int32_t stack_used;
} EsfLogManagerStackInfo;

static EsfLogManagerStatus EsfLogManagerCreateMetricsGenerationThread(void);
static EsfLogManagerStatus EsfLogManagerCreateMetricsSendingThread(void);
static void *EsfLogManagerMetricsGenerationThread(void *p);
static void *EsfLogManagerMetricsSendingThread(void *p);

static EsfLogManagerStatus EsfLogManagerGenerateMsg(
    EsfJsonHandle json_handle, const char **serialized_string);
static EsfLogManagerStatus EsfLogManagerGenerateMsgFormat(
    EsfJsonHandle json_handle, EsfJsonValue *metrics_object_id);
static EsfLogManagerStatus EsfLogManagerGenerateCpuInfo(
    EsfJsonHandle json_handle, EsfJsonValue *metrics_object_id);
static EsfLogManagerStatus EsfLogManagerGenerateMemInfo(
    EsfJsonHandle json_handle, EsfJsonValue *metrics_object_id);
static EsfLogManagerStatus EsfLogManagerGenerateStackInfo(
    EsfJsonHandle json_handle, EsfJsonValue *metrics_object_id);
static EsfLogManagerStatus EsfLogManagerOpenProc(char *buf, char *filepath);
static EsfLogManagerStatus EsfLogManagerReadStackInfo(
    const char *filepath, EsfLogManagerStackInfo *stack_info);
static EsfLogManagerStatus EsfLogManagerGetKernelHeap(EsfJsonHandle json_handle,
                                                      EsfJsonValue *mem_obj);
static EsfLogManagerStatus EsfLogManagerGetNormalHeap(EsfJsonHandle json_handle,
                                                      EsfJsonValue *mem_obj);
static EsfLogManagerStatus EsfLogManagerGetWasmHeap(EsfJsonHandle json_handle,
                                                    EsfJsonValue *mem_obj);
static EsfLogManagerStatus EsfLogManagerGetLargeHeap(EsfJsonHandle json_handle,
                                                     EsfJsonValue *mem_obj);
static EsfLogManagerStatus EsfLogManagerGetDmaMemory(EsfJsonHandle json_handle,
                                                     EsfJsonValue *mem_obj);
static EsfLogManagerStatus EsfLogManagerMetricsSetMemJsonObject(
    EsfJsonHandle json_handle, EsfJsonValue *mem_obj, char *mem_name,
    struct meminfo *mem_info);
static EsfLogManagerStatus EsfLogManagerMetricsSetStackJsonObject(
    EsfJsonHandle json_handle, EsfJsonValue *stack_obj, char *cpu_command,
    EsfLogManagerStackInfo *stack_info);

static void TelemetryCb(struct SYS_client *c, enum SYS_callback_reason reason,
                        void *user);

static pthread_t s_metrics_create_tid =
    ESF_LOG_MANAGER_METRICS_INVALID_THREAD_ID;
static void *s_metrics_create_thread_stack = (void *)NULL;

static pthread_t s_metrics_send_tid = ESF_LOG_MANAGER_METRICS_INVALID_THREAD_ID;
static void *s_metrics_send_thread_stack = (void *)NULL;

static pthread_mutex_t s_metrics_generate_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t s_metrics_generate_cond = PTHREAD_COND_INITIALIZER;

static pthread_mutex_t s_metrics_send_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t s_metrics_send_cond = PTHREAD_COND_INITIALIZER;

static bool s_metrics_loop_generate, s_metrics_loop_send;

static int32_t s_queue_handle;

static struct SYS_client *s_metrics_sys_client = NULL;

// """ Initialize metrics
// Args:
//    none
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination
EsfLogManagerStatus EsfLogManagerInitializeMetrics(void) {
  EsfLogManagerStatus ret = kEsfLogManagerStatusOk;
  UtilityMsgErrCode msg_ret = kUtilityMsgOk;

  s_metrics_loop_generate = true;
  s_metrics_loop_send = true;

  msg_ret = UtilityMsgOpen(&s_queue_handle,
                           CONFIG_EXTERNAL_LOG_MANAGER_METRICS_QUEUE_NUM,
                           CONFIG_EXTERNAL_LOG_MANAGER_METRICS_QUEUE_SIZE);
  if (msg_ret != kUtilityMsgOk) {
    ESF_LOG_MANAGER_ERROR("%d\n", msg_ret);
    return kEsfLogManagerStatusFailed;
  }

  ret = EsfLogManagerCreateMetricsSendingThread();
  if (ret != kEsfLogManagerStatusOk) {
    ESF_LOG_MANAGER_ERROR("%d\n", ret);
    s_metrics_loop_generate = false;
    s_metrics_loop_send = false;

    (void)UtilityMsgClose(s_queue_handle);
    s_queue_handle = 0;
    return kEsfLogManagerStatusFailed;
  }

  ret = EsfLogManagerCreateMetricsGenerationThread();
  if (ret != kEsfLogManagerStatusOk) {
    ESF_LOG_MANAGER_ERROR("%d\n", ret);
    s_metrics_loop_generate = false;
    s_metrics_loop_send = false;

    (void)pthread_mutex_lock(&s_metrics_send_mutex);
    (void)pthread_cond_signal(&s_metrics_send_cond);
    (void)pthread_mutex_unlock(&s_metrics_send_mutex);
    (void)pthread_join(s_metrics_send_tid, NULL);

    (void)UtilityMsgClose(s_queue_handle);
    s_queue_handle = 0;
    return kEsfLogManagerStatusFailed;
  }
  return kEsfLogManagerStatusOk;
}

// """ Deinitialize metrics
// Args:
//    none
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination
EsfLogManagerStatus EsfLogManagerDeinitMetrics(void) {
  EsfLogManagerStatus ret = kEsfLogManagerStatusOk;
  UtilityMsgErrCode utility_ret = kUtilityMsgOk;

  // Deinit EsfLogManagerCreateMetricsGenerationThread
  s_metrics_loop_generate = false;
  (void)pthread_mutex_lock(&s_metrics_generate_mutex);
  (void)pthread_cond_signal(&s_metrics_generate_cond);
  (void)pthread_mutex_unlock(&s_metrics_generate_mutex);
  (void)pthread_join(s_metrics_create_tid, NULL);

  // Deinit EsfLogManagerCreateMetricsSendingThread
  s_metrics_loop_send = false;
  (void)pthread_mutex_lock(&s_metrics_send_mutex);
  (void)pthread_cond_signal(&s_metrics_send_cond);
  (void)pthread_mutex_unlock(&s_metrics_send_mutex);
  (void)pthread_join(s_metrics_send_tid, NULL);

  if (s_metrics_create_thread_stack != NULL) {
    free(s_metrics_create_thread_stack);
    s_metrics_create_thread_stack = NULL;
  }

  if (s_metrics_send_thread_stack != NULL) {
    free(s_metrics_send_thread_stack);
    s_metrics_send_thread_stack = NULL;
  }

  if (s_queue_handle != 0) {
    utility_ret = UtilityMsgClose(s_queue_handle);
    if (utility_ret != kUtilityMsgOk) {
      ESF_LOG_MANAGER_ERROR("%d\n", utility_ret);
      ret = kEsfLogManagerStatusFailed;
    }
    s_queue_handle = 0;
  }

  return ret;
}

// """ Create metrics collector thread
// Args:
//    none
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination
static EsfLogManagerStatus EsfLogManagerCreateMetricsGenerationThread(void) {
  pthread_attr_t metrics_attr;
  struct sched_param metrics_sched_param;
  EsfLogManagerStatus ret = kEsfLogManagerStatusOk;
  int p_ret = 0;

  p_ret = pthread_attr_init(&metrics_attr);
  if (p_ret != 0) {
    ESF_LOG_MANAGER_ERROR("%d\n", p_ret);
    return kEsfLogManagerStatusFailed;
  }

  s_metrics_create_thread_stack =
      malloc(CONFIG_EXTERNAL_LOG_MANAGER_METRICS_GENERATE_THREAD_STACK_SIZE);
  if (s_metrics_create_thread_stack == NULL) {
    ESF_LOG_MANAGER_ERROR("allocate memory failed \n");
    ret = kEsfLogManagerStatusFailed;
    goto cleanup_attr;
  }
  p_ret = pthread_attr_setstack(
      &metrics_attr, s_metrics_create_thread_stack,
      CONFIG_EXTERNAL_LOG_MANAGER_METRICS_GENERATE_THREAD_STACK_SIZE);
  if (p_ret != 0) {
    ESF_LOG_MANAGER_ERROR("%d\n", p_ret);
    ret = kEsfLogManagerStatusFailed;
    goto cleanup_stack;
  }

  metrics_sched_param.sched_priority =
      CONFIG_EXTERNAL_LOG_MANAGER_METRICS_GENERATE_PRIORITY;
  p_ret = pthread_attr_setschedparam(&metrics_attr, &metrics_sched_param);
  if (p_ret != 0) {
    ESF_LOG_MANAGER_ERROR("%d\n", p_ret);
    ret = kEsfLogManagerStatusFailed;
    goto cleanup_stack;
  }

  p_ret = pthread_create(&s_metrics_create_tid, &metrics_attr,
                         EsfLogManagerMetricsGenerationThread, NULL);
  if (p_ret != 0) {
    ESF_LOG_MANAGER_ERROR("%d\n", p_ret);
    ret = kEsfLogManagerStatusFailed;
    goto cleanup_stack;
  }

  p_ret = pthread_setname_np(s_metrics_create_tid,
                             "EsfLogManagerMetricsGenerationThread");
  if (p_ret != 0) {
    ESF_LOG_MANAGER_ERROR("%d\n", p_ret);
    ret = kEsfLogManagerStatusFailed;
    goto cleanup_attr;
  }

  ret = kEsfLogManagerStatusOk;

cleanup_attr:
  p_ret = pthread_attr_destroy(&metrics_attr);
cleanup_stack:
  if (p_ret != 0) {
    ESF_LOG_MANAGER_ERROR("%d\n", p_ret);
    free(s_metrics_create_thread_stack);
    s_metrics_create_thread_stack = NULL;
    ret = kEsfLogManagerStatusFailed;
  }
  return ret;
}

// """ Create metrics sender thread
// Args:
//    none
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination
static EsfLogManagerStatus EsfLogManagerCreateMetricsSendingThread(void) {
  pthread_attr_t metrics_attr;
  struct sched_param metrics_sched_param;
  EsfLogManagerStatus ret = kEsfLogManagerStatusOk;
  int p_ret = 0;

  p_ret = pthread_attr_init(&metrics_attr);
  if (p_ret != 0) {
    ESF_LOG_MANAGER_ERROR("%d\n", p_ret);
    return kEsfLogManagerStatusFailed;
  }

  s_metrics_send_thread_stack =
      malloc(CONFIG_EXTERNAL_LOG_MANAGER_METRICS_SEND_THREAD_STACK_SIZE);
  if (s_metrics_send_thread_stack == NULL) {
    ESF_LOG_MANAGER_ERROR("allocate memory failed \n");
    ret = kEsfLogManagerStatusFailed;
    goto cleanup_attr;
  }
  p_ret = pthread_attr_setstack(
      &metrics_attr, s_metrics_send_thread_stack,
      CONFIG_EXTERNAL_LOG_MANAGER_METRICS_SEND_THREAD_STACK_SIZE);
  if (p_ret != 0) {
    ESF_LOG_MANAGER_ERROR("%d\n", p_ret);
    ret = kEsfLogManagerStatusFailed;
    goto cleanup_stack;
  }

  metrics_sched_param.sched_priority =
      CONFIG_EXTERNAL_LOG_MANAGER_METRICS_SEND_PRIORITY;
  p_ret = pthread_attr_setschedparam(&metrics_attr, &metrics_sched_param);
  if (p_ret != 0) {
    ESF_LOG_MANAGER_ERROR("%d\n", p_ret);
    ret = kEsfLogManagerStatusFailed;
    goto cleanup_stack;
  }

  p_ret = pthread_create(&s_metrics_send_tid, &metrics_attr,
                         EsfLogManagerMetricsSendingThread, NULL);
  if (p_ret != 0) {
    ESF_LOG_MANAGER_ERROR("%d\n", p_ret);
    ret = kEsfLogManagerStatusFailed;
    goto cleanup_stack;
  }

  p_ret = pthread_setname_np(s_metrics_send_tid,
                             "EsfLogManagerMetricsSendingThread");
  if (p_ret != 0) {
    ESF_LOG_MANAGER_ERROR("%d\n", p_ret);
    ret = kEsfLogManagerStatusFailed;
    goto cleanup_attr;
  }

  ret = kEsfLogManagerStatusOk;

cleanup_attr:
  p_ret = pthread_attr_destroy(&metrics_attr);
cleanup_stack:
  if (p_ret != 0) {
    ESF_LOG_MANAGER_ERROR("%d\n", p_ret);
    free(s_metrics_create_thread_stack);
    s_metrics_create_thread_stack = NULL;
    ret = kEsfLogManagerStatusFailed;
  }
  return ret;
}

// """ Metrics collector thread function
// Args:
//    void *p: thread argument
// Returns:
//    void*: thread exit status
static void *EsfLogManagerMetricsGenerationThread(void *p) {
  (void)p;
  EsfJsonErrorCode json_result = kEsfJsonSuccess;
  EsfLogManagerStatus ret = kEsfLogManagerStatusOk;
  UtilityMsgErrCode msg_ret = kUtilityMsgOk;

  EsfJsonHandle json_handle;
  struct timespec ts;
  const char *serialized_string;
  int32_t send_size;

  while (s_metrics_loop_generate) {
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += CONFIG_EXTERNAL_LOG_MANAGER_METRICS_GENERATE_INTERVAL;

    json_result = EsfJsonOpen(&json_handle);
    if (json_result != kEsfJsonSuccess) {
      ESF_LOG_MANAGER_ERROR("%d\n", json_result);
      break;
    }

    ret = EsfLogManagerGenerateMsg(json_handle, &serialized_string);
    if (ret != kEsfLogManagerStatusOk) {
      ESF_LOG_MANAGER_ERROR("%d\n", ret);
      (void)EsfJsonClose(json_handle);
      break;
    }

    msg_ret = UtilityMsgSend(s_queue_handle, serialized_string,
                             strlen(serialized_string) + 1, 0, &send_size);
    (void)EsfJsonSerializeFree(json_handle);
    (void)EsfJsonClose(json_handle);
    if (msg_ret != kUtilityMsgOk) {
      ESF_LOG_MANAGER_ERROR("%d\n", msg_ret);
      break;
    }

    (void)pthread_mutex_lock(&s_metrics_generate_mutex);
    (void)pthread_cond_timedwait(&s_metrics_generate_cond,
                                 &s_metrics_generate_mutex, &ts);
    (void)pthread_mutex_unlock(&s_metrics_generate_mutex);
  }

  (void)pthread_exit((void *)NULL);
  return NULL;
}

// """ Metrics sender thread function
// Args:
//    void *p: thread argument
// Returns:
//    void*: thread exit status
static void *EsfLogManagerMetricsSendingThread(void *p) {
  (void)p;
  UtilityMsgErrCode msg_ret = kUtilityMsgOk;

  int32_t recv_size;
  char *msg = (char *)malloc(CONFIG_EXTERNAL_LOG_MANAGER_METRICS_QUEUE_SIZE);
  if (msg == NULL) {
    ESF_LOG_MANAGER_ERROR("allocate memory failed \n");
    return NULL;
  }

  enum SYS_result sys_telemetry_result;
  enum SYS_result sys_event_result;
  struct timespec ts;

  do {
    s_metrics_sys_client = EVP_Agent_register_sys_client();
    sleep(1);
  } while (s_metrics_sys_client == NULL);

  while (s_metrics_loop_send) {
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += CONFIG_EXTERNAL_LOG_MANAGER_METRICS_SEND_INTERVAL;

    msg_ret = UtilityMsgRecv(
        s_queue_handle, msg, CONFIG_EXTERNAL_LOG_MANAGER_METRICS_QUEUE_SIZE,
        CONFIG_EXTERNAL_LOG_MANAGER_METRICS_SEND_INTERVAL, &recv_size);
    if (msg_ret == kUtilityMsgErrTimedout) {
      LOG_MANAGER_TRACE_PRINT("%d\n", msg_ret);
      continue;
    } else if (msg_ret != kUtilityMsgOk) {
      ESF_LOG_MANAGER_ERROR("%d\n", msg_ret);
      break;
    }
    if (EVP_getAgentStatus() == EVP_AGENT_STATUS_CONNECTED) {
      sys_telemetry_result = SYS_send_telemetry(s_metrics_sys_client,
                                                LOG_MANAGER_METRICS_TOPIC, msg,
                                                TelemetryCb, NULL);
      if (sys_telemetry_result == SYS_RESULT_OK) {
        sys_event_result = SYS_process_event(
            s_metrics_sys_client,
            CONFIG_EXTERNAL_LOG_MANAGER_METRICS_SEND_INTERVAL);
        if (sys_event_result == SYS_RESULT_SHOULD_EXIT) {
          ESF_LOG_MANAGER_ERROR("%d\n", sys_event_result);
          break;
        }
      } else {
        ESF_LOG_MANAGER_ERROR("%d\n", sys_telemetry_result);
      }
    }

    (void)pthread_mutex_lock(&s_metrics_send_mutex);
    (void)pthread_cond_timedwait(&s_metrics_send_cond, &s_metrics_send_mutex,
                                 &ts);
    (void)pthread_mutex_unlock(&s_metrics_send_mutex);
  }

  free(msg);
  if (s_metrics_sys_client != NULL) {
    (void)EVP_Agent_unregister_sys_client(s_metrics_sys_client);
  }
  (void)pthread_exit((void *)NULL);

  return NULL;
}

// """ Create message queue
// Args:
//    EsfJsonHandle json_handle: JSON json_handle
//    const char **serialized_string: pointer to store serialized string
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination
static EsfLogManagerStatus EsfLogManagerGenerateMsg(
    EsfJsonHandle json_handle, const char **serialized_string) {
  EsfLogManagerStatus ret = kEsfLogManagerStatusOk;
  EsfJsonErrorCode json_result = kEsfJsonSuccess;

  EsfJsonValue metrics_object_id;
  json_result = EsfJsonObjectInit(json_handle, &metrics_object_id);
  if (json_result != kEsfJsonSuccess) {
    ESF_LOG_MANAGER_ERROR("%d\n", json_result);
    return kEsfLogManagerStatusFailed;
  }

  ret = EsfLogManagerGenerateMsgFormat(json_handle, &metrics_object_id);
  if (ret != kEsfLogManagerStatusOk) {
    ESF_LOG_MANAGER_ERROR("%d\n", ret);
    return kEsfLogManagerStatusFailed;
  }

  ret = EsfLogManagerGenerateCpuInfo(json_handle, &metrics_object_id);
  if (ret != kEsfLogManagerStatusOk) {
    ESF_LOG_MANAGER_ERROR("%d\n", ret);
    return kEsfLogManagerStatusFailed;
  }

  ret = EsfLogManagerGenerateMemInfo(json_handle, &metrics_object_id);
  if (ret != kEsfLogManagerStatusOk) {
    ESF_LOG_MANAGER_ERROR("%d\n", ret);
    return kEsfLogManagerStatusFailed;
  }

  ret = EsfLogManagerGenerateStackInfo(json_handle, &metrics_object_id);
  if (ret != kEsfLogManagerStatusOk) {
    ESF_LOG_MANAGER_ERROR("%d\n", ret);
    return kEsfLogManagerStatusFailed;
  }

  json_result = EsfJsonSerialize(json_handle, metrics_object_id,
                                 serialized_string);
  if (json_result != kEsfJsonSuccess) {
    ESF_LOG_MANAGER_ERROR("%d\n", json_result);
    return kEsfLogManagerStatusFailed;
  }

  return kEsfLogManagerStatusOk;
}

// """ Create message format
// Args:
//    EsfJsonHandle json_handle: JSON json_handle
//    EsfJsonValue *metrics_object_id: pointer to store metrics object ID
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination
static EsfLogManagerStatus EsfLogManagerGenerateMsgFormat(
    EsfJsonHandle json_handle, EsfJsonValue *metrics_object_id) {
  if (metrics_object_id == NULL) {
    ESF_LOG_MANAGER_ERROR("invalid argument \n");
    return kEsfLogManagerStatusFailed;
  }
  EsfJsonValue time_value, version_value;
  EsfJsonErrorCode json_result = kEsfJsonSuccess;

  const char *time_key = "Time";
  const char *version_key = "Version";
  const char *version_data = LOG_MANAGER_METRICS_VERSION;

  time_t t;
  struct tm tm_info;
  char now_time[LOG_MANAGER_METRICS_DATE_SIZE];

  time(&t);
  localtime_r(&t, &tm_info);

  snprintf(now_time, LOG_MANAGER_METRICS_DATE_SIZE, "%.4d%.2d%.2d%.2d%.2d%.2d",
           abs((tm_info.tm_year + 1900)) % 10000,
           abs((tm_info.tm_mon + 1)) % 100, abs(tm_info.tm_mday) % 100,
           abs(tm_info.tm_hour) % 100, abs(tm_info.tm_min) % 100,
           abs(tm_info.tm_sec) % 100);

  json_result = EsfJsonStringInit(json_handle, now_time, &time_value);
  if (json_result != kEsfJsonSuccess) {
    ESF_LOG_MANAGER_ERROR("%d\n", json_result);
    return kEsfLogManagerStatusFailed;
  }
  json_result = EsfJsonStringInit(json_handle, version_data, &version_value);
  if (json_result != kEsfJsonSuccess) {
    ESF_LOG_MANAGER_ERROR("%d\n", json_result);
    return kEsfLogManagerStatusFailed;
  }

  json_result = EsfJsonObjectSet(json_handle, *metrics_object_id, time_key,
                                 time_value);
  if (json_result != kEsfJsonSuccess) {
    ESF_LOG_MANAGER_ERROR("%d\n", json_result);
    return kEsfLogManagerStatusFailed;
  }
  json_result = EsfJsonObjectSet(json_handle, *metrics_object_id, version_key,
                                 version_value);
  if (json_result != kEsfJsonSuccess) {
    ESF_LOG_MANAGER_ERROR("%d\n", json_result);
    return kEsfLogManagerStatusFailed;
  }

  return kEsfLogManagerStatusOk;
}

// """ Check if process directory exists
// Args:
//    const char *dirpath: directory path
// Returns:
//    int: 1 if exists, 0 otherwise
static int IsProcDirExist(const char *dirpath) {
  for (int i = 0; i < NAME_MAX && dirpath[i] != '\0'; i++) {
    if (!isdigit(dirpath[i])) {
      return 0;
    }
  }
  return 1;
}

// """ Create Cpu info
// Args:
//    EsfJsonHandle json_handle: JSON json_handle
//    EsfJsonValue *metrics_object_id: pointer to store metrics object ID
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination
static EsfLogManagerStatus EsfLogManagerGenerateCpuInfo(
    EsfJsonHandle json_handle, EsfJsonValue *metrics_object_id) {
  if (metrics_object_id == NULL) {
    ESF_LOG_MANAGER_ERROR("invalid argument \n");
    return kEsfLogManagerStatusFailed;
  }

  EsfLogManagerStatus ret = kEsfLogManagerStatusOk;
  EsfJsonErrorCode json_result = kEsfJsonSuccess;
  EsfJsonValue cpu_array, cpu_obj;
  const char *cpu_key = "CPU";

  char *filepath = NULL;

  char *cpu_percent = malloc(LOG_MANAGER_METRICS_PROC_VALUE_SIZE);
  if (cpu_percent == NULL) {
    ESF_LOG_MANAGER_ERROR("allocate memory failed \n");
    return kEsfLogManagerStatusFailed;
  }
  char *cpu_command = malloc(LOG_MANAGER_METRICS_PROC_VALUE_SIZE);
  if (cpu_command == NULL) {
    ESF_LOG_MANAGER_ERROR("allocate memory failed \n");
    free(cpu_percent);
    return kEsfLogManagerStatusFailed;
  }

  const char *dirpath = "/proc";
  DIR *dirp = opendir(dirpath);
  if (dirp == NULL) {
    ESF_LOG_MANAGER_ERROR("open directory failed \n");
    ret = kEsfLogManagerStatusFailed;
    goto cleanup;
  }

  json_result = EsfJsonObjectInit(json_handle, &cpu_obj);
  if (json_result != kEsfJsonSuccess) {
    ESF_LOG_MANAGER_ERROR("%d\n", json_result);
    ret = kEsfLogManagerStatusFailed;
    goto cleanup_dir;
  }

  struct dirent *entry;
  while ((entry = readdir(dirp)) != NULL) {
    if (!IsProcDirExist(entry->d_name)) {
      break;
    }

    asprintf(&filepath, "%s/%s/loadavg", dirpath, entry->d_name);
    ret = EsfLogManagerOpenProc(cpu_percent, filepath);
    free(filepath);
    filepath = NULL;
    if (ret != kEsfLogManagerStatusOk) {
      ESF_LOG_MANAGER_ERROR("%d\n", ret);
      break;
    }

    asprintf(&filepath, "%s/%s/cmdline", dirpath, entry->d_name);
    ret = EsfLogManagerOpenProc(cpu_command, filepath);
    free(filepath);
    filepath = NULL;
    if (ret != kEsfLogManagerStatusOk) {
      ESF_LOG_MANAGER_ERROR("%d\n", ret);
      break;
    }

    EsfJsonValue cpu_value;
    json_result = EsfJsonStringInit(json_handle, cpu_percent, &cpu_value);
    if (json_result != kEsfJsonSuccess) {
      ESF_LOG_MANAGER_ERROR("%d\n", json_result);
      ret = kEsfLogManagerStatusFailed;
      goto cleanup_dir;
    }
    json_result = EsfJsonObjectSet(json_handle, cpu_obj, cpu_command,
                                   cpu_value);
    if (json_result != kEsfJsonSuccess) {
      ESF_LOG_MANAGER_ERROR("%d\n", json_result);
      ret = kEsfLogManagerStatusFailed;
      goto cleanup_dir;
    }
  }

  json_result = EsfJsonArrayInit(json_handle, &cpu_array);
  if (json_result != kEsfJsonSuccess) {
    ESF_LOG_MANAGER_ERROR("%d\n", json_result);
    ret = kEsfLogManagerStatusFailed;
    goto cleanup_dir;
  }

  json_result = EsfJsonArrayAppend(json_handle, cpu_array, cpu_obj);
  if (json_result != kEsfJsonSuccess) {
    ESF_LOG_MANAGER_ERROR("%d\n", json_result);
    ret = kEsfLogManagerStatusFailed;
    goto cleanup_dir;
  }

  json_result = EsfJsonObjectSet(json_handle, *metrics_object_id, cpu_key,
                                 cpu_array);
  if (json_result != kEsfJsonSuccess) {
    ESF_LOG_MANAGER_ERROR("%d\n", json_result);
    ret = kEsfLogManagerStatusFailed;
  }

cleanup_dir:
  closedir(dirp);
cleanup:
  free(cpu_percent);
  free(cpu_command);

  return ret;
}

// """ Create Memory info
// Args:
//    EsfJsonHandle json_handle: JSON json_handle
//    EsfJsonValue *metrics_object_id: pointer to store metrics object ID
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination
static EsfLogManagerStatus EsfLogManagerGenerateMemInfo(
    EsfJsonHandle json_handle, EsfJsonValue *metrics_object_id) {
  if (metrics_object_id == NULL) {
    ESF_LOG_MANAGER_ERROR("invalid argument \n");
    return kEsfLogManagerStatusFailed;
  }

  EsfLogManagerStatus ret = kEsfLogManagerStatusOk;
  EsfJsonErrorCode json_result = kEsfJsonSuccess;
  EsfJsonValue mem_array, mem_obj;
  const char *mem_key = "Memory";

  json_result = EsfJsonObjectInit(json_handle, &mem_obj);
  if (json_result != kEsfJsonSuccess) {
    ESF_LOG_MANAGER_ERROR("%d\n", json_result);
    return kEsfLogManagerStatusFailed;
  }

  ret = EsfLogManagerGetKernelHeap(json_handle, &mem_obj);
  if (ret != kEsfLogManagerStatusOk) {
    ESF_LOG_MANAGER_ERROR("%d\n", ret);
    return ret;
  }

  ret = EsfLogManagerGetNormalHeap(json_handle, &mem_obj);
  if (ret != kEsfLogManagerStatusOk) {
    ESF_LOG_MANAGER_ERROR("%d\n", ret);
    return ret;
  }

  ret = EsfLogManagerGetWasmHeap(json_handle, &mem_obj);
  if (ret != kEsfLogManagerStatusOk) {
    ESF_LOG_MANAGER_ERROR("%d\n", ret);
    return ret;
  }

  ret = EsfLogManagerGetLargeHeap(json_handle, &mem_obj);
  if (ret != kEsfLogManagerStatusOk) {
    ESF_LOG_MANAGER_ERROR("%d\n", ret);
    return ret;
  }

  ret = EsfLogManagerGetDmaMemory(json_handle, &mem_obj);
  if (ret != kEsfLogManagerStatusOk) {
    ESF_LOG_MANAGER_ERROR("%d\n", ret);
    return ret;
  }

  json_result = EsfJsonArrayInit(json_handle, &mem_array);
  if (json_result != kEsfJsonSuccess) {
    ESF_LOG_MANAGER_ERROR("%d\n", json_result);
    return kEsfLogManagerStatusFailed;
  }

  json_result = EsfJsonArrayAppend(json_handle, mem_array, mem_obj);
  if (json_result != kEsfJsonSuccess) {
    ESF_LOG_MANAGER_ERROR("%d\n", json_result);
    return kEsfLogManagerStatusFailed;
  }

  json_result = EsfJsonObjectSet(json_handle, *metrics_object_id, mem_key,
                                 mem_array);
  if (json_result != kEsfJsonSuccess) {
    ESF_LOG_MANAGER_ERROR("%d\n", json_result);
    return kEsfLogManagerStatusFailed;
  }

  return kEsfLogManagerStatusOk;
}

// """ Generate Stack info
// Args:
//    EsfJsonHandle json_handle: JSON json_handle
//    EsfJsonValue *metrics_object_id: pointer to store metrics object ID
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination
static EsfLogManagerStatus EsfLogManagerGenerateStackInfo(
    EsfJsonHandle json_handle, EsfJsonValue *metrics_object_id) {
  if (metrics_object_id == NULL) {
    ESF_LOG_MANAGER_ERROR("Invalid argument.\n");
    return kEsfLogManagerStatusFailed;
  }

  EsfLogManagerStatus ret = kEsfLogManagerStatusOk;
  EsfJsonValue stack_obj;
  const char *stack_key = "STACK";

  char *filepath = NULL;

  char *cpu_command = malloc(LOG_MANAGER_METRICS_PROC_VALUE_SIZE);
  if (cpu_command == NULL) {
    ESF_LOG_MANAGER_ERROR("Allocate memory failed.\n");
    return kEsfLogManagerStatusFailed;
  }

  const char *dirpath = "/proc";
  DIR *dirp = opendir(dirpath);
  if (dirp == NULL) {
    ESF_LOG_MANAGER_ERROR("Open directory failed.\n");
    ret = kEsfLogManagerStatusFailed;
    goto cleanup;
  }

  EsfJsonErrorCode json_result = EsfJsonObjectInit(json_handle, &stack_obj);
  if (json_result != kEsfJsonSuccess) {
    ESF_LOG_MANAGER_ERROR("Failed to init json object. json_result=%d\n",
                          json_result);
    ret = kEsfLogManagerStatusFailed;
    goto cleanup_dir;
  }

  struct dirent *entry;
  while ((entry = readdir(dirp)) != NULL) {
    if (!IsProcDirExist(entry->d_name)) {
      break;
    }
    asprintf(&filepath, "%s/%s/cmdline", dirpath, entry->d_name);
    ret = EsfLogManagerOpenProc(cpu_command, filepath);
    free(filepath);
    filepath = NULL;
    if (ret != kEsfLogManagerStatusOk) {
      ESF_LOG_MANAGER_ERROR("Failed to read cpu command. ret=%d\n", ret);
      break;
    }

    EsfLogManagerStackInfo stack_info = {
        .stack_alloc = "", .stack_base = "", .stack_size = 0, .stack_used = 0};
    asprintf(&filepath, "%s/%s/stack", dirpath, entry->d_name);

    ret = EsfLogManagerReadStackInfo(filepath, &stack_info);

    free(filepath);
    filepath = NULL;
    if (ret != kEsfLogManagerStatusOk) {
      ESF_LOG_MANAGER_ERROR("Failed to read stack info. ret=%d\n", ret);
      break;
    }

#if 0
    ESF_LOG_MANAGER_DEBUG("cpu_command: %s\n", cpu_command);
    ESF_LOG_MANAGER_DEBUG("StackAlloc: %s\n", stack_info.stack_alloc);
    ESF_LOG_MANAGER_DEBUG("StackBase: %s\n", stack_info.stack_base);
    ESF_LOG_MANAGER_DEBUG("StackSize: %d\n", stack_info.stack_size);
    ESF_LOG_MANAGER_DEBUG("StackUsed: %d\n", stack_info.stack_used);
#endif

    ret = EsfLogManagerMetricsSetStackJsonObject(json_handle, &stack_obj,
                                                 cpu_command, &stack_info);
    if (ret != kEsfLogManagerStatusOk) {
      ESF_LOG_MANAGER_ERROR("Failed to set stack json object. ret=%d\n", ret);
      break;
    }
  }

  {
    EsfJsonValue stack_array;
    json_result = EsfJsonArrayInit(json_handle, &stack_array);
    if (json_result != kEsfJsonSuccess) {
      ESF_LOG_MANAGER_ERROR("Failed to init json array. json_result=%d\n",
                            json_result);
      ret = kEsfLogManagerStatusFailed;
      goto cleanup_dir;
    }

    json_result = EsfJsonArrayAppend(json_handle, stack_array, stack_obj);
    if (json_result != kEsfJsonSuccess) {
      ESF_LOG_MANAGER_ERROR("Failed to append json array. json_result=%d\n",
                            json_result);
      ret = kEsfLogManagerStatusFailed;
      goto cleanup_dir;
    }

    json_result = EsfJsonObjectSet(json_handle, *metrics_object_id, stack_key,
                                   stack_array);
    if (json_result != kEsfJsonSuccess) {
      ESF_LOG_MANAGER_ERROR("Failed to set json object. json_result=%d\n",
                            json_result);
      ret = kEsfLogManagerStatusFailed;
    }
  }

cleanup_dir:
  closedir(dirp);
cleanup:
  free(cpu_command);

  return ret;
}

// """ Open Proc file and read content
// Args:
//    char *buf: buffer to store content
//    char *filepath: file path
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination
static EsfLogManagerStatus EsfLogManagerOpenProc(char *buf, char *filepath) {
  FILE *fp;
  if (filepath == NULL) {
    return kEsfLogManagerStatusFailed;
  }
  fp = fopen(filepath, "r");
  if (fp == NULL) {
    return kEsfLogManagerStatusFailed;
  }
  if (fgets(buf, LOG_MANAGER_METRICS_PROC_VALUE_SIZE, fp) != NULL) {
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
      buf[len - 1] = '\0';
    }
  } else {
    fclose(fp);
    return kEsfLogManagerStatusFailed;
  }
  while (*buf == ' ') {
    memmove(buf, buf + 1, strlen(buf));
  }
  fclose(fp);

  return kEsfLogManagerStatusOk;
}

// """ Open stack file and read content
// Args:
//    char *buf: buffer to store content
//    char *filepath: file path
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination
static EsfLogManagerStatus EsfLogManagerReadStackInfo(
    const char *filepath, EsfLogManagerStackInfo *stack_info) {
  if ((filepath == NULL) || (stack_info == NULL)) {
    ESF_LOG_MANAGER_ERROR("Invalid param. filepath=%p stack_info=%p\n",
                          filepath, stack_info);
    return kEsfLogManagerStatusFailed;
  }

  FILE *fp = fopen(filepath, "r");
  if (fp == NULL) {
    ESF_LOG_MANAGER_ERROR("Failed to open %s.\n", filepath);
    return kEsfLogManagerStatusFailed;
  }

  EsfLogManagerStatus ret = kEsfLogManagerStatusOk;

  char line[50];
  while (fgets(line, sizeof(line), fp)) {
    char key[LOG_MANAGER_METRICS_STACK_SIZE],
        value[LOG_MANAGER_METRICS_STACK_SIZE];

    if (sscanf(line, "%19s %19s", key, value) == 2) {
      if (strcmp(key, "StackAlloc:") == 0) {
        strncpy(stack_info->stack_alloc, value, LOG_MANAGER_METRICS_STACK_SIZE);
      } else if (strcmp(key, "StackBase:") == 0) {
        strncpy(stack_info->stack_base, value, LOG_MANAGER_METRICS_STACK_SIZE);
      } else if (strcmp(key, "StackSize:") == 0) {
        stack_info->stack_size = atoi(value);
      } else if (strcmp(key, "StackUsed:") == 0) {
        stack_info->stack_used = atoi(value);
      } else {
        ESF_LOG_MANAGER_ERROR("Invalid stack info\n");
        ret = kEsfLogManagerStatusFailed;
        break;
      }
    } else {
      ESF_LOG_MANAGER_ERROR("Invalid stack info\n");
      ret = kEsfLogManagerStatusFailed;
      break;
    }
  }

  fclose(fp);

  return ret;
}

// """ Get memory information (Kernel heap)
// Args:
//    EsfJsonHandle json_handle: JSON json_handle
//    EsfJsonValue *mem_obj: pointer to store metrics object ID
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination
static EsfLogManagerStatus EsfLogManagerGetKernelHeap(EsfJsonHandle json_handle,
                                                      EsfJsonValue *mem_obj) {
  EsfLogManagerStatus ret = kEsfLogManagerStatusOk;
  EsfJsonErrorCode json_result = kEsfJsonSuccess;

  char *mem_name = "Kernel heap";
  struct meminfo mem_info = {-1, -1, -1, -1};

#ifdef __NuttX__

  struct mallinfo kernel_mem_info = kmm_mallinfo();
  if (kernel_mem_info.arena >= 0) {
    mem_info.total = kernel_mem_info.arena;
  }
  if (kernel_mem_info.uordblks >= 0) {
    mem_info.total_used = kernel_mem_info.uordblks;
  }
  if (kernel_mem_info.fordblks >= 0) {
    mem_info.total_free = kernel_mem_info.fordblks;
  }
  if (kernel_mem_info.mxordblk >= 0) {
    mem_info.linear_maxfree = kernel_mem_info.mxordblk;
  }

#endif

  ret = EsfLogManagerMetricsSetMemJsonObject(json_handle, mem_obj, mem_name,
                                             &mem_info);
  if (ret != kEsfLogManagerStatusOk) {
    ESF_LOG_MANAGER_ERROR("%d\n", ret);
    return kEsfLogManagerStatusFailed;
  }

  return ret;
}

// """ Get memory information (Normal heap)
// Args:
//    EsfJsonHandle json_handle: JSON json_handle
//    EsfJsonValue *mem_obj: pointer to store metrics object ID
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination
static EsfLogManagerStatus EsfLogManagerGetNormalHeap(EsfJsonHandle json_handle,
                                                      EsfJsonValue *mem_obj) {
  EsfLogManagerStatus ret = kEsfLogManagerStatusOk;
  EsfJsonErrorCode json_result = kEsfJsonSuccess;

  char *mem_name = "Normal heap";
  struct meminfo mem_info = {-1, -1, -1, -1};

  struct mallinfo normal_mem_info = mallinfo();
  if (normal_mem_info.arena >= 0) {
    mem_info.total = normal_mem_info.arena;
  }
  if (normal_mem_info.uordblks >= 0) {
    mem_info.total_used = normal_mem_info.uordblks;
  }
  if (normal_mem_info.fordblks >= 0) {
    mem_info.total_free = normal_mem_info.fordblks;
  }
#ifdef __NuttX__
  if (normal_mem_info.mxordblk >= 0) {
    mem_info.linear_maxfree = normal_mem_info.mxordblk;
  }
#endif

  ret = EsfLogManagerMetricsSetMemJsonObject(json_handle, mem_obj, mem_name,
                                             &mem_info);
  if (ret != kEsfLogManagerStatusOk) {
    ESF_LOG_MANAGER_ERROR("%d\n", ret);
    return kEsfLogManagerStatusFailed;
  }

  return ret;
}

// """ Get memory information (WASM heap)
// Args:
//    EsfJsonHandle json_handle: JSON json_handle
//    EsfJsonValue *mem_obj: pointer to store metrics object ID
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination
static EsfLogManagerStatus EsfLogManagerGetWasmHeap(EsfJsonHandle json_handle,
                                                    EsfJsonValue *mem_obj) {
  EsfLogManagerStatus ret = kEsfLogManagerStatusOk;
  EsfJsonErrorCode json_result = kEsfJsonSuccess;

  char *mem_name = "WASM heap";
  struct meminfo mem_info = {-1, -1, -1, -1};

  mem_alloc_info_t wasm_mem_info;
  // Error handling is not implemented because the return value of
  // wasm_runtime_get_mem_alloc_info() is false
  (void)wasm_runtime_get_mem_alloc_info(&wasm_mem_info);
  if (INT32_MAX >= wasm_mem_info.total_size) {
    mem_info.total = wasm_mem_info.total_size;
  }
  if (INT32_MAX >= wasm_mem_info.total_free_size) {
    mem_info.total_used = mem_info.total - wasm_mem_info.total_free_size;
  }
  if (INT32_MAX >= wasm_mem_info.total_free_size) {
    mem_info.total_free = wasm_mem_info.total_free_size;
  }
  if (INT32_MAX >= wasm_mem_info.total_free_size) {
    mem_info.linear_maxfree = 0;
  }

  ret = EsfLogManagerMetricsSetMemJsonObject(json_handle, mem_obj, mem_name,
                                             &mem_info);
  if (ret != kEsfLogManagerStatusOk) {
    ESF_LOG_MANAGER_ERROR("%d\n", ret);
    return kEsfLogManagerStatusFailed;
  }

  return ret;
}

// """ Get memory information (Large heap)
// Args:
//    EsfJsonHandle json_handle: JSON json_handle
//    EsfJsonValue *mem_obj: pointer to store metrics object ID
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination
static EsfLogManagerStatus EsfLogManagerGetLargeHeap(EsfJsonHandle json_handle,
                                                     EsfJsonValue *mem_obj) {
  EsfLogManagerStatus ret = kEsfLogManagerStatusOk;
  EsfJsonErrorCode json_result = kEsfJsonSuccess;
  PlErrCode pl_ercd = kPlErrCodeOk;

  char *mem_name = "Large heap";
  struct meminfo mem_info = {-1, -1, -1, -1};

  PlLheapMeminfo large_mem_info;
  pl_ercd = PlLheapGetMeminfo(&large_mem_info);
  if (pl_ercd == kPlErrCodeOk) {
    if (INT32_MAX >= large_mem_info.total) {
      mem_info.total = large_mem_info.total;
    }
    if (INT32_MAX >= large_mem_info.used) {
      mem_info.total_used = large_mem_info.used;
    }
    if (INT32_MAX >= large_mem_info.free) {
      mem_info.total_free = large_mem_info.free;
    }
    if (INT32_MAX >= large_mem_info.linear_free) {
      mem_info.linear_maxfree = large_mem_info.linear_free;
    }
  }

  ret = EsfLogManagerMetricsSetMemJsonObject(json_handle, mem_obj, mem_name,
                                             &mem_info);
  if (ret != kEsfLogManagerStatusOk) {
    ESF_LOG_MANAGER_ERROR("%d\n", ret);
    return kEsfLogManagerStatusFailed;
  }

  return ret;
}

// """ Get memory information (DMA Memory)
// Args:
//    EsfJsonHandle json_handle: JSON json_handle
//    EsfJsonValue *mem_obj: pointer to store metrics object ID
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination
static EsfLogManagerStatus EsfLogManagerGetDmaMemory(EsfJsonHandle json_handle,
                                                     EsfJsonValue *mem_obj) {
  EsfLogManagerStatus ret = kEsfLogManagerStatusOk;
  EsfJsonErrorCode json_result = kEsfJsonSuccess;
  PlErrCode pl_ercd = kPlErrCodeOk;

  char *mem_name = "DMA Memory";
  struct meminfo mem_info = {-1, -1, -1, -1};

  PlDmaMemInfo dma_mem_info;
  pl_ercd = PlDmaMemGetMemInfo(&dma_mem_info);
  if (pl_ercd == kPlErrCodeOk) {
    if (INT32_MAX >= dma_mem_info.total_bytes) {
      mem_info.total = dma_mem_info.total_bytes;
    }
    if (INT32_MAX >= dma_mem_info.used_bytes) {
      mem_info.total_used = dma_mem_info.used_bytes;
    }
    if (INT32_MAX >= dma_mem_info.free_bytes) {
      mem_info.total_free = dma_mem_info.free_bytes;
    }
    if (INT32_MAX >= dma_mem_info.free_linear_bytes) {
      mem_info.linear_maxfree = dma_mem_info.free_linear_bytes;
    }
  }

  ret = EsfLogManagerMetricsSetMemJsonObject(json_handle, mem_obj, mem_name,
                                             &mem_info);
  if (ret != kEsfLogManagerStatusOk) {
    ESF_LOG_MANAGER_ERROR("%d\n", ret);
    return kEsfLogManagerStatusFailed;
  }

  return ret;
}

// """ Set Mem Json Object
// Args:
//    EsfJsonHandle json_handle: JSON json_handle
//    EsfJsonValue *mem_obj: pointer to store metrics object ID
//    char *mem_name: Memory information json key
//    meminfo *mem_info: Memory information json value
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination
static EsfLogManagerStatus EsfLogManagerMetricsSetMemJsonObject(
    EsfJsonHandle json_handle, EsfJsonValue *mem_obj, char *mem_name,
    struct meminfo *mem_info) {
  EsfLogManagerStatus ret = kEsfLogManagerStatusOk;
  EsfJsonErrorCode json_result = kEsfJsonSuccess;

  EsfJsonValue total_value, total_used_value, total_free_value,
      linear_maxfree_value;
  json_result = EsfJsonIntegerInit(json_handle, mem_info->total, &total_value);
  if (json_result != kEsfJsonSuccess) {
    ESF_LOG_MANAGER_ERROR("%d\n", json_result);
    return kEsfLogManagerStatusFailed;
  }
  json_result = EsfJsonIntegerInit(json_handle, mem_info->total_used,
                                   &total_used_value);
  if (json_result != kEsfJsonSuccess) {
    ESF_LOG_MANAGER_ERROR("%d\n", json_result);
    return kEsfLogManagerStatusFailed;
  }
  json_result = EsfJsonIntegerInit(json_handle, mem_info->total_free,
                                   &total_free_value);
  if (json_result != kEsfJsonSuccess) {
    ESF_LOG_MANAGER_ERROR("%d\n", json_result);
    return kEsfLogManagerStatusFailed;
  }
  json_result = EsfJsonIntegerInit(json_handle, mem_info->linear_maxfree,
                                   &linear_maxfree_value);
  if (json_result != kEsfJsonSuccess) {
    ESF_LOG_MANAGER_ERROR("%d\n", json_result);
    return kEsfLogManagerStatusFailed;
  }

  EsfJsonValue mem_size_object;
  json_result = EsfJsonObjectInit(json_handle, &mem_size_object);
  if (json_result != kEsfJsonSuccess) {
    ESF_LOG_MANAGER_ERROR("%d\n", json_result);
    return kEsfLogManagerStatusFailed;
  }

  json_result = EsfJsonObjectSet(json_handle, mem_size_object,
                                 LOG_MANAGER_METRICS_MEM_KEY_TOTAL,
                                 total_value);
  if (json_result != kEsfJsonSuccess) {
    ESF_LOG_MANAGER_ERROR("%d\n", json_result);
    return kEsfLogManagerStatusFailed;
  }
  json_result = EsfJsonObjectSet(json_handle, mem_size_object,
                                 LOG_MANAGER_METRICS_MEM_KEY_TOTAL_USED,
                                 total_used_value);
  if (json_result != kEsfJsonSuccess) {
    ESF_LOG_MANAGER_ERROR("%d\n", json_result);
    return kEsfLogManagerStatusFailed;
  }
  json_result = EsfJsonObjectSet(json_handle, mem_size_object,
                                 LOG_MANAGER_METRICS_MEM_KEY_TOTAL_FREE,
                                 total_free_value);
  if (json_result != kEsfJsonSuccess) {
    ESF_LOG_MANAGER_ERROR("%d\n", json_result);
    return kEsfLogManagerStatusFailed;
  }
  json_result = EsfJsonObjectSet(json_handle, mem_size_object,
                                 LOG_MANAGER_METRICS_MEM_KEY_LINER_MAXFREE,
                                 linear_maxfree_value);
  if (json_result != kEsfJsonSuccess) {
    ESF_LOG_MANAGER_ERROR("%d\n", json_result);
    return kEsfLogManagerStatusFailed;
  }

  EsfJsonValue mem_value;
  json_result = EsfJsonObjectSet(json_handle, *mem_obj, mem_name,
                                 mem_size_object);
  if (json_result != kEsfJsonSuccess) {
    ESF_LOG_MANAGER_ERROR("%d\n", json_result);
    return kEsfLogManagerStatusFailed;
  }

  return ret;
}

// """ Set Stack Json Object
// Args:
//    EsfJsonHandle json_handle: JSON json_handle
//    EsfJsonValue *stack_obj: pointer to store metrics object ID
//    char *cpu_command: Stack information json key
//    EsfLogManagerStackInfo *stack_info: Memory information json value
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination
static EsfLogManagerStatus EsfLogManagerMetricsSetStackJsonObject(
    EsfJsonHandle json_handle, EsfJsonValue *stack_obj, char *cpu_command,
    EsfLogManagerStackInfo *stack_info) {
  EsfLogManagerStatus ret = kEsfLogManagerStatusOk;
  EsfJsonErrorCode json_result = kEsfJsonSuccess;

  EsfJsonValue stack_alloc, stack_base, stack_size, stack_used;

  json_result = EsfJsonStringInit(json_handle, stack_info->stack_alloc,
                                  &stack_alloc);
  if (json_result != kEsfJsonSuccess) {
    ESF_LOG_MANAGER_ERROR("%d\n", json_result);
    return kEsfLogManagerStatusFailed;
  }

  json_result = EsfJsonStringInit(json_handle, stack_info->stack_base,
                                  &stack_base);
  if (json_result != kEsfJsonSuccess) {
    ESF_LOG_MANAGER_ERROR("%d\n", json_result);
    return kEsfLogManagerStatusFailed;
  }

  json_result = EsfJsonIntegerInit(json_handle, stack_info->stack_size,
                                   &stack_size);
  if (json_result != kEsfJsonSuccess) {
    ESF_LOG_MANAGER_ERROR("%d\n", json_result);
    return kEsfLogManagerStatusFailed;
  }

  json_result = EsfJsonIntegerInit(json_handle, stack_info->stack_used,
                                   &stack_used);
  if (json_result != kEsfJsonSuccess) {
    ESF_LOG_MANAGER_ERROR("%d\n", json_result);
    return kEsfLogManagerStatusFailed;
  }

  EsfJsonValue stack_info_object;
  json_result = EsfJsonObjectInit(json_handle, &stack_info_object);
  if (json_result != kEsfJsonSuccess) {
    ESF_LOG_MANAGER_ERROR("%d\n", json_result);
    return kEsfLogManagerStatusFailed;
  }

  json_result = EsfJsonObjectSet(json_handle, stack_info_object, "StackAlloc",
                                 stack_alloc);
  if (json_result != kEsfJsonSuccess) {
    ESF_LOG_MANAGER_ERROR("%d\n", json_result);
    return kEsfLogManagerStatusFailed;
  }

  json_result = EsfJsonObjectSet(json_handle, stack_info_object, "StackBase",
                                 stack_base);
  if (json_result != kEsfJsonSuccess) {
    ESF_LOG_MANAGER_ERROR("%d\n", json_result);
    return kEsfLogManagerStatusFailed;
  }

  json_result = EsfJsonObjectSet(json_handle, stack_info_object, "StackSize",
                                 stack_size);
  if (json_result != kEsfJsonSuccess) {
    ESF_LOG_MANAGER_ERROR("%d\n", json_result);
    return kEsfLogManagerStatusFailed;
  }

  json_result = EsfJsonObjectSet(json_handle, stack_info_object, "StackUsed",
                                 stack_used);
  if (json_result != kEsfJsonSuccess) {
    ESF_LOG_MANAGER_ERROR("%d\n", json_result);
    return kEsfLogManagerStatusFailed;
  }

  json_result = EsfJsonObjectSet(json_handle, *stack_obj, cpu_command,
                                 stack_info_object);
  if (json_result != kEsfJsonSuccess) {
    ESF_LOG_MANAGER_ERROR("%d\n", json_result);
    return kEsfLogManagerStatusFailed;
  }

  return ret;
}

// """ Telemetry callback function
// Args:
//    struct SYS_client *c: system client
//    enum SYS_callback_reason reason: callback reason
//    void *user: user data
// Returns:
//    none
static void TelemetryCb(struct SYS_client *c, enum SYS_callback_reason reason,
                        void *user) {
  (void)c;
  (void)user;
  switch (reason) {
    case SYS_REASON_FINISHED:
      LOG_MANAGER_TRACE_PRINT("Send Telemetry is done, Sent Message.\n");
      break;
    case SYS_REASON_ERROR:
      LOG_MANAGER_TRACE_PRINT("Send Telemetry is Error.\n");
      break;
    case SYS_REASON_TIMEOUT:
      LOG_MANAGER_TRACE_PRINT("Send Telemetry is timeout.\n");
      break;
    case SYS_REASON_MORE_DATA:
      LOG_MANAGER_TRACE_PRINT("Telemetry data is short.\n");
      break;
    default:
      ESF_LOG_MANAGER_ERROR("Unexpected telemetry error occurred.\n");
      break;
  }
}
