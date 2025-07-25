/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "button_manager_internal.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "button_manager.h"
#include "button_manager_linked_list.h"
#include "pl.h"
#include "pl_button.h"
#include "utility_msg.h"
#include "utility_timer.h"

/****************************************************************************
 * private Data
 ****************************************************************************/

#define BUTTON_MANAGER_THREAD_STACK_SIZE \
  CONFIG_ESF_BUTTON_MANAGER_THREAD_STACK_SIZE
#define BUTTON_MANAGER_MESSAGE_INIT ((int32_t)INT32_MAX)
#define BUTTON_MANAGER_QUEUE_SIZE (10)
#define BUTTON_MANAGER_MESSAGE_PRIORITY (100)
#define BUTTON_MANAGER_TIMEOUT (-1)
#define BUTTON_MANAGER_MAX_START_SECOND (120)
#define BUTTON_MANAGER_MAX_END_SECOND (120)
#define BUTTON_MANAGER_MAX_LONG_PRESSED_SECOND (120)
#define BUTTON_MANAGER_COMMAND_INITIALIZER       \
  {                                              \
    kEsfButtonManagerCommandTypeExecButtonEvent, \
        kEsfButtonManagerButtonStateTypeUnknown, \
        (EsfButtonManagerButtonData *)NULL       \
  }

// This variable is a pointer to the thread's stack and is used in the
// ButtonManager thread.
static void *static_button_manager_thread_stack = (void *)NULL;

// This variable is an object of the thread and is used to generate a
// ButtonManager thread. The pthread_t type does not have an initialization
// method, so we manage the state by setting a flag.
static pthread_t static_button_manager_thread;

// This value is a flag that manages the state of thread startup.
static bool static_is_thread_starting = false;

// This value is used to determine if pl_button has been initialized.
static bool static_is_pl_button_initialized = false;

// This variable is a mutex object and is used for exclusive control of the
// static_handle_list.
static pthread_mutex_t static_button_manager_mutex = PTHREAD_MUTEX_INITIALIZER;

// This variable is a handle to the message queue and is used by threads to send
// commands and data.
static int32_t static_button_manager_message_handle =
    BUTTON_MANAGER_MESSAGE_INIT;

// This variable represents the state transition of ButtonManager.
static EsfButtonManagerState static_button_manager_state =
    kEsfButtonManagerStateClose;

// This variable is a linked list and is used to manage handles for
// ButtonManager.
static EsfButtonManagerList *static_handle_list = (EsfButtonManagerList *)NULL;

// This variable is a linked list and is used to manage information and states
// of buttons. In order to use this list's elements as user data for the
// callback to register them in HAL, they must exist statically.
static EsfButtonManagerList *static_button_data_list =
    (EsfButtonManagerList *)NULL;

// This code defines an enumeration type that defines the
// `EsfButtonManagerThread` command types.
typedef enum {
  kEsfButtonManagerCommandTypeExecButtonEvent,  // Execute button event.
  kEsfButtonManagerCommandTypeCloseThread       // Close the thread.
} EsfButtonManagerCommandType;

// This code defines a structure type for the state of executing an API.
typedef struct {
  // Callback function to notify when button is pressed.
  EsfButtonManagerCallback callback;

  // Pass a pointer to user data for the button pressed
  // notification callback function. Set it to NULL if not
  // needed.
  void *user_data;
} EsfButtonManagerButtonPressedCallbackInfo;

// A structure that combines a callback function for button release notification
// and user data into a button press notification callback information.
typedef struct {
  // The callback function pointer for the button release
  // notification registration. Setting NULL is not allowed.
  EsfButtonManagerCallback callback;

  // Pass a pointer to the user data for the button release
  // notification callback function. Set it to NULL if not
  // needed.
  void *user_data;

  // The start time in seconds for the long press duration
  // to execute the button release notification callback.
  // The settable values are within the range of 0 to 120.
  int32_t min_second;

  // The button for executing the callback on long press
  // duration. The settable values are within the range of
  // 0 to 120 seconds.
  int32_t max_second;
} EsfButtonManagerButtonReleasedCallbackInfo;

// A structure that combines a callback function for long button press
// notifications and user data for the button click notification information.
typedef struct {
  // The pointer to the callback function for
  // long press notification on the "Register"
  // button. Setting NULL is not allowed.
  EsfButtonManagerCallback callback;

  // Pass NULL to the user data pointer for the callback
  // function of long button press notifications if it's not
  // needed.
  void *user_data;

  // The button long press notification callback execution time
  // (seconds) for the button long press duration. The settable
  // values are within the range of 1 to 120.
  int32_t second;
} EsfButtonManagerButtonLongPressedCallbackInfo;

// A structure that defines the data held by the handle.
typedef struct {
  // Handle data list.
  EsfButtonManagerList *handle_data_list;

  // This flag is used to determine whether calling a callback is allowed.
  EsfButtonManagerNotificationCallbackState callback_execution;
} EsfButtonManagerHandleFrame;

// A structure that defines handle data.
typedef struct {
  // Button id.
  uint32_t button_id;

  // The amount of time elapsed between when the timer was pressed and when it
  // stopped.
  int32_t long_pressed_time;

  // Pressed callback infomation list.
  EsfButtonManagerList *pressed_list;

  // Released callback infomation list.
  EsfButtonManagerList *released_list;

  // Long Pressed callback infomation list.
  EsfButtonManagerList *long_pressed_list;
} EsfButtonManagerHandleData;

// A structure that defines button data.
typedef struct {
  // Button id.
  uint32_t button_id;

  // Button state received from button detection callback. Used to determine
  // whether to execute a pressed callback when an OPEN or callback is
  // registered while the button is held down.
  EsfButtonManagerButtonStateType state;

  // This handle is for controlling the Utility Timer.
  UtilityTimerHandle timer_handle;

  // This member is used to check whether the callback to be registered in HAL
  // has been successfully registered.
  bool is_callback_registered;
} EsfButtonManagerButtonData;

// A structure that defines the data to send to the thread.
typedef struct {
  // Type of command.
  EsfButtonManagerCommandType type;

  // Button state received from button detection callback.
  EsfButtonManagerButtonStateType state;

  // Button data received from button detection callback.
  EsfButtonManagerButtonData *data;
} EsfButtonManagerCommand;

/****************************************************************************
 * Private Functions Prototypes
 ****************************************************************************/

// """Create a button data list.

// Create button data list and button data. The number of button data is the
// total number of buttons received from HAL Button.

// Args:
//     button_info (const PlButtonInfo *): Button information from HAL Button.
//       Setting NULL is not allowed.

// Returns:
//     EsfButtonManagerStatus: The code returns one of the values
//       EsfButtonManagerStatus depending on the execution result.
//       Returns the value kEsfButtonManagerStatusOk if the process is
//       successful.

// Raises:
//     kEsfButtonManagerStatusParamError: Returns if button_info is NULL.
//     kEsfButtonManagerStatusInternalError: Returns when creating a list fails,
//       or when creating and adding elements to the list fails.

// """
static EsfButtonManagerStatus EsfButtonManagerCreateButtonDataList(
    const PlButtonInfo *button_info);

// """Destroy the button data list.

// Delete the elements of the button data list, destroy the list, and cancel the
// callback registered in HAL Button.

// Returns:
//     EsfButtonManagerStatus: The code returns one of the values
//       EsfButtonManagerStatus depending on the execution result.
//       Returns the value kEsfButtonManagerStatusOk if the process is
//       successful.

// Raises:
//     kEsfButtonManagerStatusParamError: Returns if static_button_data_list is
//       NULL. kEsfButtonManagerStatusInternalError: Called when the element
//       fails to be destroyed or the callback fails to release.

// """
static EsfButtonManagerStatus EsfButtonManagerDestroyButtonDataList(void);

// """Get button data.

// Gets the button data associated with the specified button ID.

// Args:
//     button_id (uint32_t): Button id.

// Returns:
//    EsfButtonManagerButtonData*: Returns button data for the specified button
//      ID. Returns NULL if not found.

// """
static EsfButtonManagerButtonData *EsfButtonManagerGetButtonData(
    uint32_t button_id);

// """Check the button state and exec pressed callback.

// Check the all button state. If the button is pressed, execute the pressed
// notification callback.

// Args:
//     handle_data_list (EsfButtonManagerList *): Specify from the data held
//       by the handle. Setting NULL is not allowed.

// Returns:
//     EsfButtonManagerStatus: The code returns one of the values
//       EsfButtonManagerStatus depending on the execution result.
//       Returns the value kEsfButtonManagerStatusOk if the process is
//       successful.

// Raises:
//     kEsfButtonManagerStatusParamError: Returns if lists is NULL.
//     kEsfButtonManagerStatusInternalError: Returns if list operation fails.

// """
static EsfButtonManagerStatus EsfButtonManagerCheckButtonState(
    EsfButtonManagerList *handle_data_list);

// """Create a handle frame.

// Create a handle frame and handle data list.

// Returns:
//    EsfButtonManagerHandleFrame*: Returns a handle frame. NULL is returned
//      when an error occurs.

// """
static EsfButtonManagerHandleFrame *EsfButtonManagerCreateHandleFrame(void);

// """Destroy the handle frame.

// Destroy the handle frame. All elements of the handle data list and the
// callback info lists are also destroyed.

// Args:
//     handle_frame (EsfButtonManagerHandleFrame *): Handle frame. Setting NULL
//       is not allowed.

// Returns:
//     EsfButtonManagerStatus: The code returns one of the values
//       EsfButtonManagerStatus depending on the execution result.
//       Returns the value kEsfButtonManagerStatusOk if the process is
//       successful.

// Raises:
//     kEsfButtonManagerStatusParamError: Returns if handle_frame is NULL.
//     kEsfButtonManagerStatusInternalError: Returns if list operation fails.

// """
static EsfButtonManagerStatus EsfButtonManagerDestroyHandleFrame(
    EsfButtonManagerHandleFrame *handle_frame);

// """Destroy the handle data list.

// Destroys the handle data list. If it has any elements, they will also be
// destroyed.

// Args:
//     handle_data_list (EsfButtonManagerList *): Specify from the data held
//       by the handle. Setting NULL is not allowed.

// Returns:
//     EsfButtonManagerStatus: The code returns one of the values
//       EsfButtonManagerStatus depending on the execution result.
//       Returns the value kEsfButtonManagerStatusOk if the process is
//       successful.

// Raises:
//     kEsfButtonManagerStatusParamError: Returns if handle_data_list is NULL.
//     kEsfButtonManagerStatusInternalError: Called if the element fails to be
//       destroyed.

// """
static EsfButtonManagerStatus EsfButtonManagerDestroyHandleDataList(
    EsfButtonManagerList *handle_data_list);

// """Create a handle data.

// Create a handle data. Set the button ID and create callback info lists.

// Args:
//     button_id (uint32_t): Button id.

// Returns:
//    EsfButtonManagerHandleData*: Returns handle data for the specified button
//      ID. Returns NULL when processing fails.

// """
static EsfButtonManagerHandleData *EsfButtonManagerCreateHandleData(
    uint32_t button_id);

// """Destroy the handle data.

// Destroy the handle data. All elements of the callback info lists are also
// discarded.

// Args:
//     handle_data (EsfButtonManagerHandleData *): Handle data to discard.

// Returns:
//     EsfButtonManagerStatus: The code returns one of the values
//       EsfButtonManagerStatus depending on the execution result.
//       Returns the value kEsfButtonManagerStatusOk if the process is
//       successful.

// Raises:
//     kEsfButtonManagerStatusParamError: Returns if handle_data is NULL.
//     kEsfButtonManagerStatusInternalError: Returns if it fails to destroy each
//       list element.

// """
static EsfButtonManagerStatus EsfButtonManagerDestroyHandleData(
    EsfButtonManagerHandleData *handle_data);

// """Get handle data.

// Gets the handle data associated with the specified button ID.

// Args:
//     button_id (uint32_t): Button id.
//     handle_data_list (const EsfButtonManagerList *): Specify from the data
//       held by the handle. Setting NULL is not allowed.

// Returns:
//    EsfButtonManagerHandleData*: Returns handle data for the specified button
//      ID. Returns NULL if not found.

// """
static EsfButtonManagerHandleData *EsfButtonManagerGetHandleData(
    uint32_t button_id, const EsfButtonManagerList *handle_data_list);

// """Get handle data or prepare new one.

// Gets the handle data with the specified button ID from the specified list. If
// it cannot be obtained, prepare new handle data.

// Args:
//     button_id (uint32_t): Button id.
//     handle_data_list (EsfButtonManagerList *): Specify from the data held
//       by the handle. Setting NULL is not allowed.

// Returns:
//    EsfButtonManagerHandleData*: Returns handle data for the specified button
//      ID. Returns NULL when processing fails.

// """
static EsfButtonManagerHandleData *EsfButtonManagerGetOrPrepareNewHandleData(
    uint32_t button_id, EsfButtonManagerList *handle_data_list);

// """Remove the handle data.

// Removes the specified handle data from the list.

// Args:
//     button_id (uint32_t): Button id.
//     handle_data_list (EsfButtonManagerList *): Specify from the data held
//       by the handle. Setting NULL is not allowed.

