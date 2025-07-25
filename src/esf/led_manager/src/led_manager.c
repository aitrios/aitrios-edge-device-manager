/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#include "led_manager/include/led_manager.h"

#include <inttypes.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "led_manager/src/internal/led_manager_internal.h"
#ifndef CONFIG_EXTERNAL_LED_MANAGER_DISABLE
#include "pl.h"
#include "pl_led.h"

// Enumerated type of the state of Led Manager.
typedef enum EsfLedManagerStatus {
  kEsfLedManagerStatusUninit,  // Uninitialized state.
  kEsfLedManagerStatusIdle,    // Waiting for processing.
} EsfLedManagerStatus;
#endif  /* CONFIG_EXTERNAL_LED_MANAGER_DISABLE */

#ifndef CONFIG_EXTERNAL_LED_MANAGER_DISABLE
// Led Manager Status.
static EsfLedManagerStatus led_manager_status = kEsfLedManagerStatusUninit;
#endif  /* CONFIG_EXTERNAL_LED_MANAGER_DISABLE */

EsfLedManagerResult EsfLedManagerInit(void) {
#ifndef CONFIG_EXTERNAL_LED_MANAGER_DISABLE
  ESF_LED_MANAGER_TRACE("entry.");
  EsfLedManagerResult ret = EsfLedManagerMutexLock();
  if (ret != kEsfLedManagerSuccess) {
    ESF_LED_MANAGER_ERR("EsfLedManagerMutexLock func failed ret = %u.", ret);
    return ret;
  }
  EsfLedManagerResult result = kEsfLedManagerSuccess;
  do {
    if (led_manager_status != kEsfLedManagerStatusUninit) {
      ESF_LED_MANAGER_DEBUG("Led Manager State already Idle.");
      result = kEsfLedManagerSuccess;
      break;
    }

    result = EsfLedManagerInitResource();
    if (result != kEsfLedManagerSuccess) {
      ESF_LED_MANAGER_ERR("EsfLedManagerInitResource func failed ret = %u.",
                          result);
      break;
    }

    PlErrCode led_ret = PlLedInitialize();
    if (led_ret != kPlErrCodeOk) {
      ESF_LED_MANAGER_ERR("PlLedInitialize func failed ret = %u.", led_ret);
      result = kEsfLedManagerLedOperateError;
      break;
    }
    led_manager_status = kEsfLedManagerStatusIdle;
  } while (0);
  ret = EsfLedManagerMutexUnlock();
  if (ret != kEsfLedManagerSuccess) {
    ESF_LED_MANAGER_ERR("EsfLedManagerMutexUnlock func failed ret = %u.", ret);
    return ret;
  }
  if (result != kEsfLedManagerSuccess) {
    ESF_LED_MANAGER_TRACE("exit.");
    return result;
  }
  ESF_LED_MANAGER_TRACE("exit.");
#else
  ESF_LED_MANAGER_DEBUG("Always returns kEsfLedManagerSuccess.");
#endif  /* CONFIG_EXTERNAL_LED_MANAGER_DISABLE */
  return kEsfLedManagerSuccess;
}

EsfLedManagerResult EsfLedManagerDeinit(void) {
#ifndef CONFIG_EXTERNAL_LED_MANAGER_DISABLE
  ESF_LED_MANAGER_TRACE("entry.");
  EsfLedManagerResult ret = EsfLedManagerMutexLock();
  if (ret != kEsfLedManagerSuccess) {
    ESF_LED_MANAGER_ERR("EsfLedManagerMutexLock func failed ret = %u.", ret);
    return ret;
  }
  EsfLedManagerResult result = kEsfLedManagerSuccess;
  do {
    if (led_manager_status != kEsfLedManagerStatusIdle) {
      ESF_LED_MANAGER_DEBUG("Led Manager State already Uninit.");
      result = kEsfLedManagerSuccess;
      break;
    }
    result = EsfLedManagerLedStop();
    if (result != kEsfLedManagerSuccess) {
      ESF_LED_MANAGER_ERR("EsfLedManagerPlFinalize func failed ret = %u.",
                          result);
      break;
    }
    PlErrCode pl_ret = PlLedFinalize();
    if (pl_ret != kPlErrCodeOk) {
      ESF_LED_MANAGER_ERR("PlLedFinalize func failed ret = %u.", pl_ret);
      result = kEsfLedManagerLedOperateError;
      break;
    }
    EsfLedManagerDeinitResource();
    led_manager_status = kEsfLedManagerStatusUninit;
  } while (0);
  ret = EsfLedManagerMutexUnlock();
  if (ret != kEsfLedManagerSuccess) {
    ESF_LED_MANAGER_ERR("EsfLedManagerMutexUnlock func failed ret = %u.", ret);
    return ret;
  }
  if (result != kEsfLedManagerSuccess) {
    return result;
  }

  ESF_LED_MANAGER_TRACE("exit.");
#else
  ESF_LED_MANAGER_DEBUG("Always returns kEsfLedManagerSuccess.");
#endif  /* CONFIG_EXTERNAL_LED_MANAGER_DISABLE */
  return kEsfLedManagerSuccess;
}

