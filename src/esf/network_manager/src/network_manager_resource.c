/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// self header.
#include "network_manager/network_manager_resource.h"

// system header
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// other header
#include "network_manager/network_manager_log.h"
#include "porting_layer/include/pl.h"
#include "porting_layer/include/pl_network.h"

#ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_LOCKTIME
#define CONFIG_EXTERNAL_NETWORK_MANAGER_LOCKTIME (1000)
#endif  // CONFIG_EXTERNAL_NETWORK_MANAGER_LOCKTIME

// A structure that defines network_manager handle management.
typedef struct EsfNetworkManagerHandleManager {
  // An array of internal handle information.
  EsfNetworkManagerHandleInternal handle[ESF_NETWORK_MANAGER_HANDLE_MAX];

  // A two-dimensional array of valid handles.
  // Maintains the number of valid handles for each connection mode and handle
  // type.
  int32_t valid_handle_count[kEsfNetworkManagerModeNum]
                            [kEsfNetworkManagerHandleTypeNum];
} EsfNetworkManagerHandleManager;

// A structure that defines the internal resources of the Network.
typedef struct EsfNetworkManagerResource {
  // Mutex for resource access.
  pthread_mutex_t resource_lock;

  // Mutex for handle access.
  pthread_mutex_t handle_lock;

  // Network initialization state.
  EsfNetworkManagerStatus status;

  // Handle management information.
  EsfNetworkManagerHandleManager *handle_manager;

  // Array holding mode information.
  EsfNetworkManagerModeInfo *mode_info[kEsfNetworkManagerModeNum];

  // Network connection information mask work area.
  EsfNetworkManagerParameterMask *mask_work;

  // Network connection information work area.
  EsfNetworkManagerParameter *parameter_work;

  // Network connection information internal work area.
  EsfNetworkManagerParameterInternal *parameter_internal_work;
} EsfNetworkManagerResource;

// Global variables to hold resources.
static EsfNetworkManagerResource resource = {
    PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER,
    kEsfNetworkManagerStatusUninit,
    NULL,
    {NULL, NULL},
    NULL,
    NULL,
    NULL,
};

// Maximum number of handles.
static const int32_t
    kEsfNetworkManagerHandleNumMax[kEsfNetworkManagerHandleTypeNum] = {
        ESF_NETWORK_MANAGER_HANDLE_TYPE_CONTROL_MAX,
        ESF_NETWORK_MANAGER_HANDLE_TYPE_INFORMATION_MAX,
};

// """Free dynamic memory.

// Free the specified pointer if it is not NULL.