// Returns:
//     EsfButtonManagerStatus: The code returns one of the values
//       EsfButtonManagerStatus depending on the execution result.
//       Returns the value kEsfButtonManagerStatusOk if the process is
//       successful.

// Raises:
//     kEsfButtonManagerStatusParamError: Returns if handle_data_list is NULL or
//       if not found the specified button id.
//     kEsfButtonManagerStatusInternalError: Returns when area allocation fails
//       or list operation fails.

// """
static EsfButtonManagerStatus EsfButtonManagerRemoveHandleData(
    uint32_t button_id, EsfButtonManagerList *handle_data_list);

// """Reset long pressed time.

// Resets the long pressed time of the handle data.

// Args:
//     handle_data_list (EsfButtonManagerList *): Specify from the data held
//       by the handle. Setting NULL is not allowed.

// Returns:
//     EsfButtonManagerStatus: The code returns one of the values
//       EsfButtonManagerStatus depending on the execution result.
//       Returns the value kEsfButtonManagerStatusOk if the process is
//       successful.

// Raises:
//     kEsfButtonManagerStatusParamError: Returns if handle_data_list is NULL or
//       if not found the specified button id.
//     kEsfButtonManagerStatusInternalError: Returns when area allocation fails
//       or list operation fails.

// """
static EsfButtonManagerStatus EsfButtonManagerResetLongPressedTime(
    EsfButtonManagerList *handle_data_list);

// """Destroy the callback info list.

// Destroy the callback info list. If it has any elements, they will
// also be destroyed.

// Args:
//     callback_info_list (EsfButtonManagerList *): A list of handle data.

// Returns:
//     EsfButtonManagerStatus: The code returns one of the values
//       EsfButtonManagerStatus depending on the execution result.
//       Returns the value kEsfButtonManagerStatusOk if the process is
//       successful.

// Raises:
//     kEsfButtonManagerStatusParamError: Returns if callback_info_list is NULL.
//     kEsfButtonManagerStatusInternalError: Returns if it fails to destroy each
//       list element.

// """
static EsfButtonManagerStatus EsfButtonManagerDestroyCallbackInfoList(
    EsfButtonManagerList *callback_info_list);

// """Register the pressed callback info.

// Register the pressed callback info.

// Args:
//     callback (const EsfButtonManagerCallback): Callback to register.
//     user_data (void*): Pass a pointer to user data for the button pressed
//       notification callback function. Set it to NULL if not needed.
//     pressed_info_list (EsfButtonManagerList *): A list of handle data.

// Returns:
//     EsfButtonManagerStatus: The code returns one of the values
//       EsfButtonManagerStatus depending on the execution result.
//       Returns the value kEsfButtonManagerStatusOk if the process is
//       successful.

// Raises:
//     kEsfButtonManagerStatusParamError: Returns if callback or
//       pressed_info_list is NULL.
//     kEsfButtonManagerStatusInternalError: Returns when area allocation fails
//       or list operation fails.
//     kEsfButtonManagerStatusResourceError: Returns if the specified button has
//       been registered and the maximum number of callbacks would be exceeded.

// """
static EsfButtonManagerStatus EsfButtonManagerRegisterPressedCallbackInfo(
    const EsfButtonManagerCallback callback, void *user_data,
    EsfButtonManagerList *pressed_info_list);

// """Register a released callback info.

// Register a released callback info.

// Args:
//     min_second (int32_t): This is the starting number of seconds for the
//       button long press time to execute the button released notification
//       callback.
//     max_second (int32_t): This is the number of seconds that the button
//       long press time ends when the button release notification callback
//       is executed.
//     callback (const EsfButtonManagerCallback): Callback to register.
//     user_data (void*): Pass a pointer to user data for the button released
//       notification callback function. Set it to NULL if not needed.
//     released_info_list (EsfButtonManagerList *): A list of handle data.

// Returns:
//     EsfButtonManagerStatus: The code returns one of the values
//       EsfButtonManagerStatus depending on the execution result.
//       Returns the value kEsfButtonManagerStatusOk if the process is
//       successful.

// Raises:
//     kEsfButtonManagerStatusParamError: Returns if callback or
//       released_info_list is NULL.
//     kEsfButtonManagerStatusInternalError: Returns when area allocation fails
//       or list operation fails.
//     kEsfButtonManagerStatusResourceError: Returns if the specified button has
//       been registered and the maximum number of callbacks would be exceeded.

// """
static EsfButtonManagerStatus EsfButtonManagerRegisterReleasedCallbackInfo(
    int32_t min_second, int32_t max_second,
    const EsfButtonManagerCallback callback, void *user_data,
    EsfButtonManagerList *released_info_list);

// """Register a long pressed callback info.

// Register a long pressed callback info.

// Args:
//     second (int32_t): This is the button long press time to execute the
//       button press notification callback.
//     callback (const EsfButtonManagerCallback): Callback to register.
//     user_data (void*): Pass a pointer to user data for the button long
//       pressed notification callback function. Set it to NULL if not needed.
//     long_pressed_info_list (EsfButtonManagerList *): A list of handle data.

// Returns:
//     EsfButtonManagerStatus: The code returns one of the values
//       EsfButtonManagerStatus depending on the execution result.
//       Returns the value kEsfButtonManagerStatusOk if the process is
//       successful.

// Raises:
//     kEsfButtonManagerStatusParamError: Returns if callback or
//       long_pressed_info_list is NULL.
//     kEsfButtonManagerStatusInternalError: Returns when area allocation fails
//       or list operation fails.
//     kEsfButtonManagerStatusResourceError: Returns if the specified button has
//       been registered and the maximum number of callbacks would be exceeded.

// """
static EsfButtonManagerStatus EsfButtonManagerRegisterLongPressedCallbackInfo(
    int32_t second, const EsfButtonManagerCallback callback, void *user_data,
    EsfButtonManagerList *long_pressed_info_list);

// """Remove the callback info from specify the list.

// Removes callback info from a list corresponding to the type.

// Args:
//     type (EsfButtonManagerButtonStateType): The type of button state.
//     handle_data (EsfButtonManagerHandleData *): The callback info will be
//       removed from the list that the specified handle_data.

// Raises:
//     kEsfButtonManagerStatusParamError: Returns if handle_data is NULL.
//     kEsfButtonManagerStatusInternalError: If the specified list is NULL or
//       fails to be deleted, it returns.

// """
static EsfButtonManagerStatus
EsfButtonManagerRemoveAllCallbackInfoFromSpecifyList(
    EsfButtonManagerButtonStateType type,
    EsfButtonManagerHandleData *handle_data);

// """Remove the all callback info.

// Removes the all callback info from the list.

// Args:
//     callback_info_list (EsfButtonManagerList *): A list of handle data.

// Returns:
//     EsfButtonManagerStatus: The code returns one of the values
//       EsfButtonManagerStatus depending on the execution result.
//       Returns the value kEsfButtonManagerStatusOk if the process is
//       successful.

// Raises:
//     kEsfButtonManagerStatusParamError: Returns if callback_info_list is NULL.
//     kEsfButtonManagerStatusInternalError: Returns when area allocation fails
//       or list operation fails.

// """
static EsfButtonManagerStatus EsfButtonManagerRemoveAllCallbackInfo(
    EsfButtonManagerList *callback_info_list);

// """Execute pressed notification callback.

// Execute pressed notification callback. If the same callback is registered, it
// will be executed from the beginning.

// Args:
//     pressed_info_list (EsfButtonManagerList *): A list of handle data.

// """
static void EsfButtonManagerExecutePressedCallback(
    EsfButtonManagerList *pressed_info_list);

// """Execute released notification callback.

// Execute released notification callback. If there are items with the same
// conditions, they will be executed from the beginning.

// Args:
//     long_pressed_time (int32_t): Total pressed time.
//     released_info_list (EsfButtonManagerList *): A list of handle data.

// """
static void EsfButtonManagerExecuteReleasedCallback(
    int32_t long_pressed_time, EsfButtonManagerList *released_info_list);

// """Execute long pressed notification callback.

// Execute long pressed notification callback. If there are items with the same
// conditions, they will be executed from the beginning.

// Args:
//     long_pressed_time (int32_t): Total pressed time.
//     long_pressed_info_list (EsfButtonManagerList *): A list of handle
//       data.

// """
static void EsfButtonManagerExecuteLongPressedCallback(
    int32_t long_pressed_time, EsfButtonManagerList *long_pressed_info_list);

// """Create the thread.

// Create the EsfButtonManagerThread.

// Returns:
//     EsfButtonManagerStatus: The code returns one of the values
//       EsfButtonManagerStatus depending on the execution result.
//       Returns the value kEsfButtonManagerStatusOk if the process is
//       successful.

// Raises:
//     kEsfButtonManagerStatusInternalError: Returns if stack area allocation
//       fails or pthread function processing fails.

// """
static EsfButtonManagerStatus EsfButtonManagerCreateThread(void);

// """Destroy the thread.

// Destroy the EsfButtonManagerThread.

// Returns:
//     EsfButtonManagerStatus: The code returns one of the values
//       EsfButtonManagerStatus depending on the execution result.
//       Returns the value kEsfButtonManagerStatusOk if the process is
//       successful.

// Raises:
//     kEsfButtonManagerStatusInternalError: Returns if send message fails or
//       pthread function processing fails.

// """
static EsfButtonManagerStatus EsfButtonManagerDestroyThread(void);

// """Set button detection callback

// Register a callback for button detection to HAL Button. If registration
// fails, the registration will be canceled and an error will be returned.

// Args:
//     button_info (const PlButtonInfo *): Button information from HAL Button.
//       Setting NULL is not allowed.

// Returns:
//     EsfButtonManagerStatus: The code returns one of the values
//       EsfButtonManagerStatus depending on the execution result.
//       Returns the value kEsfButtonManagerStatusOk if the process is
//       successful.

// Raises:
//     kEsfButtonManagerStatusParamError: Called if button_info or
//       static_button_data_list is NULL.
//     kEsfButtonManagerStatusInternalError: Called if obtaining button
//       information from HAL Button fails, creating a button data list fails,
//       or registering a callback to HAL Button fails.

// """
static EsfButtonManagerStatus EsfButtonManagerSetButtonDetectionCallback(
    const PlButtonInfo *button_info);

// """Unset button detection callback

// Cancels the button detection callback registered to HAL.

// Returns:
//     EsfButtonManagerStatus: The code returns one of the values
//       EsfButtonManagerStatus depending on the execution result.
//       Returns the value kEsfButtonManagerStatusOk if the process is
//       successful.

// Raises:
//     kEsfButtonManagerStatusParamError: Returned if static_button_data_list is
//       NULL.
//     kEsfButtonManagerStatusInternalError: Returned if list operation or
//       cancellation fails.

// """
static EsfButtonManagerStatus EsfButtonManagerUnsetButtonDetectionCallback(
    void);

// """Set timer callback

// Register a callback for timer to Utility Timer. If registration
// fails, the registration will be canceled and an error will be returned.

// Args:
//     button_info (const PlButtonInfo *): Button information from HAL Button.
//       Setting NULL is not allowed.

// Returns:
//     EsfButtonManagerStatus: The code returns one of the values
//       EsfButtonManagerStatus depending on the execution result.
//       Returns the value kEsfButtonManagerStatusOk if the process is
//       successful.

// Raises:
//     kEsfButtonManagerStatusParamError: Called if button_info or
//       static_button_data_list is NULL.
//     kEsfButtonManagerStatusInternalError: Called if obtaining button
//       information from Utility Timer fails, creating a button data list
//       fails, or registering a callback to Utility Timer fails.

// """
static EsfButtonManagerStatus EsfButtonManagerSetTimerCallback(
    const PlButtonInfo *button_info);

// """Unset timer callback

// Cancels the timer callback registered to Utility Timer.

// Returns:
//     EsfButtonManagerStatus: The code returns one of the values
//       EsfButtonManagerStatus depending on the execution result.
//       Returns the value kEsfButtonManagerStatusOk if the process is
//       successful.

// Raises:
//     kEsfButtonManagerStatusParamError: Returned if static_button_data_list is
//       NULL.
//     kEsfButtonManagerStatusInternalError: Returned if list operation or
//       cancellation fails.

// """
static EsfButtonManagerStatus EsfButtonManagerUnsetTimerCallback(void);

// """Verify the notification callback params.

// Verify the notification callback params. When pressed, there are no
// parameters to check, so it always succeeds.

// Returns:
//     min_second (int32_t): This is the starting number of seconds for the
//       button long press time to execute the button notification
//       callback.  This argument is valid only for released and long pressed.
//     max_second (int32_t): This is the number of seconds that the button
//       long press time ends when the button notification callback
//       is executed. This is an argument that is checked only when releasing.
//     type (EsfButtonManagerButtonStateType): The type of button state.

// Returns:
//     EsfButtonManagerStatus: The code returns one of the values
//       EsfButtonManagerStatus depending on the execution result.
//       Returns the value kEsfButtonManagerStatusOk if the process is
//       successful.

// Raises:
//     kEsfButtonManagerStatusParamError: Returns if the time value is invalid.

// """
static EsfButtonManagerStatus EsfButtonManagerVerifyNotificationCallbackParams(
    int32_t min_second, int32_t max_second,
    EsfButtonManagerButtonStateType type);

