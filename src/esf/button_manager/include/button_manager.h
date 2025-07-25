/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

// Define the external public API for ButtonManager code.

#ifndef ESF_BUTTON_MANAGER_INCLUDE_BUTTON_MANAGER_H_
#define ESF_BUTTON_MANAGER_INCLUDE_BUTTON_MANAGER_H_

#include <stdint.h>

// This code defines an enumeration type for the state of executing an API.
typedef enum {
  kEsfButtonManagerStatusOk,                    // No errors.
  kEsfButtonManagerStatusHandleError,           // Invalid Handle Error.
  kEsfButtonManagerStatusParamError,            // Parameter Error.
  kEsfButtonManagerStatusInternalError,         // Internal Error.
  kEsfButtonManagerStatusResourceError,         // Resource Error.
  kEsfButtonManagerStatusStateTransitionError,  // State Transition Error.
} EsfButtonManagerStatus;

// This code defines an handle type of executing an API.
typedef void *EsfButtonManagerHandle;

// This code defines a pointer that register a notification callback function to
// button manager.
typedef void (*EsfButtonManagerCallback)(void *user_data);

// """Launch the ButtonManager startup process and obtain a control handle.

// Perform the startup process for ButtonManager and register the following
// detection callbacks with HAL:
// - Button detection callback
// - Time elapsed detection callback
// Transition to the OPEN state and acquire control handles. A maximum of
// CONFIG_ESF_BUTTON_MANAGER_HANDLE_MAX_NUM handles can be obtained, and if
// already in the OPEN state, only handle acquisition is performed. If an error
// occurs, no state transition takes place.

// Args:
//     handle (EsfButtonManagerHandle *): Control handle for ButtonManager.
//       Setting NULL is not allowed.

// Returns:
//     kEsfButtonManagerStatusOk: Normal termination.
//     kEsfButtonManagerStatusParamError: If the argument's handle is NULL.
//     kEsfButtonManagerStatusInternalError: If there is a failure in creating
//       threads, registering callbacks for HAL detection, etc., ButtonManager
//       cannot be launched.
//     kEsfButtonManagerStatusResourceError: When the maximum number
//       of handles has been reached and no more handles can be acquired.

// """
EsfButtonManagerStatus EsfButtonManagerOpen(EsfButtonManagerHandle *handle);

// """Stop the ButtonManager and close its handle.

// Stop the ButtonManager and close its handle. All registered callbacks for
// notifications on the handle will be unregistered. If all handles are closed,
// the detection callback registered with HAL is unregistered and the state
// transitions to CLOSED. No state transition occurs in case of an error.

// Args:
//     handle (EsfButtonManagerHandle): Control handle for ButtonManager.
//       Setting NULL is not allowed.

// Returns:
//     kEsfButtonManagerStatusOk: Normal termination.
//     kEsfButtonManagerStatusHandleError: If the handle passed as an argument
//     is
//       an invalid handle.
//     kEsfButtonManagerStatusParamError: If the argument's handle is NULL.
//     kEsfButtonManagerStatusInternalError: If there is a failure in destroying
//       threads, unregistering callbacks for HAL detection, etc., and it
//       becomes impossible to stop ButtonManager.
//     kEsfButtonManagerStatusStateTransitionError: When called in the "CLOSE"
//       state.

// """
EsfButtonManagerStatus EsfButtonManagerClose(EsfButtonManagerHandle handle);

// """Register a callback for button pressed notification.

// Register a callback for button pressed notification. Notification callback
// can be registered when callback execution permission is disabled. The maximum
// number of callbacks that can be registered is
// CONFIG_ESF_BUTTON_MANAGER_NOTIFICATION_CALLBACK_MAX_NUM. Callbacks are
// executed in the order they are registered.

// Args:
//     button_id (uint32_t): button id.
//     callback (const EsfButtonManagerCallback): Callback function to notify
//       when button is pressed.
//     user_data (void*): Pass a pointer to user data for the button pressed
//       notification callback function. Set it to NULL if not needed.
//     handle (EsfButtonManagerHandle): Control handle for ButtonManager.
//       Setting NULL is not allowed.

// Returns:
//     kEsfButtonManagerStatusOk: Normal termination.
//     kEsfButtonManagerStatusHandleError: If the handle passed as an argument
//       is an invalid handle.
//     kEsfButtonManagerStatusParamError: If handle or callback is NULL, if the
//       specified button ID is invalid.
//     kEsfButtonManagerStatusInternalError: When mutex locking fails, memory
//       area allocation fails, or other internal processing fails.
//     kEsfButtonManagerStatusResourceError: Returns if the specified button has
//       been registered and the maximum number of callbacks would be exceeded.
//     kEsfButtonManagerStatusStateTransitionError: When called in the "CLOSE"
//       state or notification callback execution permission is enabled.

