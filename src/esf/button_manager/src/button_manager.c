/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "button_manager.h"

#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>

#ifndef CONFIG_EXTERNAL_BUTTON_MANAGER_DISABLE
#include "button_manager_internal.h"
#endif

/****************************************************************************
 * private Data
 ****************************************************************************/

#define BUTTON_MANAGER_UNUSED_ARGUMENTS (0)

#ifndef CONFIG_EXTERNAL_BUTTON_MANAGER_DISABLE
// This variable is a mutex object and is used for exclusive control of the
// ButtonManager API.
static pthread_mutex_t static_button_manager_mutex_ext =
    PTHREAD_MUTEX_INITIALIZER;
#endif

/****************************************************************************
 * Functions
 ****************************************************************************/

EsfButtonManagerStatus EsfButtonManagerOpen(EsfButtonManagerHandle *handle) {
#ifndef CONFIG_EXTERNAL_BUTTON_MANAGER_DISABLE
  if (pthread_mutex_lock(&static_button_manager_mutex_ext) != 0) {
    ESF_BUTTON_MANAGER_ERROR("Mutex lock error.");
    return kEsfButtonManagerStatusInternalError;
  }
  // Parameter check.
  if (handle == (EsfButtonManagerHandle *)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Invalid parameter. Handle is null.");
    pthread_mutex_unlock(&static_button_manager_mutex_ext);
    return kEsfButtonManagerStatusParamError;
  }

  EsfButtonManagerStatus result = kEsfButtonManagerStatusInternalError;
  EsfButtonManagerState state = EsfButtonManagerGetState();
  if (state == kEsfButtonManagerStateClose) {
    result = EsfButtonManagerInitialize();
    if (result != kEsfButtonManagerStatusOk) {
      ESF_BUTTON_MANAGER_ERROR("Failed to initialize.");
      EsfButtonManagerFinalize();
      pthread_mutex_unlock(&static_button_manager_mutex_ext);
      return result;
    }
  }

  result = EsfButtonManagerCreateHandle(handle);
  if (result != kEsfButtonManagerStatusOk) {
    ESF_BUTTON_MANAGER_ERROR("Failed to create handle");
    if (state == kEsfButtonManagerStateClose) {
      EsfButtonManagerFinalize();
    }
    pthread_mutex_unlock(&static_button_manager_mutex_ext);
    return result;
  }

  EsfButtonManagerSetState(kEsfButtonManagerStateOpen);

  if (pthread_mutex_unlock(&static_button_manager_mutex_ext) != 0) {
    ESF_BUTTON_MANAGER_ERROR("Mutex unlock error.");
    return kEsfButtonManagerStatusInternalError;
  }
#endif  // CONFIG_EXTERNAL_BUTTON_MANAGER_DISABLE
  return kEsfButtonManagerStatusOk;
}

EsfButtonManagerStatus EsfButtonManagerClose(EsfButtonManagerHandle handle) {
#ifndef CONFIG_EXTERNAL_BUTTON_MANAGER_DISABLE
  if (pthread_mutex_lock(&static_button_manager_mutex_ext) != 0) {
    ESF_BUTTON_MANAGER_ERROR("Mutex lock error.");
    return kEsfButtonManagerStatusInternalError;
  }
  // Parameter check.
  if (handle == (EsfButtonManagerHandle)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Invalid parameter. Handle is null.");
    pthread_mutex_unlock(&static_button_manager_mutex_ext);
    return kEsfButtonManagerStatusParamError;
  }

  // State check.
  if (EsfButtonManagerGetState() == kEsfButtonManagerStateClose) {
    ESF_BUTTON_MANAGER_ERROR("Transition error. ButtonManager is not opened.");
    pthread_mutex_unlock(&static_button_manager_mutex_ext);
    return kEsfButtonManagerStatusStateTransitionError;
  }

  EsfButtonManagerStatus result = EsfButtonManagerDestroyHandle(handle);
  if (result != kEsfButtonManagerStatusOk) {
    ESF_BUTTON_MANAGER_ERROR("Failed to destroy handle.");
    pthread_mutex_unlock(&static_button_manager_mutex_ext);
    return result;
  }

  // If all handles destroyed, close the button manager.
  if (EsfButtonManagerGetNumberOfHandles() == 0) {
    result = EsfButtonManagerFinalize();
    EsfButtonManagerSetState(kEsfButtonManagerStateClose);
    if (result != kEsfButtonManagerStatusOk) {
      ESF_BUTTON_MANAGER_ERROR("Failed to Finalize.");
      pthread_mutex_unlock(&static_button_manager_mutex_ext);
      return result;
    }
  }
  if (pthread_mutex_unlock(&static_button_manager_mutex_ext) != 0) {
    ESF_BUTTON_MANAGER_ERROR("Mutex unlock error.");
    return kEsfButtonManagerStatusInternalError;
  }
#endif  // CONFIG_EXTERNAL_BUTTON_MANAGER_DISABLE
  return kEsfButtonManagerStatusOk;
}