// """Execute the button callbacks.

// Executes the button notification callback for the corresponding button event.
// In case of release and long pressed, it will be executed only if the
// conditions are met.

// Args:
//     state (EsfButtonManagerButtonStateType): button from received command.
//     handle_data (EsfButtonManagerHandleData *): The callback with the
//       specified handle data is executed.

// """
static void EsfButtonManagerExecuteButtonCallback(
    EsfButtonManagerButtonStateType state,
    EsfButtonManagerHandleData *handle_data);

// """Executes a notification callback that applies to all handle conditions.

// Check all handles registered in the handle list and execute the button
// notification callback.

// Args:
//     button_id (uint32_t): button id from received command.
//     state (EsfButtonManagerButtonStateType): button from received command.

// """
static void EsfButtonManagerExecuteButtonCallbackAllHandles(
    uint32_t button_id, EsfButtonManagerButtonStateType state);

// """Handles preprocess of button events.

// In order to perform preprocessing for handling button events, we will check
// if a button event is to be performed and update the button state.

// Returns:
//     false: In the case of receiving an invalid parameter or when the current
//       state is Unknown and the next state is Released, it will be returned.
//     true: Returned when the false condition is not met.

// Args:
//     command (EsfButtonManagerCommand *): Command information received from
//       the button detection callback.

// """
static bool EsfButtonManagerPreProcessExecuteButtonEvent(
    EsfButtonManagerCommand *command);

// """Handles button events.

// Performs various processes corresponding to the state of the button received
// from the button detection callback. When in the pressed state, it starts the
// timer and registers a callback, and when in the released state, it stops the
// timer and cancels the callback. When pressed for a long time, the pressed
// time is updated. In either case, call
// EsfButtonManagerExecuteButtonCallbackAllHandles() to call the notification
// callback.

// Args:
//     button_data (EsfButtonManagerButtonData *): Button data received from
//       the button detection callback.

// """
static void EsfButtonManagerExecuteButtonEvent(
    EsfButtonManagerButtonData *button_data);

// """cancel remained timers.

// Cancel any remaining timers. This is a countermeasure because it will remain
// if the button is closed while being pressed for a long time.

// """
static void EsfButtonManagerCancelRemainedTimer(void);

// """Informs the thread of pressed or released status.

// This is a callback for button detection that is registered in HAL. When a
// button pressed or release is detected, a message is sent to the button
// detection event processing thread.

// Args:
//     status (PlButtonStatus): This is the state of the button received from
//       HAL.
//     private_data (void *): Store button data in user data to determine from
//       which button the notification was received.

// """
static void EsfButtonManagerButtonDetectionCallback(PlButtonStatus status,
                                                    void *private_data);

// """Informs the thread of long pressed status.

// This is a callback for detecting the passage of time that is registered in
// HAL. When a long button press is detected, a message is sent to the button
// detection event processing thread.

// Args:
//     private_data (void *): Store button data in user data to determine from
//       which button the notification was received.

// """
static void EsfButtonManagerTimerCallback(void *private_data);

// """Processing button detection events.

// This is a thread for processing button detection events. Receives messages
// from the button detection notification function/time elapsed detection
// notification function and executes callbacks for various button
// notifications.

// Args:
//     arg (void *): It is never used.

// """
static void *EsfButtonManagerThread(void *arg);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static EsfButtonManagerStatus EsfButtonManagerCreateButtonDataList(
    const PlButtonInfo *button_info) {
  if (button_info == (PlButtonInfo *)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Invalid parameter. button_info is null.");
    return kEsfButtonManagerStatusParamError;
  }

  static_button_data_list = EsfButtonManagerCreateList();
  if (static_button_data_list == (EsfButtonManagerList *)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Failed to create button data list.");
    return kEsfButtonManagerStatusInternalError;
  }

  for (uint32_t i = 0; i < button_info->button_total_num; i++) {
    EsfButtonManagerButtonData *button_data =
        (EsfButtonManagerButtonData *)malloc(
            sizeof(EsfButtonManagerButtonData));
    if (button_data == (EsfButtonManagerButtonData *)NULL) {
      ESF_BUTTON_MANAGER_ERROR("Malloc error.");
      break;
    }
    button_data->button_id = button_info->button_ids[i];
    button_data->state = kEsfButtonManagerButtonStateTypeUnknown;
    button_data->timer_handle = (UtilityTimerHandle)NULL;
    button_data->is_callback_registered = false;

    EsfButtonManagerCell *button_cell =
        EsfButtonManagerCreateCell((void *)button_data);
    if (button_cell == (EsfButtonManagerCell *)NULL) {
      ESF_BUTTON_MANAGER_ERROR("Failed to create cell.");
      free(button_data);
      break;
    }
    if (EsfButtonManagerPushCell(button_cell, static_button_data_list) !=
        kEsfButtonManagerStatusOk) {
      ESF_BUTTON_MANAGER_ERROR("Failed to push cell.");
      free(button_data);
      EsfButtonManagerDestroyCell(&button_cell);
      break;
    }
  }

  // If create the button list failed, release the memory resource.
  if ((uint32_t)(static_button_data_list->size) !=
      button_info->button_total_num) {
    ESF_BUTTON_MANAGER_ERROR("Operation error ocurred.");
    EsfButtonManagerDestroyButtonDataList();
    return kEsfButtonManagerStatusInternalError;
  }

  return kEsfButtonManagerStatusOk;
}

static EsfButtonManagerStatus EsfButtonManagerDestroyButtonDataList(void) {
  if (static_button_data_list == (EsfButtonManagerList *)NULL) {
    ESF_BUTTON_MANAGER_ERROR(
        "Invalid parameter. static_button_data_list is null.");
    return kEsfButtonManagerStatusParamError;
  }

  int32_t size = static_button_data_list->size;
  for (int32_t i = 0; i < size; i++) {
    EsfButtonManagerCell *button_cell =
        EsfButtonManagerGetFrontCell(static_button_data_list);
    if (button_cell == (EsfButtonManagerCell *)NULL) {
      ESF_BUTTON_MANAGER_ERROR("Failed to get front cell.");
      return kEsfButtonManagerStatusInternalError;
    }

    EsfButtonManagerButtonData *button_data =
        (EsfButtonManagerButtonData *)(button_cell->data);
    if (button_data == (EsfButtonManagerButtonData *)NULL) {
      ESF_BUTTON_MANAGER_ERROR("button_data is null.");
      return kEsfButtonManagerStatusInternalError;
    }

    free(button_data);
    button_data = (EsfButtonManagerButtonData *)NULL;
    if (EsfButtonManagerRemoveCell(&button_cell, static_button_data_list) !=
        kEsfButtonManagerStatusOk) {
      ESF_BUTTON_MANAGER_ERROR("Failed to remove cell.");
      return kEsfButtonManagerStatusInternalError;
    }
  }
  if (EsfButtonManagerDestroyList(&static_button_data_list) !=
      kEsfButtonManagerStatusOk) {
    ESF_BUTTON_MANAGER_ERROR("Failed to destroy list.");
    return kEsfButtonManagerStatusInternalError;
  }

  return kEsfButtonManagerStatusOk;
}

static EsfButtonManagerButtonData *EsfButtonManagerGetButtonData(
    uint32_t button_id) {
  if (static_button_data_list == (EsfButtonManagerList *)NULL) {
    ESF_BUTTON_MANAGER_ERROR(
        "Invalid parameter. static_button_data_list is null.");
    return (EsfButtonManagerButtonData *)NULL;
  }

  EsfButtonManagerCell *button_cell = (EsfButtonManagerCell *)NULL;
  for (int32_t i = 0; i < static_button_data_list->size; i++) {
    if (i == 0) {
      button_cell = EsfButtonManagerGetFrontCell(static_button_data_list);
    } else {
      button_cell = EsfButtonManagerGetNextCell(button_cell);
    }
    if (button_cell == (EsfButtonManagerCell *)NULL) {
      ESF_BUTTON_MANAGER_ERROR("button_cell is null.");
      break;
    }

    EsfButtonManagerButtonData *button_data =
        (EsfButtonManagerButtonData *)(button_cell->data);
    if (button_data == (EsfButtonManagerButtonData *)NULL) {
      ESF_BUTTON_MANAGER_ERROR("button_data is null.");
      break;
    }

    if (button_data->button_id == button_id) {
      // Data associated with the button id was found.
      return button_data;
    }
  }

  // Not found specified the button id.
  return (EsfButtonManagerButtonData *)NULL;
}

static EsfButtonManagerStatus EsfButtonManagerCheckButtonState(
    EsfButtonManagerList *handle_data_list) {
  if (static_button_data_list == (EsfButtonManagerList *)NULL) {
    ESF_BUTTON_MANAGER_ERROR(
        "Invalid parameter. static_button_data_list is null.");
    return kEsfButtonManagerStatusParamError;
  }
  if (handle_data_list == (EsfButtonManagerList *)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Invalid parameter. handle_data_list is null.");
    return kEsfButtonManagerStatusParamError;
  }

  EsfButtonManagerCell *button_cell = (EsfButtonManagerCell *)NULL;
  for (int32_t i = 0; i < static_button_data_list->size; i++) {
    if (i == 0) {
      button_cell = EsfButtonManagerGetFrontCell(static_button_data_list);
    } else {
      button_cell = EsfButtonManagerGetNextCell(button_cell);
    }
    if (button_cell == (EsfButtonManagerCell *)NULL) {
      ESF_BUTTON_MANAGER_ERROR("button_cell is null.");
      return kEsfButtonManagerStatusInternalError;
    }

    EsfButtonManagerButtonData *button_data =
        (EsfButtonManagerButtonData *)(button_cell->data);
    if (button_data == (EsfButtonManagerButtonData *)NULL) {
      ESF_BUTTON_MANAGER_ERROR("button_data is null.");
      return kEsfButtonManagerStatusInternalError;
    }

    if ((button_data->state == kEsfButtonManagerButtonStateTypePressed) ||
        (button_data->state == kEsfButtonManagerButtonStateTypeLongPressed)) {
      EsfButtonManagerHandleData *handle_data = EsfButtonManagerGetHandleData(
          button_data->button_id, handle_data_list);
      if (handle_data != (EsfButtonManagerHandleData *)NULL) {
        EsfButtonManagerExecutePressedCallback(handle_data->pressed_list);
      }
    }
  }

  return kEsfButtonManagerStatusOk;
}

static EsfButtonManagerHandleFrame *EsfButtonManagerCreateHandleFrame(void) {
  EsfButtonManagerHandleFrame *handle_frame =
      (EsfButtonManagerHandleFrame *)malloc(
          sizeof(EsfButtonManagerHandleFrame));
  if (handle_frame == (EsfButtonManagerHandleFrame *)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Malloc error.");
    return (EsfButtonManagerHandleFrame *)NULL;
  }

  handle_frame->handle_data_list = EsfButtonManagerCreateList();
  if (handle_frame->handle_data_list == (EsfButtonManagerList *)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Failed to create list.");
    free(handle_frame);
    return (EsfButtonManagerHandleFrame *)NULL;
  }
  handle_frame->callback_execution =
      kEsfButtonManagerNotificationCallbackStateDisable;

  return handle_frame;
}

static EsfButtonManagerStatus EsfButtonManagerDestroyHandleFrame(
    EsfButtonManagerHandleFrame *handle_frame) {
  if (handle_frame == (EsfButtonManagerHandleFrame *)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Invalid parameter. handle_frame is null.");
    return kEsfButtonManagerStatusParamError;
  }

  if (EsfButtonManagerDestroyHandleDataList(handle_frame->handle_data_list) !=
      kEsfButtonManagerStatusOk) {
    ESF_BUTTON_MANAGER_ERROR("Failed to destroy handle data list.");
    return kEsfButtonManagerStatusInternalError;
  }

  free(handle_frame);
  handle_frame = (EsfButtonManagerHandleFrame *)NULL;

  return kEsfButtonManagerStatusOk;
}

static EsfButtonManagerStatus EsfButtonManagerDestroyHandleDataList(
    EsfButtonManagerList *handle_data_list) {
  if (handle_data_list == (EsfButtonManagerList *)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Invalid parameter. handle_data_list is null.");
    return kEsfButtonManagerStatusParamError;
  }

  int32_t size = handle_data_list->size;
  for (int32_t i = 0; i < size; i++) {
    // clang-format off
    EsfButtonManagerCell *handle_data_cell = EsfButtonManagerGetFrontCell(handle_data_list);
    // clang-format on
    if (handle_data_cell == (EsfButtonManagerCell *)NULL) {
      ESF_BUTTON_MANAGER_ERROR("handle_data_cell is null.");
      return kEsfButtonManagerStatusInternalError;
    }

    EsfButtonManagerHandleData *handle_data =
        (EsfButtonManagerHandleData *)(handle_data_cell->data);
    if (handle_data == (EsfButtonManagerHandleData *)NULL) {
      ESF_BUTTON_MANAGER_ERROR("handle_data is null.");
      return kEsfButtonManagerStatusInternalError;
    }

    if (EsfButtonManagerDestroyHandleData(handle_data) !=
        kEsfButtonManagerStatusOk) {
      ESF_BUTTON_MANAGER_ERROR("Failed to destroy handle data.");
      return kEsfButtonManagerStatusInternalError;
    }
    if (EsfButtonManagerRemoveCell(&handle_data_cell, handle_data_list) !=
        kEsfButtonManagerStatusOk) {
      ESF_BUTTON_MANAGER_ERROR("Failed to remove cell.");
      return kEsfButtonManagerStatusInternalError;
    }
  }
  if (EsfButtonManagerDestroyList(&handle_data_list) !=
      kEsfButtonManagerStatusOk) {
    ESF_BUTTON_MANAGER_ERROR("Failed to destroy list.");
    return kEsfButtonManagerStatusInternalError;
  }

  return kEsfButtonManagerStatusOk;
}

