/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESF_NETWORK_MANAGER_SRC_INCLUDE_NETWORK_MANAGER_NETWORK_MANAGER_RESOURCE_H_
#define ESF_NETWORK_MANAGER_SRC_INCLUDE_NETWORK_MANAGER_NETWORK_MANAGER_RESOURCE_H_

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#include "network_manager.h"
#include "porting_layer/include/pl.h"
#include "porting_layer/include/pl_network.h"

// This is the upper limit number of handles for which the handle type is
// "control".
#define ESF_NETWORK_MANAGER_HANDLE_TYPE_CONTROL_MAX (1)

// This is the upper limit number of handles for which the handle type is
// "information".
#define ESF_NETWORK_MANAGER_HANDLE_TYPE_INFORMATION_MAX (2)

// clang-format off
// This is the upper limit on the number of handles, regardless of handle type.
#define ESF_NETWORK_MANAGER_HANDLE_TYPE_MAX      \
  (ESF_NETWORK_MANAGER_HANDLE_TYPE_CONTROL_MAX + \
  ESF_NETWORK_MANAGER_HANDLE_TYPE_INFORMATION_MAX)
// clang-format on

// This is the upper limit on the number of handles for the entire network.
#define ESF_NETWORK_MANAGER_HANDLE_MAX \
  (kEsfNetworkManagerModeNum * ESF_NETWORK_MANAGER_HANDLE_TYPE_MAX)

// Defines an enumerated type that defines the IF type.
typedef enum EsfNetworkManagerInterfaceKind {
  // IF type Wi-Fi.
  kEsfNetworkManagerInterfaceKindWifi = 0,

  // IF type Ether.
  kEsfNetworkManagerInterfaceKindEther,

  // The number of definitions.
  kEsfNetworkManagerInterfaceKindNum,
} EsfNetworkManagerInterfaceKind;

// Defines an enumeration type that defines the initialization state of the
// Network.
typedef enum EsfNetworkManagerStatus {
  // An uninitialized state.
  kEsfNetworkManagerStatusUninit = 0,

  // An initialized state.
  kEsfNetworkManagerStatusInit,
} EsfNetworkManagerStatus;

// Defines an enumerated type that defines the network connection state.
typedef enum EsfNetworkManagerConnectStatus {
  // Disconnected state.
  kEsfNetworkManagerConnectStatusDisconnected = 0,

  // Connecting state.
  kEsfNetworkManagerConnectStatusConnecting,

  // Connected state.
  kEsfNetworkManagerConnectStatusConnected,

  // Disconnecting state.
  kEsfNetworkManagerConnectStatusDisconnecting,
} EsfNetworkManagerConnectStatus;

// A structure that defines network mode information.
typedef struct EsfNetworkManagerModeInfo {
  // Network connection status.
  EsfNetworkManagerConnectStatus connect_status;

  // Connection information Saved identification information.
  // false: not saved
  // true:Saved
  bool is_connect_info_saved;

  // Connection information storage information.
  // Saved when executing EsfNetworkManagerStart with
  // connection information parameters.
  EsfNetworkManagerOSInfo connect_info;

  // IP address information storage information.
  // When performing EsfNetworkManagerStart with DHCP specified,
  // saves the IP address information obtained by DHCP.
  // If it is Static or the DHCP address has not been obtained,
  // it will be a NULL character.
  EsfNetworkManagerIPInfo ip_info;

  // HAL system information.
  // It is maintained for each IF type.
  // When the connection mode is AccessPoint mode,
  // only Wi-Fi information is retained.
  PlNetworkSystemInfo *hal_system_info[kEsfNetworkManagerInterfaceKindNum];

  // This is the number of devices connected while operating
  // in AccessPoint mode.
  int32_t ap_connected_count;

  // Interface status.
  // Valid only in Normal mode and IF type Ether.
  // Updates when receiving events from HAL.
  EsfNetworkManagerStatusInfo status_info;

  // Network connection status.
  // Keep up-to-date information on the status to be notified
  // by registered callback functions.
  EsfNetworkManagerNotifyInfo notify_info;

  // Holds registered callback functions.
  EsfNetworkManagerNotifyInfoCallback callback;

  // Holds private data for registered callback functions.
  void *callback_private_data;
} EsfNetworkManagerModeInfo;

// A structure that defines the internal handle of the Network.
typedef struct EsfNetworkManagerHandleInternal {
  // Connection mode.
  EsfNetworkManagerMode mode;

  // Handle type.
  EsfNetworkManagerHandleType handle_type;

  // Valid/invalid information.
  // false: invalid
  // true: valid
  bool is_valid;

  // Number of references.
  // EsfNetworkManagerClose cannot be performed while it is being referenced.
  int32_t count;

  // Control usage information.
  // Information on whether the control API (EsfNetworkManagerStart,
  // EsfNetworkManagerStop) is being executed on the corresponding handle.
  // false: not running
  // true: running
  bool is_in_use_control;

  // A pointer to mode information.
  // Holds a pointer to mode information that corresponds
  // to the connection mode specified when acquiring the handle.
  EsfNetworkManagerModeInfo *mode_info;
} EsfNetworkManagerHandleInternal;