// Args:
//     target (void *): Pointer to be released.
// Note:
// """
static void EsfNetworkManagerResourceFree(void *target) {
  ESF_NETWORK_MANAGER_TRACE("START");
  if (target != NULL) {
    ESF_NETWORK_MANAGER_TRACE("free %p", target);
    free(target);
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return;
}

// """Free all dynamic memory in resource submodules.

// Free all dynamic memory in resource submodules.
// Note:
// """
static void EsfNetworkManagerResourceFreeAllMemory(void) {
  ESF_NETWORK_MANAGER_TRACE("START");
  EsfNetworkManagerResourceFree(resource.parameter_internal_work);
  resource.parameter_internal_work = NULL;
  EsfNetworkManagerResourceFree(resource.parameter_work);
  resource.parameter_work = NULL;
  EsfNetworkManagerResourceFree(resource.mask_work);
  resource.mask_work = NULL;
  int i = 0;
  for (; i < kEsfNetworkManagerModeNum; ++i) {
    EsfNetworkManagerResourceFree(resource.mode_info[i]);
    resource.mode_info[i] = NULL;
  }
  EsfNetworkManagerResourceFree(resource.handle_manager);
  resource.handle_manager = NULL;
  ESF_NETWORK_MANAGER_TRACE("END");
  return;
}

#ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_DISABLE
// """Set HAL system information to resource information.

// Set HAL system information to resource information.

// Args:
//     info_total_num (int32_t): Specify the number of HAL system information.
//     infos (PlNetworkSystemInfo *):
//       Specifies the HAL system information array.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultInternalError: Internal error.

// Note:
// """
static EsfNetworkManagerResult EsfNetworkManagerResourceSetHalSystemInfo(
    int32_t info_total_num, PlNetworkSystemInfo *infos) {
  ESF_NETWORK_MANAGER_TRACE("START info_total_num=%d infos=%p", info_total_num,
                            infos);
  bool cloud_if_available = false;
  int i = 0;
  for (; i < info_total_num; ++i) {
    if (infos[i].cloud_enable == true) {
      cloud_if_available = true;
      if (infos[i].type == kPlNetworkTypeEther) {
        if (resource.mode_info[kEsfNetworkManagerModeNormal]
                ->hal_system_info[kEsfNetworkManagerInterfaceKindEther] ==
            NULL) {
          ESF_NETWORK_MANAGER_DBG("Normal mode ether interface is %s.",
                                  infos[i].if_name);
          resource.mode_info[kEsfNetworkManagerModeNormal]
              ->hal_system_info[kEsfNetworkManagerInterfaceKindEther] =
              &infos[i];
        }
      } else {
        if (resource.mode_info[kEsfNetworkManagerModeNormal]
                ->hal_system_info[kEsfNetworkManagerInterfaceKindWifi] ==
            NULL) {
          ESF_NETWORK_MANAGER_DBG("Normal mode wifi interface is %s.",
                                  infos[i].if_name);
          resource.mode_info[kEsfNetworkManagerModeNormal]
              ->hal_system_info[kEsfNetworkManagerInterfaceKindWifi] =
              &infos[i];
        }
      }
    }
    if (infos[i].local_enable == true) {
      if (infos[i].type == kPlNetworkTypeWifi) {
        if (resource.mode_info[kEsfNetworkManagerModeAccessPoint]
                ->hal_system_info[kEsfNetworkManagerInterfaceKindWifi] ==
            NULL) {
          ESF_NETWORK_MANAGER_DBG("Access point mode wifi interface is %s.",
                                  infos[i].if_name);
          resource.mode_info[kEsfNetworkManagerModeAccessPoint]
              ->hal_system_info[kEsfNetworkManagerInterfaceKindWifi] =
              &infos[i];
        }
      }
    }
  }
  if (cloud_if_available == false) {
    ESF_NETWORK_MANAGER_ERR("HAL config invalid. Cloud interface not found.");
    return kEsfNetworkManagerResultInternalError;
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return kEsfNetworkManagerResultSuccess;
}
#endif  // CONFIG_EXTERNAL_NETWORK_MANAGER_DISABLE

// """Perform MutexLock with timeout specification.

// Perform MutexLock with timeout specification.

// Args:
//     lock_mutex (pthread_mutex_t *): Mutex to be locked.
//     timeout_msec (int32_t): Timeout time.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultInternalError: Internal error.

// Note:
// """
static EsfNetworkManagerResult EsfNetworkManagerResourceTimedLock(
    pthread_mutex_t *lock_mutex, int32_t timeout_msec) {
  ESF_NETWORK_MANAGER_TRACE("START lock_mutex=%p timeout_msec=%d", lock_mutex,
                            timeout_msec);
  struct timespec timeout = {
      0,
      0,
  };
  int ret = clock_gettime(CLOCK_REALTIME, &timeout);
  if (ret != 0) {
    ESF_NETWORK_MANAGER_ERR("clock_gettime error.");
    return kEsfNetworkManagerResultInternalError;
  }
  int32_t timeout_sec = timeout_msec / 1000;
  static const int64_t kSecToNanoSec = 1 * 1000 * 1000 * 1000;
  timeout.tv_sec += timeout_sec;
  timeout.tv_nsec += (timeout_msec - timeout_sec * 1000) * 1000 * 1000;
  if (timeout.tv_nsec >= kSecToNanoSec) {
    ++timeout.tv_sec;
    timeout.tv_nsec -= kSecToNanoSec;
  }
  ret = pthread_mutex_timedlock(lock_mutex, &timeout);
  if (ret != 0) {
    ESF_NETWORK_MANAGER_ERR("pthread_mutex_timedlock error=%d", ret);
    return kEsfNetworkManagerResultInternalError;
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return kEsfNetworkManagerResultSuccess;
}

EsfNetworkManagerResult EsfNetworkManagerResourceInit(
    int32_t info_total_num, PlNetworkSystemInfo *infos) {
#ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_DISABLE
  int i = 0;

  ESF_NETWORK_MANAGER_TRACE("START info_total_num=%d infos=%p", info_total_num,
                            infos);
  if (info_total_num <= 0 || infos == NULL) {
    ESF_NETWORK_MANAGER_ERR("Parameter error.(%d %p)", info_total_num, infos);
    // PL data failure
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_PL_FAILURE);
    return kEsfNetworkManagerResultInternalError;
  }
  EsfNetworkManagerResult ret = EsfNetworkManagerResourceLockResource();
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Resource lock error.");
    return ret;
  }
  EsfNetworkManagerResult return_result = kEsfNetworkManagerResultSuccess;
  do {
    if (resource.status == kEsfNetworkManagerStatusInit) {
      ESF_NETWORK_MANAGER_WARN("Resource already initialized.");
      ESF_NETWORK_MANAGER_ELOG_WARN(
          ESF_NETWORK_MANAGER_ELOG_INTERNAL_ERROR_WARN);
      break;
    }
    resource.handle_manager = (EsfNetworkManagerHandleManager *)calloc(
        1, sizeof(EsfNetworkManagerHandleManager));
    if (resource.handle_manager == NULL) {
      ESF_NETWORK_MANAGER_ERR("Memory allocate error.");
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INTERNAL_ERROR);
      return_result = kEsfNetworkManagerResultResourceExhausted;
      break;
    }
    for (i = 0; i < kEsfNetworkManagerModeNum; ++i) {
      resource.mode_info[i] = (EsfNetworkManagerModeInfo *)calloc(
          1, sizeof(EsfNetworkManagerModeInfo));
      if (resource.mode_info[i] == NULL) {
        ESF_NETWORK_MANAGER_ERR("Memory allocate error.");
        ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INTERNAL_ERROR);
        return_result = kEsfNetworkManagerResultResourceExhausted;
        break;
      }
      resource.mode_info[i]->notify_info =
          kEsfNetworkManagerNotifyInfoDisconnected;
    }
    if (return_result != kEsfNetworkManagerResultSuccess) {
      break;
    }
    resource.mask_work = (EsfNetworkManagerParameterMask *)calloc(
        1, sizeof(EsfNetworkManagerParameterMask));
    if (resource.mask_work == NULL) {
      ESF_NETWORK_MANAGER_ERR("Memory allocate error.");
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INTERNAL_ERROR);
      return_result = kEsfNetworkManagerResultResourceExhausted;
      break;
    }
    resource.parameter_work = (EsfNetworkManagerParameter *)calloc(
        1, sizeof(EsfNetworkManagerParameter));
    if (resource.parameter_work == NULL) {
      ESF_NETWORK_MANAGER_ERR("Memory allocate error.");
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INTERNAL_ERROR);
      return_result = kEsfNetworkManagerResultResourceExhausted;
      break;
    }
    resource.parameter_internal_work =
        (EsfNetworkManagerParameterInternal *)calloc(
            1, sizeof(EsfNetworkManagerParameterInternal));
    if (resource.parameter_internal_work == NULL) {
      ESF_NETWORK_MANAGER_ERR("Memory allocate error.");
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INTERNAL_ERROR);
      return_result = kEsfNetworkManagerResultResourceExhausted;
      break;
    }
    ret = EsfNetworkManagerResourceSetHalSystemInfo(info_total_num, infos);
    if (ret != kEsfNetworkManagerResultSuccess) {
      ESF_NETWORK_MANAGER_ERR("HAL SystemInfo failure.");
      // PL data failure
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_PL_FAILURE);
      return_result = ret;
    }
  } while (0);
  if (return_result != kEsfNetworkManagerResultSuccess) {
    ret = EsfNetworkManagerResourceUnlockResource();
    if (ret != kEsfNetworkManagerResultSuccess) {
      ESF_NETWORK_MANAGER_ERR("Resource unlock error.");
    }
    EsfNetworkManagerResourceFreeAllMemory();
  } else {
    resource.status = kEsfNetworkManagerStatusInit;
    ret = EsfNetworkManagerResourceUnlockResource();
    if (ret != kEsfNetworkManagerResultSuccess) {
      ESF_NETWORK_MANAGER_ERR("Resource unlock error.");
      return_result = kEsfNetworkManagerResultInternalError;
      resource.status = kEsfNetworkManagerStatusUninit;
      EsfNetworkManagerResourceFreeAllMemory();
    }
  }
