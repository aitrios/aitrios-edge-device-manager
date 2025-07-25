/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "firmware_manager.h"

#include <errno.h>
#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>

#include "firmware_manager_common.h"
#ifndef CONFIG_EXTERNAL_TARGET_RPI
#include "firmware_manager_factory_reset.h"
#endif
#include "firmware_manager_log.h"
#ifndef CONFIG_EXTERNAL_TARGET_RPI
#include "firmware_manager_porting_layer.h"
#endif
#include "firmware_manager_submodule.h"
#include "mbedtls/sha256.h"
#include "memory_manager.h"
#include "processor/firmware_manager_processor.h"
#include "sensor/firmware_manager_sensor.h"

#if defined(CONFIG_EXTERNAL_TARGET_T3P) || defined(CONFIG_EXTERNAL_TARGET_T5)
#include "sensor_ai_lib/sensor_ai_lib_fwupdate.h"
#endif /* CONFIG_EXTERNAL_TARGET_T3P || CONFIG_EXTERNAL_TARGET_T5 */

#ifdef CONFIG_EXTERNAL_FIRMWARE_MANAGER_USE_MBEDTLS_V2
#define ESF_FW_MGR_HANDLE_MBEDTLS_ERROR(operation, error_handling) operation
#else
#define ESF_FW_MGR_HANDLE_MBEDTLS_ERROR(operation, error_handling) \
  do {                                                             \
    int r = operation;                                             \
    if (r != 0) error_handling;                                    \
  } while (0)
#endif

// Internal enum and structures ###############################################

typedef enum EsfFwMgrState {
  kEsfFwMgrStateUninit,
  kEsfFwMgrStateIdle,
  kEsfFwMgrStateErasable,
  kEsfFwMgrStateWritable,
  kEsfFwMgrStateDone,
  kEsfFwMgrStateError,
} EsfFwMgrState;

typedef EsfFwMgrResult (*GetOpsFunc)(EsfFwMgrSubmoduleOps*);
static const GetOpsFunc s_get_ops_functions[] = {
#ifdef CONFIG_FIRMWARE_MANAGER_PORTING_LAYER
    EsfFwMgrProcessorGetOps,
#endif
    EsfFwMgrSensorGetOps,
};

typedef enum EsfFwMgrInternalBufferState {
  kEsfFwMgrInternalBufferNotAllocated,
  kEsfFwMgrInternalBufferAllocated,  // Not mapped or opened
  kEsfFwMgrInternalBufferMapped,
  kEsfFwMgrInternalBufferOpen,
} EsfFwMgrInternalBufferState;

typedef struct EsfFwMgrContext {
  EsfFwMgrSubmoduleHandle submodule_handle;
  EsfFwMgrSubmoduleOps* submodule_ops;
  bool submodule_is_open;

  int32_t remaining_write_size;
  int32_t internal_buffer_size;

  // internal_buffer
  EsfFwMgrInternalBufferState internal_buffer_state;
  EsfMemoryManagerHandle internal_buffer_handle;
  uint8_t* internal_buffer;
  bool is_map_supported_for_internal_buffer;

  // temporary buffer for calculating sha256 hash
  int32_t tmp_buffer_size;
  uint8_t* tmp_buffer;

#if defined(CONFIG_EXTERNAL_TARGET_T3P) || defined(CONFIG_EXTERNAL_TARGET_T5)
  // sensor_ai_lib_handle for a dummy update
  bool dummy_update_is_open;
  SsfSensorLibFwUpdateHandle dummy_update_handle;
#endif /* CONFIG_EXTERNAL_TARGET_T3P || CONFIG_EXTERNAL_TARGET_T5 */

  mbedtls_sha256_context sha256_context;
  uint8_t hash[ESF_FIRMWARE_MANAGER_TARGET_HASH_SIZE];
  bool sha256_freed;
} EsfFwMgrContext;

// Global variables ############################################################

// s_main_apis_mutex is used to avoid executing main APIs at the same
// time. Each main API locks it at the beginning of the API and unlocks it
// at the end. s_state MUST be updated by the owner of s_main_apis_mutex.
static pthread_mutex_t s_main_apis_mutex = PTHREAD_MUTEX_INITIALIZER;
static EsfFwMgrState s_state = kEsfFwMgrStateUninit;

// s_sub_apis_mutex is used to avoid executing Deinit() while executing sub
// APIs.
static pthread_mutex_t s_sub_apis_mutex = PTHREAD_MUTEX_INITIALIZER;

static int s_num_submodules = 0;  // Initialized in EsfFwMgrInit
static EsfFwMgrSubmoduleOps* s_submodule_ops_list = NULL;

static EsfFwMgrContext* s_active_context = NULL;

// Internal functions ##########################################################

#if defined(CONFIG_EXTERNAL_TARGET_T3P) || defined(CONFIG_EXTERNAL_TARGET_T5)
// Blocking and Unblocking streaming is used to prevent other modules from
// starting streaming while the firmware update is in progress.
// This feature is only available for T3P and T5 devices.

/// @brief // Prevents other modules from starting streaming.
/// @param context [in/out]
/// @return
static EsfFwMgrResult BlockStreaming(EsfFwMgrContext* context) {
  if (context == NULL) {
    ESF_FW_MGR_DLOG_ERROR("context is NUll.\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x00Core);
    return kEsfFwMgrResultInvalidArgument;
  }

  SsfSensorLibComponentInfo info = {0};
  // While the firmware update is in progress, the Sensor AI Library will reject
  // requests to start streaming. Therefore, by initiating a dummy firmware
  // update, any attempts to start streaming by other modules will be blocked.
  SsfSensorLibResult ssf_ret =
      SsfSensorLibFwUpdateBegin2(kSsfSensorLibFwUpdateTargetDummy, "", &info,
                                 &context->dummy_update_handle);
  if (ssf_ret != kSsfSensorLibResultOk) {
    ESF_FW_MGR_DLOG_ERROR("SsfSensorLibFwUpdateBegin2 failed. ret = %u\n",
                          ssf_ret);
    if (ssf_ret == kSsfSensorLibResultFailedPrecondition) {
      ESF_FW_MGR_ELOG_WARNING(kEsfFwMgrElogWarningId0x00Core)
      return kEsfFwMgrResultUnavailable;
    } else {
      ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0x00CoreBlockStreaming)
      return kEsfFwMgrResultInternal;
    }
  }

  context->dummy_update_is_open = true;
  return kEsfFwMgrResultOk;
}

/// @brief Allows other modules to start streaming by removing the block imposed
// by the BlockStreaming.
/// @param context [in/out]
/// @return
static EsfFwMgrResult UnblockStreaming(EsfFwMgrContext* context) {
  if (context == NULL) {
    ESF_FW_MGR_DLOG_WARNING("context is NUll. Do nothing.\n");
    ESF_FW_MGR_ELOG_WARNING(kEsfFwMgrElogWarningId0x01Core);
    return kEsfFwMgrResultInvalidArgument;
  }

  if (context->dummy_update_is_open == false) {
    ESF_FW_MGR_DLOG_WARNING(
        "Sensor AI Lib has already been closed. Skip the process\n");
    ESF_FW_MGR_ELOG_WARNING(kEsfFwMgrElogWarningId0x02Core);
    return kEsfFwMgrResultOk;
  }

  SsfSensorLibResult ssf_ret =
      SsfSensorLibFwUpdateCancel(context->dummy_update_handle);
  if (ssf_ret != kSsfSensorLibResultOk) {
    ESF_FW_MGR_DLOG_WARNING("SsfSensorLibFwUpdateCancel failed. ret = %u\n",
                            ssf_ret);
    ESF_FW_MGR_ELOG_WARNING(kEsfFwMgrElogWarningId0x03Core);
    // SsfSensorLibFwUpdateCancel should always success (for the dummy
    // target). So this function returns the internal error.
    return kEsfFwMgrResultInternal;
  }

  context->dummy_update_is_open = false;
  return kEsfFwMgrResultOk;
}
#endif /* CONFIG_EXTERNAL_TARGET_T3P || CONFIG_EXTERNAL_TARGET_T5 */

// Public functions ############################################################
// Init ------------------------------------------------------------------------
static void InitializeSubmoduleOps(EsfFwMgrSubmoduleOps* ops) {
  if (ops == NULL) {
    ESF_FW_MGR_DLOG_WARNING("ops is NULL. Do nothing.\n");
    ESF_FW_MGR_ELOG_WARNING(kEsfFwMgrElogWarningId0x04Core);
    return;
  }

  ops->init = NULL;
  ops->deinit = NULL;
  ops->is_supported = NULL;
  ops->open = NULL;
  ops->close = NULL;
  ops->write = NULL;
  ops->post_process = NULL;
  ops->erase = NULL;
  ops->get_binary_header_info = NULL;
  ops->get_info = NULL;
}