EsfLedManagerResult EsfLedManagerSetStatus(
    const EsfLedManagerLedStatusInfo* status) {
#ifndef CONFIG_EXTERNAL_LED_MANAGER_DISABLE
  ESF_LED_MANAGER_TRACE("entry.");
  if (status == NULL) {
    ESF_LED_MANAGER_ERR("Parameter error. (status = %p)", status);
    return kEsfLedManagerInvalidArgument;
  }
  if (status->led < 0 || status->led >= kEsfLedManagerTargetLedNum) {
    ESF_LED_MANAGER_ERR("The specified LED ID is invalid. (LED ID = %u)",
                        status->led);
    ESF_LED_MANAGER_ELOG_ERR(ESF_LED_MANAGER_ELOG_INVALID_PARAM);
    return kEsfLedManagerInvalidArgument;
  }
  if (status->status < 0 || kEsfLedManagerLedStatusNum <= status->status) {
    ESF_LED_MANAGER_ERR("The specified state is invalid. (status = %u)",
                        status->status);
    ESF_LED_MANAGER_ELOG_ERR(ESF_LED_MANAGER_ELOG_INVALID_PARAM);
    return kEsfLedManagerStatusNotFound;
  }
  EsfLedManagerResult ret = EsfLedManagerMutexLock();
  if (ret != kEsfLedManagerSuccess) {
    ESF_LED_MANAGER_ERR("EsfLedManagerMutexLock func failed ret = %u.", ret);
    return ret;
  }
  EsfLedManagerResult result = kEsfLedManagerSuccess;
  do {
    if (led_manager_status != kEsfLedManagerStatusIdle) {
      ESF_LED_MANAGER_ERR("State transition error.");
      ESF_LED_MANAGER_ELOG_ERR(ESF_LED_MANAGER_ELOG_INVALID_PARAM);
      result = kEsfLedManagerStateTransitionError;
      break;
    }

    bool bool_ret = EsfLedManagerCheckLedStatus(status);
    ESF_LED_MANAGER_DEBUG("Set Info Led ID = %u Status = %u Enable = %d",
                          status->led, status->status, status->enabled);
    if (bool_ret != true) {
      ESF_LED_MANAGER_DEBUG("The specified state has already been set.");
      result = kEsfLedManagerSuccess;
      break;
    }
    result = EsfLedManagerSetStatusInternal(status);
    if (result != kEsfLedManagerSuccess) {
      ESF_LED_MANAGER_ERR(
          "EsfLedManagerSetStatusInternal func failed ret = %u.", result);
      break;
    }
  } while (0);
  ret = EsfLedManagerMutexUnlock();
  if (ret != kEsfLedManagerSuccess) {
    ESF_LED_MANAGER_ERR("EsfLedManagerMutexUnlock func failed ret = %u.", ret);
    return ret;
  }
  if (result != kEsfLedManagerSuccess) {
    return result;
  }

  ESF_LED_MANAGER_TRACE("exit.");
#else
  ESF_LED_MANAGER_DEBUG("Always returns kEsfLedManagerSuccess.");
#endif  /* CONFIG_EXTERNAL_LED_MANAGER_DISABLE */
  return kEsfLedManagerSuccess;
}

