/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef ESF_LED_MANAGER_SRC_INTERNAL_LED_MANAGER_INTERNAL_H_
#define ESF_LED_MANAGER_SRC_INTERNAL_LED_MANAGER_INTERNAL_H_

#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#include "led_manager/include/led_manager.h"
#include "utility_log.h"
#include "utility_log_module_id.h"

// LED Manager ELOG eventID.
#define ESF_LED_MANAGER_ELOG_INVALID_PARAM (0x8601)
#define ESF_LED_MANAGER_ELOG_INTERNAL_ERROR (0x8602)
#define ESF_LED_MANAGER_ELOG_PL_ERROR (0x8603)

// Led Manager Log.
#ifdef CONFIG_EXTERNAL_LED_MANAGER_UTILITY_LOG_ENABLE
#define ESF_LED_MANAGER_ERR(fmt, ...)                                   \
  (WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:" fmt, __FILE__, __LINE__, \
                    ##__VA_ARGS__))
#define ESF_LED_MANAGER_WARN(fmt, ...)                                 \
  (WRITE_DLOG_WARN(MODULE_ID_SYSTEM, "%s-%d:" fmt, __FILE__, __LINE__, \
                   ##__VA_ARGS__))
#define ESF_LED_MANAGER_INFO(fmt, ...)                                 \
  (WRITE_DLOG_INFO(MODULE_ID_SYSTEM, "%s-%d:" fmt, __FILE__, __LINE__, \
                   ##__VA_ARGS__))
#define ESF_LED_MANAGER_DEBUG(fmt, ...)                                 \
  (WRITE_DLOG_DEBUG(MODULE_ID_SYSTEM, "%s-%d:" fmt, __FILE__, __LINE__, \
                    ##__VA_ARGS__))
#define ESF_LED_MANAGER_TRACE(fmt, ...)

#define ESF_LED_MANAGER_ELOG_ERR(event_id) \
  (WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, event_id))

#else
#define ESF_LED_MANAGER_PRINT(fmt, ...)                           \
  (printf("[%s][%s][%d] " fmt "\n", __FILE__, __func__, __LINE__, \
          ##__VA_ARGS__))
#define ESF_LED_MANAGER_ERR(fmt, ...)                                  \
  (printf("[ERR][%s][%s][%d] " fmt "\n", __FILE__, __func__, __LINE__, \
          ##__VA_ARGS__))
#define ESF_LED_MANAGER_WARN(fmt, ...)                                 \
  (printf("[WRN][%s][%s][%d] " fmt "\n", __FILE__, __func__, __LINE__, \
          ##__VA_ARGS__))
#define ESF_LED_MANAGER_INFO(fmt, ...)                                 \
  (printf("[INF][%s][%s][%d] " fmt "\n", __FILE__, __func__, __LINE__, \
          ##__VA_ARGS__))
#define ESF_LED_MANAGER_DEBUG(fmt, ...)                                \
  (printf("[DBG][%s][%s][%d] " fmt "\n", __FILE__, __func__, __LINE__, \
          ##__VA_ARGS__))
#define ESF_LED_MANAGER_TRACE(fmt, ...)
#define ESF_LED_MANAGER_ELOG_ERR(event_id)                                  \
  (printf("[ELOG][ERR][%s][%s][%d] 0x%04x\n", __FILE__, __func__, __LINE__, \
          (unsigned int)event_id))

#endif  // CONFIG_EXTERNAL_LED_MANAGER_ENABLE_DEBUG_LOG

// """Initialize internal resources.

// Args:
//    void.

// Returns:
//    kEsfLedManagerSuccess: Normal termination.
//    kEsfLedManagerOutOfMemory: Memory allocation failure.
EsfLedManagerResult EsfLedManagerInitResource(void);

// """Releases internal resources.

// Args:
//    void.

// Returns:
//    void.
void EsfLedManagerDeinitResource(void);

// """This function is used to turn off the LEDs and terminate the PL LED.

// Args:
//    void.

// Returns:
//    kEsfLedManagerSuccess: Normal termination.
//    kEsfLedManagerOutOfMemory: Memory allocation failure.
//    kEsfLedManagerLedOperateError: LED operation failure.
EsfLedManagerResult EsfLedManagerLedStop(void);

// """Determines if the specified LED status needs to be updated.

// Args:
//    status (EsfLedManagerLedStatusInfo*): This is a structure that contains
//    the state of the LED to be determined.
//      NULL is not acceptable.

// Returns:
//    true: Need to update status.
//    false: No need for status updates.
bool EsfLedManagerCheckLedStatus(const EsfLedManagerLedStatusInfo* status);

// """Function to set the specified LED status and turn on the LED.

// Args:
//    status (EsfLedManagerLedStatusInfo*): This structure stores the LED
//    status.
//      NULL is not acceptable.

// Returns:
//    kEsfLedManagerSuccess: Normal termination.
//    kEsfLedManagerInternalError: Internal error.
//    kEsfLedManagerOutOfMemory: Memory allocation failure.
//    kEsfLedManagerLedOperateError: LED operation failure.
EsfLedManagerResult EsfLedManagerSetStatusInternal(
    const EsfLedManagerLedStatusInfo* status);

// """Updates the lighting retention setting for the specified LED.

// LED operation is performed when the LED state retention setting is changed
// from true to false.

// Args:
//    led (EsfLedManagerTargetLed): LED to be specified.
//    is_enable (bool): Enable/Disable flag for lighting retention setting.

// Returns:
//    kEsfLedManagerSuccess: Normal termination.
//    kEsfLedManagerInternalError: Internal error.
//    kEsfLedManagerOutOfMemory: Memory allocation failure.
//    kEsfLedManagerLedOperateError: LED operation failure.
EsfLedManagerResult EsfLedManagerSetLightingPersistenceInternal(
    EsfLedManagerTargetLed led, bool is_enable);

// """Obtains the status of the specified LED.

// Args:
//    status (EsfLedManagerLedStatusInfo*): This structure stores the LED
//    status.
//      NULL is not acceptable.

// Returns:
//    kEsfLedManagerSuccess: Normal termination.
//    kEsfLedManagerInternalError: Internal error.
EsfLedManagerResult EsfLedManagerGetLedStatus(
    EsfLedManagerLedStatusInfo* status);

// """Starts exclusive control.

// Starts exclusive control in the specified manner.
// Methods include with and without timeout.

// Args:
//    void.

// Returns:
//    kEsfLedManagerSuccess: Normal termination.
//    kEsfLedManagerInternalError: Internal error.
//    kEsfLedManagerTimeOut: Time out error.
EsfLedManagerResult EsfLedManagerMutexLock(void);

// """Exit Exclusive Control.

// Args:
//    void.

// Returns:
//    kEsfLedManagerSuccess: Normal termination.
//    kEsfLedManagerInternalError: Internal error.
EsfLedManagerResult EsfLedManagerMutexUnlock(void);

#endif  // ESF_LED_MANAGER_SRC_INTERNAL_LED_MANAGER_INTERNAL_H_