#else
  ESF_NETWORK_MANAGER_TRACE("START");
  EsfNetworkManagerResult return_result = kEsfNetworkManagerResultSuccess;
  do {
    if (resource.status == kEsfNetworkManagerStatusInit) {
      ESF_NETWORK_MANAGER_WARN("Resource already initialized.");
      ESF_NETWORK_MANAGER_ELOG_WARN(
          ESF_NETWORK_MANAGER_ELOG_INTERNAL_ERROR_WARN);
      break;
    }
    resource.handle_manager = (EsfNetworkManagerHandleManager *)calloc(
        1, sizeof(EsfNetworkManagerHandleManager));
    if (resource.handle_manager == NULL) {
      ESF_NETWORK_MANAGER_ERR("Memory allocate error.");
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INTERNAL_ERROR);
      return_result = kEsfNetworkManagerResultResourceExhausted;
      break;
    }
    for (int i = 0; i < kEsfNetworkManagerModeNum; ++i) {
      resource.mode_info[i] = (EsfNetworkManagerModeInfo *)calloc(
          1, sizeof(EsfNetworkManagerModeInfo));
      if (resource.mode_info[i] == NULL) {
        ESF_NETWORK_MANAGER_ERR("Memory allocate error.");
        ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INTERNAL_ERROR);
        return_result = kEsfNetworkManagerResultResourceExhausted;
        break;
      }
      resource.mode_info[i]->notify_info =
          kEsfNetworkManagerNotifyInfoDisconnected;
    }
    if (return_result != kEsfNetworkManagerResultSuccess) {
      break;
    }
    resource.mask_work = (EsfNetworkManagerParameterMask *)calloc(
        1, sizeof(EsfNetworkManagerParameterMask));
    if (resource.mask_work == NULL) {
      ESF_NETWORK_MANAGER_ERR("Memory allocate error.");
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INTERNAL_ERROR);
      return_result = kEsfNetworkManagerResultResourceExhausted;
      break;
    }
    resource.parameter_work = (EsfNetworkManagerParameter *)calloc(
        1, sizeof(EsfNetworkManagerParameter));
    if (resource.parameter_work == NULL) {
      ESF_NETWORK_MANAGER_ERR("Memory allocate error.");
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INTERNAL_ERROR);
      return_result = kEsfNetworkManagerResultResourceExhausted;
      break;
    }
    resource.parameter_internal_work =
        (EsfNetworkManagerParameterInternal *)calloc(
            1, sizeof(EsfNetworkManagerParameterInternal));
    if (resource.parameter_internal_work == NULL) {
      ESF_NETWORK_MANAGER_ERR("Memory allocate error.");
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INTERNAL_ERROR);
      return_result = kEsfNetworkManagerResultResourceExhausted;
      break;
    }
  } while (0);

  if (return_result != kEsfNetworkManagerResultSuccess) {
    EsfNetworkManagerResourceFreeAllMemory();
  } else {
    resource.status = kEsfNetworkManagerStatusInit;
  }
