/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESF_LED_MANAGER_LED_MANAGER_H_
#define ESF_LED_MANAGER_LED_MANAGER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

// An enumerated type that expresses the result of Led Manager
typedef enum EsfLedManagerResult {
  kEsfLedManagerSuccess,               // No errors.
  kEsfLedManagerInternalError,         // Internal error.
  kEsfLedManagerInvalidArgument,       // Argument error.
  kEsfLedManagerTimeOut,               // Time out.
  kEsfLedManagerStatusNotFound,        // Status not found.
  kEsfLedManagerStateTransitionError,  // State transition error.
  kEsfLedManagerOutOfMemory,           // Memory allocation error.
  kEsfLedManagerLedOperateError,       // PL Led API error.
} EsfLedManagerResult;

// Enumerated type used to specify Led.
typedef enum EsfLedManagerTargetLed {
  kEsfLedManagerTargetLedPower,    // Target Power Led.
  kEsfLedManagerTargetLedWifi,     // Target Wifi Led.
  kEsfLedManagerTargetLedService,  // Target Service Led.
  kEsfLedManagerTargetLedNum       // The number of this Enum value.
} EsfLedManagerTargetLed;

// An enumerated type that defines the state of Led.
typedef enum EsfLedManagerLedStatus {
  kEsfLedManagerLedStatusForcedOff,
  kEsfLedManagerLedStatusResetting,
  kEsfLedManagerLedStatusAbleToAcceptInputs,
  kEsfLedManagerLedStatusUnableToAcceptInputs,
  kEsfLedManagerLedStatusConnectedWithTLS,
  kEsfLedManagerLedStatusConnectedWithoutTLS,
  kEsfLedManagerLedStatusDisconnectedConnectingDNSAndNTP,
  kEsfLedManagerLedStatusDisconnectedEstablishingNetworkLinkOnPhysicalLayer,
  kEsfLedManagerLedStatusDisconnectedNoInternetConnection,
  kEsfLedManagerLedStatusDisconnectedConnectingWithTLS,
  kEsfLedManagerLedStatusDisconnectedConnectingWithoutTLS,
  kEsfLedManagerLedStatusDisconnectedConnectingProxy,
  kEsfLedManagerLedStatusWaitingForInputsToConnectConsole,
  kEsfLedManagerLedStatusWaitingForInputsToConnectConsoleGlobalProvisioner,
  kEsfLedManagerLedStatusLoadingSSIDPassword,
  kEsfLedManagerLedStatusSearchingAP,
  kEsfLedManagerLedStatusAPFoundAndDoingAuthentication,
  kEsfLedManagerLedStatusLinkEstablished,
  kEsfLedManagerLedStatusErrorPeripheralDriversInitializationFailed,
  kEsfLedManagerLedStatusErrorNetworkInitializationFailed,
  kEsfLedManagerLedStatusErrorLegacyUSB,
  kEsfLedManagerLedStatusErrorInvalidQRCode,
  kEsfLedManagerLedStatusErrorUploadFailed,
  kEsfLedManagerLedStatusErrorDownloadFailed,
  kEsfLedManagerLedStatusErrorAuthProxyFailed,
  kEsfLedManagerLedStatusErrorUpdateMemoryAllocateFailed,
  kEsfLedManagerLedStatusErrorDataFlashFailed,
  kEsfLedManagerLedStatusNum,
} EsfLedManagerLedStatus;

// Structure for specifying/getting LED status.
typedef struct EsfLedManagerLedStatusInfo {
  EsfLedManagerTargetLed led;     // Target Led.
  EsfLedManagerLedStatus status;  // Led State
  bool enabled;                   // State enable/disable flag.
} EsfLedManagerLedStatusInfo;

// """Performs initialization process for Led Manager.

// Args:
//    void.

// Returns:
//    kEsfLedManagerSuccess: Normal termination.
//    kEsfLedManagerOutOfMemory: Memory allocation failure.
//    kEsfLedManagerInternalError: Internal error.
//    kEsfLedManagerLedOperateError: LED operation failure.

// Note:
//    Internal exclusion control.
EsfLedManagerResult EsfLedManagerInit(void);

// """Processes the termination of Led Manager.

// Args:
//    void.

// Returns:
//    kEsfLedManagerSuccess: Normal termination.
//    kEsfLedManagerOutOfMemory: Memory allocation failure.
//    kEsfLedManagerInternalError: Internal error.
//    kEsfLedManagerLedOperateError: LED operation failure.

// Note:
//    Internal exclusion control.
EsfLedManagerResult EsfLedManagerDeinit(void);

// """Sets the status of the specified LED.

// Enables/disables the specified LED status.
// After setting, the LEDs are illuminated with the LED lighting setting with
// the highest priority among the states enabled for each LED.

// Args:
//    status (const EsfLedManagerLedStatusInfo*): This structure sets the LED
//    status.
//      NULL is not acceptable.

// Returns:
//    kEsfLedManagerSuccess: Normal termination.
//    kEsfLedManagerInternalError: Internal error.
//    kEsfLedManagerInvalidArgument: Arg parameter error.
//    kEsfLedManagerOutOfMemory: Memory allocation failure.
//    kEsfLedManagerTimeOut: Time out error.
//    kEsfLedManagerStatusNotFound: Status not found.
//    kEsfLedManagerStateTransitionError: State transition error.
//    kEsfLedManagerLedOperateError: LED operation failure.

// Note:
//    Internal exclusion control.
//    This API is asynchronous.
EsfLedManagerResult EsfLedManagerSetStatus(
    const EsfLedManagerLedStatusInfo* status);

// """Acquire the status of the specified LEDs.

// Args:
//    status (EsfLedManagerLedStatusInfo*): A structure that stores the state.
//      NULL is not acceptable.

// Returns:
//    kEsfLedManagerSuccess: Normal termination.
//    kEsfLedManagerInternalError: Internal error.
//    kEsfLedManagerInvalidArgument: Arg parameter error.
//    kEsfLedManagerTimeOut: Time out error.
//    kEsfLedManagerStateTransitionError: State transition error.
//    kEsfLedManagerStatusNotFound: Status not found.

// Note:
//    Internal exclusion control.
EsfLedManagerResult EsfLedManagerGetStatus(EsfLedManagerLedStatusInfo* status);

#ifdef __cplusplus
}
#endif

#endif  // ESF_LED_MANAGER_LED_MANAGER_H_