EsfButtonManagerStatus EsfButtonManagerRegisterPressedCallback(
    uint32_t button_id, const EsfButtonManagerCallback callback,
    void *user_data, EsfButtonManagerHandle handle) {
#ifndef CONFIG_EXTERNAL_BUTTON_MANAGER_DISABLE
  if (pthread_mutex_lock(&static_button_manager_mutex_ext) != 0) {
    ESF_BUTTON_MANAGER_ERROR("Mutex lock error.");
    return kEsfButtonManagerStatusInternalError;
  }
  // Parameter check.
  // clang-format off
  if ((callback == (EsfButtonManagerCallback)NULL) || (handle == (EsfButtonManagerHandle)NULL)) {
    // clang-format on
    ESF_BUTTON_MANAGER_ERROR(
        "Invalid parameter. Callback is null or handle is null. callback=%p "
        "handle=%p",
        callback, handle);
    pthread_mutex_unlock(&static_button_manager_mutex_ext);
    return kEsfButtonManagerStatusParamError;
  }

  // State check.
  if (EsfButtonManagerGetState() == kEsfButtonManagerStateClose) {
    ESF_BUTTON_MANAGER_ERROR("Transition error. ButtonManager is not opened.");
    pthread_mutex_unlock(&static_button_manager_mutex_ext);
    return kEsfButtonManagerStatusStateTransitionError;
  }

  EsfButtonManagerStatus result = EsfButtonManagerRegisterNotificationCallback(
      button_id, BUTTON_MANAGER_UNUSED_ARGUMENTS,
      BUTTON_MANAGER_UNUSED_ARGUMENTS, callback, user_data,
      kEsfButtonManagerButtonStateTypePressed, handle);
  if (result != kEsfButtonManagerStatusOk) {
    ESF_BUTTON_MANAGER_ERROR("Failed to register notification callback.");
    pthread_mutex_unlock(&static_button_manager_mutex_ext);
    return result;
  }

  if (pthread_mutex_unlock(&static_button_manager_mutex_ext) != 0) {
    ESF_BUTTON_MANAGER_ERROR("Mutex unlock error.");
    return kEsfButtonManagerStatusInternalError;
  }
#endif  // CONFIG_EXTERNAL_BUTTON_MANAGER_DISABLE
  return kEsfButtonManagerStatusOk;
}

EsfButtonManagerStatus EsfButtonManagerRegisterReleasedCallback(
    uint32_t button_id, int32_t min_second, int32_t max_second,
    const EsfButtonManagerCallback callback, void *user_data,
    EsfButtonManagerHandle handle) {
#ifndef CONFIG_EXTERNAL_BUTTON_MANAGER_DISABLE
  if (pthread_mutex_lock(&static_button_manager_mutex_ext) != 0) {
    ESF_BUTTON_MANAGER_ERROR("Mutex lock error.");
    return kEsfButtonManagerStatusInternalError;
  }
  // Parameter check.
  // clang-format off
  if ((callback == (EsfButtonManagerCallback)NULL) || (handle == (EsfButtonManagerHandle)NULL)) {
    // clang-format on
    ESF_BUTTON_MANAGER_ERROR(
        "Invalid parameter. Callback is null or handle is null. callback=%p "
        "handle=%p",
        callback, handle);
    pthread_mutex_unlock(&static_button_manager_mutex_ext);
    return kEsfButtonManagerStatusParamError;
  }

  // State check.
  if (EsfButtonManagerGetState() == kEsfButtonManagerStateClose) {
    ESF_BUTTON_MANAGER_ERROR("Transition error. ButtonManager is not opened.");
    pthread_mutex_unlock(&static_button_manager_mutex_ext);
    return kEsfButtonManagerStatusStateTransitionError;
  }

  EsfButtonManagerStatus result = EsfButtonManagerRegisterNotificationCallback(
      button_id, min_second, max_second, callback, user_data,
      kEsfButtonManagerButtonStateTypeReleased, handle);
  if (result != kEsfButtonManagerStatusOk) {
    ESF_BUTTON_MANAGER_ERROR("Failed to register notification callback.");
    pthread_mutex_unlock(&static_button_manager_mutex_ext);
    return result;
  }

  if (pthread_mutex_unlock(&static_button_manager_mutex_ext) != 0) {
    ESF_BUTTON_MANAGER_ERROR("Mutex unlock error.");
    return kEsfButtonManagerStatusInternalError;
  }
#endif  // CONFIG_EXTERNAL_BUTTON_MANAGER_DISABLE
  return kEsfButtonManagerStatusOk;
}