static EsfFwMgrResult AllocateAndInitializeSubmoduleOpsList(void) {
  s_num_submodules = sizeof(s_get_ops_functions) /
                     sizeof(s_get_ops_functions[0]);

  s_submodule_ops_list = (EsfFwMgrSubmoduleOps*)malloc(
      s_num_submodules * sizeof(EsfFwMgrSubmoduleOps));
  if (s_submodule_ops_list == NULL) {
    ESF_FW_MGR_DLOG_CRITICAL(
        "Failed to allocate memory for s_submodule_ops_list\n");
    ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0x01CoreAllocOpsList);
    return kEsfFwMgrResultResourceExhausted;
  }

  for (int i = 0; i < s_num_submodules; ++i) {
    InitializeSubmoduleOps(&s_submodule_ops_list[i]);
    EsfFwMgrResult ret = s_get_ops_functions[i](&s_submodule_ops_list[i]);
    if (ret != kEsfFwMgrResultOk) {
      ESF_FW_MGR_DLOG_ERROR(
          "Failed to get ops for submodule: i = %d, ret = %u\n", i, ret);
      ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x01Core);
      free(s_submodule_ops_list);
      s_submodule_ops_list = NULL;
      return ret;
    }
  }

  return kEsfFwMgrResultOk;
}

static EsfFwMgrResult InvokeAllSubmoduleInit(void) {
  EsfFwMgrResult ret = kEsfFwMgrResultOk;

  int num_initialized_submodules = 0;
  for (int i = 0; i < s_num_submodules; ++i) {
    if (s_submodule_ops_list[i].init) {
      ret = s_submodule_ops_list[i].init();
      if (ret != kEsfFwMgrResultOk) {
        ESF_FW_MGR_DLOG_ERROR(
            "Init of submodule failed with ret = %u. (i = %d)\n", ret, i);
        ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x02Core);
        goto err_exit;
      }
    }

    ++num_initialized_submodules;
  }

  return kEsfFwMgrResultOk;

// goto this label only when initialization of a submodule failed.
err_exit:
  // Call Deinit of submodules that has been initialized.
  // Even if an error occurs in Deinit, ignore it (with warning message)
  // because it should be a bug and can not be handled.
  for (int i = 0; i < num_initialized_submodules; ++i) {
    if (s_submodule_ops_list[i].deinit) {
      ret = s_submodule_ops_list[i].deinit();
      if (ret != kEsfFwMgrResultOk) {
        ESF_FW_MGR_DLOG_WARNING(
            "Deinit of a submodule failed. (i = %d). Continue anyway.\n", i);
        ESF_FW_MGR_ELOG_WARNING(kEsfFwMgrElogWarningId0x05Core);
      }
    }
  }

  return kEsfFwMgrResultInternal;
}

EsfFwMgrResult EsfFwMgrInit(void) {
  ESF_FW_MGR_DLOG_INFO("Called.\n");
  ESF_FW_MGR_ELOG_INFO(kEsfFwMgrElogInfoId0x00CoreInit);
  if (pthread_mutex_trylock(&s_main_apis_mutex) != 0) {
    ESF_FW_MGR_DLOG_ERROR("Failed to lock the mutex. errno = %d\n", errno);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x03Core);
    return kEsfFwMgrResultBusy;
  }

  EsfFwMgrResult ret = kEsfFwMgrResultInternal;

  if (s_state != kEsfFwMgrStateUninit) {
    ESF_FW_MGR_DLOG_ERROR("Invalid state. (state = %u)\n", s_state);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x04Core);
    ret = kEsfFwMgrResultFailedPrecondition;
    goto unlock_mutex_then_exit;
  }

  ret = AllocateAndInitializeSubmoduleOpsList();
  if (ret != kEsfFwMgrResultOk) {
    ESF_FW_MGR_DLOG_ERROR("InitializeSubmoduleOpsList failed. ret = %u\n", ret);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x05Core);
    goto unlock_mutex_then_exit;
  }

  ret = InvokeAllSubmoduleInit();
  if (ret != kEsfFwMgrResultOk) {
    ESF_FW_MGR_DLOG_ERROR("InvokeAllSubmoduleInit failed. ret = %u\n", ret);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x06Core);
    free(s_submodule_ops_list);
    goto unlock_mutex_then_exit;
  }

  // NOTE: If any process that can cause error is added here, add code that
  // calls Deinit of all submodules, when an error occurs.

  s_state = kEsfFwMgrStateIdle;
  ret = kEsfFwMgrResultOk;

unlock_mutex_then_exit:
  pthread_mutex_unlock(&s_main_apis_mutex);

  return ret;
}

// Deinit ----------------------------------------------------------------------

static EsfFwMgrResult InvokeAllSubmoduleDeinit(void) {
  for (int i = 0; i < s_num_submodules; ++i) {
    if (s_submodule_ops_list[i].deinit == NULL) continue;

    EsfFwMgrResult ret = s_submodule_ops_list[i].deinit();
    if (ret != kEsfFwMgrResultOk) {
      ESF_FW_MGR_DLOG_ERROR("Deinit of Submodule failed. (i = %d)\n", i);
      ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x07Core);
      return ret;
    }

    // Set NULL to avoid execute Deinit more than once.
    s_submodule_ops_list[i].deinit = NULL;
  }

  return kEsfFwMgrResultOk;
}

EsfFwMgrResult EsfFwMgrDeinit(void) {
  ESF_FW_MGR_DLOG_INFO("Called.\n");
  ESF_FW_MGR_ELOG_INFO(kEsfFwMgrElogInfoId0x06CoreDeinit);
  if (pthread_mutex_trylock(&s_main_apis_mutex) != 0) {
    ESF_FW_MGR_DLOG_ERROR("Failed to lock the mutex. errno = %d\n", errno);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x08Core);
    return kEsfFwMgrResultBusy;
  }

  EsfFwMgrResult ret = kEsfFwMgrResultInternal;

  if (pthread_mutex_lock(&s_sub_apis_mutex) != 0) {
    ESF_FW_MGR_DLOG_ERROR("Failed to lock the mutex. errno=%d\n", errno);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x09Core);
    ret = kEsfFwMgrResultInternal;
    goto unlock_main_mutex_then_exit;
  }

  if (s_state != kEsfFwMgrStateIdle) {
    ESF_FW_MGR_DLOG_ERROR("Invalid state. (state = %u)\n", s_state);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x0aCore);
    ret = kEsfFwMgrResultFailedPrecondition;
    goto unlock_mutexes_then_exit;
  }

  ret = InvokeAllSubmoduleDeinit();
  if (ret != kEsfFwMgrResultOk) {
    ESF_FW_MGR_DLOG_ERROR("InvokeAllSubmoduleDeinit failed. ret = %u\n", ret);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x0bCore);
    goto unlock_mutexes_then_exit;
  }

  free(s_submodule_ops_list);
  s_submodule_ops_list = NULL;
  s_num_submodules = 0;

  s_state = kEsfFwMgrStateUninit;
  ret = kEsfFwMgrResultOk;

unlock_mutexes_then_exit:
  pthread_mutex_unlock(&s_sub_apis_mutex);

unlock_main_mutex_then_exit:
  pthread_mutex_unlock(&s_main_apis_mutex);

  return ret;
}

// Open ------------------------------------------------------------------------