static EsfButtonManagerHandleData *EsfButtonManagerCreateHandleData(
    uint32_t button_id) {
  // clang-format off
  EsfButtonManagerHandleData *handle_data = (EsfButtonManagerHandleData *)malloc(sizeof(EsfButtonManagerHandleData));
  // clang-format on
  if (handle_data == (EsfButtonManagerHandleData *)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Malloc error.");
    return (EsfButtonManagerHandleData *)NULL;
  }

  handle_data->button_id = button_id;
  handle_data->long_pressed_time = 0;
  handle_data->pressed_list = EsfButtonManagerCreateList();
  handle_data->released_list = EsfButtonManagerCreateList();
  handle_data->long_pressed_list = EsfButtonManagerCreateList();

  if ((handle_data->pressed_list == (EsfButtonManagerList *)NULL) ||
      (handle_data->released_list == (EsfButtonManagerList *)NULL) ||
      (handle_data->long_pressed_list == (EsfButtonManagerList *)NULL)) {
    ESF_BUTTON_MANAGER_ERROR("Failed to create lists.");
    EsfButtonManagerDestroyList(&handle_data->pressed_list);
    EsfButtonManagerDestroyList(&handle_data->released_list);
    EsfButtonManagerDestroyList(&handle_data->long_pressed_list);
    free(handle_data);
    return (EsfButtonManagerHandleData *)NULL;
  }

  return handle_data;
}

static EsfButtonManagerStatus EsfButtonManagerDestroyHandleData(
    EsfButtonManagerHandleData *handle_data) {
  if (handle_data == (EsfButtonManagerHandleData *)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Invalid parameter. handle_data is null.");
    return kEsfButtonManagerStatusParamError;
  }

  if (EsfButtonManagerDestroyCallbackInfoList(handle_data->pressed_list) !=
      kEsfButtonManagerStatusOk) {
    ESF_BUTTON_MANAGER_ERROR("Failed to destroy pressed callback info list.");
    return kEsfButtonManagerStatusInternalError;
  }

  if (EsfButtonManagerDestroyCallbackInfoList(handle_data->released_list) !=
      kEsfButtonManagerStatusOk) {
    ESF_BUTTON_MANAGER_ERROR("Failed to destroy released callback info list.");
    return kEsfButtonManagerStatusInternalError;
  }

  if (EsfButtonManagerDestroyCallbackInfoList(handle_data->long_pressed_list) !=
      kEsfButtonManagerStatusOk) {
    ESF_BUTTON_MANAGER_ERROR(
        "Failed to destroy long pressed callback info list.");
    return kEsfButtonManagerStatusInternalError;
  }

  free(handle_data);
  handle_data = (EsfButtonManagerHandleData *)NULL;

  return kEsfButtonManagerStatusOk;
}

static EsfButtonManagerHandleData *EsfButtonManagerGetHandleData(
    uint32_t button_id, const EsfButtonManagerList *handle_data_list) {
  if (handle_data_list == (EsfButtonManagerList *)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Invalid parameter. handle_data_list is null.");
    return (EsfButtonManagerHandleData *)NULL;
  }

  EsfButtonManagerCell *handle_data_cell = (EsfButtonManagerCell *)NULL;
  for (int32_t i = 0; i < handle_data_list->size; i++) {
    if (i == 0) {
      handle_data_cell = EsfButtonManagerGetFrontCell(handle_data_list);
    } else {
      handle_data_cell = EsfButtonManagerGetNextCell(handle_data_cell);
    }
    if (handle_data_cell == (EsfButtonManagerCell *)NULL) {
      ESF_BUTTON_MANAGER_ERROR("handle_data_cell is null");
      break;
    }

    EsfButtonManagerHandleData *handle_data =
        (EsfButtonManagerHandleData *)(handle_data_cell->data);
    if (handle_data == (EsfButtonManagerHandleData *)NULL) {
      ESF_BUTTON_MANAGER_ERROR("handle_data is null");
      break;
    }

    if (handle_data->button_id == button_id) {
      // Data associated with the button id was found.
      return handle_data;
    }
  }

  // Not found specified the button id.
  return (EsfButtonManagerHandleData *)NULL;
}

static EsfButtonManagerHandleData *EsfButtonManagerGetOrPrepareNewHandleData(
    uint32_t button_id, EsfButtonManagerList *handle_data_list) {
  if (handle_data_list == (EsfButtonManagerList *)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Invalid parameter. handle_data_list is null.");
    return (EsfButtonManagerHandleData *)NULL;
  }

  // Once the data is obtained, return without pushing.
  // clang-format off
  EsfButtonManagerHandleData *handle_data = EsfButtonManagerGetHandleData(button_id, handle_data_list);
  // clang-format on
  if (handle_data == (EsfButtonManagerHandleData *)NULL) {
    handle_data = EsfButtonManagerCreateHandleData(button_id);
    if (handle_data == (EsfButtonManagerHandleData *)NULL) {
      ESF_BUTTON_MANAGER_ERROR("Failed to create handle data.");
      return (EsfButtonManagerHandleData *)NULL;
    }

    // clang-format off
    EsfButtonManagerCell *handle_data_cell = EsfButtonManagerCreateCell((void *)handle_data);
    // clang-format on
    if (handle_data_cell == (EsfButtonManagerCell *)NULL) {
      ESF_BUTTON_MANAGER_ERROR("Failed to create cell.");
      EsfButtonManagerDestroyHandleData(handle_data);
      return (EsfButtonManagerHandleData *)NULL;
    }
    if (EsfButtonManagerPushCell(handle_data_cell, handle_data_list) !=
        kEsfButtonManagerStatusOk) {
      ESF_BUTTON_MANAGER_ERROR("Failed to push cell.");
      EsfButtonManagerDestroyHandleData(handle_data);
      EsfButtonManagerDestroyCell(&handle_data_cell);
      return (EsfButtonManagerHandleData *)NULL;
    }
  }

  return handle_data;
}

static EsfButtonManagerStatus EsfButtonManagerRemoveHandleData(
    uint32_t button_id, EsfButtonManagerList *handle_data_list) {
  if (handle_data_list == (EsfButtonManagerList *)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Invalid parameter. handle_data_list is null.");
    return kEsfButtonManagerStatusParamError;
  }

  EsfButtonManagerCell *handle_data_cell = (EsfButtonManagerCell *)NULL;
  for (int32_t i = 0; i < handle_data_list->size; i++) {
    if (i == 0) {
      handle_data_cell = EsfButtonManagerGetFrontCell(handle_data_list);
    } else {
      handle_data_cell = EsfButtonManagerGetNextCell(handle_data_cell);
    }
    if (handle_data_cell == (EsfButtonManagerCell *)NULL) {
      ESF_BUTTON_MANAGER_ERROR("handle_data_cell is null.");
      return kEsfButtonManagerStatusInternalError;
    }

    EsfButtonManagerHandleData *handle_data =
        (EsfButtonManagerHandleData *)(handle_data_cell->data);
    if (handle_data == (EsfButtonManagerHandleData *)NULL) {
      ESF_BUTTON_MANAGER_ERROR("handle_data is null.");
      return kEsfButtonManagerStatusInternalError;
    }

    if (handle_data->button_id == button_id) {
      if (EsfButtonManagerDestroyHandleData(handle_data) !=
          kEsfButtonManagerStatusOk) {
        ESF_BUTTON_MANAGER_ERROR("Failed to destroy handle data.");
        return kEsfButtonManagerStatusInternalError;
      }
      if (EsfButtonManagerRemoveCell(&handle_data_cell, handle_data_list) !=
          kEsfButtonManagerStatusOk) {
        ESF_BUTTON_MANAGER_ERROR("Failed to remove cell.");
        return kEsfButtonManagerStatusInternalError;
      }
      return kEsfButtonManagerStatusOk;
    }
  }

  return kEsfButtonManagerStatusParamError;
}

static EsfButtonManagerStatus EsfButtonManagerResetLongPressedTime(
    EsfButtonManagerList *handle_data_list) {
  if (handle_data_list == (EsfButtonManagerList *)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Invalid parameter. handle_data_list is null.");
    return kEsfButtonManagerStatusParamError;
  }

  EsfButtonManagerCell *handle_data_cell = (EsfButtonManagerCell *)NULL;
  for (int32_t i = 0; i < handle_data_list->size; i++) {
    if (i == 0) {
      handle_data_cell = EsfButtonManagerGetFrontCell(handle_data_list);
    } else {
      handle_data_cell = EsfButtonManagerGetNextCell(handle_data_cell);
    }
    if (handle_data_cell == (EsfButtonManagerCell *)NULL) {
      ESF_BUTTON_MANAGER_ERROR("handle_data_cell is null");
      return kEsfButtonManagerStatusInternalError;
    }

    EsfButtonManagerHandleData *handle_data =
        (EsfButtonManagerHandleData *)(handle_data_cell->data);
    if (handle_data == (EsfButtonManagerHandleData *)NULL) {
      ESF_BUTTON_MANAGER_ERROR("handle_data is null");
      return kEsfButtonManagerStatusInternalError;
    }

    handle_data->long_pressed_time = 0;
  }

  return kEsfButtonManagerStatusOk;
}

static EsfButtonManagerStatus EsfButtonManagerDestroyCallbackInfoList(
    EsfButtonManagerList *callback_info_list) {
  if (callback_info_list == (EsfButtonManagerList *)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Invalid parameter. callback_info_list is null.");
    return kEsfButtonManagerStatusParamError;
  }

  if (EsfButtonManagerRemoveAllCallbackInfo(callback_info_list) !=
      kEsfButtonManagerStatusOk) {
    ESF_BUTTON_MANAGER_ERROR("Failed to remove all callback info.");
    return kEsfButtonManagerStatusInternalError;
  }
  if (EsfButtonManagerDestroyList(&callback_info_list) !=
      kEsfButtonManagerStatusOk) {
    ESF_BUTTON_MANAGER_ERROR("Failed to destroy list.");
    return kEsfButtonManagerStatusInternalError;
  }

  return kEsfButtonManagerStatusOk;
}

static EsfButtonManagerStatus EsfButtonManagerRegisterPressedCallbackInfo(
    const EsfButtonManagerCallback callback, void *user_data,
    EsfButtonManagerList *pressed_info_list) {
  // clang-format off
  if ((callback == (EsfButtonManagerCallback)NULL) || (pressed_info_list == (EsfButtonManagerList *)NULL)) {
    // clang-format on
    ESF_BUTTON_MANAGER_ERROR(
        "Invalid parameter. Callback is null or pressed_info_list is null. "
        "callback=%p pressed_info_list=%p",
        callback, pressed_info_list);
    return kEsfButtonManagerStatusParamError;
  }
  if (pressed_info_list->size >=
      CONFIG_ESF_BUTTON_MANAGER_NOTIFICATION_CALLBACK_MAX_NUM) {
    ESF_BUTTON_MANAGER_ERROR(
        "Resource error. The number of registered callbacks has reached the "
        "limit.");
    return kEsfButtonManagerStatusResourceError;
  }

  // clang-format off
  EsfButtonManagerButtonPressedCallbackInfo *pressed_info = (EsfButtonManagerButtonPressedCallbackInfo *)malloc( sizeof(EsfButtonManagerButtonPressedCallbackInfo));
  // clang-format on
  if (pressed_info == (EsfButtonManagerButtonPressedCallbackInfo *)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Malloc error.");
    return kEsfButtonManagerStatusInternalError;
  }
  pressed_info->callback = callback;
  pressed_info->user_data = user_data;

  // clang-format off
  EsfButtonManagerCell *pressed_info_cell = EsfButtonManagerCreateCell(pressed_info);
  // clang-format on
  if (pressed_info_cell == (EsfButtonManagerCell *)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Failed to create cell.");
    free(pressed_info);
    return kEsfButtonManagerStatusInternalError;
  }
  if (EsfButtonManagerPushCell(pressed_info_cell, pressed_info_list) !=
      kEsfButtonManagerStatusOk) {
    ESF_BUTTON_MANAGER_ERROR("Failed to push cell.");
    free(pressed_info);
    EsfButtonManagerDestroyCell(&pressed_info_cell);
    return kEsfButtonManagerStatusInternalError;
  }

  return kEsfButtonManagerStatusOk;
}