// Proxy port length.
#define ESF_NETWORK_MANAGER_PROXY_PORT_LEN (5 + 1)

// Data conversion definition when accessing ParameterStorageManager.
// External to internal convert define.
typedef enum EsfNetworkManagerConvertType {
  // No convert.
  kEsfNetworkManagerConvertTypeNoConvert = 0,

  // int32_t to String.
  kEsfNetworkManagerConvertTypeIntToString,
} EsfNetworkManagerConvertType;

// Structure for storing int32 type data.
typedef struct EsfNetworkManagerInt32Parameter {
  uint32_t size;
  int32_t data;
} EsfNetworkManagerInt32Parameter;

// Internal processing structure used when calling the Parameter Storage Manager
// API. Replaces the RAW data type.
typedef struct EsfNetworkManagerParameterInternal {
  EsfNetworkManagerParameter param;
  EsfNetworkManagerInt32Parameter wifi_sta_encryption;
  EsfNetworkManagerInt32Parameter ip_method;
  EsfNetworkManagerInt32Parameter netif_kind;
  EsfNetworkManagerInt32Parameter wifi_ap_encryption;
  EsfNetworkManagerInt32Parameter wifi_ap_channel;
  char proxy_port[ESF_NETWORK_MANAGER_PROXY_PORT_LEN];
} EsfNetworkManagerParameterInternal;

// """Initialize the Resource submodule.

// Initializes the Resource submodule that manages Network's
// internal resources.
// The Resource submodule stores resources in the private
// EsfNetworkManagerResource global variable (hereinafter referred to as
// resource variable). If the resource variable status is
// kEsfNetworkManagerStatusUninit, perform initialization processing. If it is
// not kEsfNetworkManagerStatusUninit, returns kEsfNetworkManagerResultSuccess
// without processing.

// Args:
//     info_total_num (int32_t): Specify the number of HAL system information.
//       Please specify the one obtained with
//       EsfNetworkManagerAccessorGetSystemInfo.
//     infos (PlNetworkSystemInfo *):
//       Specifies the HAL system information array.
//       Please specify the one obtained with
//       EsfNetworkManagerAccessorGetSystemInfo.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultResourceExhausted: Memory allocation failure.
//     kEsfNetworkManagerResultInternalError: Internal error.

// Note:
// """
EsfNetworkManagerResult EsfNetworkManagerResourceInit(
    int32_t info_total_num, PlNetworkSystemInfo *infos);

// """Exit the Resource submodule.

// Terminates the Resource submodule that manages Network's
// internal resources.
// The Resource submodule stores resources in the private
// EsfNetworkManagerResource global variable
// (hereinafter referred to as resource variable).
// If the resource variable status is kEsfNetworkManagerStatusUninit,
// the resource will be released.
// If kEsfNetworkManagerStatusUninit, returns
// kEsfNetworkManagerResultInternalError.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultInternalError: Internal error.

// Note:
// """
EsfNetworkManagerResult EsfNetworkManagerResourceDeinit(void);

// """Starts exclusive control of Resource.

// Starts exclusive control using the Mutex for
// the Resource managed by the Resource submodule.
// lock timed out by CONFIG_EXTERNAL_NETWORK_MANAGER_LOCKTIME.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultInternalError: Internal error.

// Note:
// """
EsfNetworkManagerResult EsfNetworkManagerResourceLockResource(void);

// """Starts exclusive control of Resource.

// Starts exclusive control using the Mutex for
// the Resource managed by the Resource submodule.
// No timed out.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultInternalError: Internal error.

// Note:
// """
EsfNetworkManagerResult EsfNetworkManagerResourceLockResourceNoTimeout(void);

// """Ends exclusive control of the Resource.

// Ends exclusive control of the Resource.
// Terminate exclusive control using the Mutex for
// the Resource managed by the Resource submodule.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultInternalError: Internal error.

// Note:
// """
EsfNetworkManagerResult EsfNetworkManagerResourceUnlockResource(void);

// """Activate the handle.

// Enables and returns one invalid handle held by the Resource submodule.

// Args:
//     mode (EsfNetworkManagerMode): Specify the network connection mode.
//     handle_type (EsfNetworkManagerHandleType): Specify the handle type.
//     handle (EsfNetworkManagerHandle *): The activated handle.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultStatusUnexecutable: Uninitialized state.
//     kEsfNetworkManagerResultInvalidParameter: Input parameter error.
//     kEsfNetworkManagerResultResourceExhausted: Maximum number of handles
//     exceeded. kEsfNetworkManagerResultInternalError: Internal error.