#endif  // CONFIG_EXTERNAL_NETWORK_MANAGER_DISABLE

  ESF_NETWORK_MANAGER_TRACE("END");
  return return_result;
}

EsfNetworkManagerResult EsfNetworkManagerResourceDeinit(void) {
  ESF_NETWORK_MANAGER_TRACE("START");
  EsfNetworkManagerResult ret = EsfNetworkManagerResourceLockResource();
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Resource lock error.");
    return ret;
  }
  EsfNetworkManagerResult return_result = kEsfNetworkManagerResultSuccess;
  do {
    if (resource.status == kEsfNetworkManagerStatusUninit) {
      ESF_NETWORK_MANAGER_WARN("Resource already uninitialized.");
      ESF_NETWORK_MANAGER_ELOG_WARN(
          ESF_NETWORK_MANAGER_ELOG_INTERNAL_ERROR_WARN);
      return_result = kEsfNetworkManagerResultStatusUnexecutable;
      break;
    }
    EsfNetworkManagerResourceFreeAllMemory();
    resource.status = kEsfNetworkManagerStatusUninit;
  } while (0);

  ret = EsfNetworkManagerResourceUnlockResource();
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Resource unlock error.");
    return_result = kEsfNetworkManagerResultInternalError;
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return return_result;
}

EsfNetworkManagerResult EsfNetworkManagerResourceLockResource(void) {
  ESF_NETWORK_MANAGER_TRACE("START");
  EsfNetworkManagerResult ret = EsfNetworkManagerResourceTimedLock(
      &resource.resource_lock, CONFIG_EXTERNAL_NETWORK_MANAGER_LOCKTIME);
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Resource lock error.");
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INTERNAL_ERROR);
  }

  ESF_NETWORK_MANAGER_TRACE("END");
  return ret;
}

EsfNetworkManagerResult EsfNetworkManagerResourceLockResourceNoTimeout(void) {
  ESF_NETWORK_MANAGER_TRACE("START");
  int ret = pthread_mutex_lock(&resource.resource_lock);
  if (ret != 0) {
    ESF_NETWORK_MANAGER_ERR("pthread_mutex_lock error=%d", ret);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INTERNAL_ERROR);
    return kEsfNetworkManagerResultInternalError;
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return kEsfNetworkManagerResultSuccess;
}