static EsfButtonManagerStatus EsfButtonManagerRegisterReleasedCallbackInfo(
    int32_t min_second, int32_t max_second,
    const EsfButtonManagerCallback callback, void *user_data,
    EsfButtonManagerList *released_info_list) {
  // clang-format off
  if ((callback == (EsfButtonManagerCallback)NULL) || (released_info_list == (EsfButtonManagerList *)NULL)) {
    // clang-format on
    ESF_BUTTON_MANAGER_ERROR(
        "Invalid parameter. Callback is null or released_info_list is null. "
        "callback=%p released_info_list=%p",
        callback, released_info_list);
    return kEsfButtonManagerStatusParamError;
  }
  if (released_info_list->size >=
      CONFIG_ESF_BUTTON_MANAGER_NOTIFICATION_CALLBACK_MAX_NUM) {
    ESF_BUTTON_MANAGER_ERROR(
        "Resource error. The number of registered callbacks has reached the "
        "limit.");
    return kEsfButtonManagerStatusResourceError;
  }

  // clang-format off
  EsfButtonManagerButtonReleasedCallbackInfo *released_info = (EsfButtonManagerButtonReleasedCallbackInfo *)malloc(sizeof(EsfButtonManagerButtonReleasedCallbackInfo));
  // clang-format on
  if (released_info == (EsfButtonManagerButtonReleasedCallbackInfo *)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Malloc error.");
    return kEsfButtonManagerStatusInternalError;
  }
  released_info->callback = callback;
  released_info->user_data = user_data;
  released_info->min_second = min_second;
  released_info->max_second = max_second;

  // clang-format off
  EsfButtonManagerCell *released_info_cell = EsfButtonManagerCreateCell(released_info);
  // clang-format on
  if (released_info_cell == (EsfButtonManagerCell *)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Failed to create cell.");
    free(released_info);
    return kEsfButtonManagerStatusInternalError;
  }
  if (EsfButtonManagerPushCell(released_info_cell, released_info_list) !=
      kEsfButtonManagerStatusOk) {
    ESF_BUTTON_MANAGER_ERROR("Failed to push cell.");
    free(released_info);
    EsfButtonManagerDestroyCell(&released_info_cell);
    return kEsfButtonManagerStatusInternalError;
  }

  return kEsfButtonManagerStatusOk;
}

static EsfButtonManagerStatus EsfButtonManagerRegisterLongPressedCallbackInfo(
    int32_t second, const EsfButtonManagerCallback callback, void *user_data,
    EsfButtonManagerList *long_pressed_info_list) {
  // clang-format off
  if ((callback == (EsfButtonManagerCallback)NULL) || (long_pressed_info_list == (EsfButtonManagerList *)NULL)) {
    // clang-format on
    ESF_BUTTON_MANAGER_ERROR(
        "Invalid parameter. Callback is null or long_pressed_info_list is "
        "null. callback=%p long_pressed_info_list=%p",
        callback, long_pressed_info_list);
    return kEsfButtonManagerStatusParamError;
  }
  if (long_pressed_info_list->size >=
      CONFIG_ESF_BUTTON_MANAGER_NOTIFICATION_CALLBACK_MAX_NUM) {
    ESF_BUTTON_MANAGER_ERROR(
        "Resource error. The number of registered callbacks has reached the "
        "limit.");
    return kEsfButtonManagerStatusResourceError;
  }

  // clang-format off
  EsfButtonManagerButtonLongPressedCallbackInfo *long_pressed_info = (EsfButtonManagerButtonLongPressedCallbackInfo *)malloc(sizeof(EsfButtonManagerButtonLongPressedCallbackInfo));
  // clang-format on
  if (long_pressed_info ==
      (EsfButtonManagerButtonLongPressedCallbackInfo *)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Malloc error.");
    return kEsfButtonManagerStatusInternalError;
  }
  long_pressed_info->callback = callback;
  long_pressed_info->user_data = user_data;
  long_pressed_info->second = second;

  // clang-format off
  EsfButtonManagerCell *long_pressed_info_cell = EsfButtonManagerCreateCell(long_pressed_info);
  // clang-format on
  if (long_pressed_info_cell == (EsfButtonManagerCell *)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Failed to create cell.");
    free(long_pressed_info);
    return kEsfButtonManagerStatusInternalError;
  }
  if (EsfButtonManagerPushCell(long_pressed_info_cell,
                               long_pressed_info_list) !=
      kEsfButtonManagerStatusOk) {
    ESF_BUTTON_MANAGER_ERROR("Failed to push cell.");
    free(long_pressed_info);
    EsfButtonManagerDestroyCell(&long_pressed_info_cell);
    return kEsfButtonManagerStatusInternalError;
  }

  return kEsfButtonManagerStatusOk;
}

static EsfButtonManagerStatus
EsfButtonManagerRemoveAllCallbackInfoFromSpecifyList(
    EsfButtonManagerButtonStateType type,
    EsfButtonManagerHandleData *handle_data) {
  if (handle_data == (EsfButtonManagerHandleData *)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Invalid parameter. handle_data is null.");
    return kEsfButtonManagerStatusParamError;
  }

  EsfButtonManagerList *list = (EsfButtonManagerList *)NULL;
  if (type == kEsfButtonManagerButtonStateTypePressed) {
    list = handle_data->pressed_list;
  } else if (type == kEsfButtonManagerButtonStateTypeReleased) {
    list = handle_data->released_list;
  } else if (type == kEsfButtonManagerButtonStateTypeLongPressed) {
    list = handle_data->long_pressed_list;
  }
  if (list == (EsfButtonManagerList *)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Invalid button state type.");
    return kEsfButtonManagerStatusInternalError;
  }
  if (list->size == 0) {
    ESF_BUTTON_MANAGER_ERROR("Resource error. list size is 0.");
    return kEsfButtonManagerStatusResourceError;
  }
  if (EsfButtonManagerRemoveAllCallbackInfo(list) !=
      kEsfButtonManagerStatusOk) {
    ESF_BUTTON_MANAGER_ERROR("Failed to remove all callback info.");
    return kEsfButtonManagerStatusInternalError;
  }

  return kEsfButtonManagerStatusOk;
}

static EsfButtonManagerStatus EsfButtonManagerRemoveAllCallbackInfo(
    EsfButtonManagerList *callback_info_list) {
  if (callback_info_list == (EsfButtonManagerList *)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Invalid parameter. callback_info_list is null.");
    return kEsfButtonManagerStatusParamError;
  }

  int32_t size = callback_info_list->size;
  for (int32_t i = 0; i < size; i++) {
    // clang-format off
    EsfButtonManagerCell *callback_info_cell = EsfButtonManagerGetFrontCell(callback_info_list);
    // clang-format on
    if (callback_info_cell == (EsfButtonManagerCell *)NULL) {
      ESF_BUTTON_MANAGER_ERROR("Failed to get front cell.");
      return kEsfButtonManagerStatusInternalError;
    }

    void *callback_info = callback_info_cell->data;
    if (callback_info == (void *)NULL) {
      ESF_BUTTON_MANAGER_ERROR("callback_info is null.");
      return kEsfButtonManagerStatusInternalError;
    }

    free(callback_info);
    callback_info = (void *)NULL;
    if (EsfButtonManagerRemoveCell(&callback_info_cell, callback_info_list) !=
        kEsfButtonManagerStatusOk) {
      ESF_BUTTON_MANAGER_ERROR("Failed to remove call.");
      return kEsfButtonManagerStatusInternalError;
    }
  }

  return kEsfButtonManagerStatusOk;
}

static void EsfButtonManagerExecutePressedCallback(
    EsfButtonManagerList *pressed_info_list) {
  if (pressed_info_list == (EsfButtonManagerList *)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Invalid parameter. pressed_info_list is null.");
    return;
  }

  EsfButtonManagerCell *pressed_info_cell = (EsfButtonManagerCell *)NULL;
  for (int32_t i = 0; i < pressed_info_list->size; i++) {
    if (i == 0) {
      pressed_info_cell = EsfButtonManagerGetFrontCell(pressed_info_list);
    } else {
      pressed_info_cell = EsfButtonManagerGetNextCell(pressed_info_cell);
    }
    if (pressed_info_cell == (EsfButtonManagerCell *)NULL) {
      ESF_BUTTON_MANAGER_ERROR("pressed_info_cell is null.");
      return;
    }

    EsfButtonManagerButtonPressedCallbackInfo *pressed_info =
        (EsfButtonManagerButtonPressedCallbackInfo *)(pressed_info_cell->data);
    if (pressed_info == (EsfButtonManagerButtonPressedCallbackInfo *)NULL) {
      ESF_BUTTON_MANAGER_ERROR("pressed_info is null.");
      return;
    }

    pressed_info->callback(pressed_info->user_data);
  }

  return;
}

static void EsfButtonManagerExecuteReleasedCallback(
    int32_t long_pressed_time, EsfButtonManagerList *released_info_list) {
  if (released_info_list == (EsfButtonManagerList *)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Invalid parameter. released_info_list is null.");
    return;
  }

  EsfButtonManagerCell *released_info_cell = (EsfButtonManagerCell *)NULL;
  for (int32_t i = 0; i < released_info_list->size; i++) {
    if (i == 0) {
      released_info_cell = EsfButtonManagerGetFrontCell(released_info_list);
    } else {
      released_info_cell = EsfButtonManagerGetNextCell(released_info_cell);
    }
    if (released_info_cell == (EsfButtonManagerCell *)NULL) {
      ESF_BUTTON_MANAGER_ERROR("released_info_cell is null.");
      return;
    }

    EsfButtonManagerButtonReleasedCallbackInfo *released_info =
        (EsfButtonManagerButtonReleasedCallbackInfo *)(released_info_cell
                                                           ->data);
    if (released_info == (EsfButtonManagerButtonReleasedCallbackInfo *)NULL) {
      ESF_BUTTON_MANAGER_ERROR("released_info is null.");
      return;
    }

    if (((released_info->min_second == 0) &&
         (released_info->max_second == 0)) ||
        ((released_info->min_second <= long_pressed_time) &&
         (released_info->max_second == 0)) ||
        ((released_info->min_second <= long_pressed_time) &&
         (long_pressed_time <= released_info->max_second))) {
      released_info->callback(released_info->user_data);
    }
  }

  return;
}

static void EsfButtonManagerExecuteLongPressedCallback(
    int32_t long_pressed_time, EsfButtonManagerList *long_pressed_info_list) {
  if (long_pressed_info_list == (EsfButtonManagerList *)NULL) {
    ESF_BUTTON_MANAGER_ERROR(
        "Invalid parameter. long_pressed_info_list is null.");
    return;
  }

  EsfButtonManagerCell *long_pressed_info_cell = (EsfButtonManagerCell *)NULL;
  for (int32_t i = 0; i < long_pressed_info_list->size; i++) {
    if (i == 0) {
      // clang-format off
      long_pressed_info_cell = EsfButtonManagerGetFrontCell(long_pressed_info_list);
      // clang-format on
    } else {
      // clang-format off
      long_pressed_info_cell = EsfButtonManagerGetNextCell(long_pressed_info_cell);
      // clang-format on
    }
    if (long_pressed_info_cell == (EsfButtonManagerCell *)NULL) {
      ESF_BUTTON_MANAGER_ERROR("long_pressed_info_cell is null.");
      return;
    }

    EsfButtonManagerButtonLongPressedCallbackInfo *long_pressed_info =
        (EsfButtonManagerButtonLongPressedCallbackInfo *)(long_pressed_info_cell
                                                              ->data);
    if (long_pressed_info ==
        (EsfButtonManagerButtonLongPressedCallbackInfo *)NULL) {
      ESF_BUTTON_MANAGER_ERROR("long_pressed_info is null.");
      return;
    }

    if (long_pressed_info->second == long_pressed_time) {
      long_pressed_info->callback(long_pressed_info->user_data);
    }
  }

  return;
}

static EsfButtonManagerStatus EsfButtonManagerCreateThread(void) {
  EsfButtonManagerStatus result = kEsfButtonManagerStatusInternalError;
  bool attr_check = false;

  pthread_attr_t button_manager_thread_attr = {0};
  if (pthread_attr_init(&button_manager_thread_attr) != 0) {
    ESF_BUTTON_MANAGER_ERROR("Failed to pthread_attr_init.");
    goto finally;
  }
  attr_check = true;

  static_button_manager_thread_stack = malloc(BUTTON_MANAGER_THREAD_STACK_SIZE);
  if (static_button_manager_thread_stack == (void *)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Malloc error.");
    goto finally;
  }

  if (pthread_attr_setstack(&button_manager_thread_attr,
                            static_button_manager_thread_stack,
                            BUTTON_MANAGER_THREAD_STACK_SIZE) != 0) {
    ESF_BUTTON_MANAGER_ERROR("Failed to pthread_attr_setstack.");
    goto finally;
  }

  if (pthread_create(&static_button_manager_thread, &button_manager_thread_attr,
                     EsfButtonManagerThread, (void *)NULL) != 0) {
    ESF_BUTTON_MANAGER_ERROR("Failed to pthread_create.");
    goto finally;
  }
  static_is_thread_starting = true;

  result = kEsfButtonManagerStatusOk;

finally:
  if (attr_check == true) {
    if (pthread_attr_destroy(&button_manager_thread_attr) != 0) {
      ESF_BUTTON_MANAGER_ERROR("Failed to pthread_attr_destroy.");
      result = kEsfButtonManagerStatusInternalError;
      if (static_is_thread_starting != false) {
        EsfButtonManagerDestroyThread();
      }
    }
    if (result != kEsfButtonManagerStatusOk) {
      free(static_button_manager_thread_stack);
      static_button_manager_thread_stack = (void *)NULL;
    }
  }

  return result;
}