static EsfFwMgrResult AllocateAndInitializeContext(EsfFwMgrContext** context) {
  if (context == NULL) {
    ESF_FW_MGR_DLOG_ERROR("context is NULL.\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x0cCore);
    return kEsfFwMgrResultInvalidArgument;
  }

  if (s_active_context != NULL) {
    // This block will not be executed. But we ensure that s_active_context is
    // NULL. Because if it is not NULL, it will be overwritten and that could
    // cause memory leak.
    ESF_FW_MGR_DLOG_ERROR("A context already exists.\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x0dCore);
    return kEsfFwMgrResultInternal;
  }

  *context = (EsfFwMgrContext*)malloc(sizeof(EsfFwMgrContext));
  if (*context == NULL) {
    ESF_FW_MGR_DLOG_CRITICAL("Failed to allocate memory for the context.\n");
    ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0x02CoreAllocContext);
    return kEsfFwMgrResultResourceExhausted;
  }

  (*context)->submodule_handle = NULL;
  (*context)->submodule_ops = NULL;
  (*context)->submodule_is_open = false;

  (*context)->remaining_write_size = 0;

  (*context)->internal_buffer_state = kEsfFwMgrInternalBufferNotAllocated;
  (*context)->internal_buffer_size = 0;
  (*context)->internal_buffer = NULL;

  (*context)->tmp_buffer_size = 0;
  (*context)->tmp_buffer = NULL;

#if defined(CONFIG_EXTERNAL_TARGET_T3P) || defined(CONFIG_EXTERNAL_TARGET_T5)
  (*context)->dummy_update_is_open = false;
#endif /* CONFIG_EXTERNAL_TARGET_T3P || CONFIG_EXTERNAL_TARGET_T5 */

  (*context)->sha256_freed = true;
  memset((void*)(*context)->hash, 0, sizeof((*context)->hash));

  s_active_context = *context;

  return kEsfFwMgrResultOk;
}

static EsfFwMgrResult AllocateInternalBuffer(
    EsfFwMgrContext* context, int32_t request_size,
    EsfFwMgrPrepareWriteResponse* response) {
  if (context == NULL || response == NULL) {
    ESF_FW_MGR_DLOG_ERROR("context (%p) or response (%p) is NULL\n", context,
                          response);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x0eCore);
    return kEsfFwMgrResultInvalidArgument;
  }

  if (request_size <= 0) {
    ESF_FW_MGR_DLOG_ERROR("request size must be larger than 0, but it is %d.\n",
                          request_size);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x0fCore);
    return kEsfFwMgrResultInvalidArgument;
  }

  if (request_size > CONFIG_EXTERNAL_FIRMWARE_MANAGER_MAX_MEMORY_SIZE) {
    ESF_FW_MGR_DLOG_INFO(
        "Request size is too large. Allocate the max size. (requested: %d, "
        "allocated: %d)\n",
        request_size, CONFIG_EXTERNAL_FIRMWARE_MANAGER_MAX_MEMORY_SIZE);
    request_size = CONFIG_EXTERNAL_FIRMWARE_MANAGER_MAX_MEMORY_SIZE;
  }

  ESF_FW_MGR_DLOG_INFO("Allocate large heap memory.\n");
  ESF_FW_MGR_ELOG_INFO(kEsfFwMgrElogInfoId0x20CoreMemAlloc);

  EsfMemoryManagerResult mem_ret =
      EsfMemoryManagerAllocate(kEsfMemoryManagerTargetLargeHeap, NULL,
                               request_size, &context->internal_buffer_handle);
  if (mem_ret != kEsfMemoryManagerResultSuccess) {
    ESF_FW_MGR_DLOG_CRITICAL("EsfMemoryManagerAllocate failed. ret = %u\n",
                             mem_ret);
    ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0x03CoreAllocBuffer);
    if (mem_ret == kEsfMemoryManagerResultAllocationError) {
      return kEsfFwMgrResultResourceExhausted;
    } else {
      return kEsfFwMgrResultInternal;
    }
  }
  context->internal_buffer_state = kEsfFwMgrInternalBufferAllocated;
  context->internal_buffer_size = request_size;
  response->memory_size = context->internal_buffer_size;

  EsfMemoryManagerMapSupport supported;
  mem_ret = EsfMemoryManagerIsMapSupport(context->internal_buffer_handle,
                                         &supported);
  if (mem_ret != kEsfMemoryManagerResultSuccess) {
    ESF_FW_MGR_DLOG_CRITICAL("EsfMemoryManagerIsMapSupport failed. ret = %u\n",
                             mem_ret);
    ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0x40Core);
    // Free'ing the internal buffer is required: handled by calling
    // FreeContext() in EsfFwMgrOpen, so do nothing here.
    return kEsfFwMgrResultUnavailable;
  }

  context->is_map_supported_for_internal_buffer =
      (supported == kEsfMemoryManagerMapIsSupport);

  if (!context->is_map_supported_for_internal_buffer) {
    context->tmp_buffer_size = 0x8000;  // 32 KB
    context->tmp_buffer = (uint8_t*)malloc(context->tmp_buffer_size);
    if (context->tmp_buffer == NULL) {
      ESF_FW_MGR_DLOG_CRITICAL(
          "Failed to allocate memory for the temporay buffer.\n");
      ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0x04CoreAllocTmpBuffer);
      // Free'ing the internal buffer is required: handled by calling
      // FreeContext() in EsfFwMgrOpen, so do nothing here.
      return kEsfFwMgrResultResourceExhausted;
    }
  }

  return kEsfFwMgrResultOk;
}

static EsfFwMgrResult MapOrOpenInternalBuffer(EsfFwMgrContext* context) {
  if (context == NULL) {
    ESF_FW_MGR_DLOG_ERROR("context is NULL\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x10Core);
    return kEsfFwMgrResultInvalidArgument;
  }

  if (context->internal_buffer_state != kEsfFwMgrInternalBufferAllocated) {
    ESF_FW_MGR_DLOG_ERROR("Invalid internal_buffer_state: %u.\n",
                          context->internal_buffer_state);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x11Core);
    return kEsfFwMgrResultFailedPrecondition;
  }

  void* memory;
  EsfFwMgrResult ret = EsfFwMgrMapOrOpenLargeHeapMemory(
      context->internal_buffer_handle, context->internal_buffer_size,
      context->is_map_supported_for_internal_buffer, &memory);
  if (ret != kEsfFwMgrResultOk) {
    ESF_FW_MGR_DLOG_ERROR(
        "EsfFwMgrMapOrOpenLargeHeapMemory failed. ret = %u.\n", ret);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x6eCore);
    return ret;
  }

  if (context->is_map_supported_for_internal_buffer) {
    context->internal_buffer_state = kEsfFwMgrInternalBufferMapped;
    context->internal_buffer = (uint8_t*)memory;
  } else {
    context->internal_buffer_state = kEsfFwMgrInternalBufferOpen;
  }

  return kEsfFwMgrResultOk;
}

static EsfFwMgrResult UnmapOrCloseInternalBuffer(EsfFwMgrContext* context) {
  if (context == NULL) {
    ESF_FW_MGR_DLOG_ERROR("context is NULL\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x12Core);
    return kEsfFwMgrResultInvalidArgument;
  }

  if ((context->internal_buffer_state != kEsfFwMgrInternalBufferMapped) &&
      (context->internal_buffer_state != kEsfFwMgrInternalBufferOpen)) {
    return kEsfFwMgrResultOk;
  }

  EsfFwMgrResult ret = EsfFwMgrUnmapOrCloseLargeHeapMemory(
      context->internal_buffer_handle,
      context->internal_buffer_state == kEsfFwMgrInternalBufferMapped);
  if (ret != kEsfFwMgrResultOk) {
    ESF_FW_MGR_DLOG_ERROR(
        "EsfFwMgrUnmapOrCloseLargeHeapMemory failed. ret = %u\n", ret);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x6fCore);
    return ret;
  }

  context->internal_buffer_state = kEsfFwMgrInternalBufferAllocated;
  context->internal_buffer = NULL;

  return kEsfFwMgrResultOk;
}

static EsfFwMgrResult FreeInternalBuffer(EsfFwMgrContext* context) {
  if (context == NULL) {
    ESF_FW_MGR_DLOG_ERROR("context is NULL\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x13Core);
    return kEsfFwMgrResultInvalidArgument;
  }

  free(context->tmp_buffer);
  context->tmp_buffer = NULL;
  context->tmp_buffer_size = 0;

  if (context->internal_buffer_state == kEsfFwMgrInternalBufferNotAllocated)
    return kEsfFwMgrResultOk;

  // If the internal buffer has not been unmapped or closed, return an error.
  if (context->internal_buffer_state != kEsfFwMgrInternalBufferAllocated) {
    ESF_FW_MGR_DLOG_ERROR("Invalid internal_buffer_state: %u.\n",
                          context->internal_buffer_state);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x14Core);
    return kEsfFwMgrResultFailedPrecondition;
  }

  ESF_FW_MGR_DLOG_INFO("Free large heap memory.\n");
  ESF_FW_MGR_ELOG_INFO(kEsfFwMgrElogInfoId0x21CoreMemFree);
  EsfMemoryManagerResult mem_ret =
      EsfMemoryManagerFree(context->internal_buffer_handle, NULL);
  if (mem_ret != kEsfMemoryManagerResultSuccess) {
    ESF_FW_MGR_DLOG_CRITICAL("EsfMemoryManagerFree failed. ret = %u.\n",
                             mem_ret);
    ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0x09CoreFreeBuffer);

    // The error from EsfMemoryManagerFree indicates either an invalid
    // argument or an attempt to free memory that has already been free'd,
    // both of which are implementation bugs. Therefore, this function returns
    // an internal error.
    return kEsfFwMgrResultInternal;
  }

  context->internal_buffer_state = kEsfFwMgrInternalBufferNotAllocated;
  context->internal_buffer_size = 0;

  return kEsfFwMgrResultOk;
}

// Try to unmap the internal buffer and then free it. If either of them failed,
// return an error.
static EsfFwMgrResult UnmapOrCloseThenFreeInternalBuffer(
    EsfFwMgrContext* context) {
  if (context == NULL) {
    ESF_FW_MGR_DLOG_ERROR("context is NULL\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x15Core);
    return kEsfFwMgrResultInvalidArgument;
  }

  EsfFwMgrResult unmap_ret = UnmapOrCloseInternalBuffer(context);

  // Try to Free the buffer even if the unmap failed to minimize the risk of
  // memory leak.
  EsfFwMgrResult free_ret = FreeInternalBuffer(context);

  if (unmap_ret != kEsfFwMgrResultOk || free_ret != kEsfFwMgrResultOk) {
    ESF_FW_MGR_DLOG_ERROR(
        "Unmap or Free failed (unmap_ret = %u, free_ret = %u).\n", unmap_ret,
        free_ret);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x16Core);
    return kEsfFwMgrResultInternal;
  }

  return kEsfFwMgrResultOk;
}