EsfNetworkManagerResult EsfNetworkManagerResourceUnlockResource(void) {
  ESF_NETWORK_MANAGER_TRACE("START");
  int ret = pthread_mutex_unlock(&resource.resource_lock);
  if (ret != 0) {
    ESF_NETWORK_MANAGER_ERR("pthread_mutex_unlock error.");
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INTERNAL_ERROR);
    return kEsfNetworkManagerResultInternalError;
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return kEsfNetworkManagerResultSuccess;
}

EsfNetworkManagerResult EsfNetworkManagerResourceNewHandle(
    EsfNetworkManagerMode mode, EsfNetworkManagerHandleType handle_type,
    EsfNetworkManagerHandle *handle) {
  ESF_NETWORK_MANAGER_TRACE("START mode=%d handle_type=%d handle=%p", mode,
                            handle_type, handle);
  if (handle == NULL || mode < 0 || mode >= kEsfNetworkManagerModeNum ||
      handle_type < 0 || handle_type >= kEsfNetworkManagerHandleTypeNum) {
    ESF_NETWORK_MANAGER_ERR(
        "parameter error.(mode=%d handle_type=%d handle=%p)", mode, handle_type,
        handle);
    return kEsfNetworkManagerResultInvalidParameter;
  }
  EsfNetworkManagerResult ret = EsfNetworkManagerResourceTimedLock(
      &resource.handle_lock, CONFIG_EXTERNAL_NETWORK_MANAGER_LOCKTIME);
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Handle lock error.");
    return ret;
  }
  EsfNetworkManagerResult return_result = kEsfNetworkManagerResultSuccess;
  int i = 0;
  do {
    if (resource.status == kEsfNetworkManagerStatusUninit) {
      ESF_NETWORK_MANAGER_ERR("Uninitialized.");
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
      return_result = kEsfNetworkManagerResultStatusUnexecutable;
      break;
    }
    if (resource.handle_manager->valid_handle_count[mode][handle_type] >=
        kEsfNetworkManagerHandleNumMax[handle_type]) {
      ESF_NETWORK_MANAGER_ERR("Maximum number of handles exceeded.");
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
      return_result = kEsfNetworkManagerResultResourceExhausted;
      break;
    }
    for (; i < ESF_NETWORK_MANAGER_HANDLE_MAX; ++i) {
      if (resource.handle_manager->handle[i].is_valid == false) {
        ++resource.handle_manager->valid_handle_count[mode][handle_type];
        resource.handle_manager->handle[i].is_valid = true;
        resource.handle_manager->handle[i].mode = mode;
        resource.handle_manager->handle[i].handle_type = handle_type;
        resource.handle_manager->handle[i].count = 0;
        resource.handle_manager->handle[i].is_in_use_control = false;
        resource.handle_manager->handle[i].mode_info = resource.mode_info[mode];
        *handle = i;
        break;
      }
    }
    // Not found free handle.
    if (i >= ESF_NETWORK_MANAGER_HANDLE_MAX) {
      ESF_NETWORK_MANAGER_ERR("Not found free handle.");
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
      return_result = kEsfNetworkManagerResultInternalError;
    }
  } while (0);
  int ret_int = pthread_mutex_unlock(&resource.handle_lock);
  if (ret_int != 0) {
    ESF_NETWORK_MANAGER_ERR("pthread_mutex_unlock error.");
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INTERNAL_ERROR);
    if (i > 0 && i < ESF_NETWORK_MANAGER_HANDLE_MAX) {
      --resource.handle_manager->valid_handle_count[mode][handle_type];
      resource.handle_manager->handle[i].is_valid = false;
    }
    return kEsfNetworkManagerResultInternalError;
  }

  ESF_NETWORK_MANAGER_TRACE("END");
  return return_result;
}

