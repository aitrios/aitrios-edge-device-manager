/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#include "led_manager/src/internal/led_manager_internal.h"

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

#include "led_manager/include/led_manager.h"
#include "pl.h"
#include "pl_led.h"

#ifndef CONFIG_EXTERNAL_LED_MANAGER_TIMEOUT
// Default timeout value.
#define CONFIG_EXTERNAL_LED_MANAGER_TIMEOUT 1000
#endif  // CONFIG_EXTERNAL_LED_MANAGER_TIMEOUT

// A structure that stores the state of Led.
typedef struct EsfLedManagerLedStatusStorage {
  bool status[kEsfLedManagerTargetLedNum][kEsfLedManagerLedStatusNum];
} EsfLedManagerLedStatusStorage;

#include "led_manager/config/config.h"

// """Updates the status of the specified LED.

// Args:
//    set_status (EsfLedManagerLedStatusInfo*): This structure sets the LED
//    status.
//      NULL is not acceptable.
//    led_state (EsfLedManagerLedStatusStorage*): This is where the LED status
//    is set.
//      NULL is not acceptable.

// Returns:
//    kEsfLedManagerSuccess: Normal termination.
//    kEsfLedManagerInternalError: Internal error.
static EsfLedManagerResult EsfLedManagerSetLedStatus(
    const EsfLedManagerLedStatusInfo* set_status,
    EsfLedManagerLedStatusStorage* led_state);

// """This function obtains and operates LED lighting information.

// Args:
//    status (EsfLedManagerLedStatusStorage*): The structure contains the LED
//    status.
//      NULL is not acceptable.

// Returns:
//    kEsfLedManagerSuccess: Normal termination.
//    kEsfLedManagerInternalError: Internal error.
//    kEsfLedManagerLedOperateError: LED operation failure.
static EsfLedManagerResult EsfLedManagerLedOperation(
    const EsfLedManagerLedStatusStorage* status);

// """Obtains the lighting contents of LEDs.

// Args:
//    led_state (EsfLedManagerLedStatusStorage*): The state of each LED to be
//    illuminated.
//      NULL is not acceptable.
//    led_info (EsfLedManagerLedInfo*): Lighting contents for each LED.
//      NULL is not acceptable.

// Returns:
//    kEsfLedManagerSuccess: Normal termination.
//    kEsfLedManagerInternalError: Internal error.
static EsfLedManagerResult EsfLedManagerGetLedLightingInfo(
    const EsfLedManagerLedStatusStorage* led_state,
    EsfLedManagerLedInfo* led_info);

// """Turns LEDs on/off.

//    led_info (EsfLedManagerLedInfo*): Lighting information for each LED.
//      NULL is not acceptable.

// Returns:
//    kEsfLedManagerSuccess: Normal termination.
//    kEsfLedManagerInternalError: Internal error.
//    kEsfLedManagerOutOfMemory: Memory allocation failure.
//    kEsfLedManagerLedOperateError: LED operation failure.
static EsfLedManagerResult EsfLedManagerLedLighting(
    const EsfLedManagerLedInfo* led_info);

// """Sets the specified time.

// Args:
//    time_msec (const int32_t): Time to set [ms].
//    time (struct timespec*): Output parameters.
//      NULL is not acceptable.

// Returns:
//    kEsfLedManagerSuccess: Normal termination.
//    kEsfLedManagerInternalError: Internal error.
static EsfLedManagerResult EsfLedManagerSetTime(const int32_t time_msec,
                                                struct timespec* time);

// Exclusive control identifier
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// The status of the LEDs used in setting and retrieving status.
static EsfLedManagerLedStatusStorage* led_status;

// An array that stores information on whether the target LED is currently lit.
static bool* led_enabled;

// Array to store LED lighting retention settings.
static bool* led_keep_enabled;

// Structure for initialization of EsfLedManagerLedInfo.
static const EsfLedManagerLedInfo kLedInfoDefault = {
    .color = 0,
    .interval = kEsfLedManagerLedIntervalNone,
};

static const PlLedSequence kLightingSeq[] = {{1, {0, 0}}};
static const PlLedSequence k1HzSeq[] = {
    {0, {0, 100000000}},
    {CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_LED_OFF, {0, 900000000}}};