static EsfFwMgrResult ChooseSubmoduleOps(EsfFwMgrTarget target,
                                         EsfFwMgrSubmoduleOps** ops) {
  if (ops == NULL) {
    ESF_FW_MGR_DLOG_ERROR("ops is NULL.\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x17Core);
    return kEsfFwMgrResultInvalidArgument;
  }

  for (int i = 0; i < s_num_submodules; ++i) {
    if (s_submodule_ops_list[i].is_supported == NULL) {
      ESF_FW_MGR_DLOG_WARNING("is_supported is NULL (i = %d).\n", i);
      ESF_FW_MGR_ELOG_WARNING(kEsfFwMgrElogWarningId0x06Core);
      continue;
    }

    if (s_submodule_ops_list[i].is_supported(target)) {
      *ops = &s_submodule_ops_list[i];
      return kEsfFwMgrResultOk;
    }
  }

  ESF_FW_MGR_DLOG_WARNING("No submodule supports the target: %u\n", target);
  ESF_FW_MGR_ELOG_WARNING(kEsfFwMgrElogWarningId0x07Core);
  return kEsfFwMgrResultUnimplemented;
}

static EsfFwMgrResult PrepareForWrite(
    EsfFwMgrContext* context, const EsfFwMgrOpenRequest* request,
    const EsfFwMgrPrepareWriteRequest* prepare_write,
    EsfFwMgrPrepareWriteResponse* prepare_write_response) {
  if (context == NULL || request == NULL || prepare_write == NULL ||
      prepare_write_response == NULL) {
    ESF_FW_MGR_DLOG_ERROR(
        "One or more arguments are NULL. context: %p, request: %p, "
        "prepare_write: %p, prepare_write_response: %p\n",
        context, request, prepare_write, prepare_write_response);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x18Core);
    return kEsfFwMgrResultInvalidArgument;
  }

  mbedtls_sha256_init(&context->sha256_context);
  context->sha256_freed = false;
  ESF_FW_MGR_HANDLE_MBEDTLS_ERROR(
      mbedtls_sha256_starts(&context->sha256_context, 0), {
        ESF_FW_MGR_DLOG_ERROR("mbedtls_sha256_starts failed. ret = %d\n", r);
        ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x19Core);
        // Free'ing the sha256 context is required: handled by calling
        // FreeContext() in EsfFwMgrOpen, so do nothing here.
        return kEsfFwMgrResultInternal;
      });
  memcpy(context->hash, request->hash, sizeof(context->hash));

  EsfFwMgrResult ret = AllocateInternalBuffer(
      context, prepare_write->memory_size, prepare_write_response);
  if (ret != kEsfFwMgrResultOk) {
    ESF_FW_MGR_DLOG_ERROR("AllocateInternalBuffer failed. ret = %u\n", ret);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x1aCore);
    return ret;
  }

  ret = MapOrOpenInternalBuffer(context);
  if (ret != kEsfFwMgrResultOk) {
    ESF_FW_MGR_DLOG_ERROR("MapOrOpenInternalBuffer failed (ret = %u).\n", ret);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x1bCore);
    // Free'ing the internal buffer and sha256 context is required: handled by
    // calling FreeContext() in EsfFwMgrOpen, so do nothing here.
    return ret;
  }

  context->remaining_write_size = prepare_write->total_size;
  return kEsfFwMgrResultOk;
}

static void FreeContext(EsfFwMgrContext* context) {
  if (context == NULL) {
    ESF_FW_MGR_DLOG_WARNING("context is NULL.\n");
    ESF_FW_MGR_ELOG_WARNING(kEsfFwMgrElogWarningId0x08Core);
    return;
  }

  // Use tmp_ret because ret should not be overwritten.
  EsfFwMgrResult ret = UnmapOrCloseThenFreeInternalBuffer(context);
  if (ret != kEsfFwMgrResultOk) {
    ESF_FW_MGR_DLOG_ERROR(
        "UnmapOrCloseThenFreeInternalBuffer failed (ret = %u). Continue "
        "anyway.\n",
        ret);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x1cCore);
    // Continue to process. (It is necessary to free the context even if
    // UnmapOrCloseThenFreeInternalBuffer failed)
  }

#if defined(CONFIG_EXTERNAL_TARGET_T3P) || defined(CONFIG_EXTERNAL_TARGET_T5)
  ret = UnblockStreaming(context);
  if (ret != kEsfFwMgrResultOk) {
    ESF_FW_MGR_DLOG_ERROR(
        "UnblockStreaming failed. ret = %u. Continue anyway.\n", ret);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x1dCore);
    // Continue to process.
  }
#endif /* CONFIG_EXTERNAL_TARGET_T3P || CONFIG_EXTERNAL_TARGET_T5 */

  if (!context->sha256_freed) {
    mbedtls_sha256_free(&context->sha256_context);
    context->sha256_freed = true;
  }

  free(context);
  s_active_context = NULL;
}

static EsfFwMgrResult InvokeSubmoduleOpen(
    EsfFwMgrContext* context, const EsfFwMgrOpenRequest* request,
    const EsfFwMgrPrepareWriteRequest* prepare_write,
    EsfFwMgrOpenResponse* response) {
  if (context == NULL || response == NULL) {
    ESF_FW_MGR_DLOG_ERROR("context (%p) or request (%p) is NULL.\n", context,
                          request);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x1eCore);
    return kEsfFwMgrResultInvalidArgument;
  }

  if (context->submodule_is_open == true) {
    ESF_FW_MGR_DLOG_ERROR("Submodule has already been open.\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x1fCore);
    return kEsfFwMgrResultFailedPrecondition;
  }

  if (context->submodule_ops == NULL || context->submodule_ops->open == NULL) {
    ESF_FW_MGR_DLOG_ERROR("Open of the submodule is not registered.\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x20Core);
    return kEsfFwMgrResultInternal;
  }

  int32_t* writable_size = NULL;
  EsfFwMgrSubmodulePrepareWriteRequest* p_sub_prepare_write = NULL;
  EsfFwMgrSubmodulePrepareWriteRequest sub_prepare_write;
  if (prepare_write) {
    sub_prepare_write.total_size = prepare_write->total_size;
    sub_prepare_write.internal_buffer_size = context->internal_buffer_size;
    sub_prepare_write.internal_buffer_handle = context->internal_buffer_handle;
    p_sub_prepare_write = &sub_prepare_write;
    writable_size = &response->prepare_write.writable_size;
  }

  EsfFwMgrDummyUpdateHandleInfo dummy_update_handle_info = {
#if defined(CONFIG_EXTERNAL_TARGET_T3P) || defined(CONFIG_EXTERNAL_TARGET_T5)
      .handle = context->dummy_update_handle,
      .canceled = false,
#endif /* CONFIG_EXTERNAL_TARGET_T3P || CONFIG_EXTERNAL_TARGET_T5 */
  };

  EsfFwMgrResult ret = context->submodule_ops->open(
      request, p_sub_prepare_write, &dummy_update_handle_info,
      &context->submodule_handle, writable_size);
#if defined(CONFIG_EXTERNAL_TARGET_T3P) || defined(CONFIG_EXTERNAL_TARGET_T5)
  if (dummy_update_handle_info.canceled) context->dummy_update_is_open = false;
#endif /* CONFIG_EXTERNAL_TARGET_T3P || CONFIG_EXTERNAL_TARGET_T5 */
  if (ret != kEsfFwMgrResultOk) {
    ESF_FW_MGR_DLOG_ERROR("context->submodule_ops->open failed. ret = %u", ret);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x21Core);
    return ret;
  }

  context->submodule_is_open = true;
  return kEsfFwMgrResultOk;
}

EsfFwMgrResult EsfFwMgrOpen(const EsfFwMgrOpenRequest* request,
                            const EsfFwMgrPrepareWriteRequest* prepare_write,
                            EsfFwMgrOpenResponse* response) {
  ESF_FW_MGR_DLOG_INFO("Called.\n");
  ESF_FW_MGR_ELOG_INFO(kEsfFwMgrElogInfoId0x01CoreOpen);
  if (pthread_mutex_trylock(&s_main_apis_mutex) != 0) {
    ESF_FW_MGR_DLOG_ERROR("Failed to lock the mutex. errno = %d\n", errno);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x22Core);
    return kEsfFwMgrResultBusy;
  }

  EsfFwMgrResult ret = kEsfFwMgrResultInternal;

  if (s_state != kEsfFwMgrStateIdle) {
    ESF_FW_MGR_DLOG_ERROR("Invalid state. (state = %u)\n", s_state);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x23Core);
    ret = kEsfFwMgrResultFailedPrecondition;
    goto unlock_mutex_then_exit;
  }

  if (request == NULL || response == NULL) {
    ESF_FW_MGR_DLOG_ERROR("request or response is NULL\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x24Core);
    ret = kEsfFwMgrResultInvalidArgument;
    goto unlock_mutex_then_exit;
  }

#if defined(CONFIG_EXTERNAL_TARGET_T3P) || defined(CONFIG_EXTERNAL_TARGET_T5)
  // Check Sensor Lib State to avoid allocating memory for the context when FW
  // manager can not be opened. (BlockStreaming succeeds only when Sensor AI
  // Lib state is standby)
  SsfSensorLibState sensor_state = SsfSensorLibGetState();
  if (sensor_state != kSsfSensorLibStateStandby) {
    ESF_FW_MGR_DLOG_ERROR("Sensor state is not standby. (state = %u)\n",
                          sensor_state);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x25Core);
    ret = kEsfFwMgrResultUnavailable;
    goto unlock_mutex_then_exit;
  }