EsfNetworkManagerResult EsfNetworkManagerResourceDeleteHandle(
    EsfNetworkManagerHandle handle) {
  ESF_NETWORK_MANAGER_TRACE("START handle=%d", handle);
  if (handle < 0 || handle >= ESF_NETWORK_MANAGER_HANDLE_MAX) {
    ESF_NETWORK_MANAGER_ERR("parameter error.(handle=%d)", handle);
    return kEsfNetworkManagerResultInvalidParameter;
  }
  EsfNetworkManagerResult ret = EsfNetworkManagerResourceTimedLock(
      &resource.handle_lock, CONFIG_EXTERNAL_NETWORK_MANAGER_LOCKTIME);
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Handle lock error.");
    return ret;
  }
  EsfNetworkManagerResult return_result = kEsfNetworkManagerResultSuccess;
  do {
    if (resource.status == kEsfNetworkManagerStatusUninit) {
      ESF_NETWORK_MANAGER_ERR("Uninitialized.");
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
      return_result = kEsfNetworkManagerResultStatusUnexecutable;
      break;
    }
    if (resource.handle_manager->handle[handle].is_valid == false) {
      ESF_NETWORK_MANAGER_ERR("Specified handle is not reserved.");
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
      return_result = kEsfNetworkManagerResultNotFound;
      break;
    }
    if (resource.handle_manager->handle[handle].count > 0) {
      ESF_NETWORK_MANAGER_ERR("Specified handle in use.");
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
      return_result = kEsfNetworkManagerResultFailedPrecondition;
      break;
    }
    if (resource.handle_manager->handle[handle].handle_type ==
        kEsfNetworkManagerHandleTypeControl) {
      if (resource.handle_manager->handle[handle].mode_info->connect_status !=
              kEsfNetworkManagerConnectStatusDisconnected &&
          resource.handle_manager->handle[handle].mode_info->connect_status !=
              kEsfNetworkManagerConnectStatusDisconnecting) {
        ESF_NETWORK_MANAGER_ERR("The specified handle is connected.");
        ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
        return_result = kEsfNetworkManagerResultFailedPrecondition;
        break;
      }
    }
    EsfNetworkManagerMode mode = resource.handle_manager->handle[handle].mode;
    EsfNetworkManagerHandleType handle_type =
        resource.handle_manager->handle[handle].handle_type;
    --resource.handle_manager->valid_handle_count[mode][handle_type];
    resource.handle_manager->handle[handle].is_valid = false;
    resource.handle_manager->handle[handle].count = 0;
    resource.handle_manager->handle[handle].is_in_use_control = false;
    resource.handle_manager->handle[handle].mode_info = NULL;
  } while (0);
  int ret_int = pthread_mutex_unlock(&resource.handle_lock);
  if (ret_int != 0) {
    ESF_NETWORK_MANAGER_ERR("pthread_mutex_unlock error.");
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INTERNAL_ERROR);
    return kEsfNetworkManagerResultInternalError;
  }

  ESF_NETWORK_MANAGER_TRACE("END");
  return return_result;
}

EsfNetworkManagerResult EsfNetworkManagerResourceOpenHandle(
    bool is_control, EsfNetworkManagerHandle handle,
    EsfNetworkManagerHandleInternal **handle_internal) {
  ESF_NETWORK_MANAGER_TRACE("START is_control=%d handle=%d handle_internal=%p",
                            is_control, handle, handle_internal);
  if (handle < 0 || handle >= ESF_NETWORK_MANAGER_HANDLE_MAX ||
      handle_internal == NULL) {
    ESF_NETWORK_MANAGER_ERR("parameter error.(handle=%d handle_internal=%p)",
                            handle, handle_internal);
    return kEsfNetworkManagerResultInvalidParameter;
  }
  EsfNetworkManagerResult ret = EsfNetworkManagerResourceTimedLock(
      &resource.handle_lock, CONFIG_EXTERNAL_NETWORK_MANAGER_LOCKTIME);
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Handle lock error.");
    return ret;
  }
  EsfNetworkManagerResult return_result = kEsfNetworkManagerResultSuccess;
  do {
    if (resource.status == kEsfNetworkManagerStatusUninit) {
      ESF_NETWORK_MANAGER_ERR("Uninitialized.");
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
      return_result = kEsfNetworkManagerResultStatusUnexecutable;
      break;
    }
    if (resource.handle_manager->handle[handle].is_valid == false) {
      ESF_NETWORK_MANAGER_ERR("Specified handle is not reserved.");
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
      return_result = kEsfNetworkManagerResultNotFound;
      break;
    }
    if (is_control && resource.handle_manager->handle[handle].handle_type !=
                          kEsfNetworkManagerHandleTypeControl) {
      ESF_NETWORK_MANAGER_ERR("Specified handle is not control handle.");
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
      return_result = kEsfNetworkManagerResultInvalidHandleType;
      break;
    }
    if (is_control &&
        resource.handle_manager->handle[handle].is_in_use_control) {
      ESF_NETWORK_MANAGER_ERR(
          "Specified handle is in use for control operation.");
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
      return_result = kEsfNetworkManagerResultFailedPrecondition;
      break;
    }
    ++resource.handle_manager->handle[handle].count;
    if (is_control) {
      resource.handle_manager->handle[handle].is_in_use_control = true;
    }
    *handle_internal = &resource.handle_manager->handle[handle];
  } while (0);
  int ret_int = pthread_mutex_unlock(&resource.handle_lock);
  if (ret_int != 0) {
    ESF_NETWORK_MANAGER_ERR("pthread_mutex_unlock error.");
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INTERNAL_ERROR);
    return kEsfNetworkManagerResultInternalError;
  }

  ESF_NETWORK_MANAGER_TRACE("END");
  return return_result;
}