static EsfButtonManagerStatus EsfButtonManagerDestroyThread(void) {
  EsfButtonManagerCommand command = BUTTON_MANAGER_COMMAND_INITIALIZER;
  command.type = kEsfButtonManagerCommandTypeCloseThread;
  int32_t sent_size = 0;
  if (UtilityMsgSend(static_button_manager_message_handle, (void *)&command,
                     sizeof(command), BUTTON_MANAGER_MESSAGE_PRIORITY,
                     &sent_size) != kUtilityMsgOk) {
    ESF_BUTTON_MANAGER_ERROR("Failed to UtilityMsgSend.");
    return kEsfButtonManagerStatusInternalError;
  }
  if (pthread_join(static_button_manager_thread, (void **)NULL) != 0) {
    ESF_BUTTON_MANAGER_ERROR("Failed to pthread_join.");
    return kEsfButtonManagerStatusInternalError;
  }
  static_is_thread_starting = false;

  return kEsfButtonManagerStatusOk;
}

static EsfButtonManagerStatus EsfButtonManagerSetButtonDetectionCallback(
    const PlButtonInfo *button_info) {
  if (button_info == (PlButtonInfo *)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Invalid parameter. button_info is null.");
    return kEsfButtonManagerStatusParamError;
  }
  if (static_button_data_list == (EsfButtonManagerList *)NULL) {
    ESF_BUTTON_MANAGER_ERROR(
        "Invalid parameter. static_button_data_list is null.");
    return kEsfButtonManagerStatusParamError;
  }

  // Set the button callback function to Button.
  for (int32_t i = 0; i < static_button_data_list->size; i++) {
    EsfButtonManagerButtonData *button_data =
        EsfButtonManagerGetButtonData(button_info->button_ids[i]);
    if (button_data == (EsfButtonManagerButtonData *)NULL) {
      ESF_BUTTON_MANAGER_ERROR("Failed to get button data.");
      // Unregister the already register handler and destroy the button data
      // list.
      EsfButtonManagerUnsetButtonDetectionCallback();
      return kEsfButtonManagerStatusInternalError;
    }

    if (PlButtonRegisterHandler(button_data->button_id,
                                EsfButtonManagerButtonDetectionCallback,
                                (void *)(button_data)) != kPlErrCodeOk) {
      ESF_BUTTON_MANAGER_ERROR("Failed to PlButtonRegisterHandler.");
      EsfButtonManagerUnsetButtonDetectionCallback();
      return kEsfButtonManagerStatusInternalError;
    } else {
      button_data->is_callback_registered = true;
    }
  }

  return kEsfButtonManagerStatusOk;
}

static EsfButtonManagerStatus EsfButtonManagerUnsetButtonDetectionCallback(
    void) {
  if (static_button_data_list == (EsfButtonManagerList *)NULL) {
    ESF_BUTTON_MANAGER_ERROR(
        "Invalid parameter. static_button_data_list is null.");
    return kEsfButtonManagerStatusParamError;
  }

  EsfButtonManagerCell *button_cell = (EsfButtonManagerCell *)NULL;
  for (int32_t i = 0; i < static_button_data_list->size; i++) {
    if (i == 0) {
      button_cell = EsfButtonManagerGetFrontCell(static_button_data_list);
    } else {
      button_cell = EsfButtonManagerGetNextCell(button_cell);
    }
    if (button_cell == (EsfButtonManagerCell *)NULL) {
      ESF_BUTTON_MANAGER_ERROR("button_cell is null.");
      return kEsfButtonManagerStatusInternalError;
    }

    EsfButtonManagerButtonData *button_data =
        (EsfButtonManagerButtonData *)(button_cell->data);
    if (button_data == (EsfButtonManagerButtonData *)NULL) {
      ESF_BUTTON_MANAGER_ERROR("button_data is null.");
      return kEsfButtonManagerStatusInternalError;
    }

    if (button_data->is_callback_registered == true) {
      if (PlButtonUnregisterHandler(button_data->button_id) != kPlErrCodeOk) {
        ESF_BUTTON_MANAGER_ERROR("Failed to PlButtonUnregisterHandler.");
        return kEsfButtonManagerStatusInternalError;
      }
    }
  }

  return kEsfButtonManagerStatusOk;
}

static EsfButtonManagerStatus EsfButtonManagerSetTimerCallback(
    const PlButtonInfo *button_info) {
  if (button_info == (PlButtonInfo *)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Invalid parameter. button_info is null.");
    return kEsfButtonManagerStatusParamError;
  }
  if (static_button_data_list == (EsfButtonManagerList *)NULL) {
    ESF_BUTTON_MANAGER_ERROR(
        "Invalid parameter. static_button_data_list is null.");
    return kEsfButtonManagerStatusParamError;
  }

  // Set the button callback function to Button.
  for (int32_t i = 0; i < static_button_data_list->size; i++) {
    EsfButtonManagerButtonData *button_data =
        EsfButtonManagerGetButtonData(button_info->button_ids[i]);
    if (button_data == (EsfButtonManagerButtonData *)NULL) {
      ESF_BUTTON_MANAGER_ERROR("Failed to get button data.");
      EsfButtonManagerUnsetTimerCallback();
      return kEsfButtonManagerStatusInternalError;
    }

    if (UtilityTimerCreate(EsfButtonManagerTimerCallback, (void *)(button_data),
                           &button_data->timer_handle) != kUtilityTimerOk) {
      ESF_BUTTON_MANAGER_ERROR("Failed to UtilityTimerCreate.");
      EsfButtonManagerUnsetTimerCallback();
      return kEsfButtonManagerStatusInternalError;
    }
  }

  return kEsfButtonManagerStatusOk;
}

static EsfButtonManagerStatus EsfButtonManagerUnsetTimerCallback(void) {
  if (static_button_data_list == (EsfButtonManagerList *)NULL) {
    ESF_BUTTON_MANAGER_ERROR(
        "Invalid parameter. static_button_data_list is null.");
    return kEsfButtonManagerStatusParamError;
  }

  EsfButtonManagerCell *button_cell = (EsfButtonManagerCell *)NULL;
  for (int32_t i = 0; i < static_button_data_list->size; i++) {
    if (i == 0) {
      button_cell = EsfButtonManagerGetFrontCell(static_button_data_list);
    } else {
      button_cell = EsfButtonManagerGetNextCell(button_cell);
    }
    if (button_cell == (EsfButtonManagerCell *)NULL) {
      ESF_BUTTON_MANAGER_ERROR("button_cell is null.");
      return kEsfButtonManagerStatusInternalError;
    }

    EsfButtonManagerButtonData *button_data =
        (EsfButtonManagerButtonData *)(button_cell->data);
    if (button_data == (EsfButtonManagerButtonData *)NULL) {
      ESF_BUTTON_MANAGER_ERROR("button_data is null.");
      return kEsfButtonManagerStatusInternalError;
    }

    if (button_data->timer_handle != (UtilityTimerHandle)NULL) {
      if (UtilityTimerDelete(button_data->timer_handle) != kUtilityTimerOk) {
        ESF_BUTTON_MANAGER_ERROR("Failed to UtilityTimerDelete.");
        return kEsfButtonManagerStatusInternalError;
      }
      button_data->timer_handle = (UtilityTimerHandle)NULL;
    }
  }

  return kEsfButtonManagerStatusOk;
}

static EsfButtonManagerStatus EsfButtonManagerVerifyNotificationCallbackParams(
    int32_t min_second, int32_t max_second,
    EsfButtonManagerButtonStateType type) {
  switch (type) {
    case kEsfButtonManagerButtonStateTypePressed:
      // There are no parameters to check.
      break;

    case kEsfButtonManagerButtonStateTypeReleased:
      if (((min_second < 0) ||
           (BUTTON_MANAGER_MAX_START_SECOND < min_second)) ||
          ((max_second < 0) || (BUTTON_MANAGER_MAX_END_SECOND < max_second))) {
        return kEsfButtonManagerStatusParamError;
      }

      if ((max_second != 0) && (min_second > max_second)) {
        return kEsfButtonManagerStatusParamError;
      }
      break;

    case kEsfButtonManagerButtonStateTypeLongPressed:
      if ((min_second <= 0) ||
          (BUTTON_MANAGER_MAX_LONG_PRESSED_SECOND < min_second)) {
        return kEsfButtonManagerStatusParamError;
      }
      break;

    default:
      ESF_BUTTON_MANAGER_ERROR("Invalid button state type.");
      // There is no need to break immediately for a quick return.
      return kEsfButtonManagerStatusInternalError;
  }

  return kEsfButtonManagerStatusOk;
}

static void EsfButtonManagerExecuteButtonCallback(
    EsfButtonManagerButtonStateType state,
    EsfButtonManagerHandleData *handle_data) {
  if (handle_data == (EsfButtonManagerHandleData *)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Invalid parameter. handle_data is null.");
    return;
  }

  switch (state) {
    case kEsfButtonManagerButtonStateTypePressed:
      EsfButtonManagerExecutePressedCallback(handle_data->pressed_list);
      break;

    case kEsfButtonManagerButtonStateTypeReleased:
      EsfButtonManagerExecuteReleasedCallback(handle_data->long_pressed_time,
                                              handle_data->released_list);
      handle_data->long_pressed_time = 0;
      break;

    case kEsfButtonManagerButtonStateTypeLongPressed:
      ++handle_data->long_pressed_time;
      EsfButtonManagerExecuteLongPressedCallback(
          handle_data->long_pressed_time, handle_data->long_pressed_list);
      break;

    default:
      ESF_BUTTON_MANAGER_ERROR("Invalid button state type.");
      break;
  }

  return;
}

static void EsfButtonManagerExecuteButtonCallbackAllHandles(
    uint32_t button_id, EsfButtonManagerButtonStateType state) {
  if (pthread_mutex_lock(&static_button_manager_mutex) != 0) {
    ESF_BUTTON_MANAGER_ERROR("Mutex lock error.");
    return;
  }
  if (static_handle_list == (EsfButtonManagerList *)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Invalid parameter. static_handle_list is null.");
    pthread_mutex_unlock(&static_button_manager_mutex);
    return;
  }

  // Check all registered handles. Execute the callback if the specified
  // button id exists.
  EsfButtonManagerCell *handle_cell = (EsfButtonManagerCell *)NULL;
  for (int32_t i = 0; i < static_handle_list->size; i++) {
    if (i == 0) {
      handle_cell = EsfButtonManagerGetFrontCell(static_handle_list);
    } else {
      handle_cell = EsfButtonManagerGetNextCell(handle_cell);
    }
    if (handle_cell == (EsfButtonManagerCell *)NULL) {
      ESF_BUTTON_MANAGER_ERROR("handle_cell is null.");
      break;
    }

    EsfButtonManagerHandleFrame *handle_frame =
        (EsfButtonManagerHandleFrame *)(handle_cell->data);
    if (handle_frame == (EsfButtonManagerHandleFrame *)NULL) {
      ESF_BUTTON_MANAGER_ERROR("handle_frame is null.");
      break;
    }

    if (handle_frame->callback_execution ==
        kEsfButtonManagerNotificationCallbackStateEnable) {
      // Execute the callback corresponding to the button ID.
      EsfButtonManagerHandleData *handle_data = EsfButtonManagerGetHandleData(
          button_id, handle_frame->handle_data_list);
      if (handle_data != (EsfButtonManagerHandleData *)NULL) {
        EsfButtonManagerExecuteButtonCallback(state, handle_data);
      }
    }
  }
  if (pthread_mutex_unlock(&static_button_manager_mutex) != 0) {
    ESF_BUTTON_MANAGER_ERROR("Mutex unlock error.");
  }

  return;
}

static bool EsfButtonManagerPreProcessExecuteButtonEvent(
    EsfButtonManagerCommand *command) {
  if (command == (EsfButtonManagerCommand *)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Invalid parameter. command is null.");
    return false;
  }

  bool result = true;
  EsfButtonManagerButtonData *button_data = command->data;
  if (button_data != (EsfButtonManagerButtonData *)NULL) {
    EsfButtonManagerButtonStateType curent_state = button_data->state;
    EsfButtonManagerButtonStateType next_state = command->state;
    // Do not exec first released process.
    // clang-format off
    if ((curent_state == kEsfButtonManagerButtonStateTypeUnknown) && (next_state == kEsfButtonManagerButtonStateTypeReleased)) {
      // clang-format on
      result = false;
    }
    // Update Button State
    button_data->state = next_state;

  } else {
    result = false;
  }

  return result;
}