#endif /* CONFIG_EXTERNAL_TARGET_T3P || CONFIG_EXTERNAL_TARGET_T5 */

  EsfFwMgrContext* context;
  ret = AllocateAndInitializeContext(&context);
  if (ret != kEsfFwMgrResultOk) {
    ESF_FW_MGR_DLOG_ERROR("AllocateAndInitializeContext failed. ret = %u\n",
                          ret);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x26Core);
    goto unlock_mutex_then_exit;
  }

#if defined(CONFIG_EXTERNAL_TARGET_T3P) || defined(CONFIG_EXTERNAL_TARGET_T5)
  ret = BlockStreaming(context);
  if (ret != kEsfFwMgrResultOk) {
    ESF_FW_MGR_DLOG_ERROR("BlockStreaming failed. ret = %u", ret);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x27Core);
    goto free_context_and_unlock_mutex_then_exit;
  }
#endif /* CONFIG_EXTERNAL_TARGET_T3P || CONFIG_EXTERNAL_TARGET_T5 */

  ret = ChooseSubmoduleOps(request->target, &context->submodule_ops);
  if (ret != kEsfFwMgrResultOk) {
    ESF_FW_MGR_DLOG_ERROR("ChooseSubmoduleOps failed. ret = %u\n", ret);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x28Core);
    goto free_context_and_unlock_mutex_then_exit;
  }

  if (prepare_write) {
    ret = PrepareForWrite(context, request, prepare_write,
                          &response->prepare_write);
    if (ret != kEsfFwMgrResultOk) {
      ESF_FW_MGR_DLOG_ERROR("PrepareForWrite failed. ret = %u\n", ret);
      ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x29Core);
      goto free_context_and_unlock_mutex_then_exit;
    }
  } else {
    if (context->submodule_ops->erase == NULL) {
      ret = kEsfFwMgrResultUnimplemented;
      ESF_FW_MGR_DLOG_ERROR("Erase for the target = %u is not supported.\n",
                            request->target);
      ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x2aCore);
      goto free_context_and_unlock_mutex_then_exit;
    }
  }

  ret = InvokeSubmoduleOpen(context, request, prepare_write, response);
  if (ret != kEsfFwMgrResultOk) {
    ESF_FW_MGR_DLOG_ERROR("InvokeSubmoduleOpen failed. ret = %u\n", ret);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x2bCore);
    goto free_context_and_unlock_mutex_then_exit;
  }

  // CAUTION: If you add any process that may fail here (after
  // InvokeSubmoduleOpen), add error handling that calls InvokeSubmoduleClose)

  if (prepare_write) {
    s_state = kEsfFwMgrStateWritable;
  } else {
    s_state = kEsfFwMgrStateErasable;
  }

  response->handle = (EsfFwMgrHandle)context;

  ret = kEsfFwMgrResultOk;

  goto unlock_mutex_then_exit;

// goto this label when an error occurs after allocating context.
free_context_and_unlock_mutex_then_exit:
  FreeContext(context);

unlock_mutex_then_exit:
  pthread_mutex_unlock(&s_main_apis_mutex);

  return ret;
}

// Close -----------------------------------------------------------------------

static EsfFwMgrResult InvokeSubmoduleClose(EsfFwMgrContext* context) {
  if (context == NULL) {
    ESF_FW_MGR_DLOG_ERROR("context is NULL.\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x2cCore);
    return kEsfFwMgrResultInvalidArgument;
  }

  if (context->submodule_is_open == false) {
    ESF_FW_MGR_DLOG_WARNING("Submodule is not open. Skip closing\n");
    ESF_FW_MGR_ELOG_WARNING(kEsfFwMgrElogWarningId0x09Core);
    return kEsfFwMgrResultOk;
  }

  if (context->submodule_ops == NULL || context->submodule_ops->close == NULL) {
    ESF_FW_MGR_DLOG_ERROR("Close of the submodule is not registered.\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x2dCore);
    return kEsfFwMgrResultInternal;
  }

  bool aborted = (s_state != kEsfFwMgrStateDone);
  EsfFwMgrResult ret = context->submodule_ops->close(context->submodule_handle,
                                                     aborted);
  if (ret != kEsfFwMgrResultOk) {
    ESF_FW_MGR_DLOG_ERROR("context->submodule_ops->close failed. ret = %u.\n",
                          ret);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x2eCore);
    return ret;
  }

  context->submodule_is_open = false;
  return kEsfFwMgrResultOk;
}

static EsfFwMgrResult CloseForWrite(EsfFwMgrContext* context) {
  if (context == NULL) {
    ESF_FW_MGR_DLOG_ERROR("context is NULL.\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x2fCore);
    return kEsfFwMgrResultInvalidArgument;
  }

  if (!context->sha256_freed) {
    mbedtls_sha256_free(&context->sha256_context);
    context->sha256_freed = true;
  }

  EsfFwMgrResult ret = UnmapOrCloseThenFreeInternalBuffer(context);
  if (ret != kEsfFwMgrResultOk) {
    ESF_FW_MGR_DLOG_ERROR(
        "UnmapOrCloseThenFreeInternalBuffer failed. ret = %u\n", ret);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x30Core);
    return ret;
  }

  return kEsfFwMgrResultOk;
}

EsfFwMgrResult EsfFwMgrClose(EsfFwMgrHandle handle) {
  ESF_FW_MGR_DLOG_INFO("Called.\n");
  ESF_FW_MGR_ELOG_INFO(kEsfFwMgrElogInfoId0x05CoreClose);
  if (pthread_mutex_trylock(&s_main_apis_mutex) != 0) {
    ESF_FW_MGR_DLOG_ERROR("Failed to lock the mutex. errno = %d\n", errno);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x31Core);
    return kEsfFwMgrResultBusy;
  }

  EsfFwMgrResult ret = kEsfFwMgrResultInternal;

  if (s_state != kEsfFwMgrStateDone && s_state != kEsfFwMgrStateWritable &&
      s_state != kEsfFwMgrStateErasable && s_state != kEsfFwMgrStateError) {
    ESF_FW_MGR_DLOG_ERROR("Invalid state. (state = %u)\n", s_state);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x32Core);
    ret = kEsfFwMgrResultFailedPrecondition;
    goto unlock_mutex_then_exit;
  }

  if (!ESF_FW_MGR_VERIFY_HANDLE(handle)) {
    ESF_FW_MGR_DLOG_ERROR("Invalid handle.\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x33Core);
    ret = kEsfFwMgrResultInvalidArgument;
    goto unlock_mutex_then_exit;
  }

  EsfFwMgrContext* context = (EsfFwMgrContext*)handle;

  ret = CloseForWrite(context);
  if (ret != kEsfFwMgrResultOk) {
    ESF_FW_MGR_DLOG_ERROR("CloseForWrite failed. ret = %u\n", ret);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x34Core);
    goto unlock_mutex_then_exit;
  }

  ret = InvokeSubmoduleClose(context);
  if (ret != kEsfFwMgrResultOk) {
    ESF_FW_MGR_DLOG_ERROR("InvokeSubmoduleClose failed. ret = %u\n", ret);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x35Core);
    goto unlock_mutex_then_exit;
  }

#if defined(CONFIG_EXTERNAL_TARGET_T3P) || defined(CONFIG_EXTERNAL_TARGET_T5)
  ret = UnblockStreaming(context);
  if (ret != kEsfFwMgrResultOk) {
    ESF_FW_MGR_DLOG_ERROR("UnblockStreaming failed. ret = %u\n", ret);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x36Core);
    goto unlock_mutex_then_exit;
  }
#endif /* CONFIG_EXTERNAL_TARGET_T3P || CONFIG_EXTERNAL_TARGET_T5 */

  free(context);
  s_active_context = NULL;

  s_state = kEsfFwMgrStateIdle;
  ret = kEsfFwMgrResultOk;

unlock_mutex_then_exit:
  pthread_mutex_unlock(&s_main_apis_mutex);

  return ret;
}