EsfNetworkManagerResult EsfNetworkManagerResourceCloseHandle(
    bool is_control, EsfNetworkManagerHandle handle) {
  ESF_NETWORK_MANAGER_TRACE("START is_control=%d handle=%d", is_control,
                            handle);
  if (handle < 0 || handle >= ESF_NETWORK_MANAGER_HANDLE_MAX) {
    ESF_NETWORK_MANAGER_ERR("parameter error.(handle=%d)", handle);
    return kEsfNetworkManagerResultInvalidParameter;
  }
  EsfNetworkManagerResult ret = EsfNetworkManagerResourceTimedLock(
      &resource.handle_lock, CONFIG_EXTERNAL_NETWORK_MANAGER_LOCKTIME);
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Handle lock error.");
    return ret;
  }
  EsfNetworkManagerResult return_result = kEsfNetworkManagerResultSuccess;
  do {
    if (resource.status == kEsfNetworkManagerStatusUninit) {
      ESF_NETWORK_MANAGER_ERR("Uninitialized.");
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
      return_result = kEsfNetworkManagerResultStatusUnexecutable;
      break;
    }
    if (resource.handle_manager->handle[handle].is_valid == false) {
      ESF_NETWORK_MANAGER_ERR("Specified handle is not reserved.");
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
      return_result = kEsfNetworkManagerResultNotFound;
      break;
    }
    if (resource.handle_manager->handle[handle].count <= 0) {
      ESF_NETWORK_MANAGER_ERR("Handle reference counter is already 0.");
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
      return_result = kEsfNetworkManagerResultInternalError;
      break;
    }
    --resource.handle_manager->handle[handle].count;
    if (is_control) {
      resource.handle_manager->handle[handle].is_in_use_control = false;
    }
  } while (0);
  int ret_int = pthread_mutex_unlock(&resource.handle_lock);
  if (ret_int != 0) {
    ESF_NETWORK_MANAGER_ERR("pthread_mutex_unlock error.");
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INTERNAL_ERROR);
    return kEsfNetworkManagerResultInternalError;
  }

  ESF_NETWORK_MANAGER_TRACE("END");
  return return_result;
}

bool EsfNetworkManagerResourceCheckStatus(EsfNetworkManagerStatus status) {
  ESF_NETWORK_MANAGER_TRACE("START status=%d", status);
  bool return_value = (status == resource.status);
  ESF_NETWORK_MANAGER_TRACE("END");
  return return_value;
}

