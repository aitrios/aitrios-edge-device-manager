/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

// Define the internal API for ButtonManager code.

#ifndef ESF_BUTTON_MANAGER_SRC_BUTTON_MANAGER_INTERNAL_H_
#define ESF_BUTTON_MANAGER_SRC_BUTTON_MANAGER_INTERNAL_H_

#include <stdint.h>

#include "button_manager.h"
#include "utility_log.h"
#include "utility_log_module_id.h"

// This macro is defined to output error logs that occur within the
// ButtonManager.
#define ESF_BUTTON_MANAGER_ERROR(fmt, ...)                             \
  WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:" fmt, __FILE__, __LINE__, \
                   ##__VA_ARGS__)

// This code defines an enumeration type for the state of the button manager.
typedef enum {
  kEsfButtonManagerStateClose,  // Close state.
  kEsfButtonManagerStateOpen    // Open state.
} EsfButtonManagerState;

// This code defines an enumeration type that defines the state of the button.
typedef enum {
  kEsfButtonManagerButtonStateTypePressed,      // Pressed state.
  kEsfButtonManagerButtonStateTypeReleased,     // Released state.
  kEsfButtonManagerButtonStateTypeLongPressed,  // Long Pressed state.
  kEsfButtonManagerButtonStateTypeUnknown       // Unknown state.
} EsfButtonManagerButtonStateType;

// This is an enumeration type that defines whether execution of button
// notification callbacks is allowed.
typedef enum {
  kEsfButtonManagerNotificationCallbackStateEnable,  // Enable state.
  kEsfButtonManagerNotificationCallbackStateDisable  // Disable state.
} EsfButtonManagerNotificationCallbackState;

// """Initialize Button Manager.

// Button Manager Performs initialization processing at startup.

// Returns:
//     EsfButtonManagerStatus: The code returns one of the values
//       EsfButtonManagerStatus depending on the execution result.
//       Returns the value kEsfButtonManagerStatusOk if the process is
//       successful.

// Raises:
//     kEsfButtonManagerStatusInternalError: Returned when any initialization
//       process fails.

// """
EsfButtonManagerStatus EsfButtonManagerInitialize(void);

// """Finalize Button Manager.

// Performs stop processing of Button Manager.

// Returns:
//     EsfButtonManagerStatus: The code returns one of the values
//       EsfButtonManagerStatus depending on the execution result.
//       Returns the value kEsfButtonManagerStatusOk if the process is
//       successful.

// Raises:
//     kEsfButtonManagerStatusInternalError: Returned if canceling the callback
//       registered to HAL Button fails.

// """
EsfButtonManagerStatus EsfButtonManagerFinalize(void);

// """Create a handle.

// Create and return handles, and register them to internally managed lists.

// Args:
//     handle (EsfButtonManagerHandle *): Control handle for ButtonManager.
//       Setting NULL is not allowed.

// Returns:
//     EsfButtonManagerStatus: The code returns one of the values
//       EsfButtonManagerStatus depending on the execution result.
//       Returns the value kEsfButtonManagerStatusOk if the process is
//       successful.

// Raises:
//     kEsfButtonManagerStatusParamError: Returns if handle is NULL.
//     kEsfButtonManagerStatusInternalError: Returned when locking a mutex
//       fails, creating a handle data list fails, or adding an element to
//       the handle list fails.
//     kEsfButtonManagerStatusResourceError: Returns when the handle
//       list element is greater than or equal to
//       CONFIG_ESF_BUTTON_MANAGER_HANDLE_MAX_NUM.

// """
EsfButtonManagerStatus EsfButtonManagerCreateHandle(
    EsfButtonManagerHandle* handle);

// """Destroy the handle.

// Destroy the handle and remove it from the internally managed list.

// Args:
//     handle (EsfButtonManagerHandle): Control handle for ButtonManager.
//       Setting NULL is not allowed.

// Returns:
//     EsfButtonManagerStatus: The code returns one of the values
//       EsfButtonManagerStatus depending on the execution result.
//       Returns the value kEsfButtonManagerStatusOk if the process is
//       successful.

// Raises:
//     kEsfButtonManagerStatusHandleError: Returned when handle validation
//       fails. kEsfButtonManagerStatusParamError: Returns if handle is NULL.
//     kEsfButtonManagerStatusInternalError: Returned when locking the mutex
//       fails, failing to delete an element from the handle data
//       list destroying the list, or failing to delete an element from the
//       handle list.

// """
EsfButtonManagerStatus EsfButtonManagerDestroyHandle(
    EsfButtonManagerHandle handle);

// """Processes callback registration for button notification.

// Performs the actual process of registering a callback for button
// notification.