// CopyToInternalBuffer --------------------------------------------------------
EsfFwMgrResult EsfFwMgrCopyToInternalBuffer(
    EsfFwMgrHandle handle, const EsfFwMgrCopyToInternalBufferRequest* request) {
  ESF_FW_MGR_DLOG_DEBUG("Called.\n");
  if (pthread_mutex_trylock(&s_main_apis_mutex) != 0) {
    ESF_FW_MGR_DLOG_ERROR("Failed to lock the mutex. errno = %d\n", errno);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x37Core);
    return kEsfFwMgrResultBusy;
  }

  EsfFwMgrResult ret = kEsfFwMgrResultInternal;

  if (s_state != kEsfFwMgrStateWritable) {
    ESF_FW_MGR_DLOG_ERROR("Invalid state. (state = %u)\n", s_state);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x38Core);
    ret = kEsfFwMgrResultFailedPrecondition;
    goto unlock_mutex_then_exit;
  }

  if (!ESF_FW_MGR_VERIFY_HANDLE(handle)) {
    ESF_FW_MGR_DLOG_ERROR("Invalid handle.\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x39Core);
    ret = kEsfFwMgrResultInvalidArgument;
    goto unlock_mutex_then_exit;
  }

  if (request == NULL || request->data == NULL) {
    ESF_FW_MGR_DLOG_ERROR("request or request->data is NULL.\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x3aCore);
    ret = kEsfFwMgrResultInvalidArgument;
    goto unlock_mutex_then_exit;
  }

  EsfFwMgrContext* context = (EsfFwMgrContext*)handle;

  if (request->offset < 0 || request->size <= 0 ||
      context->internal_buffer_size < request->offset + request->size) {
    ESF_FW_MGR_DLOG_ERROR(
        "Invalid offset or size. offset: %d, size: %d (Internal buffer size: "
        "%d\n",
        request->offset, request->size, context->internal_buffer_size);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x3bCore);
    ret = kEsfFwMgrResultInvalidArgument;
    goto unlock_mutex_then_exit;
  }

  if (context->internal_buffer_state == kEsfFwMgrInternalBufferMapped) {
    memcpy(context->internal_buffer + request->offset, request->data,
           request->size);

  } else if (context->internal_buffer_state == kEsfFwMgrInternalBufferOpen) {
    off_t result_offset = 0;
    EsfMemoryManagerResult mem_ret =
        EsfMemoryManagerFseek(context->internal_buffer_handle, request->offset,
                              SEEK_SET, &result_offset);
    if ((mem_ret != kEsfMemoryManagerResultSuccess) ||
        (request->offset != result_offset)) {
      ESF_FW_MGR_DLOG_CRITICAL(
          "EsfMemoryManagerFseek failed. ret = %u, buffer_offset = %d, "
          "result_offset = %ld.\n",
          mem_ret, request->offset, result_offset);
      ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0x41Core);
      ret = kEsfFwMgrResultUnavailable;
      goto unlock_mutex_then_exit;
    }

    size_t rsize = 0;
    mem_ret = EsfMemoryManagerFwrite(context->internal_buffer_handle,
                                     request->data, request->size, &rsize);
    if ((mem_ret != kEsfMemoryManagerResultSuccess) ||
        ((size_t)request->size != rsize)) {
      ESF_FW_MGR_DLOG_CRITICAL(
          "EsfMemoryManagerFwrite failed. ret = %u, requested write size = %d, "
          "actually written size = %lu\n",
          mem_ret, request->size, rsize);
      ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0x42Core);
      ret = kEsfFwMgrResultUnavailable;
      goto unlock_mutex_then_exit;
    }

  } else {
    ESF_FW_MGR_DLOG_ERROR("Internal buffer has not been mapped or opened.\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x3cCore);
    ret = kEsfFwMgrResultAborted;
    goto unlock_mutex_then_exit;
  }

  ret = kEsfFwMgrResultOk;

unlock_mutex_then_exit:
  if (ret == kEsfFwMgrResultAborted) {
    ESF_FW_MGR_DLOG_WARNING(
        "Firmware Manager state transit to the error state.\n");
    ESF_FW_MGR_ELOG_WARNING(kEsfFwMgrElogWarningId0x0aCore);
    s_state = kEsfFwMgrStateError;
  }
  pthread_mutex_unlock(&s_main_apis_mutex);

  return ret;
}

// Write -----------------------------------------------------------------------
static EsfFwMgrResult InvokeSubmoduleWrite(
    EsfFwMgrContext* context, const EsfFwMgrWriteRequest* request) {
  if (context == NULL) {
    ESF_FW_MGR_DLOG_ERROR("context is NULL.\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x3dCore);
    return kEsfFwMgrResultInvalidArgument;
  }

  if (context->submodule_ops == NULL || context->submodule_ops->write == NULL) {
    ESF_FW_MGR_DLOG_ERROR("Write of the submodule is not registered.\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x3eCore);
    return kEsfFwMgrResultInternal;
  }

  return context->submodule_ops->write(context->submodule_handle, request);
}

static EsfFwMgrResult UpdateSha256Hash(EsfFwMgrContext* context,
                                       const EsfFwMgrWriteRequest* request) {
  if (context == NULL) {
    ESF_FW_MGR_DLOG_ERROR("context is NULL.\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x3fCore);
    return kEsfFwMgrResultInvalidArgument;
  }

  if (request->offset < 0 || request->size <= 0 ||
      context->internal_buffer_size < request->offset + request->size) {
    ESF_FW_MGR_DLOG_ERROR(
        "Invalid offset or size. offset: %d, size: %d (Internal buffer size: "
        "%d\n",
        request->offset, request->size, context->internal_buffer_size);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x6cCore);
    return kEsfFwMgrResultInvalidArgument;
  }

  if (context->internal_buffer_state == kEsfFwMgrInternalBufferMapped) {
    ESF_FW_MGR_HANDLE_MBEDTLS_ERROR(
        mbedtls_sha256_update(&context->sha256_context,
                              context->internal_buffer + request->offset,
                              request->size),
        {
          ESF_FW_MGR_DLOG_ERROR("mbedtls_sha256_update failed. ret = %d\n", r);
          ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x40Core);
          return kEsfFwMgrResultAborted;
        });

  } else if (context->internal_buffer_state == kEsfFwMgrInternalBufferOpen) {
    off_t result_offset = 0;
    EsfMemoryManagerResult mem_ret =
        EsfMemoryManagerFseek(context->internal_buffer_handle, request->offset,
                              SEEK_SET, &result_offset);
    if ((mem_ret != kEsfMemoryManagerResultSuccess) ||
        (request->offset != result_offset)) {
      ESF_FW_MGR_DLOG_CRITICAL(
          "EsfMemoryManagerFseek failed. ret = %u, buffer_offset = %d, "
          "result_offset = %ld.\n",
          mem_ret, request->offset, result_offset);
      ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0x43Core);
      return kEsfFwMgrResultUnavailable;
    }

    if ((context->tmp_buffer == NULL) || (context->tmp_buffer_size <= 0)) {
      ESF_FW_MGR_DLOG_ERROR("Invalid tmp buffer: address = %p, size = %d.\n",
                            context->tmp_buffer, context->tmp_buffer_size);
      ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x41Core);
      return kEsfFwMgrResultInvalidArgument;
    }

    EsfFwMgrResult ret = kEsfFwMgrResultInternal;

    bool error_occurred = false;
    int32_t remaining_size = request->size;
    while (remaining_size > 0) {
      int32_t size = remaining_size;
      if (size > context->tmp_buffer_size) size = context->tmp_buffer_size;

      size_t read_size = 0;
      mem_ret = EsfMemoryManagerFread(context->internal_buffer_handle,
                                      context->tmp_buffer, size, &read_size);
      if ((mem_ret != kEsfMemoryManagerResultSuccess) ||
          ((size_t)size != read_size)) {
        ESF_FW_MGR_DLOG_CRITICAL(
            "EsfMemoryManagerFread failed. ret = %u (size = %d, read_size = "
            "%lu)\n",
            mem_ret, size, read_size);
        ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0x44Core);
        error_occurred = true;
        ret = kEsfFwMgrResultUnavailable;
        break;
      }
      ESF_FW_MGR_HANDLE_MBEDTLS_ERROR(
          mbedtls_sha256_update(&context->sha256_context, context->tmp_buffer,
                                size),
          {
            ESF_FW_MGR_DLOG_ERROR("mbedtls_sha256_update failed. ret = %d\n",
                                  r);
            ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x42Core);
            error_occurred = true;
            ret = kEsfFwMgrResultAborted;
            break;
          });
      remaining_size -= size;
    }

    if (error_occurred) {
      return ret;
    }

  } else {
    ESF_FW_MGR_DLOG_ERROR("Invalid internal_buffer_state: %u.\n",
                          context->internal_buffer_state);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x43Core);
    return kEsfFwMgrResultFailedPrecondition;
  }

  return kEsfFwMgrResultOk;
}