static const PlLedSequence k4HzSeq[] = {
    {0, {0, 100000000}},
    {CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_LED_OFF, {0, 100000000}},
    {0, {0, 100000000}},
    {CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_LED_OFF, {0, 100000000}},
    {0, {0, 100000000}},
    {CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_LED_OFF, {0, 100000000}},
    {0, {0, 100000000}},
    {CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_LED_OFF, {0, 100000000}},
    {CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_LED_OFF, {0, 200000000}},
};
static const PlLedSequence kPatternSeq[] = {
    {0, {0, 100000000}},
    {CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_LED_OFF, {0, 900000000}},
    {0, {0, 100000000}},
    {CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_LED_OFF, {0, 100000000}},
    {0, {0, 100000000}},
    {CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_LED_OFF, {0, 100000000}},
    {0, {0, 100000000}},
    {CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_LED_OFF, {0, 100000000}},
    {0, {0, 100000000}},
    {CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_LED_OFF, {0, 100000000}},
    {CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_LED_OFF, {0, 200000000}},
};

static const PlLedSequence* kSeqTable[kEsfLedManagerLedIntervalNum] = {
    kLightingSeq, k1HzSeq, k4HzSeq, kPatternSeq};

static const int kSeqNumTable[kEsfLedManagerLedIntervalNum] = {
    sizeof(kLightingSeq) / sizeof(PlLedSequence),
    sizeof(k1HzSeq) / sizeof(PlLedSequence),
    sizeof(k4HzSeq) / sizeof(PlLedSequence),
    sizeof(kPatternSeq) / sizeof(PlLedSequence)};

// ID table for each LED.
LED_MANAGER_STATIC LED_MANAGER_CONSTANT uint32_t
    kLedIdTable[kEsfLedManagerTargetLedNum] = {
        (uint32_t)CONFIG_EXTERNAL_LED_MANAGER_POWER_LED_ID,
        (uint32_t)CONFIG_EXTERNAL_LED_MANAGER_WIFI_LED_ID,
        (uint32_t)CONFIG_EXTERNAL_LED_MANAGER_SERVICE_LED_ID};

EsfLedManagerResult EsfLedManagerInitResource(void) {
  ESF_LED_MANAGER_TRACE("entry.");
  EsfLedManagerLedStatusStorage* tmp_led_status =
      calloc(1, sizeof(*tmp_led_status));
  if (tmp_led_status == NULL) {
    ESF_LED_MANAGER_WARN("Failed to allocate memory for tmp_led_status.");
    ESF_LED_MANAGER_ELOG_ERR(ESF_LED_MANAGER_ELOG_INTERNAL_ERROR);
    return kEsfLedManagerOutOfMemory;
  }
  bool* tmp_led_enabled = calloc(kEsfLedManagerTargetLedNum,
                                 sizeof(*tmp_led_enabled));
  if (tmp_led_enabled == NULL) {
    ESF_LED_MANAGER_WARN("Failed to allocate memory for tmp_led_enabled.");
    ESF_LED_MANAGER_ELOG_ERR(ESF_LED_MANAGER_ELOG_INTERNAL_ERROR);
    free(tmp_led_status);
    return kEsfLedManagerOutOfMemory;
  }
  bool* tmp_led_keep_enabled = calloc(kEsfLedManagerTargetLedNum,
                                      sizeof(*tmp_led_keep_enabled));
  if (tmp_led_keep_enabled == NULL) {
    ESF_LED_MANAGER_WARN("Failed to allocate memory for tmp_led_keep_enabled.");
    ESF_LED_MANAGER_ELOG_ERR(ESF_LED_MANAGER_ELOG_INTERNAL_ERROR);
    free(tmp_led_status);
    free(tmp_led_enabled);
    return kEsfLedManagerOutOfMemory;
  }
  led_status = tmp_led_status;
  led_enabled = tmp_led_enabled;
  led_keep_enabled = tmp_led_keep_enabled;
  ESF_LED_MANAGER_TRACE("exit.");
  return kEsfLedManagerSuccess;
}

void EsfLedManagerDeinitResource(void) {
  ESF_LED_MANAGER_TRACE("entry.");
  free(led_status);
  led_status = NULL;
  free(led_enabled);
  led_enabled = NULL;
  free(led_keep_enabled);
  led_keep_enabled = NULL;
  ESF_LED_MANAGER_TRACE("exit.");
  return;
}