EsfButtonManagerStatus EsfButtonManagerRegisterLongPressedCallback(
    uint32_t button_id, int32_t second, const EsfButtonManagerCallback callback,
    void *user_data, EsfButtonManagerHandle handle) {
#ifndef CONFIG_EXTERNAL_BUTTON_MANAGER_DISABLE
  // Parameter check.
  if (pthread_mutex_lock(&static_button_manager_mutex_ext) != 0) {
    ESF_BUTTON_MANAGER_ERROR("Mutex lock error.");
    return kEsfButtonManagerStatusInternalError;
  }
  // clang-format off
  if ((callback == (EsfButtonManagerCallback)NULL) || (handle == (EsfButtonManagerHandle)NULL)) {
    // clang-format on
    ESF_BUTTON_MANAGER_ERROR(
        "Invalid parameter. Callback is null or handle is null. callback=%p "
        "handle=%p",
        callback, handle);
    pthread_mutex_unlock(&static_button_manager_mutex_ext);
    return kEsfButtonManagerStatusParamError;
  }

  // State check.
  if (EsfButtonManagerGetState() == kEsfButtonManagerStateClose) {
    ESF_BUTTON_MANAGER_ERROR("Transition error. ButtonManager is not opened.");
    pthread_mutex_unlock(&static_button_manager_mutex_ext);
    return kEsfButtonManagerStatusStateTransitionError;
  }

  EsfButtonManagerStatus result = EsfButtonManagerRegisterNotificationCallback(
      button_id, second, BUTTON_MANAGER_UNUSED_ARGUMENTS, callback, user_data,
      kEsfButtonManagerButtonStateTypeLongPressed, handle);
  if (result != kEsfButtonManagerStatusOk) {
    ESF_BUTTON_MANAGER_ERROR("Failed to register notification callback.");
    pthread_mutex_unlock(&static_button_manager_mutex_ext);
    return result;
  }

  if (pthread_mutex_unlock(&static_button_manager_mutex_ext) != 0) {
    ESF_BUTTON_MANAGER_ERROR("Mutex unlock error.");
    return kEsfButtonManagerStatusInternalError;
  }
#endif  // CONFIG_EXTERNAL_BUTTON_MANAGER_DISABLE
  return kEsfButtonManagerStatusOk;
}