EsfFwMgrResult EsfFwMgrWrite(EsfFwMgrHandle handle,
                             const EsfFwMgrWriteRequest* request) {
  ESF_FW_MGR_DLOG_INFO("Called.\n");
  ESF_FW_MGR_ELOG_INFO(kEsfFwMgrElogInfoId0x02CoreWrite);
  if (pthread_mutex_trylock(&s_main_apis_mutex) != 0) {
    ESF_FW_MGR_DLOG_ERROR("Failed to lock the mutex. errno = %d\n", errno);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x44Core);
    return kEsfFwMgrResultBusy;
  }

  EsfFwMgrResult ret = kEsfFwMgrResultInternal;

  if (s_state != kEsfFwMgrStateWritable) {
    ESF_FW_MGR_DLOG_ERROR("Invalid state. (state = %u)\n", s_state);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x45Core);
    ret = kEsfFwMgrResultFailedPrecondition;
    goto unlock_mutex_then_exit;
  }

  if (!ESF_FW_MGR_VERIFY_HANDLE(handle)) {
    ESF_FW_MGR_DLOG_ERROR("Invalid handle.\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x46Core);
    ret = kEsfFwMgrResultInvalidArgument;
    goto unlock_mutex_then_exit;
  }

  if (request == NULL) {
    ESF_FW_MGR_DLOG_ERROR("response is NULL.\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x47Core);
    ret = kEsfFwMgrResultInvalidArgument;
    goto unlock_mutex_then_exit;
  }

  EsfFwMgrContext* context = (EsfFwMgrContext*)handle;

  // Unmap the internal buffer, because another module will map it in
  // InvokeSubmoduleWrite
  ret = UnmapOrCloseInternalBuffer(context);
  if (ret != kEsfFwMgrResultOk) {
    ESF_FW_MGR_DLOG_ERROR("UnmapOrCloseInternalBuffer failed. ret = %u\n", ret);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x48Core);
    ret = kEsfFwMgrResultAborted;
    goto unlock_mutex_then_exit;
  }

  ret = InvokeSubmoduleWrite(context, request);
  if (ret != kEsfFwMgrResultOk) {
    ESF_FW_MGR_DLOG_ERROR("InvokeSubmoduleWrite failed. ret = %u\n", ret);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x49Core);
    goto unlock_mutex_then_exit;
  }

  ret = MapOrOpenInternalBuffer(context);
  if (ret != kEsfFwMgrResultOk) {
    ESF_FW_MGR_DLOG_ERROR("MapOrOpenInternalBuffer failed (ret = %u)\n", ret);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x4aCore);
    ret = kEsfFwMgrResultAborted;
    goto unlock_mutex_then_exit;
  }

  ret = UpdateSha256Hash(context, request);
  if (ret != kEsfFwMgrResultOk) {
    ESF_FW_MGR_DLOG_ERROR("UpdateSha256Hash failed (ret = %u)\n", ret);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x4bCore);
    ret = kEsfFwMgrResultAborted;
    goto unlock_mutex_then_exit;
  }

  ret = kEsfFwMgrResultOk;

unlock_mutex_then_exit:
  if (ret == kEsfFwMgrResultAborted) {
    ESF_FW_MGR_DLOG_WARNING(
        "Firmware Manager state transit to the error state.\n");
    ESF_FW_MGR_ELOG_WARNING(kEsfFwMgrElogWarningId0x0bCore);
    s_state = kEsfFwMgrStateError;
  }

  pthread_mutex_unlock(&s_main_apis_mutex);

  return ret;
}

// Post process ----------------------------------------------------------------
static EsfFwMgrResult VerifyHash(EsfFwMgrContext* context) {
  if (context == NULL) {
    ESF_FW_MGR_DLOG_ERROR("context is NULL.\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x4cCore);
    return kEsfFwMgrResultInvalidArgument;
  }

  uint8_t hash[ESF_FIRMWARE_MANAGER_TARGET_HASH_SIZE];
  memset(hash, 0, sizeof(hash));

  ESF_FW_MGR_HANDLE_MBEDTLS_ERROR(
      mbedtls_sha256_finish(&context->sha256_context, hash), {
        ESF_FW_MGR_DLOG_ERROR("mbedtls_sha256_finish failed. ret = %d\n", r);
        ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x4dCore);
        return kEsfFwMgrResultInternal;
      });
  if (0 != (memcmp(context->hash, hash, sizeof(context->hash)))) {
    ESF_FW_MGR_DLOG_CRITICAL("Hash verification failed.\n");
    ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0x0aCoreVerifyHash);

    char str[ESF_FIRMWARE_MANAGER_TARGET_HASH_SIZE * 2 + 1];
    EsfFwMgrHashToHexString(sizeof(hash), hash, str);
    ESF_FW_MGR_DLOG_ERROR("calculated: %s\n", str);
    EsfFwMgrHashToHexString(sizeof(context->hash), context->hash, str);
    ESF_FW_MGR_DLOG_ERROR("Expected  : %s\n", str);

    return kEsfFwMgrResultAborted;
  }

  return kEsfFwMgrResultOk;
}

static EsfFwMgrResult InvokeSubmodulePostProcess(EsfFwMgrContext* context) {
  if (context == NULL) {
    ESF_FW_MGR_DLOG_ERROR("context is NULL.\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x4eCore);
    return kEsfFwMgrResultInvalidArgument;
  }

  if (context->submodule_ops == NULL ||
      context->submodule_ops->post_process == NULL) {
    ESF_FW_MGR_DLOG_ERROR("PostProcess of the submodule is not registered.\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x4fCore);
    return kEsfFwMgrResultInternal;
  }
  return context->submodule_ops->post_process(context->submodule_handle);
}

EsfFwMgrResult EsfFwMgrPostProcess(EsfFwMgrHandle handle) {
  ESF_FW_MGR_DLOG_INFO("Called.\n");
  ESF_FW_MGR_ELOG_INFO(kEsfFwMgrElogInfoId0x03CorePostProcess);
  if (pthread_mutex_trylock(&s_main_apis_mutex) != 0) {
    ESF_FW_MGR_DLOG_ERROR("Failed to lock the mutex. errno = %d\n", errno);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x50Core);
    return kEsfFwMgrResultBusy;
  }

  EsfFwMgrResult ret = kEsfFwMgrResultInternal;

  if (s_state != kEsfFwMgrStateWritable) {
    ESF_FW_MGR_DLOG_ERROR("Invalid state. (state = %u)\n", s_state);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x51Core);
    ret = kEsfFwMgrResultFailedPrecondition;
    goto unlock_mutex_then_exit;
  }

  if (!ESF_FW_MGR_VERIFY_HANDLE(handle)) {
    ESF_FW_MGR_DLOG_ERROR("Invalid handle.\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x52Core);
    ret = kEsfFwMgrResultInvalidArgument;
    goto unlock_mutex_then_exit;
  }

  EsfFwMgrContext* context = (EsfFwMgrContext*)handle;

// Skip checking size
// TODO: use this block
#if 0
  if (context->remaining_write_size > 0) {
    ESF_FW_MGR_DLOG_ERROR("All data has not been written. (remaining size: %d\n",
                         context->remaining_write_size);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x53Core);
    ret = kEsfFwMgrResultFailedPrecondition;
    goto unlock_mutex_then_exit;
  }
#endif

  ret = VerifyHash(context);
  if (ret != kEsfFwMgrResultOk) {
    ESF_FW_MGR_DLOG_ERROR("VerifyHash failed. ret = %u\n", ret);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x54Core);
    goto unlock_mutex_then_exit;
  }

  ret = InvokeSubmodulePostProcess(context);
  if (ret != kEsfFwMgrResultOk) {
    ESF_FW_MGR_DLOG_ERROR("InvokeSubmodulePostProcess failed. ret = %u\n", ret);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x55Core);
    goto unlock_mutex_then_exit;
  }
  // End of post process

  s_state = kEsfFwMgrStateDone;
  ret = kEsfFwMgrResultOk;

unlock_mutex_then_exit:
  if (ret == kEsfFwMgrResultAborted) {
    ESF_FW_MGR_DLOG_WARNING(
        "Firmware Manager state transit to the error state.\n");
    ESF_FW_MGR_ELOG_WARNING(kEsfFwMgrElogWarningId0x0cCore);
    s_state = kEsfFwMgrStateError;
  }

  pthread_mutex_unlock(&s_main_apis_mutex);

  return ret;
}