EsfLedManagerResult EsfLedManagerLedStop(void) {
  ESF_LED_MANAGER_TRACE("entry.");
  for (int i = 0; i < (int)kEsfLedManagerTargetLedNum; ++i) {
    if (led_enabled[i] == true) {
      PlErrCode ret = PlLedStopSeq(kLedIdTable[i]);
      if (ret != kPlErrCodeOk) {
        ESF_LED_MANAGER_WARN("PlLedStopSync func failed. ret = %u, LED ID = %u",
                             ret, kLedIdTable[i]);
        return kEsfLedManagerLedOperateError;
      }
      led_enabled[i] = false;
    } else {
      ESF_LED_MANAGER_DEBUG("LED ID = %u is already stopped.", kLedIdTable[i]);
    }
  }
  ESF_LED_MANAGER_TRACE("exit.");
  return kEsfLedManagerSuccess;
}

bool EsfLedManagerCheckLedStatus(const EsfLedManagerLedStatusInfo* status) {
  ESF_LED_MANAGER_TRACE("entry.");
  if ((status == NULL) || (status->led < 0) ||
      (kEsfLedManagerTargetLedNum <= status->led) || (status->status < 0) ||
      (kEsfLedManagerLedStatusNum <= status->status)) {
    ESF_LED_MANAGER_TRACE("exit.");
    return false;
  }
  bool ret = status->enabled != led_status->status[status->led][status->status];
  ESF_LED_MANAGER_TRACE("exit.");
  return ret;
}

EsfLedManagerResult EsfLedManagerSetStatusInternal(
    const EsfLedManagerLedStatusInfo* status) {
  ESF_LED_MANAGER_TRACE("entry.");
  if (status == NULL) {
    ESF_LED_MANAGER_WARN("Parameter error. (status = %p)", status);
    return kEsfLedManagerInternalError;
  }
  EsfLedManagerLedStatusStorage tmp_led_status;
  memcpy(&tmp_led_status, led_status, sizeof(tmp_led_status));
  EsfLedManagerResult ret = EsfLedManagerSetLedStatus(status, &tmp_led_status);
  if (ret != kEsfLedManagerSuccess) {
    ESF_LED_MANAGER_WARN("EsfLedManagerSetLedStatus func failed ret = %u.",
                         ret);
    return ret;
  }
  ret = EsfLedManagerLedOperation(&tmp_led_status);
  if (ret != kEsfLedManagerSuccess) {
    ESF_LED_MANAGER_ERR("EsfLedManagerLedOperation func failed ret = %u.", ret);
    return ret;
  }
  memcpy(led_status, &tmp_led_status, sizeof(*led_status));
  ESF_LED_MANAGER_TRACE("exit.");
  return kEsfLedManagerSuccess;
}

EsfLedManagerResult EsfLedManagerSetLightingPersistenceInternal(
    EsfLedManagerTargetLed led, bool is_enable) {
  ESF_LED_MANAGER_TRACE("entry.");
  if (led < 0 || led >= kEsfLedManagerTargetLedNum) {
    ESF_LED_MANAGER_ERR("The specified LED ID is invalid. (LED ID = %u)", led);
    return kEsfLedManagerInternalError;
  }
  if (led_keep_enabled[led] == is_enable) {
    ESF_LED_MANAGER_DEBUG("No update on led_keep_enabled[%u].", led);
    return kEsfLedManagerSuccess;
  }
  led_keep_enabled[led] = is_enable;
  if (is_enable == true) {
    // Do nothing.
  } else if (is_enable == false) {
    EsfLedManagerResult ret = EsfLedManagerLedOperation(led_status);
    if (ret != kEsfLedManagerSuccess) {
      ESF_LED_MANAGER_WARN("EsfLedManagerLedOperation func failed ret = %u.",
                           ret);
      return ret;
    }
  }
  ESF_LED_MANAGER_TRACE("exit.");
  return kEsfLedManagerSuccess;
}

static EsfLedManagerResult EsfLedManagerSetLedStatus(
    const EsfLedManagerLedStatusInfo* set_status,
    EsfLedManagerLedStatusStorage* led_state) {
  ESF_LED_MANAGER_TRACE("entry.");
  if (set_status == NULL || led_state == NULL) {
    ESF_LED_MANAGER_WARN("Parameter error. (set_status = %p, led_state = %p)",
                         set_status, led_state);
    return kEsfLedManagerInternalError;
  }

  led_state->status[set_status->led][set_status->status] = set_status->enabled;
  ESF_LED_MANAGER_TRACE("exit.");
  return kEsfLedManagerSuccess;
}