// Args:
//     button_id (uint32_t): button id.
//     min_second (int32_t): The starting number of seconds for the button long
//       press time to execute the button notification callback. This argument
//       is valid only for released and long pressed.
//     max_second (int32_t): This is the number of seconds that the button long
//       press lasts to execute the button notification callback. This argument
//       is valid only when released.
//     callback (const EsfButtonManagerCallback): Callback to register.
//     user_data (void*): Pass a pointer to user data for the button
//       notification callback function. Set it to NULL if not needed.
//     type (EsfButtonManagerButtonStateType): The type of button state.
//     handle (EsfButtonManagerHandle): Control handle for ButtonManager.
//       Setting NULL is not allowed.

// Returns:
//     EsfButtonManagerStatus: The code returns one of the values
//       EsfButtonManagerStatus depending on the execution result.
//       Returns the value kEsfButtonManagerStatusOk if the process is
//       successful.

// Raises:
//     kEsfButtonManagerStatusHandleError: Returned when handle validation
//     fails. kEsfButtonManagerStatusParamError: Returns if handle or callback
//       is NULL, the specified button ID is invalid, or the time setting is
//       invalid.
//     kEsfButtonManagerStatusInternalError: When mutex locking fails, memory
//       area allocation fails, or other internal processing fails.
//     kEsfButtonManagerStatusResourceError: Returns if the specified button has
//       been registered and the maximum number of callbacks would be exceeded.
//     kEsfButtonManagerStatusStateTransitionError: Returns if execution of
//       button notification callback is enabled.

// """
EsfButtonManagerStatus EsfButtonManagerRegisterNotificationCallback(
    uint32_t button_id, int32_t min_second, int32_t max_second,
    const EsfButtonManagerCallback callback, void* user_data,
    EsfButtonManagerButtonStateType type, EsfButtonManagerHandle handle);

// """Processes the cancellation of the button notification callback.

// Performs the actual processing of canceling the button notification callback.

// Args:
//     button_id (uint32_t): button id.
//     type (EsfButtonManagerButtonStateType): The type of button state.
//     handle (EsfButtonManagerHandle): Control handle for ButtonManager.
//       Setting NULL is not allowed.

// Returns:
//     EsfButtonManagerStatus: The code returns one of the values
//       EsfButtonManagerStatus depending on the execution result.
//       Returns the value kEsfButtonManagerStatusOk if the process is
//       successful.

// Raises:
//     kEsfButtonManagerStatusHandleError: Returned when handle validation
//     fails. kEsfButtonManagerStatusParamError: Returned if handle is NULL, if
//       the specified button ID is invalid.
//     kEsfButtonManagerStatusInternalError: When mutex locking fails, or other
//       internal processing fails.
//     kEsfButtonManagerStatusStateTransitionError: Returns if execution of
//       button notification callback is enabled.

// """
EsfButtonManagerStatus EsfButtonManagerUnregisterNotificationCallback(
    uint32_t button_id, EsfButtonManagerButtonStateType type,
    EsfButtonManagerHandle handle);

// """Set whether to execute the button notification callback.

// Set whether to execute the button notification callback.

// Args:
//     flag (EsfButtonManagerNotificationCallbackState): Execution setting flag
//       for notification callback.
//     handle (EsfButtonManagerHandle): Control handle for ButtonManager.
//       Setting NULL is not allowed.

// Returns:
//     EsfButtonManagerStatus: The code returns one of the values
//       EsfButtonManagerStatus depending on the execution result.
//       Returns the value kEsfButtonManagerStatusOk if the process is
//       successful.

// Raises:
//     kEsfButtonManagerStatusHandleError: Returned when handle validation
//       fails. kEsfButtonManagerStatusParamError: Returns if handle is NULL.
//     kEsfButtonManagerStatusInternalError: Returned if mutex locking or value
//       setting fails.

// """
EsfButtonManagerStatus EsfButtonManagerSetCallbackExecutableState(
    EsfButtonManagerNotificationCallbackState flag,
    EsfButtonManagerHandle handle);

// """Get the state of Button Manager.

// Get the state of Button Manager.

// Returns:
//     EsfButtonManagerState: This is the state transition of Button Manager.
//       kEsfButtonManagerStateClose or kEsfButtonManagerStateOpen will be
//       returned.

// """
EsfButtonManagerState EsfButtonManagerGetState(void);

// """Set the state of Button Manager.

// Sets the state of Button Manager.

// Args:
//     state (EsfButtonManagerState) This is the state transition of Button
//       Manager.

// """
void EsfButtonManagerSetState(EsfButtonManagerState state);

// """Get the number of handles.

// Returns the number of handles registered in the handle list.

// Returns:
//     int32_t: The number of handles will be returned. Returns 0 if the list is
//       NULL.

// """
int32_t EsfButtonManagerGetNumberOfHandles(void);

#endif  // ESF_BUTTON_MANAGER_SRC_BUTTON_MANAGER_INTERNAL_H_