// Erase -----------------------------------------------------------------------
static EsfFwMgrResult InvokeSubmoduleErase(EsfFwMgrContext* context) {
  if (context == NULL) {
    ESF_FW_MGR_DLOG_ERROR("context is NULL.\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x56Core);
    return kEsfFwMgrResultInvalidArgument;
  }

  if (context->submodule_ops == NULL || context->submodule_ops->erase == NULL) {
    ESF_FW_MGR_DLOG_ERROR("Erase of the submodule is not registered.\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x57Core);
    return kEsfFwMgrResultInternal;
  }
  return context->submodule_ops->erase(context->submodule_handle);
}

EsfFwMgrResult EsfFwMgrErase(EsfFwMgrHandle handle) {
  ESF_FW_MGR_DLOG_INFO("Called.\n");
  ESF_FW_MGR_ELOG_INFO(kEsfFwMgrElogInfoId0x04CoreErase);
  if (pthread_mutex_trylock(&s_main_apis_mutex) != 0) {
    ESF_FW_MGR_DLOG_ERROR("Failed to lock the mutex. errno = %d\n", errno);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x58Core);
    return kEsfFwMgrResultBusy;
  }

  EsfFwMgrResult ret = kEsfFwMgrResultInternal;

  if (s_state != kEsfFwMgrStateErasable) {
    ESF_FW_MGR_DLOG_ERROR("Invalid state. (state = %u)\n", s_state);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x59Core);
    ret = kEsfFwMgrResultFailedPrecondition;
    goto unlock_mutex_then_exit;
  }

  if (!ESF_FW_MGR_VERIFY_HANDLE(handle)) {
    ESF_FW_MGR_DLOG_ERROR("Invalid handle.\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x5aCore);
    ret = kEsfFwMgrResultInvalidArgument;
    goto unlock_mutex_then_exit;
  }

  EsfFwMgrContext* context = (EsfFwMgrContext*)handle;

  ret = InvokeSubmoduleErase(context);
  if (ret != kEsfFwMgrResultOk) {
    ESF_FW_MGR_DLOG_ERROR("InvokeSubmoduleErase failed. ret = %u\n", ret);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x5bCore);
    goto unlock_mutex_then_exit;
  }

  ret = InvokeSubmodulePostProcess(context);
  if (ret != kEsfFwMgrResultOk) {
    ESF_FW_MGR_DLOG_ERROR("InvokeSubmodulePostProcess failed. ret = %u\n", ret);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x5cCore);
    ret = kEsfFwMgrResultAborted;
    goto unlock_mutex_then_exit;
  }

  s_state = kEsfFwMgrStateDone;
  ret = kEsfFwMgrResultOk;

unlock_mutex_then_exit:
  if (ret == kEsfFwMgrResultAborted) {
    ESF_FW_MGR_DLOG_WARNING(
        "Firmware Manager state transit to the error state.\n");
    ESF_FW_MGR_ELOG_WARNING(kEsfFwMgrElogWarningId0x0dCore);
    s_state = kEsfFwMgrStateError;
  }

  pthread_mutex_unlock(&s_main_apis_mutex);

  return ret;
}

// Get binary header info ------------------------------------------------------
static EsfFwMgrResult InvokeSubmoduleGetBinaryHeaderInfo(
    EsfFwMgrContext* context, EsfFwMgrBinaryHeaderInfo* info) {
  if (context == NULL) {
    ESF_FW_MGR_DLOG_ERROR("context is NULL.\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x70Core);
    return kEsfFwMgrResultInvalidArgument;
  }

  if (context->submodule_ops == NULL) {
    ESF_FW_MGR_DLOG_ERROR("The submodule ops are not registered.\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x71Core);
    return kEsfFwMgrResultInternal;
  }

  if (context->submodule_ops->get_binary_header_info == NULL) {
    ESF_FW_MGR_DLOG_ERROR("GetBinaryHeaderInfo is not supported.\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x72Core);
    return kEsfFwMgrResultUnimplemented;
  }
  return context->submodule_ops->get_binary_header_info(
      context->submodule_handle, info);
}

EsfFwMgrResult EsfFwMgrGetBinaryHeaderInfo(EsfFwMgrHandle handle,
                                           EsfFwMgrBinaryHeaderInfo* info) {
  ESF_FW_MGR_DLOG_INFO("Called.\n");

  if (pthread_mutex_trylock(&s_main_apis_mutex) != 0) {
    ESF_FW_MGR_DLOG_ERROR("Failed to lock the mutex. errno = %d\n", errno);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x73Core);
    return kEsfFwMgrResultBusy;
  }
  EsfFwMgrResult ret = kEsfFwMgrResultInternal;

  if (!ESF_FW_MGR_VERIFY_HANDLE(handle)) {
    ESF_FW_MGR_DLOG_ERROR("Invalid handle.\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x74Core);
    ret = kEsfFwMgrResultInvalidArgument;
    goto unlock_mutex_then_exit;
  }

  EsfFwMgrContext* context = (EsfFwMgrContext*)handle;

  ret = InvokeSubmoduleGetBinaryHeaderInfo(context, info);
  if (ret != kEsfFwMgrResultOk) {
    ESF_FW_MGR_DLOG_ERROR(
        "InvokeSubmoduleGetBinaryHeaderInfo failed. ret = %u\n", ret);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x75Core);
    goto unlock_mutex_then_exit;
  }

  ret = kEsfFwMgrResultOk;

unlock_mutex_then_exit:
  pthread_mutex_unlock(&s_main_apis_mutex);

  return ret;
}

// Get Info --------------------------------------------------------------------
EsfFwMgrResult EsfFwMgrGetInfo(EsfFwMgrGetInfoData* data) {
  ESF_FW_MGR_DLOG_INFO("Called.\n");
  ESF_FW_MGR_ELOG_INFO(kEsfFwMgrElogInfoId0x10CoreGetInfo);
  if (pthread_mutex_lock(&s_sub_apis_mutex) != 0) {
    ESF_FW_MGR_DLOG_ERROR("Failed to lock the mutex. errno = %d\n", errno);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x5dCore);
    return kEsfFwMgrResultInternal;
  }

  EsfFwMgrResult ret = kEsfFwMgrResultInternal;

  if (s_state == kEsfFwMgrStateUninit) {
    ESF_FW_MGR_DLOG_ERROR("Invalid state. (state = %u)\n", s_state);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x5eCore);
    ret = kEsfFwMgrResultFailedPrecondition;
    goto unlock_mutex_then_exit;
  }

  if (data == NULL) {
    ESF_FW_MGR_DLOG_ERROR("data is NULL.\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x5fCore);
    ret = kEsfFwMgrResultInvalidArgument;
    goto unlock_mutex_then_exit;
  }

  EsfFwMgrSubmoduleOps* ops;
  ret = ChooseSubmoduleOps(data->target, &ops);
  if (ret != kEsfFwMgrResultOk) {
    ESF_FW_MGR_DLOG_ERROR("ChooseSubmoduleOps failed. ret = %u\n", ret);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x60Core);
    goto unlock_mutex_then_exit;
  }

  // Invoke submodule GetInfo
  if (ops == NULL || ops->get_info == NULL) {
    ESF_FW_MGR_DLOG_ERROR("GetInfo of the submodule is not registered.\n");
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x61Core);
    ret = kEsfFwMgrResultInternal;
    goto unlock_mutex_then_exit;
  }
  ret = ops->get_info(data);
  if (ret != kEsfFwMgrResultOk) {
    ESF_FW_MGR_DLOG_ERROR("GetInfo of the submodule failed. ret = %u\n", ret);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x62Core);
    goto unlock_mutex_then_exit;
  }

unlock_mutex_then_exit:
  pthread_mutex_unlock(&s_sub_apis_mutex);

  return ret;
}

// Factory Reset ---------------------------------------------------------------
EsfFwMgrResult EsfFwMgrStartFactoryReset(EsfFwMgrFactoryResetCause cause) {
#ifdef CONFIG_EXTERNAL_TARGET_RPI
  (void)cause;
  return kEsfFwMgrResultUnimplemented;
#else
  ESF_FW_MGR_DLOG_INFO("Called.\n");
  ESF_FW_MGR_ELOG_INFO(kEsfFwMgrElogInfoId0x11CoreFactoryReset);
  if (pthread_mutex_trylock(&s_main_apis_mutex) != 0) {
    ESF_FW_MGR_DLOG_ERROR("Failed to lock the mutex. errno = %d\n", errno);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x63Core);
    return kEsfFwMgrResultBusy;
  }

  EsfFwMgrResult ret = kEsfFwMgrResultInternal;

  if (s_state != kEsfFwMgrStateIdle) {
    ESF_FW_MGR_DLOG_ERROR("Invalid state. (state = %u)\n", s_state);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x64Core);
    ret = kEsfFwMgrResultFailedPrecondition;
    goto unlock_mutex_then_exit;
  }

  ret = EsfFwMgrStartFactoryResetInternal(cause);
  if (ret != kEsfFwMgrResultOk) {
    ESF_FW_MGR_DLOG_ERROR(
        "EsfFwMgrStartFactoryResetInternal failed. ret = %u\n", ret);
    ESF_FW_MGR_ELOG_ERROR(kEsfFwMgrElogErrorId0x65Core);
    goto unlock_mutex_then_exit;
  }

unlock_mutex_then_exit:
  pthread_mutex_unlock(&s_main_apis_mutex);

  return ret;
#endif
}

EsfFwMgrResult EsfFwMgrSetFactoryResetFlag(bool factory_reset_flag) {
  (void)factory_reset_flag;
  return kEsfFwMgrResultOk;
}

EsfFwMgrResult EsfFwMgrGetFactoryResetFlag(bool* factory_reset_flag) {
  if (factory_reset_flag != NULL) *factory_reset_flag = true;
  return kEsfFwMgrResultOk;
}

EsfFwMgrResult EsfFwMgrSwitchProcessorFirmwareSlot(void) {
#ifdef CONFIG_EXTERNAL_TARGET_RPI
  return kEsfFwMgrResultUnimplemented;
#else
  PlErrCode pl_ret = FwMgrPlSwitchFirmwarePartition();
  if (pl_ret != kPlErrCodeOk) return kEsfFwMgrResultUnavailable;

  return kEsfFwMgrResultOk;
#endif
}