static void EsfButtonManagerExecuteButtonEvent(
    EsfButtonManagerButtonData *button_data) {
  if (button_data == (EsfButtonManagerButtonData *)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Invalid parameter. button_data is null.");
    return;
  }

  struct timespec time = {.tv_sec = 1, .tv_nsec = 0};
  switch (button_data->state) {
    case kEsfButtonManagerButtonStateTypePressed:
      if (UtilityTimerStart(button_data->timer_handle, &time,
                            kUtilityTimerRepeat) != kUtilityTimerOk) {
        ESF_BUTTON_MANAGER_ERROR("Failed to UtilityTimerStart.");
      }

      // Execute the pressed notification callback.
      EsfButtonManagerExecuteButtonCallbackAllHandles(button_data->button_id,
                                                      button_data->state);
      break;

    case kEsfButtonManagerButtonStateTypeReleased:
      if (UtilityTimerStop(button_data->timer_handle) != kUtilityTimerOk) {
        ESF_BUTTON_MANAGER_ERROR("Failed to UtilityTimerStop.");
      }

      // Execute the released notification callback.
      EsfButtonManagerExecuteButtonCallbackAllHandles(button_data->button_id,
                                                      button_data->state);
      break;

    case kEsfButtonManagerButtonStateTypeLongPressed:
      // Execute the long pressed notification callback.
      EsfButtonManagerExecuteButtonCallbackAllHandles(button_data->button_id,
                                                      button_data->state);
      break;

    default:
      ESF_BUTTON_MANAGER_ERROR("Invalid button state type.");
      break;
  }

  return;
}

static void EsfButtonManagerCancelRemainedTimer(void) {
  if (static_button_data_list == (EsfButtonManagerList *)NULL) {
    ESF_BUTTON_MANAGER_ERROR(
        "Invalid parameter. static_button_data_list is null.");
    return;
  }

  EsfButtonManagerCell *button_cell = (EsfButtonManagerCell *)NULL;
  for (int32_t i = 0; i < static_button_data_list->size; i++) {
    if (i == 0) {
      button_cell = EsfButtonManagerGetFrontCell(static_button_data_list);
    } else {
      button_cell = EsfButtonManagerGetNextCell(button_cell);
    }
    if (button_cell == (EsfButtonManagerCell *)NULL) {
      ESF_BUTTON_MANAGER_ERROR("button_cell is null.");
      return;
    }

    EsfButtonManagerButtonData *button_data =
        (EsfButtonManagerButtonData *)(button_cell->data);
    if (button_data == (EsfButtonManagerButtonData *)NULL) {
      ESF_BUTTON_MANAGER_ERROR("button_data is null.");
      return;
    }

    if ((button_data->state == kEsfButtonManagerButtonStateTypePressed) ||
        (button_data->state == kEsfButtonManagerButtonStateTypeLongPressed)) {
      if (UtilityTimerStop(button_data->timer_handle) != kUtilityTimerOk) {
        ESF_BUTTON_MANAGER_ERROR("Failed to UtilityTimerStop.");
      }
    }
  }

  return;
}

static void EsfButtonManagerButtonDetectionCallback(PlButtonStatus status,
                                                    void *private_data) {
  EsfButtonManagerCommand command = BUTTON_MANAGER_COMMAND_INITIALIZER;
  command.type = kEsfButtonManagerCommandTypeExecButtonEvent;
  if (status == kPlButtonStatusPressed) {
    command.state = kEsfButtonManagerButtonStateTypePressed;
  } else if (status == kPlButtonStatusReleased) {
    command.state = kEsfButtonManagerButtonStateTypeReleased;
  } else {
    ESF_BUTTON_MANAGER_ERROR("Invalid button state type.");
    return;
  }
  command.data = (EsfButtonManagerButtonData *)private_data;

  int32_t sent_size = 0;
  if (UtilityMsgSend(static_button_manager_message_handle, (void *)&command,
                     sizeof(command), BUTTON_MANAGER_MESSAGE_PRIORITY,
                     &sent_size) != kUtilityMsgOk) {
    ESF_BUTTON_MANAGER_ERROR("Failed to UtilityMsgSend.");
  }

  return;
}

static void EsfButtonManagerTimerCallback(void *private_data) {
  EsfButtonManagerCommand command = BUTTON_MANAGER_COMMAND_INITIALIZER;
  command.type = kEsfButtonManagerCommandTypeExecButtonEvent;
  command.state = kEsfButtonManagerButtonStateTypeLongPressed;
  command.data = (EsfButtonManagerButtonData *)private_data;

  int32_t sent_size = 0;
  if (UtilityMsgSend(static_button_manager_message_handle, (void *)&command,
                     sizeof(command), BUTTON_MANAGER_MESSAGE_PRIORITY,
                     &sent_size) != kUtilityMsgOk) {
    ESF_BUTTON_MANAGER_ERROR("Failed to UtilityMsgSend.");
  }

  return;
}