// Note:
// """
EsfNetworkManagerResult EsfNetworkManagerResourceNewHandle(
    EsfNetworkManagerMode mode, EsfNetworkManagerHandleType handle_type,
    EsfNetworkManagerHandle *handle);

// """Disable the handle.

// Disables the specified handle.

// Args:
//     handle (EsfNetworkManagerHandle): Handle to disable.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultStatusUnexecutable: Uninitialized state.
//     kEsfNetworkManagerResultInvalidParameter: Input parameter error.
//     kEsfNetworkManagerResultNotFound:Specified handle is not reserved.
//     kEsfNetworkManagerResultFailedPrecondition:Specified handle in use.
//     kEsfNetworkManagerResultInternalError: Internal error.

// Note:
// """
EsfNetworkManagerResult EsfNetworkManagerResourceDeleteHandle(
    EsfNetworkManagerHandle handle);

// """Refer to handle.

// Gets the object of the specified handle.

// Args:
//     is_control (bool): Control API execution flag.
//     handle (EsfNetworkManagerHandle): The handle to reference.
//     handle_internal (EsfNetworkManagerHandleInternal **):
//       This is the actual handle.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultStatusUnexecutable: Uninitialized state.
//     kEsfNetworkManagerResultInvalidParameter: Input parameter error.
//     kEsfNetworkManagerResultNotFound:Specified handle is not reserved.
//     kEsfNetworkManagerResultFailedPrecondition:
//       Specified handle is in use for control operation.
//     kEsfNetworkManagerResultInternalError: Internal error.

// Note:
// """
EsfNetworkManagerResult EsfNetworkManagerResourceOpenHandle(
    bool is_control, EsfNetworkManagerHandle handle,
    EsfNetworkManagerHandleInternal **handle_internal);

// """Ends the handle reference.

// Terminates the reference to the specified handle entity.

// Args:
//     is_control (bool): Control API execution flag.
//     handle (EsfNetworkManagerHandle): The handle to end the reference to.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultStatusUnexecutable: Uninitialized state.
//     kEsfNetworkManagerResultInvalidParameter: Input parameter error.
//     kEsfNetworkManagerResultNotFound:Specified handle is not reserved.
//     kEsfNetworkManagerResultInternalError: Internal error.

// Note:
// """
EsfNetworkManagerResult EsfNetworkManagerResourceCloseHandle(
    bool is_control, EsfNetworkManagerHandle handle);

// """Checks whether the state matches the parameter specified state.

// Checks whether the specified state and internal initialization state match.

// Args:
//     status (EsfNetworkManagerStatus): The state to be checked.

// Returns:
//     Returns the result of the status match check.

// Yields:
//     true:State match.
//     false:State mismatch.

// Note:
// """
bool EsfNetworkManagerResourceCheckStatus(EsfNetworkManagerStatus status);

// """Get the IF name used in Network.

// Gets the IF name from Network's internal resources.
// If the applicable IF is not defined in HAL, set NULL to the IF name.

// Args:
//     if_name_ether (char **):
//       This is the IF name used in Normal mode IF type Ether.
//     if_name_wifist (char **):
//       This is the IF name used in Normal mode IF type WiFi.
//     if_name_wifiap (char **):
//       This is the IF name used in AccessPoint mode.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultInternalError: Internal error.

// Note:
// """
EsfNetworkManagerResult EsfNetworkManagerResourceGetIfname(
    char **if_name_ether, char **if_name_wifist, char **if_name_wifiap);

// """Get the network connection information work area.

// Retrieve the work area from the internal resources of the Network Manager.
// Please call this while holding the lock.
// Maintain the lock while using the work area.

// Args:
//     mask_work (EsfNetworkManagerParameterMask **):
//       This is a mask structure for network connection information.
//     parameter_work (EsfNetworkManagerParameter **):
//       Network connection information.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultStatusUnexecutable: Uninitialized state.
//     kEsfNetworkManagerResultInternalError: Internal error.

// Note:
// """
EsfNetworkManagerResult EsfNetworkManagerResourceGetParameterWork(
    EsfNetworkManagerParameterMask **mask_work,
    EsfNetworkManagerParameter **parameter_work);

// Retrieve the work area from the internal resources of the Network Manager.
// Please call this while holding the lock.
// Maintain the lock while using the work area.

// Args:
//     parameter_internal_work (EsfNetworkManagerParameterInternal **):
//       Network connection internal information.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultStatusUnexecutable: Uninitialized state.
//     kEsfNetworkManagerResultInternalError: Internal error.

// Note:
// """
EsfNetworkManagerResult EsfNetworkManagerResourceGetParameterInternalWork(
    EsfNetworkManagerParameterInternal **parameter_internal_work);

#endif  // ESF_NETWORK_MANAGER_SRC_INCLUDE_NETWORK_MANAGER_NETWORK_MANAGER_RESOURCE_H_