EsfNetworkManagerResult EsfNetworkManagerResourceGetIfname(
    char **if_name_ether, char **if_name_wifist, char **if_name_wifiap) {
  ESF_NETWORK_MANAGER_TRACE(
      "START if_name_ether=%p if_name_wifist=%p if_name_wifiap=%p",
      if_name_ether, if_name_wifist, if_name_wifiap);
  if (if_name_ether == NULL || if_name_wifist == NULL ||
      if_name_wifiap == NULL) {
    ESF_NETWORK_MANAGER_ERR(
        "parameter error.(if_name_ether=%p if_name_wifist=%p "
        "if_name_wifiap=%p)",
        if_name_ether, if_name_wifist, if_name_wifiap);
    return kEsfNetworkManagerResultInternalError;
  }
  if (resource.status == kEsfNetworkManagerStatusUninit) {
    ESF_NETWORK_MANAGER_ERR("Uninitialized.");
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
    return kEsfNetworkManagerResultStatusUnexecutable;
  }

  if (resource.mode_info[kEsfNetworkManagerModeNormal]
              ->hal_system_info[kEsfNetworkManagerInterfaceKindEther] != NULL &&
      resource.mode_info[kEsfNetworkManagerModeNormal]
              ->hal_system_info[kEsfNetworkManagerInterfaceKindEther]
              ->if_name[0] != '\0') {
    *if_name_ether = resource.mode_info[kEsfNetworkManagerModeNormal]
                         ->hal_system_info[kEsfNetworkManagerInterfaceKindEther]
                         ->if_name;
    ESF_NETWORK_MANAGER_DBG("if_name_ether=%s", *if_name_ether);
  } else {
    *if_name_ether = NULL;
  }
  if (resource.mode_info[kEsfNetworkManagerModeNormal]
              ->hal_system_info[kEsfNetworkManagerInterfaceKindWifi] != NULL &&
      resource.mode_info[kEsfNetworkManagerModeNormal]
              ->hal_system_info[kEsfNetworkManagerInterfaceKindWifi]
              ->if_name[0] != '\0') {
    *if_name_wifist = resource.mode_info[kEsfNetworkManagerModeNormal]
                          ->hal_system_info[kEsfNetworkManagerInterfaceKindWifi]
                          ->if_name;
    ESF_NETWORK_MANAGER_DBG("if_name_wifist=%s", *if_name_wifist);
  } else {
    *if_name_wifist = NULL;
  }
  if (resource.mode_info[kEsfNetworkManagerModeAccessPoint]
              ->hal_system_info[kEsfNetworkManagerInterfaceKindWifi] != NULL &&
      resource.mode_info[kEsfNetworkManagerModeAccessPoint]
              ->hal_system_info[kEsfNetworkManagerInterfaceKindWifi]
              ->if_name[0] != '\0') {
    *if_name_wifiap = resource.mode_info[kEsfNetworkManagerModeAccessPoint]
                          ->hal_system_info[kEsfNetworkManagerInterfaceKindWifi]
                          ->if_name;
    ESF_NETWORK_MANAGER_DBG("if_name_wifiap=%s", *if_name_wifiap);
  } else {
    *if_name_wifiap = NULL;
  }

  ESF_NETWORK_MANAGER_TRACE("END");
  return kEsfNetworkManagerResultSuccess;
}

EsfNetworkManagerResult EsfNetworkManagerResourceGetParameterWork(
    EsfNetworkManagerParameterMask **mask_work,
    EsfNetworkManagerParameter **parameter_work) {
  ESF_NETWORK_MANAGER_TRACE("START mask_work=%p parameter_work=%p", mask_work,
                            parameter_work);
  if (mask_work == NULL || parameter_work == NULL) {
    ESF_NETWORK_MANAGER_ERR("parameter error.(mask_work=%p parameter_work=%p)",
                            mask_work, parameter_work);
    return kEsfNetworkManagerResultInternalError;
  }
  if (resource.status == kEsfNetworkManagerStatusUninit) {
    ESF_NETWORK_MANAGER_ERR("Uninitialized.");
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
    return kEsfNetworkManagerResultStatusUnexecutable;
  }
  if (resource.mask_work == NULL || resource.parameter_work == NULL) {
    ESF_NETWORK_MANAGER_ERR(
        "Internal resource error.(mask_work=%p parameter_work=%p)",
        resource.mask_work, resource.parameter_work);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
    return kEsfNetworkManagerResultInternalError;
  }

  *mask_work = resource.mask_work;
  *parameter_work = resource.parameter_work;
  ESF_NETWORK_MANAGER_TRACE("END");
  return kEsfNetworkManagerResultSuccess;
}

EsfNetworkManagerResult EsfNetworkManagerResourceGetParameterInternalWork(
    EsfNetworkManagerParameterInternal **parameter_internal_work) {
  ESF_NETWORK_MANAGER_TRACE("START parameter_internal_work=%p",
                            parameter_internal_work);
  if (parameter_internal_work == NULL) {
    ESF_NETWORK_MANAGER_ERR("parameter error.(parameter_internal_work=%p)",
                            parameter_internal_work);
    return kEsfNetworkManagerResultInternalError;
  }
  if (resource.status == kEsfNetworkManagerStatusUninit) {
    ESF_NETWORK_MANAGER_ERR("Uninitialized.");
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
    return kEsfNetworkManagerResultStatusUnexecutable;
  }
  if (resource.parameter_internal_work == NULL) {
    ESF_NETWORK_MANAGER_ERR(
        "Internal resource error.(parameter_internal_work=%p)",
        resource.parameter_internal_work);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
    return kEsfNetworkManagerResultInternalError;
  }

  *parameter_internal_work = resource.parameter_internal_work;
  ESF_NETWORK_MANAGER_TRACE("END");
  return kEsfNetworkManagerResultSuccess;
}