static EsfLedManagerResult EsfLedManagerLedOperation(
    const EsfLedManagerLedStatusStorage* status) {
  ESF_LED_MANAGER_TRACE("entry.");
  if (status == NULL) {
    ESF_LED_MANAGER_WARN("Parameter error. (status = %p)", status);
    return kEsfLedManagerInternalError;
  }
  EsfLedManagerLedInfo led_info[kEsfLedManagerTargetLedNum];
  for (int i = 0; i < kEsfLedManagerTargetLedNum; ++i) {
    led_info[i] = kLedInfoDefault;
  }
  EsfLedManagerResult ret = EsfLedManagerGetLedLightingInfo(status, led_info);
  if (ret != kEsfLedManagerSuccess) {
    ESF_LED_MANAGER_WARN(
        "EsfLedManagerGetLedLightingInfo func failed ret = %u.", ret);
    return ret;
  }
  ret = EsfLedManagerLedLighting(led_info);
  if (ret != kEsfLedManagerSuccess) {
    ESF_LED_MANAGER_WARN("EsfLedManagerLedLighting func failed ret = %u.", ret);
    return ret;
  }
  ESF_LED_MANAGER_TRACE("exit.");
  return kEsfLedManagerSuccess;
}

static EsfLedManagerResult EsfLedManagerGetLedLightingInfo(
    const EsfLedManagerLedStatusStorage* led_state,
    EsfLedManagerLedInfo* led_info) {
  ESF_LED_MANAGER_TRACE("entry.");
  if (led_state == NULL || led_info == NULL) {
    ESF_LED_MANAGER_WARN("Parameter error. (led_state = %p, led_info = %p)",
                         led_state, led_info);
    return kEsfLedManagerInternalError;
  }

  for (size_t led_num = 0; led_num < kEsfLedManagerTargetLedNum; ++led_num) {
    led_info[led_num].color = CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_LED_OFF;
    for (size_t table_num = 0; table_num < kLedControlTable[led_num].size;
         ++table_num) {
      if (led_state->status[led_num]
                           [kLedControlTable[led_num].table[table_num].state] ==
          true) {
        ESF_LED_MANAGER_DEBUG(
            "Obtained lighting table info led id = %zu, table num = %zu",
            led_num, table_num);
        memcpy(&led_info[led_num],
               &kLedControlTable[led_num].table[table_num].led,
               sizeof(led_info[led_num]));
        break;
      }
    }
  }
  ESF_LED_MANAGER_TRACE("exit.");
  return kEsfLedManagerSuccess;
}

static EsfLedManagerResult EsfLedManagerLedLighting(
    const EsfLedManagerLedInfo* led_info) {
  ESF_LED_MANAGER_TRACE("entry.");
  if (led_info == NULL) {
    ESF_LED_MANAGER_WARN("Parameter error. (led_info = %p)", led_info);
    return kEsfLedManagerInternalError;
  }
  for (int i = 0; i < (int)kEsfLedManagerTargetLedNum; ++i) {
    if (led_keep_enabled[i] == true) {
      continue;
    }
    if (led_info[i].color == CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_LED_OFF) {
      if (led_enabled[i] == true) {
        PlErrCode ret = PlLedStopSeq(kLedIdTable[i]);
        if (ret != kPlErrCodeOk) {
          ESF_LED_MANAGER_WARN(
              "PlLedStopSeq func failed. ret = %u, LED ID = %u", ret,
              kLedIdTable[i]);
          ESF_LED_MANAGER_ELOG_ERR(ESF_LED_MANAGER_ELOG_PL_ERROR);
          return kEsfLedManagerLedOperateError;
        }
        ESF_LED_MANAGER_DEBUG("Stop Led ID = %d", i);
        led_enabled[i] = false;
      } else {
        ESF_LED_MANAGER_DEBUG("Skip stop process because LED is not lit");
      }
    } else {
      PlLedSequence* seq = calloc(kSeqNumTable[led_info[i].interval],
                                  sizeof(*seq));
      if (seq == NULL) {
        ESF_LED_MANAGER_WARN("Failed to allocate memory for seq.");
        ESF_LED_MANAGER_ELOG_ERR(ESF_LED_MANAGER_ELOG_INTERNAL_ERROR);
        return kEsfLedManagerOutOfMemory;
      }
      memcpy(seq, kSeqTable[led_info[i].interval],
             sizeof(*seq) * kSeqNumTable[led_info[i].interval]);
      for (int offset = 0; offset < kSeqNumTable[led_info[i].interval];
           ++offset) {
        if (seq[offset].color_id !=
            CONFIG_EXTERNAL_LED_MANAGER_COLOR_CONFIG_LED_OFF) {
          seq[offset].color_id = led_info[i].color;
        }
      }
      PlErrCode ret = PlLedStartSeq(kLedIdTable[i], seq,
                                    kSeqNumTable[led_info[i].interval]);
      if (ret != kPlErrCodeOk) {
        ESF_LED_MANAGER_WARN(
            "PlLedStartSeq func failed. ret = %u, LED ID = %u, Color ID = %u, "
            "seq = %p, seq_len = %d",
            ret, kLedIdTable[i], led_info[i].color, seq,
            kSeqNumTable[led_info[i].interval]);
        ESF_LED_MANAGER_ELOG_ERR(ESF_LED_MANAGER_ELOG_PL_ERROR);
        free(seq);
        return kEsfLedManagerLedOperateError;
      }
      ESF_LED_MANAGER_DEBUG("Led lighting info Led ID = %u, seq = %u",
                            kLedIdTable[i], led_info[i].interval);
      led_enabled[i] = true;
      free(seq);
    }
  }
  ESF_LED_MANAGER_TRACE("exit.");
  return kEsfLedManagerSuccess;
}