EsfLedManagerResult EsfLedManagerGetStatus(EsfLedManagerLedStatusInfo* status) {
  ESF_LED_MANAGER_TRACE("entry.");
  if (status == NULL) {
    ESF_LED_MANAGER_ERR("Parameter error. (status = %p)", status);
    return kEsfLedManagerInvalidArgument;
  }
  if (status->led < 0 || status->led >= kEsfLedManagerTargetLedNum) {
    ESF_LED_MANAGER_ERR("The specified LED ID is invalid. (LED ID = %u)",
                        status->led);
    return kEsfLedManagerInvalidArgument;
  }
  if (status->status < 0 || kEsfLedManagerLedStatusNum <= status->status) {
    ESF_LED_MANAGER_ERR("The specified state is invalid. (status = %u)",
                        status->status);
    return kEsfLedManagerStatusNotFound;
  }

#ifndef CONFIG_EXTERNAL_LED_MANAGER_DISABLE
  EsfLedManagerResult ret = EsfLedManagerMutexLock();
  if (ret != kEsfLedManagerSuccess) {
    ESF_LED_MANAGER_ERR("EsfLedManagerMutexLock func failed ret = %u.", ret);
    return ret;
  }
  EsfLedManagerResult result = kEsfLedManagerSuccess;
  do {
    if (led_manager_status != kEsfLedManagerStatusIdle) {
      ESF_LED_MANAGER_ERR("State transition error.");
      ESF_LED_MANAGER_ELOG_ERR(ESF_LED_MANAGER_ELOG_INVALID_PARAM);
      result = kEsfLedManagerStateTransitionError;
      break;
    }

    result = EsfLedManagerGetLedStatus(status);
    if (result != kEsfLedManagerSuccess) {
      ESF_LED_MANAGER_ERR("EsfLedManagerGetSystemStatus func failed ret = %u.",
                          result);
      break;
    }
  } while (0);
  ret = EsfLedManagerMutexUnlock();
  if (ret != kEsfLedManagerSuccess) {
    ESF_LED_MANAGER_ERR("EsfLedManagerMutexUnlock func failed ret = %u.", ret);
    return ret;
  }
  if (result != kEsfLedManagerSuccess) {
    return result;
  }

#else
  status->enabled = false;
  ESF_LED_MANAGER_DEBUG("Always returns kEsfLedManagerSuccess.");
#endif  /* CONFIG_EXTERNAL_LED_MANAGER_DISABLE */
  ESF_LED_MANAGER_TRACE("exit.");
  return kEsfLedManagerSuccess;
}

EsfLedManagerResult EsfLedManagerSetLightingPersistence(
    EsfLedManagerTargetLed led, bool is_enable) {
#ifndef CONFIG_EXTERNAL_LED_MANAGER_DISABLE
  ESF_LED_MANAGER_TRACE("entry.");
  if ((led < 0) || (led >= kEsfLedManagerTargetLedNum)) {
    ESF_LED_MANAGER_ERR("The specified LED ID is invalid. (LED ID = %d)", led);
    return kEsfLedManagerInvalidArgument;
  }
  EsfLedManagerResult ret = EsfLedManagerMutexLock();
  if (ret != kEsfLedManagerSuccess) {
    ESF_LED_MANAGER_ERR("EsfLedManagerMutexLock func failed ret = %u.", ret);
    return ret;
  }
  EsfLedManagerResult result = kEsfLedManagerSuccess;
  do {
    if (led_manager_status != kEsfLedManagerStatusIdle) {
      ESF_LED_MANAGER_ERR("State transition error.");
      ESF_LED_MANAGER_ELOG_ERR(ESF_LED_MANAGER_ELOG_INVALID_PARAM);
      result = kEsfLedManagerStateTransitionError;
      break;
    }

    result = EsfLedManagerSetLightingPersistenceInternal(led, is_enable);
    if (result != kEsfLedManagerSuccess) {
      ESF_LED_MANAGER_ERR(
          "EsfLedManagerSetLightingPersistenceInternal func failed ret = %u.",
          result);
      break;
    }
  } while (0);
  ret = EsfLedManagerMutexUnlock();
  if (ret != kEsfLedManagerSuccess) {
    ESF_LED_MANAGER_ERR("EsfLedManagerMutexUnlock func failed ret = %u.", ret);
    return ret;
  }
  if (result != kEsfLedManagerSuccess) {
    return result;
  }
  ESF_LED_MANAGER_TRACE("exit.");
#else
  ESF_LED_MANAGER_DEBUG("Always returns kEsfLedManagerSuccess.");
#endif  /* CONFIG_EXTERNAL_LED_MANAGER_DISABLE */
  return kEsfLedManagerSuccess;
}
