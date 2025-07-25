/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESF_MAIN_MAIN_H_
#define ESF_MAIN_MAIN_H_
#ifdef __cplusplus
extern "C" {
#endif

// enum
// This enumeration type for the result of executing an API.
typedef enum EsfMainError {
  kEsfMainOk,                      // No errors.
  kEsfMainErrorInvalidArgument,    // Argument error.
  kEsfMainErrorResourceExhausted,  // Resource exhausted error.
  kEsfMainErrorInternal,           // Internal error.
  kEsfMainErrorUninitialize,       // Uninitialize error.
  kEsfMainErrorExternal,           // External error.
  kEsfMainErrorTimeout,            // Mutex lock timeout error.
  kEsfMainErrorNotSupport,         // Not support error.
} EsfMainError;

// This enumeration type for system outage notification type.
typedef enum EsfMainMsgType {
  kEsfMainMsgTypeReboot,                    // Reboot.
  kEsfMainMsgTypeShutdown,                  // Shutdown.
  kEsfMainMsgTypeFactoryReset,              // Factory Reset.
  kEsfMainMsgTypeFactoryResetForDowngrade,  // Factory Reset for Downgrade
} EsfMainMsgType;

// """System shutdown notification processing.

// System shutdown notification processing.
// Notifies the system outage management of
// the opportunity to start the specified process.

// Args:
//     type (EsfMainMsgType): Notification system stop type.

// Returns:
//     One of the values of EsfMainError is returned
//     depending on the execution result.

// Yields:
//     kEsfMainOk: Success.
//     kEsfMainErrorInvalidArgument: Invalid value specified for type.
//     kEsfMainErrorTimeout: Mutex lock timed out.
//     kEsfMainErrorUninitialize: Error occurs when ESFMain is uninitialized.
//     kEsfMainErrorExternal: External API error occurred.
//     kEsfMainErrorInternal: Internal API error occurred.

// Note:

// """
EsfMainError EsfMainNotifyMsg(EsfMainMsgType type);

#ifdef __cplusplus
}
#endif
#endif  // ESF_MAIN_MAIN_H_