EsfButtonManagerStatus EsfButtonManagerUnregisterPressedCallback(
    uint32_t button_id, EsfButtonManagerHandle handle) {
#ifndef CONFIG_EXTERNAL_BUTTON_MANAGER_DISABLE
  if (pthread_mutex_lock(&static_button_manager_mutex_ext) != 0) {
    ESF_BUTTON_MANAGER_ERROR("Mutex lock error.");
    return kEsfButtonManagerStatusInternalError;
  }
  // Parameter check.
  if (handle == (EsfButtonManagerHandle)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Invalid parameter. Handle is null.");
    pthread_mutex_unlock(&static_button_manager_mutex_ext);
    return kEsfButtonManagerStatusParamError;
  }

  // State check.
  if (EsfButtonManagerGetState() == kEsfButtonManagerStateClose) {
    ESF_BUTTON_MANAGER_ERROR("Transition error. ButtonManager is not opened.");
    pthread_mutex_unlock(&static_button_manager_mutex_ext);
    return kEsfButtonManagerStatusStateTransitionError;
  }

  // clang-format off
  EsfButtonManagerStatus result = EsfButtonManagerUnregisterNotificationCallback(button_id, kEsfButtonManagerButtonStateTypePressed, handle);
  // clang-format on
  if (result != kEsfButtonManagerStatusOk) {
    ESF_BUTTON_MANAGER_ERROR("Failed to unregister notification callback.");
    pthread_mutex_unlock(&static_button_manager_mutex_ext);
    return result;
  }

  if (pthread_mutex_unlock(&static_button_manager_mutex_ext) != 0) {
    ESF_BUTTON_MANAGER_ERROR("Mutex unlock error.");
    return kEsfButtonManagerStatusInternalError;
  }
#endif  // CONFIG_EXTERNAL_BUTTON_MANAGER_DISABLE
  return kEsfButtonManagerStatusOk;
}

EsfButtonManagerStatus EsfButtonManagerUnregisterReleasedCallback(
    uint32_t button_id, EsfButtonManagerHandle handle) {
#ifndef CONFIG_EXTERNAL_BUTTON_MANAGER_DISABLE
  if (pthread_mutex_lock(&static_button_manager_mutex_ext) != 0) {
    ESF_BUTTON_MANAGER_ERROR("Mutex lock error.");
    return kEsfButtonManagerStatusInternalError;
  }
  // Parameter check.
  if (handle == (EsfButtonManagerHandle)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Invalid parameter. Handle is null.");
    pthread_mutex_unlock(&static_button_manager_mutex_ext);
    return kEsfButtonManagerStatusParamError;
  }

  // State check.
  if (EsfButtonManagerGetState() == kEsfButtonManagerStateClose) {
    ESF_BUTTON_MANAGER_ERROR("Transition error. ButtonManager is not opened.");
    pthread_mutex_unlock(&static_button_manager_mutex_ext);
    return kEsfButtonManagerStatusStateTransitionError;
  }

  // clang-format off
  EsfButtonManagerStatus result = EsfButtonManagerUnregisterNotificationCallback(button_id, kEsfButtonManagerButtonStateTypeReleased, handle);
  // clang-format on
  if (result != kEsfButtonManagerStatusOk) {
    ESF_BUTTON_MANAGER_ERROR("Failed to unregister notification callback.");
    pthread_mutex_unlock(&static_button_manager_mutex_ext);
    return result;
  }

  if (pthread_mutex_unlock(&static_button_manager_mutex_ext) != 0) {
    ESF_BUTTON_MANAGER_ERROR("Mutex unlock error.");
    return kEsfButtonManagerStatusInternalError;
  }
#endif  // CONFIG_EXTERNAL_BUTTON_MANAGER_DISABLE
  return kEsfButtonManagerStatusOk;
}

EsfButtonManagerStatus EsfButtonManagerUnregisterLongPressedCallback(
    uint32_t button_id, EsfButtonManagerHandle handle) {
#ifndef CONFIG_EXTERNAL_BUTTON_MANAGER_DISABLE
  if (pthread_mutex_lock(&static_button_manager_mutex_ext) != 0) {
    ESF_BUTTON_MANAGER_ERROR("Mutex lock error.");
    return kEsfButtonManagerStatusInternalError;
  }
  // Parameter check.
  if (handle == (EsfButtonManagerHandle)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Invalid parameter. Handle is null.");
    pthread_mutex_unlock(&static_button_manager_mutex_ext);
    return kEsfButtonManagerStatusParamError;
  }

  // State check.
  if (EsfButtonManagerGetState() == kEsfButtonManagerStateClose) {
    ESF_BUTTON_MANAGER_ERROR("Transition error. ButtonManager is not opened.");
    pthread_mutex_unlock(&static_button_manager_mutex_ext);
    return kEsfButtonManagerStatusStateTransitionError;
  }

  // clang-format off
  EsfButtonManagerStatus result = EsfButtonManagerUnregisterNotificationCallback(button_id, kEsfButtonManagerButtonStateTypeLongPressed, handle);
  // clang-format on
  if (result != kEsfButtonManagerStatusOk) {
    ESF_BUTTON_MANAGER_ERROR("Failed to unregister notification callback.");
    pthread_mutex_unlock(&static_button_manager_mutex_ext);
    return result;
  }

  if (pthread_mutex_unlock(&static_button_manager_mutex_ext) != 0) {
    ESF_BUTTON_MANAGER_ERROR("Mutex unlock error.");
    return kEsfButtonManagerStatusInternalError;
  }
#endif  // CONFIG_EXTERNAL_BUTTON_MANAGER_DISABLE
  return kEsfButtonManagerStatusOk;
}