// """
EsfButtonManagerStatus EsfButtonManagerRegisterPressedCallback(
    uint32_t button_id, const EsfButtonManagerCallback callback,
    void *user_data, EsfButtonManagerHandle handle);

// """Register a callback for button released notification.

// Register a callback for button released notification. Notification callback
// can be registered when callback execution permission is disabled. The maximum
// number of callbacks that can be registered is
// CONFIG_ESF_BUTTON_MANAGER_NOTIFICATION_CALLBACK_MAX_NUM. Callbacks are
// executed in the order they are registered. See external specifications for
// time setting examples.

// Args:
//     button_id (uint32_t): button id.
//     min_second (int32_t): This is the starting number of seconds for the
//       button long press time to execute the button released notification
//       callback. Possible values are between 0 and 120.
//     max_second (int32_t): This is the number of seconds that the button
//       long press time ends when the button release notification callback
//       is executed. Possible values are between 0 and 120.
//     callback (const EsfButtonManagerCallback): Callback function to
//       notify when button is released.
//     user_data (void*): Pass a pointer to user data for the button released
//       notification callback function. Set it to NULL if not needed.
//     handle (EsfButtonManagerHandle): Control handle for ButtonManager.
//       Setting NULL is not allowed.

// Returns:
//     kEsfButtonManagerStatusOk: Normal termination.
//     kEsfButtonManagerStatusHandleError: If the handle passed as an argument
//      is an invalid handle.
//     kEsfButtonManagerStatusParamError: If handle or callback is NULL, if the
//       specified button ID is invalid, if the time setting is incorrect.
//     kEsfButtonManagerStatusInternalError: When mutex locking fails, memory
//       area allocation fails, or other internal processing fails.
//     kEsfButtonManagerStatusResourceError: Returns if the specified button has
//       been registered and the maximum number of callbacks would be exceeded.
//     kEsfButtonManagerStatusStateTransitionError: When called in the "CLOSE"
//       state or notification callback execution permission is enabled.

// """
EsfButtonManagerStatus EsfButtonManagerRegisterReleasedCallback(
    uint32_t button_id, int32_t min_second, int32_t max_second,
    const EsfButtonManagerCallback callback, void *user_data,
    EsfButtonManagerHandle handle);

// """Register a callback for button long press notification.

// Register a callback for button long press notification. Notification callback
// can be registered when callback execution permission is disabled. The maximum
// number of callbacks that can be registered is
// CONFIG_ESF_BUTTON_MANAGER_NOTIFICATION_CALLBACK_MAX_NUM. Callbacks are
// executed in the order they are registered.

// Args:
//     button_id (uint32_t): button id.
//     second (int32_t): This is the button long press time to execute the
//       button press notification callback. Possible values are between 0
//       and 120.
//     callback (const EsfButtonManagerCallback): Callback function to
//       notify when button is long pressed.
//     user_data (void*): Pass a pointer to user data for the button long
//       pressed notification callback function. Set it to NULL if not needed.
//     handle (EsfButtonManagerHandle): Control handle for ButtonManager.
//       Setting NULL is not allowed.

// Returns:
//     kEsfButtonManagerStatusOk: Normal termination.
//     kEsfButtonManagerStatusHandleError: If the handle passed as an argument
//       is an invalid handle.
//     kEsfButtonManagerStatusParamError: If handle or callback is NULL, if the
//       specified button ID is invalid, if the time setting is incorrect.
//     kEsfButtonManagerStatusInternalError: When mutex locking fails, memory
//       area allocation fails, or other internal processing fails.
//     kEsfButtonManagerStatusResourceError: Returns if the specified button has
//       been registered and the maximum number of callbacks would be exceeded.
//     kEsfButtonManagerStatusStateTransitionError: When called in the "CLOSE"
//       state or notification callback execution permission is enabled.

// """
EsfButtonManagerStatus EsfButtonManagerRegisterLongPressedCallback(
    uint32_t button_id, int32_t second, const EsfButtonManagerCallback callback,
    void *user_data, EsfButtonManagerHandle handle);

// """Unregisters the specified button pressed notification callback.

// Unregisters the specified button pressed notification callback. Cancel all
// registered callbacks. Notification callback Can be canceled when callback
// execution permission is disabled.

// Args:
//     button_id (uint32_t): button id.
//     handle (EsfButtonManagerHandle): Control handle for ButtonManager.
//     Setting
//       NULL is not allowed.