EsfLedManagerResult EsfLedManagerGetLedStatus(
    EsfLedManagerLedStatusInfo* status) {
  ESF_LED_MANAGER_TRACE("entry.");
  if (status == NULL) {
    ESF_LED_MANAGER_WARN("Parameter error. (status = %p)", status);
    return kEsfLedManagerInternalError;
  }
  status->enabled = led_status->status[status->led][status->status];
  ESF_LED_MANAGER_TRACE("exit.");
  return kEsfLedManagerSuccess;
}

EsfLedManagerResult EsfLedManagerMutexLock(void) {
  ESF_LED_MANAGER_TRACE("entry.");
  int32_t timeout_msec = CONFIG_EXTERNAL_LED_MANAGER_TIMEOUT;
  struct timespec timeout = {0};
  EsfLedManagerResult result = EsfLedManagerSetTime(timeout_msec, &timeout);
  if (result != kEsfLedManagerSuccess) {
    ESF_LED_MANAGER_WARN("EsfLedManagerSetTime func failed ret = %u.", result);
    return kEsfLedManagerInternalError;
  }
  int ret = pthread_mutex_timedlock(&mutex, &timeout);
  if (ret != 0) {
    if (ret == ETIMEDOUT) {
      ESF_LED_MANAGER_WARN("pthread_mutex_timedlock func Timeout.");
      ESF_LED_MANAGER_ELOG_ERR(ESF_LED_MANAGER_ELOG_INTERNAL_ERROR);
      return kEsfLedManagerTimeOut;
    }
    ESF_LED_MANAGER_WARN("pthread_mutex_timedlock func failed ret = %d.", ret);
    ESF_LED_MANAGER_ELOG_ERR(ESF_LED_MANAGER_ELOG_INTERNAL_ERROR);
    return kEsfLedManagerInternalError;
  }

  ESF_LED_MANAGER_TRACE("exit.");
  return kEsfLedManagerSuccess;
}

EsfLedManagerResult EsfLedManagerMutexUnlock(void) {
  ESF_LED_MANAGER_TRACE("entry.");
  int ret = pthread_mutex_unlock(&mutex);
  if (ret != 0) {
    ESF_LED_MANAGER_WARN("pthread_mutex_unlock func failed ret = %d.", ret);
    return kEsfLedManagerInternalError;
  }

  ESF_LED_MANAGER_TRACE("exit.");
  return kEsfLedManagerSuccess;
}

static EsfLedManagerResult EsfLedManagerSetTime(const int32_t time_msec,
                                                struct timespec* time) {
  ESF_LED_MANAGER_TRACE("entry.");
  if (time == NULL) {
    ESF_LED_MANAGER_WARN("Parameter error. (time = %p)", time);
    return kEsfLedManagerInternalError;
  }

  clock_gettime(CLOCK_REALTIME, time);

  int32_t time_sec = time_msec / 1000;
  static const int64_t kSecToNanoSec = 1 * 1000 * 1000 * 1000;
  time->tv_sec += time_sec;
  time->tv_nsec += (time_msec - time_sec * 1000) * 1000 * 1000;
  if (time->tv_nsec >= kSecToNanoSec) {
    ++time->tv_sec;
    time->tv_nsec -= kSecToNanoSec;
  }

  ESF_LED_MANAGER_TRACE("exit.");
  return kEsfLedManagerSuccess;
}