static void *EsfButtonManagerThread(void *arg) {
  EsfButtonManagerCommand command = BUTTON_MANAGER_COMMAND_INITIALIZER;
  int32_t recv_size = 0;

  while (1) {
    if (UtilityMsgRecv(static_button_manager_message_handle, (void *)&command,
                       sizeof(command), BUTTON_MANAGER_TIMEOUT,
                       &recv_size) == kUtilityMsgOk) {
      if (command.type == kEsfButtonManagerCommandTypeExecButtonEvent) {
        if (EsfButtonManagerPreProcessExecuteButtonEvent(&command)) {
          EsfButtonManagerExecuteButtonEvent(command.data);
        }

      } else if (command.type == kEsfButtonManagerCommandTypeCloseThread) {
        // If the timer is closed during a long press, the timer will remain, so
        // cancel it.
        EsfButtonManagerCancelRemainedTimer();
        break;

      } else {
        ESF_BUTTON_MANAGER_ERROR("Invalid command type.");
      }
    } else {
      ESF_BUTTON_MANAGER_ERROR("Failed to UtilityMsgRecv.");
    }
  }

  pthread_exit((void *)NULL);
  return (void *)NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

EsfButtonManagerStatus EsfButtonManagerInitialize(void) {
  if (UtilityMsgOpen(&static_button_manager_message_handle,
                     BUTTON_MANAGER_QUEUE_SIZE,
                     sizeof(EsfButtonManagerCommand)) != kUtilityMsgOk) {
    ESF_BUTTON_MANAGER_ERROR("Failed to UtilityMsgOpen.");
    return kEsfButtonManagerStatusInternalError;
  }

  if (PlButtonInitialize() != kPlErrCodeOk) {
    ESF_BUTTON_MANAGER_ERROR("Failed to PlButtonInitialize.");
    return kEsfButtonManagerStatusInternalError;
  }
  static_is_pl_button_initialized = true;

  PlButtonInfo button_info;
  memset(&button_info, 0, sizeof(PlButtonInfo));
  if (PlButtonGetInfo(&button_info) != kPlErrCodeOk) {
    ESF_BUTTON_MANAGER_ERROR("Failed to PlButtonGetInfo.");
    return kEsfButtonManagerStatusInternalError;
  }

  if (EsfButtonManagerCreateButtonDataList(&button_info) !=
      kEsfButtonManagerStatusOk) {
    ESF_BUTTON_MANAGER_ERROR("Failed to create button data list.");
    return kEsfButtonManagerStatusInternalError;
  }

  if (EsfButtonManagerSetButtonDetectionCallback(&button_info) !=
      kEsfButtonManagerStatusOk) {
    ESF_BUTTON_MANAGER_ERROR("Failed to set button detection callback.");
    return kEsfButtonManagerStatusInternalError;
  }

  if (EsfButtonManagerSetTimerCallback(&button_info) !=
      kEsfButtonManagerStatusOk) {
    ESF_BUTTON_MANAGER_ERROR("Failed to set timer callback.");
    return kEsfButtonManagerStatusInternalError;
  }

  static_handle_list = EsfButtonManagerCreateList();
  if (static_handle_list == (EsfButtonManagerList *)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Failed to create list.");
    return kEsfButtonManagerStatusInternalError;
  }

  if (EsfButtonManagerCreateThread() != kEsfButtonManagerStatusOk) {
    ESF_BUTTON_MANAGER_ERROR("Failed to create thread.");
    return kEsfButtonManagerStatusInternalError;
  }

  return kEsfButtonManagerStatusOk;
}

EsfButtonManagerStatus EsfButtonManagerFinalize(void) {
  EsfButtonManagerStatus result = kEsfButtonManagerStatusOk;

  if (static_button_data_list != (EsfButtonManagerList *)NULL) {
    if (EsfButtonManagerUnsetButtonDetectionCallback() !=
        kEsfButtonManagerStatusOk) {
      result = kEsfButtonManagerStatusInternalError;
    }
  }

  if (static_is_thread_starting != false) {
    if (EsfButtonManagerDestroyThread() != kEsfButtonManagerStatusOk) {
      result = kEsfButtonManagerStatusInternalError;
    } else {
      // Free the stack when the thread terminates.
      free(static_button_manager_thread_stack);
      static_button_manager_thread_stack = (void *)NULL;
    }
  }

  if (static_handle_list != (EsfButtonManagerList *)NULL) {
    // When Finalize is called, the number of elements in the list is 0, so
    // the list is simply discarded.
    if (EsfButtonManagerDestroyList(&static_handle_list) !=
        kEsfButtonManagerStatusOk) {
      result = kEsfButtonManagerStatusInternalError;
    }
  }

  if (static_button_data_list != (EsfButtonManagerList *)NULL) {
    // The function UtilityTimerDelete() must stop the timer first, so it is
    // necessary to accept the termination command beforehand using a thread.
    if (EsfButtonManagerUnsetTimerCallback() != kEsfButtonManagerStatusOk) {
      result = kEsfButtonManagerStatusInternalError;
    }
    // In order to discard the remaining timer, we use the button data list.
    // Therefore, it is destroyed after the thread finishes.
    if (EsfButtonManagerDestroyButtonDataList() != kEsfButtonManagerStatusOk) {
      result = kEsfButtonManagerStatusInternalError;
    }
  }

  if (static_is_pl_button_initialized != false) {
    if (PlButtonFinalize() != kPlErrCodeOk) {
      result = kEsfButtonManagerStatusInternalError;
    }
    static_is_pl_button_initialized = false;
  }

  if (static_button_manager_message_handle != BUTTON_MANAGER_MESSAGE_INIT) {
    if (UtilityMsgClose(static_button_manager_message_handle) !=
        kUtilityMsgOk) {
      result = kEsfButtonManagerStatusInternalError;
    } else {
      static_button_manager_message_handle = BUTTON_MANAGER_MESSAGE_INIT;
    }
  }

  return result;
}

EsfButtonManagerStatus EsfButtonManagerCreateHandle(
    EsfButtonManagerHandle *handle) {
  // Parameter check.
  if (handle == (EsfButtonManagerHandle *)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Invalid parameter. Handle is null.");
    return kEsfButtonManagerStatusParamError;
  }

  if (pthread_mutex_lock(&static_button_manager_mutex) != 0) {
    ESF_BUTTON_MANAGER_ERROR("Mutex lock error.");
    return kEsfButtonManagerStatusInternalError;
  }
  if (static_handle_list == (EsfButtonManagerList *)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Invalid parameter. static_handle_list is null.");
    pthread_mutex_unlock(&static_button_manager_mutex);
    return kEsfButtonManagerStatusParamError;
  }

  if (static_handle_list->size >= CONFIG_ESF_BUTTON_MANAGER_HANDLE_MAX_NUM) {
    ESF_BUTTON_MANAGER_ERROR(
        "Resource error. The maximum number of handles has been reached.");
    pthread_mutex_unlock(&static_button_manager_mutex);
    return kEsfButtonManagerStatusResourceError;
  }

  // Create a handle.
  EsfButtonManagerHandleFrame *handle_frame =
      EsfButtonManagerCreateHandleFrame();
  if (handle_frame == (EsfButtonManagerHandleFrame *)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Failed to create handle frame.");
    pthread_mutex_unlock(&static_button_manager_mutex);
    return kEsfButtonManagerStatusInternalError;
  }

  EsfButtonManagerCell *handle_cell =
      EsfButtonManagerCreateCell((void *)handle_frame);
  if (handle_cell == (EsfButtonManagerCell *)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Failed to cell.");
    EsfButtonManagerDestroyHandleFrame(handle_frame);
    pthread_mutex_unlock(&static_button_manager_mutex);
    return kEsfButtonManagerStatusInternalError;
  }
  if (EsfButtonManagerPushCell(handle_cell, static_handle_list) !=
      kEsfButtonManagerStatusOk) {
    ESF_BUTTON_MANAGER_ERROR("Failed to push cell.");
    EsfButtonManagerDestroyHandleFrame(handle_frame);
    EsfButtonManagerDestroyCell(&handle_cell);
    pthread_mutex_unlock(&static_button_manager_mutex);
    return kEsfButtonManagerStatusInternalError;
  }

  *handle = (EsfButtonManagerHandle)handle_cell;
  if (pthread_mutex_unlock(&static_button_manager_mutex) != 0) {
    ESF_BUTTON_MANAGER_ERROR("Mutex unlock error.");
    EsfButtonManagerDestroyHandleFrame(handle_frame);
    (void)EsfButtonManagerRemoveCell(&handle_cell, static_handle_list);
    *handle = (EsfButtonManagerHandle)NULL;
    return kEsfButtonManagerStatusInternalError;
  }

  return kEsfButtonManagerStatusOk;
}

EsfButtonManagerStatus EsfButtonManagerDestroyHandle(
    EsfButtonManagerHandle handle) {
  // Parameter check.
  if (handle == (EsfButtonManagerHandle)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Invalid parameter. Handle is null.");
    return kEsfButtonManagerStatusParamError;
  }

  if (pthread_mutex_lock(&static_button_manager_mutex) != 0) {
    ESF_BUTTON_MANAGER_ERROR("Mutex lock error.");
    return kEsfButtonManagerStatusInternalError;
  }
  EsfButtonManagerCell *handle_cell = (EsfButtonManagerCell *)handle;
  if (EsfButtonManagerVerifyCell(handle_cell, static_handle_list) !=
      kEsfButtonManagerStatusOk) {
    ESF_BUTTON_MANAGER_ERROR("Handle error. The handle is invalid.");
    pthread_mutex_unlock(&static_button_manager_mutex);
    return kEsfButtonManagerStatusHandleError;
  }

  // If there is a notification callback registration, cancel it.
  EsfButtonManagerHandleFrame *handle_frame =
      (EsfButtonManagerHandleFrame *)(handle_cell->data);
  if (EsfButtonManagerDestroyHandleFrame(handle_frame) !=
      kEsfButtonManagerStatusOk) {
    ESF_BUTTON_MANAGER_ERROR("Failed to destroy handle frame.");
    pthread_mutex_unlock(&static_button_manager_mutex);
    return kEsfButtonManagerStatusInternalError;
  }
  if (EsfButtonManagerRemoveCell(&handle_cell, static_handle_list) !=
      kEsfButtonManagerStatusOk) {
    ESF_BUTTON_MANAGER_ERROR("Failed to remove cell.");
    pthread_mutex_unlock(&static_button_manager_mutex);
    return kEsfButtonManagerStatusInternalError;
  }
  if (pthread_mutex_unlock(&static_button_manager_mutex) != 0) {
    ESF_BUTTON_MANAGER_ERROR("Mutex unlock error.");
    return kEsfButtonManagerStatusInternalError;
  }

  return kEsfButtonManagerStatusOk;
}

EsfButtonManagerStatus EsfButtonManagerRegisterNotificationCallback(
    uint32_t button_id, int32_t min_second, int32_t max_second,
    const EsfButtonManagerCallback callback, void *user_data,
    EsfButtonManagerButtonStateType type, EsfButtonManagerHandle handle) {
  // Parameter check.
  // clang-format off
  if ((callback == (EsfButtonManagerCallback)NULL) || (handle == (EsfButtonManagerHandle)NULL)) {
    // clang-format on
    ESF_BUTTON_MANAGER_ERROR(
        "Invalid parameter. Callback is null or handle is null. callback=%p "
        "handle=%p",
        callback, handle);
    return kEsfButtonManagerStatusParamError;
  }
  if (EsfButtonManagerGetButtonData(button_id) ==
      (EsfButtonManagerButtonData *)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Failed to get button data.");
    return kEsfButtonManagerStatusParamError;
  }
  // clang-format off
  if (EsfButtonManagerVerifyNotificationCallbackParams(min_second, max_second, type) != kEsfButtonManagerStatusOk) {
    // clang-format on
    ESF_BUTTON_MANAGER_ERROR(
        "Invalid parameter. Please review the parameters. min_second=%d "
        "max_second=%d type=%d",
        min_second, max_second, type);
    return kEsfButtonManagerStatusParamError;
  }

  if (pthread_mutex_lock(&static_button_manager_mutex) != 0) {
    ESF_BUTTON_MANAGER_ERROR("Mutex lock error.");
    return kEsfButtonManagerStatusInternalError;
  }
  EsfButtonManagerCell *handle_cell = (EsfButtonManagerCell *)handle;
  if (EsfButtonManagerVerifyCell(handle_cell, static_handle_list) !=
      kEsfButtonManagerStatusOk) {
    ESF_BUTTON_MANAGER_ERROR("Handle error. The handle is invalid.");
    pthread_mutex_unlock(&static_button_manager_mutex);
    return kEsfButtonManagerStatusHandleError;
  }
  EsfButtonManagerHandleFrame *handle_frame =
      (EsfButtonManagerHandleFrame *)(handle_cell->data);
  if (handle_frame == (EsfButtonManagerHandleFrame *)NULL) {
    ESF_BUTTON_MANAGER_ERROR("handle_frame is null.");
    pthread_mutex_unlock(&static_button_manager_mutex);
    return kEsfButtonManagerStatusInternalError;
  }
  if (handle_frame->callback_execution !=
      kEsfButtonManagerNotificationCallbackStateDisable) {
    ESF_BUTTON_MANAGER_ERROR(
        "Transition error. Execution of the callback is in an enable state.");
    pthread_mutex_unlock(&static_button_manager_mutex);
    return kEsfButtonManagerStatusStateTransitionError;
  }

  // clang-format off
  EsfButtonManagerHandleData *handle_data = EsfButtonManagerGetOrPrepareNewHandleData(button_id, handle_frame->handle_data_list);
  // clang-format on
  if (handle_data == (EsfButtonManagerHandleData *)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Failed to get or prepare new handle data.");
    pthread_mutex_unlock(&static_button_manager_mutex);
    return kEsfButtonManagerStatusInternalError;
  }

  EsfButtonManagerStatus result = kEsfButtonManagerStatusInternalError;
  if (type == kEsfButtonManagerButtonStateTypePressed) {
    result = EsfButtonManagerRegisterPressedCallbackInfo(
        callback, user_data, handle_data->pressed_list);
  } else if (type == kEsfButtonManagerButtonStateTypeReleased) {
    result = EsfButtonManagerRegisterReleasedCallbackInfo(
        min_second, max_second, callback, user_data,
        handle_data->released_list);
  } else if (type == kEsfButtonManagerButtonStateTypeLongPressed) {
    result = EsfButtonManagerRegisterLongPressedCallbackInfo(
        min_second, callback, user_data, handle_data->long_pressed_list);
  }
  if (result != kEsfButtonManagerStatusOk) {
    ESF_BUTTON_MANAGER_ERROR("Failed to register callback info.");
    pthread_mutex_unlock(&static_button_manager_mutex);
    return result;
  }
  if (pthread_mutex_unlock(&static_button_manager_mutex) != 0) {
    ESF_BUTTON_MANAGER_ERROR("Mutex unlock error.");
    return kEsfButtonManagerStatusInternalError;
  }

  return kEsfButtonManagerStatusOk;
}

EsfButtonManagerStatus EsfButtonManagerUnregisterNotificationCallback(
    uint32_t button_id, EsfButtonManagerButtonStateType type,
    EsfButtonManagerHandle handle) {
  // Parameter check.
  if (handle == (EsfButtonManagerHandle)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Invalid parameter. Handle is null.");
    return kEsfButtonManagerStatusParamError;
  }
  if (EsfButtonManagerGetButtonData(button_id) ==
      (EsfButtonManagerButtonData *)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Failed to get button data.");
    return kEsfButtonManagerStatusParamError;
  }

  if (pthread_mutex_lock(&static_button_manager_mutex) != 0) {
    ESF_BUTTON_MANAGER_ERROR("Mutex lock error.");
    return kEsfButtonManagerStatusInternalError;
  }
  EsfButtonManagerCell *handle_cell = (EsfButtonManagerCell *)handle;
  if (EsfButtonManagerVerifyCell(handle_cell, static_handle_list) !=
      kEsfButtonManagerStatusOk) {
    ESF_BUTTON_MANAGER_ERROR("Handle error. The handle is invalid.");
    pthread_mutex_unlock(&static_button_manager_mutex);
    return kEsfButtonManagerStatusHandleError;
  }
  EsfButtonManagerHandleFrame *handle_frame =
      (EsfButtonManagerHandleFrame *)(handle_cell->data);
  if (handle_frame == (EsfButtonManagerHandleFrame *)NULL) {
    ESF_BUTTON_MANAGER_ERROR("handle_frame is null.");
    pthread_mutex_unlock(&static_button_manager_mutex);
    return kEsfButtonManagerStatusInternalError;
  }
  if (handle_frame->callback_execution !=
      kEsfButtonManagerNotificationCallbackStateDisable) {
    ESF_BUTTON_MANAGER_ERROR(
        "Transition error. Execution of the callback is in an enable state.");
    pthread_mutex_unlock(&static_button_manager_mutex);
    return kEsfButtonManagerStatusStateTransitionError;
  }
  // clang-format off
  EsfButtonManagerHandleData *handle_data = EsfButtonManagerGetHandleData(button_id, handle_frame->handle_data_list);
  // clang-format on
  if (handle_data == (EsfButtonManagerHandleData *)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Failed to get handle data.");
    pthread_mutex_unlock(&static_button_manager_mutex);
    return kEsfButtonManagerStatusResourceError;
  }

  EsfButtonManagerStatus result = kEsfButtonManagerStatusInternalError;
  result = EsfButtonManagerRemoveAllCallbackInfoFromSpecifyList(type,
                                                                handle_data);
  if (result != kEsfButtonManagerStatusOk) {
    ESF_BUTTON_MANAGER_ERROR("Failed to remove callback info.");
    pthread_mutex_unlock(&static_button_manager_mutex);
    return result;
  }

  int32_t total_lists_size = handle_data->pressed_list->size +
                             handle_data->released_list->size +
                             handle_data->long_pressed_list->size;
  if (total_lists_size == 0) {
    // clang-format off
    if (EsfButtonManagerRemoveHandleData(button_id, handle_frame->handle_data_list) != kEsfButtonManagerStatusOk) {
      // clang-format on
      ESF_BUTTON_MANAGER_ERROR("Failed to remove handle data.");
      pthread_mutex_unlock(&static_button_manager_mutex);
      return kEsfButtonManagerStatusInternalError;
    }
  }
  if (pthread_mutex_unlock(&static_button_manager_mutex) != 0) {
    ESF_BUTTON_MANAGER_ERROR("Mutex unlock error.");
    return kEsfButtonManagerStatusInternalError;
  }

  return kEsfButtonManagerStatusOk;
}

EsfButtonManagerStatus EsfButtonManagerSetCallbackExecutableState(
    EsfButtonManagerNotificationCallbackState flag,
    EsfButtonManagerHandle handle) {
  // Parameter check.
  if (handle == (EsfButtonManagerHandle)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Invalid parameter. Handle is null.");
    return kEsfButtonManagerStatusParamError;
  }

  if (pthread_mutex_lock(&static_button_manager_mutex) != 0) {
    ESF_BUTTON_MANAGER_ERROR("Mutex lock error.");
    return kEsfButtonManagerStatusInternalError;
  }
  EsfButtonManagerCell *handle_cell = (EsfButtonManagerCell *)handle;
  if (EsfButtonManagerVerifyCell(handle_cell, static_handle_list) !=
      kEsfButtonManagerStatusOk) {
    ESF_BUTTON_MANAGER_ERROR("Handle error. The handle is invalid.");
    pthread_mutex_unlock(&static_button_manager_mutex);
    return kEsfButtonManagerStatusHandleError;
  }
  EsfButtonManagerHandleFrame *handle_frame =
      (EsfButtonManagerHandleFrame *)(handle_cell->data);
  if (handle_frame == (EsfButtonManagerHandleFrame *)NULL) {
    ESF_BUTTON_MANAGER_ERROR("handle_frame is null.");
    pthread_mutex_unlock(&static_button_manager_mutex);
    return kEsfButtonManagerStatusInternalError;
  }
  if (handle_frame->callback_execution == flag) {
    ESF_BUTTON_MANAGER_ERROR(
        "Transition error. It is already transitioning to the specified "
        "state.");
    pthread_mutex_unlock(&static_button_manager_mutex);
    return kEsfButtonManagerStatusStateTransitionError;
  }

  handle_frame->callback_execution = flag;
  if (flag == kEsfButtonManagerNotificationCallbackStateEnable) {
    if (EsfButtonManagerCheckButtonState(handle_frame->handle_data_list) !=
        kEsfButtonManagerStatusOk) {
      ESF_BUTTON_MANAGER_ERROR("Failed to check button state.");
      pthread_mutex_unlock(&static_button_manager_mutex);
      return kEsfButtonManagerStatusInternalError;
    }
  } else {
    if (EsfButtonManagerResetLongPressedTime(handle_frame->handle_data_list) !=
        kEsfButtonManagerStatusOk) {
      ESF_BUTTON_MANAGER_ERROR("Failed to reset long pressed time.");
      pthread_mutex_unlock(&static_button_manager_mutex);
      return kEsfButtonManagerStatusInternalError;
    }
  }
  if (pthread_mutex_unlock(&static_button_manager_mutex) != 0) {
    ESF_BUTTON_MANAGER_ERROR("Mutex unlock error.");
    return kEsfButtonManagerStatusInternalError;
  }

  return kEsfButtonManagerStatusOk;
}

EsfButtonManagerState EsfButtonManagerGetState(void) {
  return static_button_manager_state;
}

void EsfButtonManagerSetState(EsfButtonManagerState state) {
  static_button_manager_state = state;
}

int32_t EsfButtonManagerGetNumberOfHandles(void) {
  if (static_handle_list == (EsfButtonManagerList *)NULL) {
    return 0;
  }
  return static_handle_list->size;
}