// Returns:
//     kEsfButtonManagerStatusOk: Normal termination.
//     kEsfButtonManagerStatusHandleError: If the handle passed as an argument
//       is an invalid handle.
//     kEsfButtonManagerStatusParamError: Returned if handle is NULL, if the
//       specified button ID is invalid.
//     kEsfButtonManagerStatusInternalError: When mutex locking fails, or other
//       internal processing fails.
//     kEsfButtonManagerStatusStateTransitionError: When called in the "CLOSE"
//       state or notification callback execution permission is enabled.

// """
EsfButtonManagerStatus EsfButtonManagerUnregisterPressedCallback(
    uint32_t button_id, EsfButtonManagerHandle handle);

// """Unregisters the specified button released notification callback.

// Unregisters the specified button released notification callback. Cancel all
// registered callbacks. Notification callback If the callback execution
// permission is disabled, it can be canceled.

// Args:
//     button_id (uint32_t): button id.
//     handle (EsfButtonManagerHandle): Control handle for ButtonManager.
//       Setting NULL is not allowed.

// Returns:
//     kEsfButtonManagerStatusOk: Normal termination.
//     kEsfButtonManagerStatusHandleError: If the handle passed as an argument
//       is an invalid handle.
//     kEsfButtonManagerStatusParamError: Returned if handle is NULL, if the
//       specified button ID is invalid.
//     kEsfButtonManagerStatusInternalError: When mutex locking fails, or other
//       internal processing fails.
//     kEsfButtonManagerStatusStateTransitionError: When called in the "CLOSE"
//       state or notification callback execution permission is enabled.

// """
EsfButtonManagerStatus EsfButtonManagerUnregisterReleasedCallback(
    uint32_t button_id, EsfButtonManagerHandle handle);

// """Unregisters the specified button long pressed notification callback.

// Unregisters the specified button long pressed notification callback. Cancel
// all registered callbacks. Notification callback If the callback execution
// permission is disabled, it can be canceled.

// Args:
//     button_id (uint32_t): button id.
//     handle (EsfButtonManagerHandle): Control handle for ButtonManager.
//       Setting NULL is not allowed.

// Returns:
//     kEsfButtonManagerStatusOk: Normal termination.
//     kEsfButtonManagerStatusHandleError: If the handle passed as an argument
//       is an invalid handle.
//     kEsfButtonManagerStatusParamError: Returned if handle is NULL,if the
//       specified button ID is invalid.
//     kEsfButtonManagerStatusInternalError: When mutex locking fails, or other
//       internal processing fails.
//     kEsfButtonManagerStatusStateTransitionError: When called in the "CLOSE"
//       state or notification callback execution permission is enabled.

// """
EsfButtonManagerStatus EsfButtonManagerUnregisterLongPressedCallback(
    uint32_t button_id, EsfButtonManagerHandle handle);

// """Enables execution of button notification callbacks.

// Enables execution of button notification callbacks.

// Args:
//     handle (EsfButtonManagerHandle): Control handle for ButtonManager.
//       Setting NULL is not allowed.

// Returns:
//     kEsfButtonManagerStatusOk: Normal termination.
//     kEsfButtonManagerStatusHandleError: If the handle passed as an argument
//       is an invalid handle.
//     kEsfButtonManagerStatusParamError: Returns if handle is NULL.
//     kEsfButtonManagerStatusInternalError: When mutex locking fails.
//     kEsfButtonManagerStatusStateTransitionError: When called in the "CLOSE"
//       state.

// """
EsfButtonManagerStatus EsfButtonManagerEnableNotificationCallback(
    EsfButtonManagerHandle handle);

// """Disables execution of button notification callbacks.

// Disables execution of button notification callbacks.

// Args:
//     handle (EsfButtonManagerHandle): Control handle for ButtonManager.
//       Setting NULL is not allowed.

// Returns:
//     kEsfButtonManagerStatusOk: Normal termination.
//     kEsfButtonManagerStatusHandleError: If the handle passed as an argument
//       is an invalid handle.
//     kEsfButtonManagerStatusParamError: Returns if handle is NULL.
//     kEsfButtonManagerStatusInternalError: When mutex locking fails.
//     kEsfButtonManagerStatusStateTransitionError: When called in the "CLOSE"
//       state.

// """
EsfButtonManagerStatus EsfButtonManagerDisableNotificationCallback(
    EsfButtonManagerHandle handle);

#endif  // ESF_BUTTON_MANAGER_INCLUDE_BUTTON_MANAGER_H_