EsfButtonManagerStatus EsfButtonManagerEnableNotificationCallback(
    EsfButtonManagerHandle handle) {
#ifndef CONFIG_EXTERNAL_BUTTON_MANAGER_DISABLE
  if (pthread_mutex_lock(&static_button_manager_mutex_ext) != 0) {
    ESF_BUTTON_MANAGER_ERROR("Mutex lock error.");
    return kEsfButtonManagerStatusInternalError;
  }
  // Parameter check.
  if (handle == (EsfButtonManagerHandle)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Invalid parameter. Handle is null.");
    pthread_mutex_unlock(&static_button_manager_mutex_ext);
    return kEsfButtonManagerStatusParamError;
  }

  // State check.
  if (EsfButtonManagerGetState() == kEsfButtonManagerStateClose) {
    ESF_BUTTON_MANAGER_ERROR("Transition error. ButtonManager is not opened.");
    pthread_mutex_unlock(&static_button_manager_mutex_ext);
    return kEsfButtonManagerStatusStateTransitionError;
  }

  EsfButtonManagerStatus result = EsfButtonManagerSetCallbackExecutableState(
      kEsfButtonManagerNotificationCallbackStateEnable, handle);
  if (result != kEsfButtonManagerStatusOk) {
    ESF_BUTTON_MANAGER_ERROR("Failed to enable notification callback.");
    pthread_mutex_unlock(&static_button_manager_mutex_ext);
    return result;
  }

  if (pthread_mutex_unlock(&static_button_manager_mutex_ext) != 0) {
    ESF_BUTTON_MANAGER_ERROR("Mutex unlock error.");
    return kEsfButtonManagerStatusInternalError;
  }
#endif  // CONFIG_EXTERNAL_BUTTON_MANAGER_DISABLE
  return kEsfButtonManagerStatusOk;
}

EsfButtonManagerStatus EsfButtonManagerDisableNotificationCallback(
    EsfButtonManagerHandle handle) {
#ifndef CONFIG_EXTERNAL_BUTTON_MANAGER_DISABLE
  if (pthread_mutex_lock(&static_button_manager_mutex_ext) != 0) {
    ESF_BUTTON_MANAGER_ERROR("Mutex lock error.");
    return kEsfButtonManagerStatusInternalError;
  }
  // Parameter check.
  if (handle == (EsfButtonManagerHandle)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Invalid parameter. Handle is null.");
    pthread_mutex_unlock(&static_button_manager_mutex_ext);
    return kEsfButtonManagerStatusParamError;
  }

  // State check.
  if (EsfButtonManagerGetState() == kEsfButtonManagerStateClose) {
    ESF_BUTTON_MANAGER_ERROR("Transition error. ButtonManager is not opened.");
    pthread_mutex_unlock(&static_button_manager_mutex_ext);
    return kEsfButtonManagerStatusStateTransitionError;
  }

  EsfButtonManagerStatus result = EsfButtonManagerSetCallbackExecutableState(
      kEsfButtonManagerNotificationCallbackStateDisable, handle);
  if (result != kEsfButtonManagerStatusOk) {
    ESF_BUTTON_MANAGER_ERROR("Failed to disable notification callback.");
    pthread_mutex_unlock(&static_button_manager_mutex_ext);
    return result;
  }

  if (pthread_mutex_unlock(&static_button_manager_mutex_ext) != 0) {
    ESF_BUTTON_MANAGER_ERROR("Mutex unlock error.");
    return kEsfButtonManagerStatusInternalError;
  }
#endif  // CONFIG_EXTERNAL_BUTTON_MANAGER_DISABLE
  return kEsfButtonManagerStatusOk;
}
